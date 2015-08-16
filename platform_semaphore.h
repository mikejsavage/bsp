// TODO: platform_concurrency.h?
#ifndef _PLATFORM_SEMAPHORE_H_
#define _PLATFORM_SEMAPHORE_H_

#ifdef __linux__
#include "linux_semaphore.cc"
#endif

#ifdef __APPLE__
#include "darwin_semaphore.cc"
#endif

#endif // _PLATFORM_SEMAPHORE_H_
