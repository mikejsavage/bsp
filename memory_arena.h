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

void memarena_init( MemoryArena * const arena, u8 * const memory, const size_t size );
u8 * memarena_push_size( MemoryArena * const arena, const size_t size, const size_t alignment = 4 );
#define memarena_push_type( arena, type ) ( ( type * ) memarena_push_size( arena, sizeof( type ), sizeof( type ) ) )
void memarena_clear( MemoryArena * const arena );
MemoryArenaCheckpoint memarena_checkpoint( MemoryArena * const arena );
void memarena_restore( MemoryArena * arena, MemoryArenaCheckpoint * const cp );

#endif // _MEMORY_ARENA_H_
