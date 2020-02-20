#include <stddef.h>
#include <stdlib.h>
#include "queue.h"
#include "sem.h"
#include "thread.h"
// here is a structure to hold our semaphore and it's associated queue.
struct semaphore {
	size_t count;
	queue_t blocked_threads;
};

sem_t sem_create(size_t count)
{
	// Allocate the space required for the semaphore
	sem_t new_sem = malloc(sizeof(sem_t));
	if(new_sem == NULL){
		return NULL;
	}
	// Initialize count and the blocked queue
	enter_critical_section();
	new_sem->count = count;
	new_sem->blocked_threads = queue_create();
	exit_critical_section();
	return new_sem;
}

int sem_destroy(sem_t sem)
{
	if(sem == NULL || queue_length(sem->blocked_threads)) {
		return -1;
	}
	enter_critical_section();
	queue_destroy(sem->blocked_threads);
	free(sem);
	exit_critical_section();
	return 0;
}

int sem_down(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
	enter_critical_section();
	// Try to get a lock on the sem
	while(sem->count == 0){
		// We will free this in sem_up
		pthread_t *self = malloc(sizeof(pthread_t));
		*self = pthread_self();
		queue_enqueue(sem->blocked_threads, (void*)self);
		thread_block();
		// Running this in a while loop ensures that we truly have a lock on the
		// sem, validating the corner case.
	}
	sem->count -= 1;
	exit_critical_section();
	return 0;
}

int sem_up(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
	enter_critical_section();
	//increase resource count for calling thread
	sem->count += 1;
	pthread_t *next_blocked_thread = NULL;
	queue_dequeue(sem->blocked_threads, (void**)&next_blocked_thread);
	if (next_blocked_thread != NULL) {
		thread_unblock(*next_blocked_thread);
		// Free the memory we allocated in sem_down
		free(next_blocked_thread);
	}
	exit_critical_section();
	return 0;
}

int sem_getvalue(sem_t sem, int *sval)
{
	if (sem == NULL || sval == NULL) {
		return -1;
	}
	enter_critical_section();
	if(sem->count > 0){
		// If the count is positive, just set sval to the count.
		*sval = sem->count;
		exit_critical_section();
		return 0;
	}
	if(sem->count == 0){
		// If the count is zero we should set sval the number of blocked threads
		// as a negative number.
		int len = queue_length(sem->blocked_threads);
		*sval = -1 * len;
		exit_critical_section();
		return 0;
	}
	exit_critical_section();
	return -1;
}
