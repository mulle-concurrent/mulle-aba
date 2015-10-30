0.2
===
   Use a unified naming scheme across libraries.

0.1
===
   I exposed various underscore prefix methods, because I need them in
   _mulle_objc_runtime, where I want possibly multiple aba instances.
   
   I changed the call signature of:
   
   int   _mulle_aba_thread_free_block( struct _mulle_aba *p,
                                       mulle_thread_t thread,
                                       void *owner,
                                       void (*p_free)( void *, void *),
                                       void *pointer);

   The owner, which more often than not is NULL, is now separate from the 
   pointer, which makes the type checking a bit easier.