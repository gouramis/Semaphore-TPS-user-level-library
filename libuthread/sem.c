#include <stddef.h>
#include <stdlib.h>

#include "queue.h"
#include "sem.h"
#include "thread.h"

struct semaphore {
	/* TODO Phase 1 */
};

sem_t sem_create(size_t count)
{
	/* TODO Phase 1 */
}

int sem_destroy(sem_t sem)
{
	/* TODO Phase 1 */
}

int sem_down(sem_t sem)
{
	/* TODO Phase 1 */
}

int sem_up(sem_t sem)
{
	/* TODO Phase 1 */
}

int sem_getvalue(sem_t sem, int *sval)
{
	/* TODO Phase 1 */
}

