#include "intrinsics.h"
#include "work_queue.h"
#include "platform_barrier.h"
#include "platform_thread.h"
#include "platform_atomic.h"
#include "platform_semaphore.h"

struct ThreadInfo {
	u32 thread_id;
	WorkQueue * queue;
	volatile u32 * started_threads;
};

static bool workqueue_step( const u32 thread_id, WorkQueue * const queue ) {
	const u16 current_head = queue->head;
	const u16 new_head = ( current_head + 1 ) % array_count( queue->jobs );

	if( current_head != queue->tail ) {
		if( atomic_cas_u16( &queue->head, current_head, new_head ) ) {
			const Job & job = queue->jobs[ current_head ];
			job.callback( job.data, &queue->arenas[ thread_id ] );

			atomic_add_u16( &queue->jobs_completed, 1 );
		}

		return true;
	}

	return false;
}

static void * workqueue_worker( void * const data ) {
	ThreadInfo * const info = ( ThreadInfo * const ) data;

	WorkQueue * const queue = info->queue;
	const u32 thread_id = info->thread_id;

	write_barrier();
	atomic_add_u32( info->started_threads, 1 );

	for( ;; ) {
		if( !workqueue_step( thread_id, queue ) ) {
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
	ThreadInfo * infos = memarena_push_many( arena, ThreadInfo, num_threads );
	volatile u32 started_threads = 0;

	for( u32 i = 0; i < num_threads; i++ ) {
		infos[ i ] = { i, queue, &started_threads };

		Thread thread;
		thread_init( &thread, workqueue_worker, &infos[ i ] );
	}

	// wait until all threads have a local copy of ThreadInfo
	while( started_threads < num_threads );

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
