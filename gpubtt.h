#ifndef _GPUBTT_H_
#define _GPUBTT_H_

#include "platform_opengl.h"

#include "heightmap.h"
#include "btt.h"

struct GPUBTT {
	GLuint vao;
	GLuint vbo_verts;
	u32 num_verts;
};

void gpubtt_init( MemoryArena * const mem, GPUBTT * const gpubtt,
	const OffsetHeightmap * const ohm, const BTTs btts,
	const GLuint at_position );

void gpubtt_destroy( GPUBTT * const gpubtt );

void gpubtt_render( const GPUBTT * const gpubtt );

#endif // _GPUBTT_H_
