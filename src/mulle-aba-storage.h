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

#ifndef mulle_aba_storage_h__
#define mulle_aba_storage_h__

#include "include.h"

#include "mulle-aba-defines.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if MULLE_ABA_MEMTYPE_DEBUG
enum
{
   _mulle_aba_world_memtype            = 0xB16B16B1,
   _mulle_aba_timestampstorage_memtype = 0x10011001,
   _mulle_aba_timestampentry_memtype   = 0x00011000,
   _mulle_aba_freeentry_memtype        = 0x5a775a77
};
#endif


struct _mulle_aba_freeentry
{
   struct _mulle_concurrent_linkedlistentry   _link;

#if MULLE_ABA_MEMTYPE_DEBUG
   uintptr_t   _memtype;
#endif

   void   (*_free)( void *, void *);  // owner, pointer
   void   *_pointer;
   void   *_owner;
};



MULLE__ABA_GLOBAL
void   _mulle_aba_freeentry_set( struct _mulle_aba_freeentry *entry,
                                 void (*p_free)( void *, void *),
                                 void *pointer,
                                 void *owner);

static inline void _mulle_aba_freeentry_free( struct _mulle_aba_freeentry *entry,
                                              struct mulle_allocator *allocator)
{
   mulle_allocator_free( allocator, entry);
}

MULLE__ABA_GLOBAL
void   _mulle_aba_free_list_print( struct _mulle_concurrent_linkedlist *p);


# pragma mark -
# pragma mark _mulle_aba_timestampentry

struct _mulle_aba_timestampentry
{
   mulle_atomic_pointer_t         _retain_count_1;  // really an inptr_t ...
#if MULLE_ABA_MEMTYPE_DEBUG
   uintptr_t                      _memtype;
#endif
   struct _mulle_concurrent_linkedlist   _pointer_list;
};


# pragma mark -
# pragma mark _mulle_aba_timestampstorage

#if DEBUG
enum
{
   _mulle_aba_timestampstorage_n_entries = 2
};
#else
enum
{
   _mulle_aba_timestampstorage_n_entries = sizeof( uintptr_t) * 8
};
#endif

struct _mulle_aba_timestampstorage
{
   mulle_atomic_pointer_t             _usage_bits;
#if MULLE_ABA_MEMTYPE_DEBUG
   uintptr_t                          _memtype;
#endif
   struct _mulle_aba_timestampentry   _entries[ _mulle_aba_timestampstorage_n_entries];
};


MULLE__ABA_GLOBAL
struct _mulle_aba_timestampstorage *
   _mulle_aba_timestampstorage_alloc( struct mulle_allocator *allocator);


static inline unsigned int
   _mulle_aba_timestampstorage_get_usage_bit( struct _mulle_aba_timestampstorage *ts_storage,
                                               unsigned int index)
{
   uintptr_t   usage;

   usage = (uintptr_t) _mulle_atomic_pointer_read( &ts_storage->_usage_bits);
   return( (usage & (1UL << index)) ? 1 : 0);
}



MULLE__ABA_GLOBAL
int   _mulle_aba_timestampstorage_set_usage_bit( struct _mulle_aba_timestampstorage *ts_storage,
                                                 unsigned int index,
                                                 int bit);


static inline unsigned int  mulle_aba_timestampstorage_get_timestamp_index( uintptr_t timestamp)
{
   return( (unsigned int)( timestamp % _mulle_aba_timestampstorage_n_entries));
}


static inline struct _mulle_aba_timestampentry  *
   _mulle_aba_timestampstorage_get_timestampentry( struct _mulle_aba_timestampstorage *ts_storage,
                                                   uintptr_t timestamp)
{
   return( &ts_storage->_entries[ mulle_aba_timestampstorage_get_timestamp_index( timestamp)]);
}


static inline struct _mulle_aba_timestampentry  *
   _mulle_aba_timestampstorage_get_timestampentry_at_index( struct _mulle_aba_timestampstorage *ts_storage,
                                                            unsigned int index)
{
   assert( index >= 0 && index < _mulle_aba_timestampstorage_n_entries);
   return( &ts_storage->_entries[ index]);
}

MULLE__ABA_GLOBAL
void   __mulle_aba_timestampstorage_free( struct _mulle_aba_timestampstorage *ts_storage,
                                          struct mulle_allocator *allocator);

MULLE__ABA_GLOBAL
void   _mulle_aba_timestampstorage_free( struct _mulle_aba_timestampstorage *ts_storage,
                                         struct mulle_allocator *allocator);




// the _mulle_aba_worldpointer_t combines a bit and the struct pointer
// to the world
typedef struct _mulle_aba_worldpointer_t  *_mulle_aba_worldpointer_t;

struct _mulle_aba_worldpointers
{
   _mulle_aba_worldpointer_t            new_world_p;
   _mulle_aba_worldpointer_t            old_world_p;
   struct _mulle_concurrent_linkedlistentry   *free_worlds;
};

struct _mulle_aba_world;

# pragma mark -
# pragma mark _mulle_aba_worldpointer_t

static inline _mulle_aba_worldpointer_t
   mulle_aba_worldpointer_set_lock( _mulle_aba_worldpointer_t p_world, int bit)
{
   return( (void *) ( ((intptr_t) p_world & ~0x2) | (bit ? 0x2 : 0x0)));
}


static inline int   mulle_aba_worldpointer_get_lock( _mulle_aba_worldpointer_t p_world)
{
   return( ((intptr_t) p_world & 0x2) ? 1 : 0);
}


static inline _mulle_aba_worldpointer_t
   mulle_aba_worldpointer_set_bit( _mulle_aba_worldpointer_t p_world, int bit)
{
   assert( bit >= 0 && bit <= 1);
   return( (void *) ( ((intptr_t) p_world & ~0x1) | bit));
}


static inline int   mulle_aba_worldpointer_get_bit( _mulle_aba_worldpointer_t p_world)
{
   return( ((intptr_t) p_world & 0x1));
}


static inline _mulle_aba_worldpointer_t   *
   mulle_aba_worldpointer_set_struct( _mulle_aba_worldpointer_t p_world,
                                      struct _mulle_aba_world *world)
{
   assert( ! ((intptr_t) world & 0x3));
   return( (void *) (((intptr_t) p_world & 0x3) | (intptr_t) world));
}


static inline struct _mulle_aba_world   *
   mulle_aba_worldpointer_get_struct( _mulle_aba_worldpointer_t p_world)
{
   return( (void *) ((intptr_t) p_world & ~0x3));
}


static inline _mulle_aba_worldpointer_t
   mulle_aba_worldpointer_make( struct _mulle_aba_world *world, int bits)
{
   assert( bits >= 0 && bits <= 3);
   assert( ! ((intptr_t) world & 0x3));

   return( (void *) ((intptr_t) world | bits));
}


static inline struct _mulle_aba_worldpointers
   _mulle_aba_worldpointers_make( _mulle_aba_worldpointer_t new_world_p,
                                  _mulle_aba_worldpointer_t old_world_p)
{
   assert( (! new_world_p && ! old_world_p) || (mulle_aba_worldpointer_get_struct( new_world_p) && mulle_aba_worldpointer_get_struct( old_world_p)));

   return( (struct _mulle_aba_worldpointers) { .new_world_p = new_world_p, .old_world_p = old_world_p });
}


# pragma mark -
# pragma mark _mulle_aba_world

//
// this is the world_state but its been abbreviated to world
//
struct _mulle_aba_world
{
   struct _mulle_concurrent_linkedlistentry     _link;            // chain, used when deallocing/dealloced

#if MULLE_ABA_MEMTYPE_DEBUG
   uintptr_t                             _memtype;
   uintptr_t                             _generation;      // debugging
#endif
   uintptr_t                             _timestamp;       // current timestamp
   intptr_t                              _n_threads;       // currently active threads

   // storage for registered threads
   uintptr_t                             _offset;          // _min_timestamp / _mulle_aba_timestampstorage_n_entries
   unsigned int                          _size;            // storage of timestamp, rc, blocks
   unsigned int                          _n;
   struct _mulle_aba_timestampstorage   *_storage[];
};



MULLE__ABA_GLOBAL
struct _mulle_aba_timestampstorage   *
_mulle_aba_world_pop_storage( struct _mulle_aba_world *world);

MULLE__ABA_GLOBAL
int  _mulle_aba_world_push_storage( struct _mulle_aba_world *world,
                                    struct _mulle_aba_timestampstorage *ts_storage);

MULLE__ABA_GLOBAL
void   _mulle_aba_world_rotate_storage( struct _mulle_aba_world *world);

MULLE__ABA_GLOBAL
unsigned int
   _mulle_aba_world_get_timestampstorage_index( struct _mulle_aba_world *world,
                                                 uintptr_t timestamp);

MULLE__ABA_GLOBAL
struct _mulle_aba_timestampstorage *
   _mulle_aba_world_get_timestampstorage( struct _mulle_aba_world *world,
                                           uintptr_t timestamp);

static inline size_t   _mulle_aba_world_get_size( struct _mulle_aba_world *world)
{
   /* casty cast madness, code looks like c++ */

   return( (size_t) ((char *) &world->_storage[ world->_size] - (char *) world));
}


static inline struct _mulle_aba_timestampstorage  *
   _mulle_aba_world_get_timestampstorage_at_index( struct _mulle_aba_world *world,
                                                   unsigned int ts_index)
{
   struct _mulle_aba_timestampstorage  *ts_storage;

   assert( ts_index != (unsigned int) -1 && ts_index < world->_n);
   ts_storage = world->_storage[ ts_index];
   assert( ts_storage);
   return( ts_storage);
}

struct _mulle_aba_timestampentry   *
   _mulle_aba_world_get_timestampentry( struct _mulle_aba_world *world,
                                         uintptr_t timestamp);

#pragma mark - _mulle_aba_storage

//
// since the mulle_atomic_pointer_t is opaque, I use this union
// to make it easier to debug
//
union _mulle_aba_atomicworldpointer_t
{
   _mulle_aba_worldpointer_t   world;     // never read it except in the debugger
   mulle_atomic_pointer_t      pointer;
};


struct _mulle_aba_storage
{
   union _mulle_aba_atomicworldpointer_t   _world;

#if MULLE_ABA_MEMTYPE_DEBUG
   uintptr_t                      _memtype;
#endif
   struct mulle_allocator         *_allocator;
   struct _mulle_concurrent_linkedlist   _leaks;
   struct _mulle_concurrent_linkedlist   _free_entries;
   struct _mulle_concurrent_linkedlist   _free_worlds;
};


# pragma mark -
# pragma mark storage init/done

MULLE__ABA_GLOBAL
int   _mulle_aba_storage_init( struct _mulle_aba_storage *q,
                               struct mulle_allocator *allocator);

MULLE__ABA_GLOBAL
void   _mulle_aba_storage_done( struct _mulle_aba_storage *q);


static inline int   _mulle_aba_storage_is_setup( struct _mulle_aba_storage *q)
{
   return( _mulle_atomic_pointer_read( &q->_world.pointer) != NULL);
}


# pragma mark -
# pragma mark world init/free

MULLE__ABA_GLOBAL
struct _mulle_aba_freeentry  *
   _mulle_aba_storage_alloc_freeentry( struct _mulle_aba_storage *q);

MULLE__ABA_GLOBAL
void   _mulle_aba_storage_free_freeentry( struct _mulle_aba_storage *q,
                                          struct _mulle_aba_freeentry  *entry);

struct _mulle_aba_world  *_mulle_aba_storage_alloc_world( struct _mulle_aba_storage *q,
                                                          unsigned int size);
MULLE__ABA_GLOBAL
void   _mulle_aba_storage_free_world( struct _mulle_aba_storage *q,
                                      struct _mulle_aba_world *world);

MULLE__ABA_GLOBAL
void   _mulle_aba_storage_owned_pointer_free_world( struct _mulle_aba_world *world,
                                                    struct _mulle_aba_storage *q);

MULLE__ABA_GLOBAL
void   _mulle_aba_world_assert_sanity( struct _mulle_aba_world *world);


static inline _mulle_aba_worldpointer_t
   _mulle_aba_storage_get_worldpointer( struct _mulle_aba_storage *q)
{
   _mulle_aba_worldpointer_t   world_p;

   // if your thread isn't registered yet, you must not read the struct
   // it may be dealloced already
   world_p = (_mulle_aba_worldpointer_t) _mulle_atomic_pointer_read( &q->_world.pointer);
#if MULLE_ABA_TRACE
   {
      extern char  *mulle_aba_thread_name( void);

      fprintf( stderr, "%s: read world as %p\n", mulle_aba_thread_name(), world_p);
   }
#endif
   return( world_p);
}


// TODO: don't use free_entries for this
static inline void   _mulle_aba_storage_add_leak_world( struct _mulle_aba_storage *q,
                                                        struct _mulle_aba_world *orphan)
{
   // must not zero orphan
   _mulle_concurrent_linkedlist_add( &q->_leaks, (void *) orphan);
#if MULLE_ABA_TRACE
   fprintf( stderr, "%s: add leak world %p to storage %p\n",
                        mulle_aba_thread_name(), orphan, q);
#endif
}


MULLE__ABA_GLOBAL
void   _mulle_aba_storage_free_leak_worlds( struct _mulle_aba_storage *q);

MULLE__ABA_GLOBAL
void   _mulle_aba_storage_free_unused_worlds( struct _mulle_aba_storage *q);

MULLE__ABA_GLOBAL
void   _mulle_aba_storage_free_unused_free_entries( struct _mulle_aba_storage *q);


# pragma mark -
# pragma mark world atomic world changes

enum _mulle_swap_intent
{
   _mulle_swap_register_intent,
   _mulle_swap_unregister_intent,
   _mulle_swap_free_intent,
   _mulle_swap_checkin_intent,
   _mulle_swap_lock_intent,
   _mulle_swap_unlock_intent
};


enum
{
   _mulle_aba_world_modify,
   _mulle_aba_world_reuse,
   _mulle_aba_world_discard  // always returns 0
};


struct _mulle_aba_callback_info
{
   struct _mulle_aba_world   *new_world;
   struct _mulle_aba_world   *old_world;
   int                       old_bit;
   int                       new_bit;
   int                       old_locked;
   int                       new_locked; // only used by copy_change_world for now
};


MULLE__ABA_GLOBAL
_mulle_aba_worldpointer_t
   _mulle_aba_storage_try_set_lock_worldpointer( struct _mulle_aba_storage *q,
                                                 _mulle_aba_worldpointer_t old_world_p,
                                                 int bit);


static inline _mulle_aba_worldpointer_t
_mulle_aba_storage_try_lock_worldpointer( struct _mulle_aba_storage *q,
                                          _mulle_aba_worldpointer_t old_world_p)
{
   return( _mulle_aba_storage_try_set_lock_worldpointer( q, old_world_p, 1));
}


static inline _mulle_aba_worldpointer_t
_mulle_aba_storage_try_unlock_worldpointer( struct _mulle_aba_storage *q,
                                             _mulle_aba_worldpointer_t old_world_p)
{
   return( _mulle_aba_storage_try_set_lock_worldpointer( q, old_world_p, 0));
}


// this is now a static (hiding the uglies :))
//struct _mulle_aba_worldpointers
//   _mulle_aba_storage_lock_worldpointer( struct _mulle_aba_storage *q);


MULLE__ABA_GLOBAL
struct _mulle_aba_worldpointers
_mulle_aba_storage_change_worldpointer_2( struct _mulle_aba_storage *q,
                                          enum _mulle_swap_intent intention,
                                          _mulle_aba_worldpointer_t  old_world_p,
                                          int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                          void *userinfo);

MULLE__ABA_GLOBAL
struct _mulle_aba_worldpointers
_mulle_aba_storage_change_worldpointer( struct _mulle_aba_storage *q,
                                        enum _mulle_swap_intent intention,
                                        _mulle_aba_worldpointer_t  old_world_p,
                                        int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                        void *userinfo);

MULLE__ABA_GLOBAL
struct _mulle_aba_worldpointers
_mulle_aba_storage_copy_lock_worldpointer( struct _mulle_aba_storage *q,
                                           int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                           void *userinfo,
                                           struct _mulle_aba_world **dealloced);

MULLE__ABA_GLOBAL
struct _mulle_aba_worldpointers
_mulle_aba_storage_copy_change_worldpointer( struct _mulle_aba_storage *q,
                                             enum _mulle_swap_intent intention,
                                             _mulle_aba_worldpointer_t old_world_p,
                                             int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                             void *userinfo);

MULLE__ABA_GLOBAL
struct _mulle_aba_worldpointers
_mulle_aba_storage_grow_worldpointer( struct _mulle_aba_storage *q,
                                      unsigned int target_size,
                                      int (*callback)( int, struct _mulle_aba_callback_info *, void *),
                                      void *userinfo);


MULLE__ABA_GLOBAL
void   _mulle_aba_world_check_timerange( struct _mulle_aba_world *world,
                                         uintptr_t old,
                                         uintptr_t new,
                                         struct _mulle_aba_storage *q);

MULLE__ABA_GLOBAL
unsigned int  _mulle_aba_world_reuse_storages( struct _mulle_aba_world *world);

MULLE__ABA_GLOBAL
unsigned int
   _mulle_aba_world_count_available_reusable_storages( struct _mulle_aba_world *world);

#if DEBUG
MULLE__ABA_GLOBAL
void   _mulle_concurrent_linkedlist_print( struct _mulle_concurrent_linkedlist *p);
#endif

#endif /* defined(__test_delayed_deallocator_storage__delayed_deallocator_storage__) */
