#include <err.h>
#include <pthread.h>
// #include <semaphore.h>
#include <dispatch/dispatch.h>

#include "int.h"
#include "work_queue.h"

// TODO: move me somewhere else
#define assert( predicate ) { if( !( predicate ) ) { __builtin_trap(); } }
#define array_count( x ) ( sizeof( x ) / sizeof( ( x )[ 0 ] ) )

#define read_barrier() asm volatile ( "" ::: "memory" )
#define write_barrier() asm volatile ( "" ::: "memory" )

#include <stdio.h>
static bool workqueue_step( WorkQueue * const queue ) {
	const u16 current_head = queue->head;
	const u16 new_head = ( current_head + 1 ) % array_count( queue->jobs );

	// read_barrier(); TODO:

	if( current_head != queue->tail ) {
		if( __sync_bool_compare_and_swap( &queue->head, current_head, new_head ) ) {
			const Job & job = queue->jobs[ current_head ];
			job.callback( job.data );

			__sync_fetch_and_add( &queue->jobs_completed, 1 );
		}

		return true;
	}

	return false;
}

#include <stdio.h>
static void * workqueue_worker( void * data ) {
	WorkQueue * queue = ( WorkQueue * ) data;

	for( ;; ) {
		if( !workqueue_step( queue ) ) {
			// sem_wait( queue->sem );
			dispatch_semaphore_wait( queue->sem, DISPATCH_TIME_FOREVER );
		}

		// pthread_testcancel();
	}

	return nullptr;
}

void workqueue_init( WorkQueue * const queue, const u32 num_threads ) {
	*queue = { };

	// if( sem_init( queue->sem, 0, 0 ) == -1 ) {
	// 	err( 1, "can't create semaphore" );
	// }
	queue->sem = dispatch_semaphore_create( 0 );

	for( u32 i = 0; i < num_threads; i++ ) {
		pthread_t thread;
		pthread_create( &thread, nullptr, workqueue_worker, queue );
	}
}

void workqueue_enqueue( WorkQueue * const queue, WorkQueueCallback * const callback, void * const data ) {
	assert( queue->jobs_queued < array_count( queue->jobs ) );

	const Job job = { callback, data };

	queue->jobs[ queue->tail ] = job;
	queue->jobs_queued++;

	write_barrier();
	queue->tail = ( queue->tail + 1 ) % array_count( queue->jobs );

	// sem_post( queue->sem );
	dispatch_semaphore_signal( queue->sem );
}

void workqueue_exhaust( WorkQueue * const queue ) {
	while( queue->jobs_completed < queue->jobs_queued ) {
		workqueue_step( queue );
	}

	queue->jobs_queued = 0;
	queue->jobs_completed = 0;
}
