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
typedef struct page_struct{
	void* page_address;
	size_t reference_counter;
};
typedef struct mem_page_struct {
	struct page_struct *page_address;
	pthread_t tid;
} mem_page;

// Find the mempage associated with tid @arg
static int find_tid(void *data, void *arg) {
	mem_page *d = data;
	pthread_t match = *(pthread_t*)arg;
	if (d->tid == match) {
		return 1;
	}
	return 0;
}

static int find_tps_area(void* data, void* arg){
	mem_page *d = data;
	if (d->page_address == arg) {
		return 1;
	}
	return 0;
}
/*
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
*/
static void segv_handler(int sig, siginfo_t *si,
	__attribute__((unused)) void *context)
{
	/*
	* Get the address corresponding to the beginning of the page where the
	* fault occurred
	*/
	void *p_fault = (void*)((uintptr_t)si->si_addr & ~(TPS_SIZE - 1));

	/*
	 * Iterate through all the TPS areas and find if p_fault matches one of them
	 */
	mem_page *guilty_page = NULL;
	queue_iterate(pages, find_tps_area, p_fault, (void**)&guilty_page);
	if (guilty_page)
		/* Printf the following error message */
		fprintf(stderr, "TPS protection error!\n");
	/* In any case, restore the default signal handlers */
	signal(SIGSEGV, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	/* And transmit the signal again in order to cause the program to crash */
	raise(sig);
}
int tps_init(int segv)
{
	if (segv) {
		struct sigaction sa;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = segv_handler;
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGSEGV, &sa, NULL);
	}
	//create static queue to hold mem pages
	pages = queue_create();
	return 0;
}

int tps_create(void)
{
	// TODO: Check if this is a critical section
	enter_critical_section();
	pthread_t tid = pthread_self();
	// Find out whether the current has a tps already
	mem_page* existing_page = NULL;
	queue_iterate(pages, find_tid, (void*)tid, (void**)existing_page);
	// If we GET a page back, we already have a tps
	if (existing_page != NULL) return -1;
	// Otherwise we create a new memory page to hold the tps
	mem_page* new_mem_page = malloc(sizeof(mem_page));
	void* addr = mmap(NULL, TPS_SIZE, PROT_NONE,
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
	queue_iterate(pages, find_tid, (void*)tid, (void**)&existing_page);
	// Make sure that the page actually exists:
	if (existing_page == NULL) return -1;
	queue_delete(pages, (void*)existing_page);
	munmap(existing_page->page_address, TPS_SIZE);
	free(existing_page);
	exit_critical_section();
	return 0;
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	// Error checking:
	if (buffer == NULL) return -1;
	// TODO: check if we need >=
	if ((length + offset) > TPS_SIZE){
		return -1;
	}
	enter_critical_section();
	pthread_t tid = pthread_self();
	mem_page* existing_page = NULL;
	queue_iterate(pages, find_tid, (void*)tid, (void**)&existing_page);
	// Make sure the page actually exists:
	if (existing_page == NULL) return -1;
	void* tps_address = existing_page->page_address;
	mprotect(tps_address, TPS_SIZE, PROT_READ);
	memcpy(buffer, tps_address + offset, length);
	mprotect(tps_address, TPS_SIZE, PROT_NONE);
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
	queue_iterate(pages, find_tid, (void*)tid, (void**)&existing_page);
	// make sure the page actually exists:
	if (existing_page == NULL) return -1;
	// Cur_thread does not have a TPS
	void* tps_address = existing_page->page_address;
	mprotect(tps_address, TPS_SIZE, PROT_WRITE);
	memcpy(tps_address + offset, buffer, length);
	mprotect(tps_address, TPS_SIZE, PROT_NONE);
	exit_critical_section();
	return 0;
}

int tps_clone(pthread_t tid)
{
	enter_critical_section();
	pthread_t cur_tid = pthread_self();
	mem_page* existing_page = NULL;
	// Check if the current thread has a page or not
	queue_iterate(pages, find_tid, (void*)cur_tid, (void**)&existing_page);
	// if we get something back, cur_thread already has a TPS!
	if (existing_page != NULL) return -1;
	// Check if the new tid doesn't have a TPS
	mem_page *src_page = NULL;
	queue_iterate(pages, find_tid, (void*)tid, (void**)&src_page);
	// if we don't get anything back the page does not exist
	if (src_page == NULL) return -1;
	// If we get here, we know cur_thread does NOT have a TPS and @TID
	// DOES have a tps
	// Create a new TPS associated with the current tid
	tps_create();
	// Get the ptr to the page we just created
	mem_page *dst_page = NULL;
	queue_iterate(pages, find_tid, (void*)cur_tid, (void**)&dst_page);
	// TODO: Implement with tps_read, tps_write
	memcpy(dst_page->page_address, src_page->page_address, TPS_SIZE);
	exit_critical_section();
	return 0;
}
