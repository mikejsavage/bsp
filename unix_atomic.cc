#include "intrinsics.h"

inline u8 atomic_add_u8( volatile u8 * dest, u8 i ) {
	return __sync_add_and_fetch( dest, i );
}

inline u16 atomic_add_u16( volatile u16 * dest, u16 i ) {
	return __sync_add_and_fetch( dest, i );
}

inline u32 atomic_add_u32( volatile u32 * dest, u32 i ) {
	return __sync_add_and_fetch( dest, i );
}

inline u64 atomic_add_u64( volatile u64 * dest, u64 i ) {
	return __sync_add_and_fetch( dest, i );
}

inline bool atomic_cas_u8( volatile u8 * dest, u8 oldval, u8 newval ) {
	return __sync_bool_compare_and_swap( dest, oldval, newval );
}

inline bool atomic_cas_u16( volatile u16 * dest, u16 oldval, u16 newval ) {
	return __sync_bool_compare_and_swap( dest, oldval, newval );
}

inline bool atomic_cas_u32( volatile u32 * dest, u32 oldval, u32 newval ) {
	return __sync_bool_compare_and_swap( dest, oldval, newval );
}

inline bool atomic_cas_u64( volatile u64 * dest, u64 oldval, u64 newval ) {
	return __sync_bool_compare_and_swap( dest, oldval, newval );
}

inline bool atomic_cas_pointer( volatile void ** dest, void * oldval, void * newval ) {
	return __sync_bool_compare_and_swap( dest, oldval, newval );
}
