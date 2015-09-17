#ifndef _HEIGHTMAP_H_
#define _HEIGHTMAP_H_

#include <string>

#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "memory_arena.h"

class Heightmap {
public:
	u8 * pixels;
	u32 width, height;

	GLuint vbo_verts = 0;
	GLuint vbo_normals;
	GLuint vbo_lit;
	GLuint ebo;
	GLuint vao;

	glm::vec3 point( u32 x, u32 y ) const;
	glm::vec3 point_normal( u32 x, u32 y ) const;

	float bilerp_height( const float x, const float y ) const;

	void render() const;
};

struct OffsetHeightmap {
	Heightmap hm;
	float x_offset, y_offset;
};

void heightmap_init( Heightmap * const hm, MemoryArena * const mem,
	u8 * const pixels, const u32 width, const u32 height,
	const float ox, const float oy, // TODO: take rendering out of Heightmap
	const GLint at_pos, const GLint at_normal, const GLint at_lit );

void heightmap_destroy( Heightmap * const hm );

#endif // _HEIGHTMAP_H_
