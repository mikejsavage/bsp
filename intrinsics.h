#ifndef _INTRINSICS_H_
#define _INTRINSICS_H_

#include <stdint.h>
#include "platform_backtrace.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define array_len( x ) ( sizeof( x ) / sizeof( ( x )[ 0 ] ) )

#ifdef assert
#undef assert
#endif
#define assert( predicate ) { if( !( predicate ) ) { print_backtrace(); __builtin_trap(); } }

#define is_power_of_2( n ) ( ( ( n ) & ( ( n ) - 1 ) ) == 0 )
#define align_power_of_2( n, alignment ) ( ( ( n ) + ( alignment ) - 1 ) & ~( ( alignment ) - 1 ) )
#define align4( n ) align_power_of_2( n, 4 )
#define align8( n ) align_power_of_2( n, 8 )
#define align16( n ) align_power_of_2( n, 16 )

inline size_t kilobytes( const size_t kb ) {
	return kb * 1024;
}

inline size_t megabytes( const size_t mb ) {
	return kilobytes( mb ) * 1024;
}

inline size_t gigabytes( const size_t gb ) {
	return megabytes( gb ) * 1024;
}

#endif // _INTRINSICS_H_
