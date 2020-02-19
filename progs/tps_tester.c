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
static sem_t sem1;
static pthread_t tid[3];
static void *corner_case_tc(__attribute__((unused)) void *arg){
	sem_down(sem1);
	return NULL;
}
static void *corner_case_tb(__attribute__((unused)) void *arg){
	sem_up(sem1);
	return NULL;
}
static void *corner_case_ta(__attribute__((unused)) void *arg){
	sem_down(sem1);
	printf("should hang here\n");
	return NULL;
}
static void *sem_corner_case() {
	sem1 = sem_create(0);
	pthread_create(&tid[0], NULL, corner_case_ta, NULL);
	pthread_create(&tid[1], NULL, corner_case_tb, NULL);
	pthread_create(&tid[2], NULL, corner_case_tc, NULL);
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);
	pthread_join(tid[2], NULL);
	return NULL;
}

static void* tps_init_test_thread(void* arg){
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
	assert(0 == tps_create());
	assert(-1 == tps_create());
	assert(0 == tps_destroy());
	return NULL;
}
static int tps_create_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_create_test_thread, NULL));
	pthread_join(tid, NULL);
	printf("Completed tps_create_test\n");
	return 0;
}

static void* tps_read_test_thread(void* arg) {
	char buf[1024];
	assert(-1 == tps_read(1024, 512, buf));
	assert(-1 == tps_read(1024, 512, buf));
	assert(0 == tps_create());
	char* newstring = "Hello World!\n";
	assert(0 == tps_write(0, 14, newstring));
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, newstring));
	assert(-1 == tps_read(5000, 1, buf));
	assert(-1 == tps_read(4096, 10, buf));
	assert(-1 == tps_read(0, 1, NULL));
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

static void* tps_write_test_thread(void* arg) {
	char buf[1024];
	assert(-1 == tps_write(1024, 512, buf));
	assert(-1 == tps_write(1024, 512, buf));
	assert(0 == tps_create());
	char* newstring = "Hello World!\n";
	assert(0 == tps_write(0, 14, newstring));
	assert(0 == tps_write(15, 14, newstring));
	assert(-1 == tps_write(15, 14, NULL));
	assert(-1 == tps_write(4096, 1, NULL));
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, newstring));
	assert(0 == tps_destroy());
	return NULL;
}
static int tps_write_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_write_test_thread, NULL));
	pthread_join(tid, NULL);
	printf("Completed tps_write_test\n");
	return 0;
}

static void* tps_destroy_test_thread(void* arg) {
	assert(-1 == tps_destroy());
	assert(-1 == tps_destroy());
	assert(0 == tps_create());
	assert(0 == tps_destroy());
	assert(-1 == tps_destroy());
	return NULL;
}
static int tps_destroy_test(void) {
	pthread_t tid;
	assert(0 == pthread_create(&tid, NULL, tps_destroy_test_thread, NULL));
	pthread_join(tid, NULL);
	printf("Completed tps_destroy_test\n");
	return 0;
}
static pthread_t clone_threads[2];
static void* tps_clone_test_threadB(void* arg) {
	// Try to clone a thread without a tps
	assert(-1 == tps_clone(clone_threads[0]));
	assert(0 == tps_create());
	char* newstring = "Hello World!\n";
	assert(0 == tps_write(0, 14, newstring));
	sem_up(*(sem_t*)arg);
	return NULL;
}
static void* tps_clone_test_threadA(void* arg) {
	// Get the semaphore iff threadB has been created
	sem_down(*(sem_t*)arg);
	// Clone the tps, for the first time successfully
	assert(0 == tps_clone(clone_threads[1]));
	// Unsuccessfully clone it
	assert(-1 == tps_clone(clone_threads[1]));
	// Read and validate
	char buf[14];
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, "Hello World!\n"));
	// Write to perform a copy on write

	return NULL;
}
static int tps_clone_test(void) {
	sem_t sem1 = sem_create(0);
	assert(0 == pthread_create(&(clone_threads[0]), NULL,
		tps_clone_test_threadA, (void*)&sem1));
	assert(0 == pthread_create(&(clone_threads[1]), NULL,
		tps_clone_test_threadB, (void*)&sem1));
	pthread_join(clone_threads[0], NULL);
	sem_destroy(sem1);
	printf("Completed tps_clone_test\n");
	return 0;
}
static void tps_access_without_creation_test(void){
	assert(-1 == tps_write(0,TPS_SIZE,"Hello"));
	assert(-1 == tps_read(0,TPS_SIZE,"Hello"));
}
static void *thread1(__attribute__((unused)) void *arg){
	// this thread will create a tps, write into it, and test
	// if it reads the same message
	char *buf = malloc(TPS_SIZE);
	static char *msg = "hello";
	tps_create();
	tps_write(0, TPS_SIZE, msg);
	memset(buf, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buf);
	assert(!memcmp(msg, buf, TPS_SIZE));
	free(buf);
	tps_destroy();
	return NULL;
}
static void create_thread(void){
	pthread_t tid;
	pthread_create(&tid, NULL, thread1, NULL);
	pthread_join(tid, NULL);
}
int main() {
	// -- TPS Testing -- //
	assert(0 == tps_init(0));
	tps_create_test();
	tps_init_test();
	tps_destroy_test();
	tps_read_test();
	tps_write_test();
	tps_clone_test();
	tps_access_without_creation_test();
	create_thread();
	// -- Semaphore Testing -- //
	sem_corner_case();

	return 0;
}
