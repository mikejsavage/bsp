#ifndef _BSP_RENDERER_H_
#define _BSP_RENDERER_H_

#include <glm/glm.hpp>

#include "intrinsics.h"
#include "bsp.h"
#include "renderer.h"
#include "memory_arena.h"

struct BSPRenderer {
	MemoryArena * arena;
	const BSP * bsp;

	GLuint shader;
	GLuint * vaos;
	GLuint * vbos;
	GLuint * ebos;
	u32 * vertex_counts;
};

void bspr_init( BSPRenderer * const bspr, MemoryArena * const arena, const BSP * const bsp,
	const GLuint shader, const GLint at_position, const GLint at_colour );
void bspr_render( Renderer * const renderer, const BSPRenderer * const bspr, const glm::vec3 & pos );
void bspr_destroy( BSPRenderer * const bspr );

#endif // _BSP_RENDERER_H_
