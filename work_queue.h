#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#include "intrinsics.h"
#include "platform_semaphore.h"
#include "memory_arena.h"

#define WORK_QUEUE_CALLBACK( name ) void name( void * const data, MemoryArena * const arena )
typedef WORK_QUEUE_CALLBACK( WorkQueueCallback );

struct Job {
	WorkQueueCallback * callback;
	void * data;
};

struct WorkQueue {
	Job jobs[ 256 ];

	Semaphore sem;

	// using head/length means we need to an atomic pair which is a pain
	volatile u16 head;
	volatile u16 tail;

	volatile u16 jobs_queued;
	volatile u16 jobs_completed;

	u32 num_threads;
	MemoryArena * arenas;
};

void workqueue_init( WorkQueue * const queue, MemoryArena * const arena, const u32 num_threads );
void workqueue_enqueue( WorkQueue * const queue, WorkQueueCallback * const callback, void * const data );
void workqueue_exhaust( WorkQueue * const queue );

#endif // _WORK_QUEUE_H_
