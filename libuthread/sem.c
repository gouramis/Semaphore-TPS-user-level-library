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

// -- Static Functions -- //
/*
int test_and_set(int *mem){
	int oldVal = *mem;
	*mem = 1;
	return oldVal;
}
static spinlock_aquire(int *lock) {
	while (test_and_set(lock) == 1);
}
static spinlock_release(int *lock){
	*lock = 0;
}
*/
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
	free(sem);
	return 0;
}

int sem_down(sem_t sem)
{
	if(sem == NULL){
		return -1;
	}
	enter_critical_section();
	pthread_t *self = malloc(sizeof(pthread_t));
	while(sem->count == 0){
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
	// TODO: Wake up next in queue
	pthread_t *next_blocked_thread = NULL;
	queue_dequeue(sem->blocked_threads, (void**)&next_blocked_thread);
	// TODO: Make sure this works
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
