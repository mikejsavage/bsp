#include <err.h>
#include <pthread.h>

#include "intrinsics.h"
#include "work_queue.h"
#include "platform_barrier.h"
#include "platform_semaphore.h"

struct ThreadIDAndWorkQueue {
	u32 thread_id;
	WorkQueue * queue;
};

static bool workqueue_step( const u32 thread_id, WorkQueue * const queue ) {
	const u16 current_head = queue->head;
	const u16 new_head = ( current_head + 1 ) % array_count( queue->jobs );

	if( current_head != queue->tail ) {
		if( __sync_bool_compare_and_swap( &queue->head, current_head, new_head ) ) {
			const Job & job = queue->jobs[ current_head ];
			job.callback( job.data, &queue->arenas[ thread_id ] );

			__sync_fetch_and_add( &queue->jobs_completed, 1 );
		}

		return true;
	}

	return false;
}

static void * workqueue_worker( void * const data ) {
	ThreadIDAndWorkQueue * const idq = ( ThreadIDAndWorkQueue * const ) data;
	WorkQueue * const queue = idq->queue;

	for( ;; ) {
		if( !workqueue_step( idq->thread_id, queue ) ) {
			semaphore_wait( &queue->sem );
		}
	}

	return nullptr;
}

void workqueue_init( WorkQueue * const queue, MemoryArena * const arena, const u32 num_threads ) {
	*queue = { };
	semaphore_init( &queue->sem );

	queue->num_threads = num_threads;
	queue->arenas = memarena_push_many( arena, MemoryArena, num_threads + 1 );

	for( u32 i = 0; i <= num_threads; i++ ) {
		queue->arenas[ i ] = memarena_push_arena( arena, megabytes( 1 ) );
	}

	MemoryArenaCheckpoint cp = memarena_checkpoint( arena );
	ThreadIDAndWorkQueue * idqs = memarena_push_many( arena, ThreadIDAndWorkQueue, num_threads );

	for( u32 i = 0; i < num_threads; i++ ) {
		idqs[ i ] = { i, queue };

		pthread_t thread;
		pthread_create( &thread, nullptr, workqueue_worker, &idqs[ i ] );
	}

	memarena_restore( arena, &cp );
}

void workqueue_enqueue( WorkQueue * const queue, WorkQueueCallback * const callback, void * const data ) {
	assert( queue->jobs_queued < array_count( queue->jobs ) );

	const Job job = { callback, data };

	queue->jobs[ queue->tail ] = job;
	queue->jobs_queued++;

	write_barrier();
	queue->tail = ( queue->tail + 1 ) % array_count( queue->jobs );

	semaphore_signal( &queue->sem );
}

void workqueue_exhaust( WorkQueue * const queue ) {
	while( queue->jobs_completed < queue->jobs_queued ) {
		workqueue_step( queue->num_threads, queue );
	}

	queue->jobs_queued = 0;
	queue->jobs_completed = 0;
}
