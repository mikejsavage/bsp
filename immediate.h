#ifndef _IMMEDIATE_H_
#define _IMMEDIATE_H_

#include <glm/glm.hpp>

struct ImmediateVertex {
	glm::vec3 pos;
	glm::vec3 colour;
};

struct ImmediateTriangle {
	ImmediateVertex vertices[ 3 ];
};

struct ImmediateContext {
	ImmediateTriangle * triangles;
	size_t num_triangles;
	size_t max_triangles;
};

void immediate_init( ImmediateContext * const ctx, ImmediateTriangle * const memory, const size_t max_triangles );
void immediate_triangle( ImmediateContext * const ctx, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3, const glm::vec3 colour );
void immediate_render( ImmediateContext * const ctx, const GLuint at_position, const GLuint at_colour );
void immediate_clear( ImmediateContext * const ctx );

#endif // _IMMEDIATE_H_
