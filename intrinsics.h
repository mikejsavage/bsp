#ifndef _INTRINSICS_H_
#define _INTRINSICS_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define array_count( x ) ( sizeof( x ) / sizeof( ( x )[ 0 ] ) )

#define is_power_of_2( n ) ( ( ( n ) & ( ( n ) - 1 ) ) == 0 )

#define align_power_of_2( n, alignment ) ( ( ( n ) + ( alignment ) - 1 ) & ~( ( alignment ) - 1 ) )
#define align4( n ) align_power_of_2( n, 4 )
#define align8( n ) align_power_of_2( n, 8 )
#define align16( n ) align_power_of_2( n, 16 )

// TODO: clashes with some crap in std::string
#define align_TODO( n, alignment ) ( ( ( n ) + ( alignment ) - 1 ) / ( alignment ) * ( alignment ) )

inline size_t kilobytes( const size_t kb ) {
	return kb * 1024;
}

inline size_t megabytes( const size_t mb ) {
	return kilobytes( mb ) * 1024;
}

inline size_t gigabytes( const size_t gb ) {
	return megabytes( gb ) * 1024;
}

inline u32 max_u32( u32 a, u32 b ) {
	return a > b ? a : b;
}

#include "platform_backtrace.h"

#ifdef assert
#undef assert
#endif

inline void mike_assert( const bool predicate, const char * const message ) {
	if( !( predicate ) ) {
		puts( message );
		print_backtrace();
		__builtin_trap();
	}
}

#define STRINGIFY_HELPER( x ) #x
#define STRINGIFY( x ) STRINGIFY_HELPER( x )
#define assert( predicate ) mike_assert( predicate, "\e[1;31massertion failed at " __FILE__ " line " STRINGIFY( __LINE__ ) ": \e[0;1m" #predicate "\e[0m" )

// TODO: this sucks
inline u8 * file_get_contents( const char * const path, size_t * const out_len = nullptr ) {
	FILE * const file = fopen( path, "rb" );
	assert( file );

	fseek( file, 0, SEEK_END );
	size_t len = ftell( file );
	fseek( file, 0, SEEK_SET );

	// TODO: oh-no-verflow
	u8 * const contents = ( u8 * const ) malloc( len + 1 );
	size_t bytes_read = fread( contents, 1, len, file );
	contents[ len ] = '\0';
	assert( bytes_read == len );

	if( out_len ) {
		*out_len = len;
	}

	fclose( file );

	return contents;
}

#endif // _INTRINSICS_H_
