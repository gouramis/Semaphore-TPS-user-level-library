/*
 * Semaphore simple test
 *
 * Test the synchronization of three threads, by having them print messages in
 * a certain order.
 */

#include <limits.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>

#include <sem.h>

sem_t sem1;
sem_t sem2;
sem_t sem3;

static void *thread2(__attribute__((unused)) void *arg)
{
	sem_down(sem3);		/* Wait for main */
	printf("thread2\n");
	sem_up(sem2);		/* Unblock thread1 */

	return NULL;
}

static void *thread1(__attribute__((unused)) void *arg)
{
	sem_down(sem2);		/* Wait for thread 2 */
	printf("thread1\n");
	sem_up(sem1);		/* Unblock main */

	return NULL;
}

int main(void)
{
	pthread_t tid[2];

	sem1 = sem_create(0);
	sem2 = sem_create(0);
	sem3 = sem_create(0);

	pthread_create(&tid[0], NULL, thread1, NULL);
	sched_yield();		/* Get thread 1 to run */
	pthread_create(&tid[1], NULL, thread2, NULL);
	sched_yield();		/* Get thread 2 to run */

	printf("begin\n");
	sem_up(sem3);		/* Unblock thread 2 */
	sem_down(sem1); 	/* Wait for thread 1 */
	printf("end\n");

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	sem_destroy(sem1);
	sem_destroy(sem2);
	sem_destroy(sem3);

	return 0;
}
