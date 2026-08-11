#include <mulle-aba/mulle-aba.h>
#include <stdio.h>


static void   *observed_arg1;
static void   *observed_arg2;

static mulle_atomic_pointer_t   helper_registered;
static mulle_atomic_pointer_t   go_flag;


static void   record_free( void *pointer, void *owner)
{
   observed_arg1 = pointer;
   observed_arg2 = owner;
}


static mulle_thread_rval_t   run_helper( void *unused)
{
   MULLE_C_UNUSED( unused);

   mulle_aba_register();

   _mulle_atomic_pointer_nonatomic_write( &helper_registered, (void *) 1);

   // wait until main has done the deferred free, then check in
   while( ! _mulle_atomic_pointer_read( &go_flag))
      mulle_thread_yield();

   mulle_aba_checkin();
   mulle_aba_unregister();

   return( 0);
}


int   main( void)
{
   mulle_thread_t   thread;
   char             owner;
   char             pointer;

   mulle_aba_init( NULL);
   mulle_aba_register();

   _mulle_atomic_pointer_nonatomic_write( &helper_registered, (void *) 0);
   _mulle_atomic_pointer_nonatomic_write( &go_flag, (void *) 0);

   if( mulle_thread_create( (void *) run_helper, NULL, &thread))
   {
      mulle_aba_unregister();
      mulle_aba_done();
      return( 1);
   }

   // wait until helper is registered, so that n_threads == 2
   // and the free below is deferred
   while( ! _mulle_atomic_pointer_read( &helper_registered))
      mulle_thread_yield();

   owner   = 1;
   pointer = 2;

   // deferred: callback must be invoked later with p_free( pointer, owner)
   mulle_aba_free_owned_pointer( record_free, &pointer, &owner);

   _mulle_atomic_pointer_nonatomic_write( &go_flag, (void *) 1);

   mulle_aba_checkin();
   mulle_thread_join( thread);

   mulle_aba_unregister();
   mulle_aba_done();

   if( observed_arg1 == &pointer && observed_arg2 == &owner)
      printf( "delayed callback order pointer-first: OK\n");
   else
      printf( "delayed callback order pointer-first: BUG\n");

   return( 0);
}
