#include "intrinsics.h"
#include "memory_arena.h"

void memarena_init( MemoryArena * const arena, u8 * const memory, const size_t size ) {
	arena->memory = memory;
	arena->size = size;
	arena->used = 0;
}

u8 * memarena_push_size( MemoryArena * const arena, const size_t size, const size_t alignment ) {
	size_t base_index = align_TODO( arena->used, alignment );
	size_t new_used = arena->used + size + ( base_index - arena->used );
	assert( new_used <= arena->size );
	assert( new_used >= arena->used );

	arena->used = new_used;

	return arena->memory + base_index;
}

MemoryArena memarena_push_arena( MemoryArena * const arena, const size_t size ) {
	u8 * const memory = memarena_push_size( arena, size );

	MemoryArena new_arena = { };
	memarena_init( &new_arena, memory, size );

	return new_arena;
}

void memarena_clear( MemoryArena * const arena ) {
	arena->used = 0;
	arena->num_checkpoints = 0;
}

MemoryArenaCheckpoint memarena_checkpoint( MemoryArena * const arena ) {
	MemoryArenaCheckpoint cp = { };
	// cp.arena = arena;
	cp.used = arena->used;
	arena->num_checkpoints++;

	return cp;
}

void memarena_restore( MemoryArena * arena, MemoryArenaCheckpoint * const cp ) {
	assert( arena->num_checkpoints > 0 );
	assert( !cp->restored );
	assert( arena->used >= cp->used );

	arena->used = cp->used;
	arena->num_checkpoints--;
	cp->restored = true;
}

MemoryArenaAutoCheckpoint::MemoryArenaAutoCheckpoint( MemoryArena * arena, MemoryArenaCheckpoint cp ) {
	this->arena = arena;
	this->cp = cp;
}

MemoryArenaAutoCheckpoint::~MemoryArenaAutoCheckpoint() {
	memarena_restore( arena, &cp );
}
