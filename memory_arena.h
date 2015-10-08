#ifndef _MEMORY_ARENA_H_
#define _MEMORY_ARENA_H_

#include "intrinsics.h"

struct MemoryArena {
	u8 * memory;
	size_t size;
	size_t used;
	size_t num_checkpoints;
};

struct MemoryArenaCheckpoint {
	// MemoryArena * arena;
	size_t used;
	bool restored;
};

struct MemoryArenaAutoCheckpoint {
	MemoryArena * arena;
	MemoryArenaCheckpoint cp;

	MemoryArenaAutoCheckpoint( MemoryArena * arena, MemoryArenaCheckpoint cp );
	~MemoryArenaAutoCheckpoint();
};

void memarena_init( MemoryArena * const arena, u8 * const memory, const size_t size );

u8 * memarena_push_size( MemoryArena * const arena, const size_t size, const size_t alignment = sizeof( void * ) );
#define memarena_push_type( arena, type, ... ) ( ( type * ) memarena_push_size( arena, sizeof( type ), ##__VA_ARGS__ ) )
#define memarena_push_many( arena, type, count, ... ) ( ( type * ) memarena_push_size( arena, sizeof( type ) * count, ##__VA_ARGS__ ) )

MemoryArena memarena_push_arena( MemoryArena * const arena, const size_t size );

void memarena_clear( MemoryArena * const arena );

MemoryArenaCheckpoint memarena_checkpoint( MemoryArena * const arena );
void memarena_restore( MemoryArena * arena, MemoryArenaCheckpoint * const cp );

#define MEMARENA_SCOPED_CHECKPOINT( arena ) MemoryArenaAutoCheckpoint mem_cp##__COUNTER__( arena, memarena_checkpoint( arena ) );


#endif // _MEMORY_ARENA_H_
