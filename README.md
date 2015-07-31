# mulle_aba

## Intro

**mulle_aba** is a (pretty much) lock-free solution to the [ABA problem](https://en.wikipedia.org/wiki/ABA_problem) base on [pthreads](https://en.wikipedia.org/wiki/POSIX_Threads) written in C.

Threads have to cooperate for this scheme to work. If a non-registered thread is accessing memory referenced by a **mulle_aba** pointer, your program is likely to crash. If a registered thread is not checking in regularly, your program will bloat. Such is the rule of **mulle_aba**.

This page gives you the necessary information to use **mulle_aba** in your own programs. 

![Mind Map](./dox/process-thread.png "Mind Map")

Node Colors

* Red   : single threaded
* Blue  : soft locking, multi threaded
* Black : lock free, multi threaded
* Gray  : context

Edge style

* Dashed : main thread (the one who called mulle_aba_init)
* Solid  : any thread

# API

## mulle_aba_init

`void mulle_aba_init( struct _mulle_aba *aba)`

*Available in state 1 (red)*

Call this function to initialize **mulle_aba**. Don't call any other **mulle_aba** function before calling this. In terms of the diagram you are in State 1 (red).

This function is not thread safe (red).

Ex.

~~~
static struct _mulle_aba   my_aba;

mulle_aba_init( &aba);
~~~


## mulle_aba_done

`void mulle_aba_done( void)`

*Available in state 2 (red) an 6 (purple)*

Call this function to finish **mulle_aba**. Your process needs to call `mulle_aba_init` again before doing anything with **mulle_aba**. Be very sure that all participating threads have unregistered **and** joined before calling **mulle_aba_done**.

This function is not thread safe (red).


## mulle_aba_register

`void   mulle_aba_register( void)`

*Available in states 3 (red) and 5 (purple)*

Your thread must call this function before accessing any shared resources that should be protected by **mulle_aba**. The main thread should also call  **mulle_aba_register**.

Only one thread can be registering (or unregistering) at once, and other threads will be looping. The registration does not block or lock **mulle_aba_free** or **mulle_aba_checkin**!

This is a soft-blocking (blue) operation.

Ex.

~~~
static void  run_thread( void *info)
{
    mulle_aba_register();
    ...
}

...
pthread_create( &threads, NULL, (void *) run_thread, "VfL Bochum 1848");
...
~~~

## mulle_aba_unregister

`void   mulle_aba_unregister( void)`

*Available in state 4 (black)*

You usually don't need to call this function, since it will be called by the thread destructor. You do need to call this if you are the main thread.

Only one thread can be unregistering (or registering) at once, and other threads will be looping. You don't have to call checkin before calling **mulle_aba_unregister**.

This is a soft-blocking (blue) operation. 


## mulle_aba_free

`int   mulle_aba_free( void *pointer, void (*free)( void *))`

*Available in state 4 (black)*

Use this function to free a shared pointer in a delayed way. In a cooperative setting, this guarantees that the freed pointer does not run afoul of the ABA problem. The actual freeing of the pointer is delayed until all threads have checked in.

This operation is lockfree (black). Note that if your **free** operation is locking or blocking, then **mulle_aba**'s main operation is not lockfree.

Ex.

~~~
    s = some_unused_shared_malloced_resource();
    mulle_aba_free( s, free);
~~~



## mulle_aba_checkin

`void   mulle_aba_checkin( void)`

*Available in state 4 (black)*

This function must be periodically by all cooperating threads. It acts like a memory barrier and frees delayed pointers, that have been relinquished by all other threads.

This operation is lockfree (black). If any `free` routine of the pointers blocks or locks, this operation also blocks.





