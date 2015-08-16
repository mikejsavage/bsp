// TODO: platform_concurrency.h?
#ifndef _PLATFORM_SEMAPHORE_H_
#define _PLATFORM_SEMAPHORE_H_

#ifdef __APPLE__
#include <dispatch/dispatch.h>

struct Semaphore {
	dispatch_semaphore_t sem;
};
#endif

#ifdef __linux__ // this is wrong
#include <semaphore.h>

struct Semaphore {
	sem_t * sem;
};
#endif

void semaphore_init( Semaphore * const sem );
void semaphore_signal( Semaphore * const sem );
void semaphore_wait( Semaphore * const sem );

#endif // _PLATFORM_SEMAPHORE_H_
