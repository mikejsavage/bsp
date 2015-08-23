#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "immediate.h"

void immediate_init( ImmediateContext * const ctx, ImmediateTriangle * const memory, const size_t max_triangles ) {
	ctx->triangles = memory;
	ctx->num_triangles = 0;
	ctx->max_triangles = max_triangles;
}

void immediate_triangle( ImmediateContext * const ctx,
	const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3,
	const glm::vec4 colour
) {
	assert( ctx->num_triangles < ctx->max_triangles - 1 );

	const ImmediateTriangle triangle = { {
		{ v1, colour },
		{ v2, colour },
		{ v3, colour },
	} };

	ctx->triangles[ ctx->num_triangles++ ] = triangle;
}

void immediate_render( ImmediateContext * const ctx, const GLuint at_position, const GLuint at_colour ) {
	GLuint vao;
	glGenVertexArrays( 1, &vao );

	glBindVertexArray( vao );

	GLuint vbos[ 2 ];
	glGenBuffers( 2, vbos );

	glBindBuffer( GL_ARRAY_BUFFER, vbos[ 0 ] );
	glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, sizeof( ImmediateVertex ), 0 );

	glBindBuffer( GL_ARRAY_BUFFER, vbos[ 1 ] );
	glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_colour );
	glVertexAttribPointer( at_colour, 3, GL_FLOAT, GL_FALSE, sizeof( ImmediateVertex ), ( GLvoid * ) sizeof( glm::vec3 ) );

	glDrawArrays( GL_TRIANGLES, 0, ctx->num_triangles * 3 );

	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	glDeleteBuffers( 2, vbos );
	glDeleteVertexArrays( 1, &vao );
}

void immediate_clear( ImmediateContext * const ctx ) {
	ctx->num_triangles = 0;
}
