#include <mulle-aba/mulle-aba.h>
#include <stdio.h>


static void   *observed_arg1;
static void   *observed_arg2;


static void   record_free( void *pointer, void *owner)
{
   observed_arg1 = pointer;
   observed_arg2 = owner;
}


int   main( void)
{
   char   owner;
   char   pointer;

   mulle_aba_init( NULL);
   mulle_aba_register();

   owner   = 1;
   pointer = 2;

   // single-threaded: this takes the immediate path and must call
   // p_free( pointer, owner) as implemented in mulle-aba.c
   mulle_aba_free_owned_pointer( record_free, &pointer, &owner);

   if( observed_arg1 == &pointer && observed_arg2 == &owner)
      printf( "immediate callback order pointer-first: OK\n");
   else
      printf( "immediate callback order pointer-first: BUG\n");

   mulle_aba_unregister();
   mulle_aba_done();

   return( 0);
}
