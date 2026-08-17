# mulle-aba

#### 🚮 A lock-free, cross-platform solution to the ABA problem

**mulle_aba** is a (pretty much) lock-free, cross-platform solution to the
[ABA problem](//en.wikipedia.org/wiki/ABA_problem) written in C.

The ABA problem appears, when you are freeing memory, that is shared by
multiple threads and is not protected by a lock. As the subject matter is
fairly complicated, please read the [Wikipedia article](//en.wikipedia.org/wiki/ABA_problem) and maybe [Preshing: An Introduction to Lock-Free Programming](http://preshing.com/20120612/an-introduction-to-lock-free-programming/) first and then checkout the following items:

* [mulle-aba: How it works 1](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_1.html)
* [mulle-aba: How it works 2](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_2.html)
* [mulle-aba: How it works 3](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_3.html)
* [Example](example/main.c)




| Release Version                                       | Release Notes  | AI Documentation
|-------------------------------------------------------|----------------|---------------
| ![Mulle kybernetiK tag](https://img.shields.io/github/tag/mulle-concurrent/mulle-aba.svg) [![Build Status](https://github.com/mulle-concurrent/mulle-aba/workflows/CI/badge.svg)](//github.com/mulle-concurrent/mulle-aba/actions) | [RELEASENOTES](RELEASENOTES.md) | [DeepWiki for mulle-aba](https://deepwiki.com/mulle-concurrent/mulle-aba)


## API

* [Aba](dox/API.md)




## Documentation & Guides

* [API Summary](asset/dox/api/toc)




### You are here

![Overview](overview.dot.svg)





## Add

mulle-aba is a component of the [mulle-core](//github.com/mulle-core/mulle-core) library. So in your code include the mulle-core umbrella header:

``` c
#include <mulle-core/mulle-core.h>
```

### Add mulle-core to a cmake and git project

``` bash
git submodule add https://github.com/mulle-core/mulle-core.git mulle-core
```

Add this to your `CMakeLists.txt`:

``` cmake
add_subdirectory( mulle-core)
target_link_libraries( ${PROJECT_NAME} PRIVATE mulle-core)
```


### Add mulle-core to a mulle-sde project

``` sh
mulle-sde add github:mulle-core/mulle-core
```

### Embed mulle-aba with clib

``` sh
clib install --out src mulle-concurrent/mulle-aba
```

Append `src` to your include path (e.g. add `-isystem src`  to your `CFLAGS`)
and compile all the sources that were downloaded.

## Install

Use [mulle-sde](//github.com/mulle-sde) to build and install mulle-aba and all dependencies:

``` sh
mulle-sde install --prefix /usr/local \
   https://github.com/mulle-concurrent/mulle-aba/archive/latest.tar.gz
```

### Legacy Installation

Install the requirements:

| Requirements                                 | Description
|----------------------------------------------|-----------------------
| [mulle-allocator](https://github.com/mulle-c/mulle-allocator)             | 🔄 Flexible C memory allocation scheme
| [mulle-thread](https://github.com/mulle-concurrent/mulle-thread)             | 🔠 Cross-platform thread/mutex/tss/atomic operations in C
| [mulle-linkedlist](https://github.com/mulle-concurrent/mulle-linkedlist)             | 🔂 mulle-linkedlist a wait and lock-free linked list

Download the latest [tar](https://github.com/mulle-concurrent/mulle-aba/archive/refs/tags/latest.tar.gz) or [zip](https://github.com/mulle-concurrent/mulle-aba/archive/refs/tags/latest.zip) archive and unpack it.

Install **mulle-aba** into `/usr/local` with [cmake](https://cmake.org):

``` sh
PREFIX_DIR="/usr/local"
cmake -B build                               \
      -DMULLE_SDK_PATH="${PREFIX_DIR}"       \
      -DCMAKE_INSTALL_PREFIX="${PREFIX_DIR}" \
      -DCMAKE_PREFIX_PATH="${PREFIX_DIR}"    \
      -DCMAKE_BUILD_TYPE=Release &&
cmake --build build --config Release &&
cmake --install build --config Release
```


## Author

[Nat!](https://mulle-kybernetik.com/weblog) for Mulle kybernetiK  



