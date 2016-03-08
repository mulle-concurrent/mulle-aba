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

`mulle-aba` is maintained with `mulle-bootstrap` a homegrown dependency manager.

You can either obtain it like this with **homebrew**

```console
brew tap mulle-kybernetik/software
brew install mulle-bootstrap
```

or manually

```
git clone https://www.mulle-kybernetik.com/repositories/mulle-bootstrap
cd mulle-bootstrap
sudo ./install.sh 
```

# Install on OS X or other Unixes

Use mulle-bootstrap to clone [mulle-aba](//www.mulle-kybernetik.com/software/git/mulle-aba), resolve the dependencies and build it.

```console
mulle-bootstrap clone https://www.mulle-kybernetik.com/software/git/mulle-aba
```


### Compile Flags

For development use no flags.

For production use NDEBUG

* DEBUG : turns on some compile time facilities to aid debugging `mulle-aba`
          itself.
* MULLE_ABA_TRACE : turns on a very detailed amount of tracing, which may be
          too much. There are more detailed MULLE_ABA_TRACE flags available,
          consult the source.



