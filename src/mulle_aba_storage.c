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

#include "mulle_aba_storage.h"

#include "mulle_aba_defines.h"
#include <string.h>
#include <stdio.h>
#include <errno.h>


extern char  *pthread_name( void);

void   _mulle_aba_linked_list_entry_set( struct _mulle_aba_linked_list_entry *entry,
                                         void *owner,
                                         void *pointer,
                                         void (*free)( void *, void *))
{
#if DEBUG
   entry->_memtype = _mulle_aba_linked_list_entry_memtype;
#endif
   entry->_owner   = owner;
   entry->_pointer = pointer;
   entry->_free    = free;
#if TRACE || TRACE_ALLOC
   fprintf( stderr, "%s: alloc linked list entry %p (for payload %p/%p)\n", pthread_name(), entry, owner, pointer);
#endif
}


static int   print( struct _mulle_aba_linked_list_entry   *entry,
                    struct _mulle_aba_linked_list_entry   *prev,
                    void *userinfo)
{
   fprintf( stderr, "%s:   %s[%p=%p]\n", pthread_name(), prev ? "->" : "", entry, entry->_pointer);
   return( 0);
}


void   _mulle_aba_linked_list_print( struct _mulle_aba_linked_list *p)
{
   _mulle_aba_linked_list_walk( p, (void *)  print, NULL);
}



#pragma mark -
#pragma mark timestamp storage

struct _mulle_aba_timestamp_storage *_mulle_aba_timestamp_storage_alloc( struct _mulle_allocator *allocator)
{
   struct _mulle_aba_timestamp_storage   *ts_storage;
   unsigned int                          i;
   
   ts_storage = (*allocator->calloc)( 1, sizeof( struct _mulle_aba_timestamp_storage));
   if( ! ts_storage)
      return( ts_storage);
#if DEBUG
   ts_storage->_memtype = _mulle_aba_timestamp_storage_memtype;
#endif
   
   for( i = 0; i < _mulle_aba_timestamp_storage_n_entries; i++)
      ts_storage->_entries[ i]._retain_count_1._nonatomic = (void *) -1;
   
#if TRACE || TRACE_ALLOC
   fprintf( stderr, "%s: alloc ts_storage %p\n", pthread_name(), ts_storage);
#endif
   
   return( ts_storage);
}


void  _mulle_aba_timestamp_storage_empty_assert( struct _mulle_aba_timestamp_storage *ts_storage)
{
   unsigned int   i;
   
   for( i = 0; i < _mulle_aba_timestamp_storage_n_entries; i++)
   {
//      assert( ts_storage->_entries[ i]._retain_count_1._nonatomic == (void *) -1);
      assert( ! ts_storage->_entries[ i]._block_list._head._nonatomic);
   }
}

void   __mulle_aba_timestamp_storage_free( struct _mulle_aba_timestamp_storage *ts_storage, struct _mulle_allocator *allocator)
{
   if( ! ts_storage)
      return;

#if TRACE
   fprintf( stderr, "%s: free ts_storage %p\n", pthread_name(), ts_storage);
#endif
   
   (*allocator->free)( ts_storage);
}


void   _mulle_aba_timestamp_storage_free( struct _mulle_aba_timestamp_storage *ts_storage,
                                          struct _mulle_allocator *allocator)
{
   if( ! ts_storage)
      return;

#if TRACE
   fprintf( stderr, "%s: free ts_storage %p\n", pthread_name(), ts_storage);
#endif
   
   _mulle_aba_timestamp_storage_empty_assert( ts_storage);
   (*allocator->free)( ts_storage);
}


#pragma mark -
#pragma mark delete/free stuff

struct _mulle_aba_world_pointer;

//
// pops off first storage
//
struct _mulle_aba_timestamp_storage   *_mulle_aba_world_pop_storage( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestamp_storage   *storage;
   unsigned int                          n;
   
   n = world->_n;
   if( ! n)
      return( NULL);
   
   --n;

   storage = world->_storage[ 0];

   memcpy( &world->_storage[ 0], &world->_storage[ 1], n * sizeof( struct _mulle_aba_timestamp_storage *));

   world->_storage[ n] = 0;
   world->_n           = n;
   world->_offset     += _mulle_aba_timestamp_storage_n_entries;
#if TRACE
   fprintf( stderr, "%s: pop storage (0) %p\n", pthread_name(), storage);
#endif
   return( storage);
}


int  _mulle_aba_world_push_storage( struct _mulle_aba_world *world,
                                    struct _mulle_aba_timestamp_storage *ts_storage)
{
   assert( ts_storage);
   
   if( world->_size == world->_n)
   {
      errno = EACCES;
      assert( 0);
      return( -1);
   }
#if TRACE
   fprintf( stderr, "%s: push storage (%u) %p\n", pthread_name(), world->_n, ts_storage);
#endif
   
   world->_storage[ world->_n++] = ts_storage;
   return( 0);
}


void   _mulle_aba_world_rotate_storage( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestamp_storage   *storage;
   
   assert( world->_n);
#if TRACE
   fprintf( stderr, "%s: rotate storage of world %p\n", pthread_name(), world);
#endif
   storage = _mulle_aba_world_pop_storage( world);
   _mulle_aba_world_push_storage( world, storage);
}


unsigned int   _mulle_aba_world_get_timestamp_storage_index( struct _mulle_aba_world *world,
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
   s_index    = (unsigned int) (timestamp / _mulle_aba_timestamp_storage_n_entries);
   
   if( s_index >= world->_n)
   {
      if( world->_size == world->_n)
      {
         errno = ENOSPC;
         assert( timestamp <= world->_timestamp + 1);  // that's OK
         return( -1);
      }

      errno = EINVAL;
      assert( timestamp <= world->_timestamp + 1);  // that's OK
      return( -1);
   }
   return( s_index);
}


struct _mulle_aba_timestamp_storage  *
   _mulle_aba_world_get_timestamp_storage( struct _mulle_aba_world *world,
                                           uintptr_t timestamp)
{
   unsigned int   ts_index;
   
   ts_index = _mulle_aba_world_get_timestamp_storage_index( world, timestamp);
   if( ts_index == -1)
      return( NULL);
   return( _mulle_aba_world_get_timestamp_storage_at_index( world, ts_index));
}


struct _mulle_aba_timestamp_entry  *
   _mulle_aba_world_get_timestamp_entry( struct _mulle_aba_world *world,
                                         uintptr_t timestamp)
{
   struct _mulle_aba_timestamp_storage  *ts_storage;

   if( ! timestamp)
   {
      errno = EINVAL;
      return( NULL);
   }
   
   ts_storage = _mulle_aba_world_get_timestamp_storage( world, timestamp);
   if( ! ts_storage)
   {
      errno = EINVAL;
      return( NULL);
   }
   
   return( _mulle_aba_timestamp_storage_get_timestamp_entry( ts_storage, timestamp));
}


static void   _mulle_aba_world_copy( struct _mulle_aba_world *dst,
                                     struct _mulle_aba_world *src)
{
   size_t   bytes;

   bytes = (char *) &dst->_storage[ src->_size] - (char *) (&dst->_next + 1);
   memcpy( (&dst->_next + 1), (&src->_next + 1), bytes);

   dst->_next       = 0;
   dst->_generation = src->_generation + 1;
}


# pragma mark -
# pragma mark Debug

#ifndef NDEBUG
struct _mulle_aba_timestamp_range
{
   uintptr_t   start;
   uintptr_t   length;
};

static int   timestamp_range_contains_timestamp( struct _mulle_aba_timestamp_range  range, uintptr_t timestamp)
{
   return( timestamp < range.start + range.length && timestamp >= range.start);
}


static struct _mulle_aba_timestamp_range
   _mulle_aba_world_get_timestamp_range( struct _mulle_aba_world *world)
{
   struct _mulle_aba_timestamp_range   range;
   
   range.start  = world->_offset;
   range.length = world->_n * _mulle_aba_timestamp_storage_n_entries;
   if( ! world->_offset)
   {
      range.start = 1;
      range.length--;
   }
   return( range);
}
#endif


void   _mulle_aba_world_sanity_assert( struct _mulle_aba_world *world)
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

   assert( ! world->_timestamp || _mulle_aba_world_get_timestamp_entry( world, world->_timestamp));
#endif
}


#pragma mark -
#pragma mark init/done

int   _mulle_aba_storage_init( struct _mulle_aba_storage *q,
                               struct _mulle_allocator *allocator)
{
   struct _mulle_aba_world   *world;
   
   memset( q, 0, sizeof( *q));
   
   q->_allocator = *allocator;
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
   
   q->_world._nonatomic = world;
   return( 0);
}

void   _mulle_aba_storage_done( struct _mulle_aba_storage *q)
{
   _mulle_aba_world_pointer_t   world_p;
   struct _mulle_aba_world      *world;
   
#if TRACE || TRACE_FREE
   fprintf( stderr, "%s: done with storage %p\n", pthread_name(), q);
#endif
   world_p = _mulle_aba_storage_get_world_pointer( q);
   world   = mulle_aba_world_pointer_get_struct( world_p);

   // in dire circumstances, there can be some left overs  (see [^1] other file)
   _mulle_aba_storage_free_leak_worlds( q);
   _mulle_aba_storage_free_world( q, world);

   _mulle_aba_storage_free_leak_entries( q);
   
#ifndef NDEBUG
   memset( q, 0xAF, sizeof( *q));
#endif
}


#pragma mark -

struct _mulle_aba_linked_list_entry
   *_mulle_aba_storage_alloc_linked_list_entry( struct _mulle_aba_storage *q)
{
   struct _mulle_aba_linked_list_entry  *entry;
   
   entry = (void *) _mulle_aba_linked_list_remove_one( &q->_free_entries);
   if( entry)
   {
      entry->_next = 0;
#if TRACE || TRACE_LIST
      fprintf( stderr, "%s: reuse cached list entry %p\n", pthread_name(), entry);
#endif     
      assert( entry->_free == NULL);
      assert( entry->_owner == NULL);
      assert( entry->_pointer == NULL);
   }
   else
   {
      entry = (*q->_allocator.calloc)( 1, sizeof( struct _mulle_aba_linked_list_entry));
      if( ! entry)
         return( NULL);
      
#if TRACE || TRACE_LIST || TRACE_ALLOC
      fprintf( stderr, "%s: allocated list entry %p\n", pthread_name(), entry);
#endif     
      assert( entry->_next == NULL);
      assert( entry->_free == NULL);
      assert( entry->_owner == NULL);
      assert( entry->_pointer == NULL);
   }
   
#if DEBUG
   entry->_memtype = _mulle_aba_linked_list_entry_memtype;
#endif
   return( entry);
}


static int  free_block_and_entry( struct _mulle_aba_linked_list_entry *entry,
                                  struct _mulle_aba_linked_list_entry *prev,
                                  void *userinfo)
{
   struct _mulle_aba_storage *q;
   
   q = userinfo;
#if TRACE || TRACE_LIST
   fprintf( stderr, "%s: free linked list entry %p (for %p)\n", pthread_name(), entry, entry->_pointer);
#endif
   // leak worlds occur sometimes multiple times fix that
   (*entry->_free)( entry->_owner, entry->_pointer);
   _mulle_aba_storage_free_linked_list_entry( q, entry);
#if TRACE || TRACE_FREE || TRACE_LIST
   fprintf( stderr, "%s: added entry %p (%p) to reuse storage %p (%p)\n", pthread_name(), entry, entry->_next, &q->_free_entries, q->_free_entries._head._nonatomic);
#endif
   return( 0);
}


void   _mulle_aba_storage_linked_list_free( struct _mulle_aba_storage *q,
                                            struct _mulle_aba_linked_list  *list)
{
   assert( q);
   assert( list);
   
#if TRACE || TRACE_LIST
   fprintf( stderr, "%s: free linked list %p\n", pthread_name(), list);
#endif
   _mulle_aba_linked_list_walk( list, (void *) free_block_and_entry, q);
}


#pragma mark -

struct _mulle_aba_world   *_mulle_aba_storage_alloc_world( struct _mulle_aba_storage *q,
                                                           unsigned int size)
{
   struct _mulle_aba_world   *world;
   
   assert( size);
   
   for(;;)
   {
      world = (void *) _mulle_aba_linked_list_remove_one( &q->_free_worlds);
      if( ! world)
         break;
      
      if( world->_size >= size)
      {
#if TRACE || TRACE_ALLOC
         fprintf( stderr, "%s: reuse cached world %p\n", pthread_name(), world);
#endif     
         return( world);
      }
      
#if TRACE || TRACE_FREE
      fprintf( stderr, "%s: free too small cached world %p\n", pthread_name(), world);
#endif     
      (*q->_allocator.free)( world);
   }
   
   world = (*q->_allocator.calloc)( 1, sizeof( struct _mulle_aba_world) + sizeof( struct _mulle_aba_timestamp_storage *) * size);
   
   if( world)
   {
#if DEBUG
      world->_memtype = _mulle_aba_world_memtype;
#endif      
      world->_size = size;
      assert( ! world->_next);
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( world);
#endif
#if TRACE || TRACE_ALLOC
      fprintf( stderr, "%s: alloc world %p\n", pthread_name(), world);
#endif
   }
   return( world);
}


#if DEBUG
struct
{
   char   *name;
   void   *world;
} last_freed_world;
#endif

void   _mulle_aba_storage_free_world( struct _mulle_aba_storage *q,
                                      struct _mulle_aba_world *world)
{
#if TRACE || TRACE_FREE
   fprintf( stderr, "%s: actually freeing world %p\n", pthread_name(), world);
#endif
   (*q->_allocator.free)( world);

#if DEBUG
   last_freed_world.name  = pthread_name();
   last_freed_world.world = world;
#endif
}


void   _mulle_aba_storage_free_leak_worlds( struct _mulle_aba_storage *q)
{
   struct _mulle_aba_linked_list   list;

#if TRACE
   fprintf( stderr, "%s: NULLing leaks %p of storage %p\n", pthread_name(), &q->_leaks, q);
#endif
   list._head._nonatomic = _mulle_aba_linked_list_remove_all( &q->_leaks);
   
   _mulle_aba_storage_linked_list_free( q, &list);
}


static int  free_entry( struct _mulle_aba_linked_list_entry *entry,
                        struct _mulle_aba_linked_list_entry *prev,
                        void *userinfo)
{
   struct _mulle_allocator   *allocator;
   
   allocator = userinfo;
#if TRACE || TRACE_LIST || TRACE_FREE
   fprintf( stderr, "%s: free unused linked list entry %p\n", pthread_name(), entry);
#endif
   (*allocator->free)( entry);
   return( 0);
}


void   _mulle_aba_storage_free_leak_entries( struct _mulle_aba_storage *q)
{
   struct _mulle_aba_linked_list   list;

#if TRACE || TRACE_LIST || TRACE_FREE
   fprintf( stderr, "%s: NULLing unused entries %p of storage %p\n", pthread_name(), &q->_free_entries, q);
#endif
   list._head._nonatomic = _mulle_aba_linked_list_remove_all( &q->_free_entries);
   
   _mulle_aba_linked_list_walk( &list, (void *) free_entry, &q->_allocator);
}


#pragma mark -
#pragma mark world changing code

#if TRACE || TRACE_SWAP
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

static inline void  assert_valid_transition(  _mulle_aba_world_pointer_t new_world_p,
                                              _mulle_aba_world_pointer_t old_world_p,
                                               enum _mulle_swap_intent intention)
{
   uintptr_t   new_state;
   uintptr_t   old_state;
   struct _mulle_aba_world   *new_world; // just for debugging
   struct _mulle_aba_world   *old_world;
   
   // two lines just for debugging
   new_world = mulle_aba_world_pointer_get_struct( new_world_p);
   old_world = mulle_aba_world_pointer_get_struct( old_world_p);
   
   // when the intent is locking, we can't read the world contents
   if( intention == _mulle_swap_lock_intent)
   {
      new_state = (uintptr_t) new_world_p & 0x3;
      old_state = (uintptr_t) old_world_p & 0x3;
   }
   else
   {
      new_state = _mulle_aba_world_pointer_state( new_world_p);
      old_state = _mulle_aba_world_pointer_state( old_world_p);
   }

   switch( old_state)
   {
   case 0 :
      assert( new_state == 2 && intention == _mulle_swap_lock_intent);
      break;
      
   case 1 :
      assert( new_state == 3 && intention == _mulle_swap_lock_intent);
      break;
         
   case 2 :
      assert( new_state == 0 && intention == _mulle_swap_unlock_intent ||
              new_state == 1 && intention == _mulle_swap_register_intent );
      break;

   case 3 :
      assert( new_state == 1 && intention == _mulle_swap_unlock_intent ||
              new_state == 2 && intention == _mulle_swap_unregister_intent ||
              new_state == 9 && intention == _mulle_swap_register_intent);
      break;

   case 5 :
      assert( new_state ==  7 && intention == _mulle_swap_lock_intent ||
              new_state ==  5 && intention == _mulle_swap_checkin_intent);
      break;

   case 7 :
      assert( new_state ==  2 && intention == _mulle_swap_unregister_intent ||
              new_state ==  5 && intention == _mulle_swap_unlock_intent ||
              new_state ==  5 && intention == _mulle_swap_checkin_intent ||
              new_state == 13 && intention == _mulle_swap_register_intent);
      break;

   case 9 :
      assert( new_state == 11 && intention == _mulle_swap_lock_intent ||
              new_state == 12 && intention == _mulle_swap_free_intent);
      break;

   case 11 :
      assert( new_state ==  1 && intention == _mulle_swap_unregister_intent ||
              new_state ==  9 && intention == _mulle_swap_unlock_intent ||
              new_state ==  9 && intention == _mulle_swap_unregister_intent ||
              new_state ==  9 && intention == _mulle_swap_register_intent ||
              new_state == 12 && intention == _mulle_swap_free_intent);
      break;

   case 12 :
      assert( new_state == 13 && intention == _mulle_swap_checkin_intent ||
              new_state == 14 && intention == _mulle_swap_lock_intent);
      break;

   case 13 :
      assert( new_state == 12 && intention == _mulle_swap_free_intent ||
              new_state == 13 && intention == _mulle_swap_checkin_intent ||
              new_state == 15 && intention == _mulle_swap_lock_intent);
      break;
      
   case 14 :
      assert( new_state ==  5 && intention == _mulle_swap_unregister_intent ||
              new_state == 12 && intention == _mulle_swap_unlock_intent ||
              new_state == 13 && intention == _mulle_swap_register_intent ||
              new_state == 13 && intention == _mulle_swap_unregister_intent ||
              new_state == 13 && intention == _mulle_swap_checkin_intent ||
              new_state == 14 && intention == _mulle_swap_free_intent);
      break;

   case 15 :
      assert( new_state ==  5 && intention == _mulle_swap_unregister_intent ||
              new_state == 12 && intention == _mulle_swap_free_intent ||
              new_state == 13 && intention == _mulle_swap_unlock_intent ||
              new_state == 13 && intention == _mulle_swap_checkin_intent ||
              new_state == 13 && intention == _mulle_swap_register_intent ||
              new_state == 13 && intention == _mulle_swap_unregister_intent);
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
                                       _mulle_aba_world_pointer_t new_world_p,
                                       _mulle_aba_world_pointer_t old_world_p)
{
#ifndef NDEBUG
   
   struct _mulle_aba_world   *new_world;
   struct _mulle_aba_world   *old_world;
   
   new_world = mulle_aba_world_pointer_get_struct( new_world_p);
   old_world = mulle_aba_world_pointer_get_struct( old_world_p);
#endif
   
#if TRACE_SWAP
   fprintf( stderr, "\n%s:     %s swap %p -> %p\n", pthread_name(), _mulle_swap_intent_name( intention),
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
      assert( ! new_world->_next || (new_world == old_world && intention == _mulle_swap_checkin_intent));
      assert( new_world->_memtype == _mulle_aba_world_memtype);
      assert( old_world->_memtype == _mulle_aba_world_memtype);
      assert( new_world->_n_threads < 16 && old_world->_n_threads < 16);
      assert( new_world->_generation == old_world->_generation + 1 || new_world == old_world);
#endif
      
      // do not allow an implicit unlock, to keep the world pointer
      if( mulle_aba_world_pointer_get_lock( old_world_p) && intention != _mulle_swap_unlock_intent)
         assert( new_world != old_world);
      
      //
      // if bit was set and will be cleared, make sure timestamp has changed
      //
      if( ((intptr_t) old_world_p & 1) && ! ((intptr_t) old_world_p & 1))
         assert( old_world->_timestamp != new_world->_timestamp || ! new_world->_timestamp);
      
      //
      // conversely make sure timestamp stays unchanged, when bit hasn't been set
      // (except timestamp changes to 0)
      //
      if( ! ((intptr_t) old_world_p & 1))
         assert( old_world->_timestamp == new_world->_timestamp || new_world->_timestamp == 0 || old_world->_timestamp == 0);
      
      // make sure timestamp only increases
      assert( ! mulle_aba_world_pointer_get_struct( new_world_p)->_timestamp ||   (mulle_aba_world_pointer_get_struct( new_world_p)->_timestamp >=  mulle_aba_world_pointer_get_struct( old_world_p)->_timestamp));
   }
#endif
}


static inline void  log_swap_worlds( enum _mulle_swap_intent intention,
                                     _mulle_aba_world_pointer_t new_world_p,
                                     _mulle_aba_world_pointer_t old_world_p,
                                     int rval)
{
#if TRACE || TRACE_SWAP
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
      fprintf( stderr, "%s: @+@ %s swap %p (%d) -> %p (%d) @cputime %ld.%ld\n", pthread_name(), _mulle_swap_intent_name( intention),
              old_world_p, _mulle_aba_world_pointer_state( old_world_p),
              new_world_p, _mulle_aba_world_pointer_state( new_world_p),
              cpu_time.tv_sec,cpu_time.tv_nsec);
   else
      fprintf( stderr, "%s: @-@ FAILED %s swap %p -> %p\n", pthread_name(), _mulle_swap_intent_name( intention),
              old_world_p,
              new_world_p);
#endif

// this only works with guard malloc, if addresses aren't reused
#ifndef NDEBUG
{
   static struct
   {
      _mulle_aba_world_pointer_t   world_p;
      pthread_t                    thread;
   } last_world_ps[ 0x20];
   static mulle_atomic_ptr_t    last_world_ps_index;
   unsigned int                 i;

   if( rval)
   {
//      for( i = 0; i < 0x20; i++)
//         if( pthread_self() != last_world_ps[ i].thread)
//            assert( new_world_p != last_world_ps[ i].world_p);
   
      i = ((uintptr_t) _mulle_atomic_read_pointer( &last_world_ps_index)) & 0x1F;
      last_world_ps[ i].world_p = new_world_p;
      last_world_ps[ i].thread      = pthread_self();
      _mulle_atomic_increment_pointer( &last_world_ps_index);
   }
}
#endif
  
#if STATE_STATS
   mulle_transitions_count[ _mulle_aba_world_pointer_state( old_world_p)][ _mulle_aba_world_pointer_state( new_world_p)]++;
#endif
}


//
// TODO: if we'd return the actual read, we could later save the next read
//
static inline int
   _mulle_aba_storage_swap_worlds( struct _mulle_aba_storage *q,
                                   enum _mulle_swap_intent intention,
                                   _mulle_aba_world_pointer_t new_world_p,
                                   _mulle_aba_world_pointer_t old_world_p)
{
   int   rval;

   assert_swap_worlds( intention, new_world_p, old_world_p);
   
   rval = _mulle_atomic_compare_and_swap_pointer( &q->_world, new_world_p, old_world_p);
   
   log_swap_worlds( intention, new_world_p, old_world_p, rval);
   
   return( rval ? 0 : -1);
}


struct _mulle_aba_world_pointers
   _mulle_aba_storage_change_world_pointer( struct _mulle_aba_storage *q,
                                            enum _mulle_swap_intent intention,
                                            _mulle_aba_world_pointer_t
                                            old_world_p,
                                            int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                            void *userinfo)
{
   _mulle_aba_world_pointer_t        new_world_p;
   struct _mulle_aba_callback_info   ctxt;
   int                               rval;

   goto start;
   
   do
   {
      old_world_p     = _mulle_aba_storage_get_world_pointer( q);
start:
      ctxt.old_locked = mulle_aba_world_pointer_get_lock( old_world_p);
      
      //
      // changing bits on the a locked world is not good, except if we unlock
      // If intent is to lock, we should fail in callback
      //
      if( ctxt.old_locked && intention != _mulle_swap_unlock_intent)
      {
         errno = EWOULDBLOCK;  // special error, means redo with new world
         return( _mulle_aba_world_pointers_make( NULL, NULL));
      }

      ctxt.old_world  = mulle_aba_world_pointer_get_struct( old_world_p);
      ctxt.old_bit    = mulle_aba_world_pointer_get_bit( old_world_p);
      ctxt.new_locked = 0;

      ctxt.new_world  = ctxt.old_world;
      ctxt.new_bit    = ctxt.old_bit;

      rval = (*callback)( _mulle_aba_world_modify, &ctxt, userinfo);
      UNPLEASANT_RACE_YIELD();
      if( rval)
      {
         errno = rval;
         return( _mulle_aba_world_pointers_make( NULL, NULL));
      }
      
      new_world_p = mulle_aba_world_pointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == old_world_p)
         break;
      
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( new_world_p));
#endif   
   }
   while( _mulle_aba_storage_swap_worlds( q, intention, new_world_p, old_world_p));
   
   return( _mulle_aba_world_pointers_make( new_world_p, old_world_p));
}


struct _mulle_aba_world_pointers
   _mulle_aba_storage_change_world_pointer_2( struct _mulle_aba_storage *q,
                                              enum _mulle_swap_intent intention,
                                              _mulle_aba_world_pointer_t  old_world_p,
                                              int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                              void *userinfo)
{
   struct _mulle_aba_world_pointers   world_ps;
   
   world_ps = _mulle_aba_storage_change_world_pointer( q, intention, old_world_p, callback, userinfo);
   if( world_ps.new_world_p || errno != EWOULDBLOCK)
      return( world_ps);
   
   world_ps = _mulle_aba_storage_copy_change_world_pointer( q, intention, old_world_p, callback, userinfo);
   return( world_ps);
}

/*
 *
 */

int   _mulle_aba_world_pointer_state( _mulle_aba_world_pointer_t world_p)
{
   int                                  state;
   struct _mulle_aba_world   *world;
   
   state = (int) ((uintptr_t) world_p & 0x3);  // get bit and lock
   world = mulle_aba_world_pointer_get_struct( world_p);
   if( world->_timestamp != 0)
      state += 4;
   if( world->_n_threads > 1)
      state += 8;
   return( state);
}


_mulle_aba_world_pointer_t
   _mulle_aba_storage_try_set_lock_world_pointer( struct _mulle_aba_storage *q,
                                                  _mulle_aba_world_pointer_t old_world_p,
                                                  int bit)
{
   _mulle_aba_world_pointer_t   new_world_p;
   _mulle_aba_world_pointer_t   last_world_p;
   struct _mulle_aba_world      *old_world;
   enum _mulle_swap_intent                 intention;
   
   // this is the world we are trying to lock
   intention = bit ? _mulle_swap_lock_intent : _mulle_swap_unlock_intent;
   old_world = mulle_aba_world_pointer_get_struct( old_world_p);
   for(;;)
   {
      new_world_p = mulle_aba_world_pointer_set_lock( old_world_p, bit);

      //
      // if already there, hmm when unlocking it's ok
      // in the case of the lock, that's bad
      //
      if( mulle_aba_world_pointer_get_lock( old_world_p) == bit)
      {
         log_swap_worlds( intention, new_world_p, old_world_p, 0);
         return( NULL);
      }
   
      
      assert_swap_worlds( intention, new_world_p, old_world_p);
      
      last_world_p = __mulle_atomic_compare_and_swap_pointer( &q->_world, new_world_p, old_world_p);
      
      log_swap_worlds( intention, new_world_p, old_world_p, last_world_p == old_world_p);

      if( last_world_p == old_world_p)
         return( new_world_p);  // bingo
      
      
      // different world now ? fail
      if( mulle_aba_world_pointer_get_struct( last_world_p) != old_world)
         return( NULL);
      
      // retry with this world pointer still pointing to the world we want to
      // lock/unlock
      
      old_world_p = last_world_p;
   };
}



struct _mulle_aba_world_pointers
   _mulle_aba_storage_lock_world_pointer( struct _mulle_aba_storage *q)
{
   _mulle_aba_world_pointer_t   new_world_p;
   _mulle_aba_world_pointer_t   old_world_p;
   
   for(;;)
   {
      old_world_p = _mulle_aba_storage_get_world_pointer( q);
      new_world_p = _mulle_aba_storage_try_lock_world_pointer( q, old_world_p);
      if( new_world_p)
         return( _mulle_aba_world_pointers_make( new_world_p, old_world_p));
      
      sched_yield();
   }
}

//
// this is just used for threads registering
//
struct _mulle_aba_world_pointers
   _mulle_aba_storage_copy_lock_world_pointer( struct _mulle_aba_storage *q,
                                               int (*callback)( int, struct _mulle_aba_callback_info *, void *) ,
                                               void *userinfo,
                                               struct _mulle_aba_world **dealloced)
{
   _mulle_aba_world_pointer_t        new_world_p;
   _mulle_aba_world_pointer_t        locked_world_p;
   int                                          rval;
   struct _mulle_aba_callback_info   ctxt;
   struct _mulle_aba_world           *tofree;
   int                                          fail;
   struct _mulle_aba_world_pointers  world_ps;
   int                                          loops;
   
   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;
   
   *dealloced = NULL;
   
   for( loops = 0;; loops++) // loops is just for debugging
   {
      world_ps       = _mulle_aba_storage_lock_world_pointer( q);

      /* set locked_world_p to the locked world, on which we work
       */
      
      locked_world_p  = world_ps.new_world_p;

      ctxt.old_world  = mulle_aba_world_pointer_get_struct( locked_world_p);
      ctxt.old_bit    = mulle_aba_world_pointer_get_bit( locked_world_p);
      ctxt.old_locked = mulle_aba_world_pointer_get_lock( locked_world_p);
      assert( ctxt.old_locked);
      
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( locked_world_p));
#endif
      if( ! ctxt.new_world || ctxt.new_world->_size != ctxt.old_world->_size)
      {
         if( ctxt.new_world)
         {
            (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
            _mulle_aba_storage_free_world( q, ctxt.new_world);
            
            UNPLEASANT_RACE_YIELD();
         }

         ctxt.new_world = _mulle_aba_storage_alloc_world( q, ctxt.old_world->_size);
         if( ! ctxt.new_world)
         {
            // TODO: locked worlds leak...
            return( _mulle_aba_world_pointers_make( NULL, NULL));
         }
      }
      
      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);
      
      ctxt.new_bit = ctxt.old_bit;
      
#ifndef NDEBUG
      assert( ! ctxt.new_world->_next);
      _mulle_aba_world_sanity_assert( ctxt.new_world);
      _mulle_aba_world_sanity_assert( ctxt.old_world);
#endif
      rval = (*callback)( _mulle_aba_world_modify, &ctxt, userinfo);
      if( rval)
      {
         // TODO: locked worlds leak...
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_world_pointers_make( NULL, NULL));
      }
      
      new_world_p = mulle_aba_world_pointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == locked_world_p)
         break;
      
      UNPLEASANT_RACE_YIELD();
      
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( new_world_p));
#endif
      fail = _mulle_aba_storage_swap_worlds( q, _mulle_swap_register_intent, new_world_p, locked_world_p);
      
      //
      // we need to clean up locked worlds, so collect them.
      //
      assert( locked_world_p);
      
      tofree = mulle_aba_world_pointer_get_struct( locked_world_p);
      
      //
      // tofree == dealloced:: this can happen, when we locked in and somebody
      // checks in then the lock may get clobbered.
      // This could be avoided, but then the free
      // checkin code wouldn't be lock agnostic anymore, which I would like to
      // avoid for principles sake
      //
      if( tofree != *dealloced)
      {
         assert( ! tofree->_next);
         assert( ! new_world_p || mulle_aba_world_pointer_get_struct( new_world_p) != tofree);
         
#if TRACE || TRACE_FREE
         fprintf( stderr, "%s: add world %p to free list (%p)\n", pthread_name(), tofree, dealloced);
#endif
         tofree->_next = *dealloced;
         *dealloced    = tofree;
      }
      
      if( ! fail)
         break;
      
      UNPLEASANT_RACE_YIELD();
   }
   
   return( _mulle_aba_world_pointers_make( new_world_p, locked_world_p));
}


struct _mulle_aba_world_pointers
   _mulle_aba_storage_copy_change_world_pointer( struct _mulle_aba_storage *q,
                                                 enum _mulle_swap_intent  intention,
                                                 _mulle_aba_world_pointer_t   old_world_p,
                                                 int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                                 void *userinfo)
{
   _mulle_aba_world_pointer_t        new_world_p;
   int                               rval;
   struct _mulle_aba_callback_info   ctxt;
   int                               loops;
   int                               cmd;
   
   assert( old_world_p);

   loops = 0;
   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;
   cmd             = _mulle_aba_world_modify;
   goto start_with_old_world;
   
   do
   {
      ++loops;
      cmd            = _mulle_aba_world_reuse;
      old_world_p    = _mulle_aba_storage_get_world_pointer( q);
start_with_old_world:
      ctxt.old_world  = mulle_aba_world_pointer_get_struct( old_world_p);
      ctxt.old_bit    = mulle_aba_world_pointer_get_bit( old_world_p);
      ctxt.old_locked = mulle_aba_world_pointer_get_lock( old_world_p);
     
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( old_world_p));
#endif
      if( ! ctxt.new_world || ctxt.new_world->_size != ctxt.old_world->_size)
      {
         if( ctxt.new_world)
         {
            (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
            _mulle_aba_storage_free_world( q, ctxt.new_world);

            UNPLEASANT_RACE_YIELD();
         }

         cmd            = _mulle_aba_world_modify;
         ctxt.new_world = _mulle_aba_storage_alloc_world( q, ctxt.old_world->_size);
         if( ! ctxt.new_world)
            return( _mulle_aba_world_pointers_make( NULL, NULL));
      }
      
      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);
      
      ctxt.new_bit    = ctxt.old_bit;
      ctxt.new_locked = 0;
      
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( ctxt.old_world);
      _mulle_aba_world_sanity_assert( ctxt.new_world);
#endif
      rval = (*callback)( cmd, &ctxt, userinfo);
      if( rval)
      {
         (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_world_pointers_make( NULL, NULL));
      }
      
      new_world_p = mulle_aba_world_pointer_make( ctxt.new_world, ctxt.new_bit);
      if( ctxt.new_locked)
         new_world_p = mulle_aba_world_pointer_set_lock( new_world_p, 1);
      if( new_world_p == old_world_p)
         break;

      UNPLEASANT_RACE_YIELD();
      
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( new_world_p));
#endif   
   }
   while( _mulle_aba_storage_swap_worlds( q, intention, new_world_p, old_world_p));

   return( _mulle_aba_world_pointers_make( new_world_p, old_world_p));
}



struct _mulle_aba_world_pointers
   _mulle_aba_storage_grow_world_pointer( struct _mulle_aba_storage *q,
                                          unsigned int target_size,
                                          int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                          void *userinfo)
{
   _mulle_aba_world_pointer_t        new_world_p;
   _mulle_aba_world_pointer_t        old_world_p;
   int                               rval;
   int                               cmd;
   struct _mulle_aba_callback_info   ctxt;
   unsigned int                      size;
   
   ctxt.new_world  = NULL;
   ctxt.new_bit    = 0;
   ctxt.new_locked = 0;
   do
   {
      old_world_p    = _mulle_aba_storage_get_world_pointer( q);
      ctxt.old_world = mulle_aba_world_pointer_get_struct( old_world_p);
      if( ctxt.old_world->_size >= target_size)
      {
#if TRACE
         fprintf( stderr, "%s: discard %p (%p)\n", pthread_name(), ctxt.old_world, callback);
#endif
         (*callback)( _mulle_aba_world_discard, &ctxt, userinfo);
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = EBUSY;
         return( _mulle_aba_world_pointers_make( NULL, NULL));  // someone else did the dirty job
      }
      
      ctxt.old_bit    = mulle_aba_world_pointer_get_bit( old_world_p);
      ctxt.old_locked = mulle_aba_world_pointer_get_lock( old_world_p);
      
      if( ! ctxt.new_world)
      {
         ctxt.new_world = _mulle_aba_storage_alloc_world( q, target_size);
         if( ! ctxt.new_world)
            return( _mulle_aba_world_pointers_make( NULL, NULL));
  // fail
         
         cmd = _mulle_aba_world_modify;
      }
      else
      {
#if TRACE
         fprintf( stderr, "%s: reuse %p (%p)\n", pthread_name(), ctxt.new_world, callback);
#endif
         cmd = _mulle_aba_world_reuse;
      }
      
      // don't overwrite size
      size = ctxt.new_world->_size;
      _mulle_aba_world_copy( ctxt.new_world, ctxt.old_world);
      ctxt.new_world->_size = size;
      ctxt.new_bit          = ctxt.old_bit;
      
      UNPLEASANT_RACE_YIELD();

      rval = (*callback)( cmd, &ctxt, userinfo);
      if( rval)
      {
         _mulle_aba_storage_free_world( q, ctxt.new_world);
         errno = rval;
         return( _mulle_aba_world_pointers_make( NULL, NULL));
      }
      
      new_world_p = mulle_aba_world_pointer_make( ctxt.new_world, ctxt.new_bit);
      if( new_world_p == old_world_p)
         break;
#ifndef NDEBUG
      _mulle_aba_world_sanity_assert( mulle_aba_world_pointer_get_struct( new_world_p));
#endif   
   }
   while( _mulle_aba_storage_swap_worlds( q, _mulle_swap_free_intent, new_world_p, old_world_p));
   
   return( _mulle_aba_world_pointers_make( new_world_p, old_world_p));
}


void   _mulle_aba_world_check_timerange( struct _mulle_aba_world *world,
                                         uintptr_t old,
                                         uintptr_t new,
                                         struct _mulle_aba_storage *q)
{
   struct _mulle_aba_timestamp_storage   *ts_storage;
   struct _mulle_aba_timestamp_entry     *ts_entry;
   uintptr_t                             timestamp;
   unsigned int                          ts_index;
   unsigned int                          index;
   void                                  *rc;
   struct _mulle_aba_linked_list         free_list;
   
   if( ! world->_timestamp)
      return;

#ifndef NDEBUG
   _mulle_aba_world_sanity_assert( world);
#endif
   
   for( timestamp = old; timestamp <= new; timestamp++)
   {
      ts_index    = _mulle_aba_world_get_timestamp_storage_index( world, timestamp);
      ts_storage  = _mulle_aba_world_get_timestamp_storage_at_index( world, ts_index);
      assert( ts_storage);
      
      index       = mulle_aba_timestamp_storage_get_timestamp_index( timestamp);
      ts_entry    = _mulle_aba_timestamp_storage_get_timestamp_entry_at_index( ts_storage, index);
      assert( ts_entry);

      UNPLEASANT_RACE_YIELD();
      
      rc = _mulle_atomic_decrement_pointer( &ts_entry->_retain_count_1);
#if TRACE
      fprintf( stderr, "%s: decrement timestamp ts=%ld to rc=%ld\n", pthread_name(), timestamp, (intptr_t) rc);
#endif      
// rc can be negative, it's OK
//      assert( (intptr_t) rc >= 0);
      if( rc)
         continue;
      
      //
      // snip out block list from entry, remove usage bit
      // now if max_timestamp still points into ts_storage it will get reused
      // so we can't reuse it
      //
#if TRACE || TRACE_FREE || TRACE_LIST
      fprintf( stderr,  "\n%s: *** freeing linked list %p of ts=%ld rc=%ld***\n", pthread_name(), &ts_entry->_block_list, timestamp, (intptr_t) _mulle_atomic_read_pointer( &ts_entry->_retain_count_1) + 1);
      _mulle_aba_linked_list_print( &ts_entry->_block_list);
#endif

      // 
      // atomic not needed as we should be single threaded here in terms
      // of ts_entry (not storage though)
      //
      free_list = ts_entry->_block_list;
      UNPLEASANT_RACE_YIELD();
      ts_entry->_block_list._head._nonatomic = NULL;
      
      _mulle_aba_timestamp_storage_set_usage_bit( ts_storage, index, 0);
      // now conceivably ts_storage may be gone, don't access any more
      
      _mulle_aba_storage_linked_list_free( q, &free_list);
   }
}


static unsigned int  __mulle_aba_world_reuse_storages( struct _mulle_aba_world *world,
                                                       int counting)
{
   uintptr_t                             ts_index;
   struct _mulle_aba_timestamp_storage   *ts_storage;
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
   
   ts_index = _mulle_aba_world_get_timestamp_storage_index( world, world->_timestamp);
   
   n = world->_n;
   for( i = 0; i < n; i++)
   {
      UNPLEASANT_RACE_YIELD();
      
      if( i == ts_index)
         break;
      
      ts_storage = _mulle_aba_world_get_timestamp_storage_at_index( world, i);
      if( ! ts_storage)
         break;
      
      if( _mulle_atomic_read_pointer( &ts_storage->_usage_bits))
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


unsigned int  _mulle_aba_world_available_count_reusable_storages( struct _mulle_aba_world *world)
{
   return( __mulle_aba_world_reuse_storages( world, 1));
}
