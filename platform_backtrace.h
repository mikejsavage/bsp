#ifndef _PLATFORM_BACKTRACE_H_
#define _PLATFORM_BACKTRACE_H_

void print_backtrace();

#if defined(__linux__) || defined(__APPLE__)
#include "linuxdarwin_backtrace.cc"
#endif

#endif // _PLATFORM_BACKTRACE_H_
