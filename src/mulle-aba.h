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

#ifndef mulle_aba_h__
#define mulle_aba_h__

//
// community version is always even
//
#define MULLE_ABA_VERSION     ((3 << 20) | (0 << 8) | 1)

#include "include.h"

#include <stdio.h>
#include "mulle-aba-storage.h"

#if MULLE_ALLOCATOR_VERSION < ((4 << 20) | (0 << 8) | 0)
# error "mulle-allocator is too old"
#endif
#if MULLE_THREAD_VERSION < ((4 << 20) | (1 << 8) | 8)
# error "mulle-thread is too old"
#endif

//
// THIS IS THREADSAFE, except where noted
// never copy it, storage contains a world pointer
//
struct mulle_aba
{
   struct _mulle_aba_storage      storage;
   mulle_thread_tss_t             timestamp_thread_key;
};


//
// you must call this before mulle_aba_init if you want to change the
// global. Hint: rarely useful.
//
MULLE_ABA_GLOBAL
void                mulle_aba_set_global( struct mulle_aba *p);

MULLE_ABA_GLOBAL
struct mulle_aba   *mulle_aba_get_global( void);

//
// call this before doing anything
// the allocator is used for mulle_aba internal allocations. Not for
// freeing stuff passed to mulle_aba_free
//
MULLE_ABA_GLOBAL
void   mulle_aba_init( struct mulle_allocator *allocator);

MULLE_ABA_GLOBAL
void   mulle_aba_done( void);

//
// your code must call this at the start of each thread once
//
MULLE_ABA_GLOBAL
void   mulle_aba_register( void);

//
// your code should call this occasionally, a good place could be
// before `select`. You may even want to `mulle_aba_unregister` before
// `select` and `mulle_aba_register` again afterwards. Reason
// being, that this would fix the "getchar" misery.
//
MULLE_ABA_GLOBAL
void   mulle_aba_checkin( void);

//
// your code must call this at the end of each thread
//
MULLE_ABA_GLOBAL
void   mulle_aba_unregister( void);


// everybody frees through this
MULLE_ABA_GLOBAL
int   mulle_aba_free( void (*p_free)( void *), void *pointer);

// same as above but owner, will be first parameter for p_free
MULLE_ABA_GLOBAL
int   mulle_aba_free_owned_pointer( void (*p_free)( void *owner, void *pointer),
                                    void *pointer,
                                    void *owner);

#pragma mark - test support

// only really useful for testing
MULLE_ABA_GLOBAL
void        mulle_aba_reset( void);

// act on global
MULLE_ABA_GLOBAL
void       *mulle_aba_get_worldpointer( void);
MULLE_ABA_GLOBAL
int         mulle_aba_is_registered( void);
MULLE_ABA_GLOBAL
uintptr_t   mulle_aba_current_thread_get_timestamp( void);


#pragma mark - multiple aba support

/*
 *
 * functions you need when dealing with multiple aba instances
 *
 */
static inline int   _mulle_aba_is_setup( struct mulle_aba *p)
{
   return( _mulle_aba_storage_is_setup( &p->storage));
}


MULLE_ABA_GLOBAL
int   _mulle_aba_init( struct mulle_aba *p,
                       struct mulle_allocator *allocator);
MULLE_ABA_GLOBAL
void  _mulle_aba_done( struct mulle_aba *p);

MULLE_ABA_GLOBAL
int   _mulle_aba_unregister_current_thread( struct mulle_aba *p);
MULLE_ABA_GLOBAL
int   _mulle_aba_register_current_thread( struct mulle_aba *p);

MULLE_ABA_GLOBAL
int   _mulle_aba_free( struct mulle_aba *p,
                       void (*p_free)( void *),
                       void *pointer);

MULLE_ABA_GLOBAL
int   _mulle_aba_free_owned_pointer( struct mulle_aba *p,
                                     void (*p_free)( void *, void *),
                                     void *pointer,
                                     void *owner);

MULLE_ABA_GLOBAL
int   _mulle_aba_checkin_current_thread( struct mulle_aba *p);

MULLE_ABA_GLOBAL
int   _mulle_aba_is_current_thread_registered( struct mulle_aba *p);


#pragma mark - test support

MULLE_ABA_GLOBAL
uintptr_t   _mulle_aba_current_thread_get_timestamp( struct mulle_aba *p);
MULLE_ABA_GLOBAL
void        *_mulle_aba_get_worldpointer( struct mulle_aba *p);


#endif
