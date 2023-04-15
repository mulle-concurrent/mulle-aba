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
* [Example](example/main.m)


| Release Version                                       | Release Notes
|-------------------------------------------------------|--------------
| ![Mulle kybernetiK tag](https://img.shields.io/github/tag//mulle-aba.svg?branch=release) [![Build Status](https://github.com//mulle-aba/workflows/CI/badge.svg?branch=release)](//github.com//mulle-aba/actions)| [RELEASENOTES](RELEASENOTES.md) |


## API

* [Aba](dox/API_ABA.md)







## Add

Use [mulle-sde](//github.com/mulle-sde) to add mulle-aba to your project:

``` sh
mulle-sde add github:/
```

To only add the sources of mulle-aba with dependency
sources use [clib](https://github.com/clibs/clib):


``` sh
clib install --out src/ /
```

Add `-isystem src/` to your `CFLAGS` and compile all the sources that were downloaded with your project.


## Install

### Install with mulle-sde

Use [mulle-sde](//github.com/mulle-sde) to build and install mulle-aba and all dependencies:

``` sh
mulle-sde install --prefix /usr/local \
   https://github.com///archive/latest.tar.gz
```

### Manual Installation

Install the requirements:

| Requirements                                 | Description
|----------------------------------------------|-----------------------
| [mulle-allocator](https://github.com/mulle-c/mulle-allocator)             | 🔄 Flexible C memory allocation scheme
| [mulle-thread](https://github.com/mulle-concurrent/mulle-thread)             | 🔠 Cross-platform thread/mutex/tss/atomic operations in C

Install **mulle-aba** into `/usr/local` with [cmake](https://cmake.org):

``` sh
cmake -B build \
      -DCMAKE_INSTALL_PREFIX=/usr/local \
      -DCMAKE_PREFIX_PATH=/usr/local \
      -DCMAKE_BUILD_TYPE=Release &&
cmake --build build --config Release &&
cmake --install build --config Release
```

## Author

[Nat!](https://mulle-kybernetik.com/weblog) for Mulle kybernetiK


