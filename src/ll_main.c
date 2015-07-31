//
//  main.c
//  mulle-aba-test
//
//  Created by Nat! on 19.03.15.
//  Copyright (c) 2015 Mulle kybernetiK. All rights reserved.
//

//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//
//  Redistributions of source code must retain the above copyright notice, this
//  list of conditions and the following disclaimer.
//
//  Redistributions in binary form must reproduce the above copyright notice,
//  this list of conditions and the following disclaimer in the documentation
//  and/or other materials provided with the distribution.
//
//  Neither the name of Mulle kybernetiK nor the names of its contributors
//  may be used to endorse or promote products derived from this software
//  without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
//  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
//  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
//  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
//  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
//  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
//  POSSIBILITY OF SUCH DAMAGE.
//
#include "mulle_aba_storage.h"
#include "mulle_aba.h"
#include <stdio.h>
#include <errno.h>
#include <pthread.h>


#define PROGRESS  0
#define FOREVER   1

#if DEBUG_PRINT
extern void   mulle_aba_print( void);
#else
# define mulle_aba_print()
#endif

static pthread_key_t   timestamp_thread_key;
char  *pthread_name( void);

#pragma mark -
#pragma mark track allocations

#include "pointer_array.h"


struct _pointer_array    allocations;
pthread_mutex_t          alloc_lock;

//void  *test_realloc( void *q, size_t size)
//{
//   void           *p;
//   unsigned int   i;
//   
//   p = realloc( q, size);
//   if( ! p)
//      return( p);
//
//   if( pthread_mutex_lock( &alloc_lock))
//      abort();
//   
//   if( q)
//   {
//      if( p != q)
//      {
//         i = _pointer_array_index( &allocations, q);  // analayzer it's ok, just a pointer comparison
//         assert( i != -1);
//         _pointer_array_set( &allocations, i, p);
//      }
//   }
//   else
//      _pointer_array_add( &allocations, p, realloc);
//   pthread_mutex_unlock( &alloc_lock);
//
//   return( p);
//}


void  *test_calloc( size_t n, size_t size)
{
   void   *p;
   
   p = calloc( n, size);
   if( ! p)
   {
#if TRACE
      abort();
#endif
      return( p);
   }
   
   if( pthread_mutex_lock( &alloc_lock))
      abort();
   _pointer_array_add( &allocations, p, realloc);
   pthread_mutex_unlock( &alloc_lock);
#if TRACE
   fprintf( stderr,  "%s: *** alloc( %p) ***\n", pthread_name(), p);
#endif
   return( p);
}


void  test_free( void *p)
{
   unsigned int   i;
   
   if( ! p)
      return;
   
#if TRACE || TRACE_FREE
   fprintf( stderr,  "%s: *** free( %p) ***\n", pthread_name(), p);
#endif

   if( pthread_mutex_lock( &alloc_lock))
      abort();
   
   i = _pointer_array_index( &allocations, p);
   assert( i != -1);  // if assert, this is a double free or not a malloc block
   _pointer_array_set( &allocations, i, NULL);
   
   pthread_mutex_unlock( &alloc_lock);
   
   free( p);
}


#pragma mark -
#pragma mark global variables

static struct _mulle_aba_linked_list   list;     // common
static mulle_atomic_ptr_t              alloced;  // common


#pragma mark -
#pragma mark reset allocator between tests

static void   reset_memory()
{
   struct  _pointer_array_enumerator   rover;
   void                                *p;

#if TRACE
   fprintf( stderr, "\n================================================\n");
#endif

   rover = _pointer_array_enumerate( &allocations);
   while( (p = _pointer_array_enumerator_next( &rover)) != (void *) -1)
   {
      if( p)
      {
         fprintf( stdout, "*");
#if TRACE
         fprintf( stderr, "%s: leak %p\n", pthread_name(), p);
#endif
#if DEBUG
         abort();
#endif
         free( p);
      }
   }
   _pointer_array_enumerator_done( &rover);

   _pointer_array_done( &allocations, free);
   memset( &allocations, 0, sizeof( allocations));
   
   memset( &alloced, 0, sizeof( alloced));
   memset( &list, 0, sizeof( list));
}



#pragma mark -
#pragma mark run test

static void    run_thread_gc_free_list_test( void)
{
   struct _mulle_aba_free_entry   *entry;
   unsigned long                  i;
   void                           *thread;
   
   thread = pthread_name();
   
   for( i = 0; i < (1 + (rand() % 100000)); i++)
   {
      entry = (void *) _mulle_aba_linked_list_remove_one( &list);
      if( entry)
      {
#if TRACE
         fprintf( stderr, "%s: reused %p (%p) from %p\n", pthread_name(), entry, entry->_next, &list);
#endif
         assert( ! entry->_next);
      }
      else
      {
         entry = test_calloc( 1, sizeof( *entry));
         _mulle_atomic_increment_pointer( &alloced);
#if TRACE            
         fprintf( stderr, "%s: allocated %p (%p)\n", pthread_name(), entry, entry->_next);
#endif
      }
      
      assert( entry);
      entry->_pointer = (void *) i;
      entry->_owner   = thread;

      _mulle_aba_linked_list_add( &list, &entry->_link);
   }
}


void  multi_threaded_test_each_thread()
{
#if PROGRESS
   fprintf( stdout,  "."); fflush( stdout);
#endif
   run_thread_gc_free_list_test();
}


static void   _wait_around( mulle_atomic_ptr_t *n_threads)
{
   // wait for all threads to materialize
   _mulle_atomic_decrement_pointer( n_threads);
   while( _mulle_atomic_read_pointer( n_threads) != 0)
      sched_yield();
}


struct thread_info
{
   char                  name[ 64];
   mulle_atomic_ptr_t    *n_threads;
};


static void   *run_test( struct thread_info *info)
{
   pthread_setspecific( timestamp_thread_key, strdup( info->name));

   _wait_around( info->n_threads);
   multi_threaded_test_each_thread();

   return( NULL);
}


static void   finish_test( void)
{
   struct _mulle_aba_free_entry   *entry;
   // remove all from list, so any leak means trouble
   
   while( _mulle_atomic_decrement_pointer( &alloced))
   {
      entry = (void *) _mulle_aba_linked_list_remove_one( &list);
      assert( entry);
      test_free( entry);
   }
}


void  multi_threaded_test( intptr_t n)
{
   int                  i;
   pthread_t            *threads;
   struct thread_info   *info;
   mulle_atomic_ptr_t   n_threads;
   
#if TRACE
   fprintf( stderr, "////////////////////////////////\n");
   fprintf( stderr, "multi_threaded_test( %ld) starts\n", n); 
#endif
   threads = alloca( n * sizeof( pthread_t));
   assert( threads);
   
   n_threads._nonatomic = (void *) n;
   info = alloca( sizeof(struct thread_info) * n);

   for( i = 1; i < n; i++)
   {
      info[ i].n_threads = &n_threads;
      sprintf( info[ i].name, "thread #%d", i);
      
      if( pthread_create( &threads[ i], NULL, (void *) run_test, &info[ i]))
         abort();
   }
   
   info[ 0].n_threads = &n_threads;
   sprintf( info[ 0].name, "thread #%d", 0);
   run_test( &info[ 0]);

   for( i = 1; i < n; i++)
      if( pthread_join( threads[ i], NULL))
      {
         perror( "pthread_join");
         abort();
      }
   
   finish_test();
   
#if TRACE
   fprintf( stderr, "%s: multi_threaded_test( %ld) ends\n", pthread_name(), n);
#endif
}


char  *pthread_name( void)
{
   return( pthread_getspecific( timestamp_thread_key));
}


#ifdef __APPLE__
#include <sys/resource.h>

__attribute__((constructor))
static void  __enable_core_dumps(void)
{
    struct rlimit   limit;

    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
}
#endif


int   main(int argc, const char * argv[])
{
   unsigned int   i;
   unsigned int   j;
   int            rval;
   
   srand( (unsigned int) time( NULL));
   
   rval = pthread_mutex_init( &alloc_lock, NULL);
   assert( ! rval);
   
   rval = pthread_key_create( &timestamp_thread_key, free);
   assert( ! rval);
   
   rval = pthread_setspecific( timestamp_thread_key, strdup( "main"));
   assert( ! rval);
   
#if TRACE
   fprintf( stderr, "%s\n", pthread_name());
#endif
   

forever:
   reset_memory();

   //
   // if there are leaks anywhere, it will assert in
   // _mulle_aba_storage_done which is called by reset_memory
   // eventually
   //
   
   for( i = 0; i < 400; i++)
   {
#if TRACE || PROGRESS
# if TRACE
      fprintf( stderr, "iteration %d\n", i);
# else
      fprintf( stdout, "iteration %d\n", i);
# endif
#endif
      for( j = 2; j <= 4; j += j)
      {
         multi_threaded_test( j);
         reset_memory();
      }
   }

#if FOREVER
   goto forever;
#endif

   pthread_mutex_destroy( &alloc_lock);

   return( 0);
}
