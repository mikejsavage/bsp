#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "immediate.h"

void immediate_init( ImmediateContext * const ctx, ImmediateTriangle * const memory, const size_t max_triangles ) {
	ctx->triangles = memory;
	ctx->num_triangles = 0;
	ctx->max_triangles = max_triangles;
}

void immediate_triangle(
	ImmediateContext * const ctx,
	const glm::vec3 v1, const glm::vec3 v2, const glm::vec3 v3,
	const glm::vec4 colour
) {
	assert( ctx->num_triangles < ctx->max_triangles - 1 );

	const glm::vec2 uv;
	const ImmediateTriangle triangle = { {
		{ v1, colour, uv },
		{ v2, colour, uv },
		{ v3, colour, uv },
	} };

	ctx->triangles[ ctx->num_triangles++ ] = triangle;
}

void immediate_triangle(
	ImmediateContext * const ctx,
	const ImmediateVertex v1, const ImmediateVertex v2, const ImmediateVertex v3
) {
	assert( ctx->num_triangles < ctx->max_triangles - 1 );

	const ImmediateTriangle triangle = { { v1, v2, v3 } };

	ctx->triangles[ ctx->num_triangles++ ] = triangle;
}

void immediate_sphere(
	ImmediateContext * const ctx, const glm::vec3 centre, const float radius,
	const glm::vec4 colour, const u32 subdivisions
) {
	const float azimuth_max = 2.0f * M_PI;
	const float pitch_max = M_PI;

	for( u32 a = 0; a < subdivisions * 2; a++ ) {
		const float azimuth0 = azimuth_max * ( float ) a / ( subdivisions * 2 );
		const float azimuth1 = azimuth_max * ( float ) ( a + 1 ) / ( subdivisions * 2 );

		for( u32 p = 0; p < subdivisions; p++ ) {
			const float pitch0 = pitch_max * ( float ) p / subdivisions;
			const float pitch1 = pitch_max * ( float ) ( p + 1 ) / subdivisions;

			const glm::vec3 top_left = centre + glm::vec3(
				radius * cosf( azimuth0 ) * sinf( pitch0 ),
				radius * sinf( azimuth0 ) * sinf( pitch0 ),
				radius * cosf( pitch0 )
			);

			const glm::vec3 top_right = centre + glm::vec3(
				radius * cosf( azimuth1 ) * sinf( pitch0 ),
				radius * sinf( azimuth1 ) * sinf( pitch0 ),
				radius * cosf( pitch0 )
			);

			const glm::vec3 bottom_left = centre + glm::vec3(
				radius * cosf( azimuth0 ) * sinf( pitch1 ),
				radius * sinf( azimuth0 ) * sinf( pitch1 ),
				radius * cosf( pitch1 )
			);

			const glm::vec3 bottom_right = centre + glm::vec3(
				radius * cosf( azimuth1 ) * sinf( pitch1 ),
				radius * sinf( azimuth1 ) * sinf( pitch1 ),
				radius * cosf( pitch1 )
			);

			immediate_triangle( ctx, top_left, top_right, bottom_left, colour );
			immediate_triangle( ctx, bottom_left, top_right, bottom_right, colour );
		}
	}
}

void immediate_render(
	ImmediateContext * const ctx,
	const GLint at_position, const GLint at_colour,
	const bool textured, const GLint at_uv, const GLint un_texture
) {
	GLuint vao;
	glGenVertexArrays( 1, &vao );

	glBindVertexArray( vao );

	const u32 num_vbos = 3;
	GLuint vbos[ num_vbos ];
	glGenBuffers( num_vbos, vbos );

	glBindBuffer( GL_ARRAY_BUFFER, vbos[ 0 ] );
	glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, sizeof( ImmediateVertex ), 0 );

	glBindBuffer( GL_ARRAY_BUFFER, vbos[ 1 ] );
	glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_colour );
	glVertexAttribPointer( at_colour, 3, GL_FLOAT, GL_FALSE, sizeof( ImmediateVertex ), ( GLvoid * ) offsetof( ImmediateVertex, colour ) );

	if( textured ) {
		glBindBuffer( GL_ARRAY_BUFFER, vbos[ 2 ] );
		glBufferData( GL_ARRAY_BUFFER, ctx->num_triangles * sizeof( ImmediateTriangle ), ctx->triangles, GL_STATIC_DRAW );
		glEnableVertexAttribArray( at_uv );
		glVertexAttribPointer( at_uv, 3, GL_FLOAT, GL_FALSE, sizeof( ImmediateVertex ), ( GLvoid * ) offsetof( ImmediateVertex, uv ) );

		glUniform1i( un_texture, 0 );
	}

	glDrawArrays( GL_TRIANGLES, 0, ctx->num_triangles * 3 );

	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );

	glDeleteBuffers( num_vbos, vbos );
	glDeleteVertexArrays( 1, &vao );
}

void immediate_clear( ImmediateContext * const ctx ) {
	ctx->num_triangles = 0;
}
