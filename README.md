# mulle-aba

**mulle_aba** is a (pretty much) lock-free, cross-platform solution to the
[ABA problem](//en.wikipedia.org/wiki/ABA_problem) written in C.

The ABA problem appears, when you are freeing memory, that is shared by
multiple threads and is not protected by a lock. As the subject matter is
fairly complicated, please read the [Wikipedia article](//en.wikipedia.org/wiki/ABA_problem) and maybe [Preshing: An Introduction to Lock-Free Programming](http://preshing.com/20120612/an-introduction-to-lock-free-programming/) first and then checkout the following items:

* [mulle-aba: How it works 1](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_1.html)
* [mulle-aba: How it works 2](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_2.html)
* [mulle-aba: How it works 3](//www.mulle-kybernetik.com/weblog/2015/mulle_aba_how_it_works_3.html)
* [Example](example/main.m)


Fork      |  Build Status | Release Version
----------|---------------|-----------------------------------
[Mulle kybernetiK](//github.com/mulle-c/mulle-aba) | [![Build Status](https://travis-ci.org/mulle-c/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-c/mulle-aba) | ![Mulle kybernetiK tag](https://img.shields.io/github/tag/mulle-c/mulle-aba.svg) [![Build Status](https://travis-ci.org/mulle-c/mulle-aba.svg?branch=release)](https://travis-ci.org/mulle-c/mulle-aba)


## Install

Install the prerequisites first:

| Prerequisites                                           |
|---------------------------------------------------------|
| [mulle-allocator](//github.com/mulle-c/mulle-allocator) |

Then build and install

```
mkdir build 2> /dev/null
(
   cd build ;
   cmake .. ;
   make install
)
```

Or let [mulle-sde](//github.com/mulle-sde) do it all for you.


## API

* [Aba](dox/API_ABA.md)

### Platforms and Compilers

All platforms and compilers supported by
[mulle-c11](//github.com/mulle-c/mulle-c11) and
[mulle-thread](//github.com/mulle-c/mulle-thread).

## Author

[Nat!](//www.mulle-kybernetik.com/weblog) for
[Mulle kybernetiK](//www.mulle-kybernetik.com) and
[Codeon GmbH](//www.codeon.de)

