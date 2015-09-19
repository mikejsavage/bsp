#include <err.h>
#include <pthread.h>

struct Thread {
	pthread_t pthread;
};

inline void thread_init( Thread * thread, void * callback( void * ), void * data ) {
	const int ok = pthread_create( &thread->pthread, nullptr, callback, data );
	if( ok == -1 ) {
		err( 1, "pthread_create failed" );
	}
}
