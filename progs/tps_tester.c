#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <thread.h>
#include <tps.h>
#include <sem.h>

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
static pthread_t clone_threads[3];
static sem_t sem1, sem2;
static void* tps_clone_test_threadC(void* arg) {
	// Tests complex scenario #2
	sem_down(sem2);
	char msg[6] = "hello";
	// Clone A's TPS
	tps_clone(clone_threads[0]);
	// Write to the cloned TPS to trigger copy on write
	assert(0 == tps_write(6, 6, msg));
	sem_up(sem1);
	tps_destroy();
	return NULL;
}
static void* tps_clone_test_threadB(void* arg) {
	// Try to clone a thread without a tps
	assert(-1 == tps_clone(clone_threads[0]));
	// Create a valid TPS
	assert(0 == tps_create());
	// Cause copy on write into our own clone
	char* newstring = "Hello World!\n";
	assert(0 == tps_write(0, 14, newstring));
	sem_up(sem1);
	sem_down(sem2);
	tps_destroy();
	return NULL;
}
static void* tps_clone_test_threadA(void* arg) {
	// Get the semaphore iff threadB has been created
	sem_down(sem1);
	// Clone the tps, for the first time successfully
	assert(0 == tps_clone(clone_threads[1]));
	// Unsuccessfully clone it
	assert(-1 == tps_clone(clone_threads[1]));
	// Read and validate
	char buf[14];
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, "Hello World!\n"));
	// Free sem2 so thread c can run
	sem_up(sem2);
	// AND so thread B can destroy its TPS
	sem_up(sem2);
	// Block on sem1
	sem_down(sem1);
	// Come back from thread c
	// Check that thread A own clone was not tampered with.
	assert(0 == tps_read(0,14,buf));
	assert(0 == strcmp(buf, "Hello World!\n"));
	sem_up(sem1);
	tps_destroy();
	return NULL;
}
static int tps_clone_test(void) {
	sem1 = sem_create(0);
	sem2 = sem_create(0);
	assert(0 == pthread_create(&(clone_threads[0]), NULL,
		tps_clone_test_threadA, NULL));
	assert(0 == pthread_create(&(clone_threads[1]), NULL,
		tps_clone_test_threadB, NULL));
	assert(0 == pthread_create(&(clone_threads[2]), NULL,
		tps_clone_test_threadC, NULL));
	pthread_join(clone_threads[0], NULL);
	pthread_join(clone_threads[1], NULL);
	pthread_join(clone_threads[2], NULL);
	sem_destroy(sem1);
	printf("Completed tps_clone_test\n");
	return 0;
}
static void tps_access_without_creation_test(void){
	assert(-1 == tps_write(0,TPS_SIZE,"Hello"));
	assert(-1 == tps_read(0,TPS_SIZE,"Hello"));
	printf("Completed tps_access_without_creation_test\n");
}
static void *simple_write_test_thread(__attribute__((unused)) void *arg){
	// this thread will create a tps, write into it, and test
	// if it reads the same message
	char *buf = malloc(TPS_SIZE);
	char msg[6] = "hello";
	tps_create();
	tps_write(0, TPS_SIZE, msg);
	memset(buf, 0, TPS_SIZE);
	tps_read(0, TPS_SIZE, buf);
	assert(!memcmp(msg, buf, TPS_SIZE));
	free(buf);
	tps_destroy();
	return NULL;
}
static void simple_write_test(void){
	pthread_t tid;
	pthread_create(&tid, NULL, simple_write_test_thread, NULL);
	pthread_join(tid, NULL);
	printf("Completed simple_write_test\n");
}
static pthread_t clone_this_tps;
static sem_t sem_master;
static void* cloned_delete_test_thread_master(void* arg) {
	// Create the primary TPS we will clone
	assert(0 == tps_create());
	// Write something into our tps
	assert(0 == tps_write(0, 14, "Hello World!\n"));
	// Get blocked on sem_master
	sem_down(sem_master);
	// Once unblocked, make sure we can still read the tps
	char buf[14];
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, "Hello World!\n"));
	// And then destroy it for good
	assert(0 == tps_destroy());
	return NULL;
}
static void* cloned_delete_test_thread(void* arg) {
	// Clone, read, then destroy the master tps
	assert(0 == tps_clone(clone_this_tps));
	char buf[14];
	assert(0 == tps_read(0, 14, buf));
	assert(0 == strcmp(buf, "Hello World!\n"));
	assert(0 == tps_destroy());
	return NULL;
}
// This function creates children threads that clone a single TPS, read,
// then destroy it. This should not delete the actual shared TPS they point to,
// and the cloned_delete_test_thread_master will verify that it's tps is not
// destroyed before it destroys it.
static void cloned_delete_test() {
	// Create threads that clone a single tps and attempt to destroy their tps's
	sem_master = sem_create(0);
	pthread_create(&clone_this_tps, NULL, cloned_delete_test_thread_master,
		NULL);
	pthread_t child_pthreads[5];
	for (int i = 0; i < 5; i++) {
		pthread_create(&child_pthreads[i], NULL,
			cloned_delete_test_thread, NULL);
	}
	for (int i = 0; i < 5; i++) {
		pthread_join(child_pthreads[i], NULL);
	}
	// Once a bunch of children have run, sem_up to unblock the master
	sem_up(sem_master);
	pthread_join(clone_this_tps, NULL);
	sem_destroy(sem_master);
	printf("Completed cloned_delete_test\n");
}
// mmap overwrite:
void *latest_mmap_addr;
// global variable to make address returned by mmap accessible
void *__real_mmap(void *addr, size_t len, int prot, int flags, int fildes,
	off_t off);
void *__wrap_mmap(void *addr, size_t len, int prot, int flags, int fildes,
	off_t off)
{
    latest_mmap_addr = __real_mmap(addr, len, prot, flags, fildes, off);
	return latest_mmap_addr;
}
static void* tps_prot_thread(void *arg){
	assert(0 == tps_create());
	char *tps_addr = latest_mmap_addr;
	// Cause an intentional TPS protection erro
	tps_addr[0] = 0;
	return NULL;
}
static void *test_tps_protection(){
	pthread_t tid;
	pthread_create(&tid, NULL, tps_prot_thread, NULL);
	pthread_join(tid,NULL);
	return NULL;
}
int main() {
	assert(0 == tps_init(1));
	tps_create_test();
	tps_init_test();
	tps_destroy_test();
	tps_read_test();
	tps_write_test();
	tps_clone_test();
	tps_access_without_creation_test();
	simple_write_test();
	cloned_delete_test();
	//test TPS Protection:
	printf("The next test should segfault!\n");
	test_tps_protection();
	return 0;
}
