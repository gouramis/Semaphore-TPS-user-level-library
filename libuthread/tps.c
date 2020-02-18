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

static queue_t tps_queue;
struct page_struct{
	void* page_address;
	size_t reference_counter;
};
typedef struct tps_struct {
	struct page_struct *mem_page;
	pthread_t tid;
} tps;
// Find the tps associated with tid @arg
static int find_tid(void *data, void *arg) {
	tps *d = data;
	pthread_t match = *(pthread_t*)arg;
	if (d->tid == match) {
		return 1;
	}
	return 0;
}

static int find_tps_area(void* data, void* arg){
	tps *d = data;
	if (d->mem_page->page_address == arg) {
		return 1;
	}
	return 0;
}

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
	struct page_struct *guilty_page = NULL;
	queue_iterate(tps_queue, find_tps_area, p_fault, (void**)&guilty_page);
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
	if (tps_queue) return -1;
	enter_critical_section();
	if (segv) {
		struct sigaction sa;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = SA_SIGINFO;
		sa.sa_sigaction = segv_handler;
		sigaction(SIGBUS, &sa, NULL);
		sigaction(SIGSEGV, &sa, NULL);
	}
	//create static queue to hold mem tps_queue
	tps_queue = queue_create();
	exit_critical_section();
	return 0;
}

int tps_create(void)
{
	// TODO: Check if this is a critical section
	enter_critical_section();
	pthread_t tid = pthread_self();
	// Find out whether the current has a tps already
	tps* existing_tps = NULL;
	queue_iterate(tps_queue, find_tid, (void*)tid, (void**)&existing_tps);
	// If we GET a page back, we already have a tps
	if (existing_tps != NULL) {
		exit_critical_section();
		return -1;
	}
	// Otherwise we allocate a tps
	tps* new_tps = malloc(sizeof(tps));
	// Allocate the page struct to hold the address and reference_counter
	struct page_struct* new_page = malloc(sizeof(struct page_struct));
	// Set the new tps' page to new_page
	new_tps->mem_page = new_page;
	// Initialize the reference_counter to zero
	new_tps->mem_page->reference_counter = 1;
	void* addr = mmap(NULL, TPS_SIZE, PROT_NONE,
		MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	new_tps->tid = tid;
	new_tps->mem_page->page_address = addr;
	queue_enqueue(tps_queue, new_tps);
	exit_critical_section();
	return 0;
}

int tps_destroy(void)
{
	enter_critical_section();
	if (!tps_queue) return -1;
	pthread_t tid = pthread_self();
	//queue iterate on tid
	tps* existing_tps = NULL;
	queue_iterate(tps_queue, find_tid, (void*)tid, (void**)&existing_tps);
	// Make sure that the page actually exists:
	if (existing_tps == NULL) {
		exit_critical_section();
		return -1;
	}
	// Disassociate the tps
	queue_delete(tps_queue, (void*)existing_tps);
	// If the tps has other threads referencing it, do not unmap tps
	if (existing_tps->mem_page->reference_counter == 1){
		munmap(existing_tps->mem_page->page_address, TPS_SIZE);
		free(existing_tps->mem_page);
	}
	free(existing_tps);
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
	tps* existing_tps = NULL;
	queue_iterate(tps_queue, find_tid, (void*)tid, (void**)&existing_tps);
	// Make sure the page actually exists:
	if (existing_tps == NULL) {
		exit_critical_section();
		return -1;
	}
	void* tps_address = existing_tps->mem_page->page_address;
	mprotect(tps_address, TPS_SIZE, PROT_READ);
	memcpy(buffer, tps_address + offset, length);
	mprotect(tps_address, TPS_SIZE, PROT_NONE);
	exit_critical_section();
	return 0;
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	if (buffer == NULL) return -1;
	if ((length + offset) > TPS_SIZE){
		return -1;
	}
	enter_critical_section();
	pthread_t tid = pthread_self();
	tps* existing_tps = NULL;
	queue_iterate(tps_queue, find_tid, (void*)tid, (void**)&existing_tps);
	// Make sure the page actually exists:
	if (existing_tps == NULL) {
		exit_critical_section();
		return -1;
	}
	// Do I need to perform a clone on the page?
	if (existing_tps->mem_page->reference_counter > 1){
		void *old_page_address = existing_tps->mem_page->page_address;
		// Copy:
		void *addr = mmap(NULL, TPS_SIZE, PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
		// Add read access to exisitng address
		mprotect(old_page_address, TPS_SIZE, PROT_READ);
		memcpy(addr, old_page_address, TPS_SIZE);
		mprotect(addr, TPS_SIZE, PROT_NONE);
		mprotect(old_page_address, TPS_SIZE, PROT_NONE);
		existing_tps->mem_page->reference_counter--;
		struct page_struct *new_page = malloc(sizeof(struct page_struct));
		new_page->reference_counter = 1;
		new_page->page_address = addr;
		existing_tps->mem_page = new_page;
	}
	// Write:
	void* tps_address = existing_tps->mem_page->page_address;
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
	tps *existing_tps = NULL;
	// Check if the current thread has a page or not
	queue_iterate(tps_queue, find_tid, (void*)cur_tid, (void**)&existing_tps);
	// if we get something back, cur_thread already has a TPS!
	if (existing_tps != NULL) return -1;
	// Check if the new tid doesn't have a TPS
	tps *src_page = NULL;
	queue_iterate(tps_queue, find_tid, (void*)tid, (void**)&src_page);
	// if we don't get anything back the page does not exist
	if (src_page == NULL) return -1;
	// If we get here, we know cur_thread does NOT have a TPS and @TID
	// DOES have a tps

	// Create a TPS for myself
	tps *new_tps = malloc(sizeof(tps));
	// Set my mem page to target's mem_page
	new_tps->mem_page = src_page->mem_page;
	// Increment the reference counter
	new_tps->mem_page->reference_counter++;

	// Associate myself with the new_tps
	new_tps->tid = cur_tid;
	queue_enqueue(tps_queue, new_tps);

	exit_critical_section();
	return 0;
}
