//
//  mulle_aba_storage.c
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
#include "mulle-aba-storage.h"

#include "mulle-aba-defines.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_SWAP || STATE_STATS || ! defined( NDEBUG)
static int   _mulle_aba_worldpointer_state( _mulle_aba_worldpointer_t world_p);
#endif


void   _mulle_aba_freeentry_set( struct _mulle_aba_freeentry *entry,
                                  void (*p_free)( void *, void *),
                                  void *pointer,
                                  void *owner)
{
#if MULLE_ABA_MEMTYPE_DEBUG
   entry->_memtype = _mulle_aba_freeentry_memtype;
#endif
   entry->_owner   = owner;
   entry->_pointer = pointer;
   entry->_free    = p_free;
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
   fprintf( stderr, "%s: alloc linked list entry %p (for payload %p/%p)\n",
                        mulle_aba_thread_name(), entry, owner, pointer);
#endif
}


#if DEBUG
static int   print( struct _mulle_aba_freeentry   *entry,
                    struct _mulle_aba_freeentry   *prev,
                    void *userinfo)
{
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
   fprintf( stderr, "%s:   %s[%p=%p]\n", mulle_aba_thread_name(),
                        prev ? "->" : "", entry, entry->_pointer);
#else
   fprintf( stderr, "   %s[%p=%p]\n", prev ? "->" : "", entry, entry->_pointer);
#endif
   return( 0);
}


void   _mulle_concurrent_linkedlist_print( struct _mulle_concurrent_linkedlist *p)
{
   _mulle_concurrent_linkedlist_walk( p, (void *)  print, NULL);
}
#endif


#pragma mark - timestamp storage

struct _mulle_aba_timestampstorage *
   _mulle_aba_timestampstorage_alloc( struct mulle_allocator *allocator)
{
   struct _mulle_aba_timestampstorage   *ts_storage;
   unsigned int                          i;

   ts_storage = mulle_allocator_calloc( allocator, 1, sizeof( *ts_storage));
   if( ! ts_storage)
      return( ts_storage);
#if MULLE_ABA_MEMTYPE_DEBUG
   ts_storage->_memtype = _mulle_aba_timestampstorage_memtype;
#endif

   for( i = 0; i < _mulle_aba_timestampstorage_n_entries; i++)
   {
      _mulle_atomic_pointer_nonatomic_write( &ts_storage->_entries[ i]._retain_count_1, (void *) -1);
   }
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
   fprintf( stderr, "%s: alloc ts_storage %p\n", mulle_aba_thread_name(), ts_storage);
#endif

   return( ts_storage);
}


static void
   _mulle_aba_timestampstorage_empty_assert( struct _mulle_aba_timestampstorage *ts_storage)
{
   unsigned int   i;

   for( i = 0; i < _mulle_aba_timestampstorage_n_entries; i++)
   {
//      assert( ts_storage->_entries[ i]._retain_count_1._nonatomic == (void *) -1);
      assert( ! _mulle_atomic_pointer_nonatomic_read( &ts_storage->_entries[ i]._pointer_list._head.pointer));
   }
}


void   __mulle_aba_timestampstorage_free( struct _mulle_aba_timestampstorage *ts_storage,
                                          struct mulle_allocator *allocator)
{
   if( ! ts_storage)
      return;

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: free ts_storage %p\n", mulle_aba_thread_name(), ts_storage);
#endif

   mulle_allocator_free( allocator, ts_storage);
}


void   _mulle_aba_timestampstorage_free( struct _mulle_aba_timestampstorage *ts_storage,
                                         struct mulle_allocator *allocator)
{
   if( ! ts_storage)
      return;

   _mulle_aba_timestampstorage_empty_assert( ts_storage);
   __mulle_aba_timestampstorage_free( ts_storage, allocator);
}


#pragma mark - delete/free stuff

struct _mulle_aba_worldpointer;

//
// pops off first storage
//
struct _mulle_aba_timestampstorage   *
   _mulle_aba_world_pop_storage( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestampstorage   *storage;
   unsigned int                          n;

   n = world->_n;
   if( ! n)
      return( NULL);

   --n;

   storage = world->_storage[ 0];

   memmove( &world->_storage[ 0],
            &world->_storage[ 1],
            n * sizeof( struct _mulle_aba_timestampstorage *));

   world->_storage[ n] = 0;
   world->_n           = n;
   world->_offset     += _mulle_aba_timestampstorage_n_entries;
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: pop storage (0) %p\n", mulle_aba_thread_name(), storage);
#endif
   return( storage);
}


int  _mulle_aba_world_push_storage( struct _mulle_aba_world *world,
                                    struct _mulle_aba_timestampstorage *ts_storage)
{
   assert( ts_storage);

   if( world->_size == world->_n)
   {
      errno = EACCES;
      assert( 0);
      return( -1);
   }
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: push storage (%u) %p\n", mulle_aba_thread_name(), world->_n, ts_storage);
#endif

   world->_storage[ world->_n++] = ts_storage;
   return( 0);
}


void   _mulle_aba_world_rotate_storage( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestampstorage   *storage;

   assert( world->_n);
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: rotate storage of world %p\n", mulle_aba_thread_name(), world);
#endif
   storage = _mulle_aba_world_pop_storage( world);
   _mulle_aba_world_push_storage( world, storage);
}


unsigned int   _mulle_aba_world_get_timestampstorage_index( struct _mulle_aba_world *world,
                                                            uintptr_t timestamp)
{
   unsigned int  s_index;

   if( timestamp < world->_offset)
   {
      errno = EINVAL;
      assert( 0);
      return( -1);
   }

   timestamp -= world->_offset;
   s_index    = (unsigned int) (timestamp / _mulle_aba_timestampstorage_n_entries);

   if( s_index >= world->_n)
   {
      if( world->_size == world->_n)
      {
         errno = ENOSPC; // that's OK
         assert( timestamp <= world->_timestamp + 1);
         return( -1);
      }

      errno = EINVAL;
      assert( timestamp <= world->_timestamp + 1);
      return( -1);
   }
   return( s_index);
}


struct _mulle_aba_timestampstorage  *
   _mulle_aba_world_get_timestampstorage( struct _mulle_aba_world *world,
                                          uintptr_t timestamp)
{
   unsigned int   ts_index;

   ts_index = _mulle_aba_world_get_timestampstorage_index( world, timestamp);
   if( ts_index == (unsigned int) -1)
      return( NULL);

   return( _mulle_aba_world_get_timestampstorage_at_index( world, ts_index));
}


struct _mulle_aba_timestampentry  *
   _mulle_aba_world_get_timestampentry( struct _mulle_aba_world *world,
                                        uintptr_t timestamp)
{
   struct _mulle_aba_timestampstorage  *ts_storage;

   if( ! timestamp)
   {
      errno = EINVAL;
      return( NULL);
   }

   ts_storage = _mulle_aba_world_get_timestampstorage( world, timestamp);
   if( ! ts_storage)
   {
      errno = EINVAL;
      return( NULL);
   }

   return( _mulle_aba_timestampstorage_get_timestampentry( ts_storage, timestamp));
}


static void   _mulle_aba_world_copy( struct _mulle_aba_world *dst,
                                     struct _mulle_aba_world *src)
{
   size_t   bytes;

   bytes = (char *) &dst->_storage[ src->_size] - (char *) (&dst->_link._next + 1);
   memcpy( (&dst->_link._next + 1), (&src->_link._next + 1), bytes);

   dst->_link._next       = 0;
#if MULLE_ABA_MEMTYPE_DEBUG
   dst->_generation = src->_generation + 1;
#endif
}


# pragma mark -
# pragma mark Debug

#ifndef NDEBUG
struct _mulle_aba_timestamp_range
{
   uintptr_t   start;
   uintptr_t   length;
};

static int   timestamp_range_contains_timestamp( struct _mulle_aba_timestamp_range range,
                                                 uintptr_t timestamp)
{
   return( timestamp < range.start + range.length && timestamp >= range.start);
}


static struct _mulle_aba_timestamp_range
   _mulle_aba_world_get_timestamp_range( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestamp_range   range;

/*   if( ! world->_n)
   {
      range.start  = 0;
      range.length = 0;
      return( range);
   }
*/
   range.start  = world->_offset;
   range.length = world->_n * _mulle_aba_timestampstorage_n_entries;
   if( ! world->_offset)
   {
      range.start = 1;
      range.length--;
   }
   return( range);
}
#endif


void   _mulle_aba_world_assert_sanity( struct _mulle_aba_world *world)
{
#ifndef NDEBUG
   struct _mulle_aba_timestamp_range     range;

   // rc = 0, not valid except when _max_timestamp == 0
   assert( world->_n_threads || ! world->_timestamp);

   // _offset must be 0 when _max_timestamp is 0
   assert( world->_timestamp || ! world->_offset);

   assert( world->_size);
   assert( world->_n <= world->_size);

   range = _mulle_aba_world_get_timestamp_range( world);
   assert( ! world->_timestamp  || timestamp_range_contains_timestamp( range, world->_timestamp));

   assert( ! world->_timestamp || _mulle_aba_world_get_timestampentry( world, world->_timestamp));
#endif
}


#pragma mark - init/done

int   _mulle_aba_storage_init( struct _mulle_aba_storage *q,
                               struct mulle_allocator *allocator)
{
   struct _mulle_aba_world   *world;

   assert( q);
   assert( allocator);

   memset( q, 0, sizeof( *q));

   q->_allocator = allocator;
#if DEBUG
   world = _mulle_aba_storage_alloc_world( q, 1); // + 64 byte for storage...
#else
   world = _mulle_aba_storage_alloc_world( q, 64 / sizeof( void *)); // + 64 byte for storage...
#endif
   if( ! world)
   {
      assert( 0);
      return( -1);
   }

   _mulle_atomic_pointer_nonatomic_write( &q->_world.pointer, world);

   return( 0);
}

void   _mulle_aba_storage_done( struct _mulle_aba_storage *q)
{
   _mulle_aba_worldpointer_t   world_p;
   struct _mulle_aba_world     *world;

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: done with storage %p\n", mulle_aba_thread_name(), q);
#endif
   world_p = _mulle_aba_storage_get_worldpointer( q);
   world   = mulle_aba_worldpointer_get_struct( world_p);

   // in dire circumstances, there can be some left overs  (see [^1] other file)
   _mulle_aba_storage_free_leak_worlds( q);
   _mulle_aba_storage_free_world( q, world);

   _mulle_aba_storage_free_unused_worlds( q);
   _mulle_aba_storage_free_unused_free_entries( q);

#ifndef NDEBUG
   memset( q, 0xAF, sizeof( *q));
#endif
}


#pragma mark -

struct _mulle_aba_freeentry
   *_mulle_aba_storage_alloc_freeentry( struct _mulle_aba_storage *q)
{
   struct _mulle_aba_freeentry  *entry;

   entry = (void *) _mulle_concurrent_linkedlist_remove_one( &q->_free_entries);
   if( entry)
   {
      entry->_link._next = 0;
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST
      fprintf( stderr, "%s: reuse cached list entry %p\n", mulle_aba_thread_name(), entry);
#endif
      assert( entry->_free == NULL);
      assert( entry->_owner == NULL);
      assert( entry->_pointer == NULL);
   }
   else
   {
      entry = mulle_allocator_calloc( q->_allocator, 1, sizeof( *entry));
      if( ! entry)
         return( NULL);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST || MULLE_ABA_TRACE_ALLOC
      fprintf( stderr, "%s: allocated list entry %p\n", mulle_aba_thread_name(), entry);
#endif
      assert( entry->_link._next == NULL);
      assert( entry->_free == NULL);
      assert( entry->_owner == NULL);
      assert( entry->_pointer == NULL);
   }

#if MULLE_ABA_MEMTYPE_DEBUG
   entry->_memtype = _mulle_aba_freeentry_memtype;
#endif
   return( entry);
}


void   _mulle_aba_storage_free_freeentry( struct _mulle_aba_storage *q,
                                          struct _mulle_aba_freeentry  *entry)
{
   memset( entry, 0, sizeof( *entry));
   _mulle_concurrent_linkedlist_add( &q->_free_entries, (void *) entry);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE || MULLE_ABA_TRACE_LIST
   fprintf( stderr, "%s: put entry %p on reuse chain %p\n",
                        mulle_aba_thread_name(),
                        entry,
                        &q->_free_entries);
#endif
}


static int  free_pointer_and_entry( struct _mulle_aba_freeentry *entry,
                                    struct _mulle_aba_freeentry *prev,
                                    void *userinfo)
{
   struct _mulle_aba_storage *q;

   q = userinfo;
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST
   fprintf( stderr, "%s: free linked list entry %p (for %p)\n", mulle_aba_thread_name(), entry, entry->_pointer);
#endif
   // leak worlds occur sometimes multiple times fix that
   (*entry->_free)( entry->_pointer, entry->_owner);
   _mulle_aba_storage_free_freeentry( q, entry);
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE || MULLE_ABA_TRACE_LIST
   fprintf( stderr, "%s: added entry %p (%p) to reuse storage %p (%p)\n",
                        mulle_aba_thread_name(),
                        entry,
                        entry->_link._next,
                        &q->_free_entries,
                        _mulle_atomic_pointer_nonatomic_read( &q->_free_entries._head.pointer));
#endif
   return( 0);
}


static void   _mulle_aba_storage_linkedlist_free( struct _mulle_aba_storage *q,
                                                  struct _mulle_concurrent_linkedlist  *list)
{
   assert( q);
   assert( list);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST
   fprintf( stderr, "%s: free linked list %p\n", mulle_aba_thread_name(), list);
#endif
   _mulle_concurrent_linkedlist_walk( list, (void *) free_pointer_and_entry, q);
}


#pragma mark -

struct _mulle_aba_world   *_mulle_aba_storage_alloc_world( struct _mulle_aba_storage *q,
                                                           unsigned int size)
{
   struct _mulle_aba_world   *world;

   assert( size);

   for(;;)
   {
      world = (void *) _mulle_concurrent_linkedlist_remove_one( &q->_free_worlds);
      if( ! world)
         break;

      if( world->_size >= size)
      {
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
         fprintf( stderr, "%s: reuse cached world %p\n", mulle_aba_thread_name(), world);
#endif
         return( world);
      }

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
      fprintf( stderr, "%s: free a cached world %p thats too small for us\n", mulle_aba_thread_name(), world);
#endif
      _mulle_allocator_free( q->_allocator, world);
   }

   world = mulle_allocator_calloc( q->_allocator,
                                   1,
                                   sizeof( struct _mulle_aba_world) +
                                   sizeof( struct _mulle_aba_timestampstorage *) * size);

   if( world)
   {
#if MULLE_ABA_MEMTYPE_DEBUG
      world->_memtype = _mulle_aba_world_memtype;
#endif
      world->_size = size;
      assert( ! world->_link._next);
#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( world);
#endif
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_ALLOC
      fprintf( stderr, "%s: alloc world %p\n", mulle_aba_thread_name(), world);
#endif
   }
   return( world);
}


void   _mulle_aba_storage_free_world( struct _mulle_aba_storage *q,
                                      struct _mulle_aba_world *world)
{
   assert( q);
   if( ! world)
      return;

   memset( world, 0, _mulle_aba_world_get_size( world));

   _mulle_concurrent_linkedlist_add( &q->_free_worlds, (void *) world);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: put world %p on reuse chain %p\n",
                    mulle_aba_thread_name(),
                    world,
                    &q->_free_worlds);
#endif
}


MULLE__ABA_GLOBAL
void   _mulle_aba_storage_owned_pointer_free_world( struct _mulle_aba_world *world,
                                                    struct _mulle_aba_storage *q)
{
   assert( q);
   if( ! world)
      return;

   memset( world, 0, _mulle_aba_world_get_size( world));

   _mulle_concurrent_linkedlist_add( &q->_free_worlds, (void *) world);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: put world %p on reuse chain %p\n",
                    mulle_aba_thread_name(),
                    world,
                    &q->_free_worlds);
#endif
}



static int  free_world( struct _mulle_aba_world *world,
                        struct _mulle_aba_world *prev,
                        void *userinfo)
{
   struct mulle_allocator   *allocator;

   allocator = userinfo;
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: free world %p\n", mulle_aba_thread_name(), world);
#endif
   mulle_allocator_free( allocator, world);
   return( 0);
}


void   _mulle_aba_storage_free_leak_worlds( struct _mulle_aba_storage *q)
{
   struct _mulle_concurrent_linkedlist        list;
   struct _mulle_concurrent_linkedlistentry  *entry;

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: freeing leaked worlds %p of storage %p\n", mulle_aba_thread_name(), &q->_leaks, q);
#endif
   entry = _mulle_concurrent_linkedlist_remove_all( &q->_leaks);
   _mulle_atomic_pointer_write( &list._head.pointer, entry);

   _mulle_concurrent_linkedlist_walk( &list, (void *) free_world, q->_allocator);
}


void   _mulle_aba_storage_free_unused_worlds( struct _mulle_aba_storage *q)
{
   struct _mulle_concurrent_linkedlist        list;
   struct _mulle_concurrent_linkedlistentry  *entry;

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: freeing unused worlds %p of storage %p\n", mulle_aba_thread_name(), &q->_free_worlds, q);
#endif
   entry = _mulle_concurrent_linkedlist_remove_all( &q->_free_worlds);
   _mulle_atomic_pointer_write( &list._head.pointer, entry);

   _mulle_concurrent_linkedlist_walk( &list, (void *) free_world, q->_allocator);
}


static int  free_entry( struct _mulle_aba_freeentry *entry,
                       struct _mulle_aba_freeentry *prev,
                       void *userinfo)
{
   struct mulle_allocator   *allocator;

   allocator = userinfo;
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: free unused linked list entry %p\n", mulle_aba_thread_name(), entry);
#endif
   mulle_allocator_free( allocator, entry);
   return( 0);
}


void   _mulle_aba_storage_free_unused_free_entries( struct _mulle_aba_storage *q)
{
   struct _mulle_concurrent_linkedlist        list;
   struct _mulle_concurrent_linkedlistentry  *entry;

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_LIST || MULLE_ABA_TRACE_FREE
   fprintf( stderr, "%s: freeing unused entries %p of storage %p\n", mulle_aba_thread_name(), &q->_free_entries, q);
#endif
   entry = _mulle_concurrent_linkedlist_remove_all( &q->_free_entries);
   _mulle_atomic_pointer_write( &list._head.pointer, entry);

   _mulle_concurrent_linkedlist_walk( &list, (void *) free_entry, q->_allocator);
}


#pragma mark - world changing code

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_SWAP
static char  *_mulle_swap_intent_name(enum _mulle_swap_intent intention )
{
   switch( intention)
   {
   case _mulle_swap_register_intent   : return( "register");
   case _mulle_swap_unregister_intent : return( "unregister");
   case _mulle_swap_free_intent       : return( "free");
   case _mulle_swap_checkin_intent    : return( "checkin");
   case _mulle_swap_lock_intent       : return( "lock");
   case _mulle_swap_unlock_intent     : return( "unlock");
   }
}
#endif


#ifndef NDEBUG

static inline void  assert_valid_transition(  _mulle_aba_worldpointer_t new_world_p,
                                              _mulle_aba_worldpointer_t old_world_p,
                                               enum _mulle_swap_intent intention)
{
   uintptr_t   new_state;
   uintptr_t   old_state;
//   struct _mulle_aba_world   *new_world; // just for debugging
//   struct _mulle_aba_world   *old_world; // just for debugging
//
//   // two lines just for debugging
//   new_world = mulle_aba_worldpointer_get_struct( new_world_p);
//   old_world = mulle_aba_worldpointer_get_struct( old_world_p);

   // when the intent is locking, we can't read the world contents
   if( intention == _mulle_swap_lock_intent)
   {
      new_state = (uintptr_t) new_world_p & 0x3;
      old_state = (uintptr_t) old_world_p & 0x3;
   }
   else
   {
      new_state = _mulle_aba_worldpointer_state( new_world_p);
      old_state = _mulle_aba_worldpointer_state( old_world_p);
   }

   switch( old_state)
   {
   case 0 :
      assert( (new_state == 2 && intention == _mulle_swap_lock_intent));
      break;

   case 1 :
      assert( (new_state == 3 && intention == _mulle_swap_lock_intent));
      break;

   case 2 :
      assert( (new_state == 0 && intention == _mulle_swap_unlock_intent) ||
              (new_state == 1 && intention == _mulle_swap_register_intent));
      break;

   case 3 :
      assert( (new_state == 1 && intention == _mulle_swap_unlock_intent) ||
              (new_state == 2 && intention == _mulle_swap_unregister_intent) ||
              (new_state == 9 && intention == _mulle_swap_register_intent));
      break;

   case 5 :
      assert( (new_state ==  7 && intention == _mulle_swap_lock_intent) ||
              (new_state ==  5 && intention == _mulle_swap_checkin_intent));
      break;

   case 7 :
      assert( (new_state ==  2 && intention == _mulle_swap_unregister_intent) ||
              (new_state ==  5 && intention == _mulle_swap_unlock_intent) ||
              (new_state ==  5 && intention == _mulle_swap_checkin_intent) ||
              (new_state == 13 && intention == _mulle_swap_register_intent));
      break;

   case 9 :
      assert( (new_state == 11 && intention == _mulle_swap_lock_intent) ||
              (new_state == 12 && intention == _mulle_swap_free_intent));
      break;

   case 11 :
      assert( (new_state ==  1 && intention == _mulle_swap_unregister_intent) ||
              (new_state ==  9 && intention == _mulle_swap_unlock_intent) ||
              (new_state ==  9 && intention == _mulle_swap_unregister_intent) ||
              (new_state ==  9 && intention == _mulle_swap_register_intent) ||
              (new_state == 12 && intention == _mulle_swap_free_intent));
      break;

   case 12 :
      assert( (new_state == 13 && intention == _mulle_swap_checkin_intent) ||
              (new_state == 14 && intention == _mulle_swap_lock_intent));
      break;

   case 13 :
      assert( (new_state == 12 && intention == _mulle_swap_free_intent) ||
              (new_state == 13 && intention == _mulle_swap_checkin_intent) ||
              (new_state == 15 && intention == _mulle_swap_lock_intent));
      break;

   case 14 :
      assert( (new_state ==  5 && intention == _mulle_swap_unregister_intent) ||
              (new_state == 12 && intention == _mulle_swap_unlock_intent) ||
              (new_state == 13 && intention == _mulle_swap_register_intent) ||
              (new_state == 13 && intention == _mulle_swap_unregister_intent) ||
              (new_state == 13 && intention == _mulle_swap_checkin_intent) ||
              (new_state == 14 && intention == _mulle_swap_free_intent));
      break;

   case 15 :
      assert( (new_state ==  5 && intention == _mulle_swap_unregister_intent) ||
              (new_state == 12 && intention == _mulle_swap_free_intent) ||
              (new_state == 13 && intention == _mulle_swap_unlock_intent) ||
              (new_state == 13 && intention == _mulle_swap_checkin_intent) ||
              (new_state == 13 && intention == _mulle_swap_register_intent) ||
              (new_state == 13 && intention == _mulle_swap_unregister_intent));
      break;

   default :
      assert( 0);
   }
}
#endif


#if STATE_STATS
unsigned long   mulle_transitions_count[ 16][ 16];

void   mulle_transitions_count_print_dot()
{
   unsigned int   i, j;

   printf( "digraph\n{\n");
   for( i = 0; i < 16; i++)
      for( j = 0; j < 16; j++)
      {
         if( mulle_transitions_count[ i][ j])
            printf( "%d -> %d [ label=\" %ld\" ];\n", i, j, mulle_transitions_count[ i][ j]);
      }
   printf( "}\n");
}
#endif


static inline void  assert_swap_worlds( enum _mulle_swap_intent intention,
                                       _mulle_aba_worldpointer_t new_world_p,
                                       _mulle_aba_worldpointer_t old_world_p)
{
#ifndef NDEBUG

   struct _mulle_aba_world   *new_world;
   struct _mulle_aba_world   *old_world;

   new_world = mulle_aba_worldpointer_get_struct( new_world_p);
   old_world = mulle_aba_worldpointer_get_struct( old_world_p);
#endif

#if MULLE_ABA_TRACE_SWAP
   fprintf( stderr, "\n%s:     %s swap %p -> %p\n", mulle_aba_thread_name(), _mulle_swap_intent_name( intention),
           old_world_p,
           new_world_p);
#endif

#ifndef NDEBUG
   assert( new_world_p);
   assert( old_world_p);
   assert( new_world_p != old_world_p);

   assert_valid_transition( new_world_p, old_world_p, intention);

   if( intention != _mulle_swap_lock_intent)
   {
#if DEBUG
      assert( ! new_world->_link._next || (new_world == old_world && intention == _mulle_swap_checkin_intent));
      assert( new_world->_n_threads < 10000 && old_world->_n_threads < 10000); // 2016: 10000 threads ? no way
#endif
#if MULLE_ABA_MEMTYPE_DEBUG
      assert( new_world->_memtype == _mulle_aba_world_memtype);
      assert( old_world->_memtype == _mulle_aba_world_memtype);
      assert( new_world->_generation == old_world->_generation + 1 || new_world == old_world);
#endif

      // do not allow an implicit unlock, to keep the world pointer
      if( mulle_aba_worldpointer_get_lock( old_world_p) && intention != _mulle_swap_unlock_intent)
         assert( new_world != old_world);

      //
      // if bit was set and will be cleared, make sure timestamp
      // or number of threads has changed (but not both at same time)
      //
      if( ((intptr_t) old_world_p & 1) && ! ((intptr_t) new_world & 1))
         if( old_world != new_world)
            assert( (old_world->_timestamp != new_world->_timestamp ||
                  ! new_world->_timestamp) ^ (new_world->_timestamp && old_world->_n_threads != new_world->_n_threads) ||
                intention == _mulle_swap_checkin_intent);

      //
      // conversely make sure timestamp stays unchanged, when bit hasn't been set
      // (except timestamp changes to 0)
      //
      if( ! ((intptr_t) old_world_p & 1))
         assert( old_world->_timestamp == new_world->_timestamp || new_world->_timestamp == 0 || old_world->_timestamp == 0);

      // make sure timestamp only increases
      assert( ! mulle_aba_worldpointer_get_struct( new_world_p)->_timestamp ||   (mulle_aba_worldpointer_get_struct( new_world_p)->_timestamp >=  mulle_aba_worldpointer_get_struct( old_world_p)->_timestamp));
   }
#endif
}


static inline void  log_swap_worlds( enum _mulle_swap_intent intention,
                                     _mulle_aba_worldpointer_t new_world_p,
                                     _mulle_aba_worldpointer_t old_world_p,
                                     int rval)
{
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_SWAP
    struct timespec cpu_time;
#ifdef __linux__
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &cpu_time);
#else
   uint64_t   mach_absolute_time(void);
   // just display ticks, sec, nsec makes no sense
   uint64_t   ticks = mach_absolute_time();

   cpu_time.tv_sec  = ticks >> 31;
   cpu_time.tv_nsec = ticks & 0x7FFFFFFF;
#endif

   if( rval)  // if successful, we can read the state
      fprintf( stderr, "%s: @+@ %s swap %p (%d) -> %p (%d) @cputime %ld.%ld\n", mulle_aba_thread_name(), _mulle_swap_intent_name( intention),
              old_world_p, _mulle_aba_worldpointer_state( old_world_p),
              new_world_p, _mulle_aba_worldpointer_state( new_world_p),
              cpu_time.tv_sec,cpu_time.tv_nsec);
   else
      fprintf( stderr, "%s: @-@ FAILED %s swap %p -> %p\n", mulle_aba_thread_name(), _mulle_swap_intent_name( intention),
              old_world_p,
              new_world_p);
#endif

// this only works with guard malloc, if addresses aren't reused
// #ifndef NDEBUG
// {
//    static struct
//    {
//       _mulle_aba_worldpointer_t    world_p;
//       mulle_thread_t               thread;
//    } last_world_ps[ 0x20];
//    static mulle_atomic_pointer_t   last_world_ps_index;
//    unsigned int                    i;
//
//    if( rval)
//    {
// //      for( i = 0; i < 0x20; i++)
// //         if( mulle_thread_self() != last_world_ps[ i].thread)
// //            assert( new_world_p != last_world_ps[ i].world_p);
//
//       i = ((uintptr_t) _mulle_atomic_pointer_read( &last_world_ps_index)) & 0x1F;
//       last_world_ps[ i].world_p = new_world_p;
//       last_world_ps[ i].thread  = mulle_thread_self();
//       _mulle_atomic_pointer_increment( &last_world_ps_index);
//    }
// }
// #endif

#if STATE_STATS
   mulle_transitions_count[ _mulle_aba_worldpointer_state( old_world_p)][ _mulle_aba_worldpointer_state( new_world_p)]++;
#endif
}


//
// TODO: if we'd return the actual read, we could later save the next read
//
static inline int
   _mulle_aba_storage_swap_worlds( struct _mulle_aba_storage *q,
                                   enum _mulle_swap_intent intention,
                                   _mulle_aba_worldpointer_t new_world_p,
                                   _mulle_aba_worldpointer_t old_world_p)
{
   int   rval;

   assert_swap_worlds( intention, new_world_p, old_world_p);

   rval = _mulle_atomic_pointer_cas( &q->_world.pointer, new_world_p, old_world_p);

   log_swap_worlds( intention, new_world_p, old_world_p, rval);

   return( rval ? 0 : -1);
}


struct _mulle_aba_worldpointers
   _mulle_aba_storage_change_worldpointer( struct _mulle_aba_storage *q,
                                           enum _mulle_swap_intent intention,
                                           _mulle_aba_worldpointer_t old_world_p,
                                           int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                           void *userinfo)
{
   _mulle_aba_worldpointer_t         new_world_p;
   struct _mulle_aba_callback_info   ctxt;
   int                               rval;

   goto start;

   do
   {
      old_world_p     = _mulle_aba_storage_get_worldpointer( q);
start:
      ctxt.old_locked = mulle_aba_worldpointer_get_lock( old_world_p);

      //
      // changing bits on the a locked world is not good, except if we unlock
      // If intent is to lock, we should fail in callback
      //
      if( ctxt.old_locked && intention != _mulle_swap_unlock_intent)
      {
         errno = EWOULDBLOCK;  // special error, means redo with new world
         return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      ctxt.old_world  = mulle_aba_worldpointer_get_struct( old_world_p);
      ctxt.old_bit    = mulle_aba_worldpointer_get_bit( old_world_p);
      ctxt.new_locked = 0;

      ctxt.new_world  = ctxt.old_world;
      ctxt.new_bit    = ctxt.old_bit;

      rval = (*callback)( _mulle_aba_world_modify, &ctxt, userinfo);
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
      if( rval)
      {
         errno = rval;
         return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      new_world_p = mulle_aba_worldpointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == old_world_p)
         break;

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( new_world_p));
#endif
   }
   while( _mulle_aba_storage_swap_worlds( q, intention, new_world_p, old_world_p));

   return( _mulle_aba_worldpointers_make( new_world_p, old_world_p));
}


struct _mulle_aba_worldpointers
   _mulle_aba_storage_change_worldpointer_2( struct _mulle_aba_storage *q,
                                              enum _mulle_swap_intent intention,
                                              _mulle_aba_worldpointer_t  old_world_p,
                                              int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                              void *userinfo)
{
   struct _mulle_aba_worldpointers   world_ps;

   world_ps = _mulle_aba_storage_change_worldpointer( q, intention, old_world_p, callback, userinfo);
   if( world_ps.new_world_p || errno != EWOULDBLOCK)
      return( world_ps);

   world_ps = _mulle_aba_storage_copy_change_worldpointer( q, intention, old_world_p, callback, userinfo);
   return( world_ps);
}

/*
 *
 */
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_SWAP || STATE_STATS || ! defined( NDEBUG)

static int   _mulle_aba_worldpointer_state( _mulle_aba_worldpointer_t world_p)
{
   int                                  state;
   struct _mulle_aba_world   *world;

   state = (int) ((uintptr_t) world_p & 0x3);  // get bit and lock
   world = mulle_aba_worldpointer_get_struct( world_p);
   if( world->_timestamp != 0)
      state += 4;
   if( world->_n_threads > 1)
      state += 8;
   return( state);
}

#endif


_mulle_aba_worldpointer_t
   _mulle_aba_storage_try_set_lock_worldpointer( struct _mulle_aba_storage *q,
                                                 _mulle_aba_worldpointer_t old_world_p,
                                                 int bit)
{
   _mulle_aba_worldpointer_t   new_world_p;
   _mulle_aba_worldpointer_t   last_world_p;
   struct _mulle_aba_world     *old_world;
   enum _mulle_swap_intent     intention;

   // this is the world we are trying to lock
   intention = bit ? _mulle_swap_lock_intent : _mulle_swap_unlock_intent;
   old_world = mulle_aba_worldpointer_get_struct( old_world_p);
   for(;;)
   {
      new_world_p = mulle_aba_worldpointer_set_lock( old_world_p, bit);

      //
      // if already there, hmm when unlocking it's ok
      // in the case of the lock, that's bad
      //
      if( mulle_aba_worldpointer_get_lock( old_world_p) == bit)
      {
         log_swap_worlds( intention, new_world_p, old_world_p, 0);
         return( NULL);
      }

      assert_swap_worlds( intention, new_world_p, old_world_p);

      last_world_p = __mulle_atomic_pointer_cas( &q->_world.pointer, new_world_p, old_world_p);

      log_swap_worlds( intention, new_world_p, old_world_p, last_world_p == old_world_p);

      if( last_world_p == old_world_p)
         return( new_world_p);  // bingo


      // different world now ? fail
      if( mulle_aba_worldpointer_get_struct( last_world_p) != old_world)
         return( NULL);

      // retry with this world pointer still pointing to the world we want to
      // lock/unlock

      old_world_p = last_world_p;
   };
}


static struct _mulle_aba_worldpointers
   _mulle_aba_storage_lock_worldpointer( struct _mulle_aba_storage *q)
{
   _mulle_aba_worldpointer_t   new_world_p;
   _mulle_aba_worldpointer_t   old_world_p;

   for(;;)
   {
      old_world_p = _mulle_aba_storage_get_worldpointer( q);
      new_world_p = _mulle_aba_storage_try_lock_worldpointer( q, old_world_p);
      if( new_world_p)
         return( _mulle_aba_worldpointers_make( new_world_p, old_world_p));

      // https://www.realworldtech.com/forum/?threadid=189711&curpostid=189752
      // TODO: doing locking fundamentally wrong here
      // Intent here was, I think, to be nice on single core CPUs
      // Could be a contention if 1000 threads checkin constantly.
      // The checkin is supposed to be done when the thread is idle..
      // Could probably "just" use a lock here.
      mulle_thread_yield();
   }
}

//
// this is just used for threads registering
//
struct _mulle_aba_worldpointers
   _mulle_aba_storage_copy_lock_worldpointer( struct _mulle_aba_storage *q,
                                               int (*callback)( int, struct _mulle_aba_callback_info *, void *) ,
                                               void *userinfo,
                                               struct _mulle_aba_world **dealloced)
{
   _mulle_aba_worldpointer_t         locked_world_p;
   _mulle_aba_worldpointer_t         new_world_p;
   int                               fail;
   int                               rval;
   struct _mulle_aba_callback_info   ctxt;
   struct _mulle_aba_world           *tofree;
   struct _mulle_aba_worldpointers   world_ps;

   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;

   *dealloced = NULL;

   for(;;) // loops is just for debugging
   {
      world_ps       = _mulle_aba_storage_lock_worldpointer( q);

      /* set locked_world_p to the locked world, on which we work
       */

      locked_world_p  = world_ps.new_world_p;

      ctxt.old_world  = mulle_aba_worldpointer_get_struct( locked_world_p);
      ctxt.old_bit    = mulle_aba_worldpointer_get_bit( locked_world_p);
      ctxt.old_locked = mulle_aba_worldpointer_get_lock( locked_world_p);
      assert( ctxt.old_locked);

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( locked_world_p));
#endif
      if( ! ctxt.new_world || ctxt.new_world->_size != ctxt.old_world->_size)
      {
         if( ctxt.new_world)
         {
            (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
            _mulle_aba_storage_free_world( q, ctxt.new_world);

            MULLE_THREAD_UNPLEASANT_RACE_YIELD();
         }

         ctxt.new_world = _mulle_aba_storage_alloc_world( q, ctxt.old_world->_size);
         if( ! ctxt.new_world)
         {
            // TODO: locked worlds leak...
            return( _mulle_aba_worldpointers_make( NULL, NULL));
         }
      }

      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);

      ctxt.new_bit = ctxt.old_bit;

#ifndef NDEBUG
      assert( ! ctxt.new_world->_link._next);
      _mulle_aba_world_assert_sanity( ctxt.new_world);
      _mulle_aba_world_assert_sanity( ctxt.old_world);
#endif
      rval = (*callback)( _mulle_aba_world_modify, &ctxt, userinfo);
      if( rval)
      {
         // TODO: locked worlds leak...
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      new_world_p = mulle_aba_worldpointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == locked_world_p)
         break;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( new_world_p));
#endif
      fail = _mulle_aba_storage_swap_worlds( q, _mulle_swap_register_intent, new_world_p, locked_world_p);

      //
      // we need to clean up locked worlds, so collect them.
      //
      assert( locked_world_p);

      tofree = mulle_aba_worldpointer_get_struct( locked_world_p);

      //
      // tofree == dealloced:: this can happen, when we locked in and somebody
      // checks in then the lock may get clobbered.
      // This could be avoided, but then the free
      // checkin code wouldn't be lock agnostic anymore, which I would like to
      // avoid for principles sake
      //
      if( tofree != *dealloced)
      {
         assert( ! tofree->_link._next);
         assert( ! new_world_p || mulle_aba_worldpointer_get_struct( new_world_p) != tofree);

#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE
         fprintf( stderr, "%s: add world %p to free list (%p)\n", mulle_aba_thread_name(), tofree, dealloced);
#endif
         tofree->_link._next = (void *) *dealloced;
         *dealloced          = tofree;
      }

      if( ! fail)
         break;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
   }

   return( _mulle_aba_worldpointers_make( new_world_p, locked_world_p));
}


struct _mulle_aba_worldpointers
   _mulle_aba_storage_copy_change_worldpointer( struct _mulle_aba_storage *q,
                                                enum _mulle_swap_intent  intention,
                                                _mulle_aba_worldpointer_t   old_world_p,
                                                int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                                void *userinfo)
{
   _mulle_aba_worldpointer_t         new_world_p;
   int                               rval;
   struct _mulle_aba_callback_info   ctxt;
   int                               cmd;

   assert( old_world_p);

   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;
   cmd             = _mulle_aba_world_modify;
   goto start_with_old_world;

   do
   {
      cmd            = _mulle_aba_world_reuse;
      old_world_p    = _mulle_aba_storage_get_worldpointer( q);
start_with_old_world:
      ctxt.old_world  = mulle_aba_worldpointer_get_struct( old_world_p);
      ctxt.old_bit    = mulle_aba_worldpointer_get_bit( old_world_p);
      ctxt.old_locked = mulle_aba_worldpointer_get_lock( old_world_p);

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( old_world_p));
#endif
      if( ! ctxt.new_world || ctxt.new_world->_size != ctxt.old_world->_size)
      {
         if( ctxt.new_world)
         {
            (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
            _mulle_aba_storage_free_world( q, ctxt.new_world);

            MULLE_THREAD_UNPLEASANT_RACE_YIELD();
         }

         cmd            = _mulle_aba_world_modify;
         ctxt.new_world = _mulle_aba_storage_alloc_world( q, ctxt.old_world->_size);
         if( ! ctxt.new_world)
            return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);

      ctxt.new_bit    = ctxt.old_bit;
      ctxt.new_locked = 0;

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( ctxt.old_world);
      _mulle_aba_world_assert_sanity( ctxt.new_world);
#endif
      rval = (*callback)( cmd, &ctxt, userinfo);
      if( rval)
      {
         (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      new_world_p = mulle_aba_worldpointer_make( ctxt.new_world, ctxt.new_bit);
      if( ctxt.new_locked)
         new_world_p = mulle_aba_worldpointer_set_lock( new_world_p, 1);
      if( new_world_p == old_world_p)
         break;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( new_world_p));
#endif
   }
   while( _mulle_aba_storage_swap_worlds( q, intention, new_world_p, old_world_p));

   return( _mulle_aba_worldpointers_make( new_world_p, old_world_p));
}



struct _mulle_aba_worldpointers
   _mulle_aba_storage_grow_worldpointer( struct _mulle_aba_storage *q,
                                          unsigned int target_size,
                                          int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                          void *userinfo)
{
   _mulle_aba_worldpointer_t        new_world_p;
   _mulle_aba_worldpointer_t        old_world_p;
   int                               rval;
   int                               cmd;
   struct _mulle_aba_callback_info   ctxt;
   unsigned int                      size;

   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;
   do
   {
      old_world_p    = _mulle_aba_storage_get_worldpointer( q);
      ctxt.old_world = mulle_aba_worldpointer_get_struct( old_world_p);
      if( ctxt.old_world->_size >= target_size)
      {
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: discard %p (%p)\n", mulle_aba_thread_name(), ctxt.old_world, callback);
#endif
         (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = EBUSY;
         return( _mulle_aba_worldpointers_make( NULL, NULL));  // someone else did the dirty job
      }

      ctxt.old_bit    = mulle_aba_worldpointer_get_bit( old_world_p);
      ctxt.old_locked = mulle_aba_worldpointer_get_lock( old_world_p);

      if( ! ctxt.new_world)
      {
         ctxt.new_world = _mulle_aba_storage_alloc_world( q, target_size);
         if( ! ctxt.new_world)
            return( _mulle_aba_worldpointers_make( NULL, NULL));
  // fail

         cmd = _mulle_aba_world_modify;
      }
      else
      {
#if MULLE_ABA_TRACE
         fprintf( stderr, "%s: reuse %p (%p)\n", mulle_aba_thread_name(), ctxt.new_world, callback);
#endif
         cmd = _mulle_aba_world_reuse;
      }

      // don't overwrite size
      size = ctxt.new_world->_size;
      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);
      ctxt.new_world->_size = size;
      ctxt.new_bit          = ctxt.old_bit;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      rval = (*callback)( cmd, &ctxt, userinfo);
      if( rval)
      {
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_worldpointers_make( NULL, NULL));
      }

      new_world_p = mulle_aba_worldpointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == old_world_p)
         break;
#ifndef NDEBUG
      _mulle_aba_world_assert_sanity( mulle_aba_worldpointer_get_struct( new_world_p));
#endif
   }
   while( _mulle_aba_storage_swap_worlds( q, _mulle_swap_free_intent, new_world_p, old_world_p));

   return( _mulle_aba_worldpointers_make( new_world_p, old_world_p));
}


int   _mulle_aba_timestampstorage_set_usage_bit( struct _mulle_aba_timestampstorage *ts_storage, unsigned int index, int bit)
{
   uintptr_t   i_mask;
   uintptr_t   usage;
   uintptr_t   expect;

   do
   {
      usage  = (uintptr_t) _mulle_atomic_pointer_read( &ts_storage->_usage_bits);
      expect = usage;
      i_mask = 1UL << index;
      usage  = (usage & ~(i_mask)) | (bit ? i_mask : 0);
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      if( usage == expect)
         break;
   }
   while( ! _mulle_atomic_pointer_weakcas( &ts_storage->_usage_bits, (void *) usage, (void *) expect));

#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: set usage bit (%d/%d) for storage %p: %p -> %p\n",
                           mulle_aba_thread_name(),
                           index, bit,
                           ts_storage, (void *) expect, (void *) usage);
#endif

   return( usage != 0);
}



void   _mulle_aba_world_check_timerange( struct _mulle_aba_world *world,
                                         uintptr_t old,
                                         uintptr_t new,
                                         struct _mulle_aba_storage *q)
{
   struct _mulle_aba_timestampstorage   *ts_storage;
   struct _mulle_aba_timestampentry     *ts_entry;
   uintptr_t                             timestamp;
   unsigned int                          ts_index;
   unsigned int                          index;
   void                                  *rc;
   struct _mulle_concurrent_linkedlist         free_list;

   if( ! world->_timestamp)
      return;

#ifndef NDEBUG
   _mulle_aba_world_assert_sanity( world);
#endif

   for( timestamp = old; timestamp <= new; timestamp++)
   {
      ts_index    = _mulle_aba_world_get_timestampstorage_index( world, timestamp);
      ts_storage  = _mulle_aba_world_get_timestampstorage_at_index( world, ts_index);
      assert( ts_storage);

      index       = mulle_aba_timestampstorage_get_timestamp_index( timestamp);
      ts_entry    = _mulle_aba_timestampstorage_get_timestampentry_at_index( ts_storage, index);
      assert( ts_entry);

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      rc = _mulle_atomic_pointer_decrement( &ts_entry->_retain_count_1);
#if MULLE_ABA_TRACE
      fprintf( stderr, "%s: decrement timestamp ts=%ld to rc=%ld\n",
                              mulle_aba_thread_name(),
                              timestamp,
                              (intptr_t) rc);
#endif
// rc can be negative, it's OK
//      assert( (intptr_t) rc >= 0);
      if( rc)
         continue;

      //
      // snip out pointer list from entry, remove usage bit
      // now if max_timestamp still points into ts_storage it will get reused
      // so we can't reuse it
      //
#if MULLE_ABA_TRACE || MULLE_ABA_TRACE_FREE || MULLE_ABA_TRACE_LIST
      fprintf( stderr,  "\n%s: *** freeing linked list %p of ts=%ld rc=%ld***\n",
                              mulle_aba_thread_name(),
                              &ts_entry->_pointer_list,
                              timestamp,
                              (intptr_t) _mulle_atomic_pointer_read( &ts_entry->_retain_count_1) + 1);
      _mulle_concurrent_linkedlist_print( &ts_entry->_pointer_list);
#endif

      //
      // atomic not needed as we should be single threaded here in terms
      // of ts_entry (not storage though)
      //
      free_list = ts_entry->_pointer_list;
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
      _mulle_atomic_pointer_nonatomic_write( &ts_entry->_pointer_list._head.pointer, NULL);

      _mulle_aba_timestampstorage_set_usage_bit( ts_storage, index, 0);
      // now conceivably ts_storage may be gone, don't access any more

      _mulle_aba_storage_linkedlist_free( q, &free_list);
   }
}


static unsigned int  __mulle_aba_world_reuse_storages( struct _mulle_aba_world *world,
                                                       int counting)
{
   uintptr_t                             ts_index;
   struct _mulle_aba_timestampstorage   *ts_storage;
   unsigned int                          i, n;
   unsigned int                          reused;

   //
   // we can reuse a storage, if max_timestamp doesn't point to it
   // and all bits are cleared (our world that is) in a different world
   // it might just get used. Therefore "sanity checks" might fail, but
   // then this world can't be swapped in.
   //
   reused = 0;
   if( ! world->_timestamp)
      return( reused);

   ts_index = _mulle_aba_world_get_timestampstorage_index( world, world->_timestamp);

   n = world->_n;
   for( i = 0; i < n; i++)
   {
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      if( i == ts_index)
         break;

      ts_storage = _mulle_aba_world_get_timestampstorage_at_index( world, i);
      if( ! ts_storage)
         break;

      if( _mulle_atomic_pointer_read( &ts_storage->_usage_bits))
         continue;

      if( ! counting)
         _mulle_aba_world_rotate_storage( world);
      ++reused;
   }
   return( reused);
}


unsigned int  _mulle_aba_world_reuse_storages( struct _mulle_aba_world *world)
{
   return( __mulle_aba_world_reuse_storages( world, 0));
}


unsigned int  _mulle_aba_world_count_available_reusable_storages( struct _mulle_aba_world *world)
{
   return( __mulle_aba_world_reuse_storages( world, 1));
}
