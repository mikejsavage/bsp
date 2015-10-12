#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "intrinsics.h"
#include "memory_arena.h"
#include "btt.h"
#include "heightmap.h"
#include "gpubtt.h"

static u32 btt_count_leaves( const BTT * const btt ) {
	if( btt->left ) {
		return btt_count_leaves( btt->left ) + btt_count_leaves( btt->right );
	}

	return 1;
}

static void gpubtt_build(
	glm::vec3 * const verts, u32 * i,
	const OffsetHeightmap * ohm, const BTT * btt,
	const glm::ivec2 iv0, const glm::ivec2 iv1, const glm::ivec2 iv2
) {
	const glm::vec3 offset( ohm->x_offset, ohm->y_offset, 0.0f );

	const glm::vec3 v0( ohm->hm.point( iv0.x, iv0.y ) + offset );
	const glm::vec3 v1( ohm->hm.point( iv1.x, iv1.y ) + offset );
	const glm::vec3 v2( ohm->hm.point( iv2.x, iv2.y ) + offset );

	if( btt->left ) {
		const glm::ivec2 mid = ( iv0 + iv2 ) / 2;

		gpubtt_build( verts, i, ohm, btt->left, iv1, mid, iv0 );
		gpubtt_build( verts, i, ohm, btt->right, iv2, mid, iv1 );
	}
	else {
		verts[ *i + 0 ] = v0;
		verts[ *i + 1 ] = v1;
		verts[ *i + 2 ] = v2;
		*i += 3;
	}
}

void gpubtt_init(
	MemoryArena * const mem, GPUBTT * const gpubtt,
	const OffsetHeightmap * const ohm, const BTTs btts,
	const GLuint at_position
) {
	MEMARENA_SCOPED_CHECKPOINT( mem );

	const u32 num_leaves = btt_count_leaves( btts.left_root )
		+ btt_count_leaves( btts.right_root );

	glm::vec3 * const verts = memarena_push_many( mem, glm::vec3, num_leaves * 3 );
	u32 i = 0;

	gpubtt_build( verts, &i, ohm, btts.left_root, glm::ivec2( 0, 0 ), glm::ivec2( 0, ohm->hm.height - 1 ), glm::ivec2( ohm->hm.width - 1, ohm->hm.height - 1 ) );
	gpubtt_build( verts, &i, ohm, btts.right_root, glm::ivec2( ohm->hm.width - 1, ohm->hm.height - 1 ), glm::ivec2( ohm->hm.width - 1, 0 ), glm::ivec2( 0, 0 ) );

	glGenVertexArrays( 1, &gpubtt->vao );
	glBindVertexArray( gpubtt->vao );

	glGenBuffers( 1, &gpubtt->vbo_verts );
	glBindBuffer( GL_ARRAY_BUFFER, gpubtt->vbo_verts );
	glBufferData( GL_ARRAY_BUFFER, i * 3 * sizeof( GLfloat ), verts, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glm::vec3 * const normals = memarena_push_many( mem, glm::vec3, ohm->hm.width * ohm->hm.height );

	for( u32 y = 0; y < ohm->hm.height; y++ ) {
		for( u32 x = 0; x < ohm->hm.width; x++ ) {
			const glm::vec3 normal = ohm->hm.point_normal( x, y );
			normals[ y * ohm->hm.width + x ] = normal;
		}
	}

	glGenTextures( 1, &gpubtt->tex_normals );
	glBindTexture( GL_TEXTURE_2D, gpubtt->tex_normals );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, ohm->hm.width, ohm->hm.height, 0, GL_RGB, GL_FLOAT, normals );

	glBindVertexArray( 0 );

	gpubtt->num_verts = i;
}

void gpubtt_destroy( GPUBTT * const gpubtt ) {
	glDeleteTextures( 1, &gpubtt->tex_normals );
	glDeleteBuffers( 1, &gpubtt->vbo_verts );
	glDeleteVertexArrays( 1, &gpubtt->vao );
}

void gpubtt_render( const GPUBTT * const gpubtt, const GLuint un_normals ) {
	glBindVertexArray( gpubtt->vao );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, gpubtt->tex_normals );
	glUniform1i( un_normals, 0 );

	glDrawArrays( GL_TRIANGLES, 0, gpubtt->num_verts * 3 );

	glBindVertexArray( 0 );
}
