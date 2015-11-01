#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "intrinsics.h"
#include "assets.h"
#include "memory_arena.h"

struct PlayingSound {
	Sound sound;

	float volume[ 2 ];
	bool loop;

	u32 num_played_samples;

	bool positional;
	glm::vec3 position;

	PlayingSound * next;
};

struct AudioOutputBuffer {
	u32 sample_rate;
	u32 num_samples;

	s16 * samples;
};

struct PlaySoundOptions {
	bool loop;
};

PlayingSound * audio_play_sound( MemoryArena * arena, Sound sound, bool loop = false );
void audio_fill_output_buffer( AudioOutputBuffer * buffer );

#endif // _AUDIO_H_
