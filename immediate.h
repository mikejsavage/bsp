#ifndef _IMMEDIATE_H_
#define _IMMEDIATE_H_

#include <glm/glm.hpp>

struct ImmediateTriangle {
	glm::vec3 v1;
	glm::vec3 v2;
	glm::vec3 v3;
};

struct ImmediateContext {
	ImmediateTriangle * triangles;
	size_t num_triangles;
	size_t max_triangles;
};

void immediate_init( ImmediateContext * const ctx, ImmediateTriangle * const memory, const size_t max_triangles );
void immediate_triangle( ImmediateContext * const ctx, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3 );
void immediate_render( ImmediateContext * const ctx, const GLuint at_position );
void immediate_clear( ImmediateContext * const ctx );

#endif // _IMMEDIATE_H_
