#include <unistd.h>
#include <execinfo.h>

#include "intrinsics.h"
#include "platform_backtrace.h"

inline void print_backtrace() {
	void * stack[ 128 ];
	const int stack_size = backtrace( stack, array_count( stack ) );
	backtrace_symbols_fd( stack, stack_size, STDERR_FILENO );
}
