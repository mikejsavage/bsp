#ifndef _IMMEDIATE_H_
#define _IMMEDIATE_H_

#include <glm/glm.hpp>

struct ImmediateVertex {
	glm::vec3 pos;
	glm::vec4 colour;
	glm::vec2 uv;
};

struct ImmediateTriangle {
	ImmediateVertex vertices[ 3 ];
};

struct ImmediateContext {
	ImmediateTriangle * triangles;
	size_t num_triangles;
	size_t max_triangles;
};

void immediate_init( ImmediateContext * const ctx,
	ImmediateTriangle * const memory, const size_t max_triangles );

void immediate_triangle( ImmediateContext * const ctx,
	const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3, const glm::vec4 colour );
void immediate_triangle( ImmediateContext * const ctx,
	const ImmediateVertex v1, const ImmediateVertex v2, const ImmediateVertex v3 );

void immediate_render( const ImmediateContext * const ctx,
	const GLint at_position, const GLint at_colour,
	const bool textured = false, const GLint at_uv = 0, const GLint un_texture = 0 );

void immediate_sphere( ImmediateContext * const ctx,
	const glm::vec3 centre, const float radius, const glm::vec4 colour,
	const u32 subdivisions = 16 );

void immediate_clear( ImmediateContext * const ctx );

#endif // _IMMEDIATE_H_
