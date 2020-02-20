# ECS 150 Project 3

## Authors
* Hunter Kennedy (915850099, hjkennedy@ucdavis.edu)
* Cameron FitzPatrick (914933438, cjfitzpatrick@ucdavis.edu)

## Semaphore API implementation
Our first decision was to use the given queue.o and thread.o. This is because,  
we know that there are no errors associated with the queue. Our Semaphore  
implementation uses the queue to store blocked threads. We represent our  
Semaphore with a struct that contains both the count, indicating the number  
of resources available, and the queue used to store the TID's of the blocked  
threads. `sem_create()` allocates an instance of this struct and initializes  
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
`tps_destroy` must handle a case where the TPS it is told to destroy has   
references to it.  
`tps_write`  
`tps_clone`  
Basic functions: `tps_create` `tps_read`

## TPS Testing
We Implemented testing in two ways. First we did an API unit test, to make sure  
that our implementation follows the correct specifications. We then created a  
few more complex test cases. In order to check our TPS protection, we added an  
overwriting of `mmap()`. This gives us access to the memory we need in order  
to test the TPS protection signal handler.

We created a more complex test case with
