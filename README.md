# mulle-aba

🚮 A lock-free, cross-platform solution to the ABA problem, written in C

**mulle_aba** is a (pretty much) lock-free, cross-platform solution to the
[ABA problem](//en.wikipedia.org/wiki/ABA_problem) written in C.

The ABA problem appears, when you are freeing memory, that is shared by
multiple threads and is not protected by a lock. As the subject matter is
fairly complicated, please read the [Wikipedia article](//en.wikipedia.org/wiki/ABA_problem) and maybe [Preshing: An Introduction to Lock-Free Programming](http://preshing.com/20120612/an-introduction-to-lock-free-programming/) first and then checkout the following items:

* [mulle-aba: How it works 1](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_1.html)
* [mulle-aba: How it works 2](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_2.html)
* [mulle-aba: How it works 3](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_3.html)
* [Example](example/main.m)


Build Status | Release Version
-------------|-----------------------------------
[![Build Status](https://travis-ci.org/mulle-concurrent/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-concurrent/mulle-aba) | ![Mulle kybernetiK tag](https://img.shields.io/github/tag/mulle-concurrent/mulle-aba.svg) [![Build Status](https://travis-ci.org/mulle-concurrent/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-concurrent/mulle-aba)


## Install


### Manually

Install the prerequisites first:

| Prerequisites                                              |
|------------------------------------------------------------|
| [mulle-allocator](//github.com/mulle-c/mulle-allocator)    |
| [mulle-thread](//github.com/mulle-concurrent/mulle-thread) |


## Add 

Use [mulle-sde](//github.com/mulle-sde) to add mulle-aba to your project:

```
mulle-sde dependency add --c --github mulle-concurrent mulle-aba
```

## Install

### mulle-sde

Use [mulle-sde](//github.com/mulle-sde) to build and install mulle-aba and all dependencies:

```
mulle-sde install --prefix /usr/local \
   https://github.com/mulle-concurrent/mulle-aba/archive/latest.tar.gz
```

### Manual Installation


Install the requirements:

Requirements                                               | Description
-----------------------------------------------------------|-----------------------
[mulle-allocator](//github.com/mulle-c/mulle-allocator)    | Memory allocation wrapper
[mulle-thread](//github.com/mulle-concurrent/mulle-thread) | Threads and atomics


Install into `/usr/local`:

```
mkdir build 2> /dev/null
(
   cd build ;
   cmake -DCMAKE_INSTALL_PREFIX=/usr/local \
         -DCMAKE_PREFIX_PATH=/usr/local \
         -DCMAKE_BUILD_TYPE=Release .. ;
   make install
)
```

### Conveniently

Or let [mulle-sde](//github.com/mulle-sde) do it all for you with `mulle-sde craft`.


## API

* [Aba](dox/API_ABA.md)

### Platforms and Compilers

All platforms and compilers supported by
[mulle-c11](//github.com/mulle-c/mulle-c11) and
[mulle-thread](//github.com/mulle-concurrent/mulle-thread).

## Author

[Nat!](//www.mulle-kybernetik.com/weblog) for
[Mulle kybernetiK](//www.mulle-kybernetik.com) and
[Codeon GmbH](//www.codeon.de)

