0.4
===
   Renamed _mulle_aba_thread_free_block to just _mulle_aba_free_block.
   
0.3
===
   Remove "fragile" #if DEBUG struct member and replace with something,
   one won't define. More unified naming changes to line up with mulle_objc.

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