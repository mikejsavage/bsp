#include <stdarg.h>
#include <assert.h>
#include <alsa/asoundlib.h>
#include <math.h>

#include "intrinsics.h"
#include "wave.h"

#define FRAMES 1024

short buffer[ FRAMES ][ 2 ]; // stereo

int milliseconds( int ms ) {
	return ms * 1000;
}

void error_handler( const char * file, int line, const char * function, int err, const char * fmt, ... ) {
	printf( "error in %s at %s:%d\n", file, function, line );

	va_list args;
	va_start( args, fmt );
	vprintf( fmt, args );
	va_end( args );
	printf( "\n" );
}

u8 memory[ megabytes( 64 ) ];
int main( int argc, char ** argv ) {
	snd_lib_error_set_handler( error_handler );

	MemoryArena arena;
	memarena_init( &arena, memory, sizeof( memory ) );

	u8 * wave = file_get_contents( "01 - Abandon.wav" );
	Sound sound;
	bool ok = wave_decode( &arena, wave, &sound );
	assert( ok );

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

	// for( int i = 0; i < FRAMES; i++ ) {
	// 	buffer[i][0] = 16384*sin(2*M_PI*i/128) + 6553*sin(4*M_PI*i/128);
	// 	buffer[i][1] = 0;
	// }

	for( u32 i = 0; i < sound.num_samples; ) {
		s16 * channels[ 2 ] = { sound.samples + i, sound.samples + sound.num_samples + i };

		snd_pcm_sframes_t written = snd_pcm_writen( pcm, ( void ** ) channels, sound.num_samples - i );
		if( written <= 0 ) {
			snd_pcm_recover( pcm, written, 1 );
			printf( "recovering: %s\n", snd_strerror( written ) );
		}
		else {
			i += written;
		}
	}

	assert( snd_pcm_close( pcm ) == 0 );

	return 0;
}
