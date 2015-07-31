//
//  mulle_aba.h
//  mulle-aba
//
//  Created by Nat! on 19.03.15.
//  Copyright (c) 2015 Mulle kybernetiK. All rights reserved.
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

#include "mulle_aba.h"

#include "mulle_aba_defines.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

extern char  *pthread_name( void);

#define DEBUG_SINGLE_THREADED  0

# pragma mark -
# pragma mark globals

static pthread_key_t   timestamp_thread_key;
struct _mulle_aba      *global;

static int   _mulle_aba_unregister_thread( struct _mulle_aba *p, pthread_t thread);
   
static void   thread_destructor( void *value)
{
   _mulle_aba_unregister_thread( global, pthread_self());
}


int   mulle_aba_set_global( struct _mulle_aba *p)
{
   global = p;
   if( ! timestamp_thread_key)
      return( pthread_key_create( &timestamp_thread_key, thread_destructor));
   return( 0);
}


# pragma mark -
# pragma mark init/done

int   _mulle_aba_init( struct _mulle_aba *p,
                       struct _mulle_allocator *allocator,
                       int (*yield)( void))
{
   int   rval;
   
   rval = _mulle_aba_storage_init( &p->storage, allocator);
   assert( ! rval);
#if DEBUG_SINGLE_THREADED
   if( ! rval)
   {
      _mulle_aba_world_pointer_t   world_p;
      struct _mulle_aba_world      *world;
      
      world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
      world   = _mulle_aba_world_pointer_get_struct( world_p);
      world->_retain_count = 1;
   }
#endif
   return( rval);
}


static void   _mulle_aba_done( struct _mulle_aba *p)
{
   _mulle_aba_storage_done( &p->storage);
}


# pragma mark -
# pragma mark get rid of old worlds

static int   _mulle_aba_thread_free_block( struct _mulle_aba *p, pthread_t thread, void *owner, void *pointer, void (*p_free)( void *, void *));


static int   _mulle_lockfree_deallocator_free_world_chain( struct _mulle_aba *p, pthread_t thread, struct _mulle_aba_world   *old_world)
{
   struct _mulle_aba_world   *tofree;
   struct _mulle_aba_world   *next;
   int                                  rval;
   
   for( tofree = old_world; tofree; tofree = next)
   {
#if TRACE || TRACE_FREE
      fprintf( stderr,  "\n%s: *** free world delayed %p\n", pthread_name(), tofree);
#endif
      next = (struct _mulle_aba_world *) tofree->_link._next;
      assert( next != tofree);
      
      UNPLEASANT_RACE_YIELD();
      //
      // sic! this looks weird, because 'p' looks like its freed, but its just
      // the first parameter to _mulle_aba_storage_free_world
      //
      rval = _mulle_aba_thread_free_block( p, thread, p, tofree, (void *) _mulle_aba_storage_free_world);
      if( rval)
         return( rval);
      UNPLEASANT_RACE_YIELD();
   }
   return( 0);
}


static int   _mulle_lockfree_deallocator_free_world_if_needed( struct _mulle_aba *p, pthread_t thread, struct _mulle_aba_world_pointers world_ps)
{
   struct _mulle_aba_world   *new_world;
   struct _mulle_aba_world   *old_world;
   
   new_world = mulle_aba_world_pointer_get_struct( world_ps.new_world_p);
   old_world = mulle_aba_world_pointer_get_struct( world_ps.old_world_p);
   if( new_world == old_world)
      return( 0);

   assert( mulle_aba_world_pointer_get_struct( _mulle_aba_storage_get_world_pointer( &p->storage)) != old_world);
   
   // don't free locked worlds
   if( mulle_aba_world_pointer_get_lock( world_ps.old_world_p))
   {
#if TRACE || TRACE_FREE
      fprintf( stderr,  "\n%s: ***not freeing locked world %p\n", pthread_name(), old_world);
#endif
      return( 0);
   }

   assert( ! old_world->_link._next);
   return( _mulle_aba_thread_free_block( p, thread, p, old_world, (void *) _mulle_aba_storage_free_world));
}


# pragma mark -
# pragma mark register

struct add_context
{
   struct _mulle_aba_timestamp_storage  *ts_storage;
   struct _mulle_allocator                   *allocator;
};


static int   add_thread( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
#if TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n", pthread_name(), __PRETTY_FUNCTION__, mode, info->new_world->_timestamp, info->new_world->_n_threads);
#endif
   assert( info->old_world != info->new_world);
   
   if( mode != _mulle_aba_world_modify)
      return( 0);
   
   assert( info->new_world->_timestamp == info->old_world->_timestamp);

   // doing this now would simplify the state graph so much
   // if( ! info->new_world->_timestamp)
   //  info->new_world->_timestamp = 1;
      
   info->new_world->_n_threads++;
   info->new_bit = 1;
   return( 0);
}


static int   _mulle_aba_thread_free_block( struct _mulle_aba *p, pthread_t thread, void *owner, void *pointer, void (*p_free)( void *, void *));


static int   _mulle_aba_register_thread( struct _mulle_aba *p, pthread_t thread)
{
   intptr_t   timestamp;
   struct _mulle_aba_world            *new_world;
   struct _mulle_aba_world            *dealloced;
   struct _mulle_aba_world_pointers   world_ps;
   
   assert( timestamp_thread_key);
   assert( ! pthread_getspecific( timestamp_thread_key));

   //
   // we need to "lock" the world pointer, before we can access it
   // If the lock succeds another thread can not lock the same world, because
   // the expected value is unlocked.
   //
   // remember that checkin and free do not observe the lock, so the
   // world maybe overwritten still. Checkins that only change the bit must
   // preserve the lock, because otherwise the world might get freed.
   //
   // So the ONLY thing we know now is that we can read the old world.
   // Other threads can be registering and deregistering.
   //
   world_ps  = _mulle_aba_storage_copy_lock_world_pointer( &p->storage, add_thread, NULL, &dealloced);
   if( ! world_ps.new_world_p)
   {
      assert( 0);
      return( -1);
   }
   
   new_world = mulle_aba_world_pointer_get_struct( world_ps.new_world_p);
   timestamp = new_world->_timestamp + 1;
   if( ! timestamp)
      timestamp = 1;
   
   pthread_setspecific( timestamp_thread_key, (void *) timestamp);
   
   return( _mulle_lockfree_deallocator_free_world_chain( p, thread, dealloced));
}


# pragma mark -
# pragma mark check in

static void   _mulle_aba_check_timestamp_range( struct _mulle_aba *p, pthread_t thread, uintptr_t old, uintptr_t new)
{
   _mulle_aba_world_pointer_t         world_p;
   struct _mulle_aba_world            *world;

   world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
   world   = mulle_aba_world_pointer_get_struct( world_p);
   
   _mulle_aba_world_check_timerange( world, old, new, &p->storage);
}


static int   set_bit( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
#if TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n", pthread_name(), __PRETTY_FUNCTION__, mode, info->new_world->_timestamp, info->new_world->_n_threads);
#endif
   if( mode == _mulle_aba_world_discard)
      return( 0);

   info->new_bit = 1;
   return( 0);
}
       
static int   _mulle_aba_checkin_thread( struct _mulle_aba *p, pthread_t thread)
{
   _mulle_aba_world_pointer_t         old_world_p;
   int                                rval;
   intptr_t                           new;
   intptr_t                           old;
   struct _mulle_aba_world            *old_world;
   struct _mulle_aba_world_pointers   world_ps;
   
   //
   // if old == 0, thread is not configured...
   // and hasn't freed anything. Therefore is not counted as an active thread
   //
   old = (intptr_t) pthread_getspecific( timestamp_thread_key);
   if( ! old)
      return( 0);
   
   old_world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
   old_world   = mulle_aba_world_pointer_get_struct( old_world_p);
   new         = old_world->_timestamp;
   
   if( old > new)
      return( 0);
   
   world_ps = _mulle_aba_storage_change_world_pointer_2( &p->storage, _mulle_swap_checkin_intent, old_world_p, set_bit, NULL);
   if( ! world_ps.new_world_p)
      return( -1);

   UNPLEASANT_RACE_YIELD();

   rval = _mulle_lockfree_deallocator_free_world_if_needed( p, thread, world_ps);
   if( ! rval)
   {
      _mulle_aba_check_timestamp_range( p, thread, old, new);
      pthread_setspecific( timestamp_thread_key, (void *) (new + 1));
   }
   
   //
   return( rval);
}


# pragma mark -
# pragma mark unregister

struct remove_context
{
   uintptr_t                             timestamp;
   struct _mulle_aba_world   *locked_world;
};


static int   remove_thread( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
   unsigned int            i;
   unsigned int            size;
   size_t                  bytes;
   struct remove_context   *context;
   
   context = userinfo;
#if TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n", pthread_name(), __PRETTY_FUNCTION__, mode, info->new_world->_timestamp, info->new_world->_n_threads);
#endif
   assert( info->new_world != info->old_world);
   
   if( mode == _mulle_aba_world_discard)
      return( 0);
   
   if( ! info->old_locked)
   {
#if TRACE
      fprintf( stderr, "%s: EAGAIN_1\n", pthread_name());
#endif  
      return( EAGAIN);
   }

   // if someone changed something inbetween
   if( info->new_world->_timestamp != context->timestamp)
   {
#if TRACE
      fprintf( stderr, "%s: EAGAIN_2\n", pthread_name());
#endif  
      return( EAGAIN);
   }

   if( info->old_world != context->locked_world)
   {
#if TRACE
      fprintf( stderr, "%s: EAGAIN_3\n", pthread_name());
#endif  
      return( EAGAIN);
   }
   
   
   if( --info->new_world->_n_threads)
   {
      info->new_bit = 1;
      return( 0);
   }
   
#if TRACE || TRACE_SWAP
      fprintf( stderr, "%s: I AM THE OMEGA MAN\n", pthread_name()); // last guy in town
#endif  
   
   /* 
    * all threads gone, reset to initial conditions and lose all storages
    * caller must clean up the old_world (and can as it's single threaded)
    */
   for( i = 0; i < info->new_world->_n; i++)
      assert( info->new_world->_storage[ i]);
   
   size  = info->new_world->_size;
   // keep memtype and generation for debugging alive
   bytes = (char *) &info->new_world->_storage[ size] - (char *) &info->new_world->_timestamp;
   memset( &info->new_world->_timestamp, 0, bytes);

   info->new_world->_size    = size;
   info->new_bit             = 0;
   info->new_locked          = 1;
   
   assert( ! info->new_world->_link._next);
   return( 0);
}

       
static int   _mulle_aba_unregister_thread( struct _mulle_aba *p, pthread_t thread)
{
   _mulle_aba_world_pointer_t         locked_world_p;
   _mulle_aba_world_pointer_t         old_world_p;
   int                                rval;
   intptr_t                           new;
   intptr_t                           old;
   struct _mulle_aba_world            *old_world;
   struct _mulle_aba_world            *locked_world;
   struct _mulle_aba_world_pointers   world_ps;
   struct remove_context              context;
   unsigned int                       i, n;
   
   /* check in first */
   assert( timestamp_thread_key);

   for(;;)
   {
      //
      // if old == 0, thread is not configured...
      //
      old = (intptr_t) pthread_getspecific( timestamp_thread_key);
      if( ! old)
         return( 0);
      
      old_world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
      old_world   = mulle_aba_world_pointer_get_struct( old_world_p);
      new         = old_world->_timestamp;
      
#if TRACE
      fprintf( stderr, "%s: ::: final check in %ld-%ld\n", pthread_name(), old, new);
#endif
#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( old_world);
#endif
      
      /* something to do ? */
      if( old <= new)
      {
         world_ps = _mulle_aba_storage_change_world_pointer_2( &p->storage, _mulle_swap_checkin_intent, old_world_p, set_bit, NULL);
         if( ! world_ps.new_world_p)
            return( -1);
         
         old_world_p = world_ps.new_world_p;
         old_world   = mulle_aba_world_pointer_get_struct( old_world_p);

         rval = _mulle_lockfree_deallocator_free_world_if_needed( p, thread, world_ps);
         if( rval)
            return( rval);
         
#ifndef NDEBUG
         _mulle_aba_world_assert_sanity( old_world);
#endif
         _mulle_aba_world_check_timerange( old_world, old, new, &p->storage);
         pthread_setspecific( timestamp_thread_key, (void *) (new + 1));
#ifndef NDEBUG
         _mulle_aba_world_assert_sanity( old_world);
#endif
      }
      
      locked_world_p = _mulle_aba_storage_try_lock_world_pointer( &p->storage, old_world_p);
      if( ! locked_world_p)
         continue;
      
      UNPLEASANT_RACE_YIELD();
      
      locked_world         = mulle_aba_world_pointer_get_struct( locked_world_p);
      context.timestamp    = new;
      context.locked_world = locked_world;
#if TRACE
      fprintf( stderr, "%s: set timestamp to %ld\n", pthread_name(), new);
#endif
      
      //
      // we can't say (old_world->_n_threads == 1) here
      //
      world_ps = _mulle_aba_storage_copy_change_world_pointer( &p->storage, _mulle_swap_unregister_intent, locked_world_p, remove_thread, &context);
      if( world_ps.new_world_p)
         break;

      if( errno == EAGAIN || errno == EBUSY)
      {
         //
         // make sure it's not locked, can happen when remove_thread figures out
         // we need to redo the checkin and fails
         //
         if( ! _mulle_aba_storage_try_unlock_world_pointer( &p->storage, locked_world_p))
         {
            //
            // if we couldn't unlock, we have now a world, that we need to
            // delay release (yay)
            //
            rval = _mulle_lockfree_deallocator_free_world_chain( p, thread, mulle_aba_world_pointer_get_struct( locked_world_p));
            if( rval)
               return( rval);
         }
         continue;
      }
#if DEBUG
      abort();
#endif
      return( -1);
   }

   pthread_setspecific( timestamp_thread_key, (void *) 0);  // mark as clean now

   // we make sure, that  old_world is the same as locked_world.
   //
   assert( world_ps.old_world_p == locked_world_p);
   
   //
   // special code: if the old world's retain count was 1, it is now 0
   // then "remove_thread" will have reverted back to initial state. And the
   // ts_storages are now orphaned.
   //
   if( locked_world->_n_threads == 1)
   {
#if TRACE || TRACE_FREE
      fprintf( stderr, "\n%s: *** returned to initial state, removing leaks***\n", pthread_name());
#endif
      //
      // get rid of all old world resources, the new world has been cleaned
      // and is not referencing storages.
      // do actual frees after lock ?
      assert( mulle_aba_world_pointer_get_lock( world_ps.new_world_p));
      
      n = locked_world->_size;
      for( i = 0; i < n; i++)
         __mulle_aba_timestamp_storage_free( locked_world->_storage[ i], &p->storage._allocator);

      _mulle_aba_storage_free_leak_worlds( &p->storage);

      assert( ! locked_world->_link._next);
      
      _mulle_aba_storage_free_world( &p->storage, locked_world);
      _mulle_aba_storage_free_unused_free_entries( &p->storage);
      
      _mulle_aba_storage_try_unlock_world_pointer( &p->storage, world_ps.new_world_p);
   }
   else
   {
      // [^1] if we are the last thread going, because the other thread just exited
      // this will not be cleaned up by
      // _mulle_aba_storage_free_leak_worlds above
      _mulle_aba_storage_add_leak_world( &p->storage, locked_world);
   }

   return( 0);
}


# pragma mark -
# pragma mark add block to free

static int   handle_callback_mode_for_add_context( int mode, struct _mulle_aba_callback_info  *info, struct add_context *context)
{
   assert( info->old_world != info->new_world);

   switch( mode)
   {
   case _mulle_aba_world_discard :
#if TRACE
      fprintf( stderr, "%s: discard ts_storage %p\n", pthread_name(), context->ts_storage);
#endif
      _mulle_aba_timestamp_storage_free( context->ts_storage, context->allocator);
      context->ts_storage = NULL;
      return( 0);

   case _mulle_aba_world_modify  :
      context->ts_storage = _mulle_aba_timestamp_storage_alloc( context->allocator);
      if( ! context->ts_storage)
         return( errno);
         
         // fall thru
         
   case _mulle_aba_world_reuse   :  // could make this better, for now just discard and redo
#if TRACE
      fprintf( stderr, "%s: %s ts_storage %p at %u in world %p\n", pthread_name(), (mode == _mulle_aba_world_reuse) ? "reuse" : "set", context->ts_storage, info->new_world->_n, info->new_world);
#endif
      assert( info->new_world->_n < info->new_world->_size);
      // if we read in a new world, we expect a zero there
      // if we reuse the previous failed world, there would be our allocated
      // storage
      //
      assert( ! info->new_world->_storage[ info->new_world->_n] ||  info->new_world->_storage[ info->new_world->_n] == context->ts_storage);
      
      info->new_world->_storage[ info->new_world->_n] = context->ts_storage;
      info->new_world->_n++;
      break;
   }
   return( 0);
}


static int   pre_check_world_for_timestamp_increment( int mode, struct _mulle_aba_callback_info  *info, struct add_context   *context)
{
   assert( info->old_world != info->new_world);
   //
   // if the world has changed to our benefit, cancel and free like usual
   // discard
   if( info->new_world->_n_threads <= 1)
   {
      handle_callback_mode_for_add_context( _mulle_aba_world_discard, info, context);
#if TRACE
      fprintf( stderr, "%s: EAGAIN_1 (%p)\n", pthread_name(), context->ts_storage);
#endif  
      return( EAGAIN);
   }

   //
   // we don't want to set the timestamp, if bit is actually 0
   //
   if( ! info->old_bit)
   {
#if TRACE
      fprintf( stderr, "%s: EBUSY_1 (%p)\n", pthread_name(), context->ts_storage);
#endif
      return( EBUSY);  // just retry
   }
   
   return( 0);
}


static int   size_check_world_for_timestamp_increment( int mode, struct _mulle_aba_callback_info *info, struct add_context *context)
{
   assert( info->old_world != info->new_world);

   // if the world is now too small, also bail
   if( (info->new_world->_timestamp - info->new_world->_offset + 1) >= info->new_world->_n * _mulle_aba_timestamp_storage_n_entries)
   {
#if TRACE
      fprintf( stderr, "%s: EBUSY_2 (%p)\n", pthread_name(), context->ts_storage);
#endif
      return( EBUSY);
   }
   return( 0);
}


static void   _increment_timestamp( struct _mulle_aba_callback_info  *info)
{
   struct _mulle_aba_timestamp_storage   *ts_storage;
   unsigned int                               index;
   
   assert( info->old_world != info->new_world);

   if( ! ++info->new_world->_timestamp)
      info->new_world->_timestamp = 1;
   
   ts_storage = _mulle_aba_world_get_timestamp_storage( info->new_world, info->new_world->_timestamp);
   index = mulle_aba_timestamp_storage_get_timestamp_index( info->new_world->_timestamp);
   _mulle_aba_timestamp_storage_set_usage_bit( ts_storage, index, 1);
   info->new_bit = 0;
}


static int   increment_timestamp( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
   int                  rval;
   struct add_context   *context;
   
#if TRACE
   fprintf( stderr, "%s: %s (%d)\n", pthread_name(), __PRETTY_FUNCTION__, mode);
#endif
   assert( info->old_world != info->new_world);
   
   context = userinfo;
   
   if( mode == _mulle_aba_world_discard)
   {
      handle_callback_mode_for_add_context( mode, info, context);
      return( 0);
   }

   rval = pre_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);
   rval = size_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);

   _increment_timestamp( info);
   return( 0);
}



extern void   _mulle_aba_timestamp_storage_empty_assert(  struct _mulle_aba_timestamp_storage  *ts_storage);

static int   add_storage_and_increment_timestamp( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
   struct add_context   *context;
   int                  rval;
   
#if TRACE
   fprintf( stderr, "%s: %s (%d)\n", pthread_name(), __PRETTY_FUNCTION__, mode);
#endif
   assert( info->old_world != info->new_world);
   
   context = userinfo;
   if( mode == _mulle_aba_world_discard)
   {
      _mulle_aba_timestamp_storage_free( context->ts_storage, context->allocator);
      context->ts_storage = NULL;
      return( 0);
   }
   
   rval = pre_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
   {
      // special case code: hackish, sry
      if( mode == _mulle_aba_world_reuse)
      {
         _mulle_aba_timestamp_storage_free( context->ts_storage, context->allocator);
         context->ts_storage = NULL;
      }
      return( rval);
   }
   
   //
   // if the world has changed to our detriment, cancel and retry
   // assume we can reuse, so keep the context->ts_storage
   //
   if( info->new_world->_n >= info->new_world->_size)
   {
#if TRACE
      fprintf( stderr, "%s: EBUSY_3 (%p)\n", pthread_name(), context->ts_storage);
#endif
      return( EBUSY);
   }
   
   rval = handle_callback_mode_for_add_context( mode, info, context);
   if( rval)
      return( rval);

   rval = size_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);

   _increment_timestamp( info);
   return( 0);
}


static int   reorganize_storage_and_increment_timestamp( int mode, struct _mulle_aba_callback_info *info, void *userinfo)
{
   int                  rval;
   struct add_context   *context;
   
#if TRACE
   fprintf( stderr, "%s: %s (%d)\n", pthread_name(), __PRETTY_FUNCTION__, mode);
#endif
   assert( info->old_world != info->new_world);

   context = userinfo;
   
   if( mode == _mulle_aba_world_discard)
   {
      handle_callback_mode_for_add_context( mode, info, context);
      return( 0);
   }
   
   rval = pre_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);
   
   if( ! _mulle_aba_world_reuse_storages( info->new_world))
   {
#if TRACE
      fprintf( stderr, "%s: EBUSY_4 (%p)\n", pthread_name(), context->ts_storage);
#endif  
      return( EBUSY);  // hmm nothing to reuse ?
   }
   
   rval = size_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);

   _increment_timestamp( info);
   return( 0);
}


int   _mulle_aba_thread_free_block( struct _mulle_aba *p,
                                    pthread_t thread,
                                    void *owner,
                                    void *pointer,
                                    void (*p_free)( void *, void *))
{
   _mulle_aba_world_pointer_t            world_p;
   int                                   old_bit;
   struct _mulle_aba_world               *free_worlds;
   struct _mulle_aba_free_entry          *entry;
   struct _mulle_aba_world_pointers      world_ps;
   struct _mulle_aba_world               *world;
   struct _mulle_aba_world               *new_world;
   struct _mulle_aba_world               *next;
   struct _mulle_aba_world               *old_world;
   struct _mulle_aba_timestamp_entry     *ts_entry;
   struct add_context                    ctxt;
   uintptr_t                             timestamp;
   int                                   loops;
   
   assert( p);
   assert( thread);
   assert( pointer);
   
   free_worlds = NULL;
   
   world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
   world   = mulle_aba_world_pointer_get_struct( world_p);
   
   assert( pointer != world);

   // TODO: move this into loop and combine with EAGAIN code
   if( world->_n_threads <= 1)
   {
#if TRACE
      fprintf( stderr,  "%s: *** p_free immediately, because RC=1 %p***\n", pthread_name(), pointer);
#endif
      (*p_free)( owner, pointer);
      return( 0);
   }
      
   // thread not properly set up.. either abort (or just free ?)
#if DEBUG
   if( ! pthread_getspecific( timestamp_thread_key))
      abort();
#endif

   ctxt.allocator = &p->storage._allocator;
   ctxt.ts_storage = NULL;


   for( loops = 0;; ++loops)
   {
      world     = mulle_aba_world_pointer_get_struct( world_p);
      old_bit   = mulle_aba_world_pointer_get_bit( world_p);
      timestamp = world->_timestamp + old_bit;
      ts_entry  = _mulle_aba_world_get_timestamp_entry( world, timestamp);
      
      if( ts_entry)
      {
         if( ! old_bit)
            break;         // preferred and only exit
         
#if TRACE
         fprintf( stderr, "%s: update timestamp in world %p\n", pthread_name(), world_p);
#endif
         world_ps = _mulle_aba_storage_copy_change_world_pointer( &p->storage, _mulle_swap_free_intent, world_p, increment_timestamp, &ctxt);
      }
      else
         if( world->_n < world->_size)
         {
#if TRACE
            fprintf( stderr, "%s: add storage and set rc in world %p\n", pthread_name(), world_p);
#endif
            // add a storage to the world,
            world_ps = _mulle_aba_storage_copy_change_world_pointer( &p->storage, _mulle_swap_free_intent, world_p, add_storage_and_increment_timestamp, &ctxt);
         }
         else
            if( _mulle_aba_world_count_avaiable_reusable_storages( world))
            {
               //
               // try to reorganize our world to make more room, but we
               // can't set retain count at the same time though
               //
#if TRACE
               fprintf( stderr, "%s: reorganize world %p\n", pthread_name(), world_p);
#endif
               world_ps = _mulle_aba_storage_copy_change_world_pointer( &p->storage, _mulle_swap_free_intent, world_p, reorganize_storage_and_increment_timestamp, &ctxt);
            }
            else
            {
               //
               // grow the available storage pointers and add a storage
               // now if this fails, we have a dangling ts_storage we need to
               // get rid off
#if TRACE
               fprintf( stderr, "%s: grow and set RC in world %p\n", pthread_name(), world_p);
#endif
               world_ps = _mulle_aba_storage_grow_world_pointer( &p->storage, world->_size + 1, add_storage_and_increment_timestamp, &ctxt);
            }

      if( world_ps.new_world_p)
      {
         new_world = mulle_aba_world_pointer_get_struct( world_ps.new_world_p);
         /* successfully incremented timestamp
            now other threads may already be calling checkin
            add n_threads to the rc which was virtually at 0, but now may be
            negative if the result is 0, then...
         */
         ts_entry  = _mulle_aba_world_get_timestamp_entry( new_world, new_world->_timestamp);
         assert( ts_entry);
         _mulle_atomic_add_pointer( &ts_entry->_retain_count_1, new_world->_n_threads);
         
         //
         // our thread hasn't checked in, so we know it can't be 0
         // don't add locked worlds to list
         //
         if( ! mulle_aba_world_pointer_get_lock( world_ps.old_world_p))
         {
            old_world = mulle_aba_world_pointer_get_struct( world_ps.old_world_p);
            if( new_world != old_world)
            {
               // old world needs to be freed later
               assert( old_world != free_worlds);
               assert( old_world->_link._next == NULL);
#if TRACE || TRACE_FREE
               fprintf( stderr, "%s: add %p to worlds to free world chain\n", pthread_name(), old_world);
#endif
               old_world->_link._next = (void *) free_worlds;
               free_worlds            = old_world;
            }
         }
         
         world_p = world_ps.new_world_p;
         continue;
      }

      if( errno == EBUSY) // if busy, then world changed behind our back, retry
      {
         world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
         continue;
      }
      
      if( errno == EAGAIN)  // went back to single threaded, just discard
      {
#if TRACE
         fprintf( stderr,  "%s: *** p_free immediately, because RC became 1 %p***\n", pthread_name(), pointer);
#endif
         (*p_free)( owner, pointer);
         
         while( free_worlds)
         {
#if TRACE
            fprintf( stderr,  "\n%s: *** free old world %p immediately***\n", pthread_name(), free_worlds);
#endif
            next = (struct _mulle_aba_world *) free_worlds->_link._next;
            (p->storage._allocator.free)( free_worlds);
            free_worlds = next;
         }
         return( 0);
      }

      // leaks worlds in this case
      
      assert( errno);
      return( -1);     // nope: regular error (gotta be ENOMEM)
   }

#ifndef NDEBUG
   _mulle_aba_world_assert_sanity( mulle_aba_world_pointer_get_struct( world_p));
#endif
   entry = _mulle_aba_storage_alloc_free_entry( &p->storage);
   if( ! entry)
   {
      assert( 0);
      return( -1);
   }
      
   _mulle_aba_free_entry_set( entry, owner, pointer, p_free);
   _mulle_aba_linked_list_add( &ts_entry->_block_list, &entry->_link);
#if TRACE || TRACE_LIST
   fprintf( stderr,  "\n%s: *** put pointer %p/%p on linked list %p of ts=%ld rc=%ld***\n", pthread_name(), pointer, owner, &ts_entry->_block_list, timestamp, (intptr_t) _mulle_atomic_read_pointer( &ts_entry->_retain_count_1) + 1);
#endif

   
   //
   // now free accumulated worlds on the same block list
   //
   while( free_worlds)
   {
      entry = _mulle_aba_storage_alloc_free_entry( &p->storage);
      if( ! entry)
         return( -1);
      
      next = (struct _mulle_aba_world *) free_worlds->_link._next;
      _mulle_aba_free_entry_set( entry, &p->storage, free_worlds, (void *) _mulle_aba_storage_free_world);
      
      _mulle_aba_linked_list_add( &ts_entry->_block_list, &entry->_link);
#if TRACE || TRACE_LIST
      fprintf( stderr,  "\n%s: *** put old world %p on linked list %p of ts=%ld rc=%ld***\n", pthread_name(), free_worlds, &ts_entry->_block_list, timestamp, (intptr_t) _mulle_atomic_read_pointer( &ts_entry->_retain_count_1) + 1);
#endif
      free_worlds = next;
   }

#if TRACE
   _mulle_aba_linked_list_print( &ts_entry->_block_list);
#endif
   return( 0);
}


# pragma mark -
# pragma mark API

// your thread must call this pretty much immediately after creation
void   mulle_aba_register( void)
{
   assert( global);
   if( _mulle_aba_register_thread( global, pthread_self()))
   {
      perror( "_mulle_aba_register_thread");
      abort();
   }
}


void   mulle_aba_checkin( void)
{
   assert( global);

   // Ok we promise not to read the old stuff anymore
   // and with the barrier, that should be enforced
   // TODO: Voodoo or necessity ? Think about it.
   mulle_atomic_memory_barrier();

   _mulle_aba_checkin_thread( global, pthread_self());
}


static void    just_free( void *owner, void *pointer)
{
   void   (*p_free)( void *);
   
   if( ! (p_free = owner))
      p_free = free;
   (*p_free)( pointer);
}


int   mulle_aba_free( void *block, void (*p_free)( void *))
{
   assert( global);
   if( ! block)
      return( 0);
   
   return( _mulle_aba_thread_free_block( global, pthread_self(), p_free, block, just_free));
}


int   mulle_aba_free_owned_pointer( void *owner, void *block, void (*p_free)( void *, void *))
{
   assert( global);
   assert( p_free);
   
   if( ! block)
      return( 0);
   
//   if( ! mulle_aba_get_thread_timestamp())
//      mulle_aba_register_current_thread();
   
   return( _mulle_aba_thread_free_block( global, pthread_self(), owner, block, p_free));
}


// this will be automatically called on thread destruction, so you usually don't
// call this
void   mulle_aba_unregister()
{
   assert( global);
   if( _mulle_aba_unregister_thread( global, pthread_self()))
   {
      perror( "_mulle_aba_unregister_thread");
      abort();
   }
}

#pragma mark -
#pragma mark init/done

void   mulle_aba_init( struct _mulle_allocator *allocator)
{
   assert( global);
   if( _mulle_aba_init( global, allocator, sched_yield))
   {
      perror( "_mulle_aba_unregister_thread");
      abort();
   }
}


void   mulle_aba_done( void)
{
   assert( global);
   _mulle_aba_done( global);
}


#pragma mark -
#pragma mark Testing stuff

// only really useful for testing
void   mulle_aba_reset()
{
   assert( global);
   struct _mulle_allocator  allocator;
   
   allocator = global->storage._allocator;
   
   _mulle_aba_done( global);
   _mulle_aba_init( global, &allocator, sched_yield);
}



uintptr_t   mulle_aba_get_thread_timestamp()
{
   return( (uintptr_t) pthread_getspecific( timestamp_thread_key));
}


void  *_mulle_aba_get_world_pointer()
{
   assert( global);
   return( _mulle_atomic_read_pointer( &global->storage._world));
}

#if DEBUG

void   _mulle_aba_print_world_pointer( _mulle_aba_world_pointer_t world_p)
{
   int                                   bit;
   struct _mulle_aba_world               *world;
   struct _mulle_aba_timestamp_storage   *ts_storage;
   unsigned int                          i;
   
   world   = mulle_aba_world_pointer_get_struct( world_p);
   bit     = mulle_aba_world_pointer_get_bit( world_p);
   
   fprintf( stderr,  "%s: world_p %p : { %p, %d }\n", pthread_name(), world_p, world, bit);
   fprintf( stderr,  "%s: world   %p : { ts=%ld, rc=%lu, of=%ld, n=%u, sz=%u }\n", pthread_name(),world, world->_timestamp, world->_n_threads, world->_offset, world->_n, world->_size);
   
   for( i = 0; i < world->_n; i++)
   {
      ts_storage = world->_storage[ i];
      assert( ts_storage);
      fprintf( stderr,  "    #%u : adr=%p bits=%p\n", i, ts_storage, _mulle_atomic_read_pointer( & ts_storage->_usage_bits));
   }
   
   if( world->_n)
      fprintf( stderr,  "                      ");
   fprintf( stderr,  "}\n");
}


void   _mulle_aba_print( struct _mulle_aba *p)
{
   _mulle_aba_world_pointer_t      world_p;

   world_p = _mulle_aba_storage_get_world_pointer( &p->storage);
   _mulle_aba_print_world_pointer( world_p);
}



void   mulle_aba_print( void)
{
   static pthread_mutex_t  print_lock;
   static int              hate;
   
   if( ! hate)
   {
      ++hate;
      pthread_mutex_init( &print_lock, NULL);
   }
   
   assert( global);
   if( pthread_mutex_lock( &print_lock))
      abort();

   _mulle_aba_print( global);

   pthread_mutex_unlock( &print_lock);
}

#endif

