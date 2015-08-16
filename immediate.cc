#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "immediate.h"

void immediate_init( ImmediateContext * const ctx, ImmediateTriangle * const memory, const size_t max_triangles ) {
	ctx->triangles = memory;
	ctx->num_triangles = 0;
	ctx->max_triangles = max_triangles;
}

void immediate_triangle( ImmediateContext * const ctx, const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3 ) {
	assert( ctx->num_triangles < ctx->max_triangles - 1 );

	ctx->triangles[ ctx->num_triangles++ ] = { v1, v2, v3 };
}

void immediate_render( ImmediateContext * const ctx, const GLuint at_position ) {
	GLuint vao;
	glGenVertexArrays( 1, &vao );
	glBindVertexArray( vao );

	GLuint vbo;
	glGenBuffers( 1, &vbo );

	glBindBuffer( GL_ARRAY_BUFFER, vbo );
	glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );

	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glDrawArrays( GL_TRIANGLES, 0, ctx->num_triangles * 3 );

	glBindVertexArray( 0 );

	glDeleteBuffers( 1, &vbo );
	glDeleteVertexArrays( 1, &vao );

	printf( "drew %zu triangles vao %u vbo %u\n", ctx->num_triangles, vbo, vbo );
}

void immediate_clear( ImmediateContext * const ctx ) {
	ctx->num_triangles = 0;
}
