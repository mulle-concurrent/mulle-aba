# Build Information


## What you get
 
* `libmulle_aba.a`, the **mulle-aba** static library

* `mulle_aba_test` is a standalone stress test of **mulle-aba**. If it doesn't crash it runs forever.
* `mulle_aba_ll_test` is a standlone stress test for a part of **mulle-aba**. If it doesn't crash it runs forever.

## Prerequisites

### mintomic

**mulle-aba** needs [mintomic](https://mintomic.github.io/) as a prerequisite. Install it in the top directory besides <tt>src</tt> and <tt>dox</tt>.

~~~
git clone https://github.com/mintomic/mintomic
~~~

## Building

### With cmake

~~~
mkdir build
cd build
cmake ..
make
~~~


### With Xcode

~~~
xcodebuild -alltargets  
~~~


### Compile Flags

For development use no flags.

For production use NDEBUG

* DEBUG : turns on some compile time facilities to aid debugging **mulle-aba** itself. 
* TRACE : turns on a very detailed amount of tracing, which may be too much. There are more detailed TRACE flags available, consult the source.



