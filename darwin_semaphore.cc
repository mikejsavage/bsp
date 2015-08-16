#include <dispatch/dispatch.h>

#include "platform_semaphore.h"

void semaphore_init( Semaphore * const sem ) {
	sem->sem = dispatch_semaphore_create( 0 );
}

void semaphore_signal( Semaphore * const sem ) {
	dispatch_semaphore_signal( sem->sem );
}

void semaphore_wait( Semaphore * const sem ) {
	dispatch_semaphore_wait( sem->sem, DISPATCH_TIME_FOREVER );
}
