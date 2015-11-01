#ifndef _WAVE_H_
#define _WAVE_H_

#include "intrinsics.h"
#include "memory_arena.h"
#include "assets.h"

bool wave_decode( MemoryArena * arena, u8 * data, Sound * sound );

#endif // _WAVE_H_
