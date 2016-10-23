//
//  main.m
//  example
//
//  Created by Nat! on 02.03.16.
//  Copyright © 2016 Mulle kybernetiK. All rights reserved.
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

#define MULLE_THREAD_USE_PTHREADS

#include "mulle_aba.h"

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static pthread_key_t    thread_name_key;
static pthread_mutex_t  output_lock;


static char  *mulle_aba_thread_name( void)
{
   return( pthread_getspecific( thread_name_key));
}


static void   trace( char *format, ...)
{
   va_list   args;

   pthread_mutex_lock( &output_lock);
   {
      fprintf( stderr, "%s (%ld): ", mulle_aba_thread_name(), mulle_aba_current_thread_get_timestamp());

      va_start( args, format);
      vfprintf( stderr, format, args);
      va_end( args);

      fputc( '\n', stderr);
   }
   pthread_mutex_unlock( &output_lock);
}


static void  print_c( void *c)
{
   printf( "%s: '%c'\n", mulle_aba_thread_name(), (int) (intptr_t) c);
}


static int  acquire_c( void)
{
   return( rand() % 26 + 'A');
}


static void  run_consumer( void *name)
{
   int   c;
   int   i;

   pthread_setspecific( thread_name_key, strdup( name));

   trace( "mulle_aba_register");
   mulle_aba_register();

   i = 0;
   for(;;)
   {
      c = acquire_c();

      trace( "mulle_aba_free");
      mulle_aba_free( print_c, (void *) (intptr_t) c);

      switch( ++i & 0x3)
      {
      case 0x0  :
         trace( "mulle_aba_checkin");
         mulle_aba_checkin();
         break;

      case 0x3 :
         if( i > 10)
            goto done;
      }
      sleep( 1);
   }

done:

   trace( "mulle_aba_unregister");
   mulle_aba_unregister();
}


/*
 * this thread uses getchar, which would block mulle_aba from releasing
 * until getchar returns. In this case, it would be preferable to unregister
 * before calling getchar
 */
static void  run_blocker( void *name)
{
   int   c;

   pthread_setspecific( thread_name_key, strdup( name));

   trace( "mulle_aba_unregister");
   mulle_aba_register();

   printf( "\npress RETURN to stop\n\n");
   fflush( stdout);

   if( ! getenv( "DONT_UNREGISTER_BEFORE_GETCHAR"))
   {
      trace( "mulle_aba_unregister");
      mulle_aba_unregister();
   }

   c = getchar();

   if( ! getenv( "DONT_UNREGISTER_BEFORE_GETCHAR"))
   {
      trace( "mulle_aba_register");
      mulle_aba_register();
   }

   trace( "mulle_aba_free");
   mulle_aba_free( print_c, (void *) (intptr_t) c);

   trace( "mulle_aba_unregister");
   mulle_aba_unregister();
}


//
// this example uses pthreads  to make it look more familiar
// it does nothing special, just shows the use of the API
//
int   main(int argc, const char * argv[])
{
   unsigned int   i;
   unsigned int   j;
   int            rval;
   pthread_t      consumer[ 2];
   pthread_t      blocker;

   /* give each thread a name for easier output */
   pthread_key_create( &thread_name_key, free);
   pthread_setspecific( thread_name_key, strdup( "main"));

   /* serialize mutiple fprintfs */
   pthread_mutex_init( &output_lock, NULL);

   trace( "mulle_aba_init");
   mulle_aba_init( NULL);

   pthread_create( &consumer[ 0], NULL, (void *) run_consumer, "consumer[ 0]");
   pthread_create( &consumer[ 1], NULL, (void *) run_consumer, "consumer[ 1]");
   pthread_create( &blocker, NULL, (void *) run_blocker, "blocker");

   pthread_join( consumer[ 0], NULL);
   pthread_join( consumer[ 1], NULL);
   pthread_join( blocker, NULL);

   trace( "mulle_aba_done");
   mulle_aba_done();

   // pedantism
   pthread_mutex_destroy( &output_lock);

   pthread_key_delete( thread_name_key);

   return( 0);
}

