#include "intrinsics.h"
#include "audio.h"
#include "assets.h"
#include "memory_arena.h"
#include "platform_barrier.h"
#include "platform_atomic.h"

static PlayingSound playing_sounds_head;

PlayingSound * audio_play_sound( MemoryArena * arena, Sound sound, bool loop ) {
	PlayingSound * ps = memarena_push_type( arena, PlayingSound );
	*ps = { };

	ps->sound = sound;
	ps->volume[ 0 ] = ps->volume[ 1 ] = 1.0f;
	ps->loop = loop;

	write_barrier();

	ps->next = playing_sounds_head.next;
	playing_sounds_head.next = ps;

	return ps;
}

void audio_fill_output_buffer( AudioOutputBuffer * buffer ) {
	for( u32 i = 0; i < buffer->num_samples; i++ ) {
		buffer->samples[ i ] = 0;
	}

	PlayingSound * prev = &playing_sounds_head;
	PlayingSound * ps = prev->next;

	while( ps ) {
		assert( ps->sound.sample_rate == buffer->sample_rate );

		u32 samples_to_write = buffer->num_samples;
		if( !ps->loop ) {
			samples_to_write = min_u32( ps->sound.num_samples - ps->num_played_samples, samples_to_write );
		}

		for( u32 i = 0; i < samples_to_write; i++ ) {
			// we should do this in float buffers or we get horrible clipping when things overflow
			buffer->samples[ i ] += ps->sound.samples[ ( i + ps->num_played_samples ) % ps->sound.num_samples ];
		}

		ps->num_played_samples += samples_to_write;
		if( ps->num_played_samples >= ps->sound.num_samples ) {
			if( ps->loop ) {
				ps->num_played_samples %= ps->sound.num_samples;
			}
			else {
				// atm we just leak ps
				prev->next = ps->next;
			}
		}

		prev = ps;
		ps = ps->next;
	}
}
