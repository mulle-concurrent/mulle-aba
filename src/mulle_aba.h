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

#include <stdio.h>
#include <mulle_thread/mulle_thread.h>
#include "mulle_aba_storage.h"

//
// THIS IS THREADSAFE, except where noted
//
struct _mulle_aba
{
   struct _mulle_aba_storage   storage;
};


int   mulle_aba_set_global( struct _mulle_aba *p);


//
// call this before doing anything
//
void   mulle_aba_init( struct _mulle_allocator *allocator);
void   mulle_aba_done( void);

//
// your foundation must call this at the start of each thread once
//
void   mulle_aba_register();

//
// your foundation should call this occasionally, a good place is
// -[NSAutoreleasePool finalize]
//
void   mulle_aba_checkin();

// this will be automatically called on thread destruction, as
// thread_delayed_deallocator_register sees to this
// so... you shouldn't call this!
void   mulle_aba_unregister();


// everybody frees through this
int   mulle_aba_free( void *block, void (*free)( void *));

// same as above but owner, will be first parameter for p_free
int   mulle_aba_free_owned_pointer( void *owner, void *pointer, void (*p_free)( void *owner, void *pointer));


// only really useful for testing
void   mulle_aba_reset( void);
uintptr_t   mulle_aba_get_thread_timestamp( void);
void        *_mulle_aba_get_worldpointer( void);

int   mulle_aba_is_registered( void);
/*
 *
 * functions you need when dealing with multiple aba instances 
 *
 */

static inline int   _mulle_aba_is_setup( struct _mulle_aba *p)
{
   return( _mulle_aba_storage_is_setup( &p->storage));
}


int   _mulle_aba_init( struct _mulle_aba *p,
                       struct _mulle_allocator *allocator,
                       int (*yield)( void));
void   _mulle_aba_done( struct _mulle_aba *p);

int   _mulle_aba_unregister_thread( struct _mulle_aba *p, mulle_thread_t thread);
int   _mulle_aba_register_thread( struct _mulle_aba *p, mulle_thread_t thread);


int   _mulle_aba_thread_free_block( struct _mulle_aba *p,
                                    mulle_thread_t thread,
                                    void *owner,
                                    void (*p_free)( void *, void *),
                                    void *pointer);
                                    
int   _mulle_aba_checkin_thread( struct _mulle_aba *p, mulle_thread_t thread);

#endif /* defined(__test_delayed_deallocator_storage__thread_storage__) */
