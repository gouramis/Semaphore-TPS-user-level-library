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

/* TODO: Phase 2 */

int tps_init(int segv)
{
	/* TODO: Phase 2 */
}

int tps_create(void)
{
	/* TODO: Phase 2 */
}

int tps_destroy(void)
{
	/* TODO: Phase 2 */
}

int tps_read(size_t offset, size_t length, void *buffer)
{
	/* TODO: Phase 2 */
}

int tps_write(size_t offset, size_t length, void *buffer)
{
	/* TODO: Phase 2 */
}

int tps_clone(pthread_t tid)
{
	/* TODO: Phase 2 */
}

