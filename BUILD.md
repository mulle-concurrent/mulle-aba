# Build Information


## What you get

* `libmulle_aba.a`, the mulle-aba static library

* `mulle_aba_test` is a standalone stress test of mulle-aba. If it doesn't crash
it runs forever.
* `mulle_aba_ll_test` is a standlone stress test for a part of mulle-aba. If it
doesn't crash it runs forever.

## Prerequisites

`mulle-aba` is maintained with `mulle-bootstrap` a homegrown dependency manager
It's easiest to install it with

```console
brew tap mulle-kybernetik/software
brew install mulle-bootstrap
```

Then clone [mulle-aba](http://www.mulle-kybernetik.com/software/git/mulle-aba),
resolve the dependencies with mulle-bootstrap and build with xcodebuild.

```console
git clone http://www.mulle-kybernetik.com/software/git/mulle-aba
cd mulle-aba
mulle-bootstrap
xcodebuild install DSTROOT=/ # or cmake
```


### Compile Flags

For development use no flags.

For production use NDEBUG

* DEBUG : turns on some compile time facilities to aid debugging `mulle-aba` itself.
* MULLE_ABA_TRACE : turns on a very detailed amount of tracing, which may be too much. There are more detailed MULLE_ABA_TRACE flags available, consult the source.



