#include <stdlib.h>
#include <stdarg.h>
#include <math.h>

#include <alsa/asoundlib.h>

#include "intrinsics.h"
#include "wave.h"
#include "assets.h"
#include "audio.h"

#define FRAMES 1024

static int milliseconds( int ms ) {
	return ms * 1000;
}

static void error_handler( const char * file, int line, const char * function, int err, const char * fmt, ... ) {
	printf( "error in %s at %s:%d\n", file, function, line );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
	printf( "\n" );
}

static Sound * make_sin_wave( u32 sample_rate, u32 frequency ) {
	const u32 num_samples = sample_rate * ( 1.0f / frequency );

	Sound * sound = ( Sound * ) malloc( sizeof( Sound ) );
	sound->samples = ( s16 * ) malloc( num_samples * sizeof( s16 ) );

	sound->num_samples = num_samples;
	sound->sample_rate = sample_rate;
	sound->num_channels = 1;

	for( u32 i = 0; i < num_samples; i++ ) {
		const float t = ( float ) i / num_samples;
		sound->samples[ i ] = 0.25f * INT16_MAX * sinf( t * 2.0f * M_PI );
	}

	return sound;
}

static u8 memory[ megabytes( 64 ) ];
int main( int argc, char ** argv ) {
	snd_lib_error_set_handler( error_handler );

	MemoryArena arena;
	memarena_init( &arena, memory, sizeof( memory ) );

	u8 * wave = file_get_contents( "01 - Abandon.wav" );
	Sound sound;
	bool ok = wave_decode( &arena, wave, &sound );
	assert( ok );

	Sound * sin_wave = make_sin_wave( sound.sample_rate, 200 );

	audio_play_sound( &arena, sound );
	audio_play_sound( &arena, *sin_wave, true );

	const char * pcmname = argc == 2 ? argv[ 1 ] : "default";

	snd_pcm_t * pcm;
	const int ok_open = snd_pcm_open( &pcm, pcmname, SND_PCM_STREAM_PLAYBACK, 0 );

	if( ok_open != 0 ) {
		printf( "nope: %s\n", snd_strerror( ok_open ) );
		return 1;
	}

	const int channels = sound.num_channels;
	const int sample_rate = sound.sample_rate;
	const int latency = milliseconds( 10 );

	const int ok_set_params = snd_pcm_set_params( pcm,
		SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_NONINTERLEAVED,
		channels, sample_rate, 1, latency );

	if( ok_set_params != 0 ) {
		printf( "nope: %s\n", snd_strerror( ok_open ) );
		return 1;
	}

	// snd_output_t *log;
	// if (snd_output_stdio_attach(&log, stderr, 0) >= 0) {
	// 	snd_pcm_dump(pcm, log); snd_output_close(log); }

	s16 samples[ 4096 ];
	AudioOutputBuffer buffer;
	buffer.sample_rate = sound.sample_rate;
	buffer.num_samples = array_count( samples );
	buffer.samples = samples;

	while( true ) {
		audio_fill_output_buffer( &buffer );

		for( u32 i = 0; i < buffer.num_samples; ) {
			// s16 * channels[ 2 ] = { sound.samples + i, sound.samples + sound.num_samples + i };
			s16 * channels[ 2 ] = { buffer.samples + i, buffer.samples + i };

			snd_pcm_sframes_t written = snd_pcm_writen( pcm, ( void ** ) channels, buffer.num_samples - i );
			if( written <= 0 ) {
				snd_pcm_recover( pcm, written, 1 );
			}
			else {
				i += written;
			}
		}
	}

	assert( snd_pcm_close( pcm ) == 0 );

	return 0;
}
