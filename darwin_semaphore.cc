#include <dispatch/dispatch.h>

struct Semaphore {
	dispatch_semaphore_t sem;
};

inline void semaphore_init( Semaphore * const sem ) {
	sem->sem = dispatch_semaphore_create( 0 );
}

inline void semaphore_signal( Semaphore * const sem ) {
	dispatch_semaphore_signal( sem->sem );
}

inline void semaphore_wait( Semaphore * const sem ) {
	dispatch_semaphore_wait( sem->sem, DISPATCH_TIME_FOREVER );
}
