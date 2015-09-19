#ifndef _PLATFORM_ATOMIC_H_
#define _PLATFORM_ATOMIC_H_

#if defined(__linux__) || defined(__APPLE__)
#include "unix_atomic.cc"
#endif

#endif // _PLATFORM_ATOMIC_H_
