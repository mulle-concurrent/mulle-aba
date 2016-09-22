# Build Information

## What you get

* `libmulle_aba.a`, the mulle-aba static library

* `mulle_aba_test` is a standalone stress test of mulle-aba. If it doesn't crash
it runs forever.
* `mulle_aba_ll_test` is a standlone stress test for a part of mulle-aba. If it
doesn't crash it runs forever.


## Dependencies

* mulle-thread
* mulle-allocator
* mulle-bootstrap (optional)
* cmake 3.0 (optional)
* xcodebuild (optional)


`mulle-aba` is maintained with **mulle-bootstrap** a cross-platform dependency manager.

You can either obtain mulle-bootstrap like this with **homebrew** (on participating platforms)

```console
brew tap mulle-kybernetik/software
brew install mulle-bootstrap
```

or manually

```
git clone https://github.com/mulle-nat/mulle-bootstrap.git
(
   cd mulle-bootstrap ;
   ./install.sh
)
```

# Install on OS X or other Unixes

Use mulle-bootstrap to clone [mulle-aba](//www.mulle-kybernetik.com/software/git/mulle-aba), resolve and build the required dependencies.

```console
mulle-bootstrap -a clone https://www.mulle-kybernetik.com/repositories/mulle-aba
#
```

Use **cmake** to build mulle-aba Makefiles

```
(
   cd mulle-aba/build ;
   cmake ..
)
```

Use **make** to build `mulle-aba` itself

```
(
   cd mulle-aba/build ;
   make
)
```

Use **mulle-bootstrap** to install the dependencies

```
(
   cd mulle-aba ;
   mulle-bootstrap install
)
```



### Compile Flags

For development use no flags.

For production use NDEBUG

* DEBUG : turns on some compile time facilities to aid debugging `mulle-aba`
          itself.
* MULLE_ABA_TRACE : turns on a very detailed amount of tracing, which may be
          too much. There are more detailed MULLE_ABA_TRACE flags available,
          consult the source.



