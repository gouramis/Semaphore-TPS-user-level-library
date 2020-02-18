#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <tps.h>
#include <sem.h>
// static pthread_t* threads;
/*
static void *corner_case_tc(void* arg){
	enter_critical_section();
	printf("'C' calls down\n");
	sem_down(*(sem_t*)arg);
	printf("'C' dies\n");
	exit_critical_section();
	return 0;
}
static void *corner_case_tb(void* arg){
	enter_critical_section();
	printf("'B' calls up\n");
	sem_up(*(sem_t*)arg);
	pthread_join(threads[2], NULL);
	printf("Returned to B\n");
	exit_critical_section();
	return 0;
}
static void *corner_case_ta(void* arg){
	enter_critical_section();
	// Get blocked on the semaphore
	printf("'A' gets blocked...\n");
	sem_down(*(sem_t*)arg);
	pthread_join(threads[1], NULL);
	printf("Returns to 'A'\n");
	exit_critical_section();
	return 0;
}
static int sem_corner_case() {
	sem_t sem1 = sem_create(0);
	threads = malloc(sizeof(pthread_t)*3);
	pthread_create(&threads[0], NULL, corner_case_ta, (void*)&sem1);
	pthread_create(&threads[1], NULL, corner_case_tb, (void*)&sem1);
	pthread_create(&threads[2], NULL, corner_case_tc, (void*)&sem1);
	pthread_join(threads[0], NULL);
	return 0;
}
*/
static void* tps_init_test_thread(void* arg){
	assert(0 == tps_init(0));
	assert(-1 == tps_init(0));
	assert(0 == tps_create());
	assert(0 == tps_destroy());
	return NULL;
}

static int tps_init_test(void) {
	pthread_t tid;
	pthread_create(&tid, NULL, tps_init_test_thread, NULL);
	pthread_join(tid, NULL);
	printf("Completed tps_init_test\n");
	return 0;
}
static void* tps_create_test_thread(void* arg) {
	assert(0 == tps_init(0));
	assert(0 == tps_create());
	assert(-1 == tps_create());
	assert(0 == tps_destroy());
	return NULL;;
}
static int tps_create_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_create_test_thread, NULL));
	pthread_join(tid, NULL);
	printf("Completed tps_create_test\n");
	return 0;
}

static void* tps_destroy_test_thread(void* arg) {
	if (arg) return NULL;
	assert(-1 == tps_destroy());
	assert(0 == tps_init(0));
	assert(0 == tps_create());
	assert(-1 == tps_create());
	assert(0 == tps_destroy());
	assert(-1 == tps_destroy());
	return NULL;
}
static int tps_destroy_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_destroy_test_thread, NULL));
	pthread_join(tid, NULL);
	return 0;
}
static void* tps_read_test_thread(void* arg) {
	char buf[1024];
	assert(-1 == tps_read(1024, 512, buf));
	assert(0 == tps_init(0));
	assert(-1 == tps_read(1024, 512, buf));
	assert(0 == tps_create());
	char* newstring = "Hello World!\n";
	assert(0 == tps_write(0, 14, newstring));
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, newstring));
	assert(0 == tps_destroy());
	return NULL;
}
static int tps_read_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_read_test_thread, NULL));
	pthread_join(tid, NULL);
	printf("Completed tps_read_test\n");
	return 0;
}
int main() {
	tps_create_test();
	tps_init_test();
	tps_destroy_test();
		printf("Completed tps_destroy_test\n");

	tps_read_test();
	
	
	
	// Corner case needs a lot of work...
	// sem_corner_case();

	return 0;
}