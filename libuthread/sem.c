#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	size_t count;
	queue_t blocked_threads;
	// int lock;
};
sem_t sem_create(size_t count)
{
	sem_t new_sem = malloc(sizeof(sem_t));
	if(new_sem == NULL){
		return NULL;
	}
	new_sem->count = count;
	new_sem->blocked_threads = queue_create();
	// new_sem->lock = 0;
	return new_sem;
}

int sem_destroy(sem_t sem)
{
	if(sem == NULL || queue_length(sem->blocked_threads)) {
		return -1;
	}
	queue_destroy(sem->blocked_threads);
	free(sem);
	return 0;
}

int sem_down(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
	enter_critical_section();
	while(sem->count == 0){
		// Need to malloc in while loop so that our corner case works correctly
		// TODO: Make sure this isn't allocating excessive memory
		pthread_t *self = malloc(sizeof(pthread_t));
		*self = pthread_self();
		queue_enqueue(sem->blocked_threads, (void*)self);
		thread_block();
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
	sem->count += 1;
	pthread_t *next_blocked_thread = NULL;
	queue_dequeue(sem->blocked_threads, (void**)&next_blocked_thread);
	if (next_blocked_thread != NULL) {
		thread_unblock(*next_blocked_thread);
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
	if(sem->count > 0){
		*sval = sem->count;
		return 0;
	}
	if(sem->count == 0){
		int len = queue_length(sem->blocked_threads);
		*sval = -1 * len;
		return 0;
	}
	return -1;
}
