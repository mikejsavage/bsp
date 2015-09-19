#ifndef _PLATFORM_THREAD_H_
#define _PLATFORM_THREAD_H_

#if defined(__linux__) || defined(__APPLE__)
#include "unix_thread.cc"
#endif

#endif // _PLATFORM_THREAD_H_
