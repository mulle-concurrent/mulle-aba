//
//  mulle_callback_types.c
//  mulle-types
//
//  Created by Nat! on 24.03.15.
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

#include "mulle_callback_types.h"

#include <stdlib.h>


#pragma mark -
#pragma mark _mulle_allocator

struct _mulle_allocator   mulle_malloc_allocator =
{
   calloc, free
};

#pragma mark -
#pragma mark _mulle_comparator

static unsigned int   pointer_hash( intptr_t value)
{
   int    bits;
   int    shift;
   
   bits  = sizeof( intptr_t) * 8;
   shift = sizeof( intptr_t) * 2;
   
   return( (unsigned int) (((uintptr_t) value >> shift) | ((uintptr_t) value << (bits - shift))));
}


static int   pointer_compare( void *a, void *b)
{
   intptr_t  diff;
   
   diff = (intptr_t) a - (intptr_t) b;
   // signed representation should keep sign, even if smaller 
   return( (int) diff);
}


struct _mulle_comparator   mulle_default_comparator =
{
   pointer_hash, pointer_compare
};


#pragma mark -
#pragma mark _mulle_callbacks

struct _mulle_callbacks   mulle_default_callbacks =
{
   {
      calloc, free
   },
   {
      pointer_hash, pointer_compare
   }
};
