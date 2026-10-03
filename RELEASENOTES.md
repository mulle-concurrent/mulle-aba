### 3.1.7


* Add API reference entry for ``_mulle_aba_free_owned_pointer`,` describing its purpose, parameters, and return value.
* Note that `aba` matches the allocator callback signature and ``p_free`` receives both pointer and owner context.



* **BREAKING**: ``_mulle_aba_free_owned_pointer`` now takes a `void *aba`
  argument instead of `struct `mulle_aba` *`, matching ``mulle_allocator_aba_t``
  directly so callers no longer need a cast
* mulle-allocator 9.0.0 or later is now required, as enforced by the
  generated version checks

### 3.1.6





* world/entry cleanup during storage teardown now uses non-atomic writes,
  avoiding unnecessary barriers in single-threaded paths
* debug and trace output now uses relaxed atomic reads where full ordering
  is not required




* corrected filenames in header comments across all source files
* added missing license header to src/generic/include.h
* updated copyright year from 2015 to 2018 across source headers
* moved API TOC from asset/dox/TOC.md to asset/dox/api/toc/index.md

### 3.1.5

Various small improvements
