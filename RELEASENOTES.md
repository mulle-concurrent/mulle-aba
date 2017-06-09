### 1.4.7

* modernized project


1.4.6
===

* community version

1.4.4
===

* merge with community version

1.4.3
===

* merge in fixes from community version

1.4.2
===

* fixes for community version

1.4.1
===

* fix documentation link

1.4.0
===

* recompile because `mulle_allocator` changed dramatically in layout

1.3.5
===

* improved .travis.yml

1.3.4
===

* use find_library in CMakeLists.txt


1.3.3
===

* moved to indirect URLs in .bootstrap


1.3.2
===

* cleanup


1.3.1
===

* fix test compile on windows

1.3
===
   * prettied up the docuementation for GitHub release
   * fix compile problems on Windows
   * unify infrastructure with other mulle projects
   * unify documentation

1.2
===
   * adapt to changes in mulle_thread
   * moved test code into ./tests/

1.1
===
   * `struct mulle_aba` is no longer compatible to `struct mulle_allocator`,
   because the smart mode sucked.

1.0
===
   * Eradicated the use of "block" as a synonym for pointer.
   * Renamed `_mulle_aba_thread_free_block` to `_mulle_aba_free`.
   * `mulle_aba_set_global` now returns only **void**. It MUST be called before
   `mulle_aba_init`. (And it's probably of not much use anyway.)
   * Enables true multiple aba instances by not sharing a global tss key.
   * Removed useless "thread" parameter from internal API
   * Added `_current_thread` to some internal API to confuse less
   * Due to limitations in pthreads (the thread local storage is gone in the
     destructor), the automatic unregistration of threads is no longer
     supported. This fixes a leak.
   * Reversed arguments of `mulle_aba_free`. Sorry but better now than never.
   * Removed yield parameter from init functions, will now use mulle_thread_yield.
   * Renamed `struct _mulle_aba` to `struct mulle_aba`, since it's API.
   * Introduced `mulle_aba_get_global` for some lazy test code cases.
   * Renamed `_mulle_aba_free_pointer` to `_mulle_aba_free`, so its like the
   global operating function.
   * Added an assert to `_mulle_aba_free`, that checks that the current thread
     is registered.
   * Experimentally make `struct mulle_aba` compatible to `struct mulle_allocator`.

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

   int   _mulle_aba_thread_free_block( struct mulle_aba *p,
                                       mulle_thread_t thread,
                                       void *owner,
                                       void (*p_free)( void *, void *),
                                       void *pointer);

   The owner, which more often than not is NULL, is now separate from the
   pointer, which makes the type checking a bit easier.
