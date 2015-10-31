#ifndef _WAVE_H_
#define _WAVE_H_

#include "memory_arena.h"

struct Sound {
	u32 num_samples;
	u32 sample_rate;
	u32 num_channels;

	s16 * samples;
};

bool wave_decode( MemoryArena * arena, u8 * data, Sound * sound );

#endif // _WAVE_H_
