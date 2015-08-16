#include <err.h>
#include <semaphore.h>

struct Semaphore {
	sem_t * sem;
};

inline void semaphore_init( Semaphore * const sem ) {
	const int ok = sem_init( &sem->sem, 0, 0 );
	if( ok == -1 ) {
		err( 1, "sem_init failed" );
	}
}

inline void semaphore_signal( Semaphore * const sem ) {
	sem_post( sem->sem );
}

inline void semaphore_wait( Semaphore * const sem ) {
	sem_wait( sem->sem );
}
