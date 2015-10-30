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
#include <mulle_thread/mulle_thread.h>
#include <stdio.h>
#include <errno.h>


#define PROGRESS  0
#define FOREVER   1

#if DEBUG_PRINT
extern void   mulle_aba_print( void);
#else
# define mulle_aba_print()
#endif

static mulle_thread_tss_t   timestamp_thread_key;
char  *mulle_aba_thread_name( void);

#pragma mark -
#pragma mark track allocations

#include "pointer_array.h"


struct _pointer_array    allocations;
mulle_thread_mutex_t          alloc_lock;

//void  *test_realloc( void *q, size_t size)
//{
//   void           *p;
//   unsigned int   i;
//   
//   p = realloc( q, size);
//   if( ! p)
//      return( p);
//
//   if( mulle_thread_mutex_lock( &alloc_lock))
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
//   mulle_thread_mutex_unlock( &alloc_lock);
//
//   return( p);
//}


void  *test_calloc( size_t n, size_t size)
{
   void   *p;
   
   p = calloc( n, size);
   if( ! p)
   {
#if MULLE_ABA_TRACE
      abort();
#endif
      return( p);
   }
   
   if( mulle_thread_mutex_lock( &alloc_lock))
      abort();
   _pointer_array_add( &allocations, p, realloc);
   mulle_thread_mutex_unlock( &alloc_lock);
#if MULLE_ABA_TRACE
   fprintf( stderr,  "%s: *** alloc( %p) ***\n", mulle_aba_thread_name(), p);
#endif
   return( p);
}


void  *test_realloc( void *p, size_t size)
{
   abort();  // unused in tests
}


void  test_free( void *p)
{
   unsigned int   i;
   
   if( ! p)
      return;
   
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
   fprintf( stderr,  "%s: *** free( %p) ***\n", mulle_aba_thread_name(), p);
#endif

   if( mulle_thread_mutex_lock( &alloc_lock))
      abort();
   
   i = _pointer_array_index( &allocations, p);
   assert( i != -1);  // if assert, this is a double free or not a malloc block
   _pointer_array_set( &allocations, i, NULL);
   
   mulle_thread_mutex_unlock( &alloc_lock);
   
   free( p);
}


#pragma mark -
#pragma mark global variables

static struct _mulle_allocator   test_allocator =
{
      test_calloc,
      test_realloc,
      test_free
};

static struct _mulle_aba   test_global;


#pragma mark -
#pragma mark reset allocator between tests

static void   reset_memory()
{
   extern void  mulle_aba_reset( void);
   struct  _pointer_array_enumerator   rover;
   void                                *p;

   mulle_aba_done();
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "\n================================================\n");
#endif

   rover = _pointer_array_enumerate( &allocations);
   while( (p = _pointer_array_enumerator_next( &rover)) != (void *) -1)
   {
      if( p)
      {
         fprintf( stdout, "*");
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: leak %p\n", mulle_aba_thread_name(), p);
#endif
         abort();
      }
   }
   _pointer_array_enumerator_done( &rover);

   _pointer_array_done( &allocations, free);
   memset( &allocations, 0, sizeof( allocations));

   _mulle_aba_init( &test_global, &test_allocator, sched_yield);
}


static void  fake_free( void *p)
{
#if MULLE_ABA_TRACE
   fprintf( stderr,  "\n%s: *** fake_free: %p ***\n", mulle_aba_thread_name(), p);
#endif
}

#pragma mark -
#pragma mark run GC

static void    run_gc_free_list_test( void)
{
   unsigned long   i;
   
   for( i = 0; i < 100000; i++)
      switch( rand() % 10)
      {
      case 0 :
      case 1 :
         if( mulle_aba_free( (void *) i, fake_free))
         {
            perror( "mulle_aba_free");
            abort();
         }
         break;

      case 2 :
         mulle_aba_checkin();
         break;
      
      default :
         sched_yield();
         break;
      }
}


static void    run_thread_gc_free_list_test( void)
{
   unsigned long   i;
   int             registered;
   
   registered = 1;
   for( i = 0; i < (1 + (rand() % 10000)); i++)
      switch( rand() % 20)
      {
      case 0 :
      case 1 :
      case 2 :
      case 3 :
      case 4 :
      case 5 :
      case 6 :
      case 7 :
         if( ! registered)
            continue;
#if MULLE_ABA_TRACE
         fprintf( stderr,  "\n------------------------------\n%s: (%lu) %p -> free block %p\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer(),
                 (void *) i);
#endif
         if( mulle_aba_free( (void *) i, fake_free))
         {
            perror( "mulle_aba_free");
            abort();
         }
#if MULLE_ABA_TRACE
            fprintf( stderr,  "\n%s: (%lu) %p <- freed block:%ld\n------------------------------\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
                    _mulle_aba_get_worldpointer(),
                    i);
         mulle_aba_print();
#endif
         break;

      case 17 :
      case 18 :
         if( ! registered)
            continue;
#if MULLE_ABA_TRACE
         fprintf( stderr,  "\n------------------------------\n%s: (%lu) %p -> check in\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
#endif
         mulle_aba_checkin();
#if MULLE_ABA_TRACE
         fprintf( stderr,  "\n%s (%lu) %p <- checked in\n------------------------------\n", mulle_aba_thread_name(),
            mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
         mulle_aba_print();
#endif
         break;

      case 19 :
         if( registered)
         {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "\n------------------------------\n%s: (%lu) %p -> unregister\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
#endif
            mulle_aba_unregister();
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "%s: (%lu) %p <- unregistered\n------------------------------\n", mulle_aba_thread_name(),
            mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
            mulle_aba_print();
#endif
         }
         else
         {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "\n------------------------------\n%s: (%lu) %p -> register\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
#endif
            mulle_aba_register();
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "\n%s: (%lu) %p <- registered\n------------------------------\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
            mulle_aba_print();
#endif
         }
         registered = ! registered;
         break;
         
      default :
         sched_yield();
         break;
      }
   
   if( ! registered)
   {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "\n------------------------------\n%s: (%lu) %p -> register\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
#endif
         mulle_aba_register();
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
            fprintf( stderr,  "\n%s: (%lu) %p <- registered\n------------------------------\n", mulle_aba_thread_name(), mulle_aba_get_thread_timestamp(),
               _mulle_aba_get_worldpointer());
            mulle_aba_print();
#endif
      }
}


void  single_threaded_test()
{
#if PROGRESS
   fprintf( stdout,  "."); fflush( stdout);
#endif
   run_gc_free_list_test();
}



void  multi_threaded_test_each_thread()
{
#if PROGRESS
   fprintf( stdout,  "."); fflush( stdout);
#endif
   run_thread_gc_free_list_test();
}


static void   _wait_around( mulle_atomic_pointer_t *n_threads)
{
   // wait for all threads to materialize
   _mulle_atomic_pointer_decrement( n_threads);
   while( _mulle_atomic_pointer_read( n_threads) != 0)
      sched_yield();
}


struct thread_info
{
   char                  name[ 64];
   mulle_atomic_pointer_t    *n_threads;
};


static mulle_thread_rval_t   run_test( struct thread_info *info)
{
   mulle_thread_tss_set( timestamp_thread_key, strdup( info->name));

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
   fprintf( stderr,  "\n------------------------------\n%s: -> register\n", mulle_aba_thread_name());
#endif
   mulle_aba_register();
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
   fprintf( stderr,  "%s: <- registered\n", mulle_aba_thread_name());
   mulle_aba_print();
#endif
   
   _wait_around( info->n_threads);
   multi_threaded_test_each_thread();

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
   fprintf( stderr,  "\n------------------------------\n%s: -> unregister\n", mulle_aba_thread_name());
#endif
   mulle_aba_unregister();
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_REGISTER
   fprintf( stderr,  "%s: <- unregistered\n------------------------------\n", mulle_aba_thread_name());
   mulle_aba_print();
#endif
  
    // voodoo 
   fflush( stdout);
   fflush( stderr);

   return( 0);
}


void  multi_threaded_test( intptr_t n)
{
   int                  i;
   mulle_thread_t            *threads;
   struct thread_info   *info;
   mulle_atomic_pointer_t   n_threads;
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "////////////////////////////////\n");
   fprintf( stderr, "multi_threaded_test( %ld) starts\n", n);
#endif
   threads = alloca( n * sizeof( mulle_thread_t));
   assert( threads);
   
   _mulle_atomic_pointer_nonatomic_write( &n_threads, (void *) n);
   info = alloca( sizeof(struct thread_info) * n);

   for( i = 1; i < n; i++)
   {
      info[ i].n_threads = &n_threads;
      sprintf( info[ i].name, "thread #%d", i);
      
      if( mulle_thread_create( (void *) run_test, &info[ i], &threads[ i]))
         abort();
   }
   
   info[ 0].n_threads = &n_threads;
   sprintf( info[ 0].name, "thread #%d", 0);
   run_test( &info[ 0]);

   for( i = 1; i < n; i++)
      if( mulle_thread_join( threads[ i]))
      {
         perror( "mulle_thread_join");
         abort();
      }
      
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: multi_threaded_test( %ld) ends\n", mulle_aba_thread_name(), n);
#endif
}


char  *mulle_aba_thread_name( void)
{
   return( mulle_thread_tss_get( timestamp_thread_key));
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


int   _main(int argc, const char * argv[])
{
   unsigned int   i;
   unsigned int   j;
   int            rval;
   
   srand( (unsigned int) time( NULL));
   
   rval = mulle_thread_mutex_init( &alloc_lock);
   assert( ! rval);

   rval = mulle_thread_tss_create( &timestamp_thread_key, free);
   assert( ! rval);
   
   rval = mulle_thread_tss_set( timestamp_thread_key, strdup( "main"));
   assert( ! rval);
   
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s\n", mulle_aba_thread_name());
#endif
   
   _mulle_aba_init( &test_global, &test_allocator, sched_yield);
   if( mulle_aba_set_global( &test_global))
   {
      perror( "mulle_aba_set_global");
      abort();
   }

forever:
   reset_memory();

   //
   // if there are leaks anywhere, it will assert in
   // _mulle_aba_storage_done which is called by reset_memory
   // eventually
   //
   
   for( i = 0; i < 400; i++)
   {
#if MULLE_ABA_TRACE || PROGRESS
# if MULLE_ABA_TRACE
      fprintf( stderr, "iteration %d\n", i);
# else
      fprintf( stdout, "iteration %d\n", i);
# endif
#endif
#if 1
      single_threaded_test();
      reset_memory();
#endif
      for( j = 1; j <= 4; j += j)
      {
         multi_threaded_test( j);
         reset_memory();
      }
   }
#if FOREVER
   goto forever;
#endif
   mulle_thread_mutex_destroy( &alloc_lock);
#if STATE_STATS
   {
      extern void   mulle_transitions_count_print_dot( void);
      
      mulle_transitions_count_print_dot();
   }
#endif
   return( 0);
}


#ifndef NO_MAIN
int   main(int argc, const char * argv[])
{
   return( _main( argc, argv));
}
#endif

   
