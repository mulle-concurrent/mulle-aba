//
//  mulle_aba_linkedlist.c
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
#pragma clang diagnostic ignored "-Wparentheses"

#include "mulle-aba-linkedlist.h"

#include "include-private.h"
#include "mulle-aba-defines.h"
#include <assert.h>


//
// retrieves the current head pointer and sets it to NULL in one atomic
// operation
//
struct _mulle_aba_linkedlistentry  *
   _mulle_aba_linkedlist_remove_all( struct _mulle_aba_linkedlist *list)
{
   struct _mulle_aba_linkedlistentry  *head;

   assert( list);

   do
   {
      head = _mulle_atomic_pointer_read( &list->_head.pointer);
      if( ! head)
         break;
   }
   while( ! _mulle_atomic_pointer_weakcas( &list->_head.pointer, NULL, head));

   return( head);
}



void  _mulle_aba_linkedlist_add( struct _mulle_aba_linkedlist *list,
                                 struct _mulle_aba_linkedlistentry  *entry)
{
   struct _mulle_aba_linkedlistentry  *head;

   assert( list);
   assert( entry);
   assert( ! entry->_next);

   do
   {
      head = _mulle_atomic_pointer_read( &list->_head.pointer);
      assert( head != entry);

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
      entry->_next = head;
   }
   while( ! _mulle_atomic_pointer_cas( &list->_head.pointer, entry, head));
}


//
// extract top entry from list
// this is surprisingly difficult, as when done naively this is ABA all over
// again
// use _mulle_aba_linkedlist_remove_all to get the whole chain
// then lop one off, and keep removing and chaining stuff from the original list
// until we can finally place the whole chain back into an empty list
//
struct _mulle_aba_linkedlistentry  *
   _mulle_aba_linkedlist_remove_one( struct _mulle_aba_linkedlist *list)
{
   struct _mulle_aba_linkedlistentry   *prev_chain;
   struct _mulle_aba_linkedlistentry   *chain;
   struct _mulle_aba_linkedlistentry   *next;
   struct _mulle_aba_linkedlistentry   *tail;
   struct _mulle_aba_linkedlistentry   *entry;

   assert( list);

   entry = NULL;
   chain = NULL;

   do
   {
      prev_chain = chain;
      chain      = _mulle_aba_linkedlist_remove_all( list);
      MULLE_THREAD_UNPLEASANT_RACE_YIELD();
      if( chain)
      {
         if( ! entry)
         {
            entry = chain;
            chain = chain->_next;
            entry->_next = NULL;
         }

         // chain together
         if( prev_chain)
         {
            for( tail = chain; next = tail->_next; tail = next)
            {
               assert( next != prev_chain);
               MULLE_THREAD_UNPLEASANT_RACE_YIELD();
            }
            tail->_next = prev_chain;
         }
      }
      else
         chain = prev_chain;

      if( ! chain)
         break;
   }
   while( ! _mulle_atomic_pointer_weakcas( &list->_head.pointer, chain, NULL));

   // the contents of this area will get reused now so invalidate
   // mulle_atomic_memory_barrier();

   return( entry);
}




int
   _mulle_aba_linkedlist_walk( struct _mulle_aba_linkedlist *list,
                               int (*callback)( struct _mulle_aba_linkedlistentry *,
                                                struct _mulle_aba_linkedlistentry *,
                                                void *),
                              void *userinfo)
{
   struct _mulle_aba_linkedlistentry   *entry;
   struct _mulle_aba_linkedlistentry   *prev;
   struct _mulle_aba_linkedlistentry   *next;
   int                                 rval;

   assert( list);
   assert( callback);

   entry = _mulle_atomic_pointer_nonatomic_read( &list->_head.pointer);
   prev  = NULL;
   rval  = 0;
   MULLE_THREAD_UNPLEASANT_RACE_YIELD();

   while( entry)
   {
      next = entry->_next; // do this now, so callback can remove it
      rval = (*callback)( entry, prev, userinfo);
      if( rval)
         break;

      MULLE_THREAD_UNPLEASANT_RACE_YIELD();

      prev  = entry;
      entry = next;  // now
   }

   return( rval);
}


