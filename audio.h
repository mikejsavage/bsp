#ifndef _AUDIO_H_
#define _AUDIO_H_

#include "intrinsics.h"
#include "assets.h"
#include "memory_arena.h"

struct PlayingSoundOptions {
	float volume[ 2 ];
	bool loop;

	bool positional;
	glm::vec3 position;

	PlayingSoundOptions * next;
};

struct PlayingSound {
	Sound sound;

	// TODO: make this signed so we can delay sounds?
	u32 num_played_samples;

	PlayingSoundOptions * options;
	volatile bool stopped;

	u32 generation;

	PlayingSound * next;
};

struct TaggedPlayingSound {
	u32 generation;
	PlayingSound ps;
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
void audio_stop_sound( PlayingSound * ps );

void audio_set_volume( PlayingSound * ps, float left, float right = -1.0f );

void audio_mix( AudioOutputBuffer * buffer );

#endif // _AUDIO_H_
