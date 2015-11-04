#include "intrinsics.h"
#include "audio.h"
#include "assets.h"
#include "memory_arena.h"
#include "platform_barrier.h"
#include "platform_atomic.h"

static PlayingSound ps_head;

static PlayingSound free_ps_head;
static PlayingSound * free_ps_tail = &free_ps_head;

static PlaySoundOptions free_ps_options_head;

// TODO: if someone calls audio_stop_sound after a sound has already been
// reused it will cause problems
// give each playingsound a counter and return the value of that counter from
// audio_play_sound, and then make other ops fail if they don't line up anymore

static PlayingSound * audio_get_free_sound() {
	if( free_ps_head.next == nullptr ) return nullptr;

	// TODO: this might not be correct if the playback thread inserts at the same time
	if( free_ps_tail == free_ps_head.next ) {
		free_ps_tail = &free_ps_head;
	}

	PlayingSound * ps = free_ps_head.next;
	// TODO: because of this
	free_ps_head.next = ps->next;

	return ps;
}

static PlayingSound * audio_put_free_sound( PlayingSound * ps ) {
	ps->next = nullptr;
	write_barrier();
	free_ps_tail->next = ps;
}

PlayingSound * audio_play_sound( MemoryArena * arena, Sound sound, bool loop ) {
	PlayingSound * ps = audio_get_free_sound();
	if( ps == nullptr ) ps = memarena_push_type( arena, PlayingSound );

	*ps = { };
	ps->sound = sound;

	// TODO: grab options from free list
	PlayingSoundOptions * options = memarena_push_type( arena, PlayingSoundOptions );
	*options = { };
	options->volume[ 0 ] = options->volume[ 1 ] = 1.0f;
	options->loop = loop;

	ps->options = options;
	ps->next = ps_head.next;

	write_barrier();

	ps_head.next = ps;

	// TODO: return a PS tagged with generation
	return ps;
}

// TODO: check generation
void audio_stop_sound( PlayingSound * ps ) {
	ps->stopped = true;
}

// TODO: check generation
void audio_set_volume( PlayingSound * ps, float left, float right ) {
	if( right == -1.0f ) right = left;

	// TODO: grab options from free list
	PlayingSoundOptions * options = ( PlayingSoundOptions * ) malloc( sizeof( PlayingSoundOptions ) );
	*options = *ps->options;

	options->volume[ 0 ] = left;
	options->volume[ 1 ] = right;

	// PlayingSoundOptions * old_options = ps->options;
	ps->options = options;

	write_barrier();

	// TODO: add old_options to free list
}

void audio_mix( AudioOutputBuffer * buffer ) {
	for( u32 i = 0; i < buffer->num_samples; i++ ) {
		buffer->samples[ i ] = 0;
	}

	PlayingSound * prev = &ps_head;
	PlayingSound * ps = prev->next;

	while( ps ) {
		assert( ps->sound.sample_rate == buffer->sample_rate );

		if( ps->stopped ) {
			prev->next = ps->next;
			// TODO: add ps to free list
			// TODO: add ps->options to free list
		}
		else {
			// we want a local copy so it doesn't get changed under our noses
			// eg if the game might change the volume of the left
			// channel, then the mixer gets run, then the game
			// changes the right channel
			// it might sound a bit weird
			PlayingSoundOptions options = *ps->options;

			u32 samples_to_write = buffer->num_samples;
			if( !options.loop ) {
				samples_to_write = min_u32( ps->sound.num_samples - ps->num_played_samples, samples_to_write );
			}

			for( u32 i = 0; i < samples_to_write; i++ ) {
				// we should do this in float buffers or we get horrible clipping when things overflow
				buffer->samples[ i ] += options.volume[ 0 ] * ps->sound.samples[ ( i + ps->num_played_samples ) % ps->sound.num_samples ];
			}

			ps->num_played_samples += samples_to_write;
			if( ps->num_played_samples >= ps->sound.num_samples ) {
				if( options.loop ) {
					ps->num_played_samples %= ps->sound.num_samples;
				}
				else {
					// TODO: add ps to free list
					// TODO: add ps->options to free list
					prev->next = ps->next;
				}
			}
		}

		prev = ps;
		ps = ps->next;
	}
}
