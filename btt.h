#ifndef _BTT_H_
#define _BTT_H_

#include "memory_arena.h"
#include "heightmap.h"

struct BTT {
	BTT * left;
	BTT * right;

	BTT * left_sibling;
	BTT * right_sibling;
	BTT * bottom;

	// u8 level;
};

struct BTTs {
	BTT * left_root;
	BTT * right_root;
};

BTTs btt_from_heightmap( const Heightmap * const hm, MemoryArena * const arena );

#endif // _BTT_H_
