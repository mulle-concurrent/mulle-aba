# mulle_aba

#### Why it is needed

Assume you have a program running with 4 threads each accessing the same memory referenced through a pointer.

~~~
static char   *info;

+ (void) load
{
   assert( ! info);
   info = strdup( "VfL Bochum");
}
~~~

Now one thread decides to change that memory, if that memory has still the original string contents:


~~~
+ (void) updateOldInfo
{
   char   *old;
   char   *s;
   
   do
   {
      old = info;
      if( strcmp( old, "VfL Bochum"))   
      {
         old = NULL;
         break;
      }
	  s = strdup( "VfL Bochum 1848");
   }
   while( ! atomic_compare_and_swap( &info, s, old));
   
   if( old)
      free( old);		
}
~~~

This code can exhibit two unwanted anomalies, first it may overwrite the pointer, even if the string is not "VfL Bochum".

How can that happen ? When you `free` old. This memory block is again up for grabs. In an unlucky constellation other threads might intervene between the `strcmp` and `strdup`, free the old pointer and reuse it with different contents. All behind the back of "our thread".

Second it may crash. Assume that the code is freed, just as another thread has just accessed it. The `strcmp` may then access invalid memory and crash.

This problem is the the [ABA problem](https://en.wikipedia.org/wiki/ABA_problem)

#### How it works

`mulle_aba` works as a delayed `free` mechanism. Because it solves a problem that occurs with lock-free algorithms, `mulle_aba` has to be lock-free too. Also because it involves the freeing of resources, it shouldn't by itself allocate and deallocate too much, lest you lose more memory managing stuff than actually freeing.


Whenever a memory block is freed via `mulle_aba`, `mulle_aba` records how many threads are currently running and saves this information as a retain count along with the memory block. A thread occasionally checks in to `mulle_aba` to declare that they are not accessing any possibly freed memory blocks. At that time the retain count of the block is decremented. If it has reached zero it is freed. 

Sounds simple enough, and it simplifies even more once some basic properties are figured out.


##### The Snafu worm

There will not be just one freed memory block, but of course a list of memory blocks.

The list of freed blocks behaves like the [Worm of Bemer](https://www.youtube.com/watch?v=Qenhe6jIPR0) more commonly known as the Snafu worm. With threads coming and going and checking in and blocks being freed, it may stretch, but it never snaps in half. 

In other words, looking at all retain counts at any given moment in time, the values will never decrease from past to present. Which in other words means, that the most delayed block is always freed first. 

Why is that ? Because whenever the value may decrease, due to a thread checking in or completely leaving, all the retain counts are decremented accordingly. Leaving the list in a state as if that thread had not existed. 

##### Using worlds to manage the state

Whenever a memory block is added to the delayed storage, the retain count must be saved alongside it. This is actually the complicated part, because threads will be freeing stuff and checking in and out uncontrollably, and we can't use a semaphore to synchronize.

What you do, like in any other block free algorithm is to build up a world.


#### Basic Assumptions

* A block whose retain count is zero, is only accessible by the current thread. It must free it. 
* code that is single threaded can call malloc/free with impunity
* it is assumed that malloc/calloc is lock-free (mullocator ? :)


#### Rules

* If running single threaded resources are freed immediately.

* When in no-thread or single-threaded mode, the world does not change.
 Why ? Assume we have a world that gets copied, the old world gets freed immediately. A thread that just registers will access invalid memory. But it's not true, the #threads change. 
 
 * Getting rid of worlds, is not easy. A world can be accessed by threads, that aren't registered yet. Therefore their retain count is by priniciple unknown (!). Therefore you first need to register to access the world pointer(!)
 


 





