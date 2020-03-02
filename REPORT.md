This project was created by Cameron Fitzpatrick and Hunter Kennedy at the
University of California, Davis 2020

## Semaphore API implementation
We represent our Semaphore with a struct that contains both the count, indicating  
the number of resources available, and the queue used to store the TID's of the  
blocked threads.  

`sem_create()` allocates an instance of this struct and initializes  
the queue using `queue_create()`. `sem_destroy()` frees these resources.  
`sem_down()` was constructed using a `while` loop to ensure that our corner  
case performs as expected. In the case that some thread A is blocked on a  
semaphore, then thread B calls `sem_up()` and wakes thread A up, but before  
thread A may resume thread C calls `sem_down()` on the same semaphore, thread A  
must continue to be blocked. The while loop ensures that we do not report a  
successful lock until, when actually resuming A, at least one resource is  
available. `sem_up()` simply unblocks the next blocked thread and  
increments the resource counter.  

## TPS API implementation

### Structs and helper functions

We required a way to store and access a list of TPS's that we have created. We  
used a queue to do this. We used two core structs to implement the TPS system.  
The first, `page_struct`, represents the disassociated memory page. This  
contains a counter for the number of TPS's referencing that page, and the  
memory address of the page. The second struct we defined, `tps_struct` is our  
internal representation of an individual TPS. This contains a `page_struct`  
pointer to keep track of which page this TPS points to, as well as the TID  
of the thread who owns this TPS. The queue global queue that holds TPS  
information actually contains pointers to `tps_struct`'s.

We had to write two quick helper functions to use for `queue_iterate()`. These  
functions help us quickly find either a tid or a memory page during an  
iteration of the queue. This provides a fast way to check if pages, or tids  
exist or not, for either error checking, or to grab a page/tid that we need.  

### Signal handler and init

The signal handler is there to help us determine if a segfault was caused by  
accessing an invalid TPS. We use our helper function for the queue to determine  
if `p_fault` is in a TPS area. The `queue_iterate()` function along with our  
helper function gave us a quick way to check if there is a match or not. If  
there is then we give the correct error message.  
`tps_init()` is responsible for the setup of our TPS queue. It is also  
responsible for installing the defined signal handler for segfaults based on  
its parameter.

### Core functions

A few of the core functions are straightforward and require little  
explanation. `tps_create()` allocates the memory for a TPS and maps a memory  
page, and adds the set up TPS to our queue. `tps_read()` simply copies data  
from the memory page into a buffer.  

The more interesting functions require some explanation.  
`tps_destroy()` must handle a case where the TPS it is told to destroy has     
other TPS's pointing to its memory page. In this case, `tps_destroy()` will  
remove the current TPS as a reference to that memory page, and simply free  
the TPS, leaving the memory page alone. If the thread calling `tps_destroy()`  
is the only thread using the memory page, the memory page is unmapped and  
freed in addition to the TPS struct.  

`tps_write()` must trigger a copy-on-write in the case that the the current  
thread is not the only thread referencing its page. Regardless of which thread  
created the original page, the thread calling `tps_write()` will map a new  
page, copy the data from its old page, and then write to the new page. It also  
then must update the old page to indicate that it is no longer referencing it.  
`tps_clone()` is a relatively simple function that creates a new TPS, adds it  
to our queue, and points to the page specified by its parameter `@pid`.

## TPS Testing
We Implemented testing in two ways. First we did an API unit test, to make sure  
that our implementation follows the correct specifications. We then created a  
few more complex test cases. In order to check our TPS protection, we added an  
overwriting of `mmap()`. This gives us access to the memory we need in order  
to test the TPS protection signal handler. Calling the test function  
`test_tps_protection()` will issue a segfault, however it will be handled by  
our TPS Protection error management system. Thus, our `tps_tester.x` should  
intentionally segfault at its last test case.  

We created more complex test cases with `tps_clone_test()` and with   
`cloned_delete_test()`. These functions test if our TPS can handle complex  
cloning with context switches. Specifically, `tps_clone_test` makes sure that  
basic writing, cloning, and reading is handled properly. `cloned_delete_test`  
tests the weird case where a thread calls delete on a TPS that points to  
memory that is shared between at least two TPS's.
