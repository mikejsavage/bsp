#include "intrinsics.h"
#include "memory_arena.h"
#include "wave.h"

// TODO: this only works on a little endian machine

#define WAVEID( a, b, c, d ) ( d << 24 ) | ( c << 16 ) | ( b << 8 ) | a

enum WaveID {
	WAVEID_RIFF = WAVEID( 'R', 'I', 'F', 'F' ),
	WAVEID_WAVE = WAVEID( 'W', 'A', 'V', 'E' ),
	WAVEID_fmt = WAVEID( 'f', 'm', 't', ' ' ),
	WAVEID_data = WAVEID( 'd', 'a', 't', 'a' ),
};

enum WaveFormat {
	WAVEFORMAT_PCM = 1,
};

struct WaveHeader {
	u32 id_riff;
	u32 length;
	u32 id_wave;
};

struct WaveChunk {
	u32 id;
	u32 length;
};

struct WaveFMT {
	u32 id;
	u32 length;

	u16 format;
	u16 num_channels;
	u32 sample_rate;
	u32 data_rate;
	u16 block_align;
	u16 bits_per_sample;
};

bool wave_decode( MemoryArena * arena, u8 * data, Sound * sound ) {
	WaveHeader * header = ( WaveHeader * ) data;

	if( header->id_riff != WAVEID_RIFF || header->id_wave != WAVEID_WAVE ) {
		printf( "bad header\n" );
		return false;
	}

	u32 pos = sizeof( WaveHeader );
	u32 data_length;

	while( pos < header->length ) {
		WaveChunk * chunk = ( WaveChunk * ) ( data + pos );

		switch( chunk->id ) {
			case WAVEID_fmt: {
				if( chunk->length != 16 ) return false;

				WaveFMT * fmt = ( WaveFMT * ) chunk;

				if(
					fmt->format != WAVEFORMAT_PCM
					|| ( fmt->num_channels != 1 && fmt->num_channels != 2 )
					|| fmt->bits_per_sample != 16
				) {
					return false;
				}

				sound->num_channels = fmt->num_channels;
				sound->sample_rate = fmt->sample_rate;

				break;
			}

			case WAVEID_data: {
				sound->samples = ( s16 * ) ( chunk + 1 );
				data_length = chunk->length;
				break;
			};

			default: {
				printf( "wtf is this\n" );
				return false;
			}
		}

		pos += sizeof( WaveChunk ) + align2( chunk->length );
	}

	sound->num_samples = data_length / ( sound->num_channels * sizeof( s16 ) );

	// uninterleave the samples
	if( sound->num_channels == 2 ) {
		MEMARENA_SCOPED_CHECKPOINT( arena );
		printf( "%u\n", sound->num_samples * 2 );
		s16 * scratch = memarena_push_many( arena, s16, sound->num_samples * 2 );

		for( u32 i = 0; i < sound->num_samples; i++ ) {
			scratch[ i ] = sound->samples[ i * 2 ];
			scratch[ i + sound->num_samples ] = sound->samples[ i * 2 + 1 ];
		}

		for( u32 i = 0; i < sound->num_samples * 2; i++ ) {
			sound->samples[ i ] = scratch[ i ];
		}
	}

	return true;
}
