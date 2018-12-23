//
//  main.c
//  mulle-aba-test
//
//  Created by Nat! on 19.03.15.
//  Copyright (c) 2015 Mulle kybernetiK. All rights reserved.
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
#include <mulle-aba/mulle-aba.h>

#include <mulle-testallocator/mulle-testallocator.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>

//
// when we compile with DEBUG and MULLE_TEST we have the MULLE_THREAD_UNPLEASANT_RACE_YIELD
// to catch errors, which is REALLY slow
//
#define PROGRESS     0
#define FOREVER      0

#if defined( MULLE_TEST)
# define LOOPS        1000   // (1 + (rand() % 100000))
# define ITERATIONS   10
#else
# define LOOPS        100000   // (1 + (rand() % 100000))
# define ITERATIONS   100
#endif
#define MAX_THREADS  4


#if DEBUG_PRINT
extern void   mulle_aba_print( void);
#else
# define mulle_aba_print()
#endif

static mulle_thread_tss_t   threadname_key;
char  *mulle_aba_thread_name( void);


#pragma mark -
#pragma mark global variables

static struct _mulle_aba_linkedlist   list;     // common
static mulle_atomic_pointer_t         alloced;  // common

#pragma mark -
#pragma mark reset allocator between tests

static void   reset_memory()
{
#if MULLE_ABA_TRACE
   fprintf( stderr, "\n================================================\n");
#endif

   // use library to track allocations
   mulle_testallocator_reset();

   memset( &alloced, 0, sizeof( alloced));
   memset( &list, 0, sizeof( list));
}



#pragma mark -
#pragma mark run test

static void    run_thread_gc_free_list_test( void)
{
   struct _mulle_aba_freeentry   *entry;
   unsigned long                  i;
   void                           *thread;

   thread = mulle_aba_thread_name();

   for( i = 0; i < LOOPS; i++)
   {
      entry = (void *) _mulle_aba_linkedlist_remove_one( &list);
      if( entry)
      {
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: reused %p (%p) from %p\n",
                              mulle_aba_thread_name(),
                              entry,
                              entry->_link._next,
                              &list);
#endif
         assert( ! entry->_link._next);
      }
      else
      {
         entry = mulle_allocator_calloc( &mulle_testallocator, 1, sizeof( *entry));
         _mulle_atomic_pointer_increment( &alloced);
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: allocated %p (%p)\n",
                              mulle_aba_thread_name(),
                              entry, entry->_link._next);
#endif
      }

      assert( entry);
      entry->_pointer = (void *) i;
      entry->_owner   = thread;

      _mulle_aba_linkedlist_add( &list, &entry->_link);
   }
}


static void  multi_threaded_test_each_thread( void)
{
#if PROGRESS
   fprintf( stdout, "."); fflush( stdout);
#endif
   run_thread_gc_free_list_test();
}


static void   _wait_around( mulle_atomic_pointer_t *n_threads)
{
   // wait for all threads to materialize
   _mulle_atomic_pointer_decrement( n_threads);
   while( _mulle_atomic_pointer_read( n_threads) != 0)
      mulle_thread_yield();
}


struct thread_info
{
   char                     name[ 64];
   mulle_atomic_pointer_t   *n_threads;
};


static mulle_thread_rval_t   run_test( struct thread_info *info)
{
   mulle_thread_tss_set( threadname_key, strdup( info->name));

   _wait_around( info->n_threads);
   multi_threaded_test_each_thread();

   return( 0);
}


static void   finish_test( void)
{
   struct _mulle_aba_freeentry   *entry;
   // remove all from list, so any leak means trouble

   while( _mulle_atomic_pointer_decrement( &alloced))
   {
      entry = (void *) _mulle_aba_linkedlist_remove_one( &list);
      assert( entry);
      mulle_allocator_free( &mulle_testallocator, entry);
   }
}


static void  multi_threaded_test( intptr_t n)
{
   int                  i;
   mulle_thread_t       *threads;
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

   finish_test();

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: multi_threaded_test( %ld) ends\n", mulle_aba_thread_name(), n);
#endif
}


char  *mulle_aba_thread_name( void)
{
   return( mulle_thread_tss_get( threadname_key));
}


#ifdef __APPLE__
#include <sys/resource.h>

MULLE_C_CONSTRUCTOR(__enable_core_dumps)
static void  __enable_core_dumps(void)
{
    struct rlimit   limit;

    limit.rlim_cur = RLIM_INFINITY;
    limit.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &limit);
}
#endif

static int   _main(int argc, const char * argv[])
{
   unsigned int   i;
   unsigned int   j;
   int            rval;

   srand( (unsigned int) time( NULL));

   rval = mulle_thread_tss_create( free, &threadname_key);
   assert( ! rval);

   rval = mulle_thread_tss_set( threadname_key, strdup( "main"));
   assert( ! rval);

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s\n", mulle_aba_thread_name());
#endif

#if FOREVER
   fprintf( stderr, "This test runs forever, waiting for a crash\n");
#endif

forever:
   reset_memory();

   //
   // if there are leaks anywhere, it will assert in
   // _mulle_aba_storage_done which is called by reset_memory
   // eventually
   //

   for( i = 0; i < ITERATIONS; i++)
   {
#if MULLE_ABA_TRACE || PROGRESS
# if MULLE_ABA_TRACE
      fprintf( stderr, "iteration %d of %d\n", i, ITERATIONS);
# else
      fprintf( stdout, "iteration %d of %d\n", i, ITERATIONS);
# endif
#endif
      for( j = 1; j <= MAX_THREADS; j += j)
      {
         multi_threaded_test( j);
         reset_memory();
      }
   }

#if FOREVER
   goto forever;
#endif

   return( 0);
}


#ifndef NO_MAIN
int   main(int argc, const char * argv[])
{
   return( _main( argc, argv));
}
#endif
