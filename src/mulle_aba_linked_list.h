//
//  mulle_aba_linked_list.c
//  mulle-aba
//
//  Created by Nat! on 01.07.15.
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

#ifndef mulle_aba_linked_list_h__
#define mulle_aba_linked_list_h__

#include "mulle_atomic.h"

#include <string.h>
#include <assert.h>



struct _mulle_aba_linked_list_entry
{
   struct _mulle_aba_linked_list_entry   *_next;
   
#if DEBUG
   uintptr_t   _memtype;
#endif
   
   void   (*_free)( void *, void *);  // owner, pointer
   void   *_owner;
   void   *_pointer;
};


struct _mulle_aba_linked_list
{
   mulle_atomic_ptr_t   _head;
};


static inline void    _mulle_aba_linked_list_init( struct _mulle_aba_linked_list *p)
{
   memset( p, 0, sizeof( *p));
}


static inline void   _mulle_aba_linked_list_done( struct _mulle_aba_linked_list *p)
{
   assert( ! p->_head._nonatomic);
}


//
// limited functionality, add to back (single)
// remove all
//
void  _mulle_aba_linked_list_add( struct _mulle_aba_linked_list *list, struct _mulle_aba_linked_list_entry  *entry);
struct _mulle_aba_linked_list_entry  *_mulle_aba_linked_list_remove_all( struct _mulle_aba_linked_list *list);

// based on remove all 
struct _mulle_aba_linked_list_entry  *_mulle_aba_linked_list_remove_one( struct _mulle_aba_linked_list *list);

//
// NOT THREADSAFE AT ALL
//
int   _mulle_aba_linked_list_walk( struct _mulle_aba_linked_list *list,
                                     int (*callback)( struct _mulle_aba_linked_list_entry *,
                                                      struct _mulle_aba_linked_list_entry *,
                                                     void *),
                                     void *userinfo);

#endif
