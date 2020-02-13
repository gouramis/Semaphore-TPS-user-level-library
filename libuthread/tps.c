#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "queue.h"
#include "thread.h"
#include "tps.h"

static queue_t pages;

typedef struct mem_page_struct {
	void* page_address;
	pthread_t tid;
} mem_page;

static int find_tid(void *data, void *arg) {
	mem_page *d = data;
	pthread_t match = *(pthread_t*)arg;
	if (d->tid == match) {
		return 1;
	}
	return 0;
}

static int has_page(pthread_t tid, void **found) {
	// Checks in pages if pages contains mem_page associated with TID
	// If it does, place the pointer to said mem_page in found.
	*found = NULL;
	queue_iterate(pages, find_tid, &tid, (void**)found);
	if (found == NULL) {
		// We don't have a page
		return 0;
	}
	return 1;
}

int tps_init(__attribute__((unused)) int segv)
{
	pages = queue_create();
	return 0;
}

int tps_create(void)
{
	// TODO: Check if this is a critical section
	enter_critical_section();
	pthread_t tid = pthread_self();
	//queue iterate on tid
	mem_page* existing_page = NULL;
	int cur_thread_has_page = has_page(tid, (void**)&existing_page);
	if (cur_thread_has_page) {
		// Has a page already
		return -1;
	}
	mem_page* new_mem_page = malloc(sizeof(mem_page));
	void* addr = mmap(NULL, TPS_SIZE, PROT_READ|PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	new_mem_page->tid = tid;
	new_mem_page->page_address = addr;
	queue_enqueue(pages, new_mem_page);
	exit_critical_section();
	return 0;
}

int tps_destroy(void)
{
	enter_critical_section();
	pthread_t tid = pthread_self();
	//queue iterate on tid
	mem_page* existing_page = NULL;
	int cur_thread_has_page = has_page(tid, (void**)&existing_page);
	if (!cur_thread_has_page) return -1;
	queue_delete(pages, (void*)existing_page);
	munmap(existing_page->page_address, TPS_SIZE);
	free(existing_page);
	exit_critical_section();
	return 0;
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	if (buffer == NULL) return -1;
	if ((length + offset) > TPS_SIZE){
		return -1;
	}
	enter_critical_section();
	pthread_t tid = pthread_self();
	mem_page* existing_page = NULL;
	int cur_thread_has_page = has_page(tid, (void**)&existing_page);
	if (!cur_thread_has_page) return -1;

	void* tps_address = existing_page->page_address;
	size_t count = offset;
	int buf_count = 0;
	for(;count<length;count++){
		((char*)buffer)[buf_count++] = ((char*)tps_address)[count];
	}
	exit_critical_section();
	return 0;
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	// TODO: Check if shared TPS
	if (buffer == NULL) return -1;
	if ((length + offset) > TPS_SIZE){
		return -1;
	}
	enter_critical_section();
	pthread_t tid = pthread_self();
	mem_page* existing_page = NULL;
	int cur_thread_has_page = has_page(tid, (void**)&existing_page);
	if (!cur_thread_has_page) return -1;
	void* tps_address = existing_page->page_address;
	size_t count = offset;
	int buf_count = 0;
	for(;count<length;count++){
		((char*)tps_address)[count] = ((char*)buffer)[buf_count++];
	}
	exit_critical_section();
	return 0;
}

int tps_clone(pthread_t tid)
{
	enter_critical_section();
	pthread_t cur_tid = pthread_self();
	mem_page* existing_page = NULL;
	// Check if the current thread has a page or not
	queue_iterate(pages, find_tid, (void*)cur_tid, (void**)existing_page);
	if (existing_page != NULL) {
		// Cur_thread already has a TPS
		return -1;
	}
	mem_page *src_page = NULL;
	// Check if the new tid doesn't have a TPS
	queue_iterate(pages, find_tid, (void*)tid, (void**)src_page);
	if (src_page == NULL) {
		// @tid doesn't have a tps
		return -1;
	}
	// If we get here, we know cur_thread does NOT have a TPS and @TID
	// DOES have a tps
	// Create a new TPS associated with the current tid
	tps_create();
	// Get the ptr to the new page
	mem_page *dst_page = NULL;
	queue_iterate(pages, find_tid, (void*)cur_tid, (void**)dst_page);
	memcpy(dst_page->page_address,
		src_page->page_address, TPS_SIZE);
	exit_critical_section();
	return 0;
}
