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

#include "mulle-aba.h"

#include "mulle-aba-defines.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>


#define DEBUG_SINGLE_THREADED  0

# pragma mark -
# pragma mark globals

static struct mulle_aba   *global;
static struct mulle_aba   global_space;


struct mulle_aba   *mulle_aba_get_global( void)
{
   return( global);
}


void   mulle_aba_set_global( struct mulle_aba *p)
{
   global = p;
}

#if defined( MULLE_ABA_TRACE) & (MULLE_ABA_TRACE == 1848)
char  *mulle_aba_thread_name( void)
{
   return( "unknown");
}
#endif


# pragma mark - init/done

int   _mulle_aba_init( struct mulle_aba *p,
                       struct mulle_allocator *allocator)
{
   int   rval;

   /*
    * the automatic destruction is only available for the global API
    */
   if( mulle_thread_tss_create( NULL, &p->timestamp_thread_key))
   {
      perror("mulle_thread_tss_create");
      abort();
   }

   rval = _mulle_aba_storage_init( &p->storage, allocator);
   assert( ! rval);
#if DEBUG_SINGLE_THREADED
   if( ! rval)
   {
      _mulle_aba_worldpointer_t   world_p;
      struct _mulle_aba_world     *world;

      world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
      world   = _mulle_aba_worldpointer_get_struct( world_p);
      world->_retain_count = 1;
   }
#endif

   // convenience for mulle_default_allocator
   if( allocator == &mulle_default_allocator)
      mulle_allocator_set_aba( allocator, p, (void *) _mulle_aba_free_owned_pointer);

   return( rval);
}


void   _mulle_aba_done( struct mulle_aba *p)
{
   if( p->storage._allocator == &mulle_default_allocator)
      mulle_allocator_set_aba( p->storage._allocator, NULL, NULL);
   _mulle_aba_storage_done( &p->storage);
   mulle_thread_tss_free( p->timestamp_thread_key);
}


# pragma mark -
# pragma mark get rid of old worlds

static int   _mulle_lockfree_deallocator_free_world_chain( struct mulle_aba *p,
                                                           struct _mulle_aba_world *old_world)
{
   struct _mulle_aba_world   *tofree;
   struct _mulle_aba_world   *next;
   int                       rval;

   for( tofree = old_world; tofree; tofree = next)
   {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
      fprintf( stderr,  "\n%s: *** free world delayed %p\n", mulle_aba_thread_name(), tofree);
#endif
      next = (struct _mulle_aba_world *) tofree->_link._next;
      assert( next != tofree);

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      rval = _mulle_aba_free_owned_pointer( p,
                                            (void (*)( void *, void *)) _mulle_aba_storage_owned_pointer_free_world,
                                            tofree,
                                            &p->storage);
      if( rval)
         return( rval);
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
   }
   return( 0);
}


static int   _mulle_lockfree_deallocator_free_world_if_needed( struct mulle_aba *p,
                                                               struct _mulle_aba_worldpointers world_ps)
{
   struct _mulle_aba_world   *new_world;
   struct _mulle_aba_world   *old_world;

   new_world = mulle_aba_worldpointer_get_struct( world_ps.new_world_p);
   old_world = mulle_aba_worldpointer_get_struct( world_ps.old_world_p);
   if( new_world == old_world)
      return( 0);

   assert( mulle_aba_worldpointer_get_struct( _mulle_aba_storage_get_worldpointer( &p->storage)) != old_world);

   // don't free locked worlds
   if( mulle_aba_worldpointer_get_lock( world_ps.old_world_p))
   {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
      fprintf( stderr,  "\n%s: ***not freeing locked world %p\n", mulle_aba_thread_name(), old_world);
#endif
      return( 0);
   }

   assert( ! old_world->_link._next);
   return( _mulle_aba_free_owned_pointer( p,
                                          (void (*)( void *, void *))_mulle_aba_storage_owned_pointer_free_world,
                                          old_world,
                                          &p->storage));
}


# pragma mark - register

struct add_context
{
   struct _mulle_aba_timestampstorage  *ts_storage;
   struct mulle_allocator              *allocator;
};


static int   add_thread( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n",
               mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode,
               info->new_world->_timestamp, info->new_world->_n_threads);
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


int   _mulle_aba_is_current_thread_registered( struct mulle_aba *p)
{
   assert( p->timestamp_thread_key != -1);
   return( mulle_thread_tss_get( p->timestamp_thread_key) != NULL);
}


int   mulle_aba_is_registered( void)
{
   assert( global);
   return( _mulle_aba_is_current_thread_registered( global));
}



int   _mulle_aba_register_current_thread( struct mulle_aba *p)
{
   uintptr_t                          timestamp;
   struct _mulle_aba_world            *new_world;
   struct _mulle_aba_world            *dealloced;
   struct _mulle_aba_worldpointers    world_ps;

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
   world_ps  = _mulle_aba_storage_copy_lock_worldpointer( &p->storage, add_thread, NULL, &dealloced);
   if( ! world_ps.new_world_p)
   {
      assert( 0);
      return( -1);
   }

   new_world = mulle_aba_worldpointer_get_struct( world_ps.new_world_p);
   timestamp = new_world->_timestamp + 1;
   if( ! timestamp)
      timestamp = 1;

   assert( p->timestamp_thread_key != -1);
   mulle_thread_tss_set( p->timestamp_thread_key, (void *) timestamp);

   return( _mulle_lockfree_deallocator_free_world_chain( p, dealloced));
}


# pragma mark - check in

static void   _mulle_aba_check_timestamp_range( struct mulle_aba *p, uintptr_t old, uintptr_t new)
{
   _mulle_aba_worldpointer_t   world_p;
   struct _mulle_aba_world     *world;

   world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
   world   = mulle_aba_worldpointer_get_struct( world_p);

   _mulle_aba_world_check_timerange( world, old, new, &p->storage);
}


static int   set_bit( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n", mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode, info->new_world->_timestamp, info->new_world->_n_threads);
#endif
   if( mode == _mulle_aba_world_discard)
      return( 0);

   info->new_bit = 1;
   return( 0);
}


int   _mulle_aba_checkin_current_thread( struct mulle_aba *p)
{
   _mulle_aba_worldpointer_t         old_world_p;
   int                               rval;
   intptr_t                          new;
   intptr_t                          last;
   struct _mulle_aba_world           *old_world;
   struct _mulle_aba_worldpointers   world_ps;

   //
   // if last == 0, thread is not configured...
   // and hasn't freed anything. Therefore is not counted as an active thread
   //
   last = (intptr_t) mulle_thread_tss_get( p->timestamp_thread_key);
   if( ! last)
      return( 0);

   old_world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
   old_world   = mulle_aba_worldpointer_get_struct( old_world_p);
   new         = old_world->_timestamp;

   // last is stored as "_timestamp + 1",
   if( last > new)
      return( 0);

   // Ok we promise not to read the old stuff anymore
   // and with the barrier, that should be enforced
   // TODO: Voodoo or necessity ? Think about it.
   mulle_atomic_memory_barrier();

   world_ps = _mulle_aba_storage_change_worldpointer_2( &p->storage,
                                                        _mulle_swap_checkin_intent,
                                                        old_world_p,
                                                        set_bit,
                                                        NULL);
   if( ! world_ps.new_world_p)
      return( -1);

   MULLE_THREAD_UNPLEASANT_RACE_YIELD();

   rval = _mulle_lockfree_deallocator_free_world_if_needed( p, world_ps);
   if( rval)
      return( -1);

   _mulle_aba_check_timestamp_range( p, last, new);
   mulle_thread_tss_set( p->timestamp_thread_key, (void *) (new + 1));

   //
   return( rval);
}


# pragma mark - unregister

struct remove_context
{
   uintptr_t                 timestamp;
   struct _mulle_aba_world   *locked_world;
};


static int   remove_thread( int mode, struct _mulle_aba_callback_info  *info, void *userinfo)
{
   unsigned int            i;
   unsigned int            size;
   size_t                  bytes;
   struct remove_context   *context;

   context = userinfo;
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (mode=%d, ts=%ld nt=%ld)\n", mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode, info->new_world->_timestamp, info->new_world->_n_threads);
#endif
   assert( info->new_world != info->old_world);

   if( mode == _mulle_aba_world_discard)
      return( 0);

   if( ! info->old_locked)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EAGAIN_1\n", mulle_aba_thread_name());
#endif
      return( EAGAIN);
   }

   // if someone changed something inbetween
   if( info->new_world->_timestamp != context->timestamp)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EAGAIN_2\n", mulle_aba_thread_name());
#endif
      return( EAGAIN);
   }

   if( info->old_world != context->locked_world)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EAGAIN_3\n", mulle_aba_thread_name());
#endif
      return( EAGAIN);
   }

   if( --info->new_world->_n_threads)
   {
      assert( info->new_world->_n_threads > 0);
      info->new_bit = 1;
      return( 0);
   }

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_SWAP
      fprintf( stderr, "%s: I AM THE OMEGA MAN\n", mulle_aba_thread_name()); // last guy in town
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


int   _mulle_aba_unregister_current_thread( struct mulle_aba *p)
{
   _mulle_aba_worldpointer_t         locked_world_p;
   _mulle_aba_worldpointer_t         old_world_p;
   int                               rval;
   intptr_t                          new;
   intptr_t                          last;
   struct _mulle_aba_world           *old_world;
   struct _mulle_aba_world           *locked_world;
   struct _mulle_aba_worldpointers   world_ps;
   struct remove_context             context;
   unsigned int                      i, n;

   /* check in first */
   assert( p->timestamp_thread_key != -1);

   for(;;)
   {
      //
      // if last == 0, thread is not configured...
      //
      last = (intptr_t) mulle_thread_tss_get( p->timestamp_thread_key);
      if( ! last)
         return( 0);

      old_world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
      old_world   = mulle_aba_worldpointer_get_struct( old_world_p);
      new         = old_world->_timestamp;

#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: ::: final check in %ld-%ld\n",
                           mulle_aba_thread_name(),
                           last,
                           new);
#endif
#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( old_world);
#endif

      /* last is the timestamp of the last checkin of this thread, it is
       * not the timestamp of the old_world.
       */
      if( last <= new)
      {
         world_ps = _mulle_aba_storage_change_worldpointer_2( &p->storage, _mulle_swap_checkin_intent, old_world_p, set_bit, NULL);
         if( ! world_ps.new_world_p)
            return( -1);

         old_world_p = world_ps.new_world_p;
         old_world   = mulle_aba_worldpointer_get_struct( old_world_p);

         rval = _mulle_lockfree_deallocator_free_world_if_needed( p, world_ps);
         if( rval)
            return( rval);

#ifndef NDEBUG
         _mulle_aba_world_assert_sanity( old_world);
#endif
         _mulle_aba_world_check_timerange( old_world, last, new, &p->storage);
         mulle_thread_tss_set( p->timestamp_thread_key, (void *) (new + 1));
#ifndef NDEBUG
         _mulle_aba_world_assert_sanity( old_world);
#endif
      }

      locked_world_p = _mulle_aba_storage_try_lock_worldpointer( &p->storage, old_world_p);
      if( ! locked_world_p)
         continue;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      locked_world         = mulle_aba_worldpointer_get_struct( locked_world_p);
      context.timestamp    = new;
      context.locked_world = locked_world;
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: set timestamp to %ld\n", mulle_aba_thread_name(), new);
#endif

      //
      // we can't say (old_world->_n_threads == 1) here
      //
      world_ps = _mulle_aba_storage_copy_change_worldpointer( &p->storage,
                                                              _mulle_swap_unregister_intent,
                                                              locked_world_p,
                                                              remove_thread,
                                                              &context);
      if( world_ps.new_world_p)
         break;

      if( errno == EAGAIN || errno == EBUSY)
      {
         //
         // make sure it's not locked, can happen when remove_thread figures out
         // we need to redo the checkin and fails
         //
         if( ! _mulle_aba_storage_try_unlock_worldpointer( &p->storage, locked_world_p))
         {
            //
            // if we couldn't unlock, we have now a world, that we need to
            // delay release (yay)
            //
            locked_world = mulle_aba_worldpointer_get_struct( locked_world_p);
            rval         = _mulle_lockfree_deallocator_free_world_chain( p, locked_world);
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

   mulle_thread_tss_set( p->timestamp_thread_key, (void *) 0);  // mark as clean now

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
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
      fprintf( stderr, "\n%s: *** %p returns to initial state, removing leaks***\n",
                                 mulle_aba_thread_name(),
                                 locked_world);
#endif
      //
      // get rid of all old world resources, the new world has been cleaned
      // and is not referencing storages.
      // do actual frees after lock ?
      assert( mulle_aba_worldpointer_get_lock( world_ps.new_world_p));

      n = locked_world->_size;
      for( i = 0; i < n; i++)
         __mulle_aba_timestampstorage_free( locked_world->_storage[ i],
                                            p->storage._allocator);

      _mulle_aba_storage_free_leak_worlds( &p->storage);

      assert( ! locked_world->_link._next);

      _mulle_aba_storage_free_world( &p->storage, locked_world);
      _mulle_aba_storage_free_unused_free_entries( &p->storage);

      _mulle_aba_storage_try_unlock_worldpointer( &p->storage, world_ps.new_world_p);
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


# pragma mark - add pointer to free

static int   handle_callback_mode_for_add_context( int mode,
                                                   struct _mulle_aba_callback_info *info,
                                                   struct add_context *context)
{
   assert( info->old_world != info->new_world);

   switch( mode)
   {
   case _mulle_aba_world_discard :
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: discard ts_storage %p\n", mulle_aba_thread_name(), context->ts_storage);
#endif
      _mulle_aba_timestampstorage_free( context->ts_storage, context->allocator);
      context->ts_storage = NULL;
      return( 0);

   case _mulle_aba_world_modify  :
      context->ts_storage = _mulle_aba_timestampstorage_alloc( context->allocator);
      if( ! context->ts_storage)
         return( errno);

         // fall thru

   case _mulle_aba_world_reuse   :  // could make this better, for now just discard and redo
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: %s ts_storage %p at %u in world %p\n", mulle_aba_thread_name(), (mode == _mulle_aba_world_reuse) ? "reuse" : "set", context->ts_storage, info->new_world->_n, info->new_world);
#endif
      assert( info->new_world->_n < info->new_world->_size);
      //
      // if we read in a new world, we expect a zero there
      // if we reuse the previous failed world, there would be our allocated
      // storage
      //
      assert( ! info->new_world->_storage[ info->new_world->_n] ||
                info->new_world->_storage[ info->new_world->_n] == context->ts_storage);

      // fishing for bugs here...
      assert( ! info->new_world->_n ||
                info->new_world->_storage[ info->new_world->_n - 1] != context->ts_storage);

      info->new_world->_storage[ info->new_world->_n] = context->ts_storage;
      info->new_world->_n++;
      break;
   }
   return( 0);
}


static int   pre_check_world_for_timestamp_increment( int mode,
                                                      struct _mulle_aba_callback_info *info,
                                                      struct add_context *context)
{
   assert( info->old_world != info->new_world);
   //
   // if the world has changed to our benefit, cancel and free like usual
   // discard
   if( info->new_world->_n_threads <= 1)
   {
      handle_callback_mode_for_add_context( _mulle_aba_world_discard, info, context);
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EAGAIN_1 (%p)\n", mulle_aba_thread_name(), context->ts_storage);
#endif
      return( EAGAIN);
   }

   //
   // we don't want to set the timestamp, if bit is actually 0
   //
   if( ! info->old_bit)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EBUSY_1 (%p)\n", mulle_aba_thread_name(), context->ts_storage);
#endif
      return( EBUSY);  // just retry
   }

   return( 0);
}


static int   size_check_world_for_timestamp_increment( int mode,
                                                       struct _mulle_aba_callback_info *info,
                                                       struct add_context *context)
{
   assert( info->old_world != info->new_world);

   // if the world is now too small, also bail
   if( (info->new_world->_timestamp - info->new_world->_offset + 1) >= info->new_world->_n * _mulle_aba_timestampstorage_n_entries)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EBUSY_2 (%p)\n", mulle_aba_thread_name(), context->ts_storage);
#endif
      return( EBUSY);
   }
   return( 0);
}


static void   _increment_timestamp( struct _mulle_aba_callback_info  *info)
{
   struct _mulle_aba_timestampstorage   *ts_storage;
   unsigned int                               index;

   assert( info->old_world != info->new_world);

   if( ! ++info->new_world->_timestamp)
      info->new_world->_timestamp = 1;

   ts_storage = _mulle_aba_world_get_timestampstorage( info->new_world, info->new_world->_timestamp);
   index      = mulle_aba_timestampstorage_get_timestamp_index( info->new_world->_timestamp);
   _mulle_aba_timestampstorage_set_usage_bit( ts_storage, index, 1);
   info->new_bit = 0;
}


static int   increment_timestamp( int mode,
                                  struct _mulle_aba_callback_info *info,
                                  void *userinfo)
{
   int                  rval;
   struct add_context   *context;

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (%d)\n", mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode);
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


static int   add_storage_and_increment_timestamp( int mode,
                                                  struct _mulle_aba_callback_info *info,
                                                  void *userinfo)
{
   struct add_context   *context;
   int                  rval;

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (%d)\n", mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode);
#endif
   assert( info->old_world != info->new_world);

   context = userinfo;
   if( mode == _mulle_aba_world_discard)
   {
      _mulle_aba_timestampstorage_free( context->ts_storage, context->allocator);
      context->ts_storage = NULL;
      return( 0);
   }

   rval = pre_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
   {
      // special case code: hackish, sry
      if( mode == _mulle_aba_world_reuse)
      {
         _mulle_aba_timestampstorage_free( context->ts_storage, context->allocator);
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
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EBUSY_3 (%p)\n", mulle_aba_thread_name(), context->ts_storage);
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


static int   reorganize_storage_and_increment_timestamp( int mode,
                                                         struct _mulle_aba_callback_info *info,
                                                         void *userinfo)
{
   int                  rval;
   struct add_context   *context;

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: %s (%d)\n", mulle_aba_thread_name(), __PRETTY_FUNCTION__, mode);
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
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: EBUSY_4 (%p)\n", mulle_aba_thread_name(), context->ts_storage);
#endif
      return( EBUSY);  // hmm nothing to reuse ?
   }

   rval = size_check_world_for_timestamp_increment( mode, info, context);
   if( rval)
      return( rval);

   _increment_timestamp( info);
   return( 0);
}


int   _mulle_aba_free_owned_pointer( struct mulle_aba *p,
                                     void (*p_free)( void *, void *),
                                     void *pointer,
                                     void *owner)
{
   _mulle_aba_worldpointer_t          world_p;
   int                                old_bit;
   struct _mulle_aba_world            *free_worlds;
   struct _mulle_aba_freeentry        *entry;
   struct _mulle_aba_worldpointers    world_ps;
   struct _mulle_aba_world            *world;
   struct _mulle_aba_world            *new_world;
   struct _mulle_aba_world            *next;
   struct _mulle_aba_world            *old_world;
   struct _mulle_aba_timestampentry   *ts_entry;
   struct add_context                 ctxt;
   uintptr_t                          timestamp;

   assert( p);
   assert( p_free);
   assert( pointer);

   assert( _mulle_aba_is_current_thread_registered( p));

   free_worlds = NULL;

   world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
   world   = mulle_aba_worldpointer_get_struct( world_p);

   assert( pointer != world);

   // TODO: move this into loop and combine with EAGAIN code
   if( world->_n_threads <= 1)
   {
#if MULLE_ABA_TRACE
      fprintf( stderr,  "%s: *** p_free immediately, because RC=1 %p***\n", mulle_aba_thread_name(), pointer);
#endif
      (*p_free)( pointer, owner);
      return( 0);
   }

   ctxt.allocator = p->storage._allocator;
   ctxt.ts_storage = NULL;

   for( ;;)
   {
      world     = mulle_aba_worldpointer_get_struct( world_p);
      old_bit   = mulle_aba_worldpointer_get_bit( world_p);
      timestamp = world->_timestamp + old_bit;
      ts_entry  = _mulle_aba_world_get_timestampentry( world, timestamp);

      if( ts_entry)
      {
         if( ! old_bit)
            break;         // preferred and only exit

#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: update timestamp in world %p\n", mulle_aba_thread_name(), world_p);
#endif
         world_ps = _mulle_aba_storage_copy_change_worldpointer( &p->storage,
                                                                 _mulle_swap_free_intent,
                                                                 world_p,
                                                                 increment_timestamp, &ctxt);
      }
      else
         if( world->_n < world->_size)
         {
#if MULLE_ABA_TRACE
            fprintf( stderr, "%s: add storage and set rc in world %p\n", mulle_aba_thread_name(), world_p);
#endif
            // add a storage to the world,
            world_ps = _mulle_aba_storage_copy_change_worldpointer( &p->storage,
                                                                    _mulle_swap_free_intent,
                                                                    world_p,
                                                                    add_storage_and_increment_timestamp,
                                                                    &ctxt);
         }
         else
            if( _mulle_aba_world_count_available_reusable_storages( world))
            {
               //
               // try to reorganize our world to make more room, but we
               // can't set retain count at the same time though
               //
#if MULLE_ABA_TRACE
               fprintf( stderr, "%s: reorganize world %p\n", mulle_aba_thread_name(), world_p);
#endif
               world_ps = _mulle_aba_storage_copy_change_worldpointer( &p->storage,
                                                                       _mulle_swap_free_intent,
                                                                       world_p,
                                                                       reorganize_storage_and_increment_timestamp,
                                                                       &ctxt);
            }
            else
            {
               //
               // grow the available storage pointers and add a storage
               // now if this fails, we have a dangling ts_storage we need to
               // get rid off
#if MULLE_ABA_TRACE
               fprintf( stderr, "%s: grow and set RC in world %p\n", mulle_aba_thread_name(), world_p);
#endif
               world_ps = _mulle_aba_storage_grow_worldpointer( &p->storage,
                                                                world->_size + 1,
                                                                add_storage_and_increment_timestamp,
                                                                &ctxt);
            }

      if( world_ps.new_world_p)
      {
         new_world = mulle_aba_worldpointer_get_struct( world_ps.new_world_p);
         /* successfully incremented timestamp
            now other threads may already be calling checkin
            add n_threads to the rc which was virtually at 0, but now may be
            negative if the result is 0, then...
         */
         ts_entry  = _mulle_aba_world_get_timestampentry( new_world, new_world->_timestamp);
         assert( ts_entry);
         _mulle_atomic_pointer_add( &ts_entry->_retain_count_1, new_world->_n_threads);

         //
         // our thread hasn't checked in, so we know it can't be 0
         // don't add locked worlds to list
         //
         if( ! mulle_aba_worldpointer_get_lock( world_ps.old_world_p))
         {
            old_world = mulle_aba_worldpointer_get_struct( world_ps.old_world_p);
            if( new_world != old_world)
            {
               // old world needs to be freed later
               assert( old_world != free_worlds);
               assert( old_world->_link._next == NULL);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
               fprintf( stderr, "%s: add %p to worlds to free world chain\n", mulle_aba_thread_name(), old_world);
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
         world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
         continue;
      }

      if( errno == EAGAIN)  // went back to single threaded, just discard
      {
#if MULLE_ABA_TRACE
         fprintf( stderr,  "%s: *** p_free immediately, because RC became 1 %p***\n", mulle_aba_thread_name(), pointer);
#endif
         (*p_free)( pointer, owner);

         while( free_worlds)
         {
#if MULLE_ABA_TRACE
            fprintf( stderr,  "\n%s: *** free old world %p immediately***\n", mulle_aba_thread_name(), free_worlds);
#endif
            next = (struct _mulle_aba_world *) free_worlds->_link._next;
            _mulle_allocator_free( p->storage._allocator, free_worlds);
            free_worlds = next;
         }
         return( 0);
      }

      // leaks worlds in this case

      assert( errno);
      return( -1);     // nope: regular error (gotta be ENOMEM)
   }

#ifndef NDEBUG
   _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( world_p));
#endif
   entry = _mulle_aba_storage_alloc_freeentry( &p->storage);
   if( ! entry)
   {
      assert( 0);
      return( -1);
   }

   _mulle_aba_freeentry_set( entry, p_free, pointer, owner);
   _mulle_concurrent_linkedlist_add( &ts_entry->_pointer_list, &entry->_link);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST
   fprintf( stderr,  "\n%s: *** put pointer %p/%p on linked list %p of ts=%ld rc=%ld***\n",
                        mulle_aba_thread_name(),
                        pointer,
                        owner,
                        &ts_entry->_pointer_list,
                        timestamp,
                        (intptr_t) _mulle_atomic_pointer_read( &ts_entry->_retain_count_1) + 1);
#endif


   //
   // now free accumulated worlds on the same pointer list
   //
   while( free_worlds)
   {
      entry = _mulle_aba_storage_alloc_freeentry( &p->storage);
      if( ! entry)
         return( -1);

      next = (struct _mulle_aba_world *) free_worlds->_link._next;
      _mulle_aba_freeentry_set( entry,
                                (void (*)( void *, void *)) _mulle_aba_storage_owned_pointer_free_world,
                                free_worlds,
                                &p->storage);

      _mulle_concurrent_linkedlist_add( &ts_entry->_pointer_list, &entry->_link);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST
      fprintf( stderr,  "\n%s: *** put old world %p on linked list %p of ts=%ld rc=%ld***\n", mulle_aba_thread_name(), free_worlds, &ts_entry->_pointer_list, timestamp, (intptr_t) _mulle_atomic_pointer_read( &ts_entry->_retain_count_1) + 1);
#endif
      free_worlds = next;
   }

#if MULLE_ABA_TRACE
   _mulle_concurrent_linkedlist_print( &ts_entry->_pointer_list);
#endif
   return( 0);
}


# pragma mark - API

// your thread must call this pretty much immediately after creation
void   mulle_aba_register( void)
{
   assert( global);
   if( _mulle_aba_register_current_thread( global))
   {
      perror( "_mulle_aba_register_thread");
      abort();
   }
}


void   mulle_aba_checkin( void)
{
   assert( global);
   _mulle_aba_checkin_current_thread( global);
}


static void    _mulle_aba_no_owner_free( void *pointer, void *owner)
{
   void   (*p_free)( void *);

   p_free = owner;
   (*p_free)( pointer);
}



int   mulle_aba_free( void (*p_free)( void *), void *pointer)
{
   assert( global);

   if( ! pointer)
      return( 0);

   assert( p_free);
   return( _mulle_aba_free_owned_pointer( global,
                                          _mulle_aba_no_owner_free,
                                          pointer,
                                          p_free));
}


int   _mulle_aba_free( struct mulle_aba *p,
                       void (*p_free)( void *),
                       void *pointer)
{
   return( _mulle_aba_free_owned_pointer( p,
                                          _mulle_aba_no_owner_free,
                                          pointer,
                                          p_free));
}


int   mulle_aba_free_owned_pointer( void (*p_free)( void *, void *), void *pointer, void *owner)
{
   assert( global);

   if( ! pointer)
      return( 0);

   assert( p_free);
//   if( ! mulle_aba_get_thread_timestamp())
//      mulle_aba_register_current_thread();

   return( _mulle_aba_free_owned_pointer( global, p_free, pointer, owner));
}


//
// this will be automatically called on thread destruction, so you usually don't
// call this, except if you are the main thread
//
void   mulle_aba_unregister()
{
   assert( global);
   if( _mulle_aba_unregister_current_thread( global))
   {
      perror( "_mulle_aba_unregister_thread");
      abort();
   }
}


#pragma mark - init/done

void   mulle_aba_init( struct mulle_allocator *allocator)
{
   assert( MULLE__THREAD_VERSION    >= ((0 << 20) | (2 << 8) | 0));
   assert( MULLE__ALLOCATOR_VERSION >= ((0 << 20) | (1 << 8) | 0));

   if( ! allocator)
      allocator = &mulle_default_allocator;

   if( ! global)
      global = &global_space;

   assert( global);
   if( _mulle_aba_init( global, allocator))
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


#pragma mark - Testing stuff

// only really useful for testing
void   mulle_aba_reset()
{
   assert( global);
   struct mulle_allocator  *allocator;

   allocator = global->storage._allocator;

   _mulle_aba_done( global);
   _mulle_aba_init( global, allocator);
}


uintptr_t   _mulle_aba_current_thread_get_timestamp( struct mulle_aba *p)
{
   return( (uintptr_t) mulle_thread_tss_get( p->timestamp_thread_key));
}


uintptr_t   mulle_aba_current_thread_get_timestamp( void)
{
   if( ! global)
      return( 0);
   return( _mulle_aba_current_thread_get_timestamp( global));
}


void  *_mulle_aba_get_worldpointer( struct mulle_aba *p)
{
   assert( p);
   return( _mulle_atomic_pointer_read( &p->storage._world.pointer));
}


void  *mulle_aba_get_worldpointer( void)
{
   if( ! global)
      return( NULL);
   return( _mulle_aba_get_worldpointer( global));
}


#if DEBUG
void   mulle_aba_print( void);
void   _mulle_aba_print( struct mulle_aba *p);
void   _mulle_aba_print_worldpointer( _mulle_aba_worldpointer_t world_p);


void   _mulle_aba_print_worldpointer( _mulle_aba_worldpointer_t world_p)
{
   int                                   bit;
   struct _mulle_aba_world               *world;
   struct _mulle_aba_timestampstorage   *ts_storage;
   unsigned int                          i;

   world   = mulle_aba_worldpointer_get_struct( world_p);
   bit     = mulle_aba_worldpointer_get_bit( world_p);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
   fprintf( stderr,  "%s: ", mulle_aba_thread_name());
#endif
   fprintf( stderr,  "world_p %p : { %p, %d }\n", world_p, world, bit);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
   fprintf( stderr,  "%s: ", mulle_aba_thread_name());
#endif
   fprintf( stderr,  "world   %p : { ts=%ld, rc=%lu, of=%ld, n=%u, sz=%u }\n",
                        world,
                        (long) world->_timestamp,
                        (unsigned long) world->_n_threads,
                        (long) world->_offset,
                        (unsigned int) world->_n,
                        (unsigned int) world->_size);

   for( i = 0; i < world->_n; i++)
   {
      ts_storage = world->_storage[ i];
      assert( ts_storage);
      fprintf( stderr,  "    #%u : adr=%p bits=%p\n",
                        i,
                        ts_storage,
                        _mulle_atomic_pointer_read( & ts_storage->_usage_bits));
   }

   if( world->_n)
      fprintf( stderr,  "                      ");
   fprintf( stderr,  "}\n");
}


void   _mulle_aba_print( struct mulle_aba *p)
{
   _mulle_aba_worldpointer_t      world_p;

   world_p = _mulle_aba_storage_get_worldpointer( &p->storage);
   _mulle_aba_print_worldpointer( world_p);
}


void   mulle_aba_print( void)
{
   static mulle_thread_mutex_t  print_lock;
   static int                   hate;

   if( ! hate)
   {
      ++hate;
      mulle_thread_mutex_init( &print_lock);
   }

   assert( global);
   if( mulle_thread_mutex_lock( &print_lock))
      abort();

   _mulle_aba_print( global);

   mulle_thread_mutex_unlock( &print_lock);
}

#endif

