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

	const glm::vec3 v0( ohm->hm.point( v0.x, v0.y ) + offset );
	const glm::vec3 v1( ohm->hm.point( v1.x, v1.y ) + offset );
	const glm::vec3 v2( ohm->hm.point( v2.x, v2.y ) + offset );

	if( btt->left ) {
		const glm::ivec2 mid = ( iv0 + iv2 ) / 2;

		gpubtt_build( verts, i, ohm, btt->left, iv1, mid, iv0 );
		gpubtt_build( verts, i, ohm, btt->right, iv2, mid, iv1 );
	}
	else {
		verts[ *i++ ] = v0;
		verts[ *i++ ] = v1;
		verts[ *i++ ] = v2;
	}
}

void gpubtt_init(
	MemoryArena * const mem, GPUBTT * const gpubtt,
	const OffsetHeightmap * const ohm, const BTTs btts,
	const GLuint at_position
) {
	MemoryArenaCheckpoint cp = memarena_checkpoint( mem );

	const u32 num_leaves = btt_count_leaves( btts.left_root )
		+ btt_count_leaves( btts.right_root );

	glm::vec3 * const verts = memarena_push_many( mem, glm::vec3, num_leaves * 3 );
	u32 i = 0;

	gpubtt_build( verts, &i, ohm, btts.left_root, glm::ivec2( 0, 0 ), glm::ivec2( 0, ohm->hm.h - 1 ), glm::ivec2( ohm->hm.w - 1, ohm->hm.h - 1 ) );
	gpubtt_build( verts, &i, ohm, btts.right_root, glm::ivec2( ohm->hm.w - 1, ohm->hm.h - 1 ), glm::ivec2( ohm->hm.w - 1, 0 ), glm::ivec2( 0, 0 ) );

	glGenVertexArrays( 1, &gpubtt->vao );
	glBindVertexArray( gpubtt->vao );

	glGenBuffers( 1, &gpubtt->vbo_verts );
	glBindBuffer( GL_ARRAY_BUFFER, gpubtt->vbo_verts );
	glBufferData( GL_ARRAY_BUFFER, num_leaves * sizeof( GLfloat ) * 3, verts, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glBindVertexArray( 0 );

	gpubtt->num_verts = i;

	memarena_restore( mem, &cp );
}

void gpubtt_destroy( GPUBTT * const gpubtt ) {
	glDeleteBuffers( 1, &gpubtt->vbo_verts );
	glDeleteVertexArrays( 1, &gpubtt->vao );
}

void gpubtt_render( const GPUBTT * const gpubtt ) {
	glBindVertexArray( gpubtt->vao );
	glDrawElements( GL_TRIANGLES, gpubtt->num_verts, GL_UNSIGNED_INT, 0 );
	glBindVertexArray( 0 );
}
