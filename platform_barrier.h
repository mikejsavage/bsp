#ifndef _PLATFORM_BARRIER_H_
#define _PLATFORM_BARRIER_H_

#define read_barrier() asm volatile ( "" ::: "memory" )
#define write_barrier() asm volatile ( "" ::: "memory" )

#endif // _PLATFORM_BARRIER_H_
