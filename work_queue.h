#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#include "int.h"

struct WorkQueue;

#define WORK_QUEUE_CALLBACK( name ) void name( void * const data )
typedef WORK_QUEUE_CALLBACK( WorkQueueCallback );

#endif // _WORK_QUEUE_H_
