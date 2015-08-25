#ifndef _BSP_RENDERER_H_
#define _BSP_RENDERER_H_

#include <glm/glm.hpp>

#include "bsp.h"
#include "memory_arena.h"

struct BSPRenderer {
	MemoryArena * arena;
	const BSP * bsp;

	GLuint * vaos;
	GLuint * vbos;
	GLuint * ebos;
	u32 * vertex_counts;
};

void bspr_init( BSPRenderer * const bspr, MemoryArena * const arena, const BSP * const bsp );
void bspr_render( const BSPRenderer * const bspr, const glm::vec3 & pos, const GLint at_position, const GLint at_colour );
void bspr_destroy( BSPRenderer * const bspr );

#endif // _BSP_RENDERER_H_
