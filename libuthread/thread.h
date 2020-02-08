#ifndef _THREAD_H
#define _THREAD_H

#include <pthread.h>

/*
 * thread_block - Block thread
 *
 * By calling this function the current thread becomes blocked. It can only be
 * unblocked by another thread calling `thread_unblock()`.
 *
 * If this function is called in a critical section (i.e. within a block of code
 * located after a call to 'enter_critical_section()'), it will exit the
 * critical section before going to sleep and re-enter the critical section upon
 * wake-up.
 *
 * Return: -1 in case of failure, 0 otherwise
 */
int thread_block(void);

/*
 * thread_unblock - Unblock thread
 * @tid: Thread ID
 *
 * Unblock thread @tid and make it ready for scheduling.
 *
 * Return: -1 if @tid does not correspond to a currently blocked thread. 0 if
 * thread @tid was successfully unblocked.
 */
int thread_unblock(pthread_t tid);

/*
 * enter_critical_section - Enter critical section
 *
 * Call this function when entering a critical section in order to ensure mutual
 * exclusion with other threads.
 */
void enter_critical_section(void);

/*
 * exit_critical_section - Exit critical section
 *
 * Call this function when leaving a critical section in order to allow another
 * thread who was waiting to enter the same critical section to enter.
 */
void exit_critical_section(void);

#endif /* _THREAD_H */
