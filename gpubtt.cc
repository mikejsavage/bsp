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

static float angle( glm::vec3 a, glm::vec3 b ) {
	const glm::vec3 d = b - a;

	const float dist_xy = sqrtf( d.x * d.x + d.y * d.y );

	return d.z / dist_xy;
}

void compute_horizons(
	MemoryArena * arena,
	const Heightmap * hm, float * horizons,
	glm::ivec2 start, glm::ivec2 step
) {
	MEMARENA_SCOPED_CHECKPOINT( arena );

	const u32 max_hull_size = 2 * max_u32( hm->width, hm->height );

	glm::vec3 * hull = memarena_push_many( arena, glm::vec3, max_hull_size );
	u32 hull_size = 1;

	hull[ 0 ] = hm->point( start.x, start.y );
	horizons[ 0 ] = 0;

	start += step;

	while( ( u32 ) start.x < hm->width && ( u32 ) start.y < hm->height && start.x >= 0 && start.y >= 0 ) {
		const glm::vec3 p = hm->point( start.x, start.y );

		while( hull_size > 1 && angle( p, hull[ hull_size - 1 ] ) < angle( hull[ hull_size - 1 ], hull[ hull_size - 2 ] ) ) {
			hull_size--;
		}

		const float horizon = max_f( 0.0, angle( p, hull[ hull_size - 1 ] ) );
		horizons[ start.y * hm->width + start.x ] = horizon;

		hull[ hull_size ] = p;
		hull_size++;

		start += step;
	}
}

void gpubtt_init(
	MemoryArena * const arena, GPUBTT * const gpubtt,
	const OffsetHeightmap * const ohm, const BTTs btts,
	const GLuint at_position
) {
	MEMARENA_SCOPED_CHECKPOINT( arena );

	const u32 num_leaves = btt_count_leaves( btts.left_root )
		+ btt_count_leaves( btts.right_root );

	glm::vec3 * const verts = memarena_push_many( arena, glm::vec3, num_leaves * 3 );
	u32 i = 0;

	// bottom left, bottom right, top left, top right
	const glm::ivec2 bl( 0, 0 );
	const glm::ivec2 br( ohm->hm.width - 1, 0 );
	const glm::ivec2 tl( 0, ohm->hm.height - 1 );
	const glm::ivec2 tr( ohm->hm.width - 1, ohm->hm.height - 1 );

	gpubtt_build( verts, &i, ohm, btts.left_root, bl, tl, tr );
	gpubtt_build( verts, &i, ohm, btts.right_root, tr, br, bl );

	glGenVertexArrays( 1, &gpubtt->vao );
	glBindVertexArray( gpubtt->vao );

	glGenBuffers( 1, &gpubtt->vbo_verts );
	glBindBuffer( GL_ARRAY_BUFFER, gpubtt->vbo_verts );
	glBufferData( GL_ARRAY_BUFFER, i * 3 * sizeof( GLfloat ), verts, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glm::vec3 * const normals = memarena_push_many( arena, glm::vec3, ohm->hm.width * ohm->hm.height );

	for( u32 y = 0; y < ohm->hm.height; y++ ) {
		for( u32 x = 0; x < ohm->hm.width; x++ ) {
			const glm::vec3 normal = ohm->hm.point_normal( x, y );
			normals[ y * ohm->hm.width + x ] = normal;
		}
	}

	glGenTextures( 1, &gpubtt->tex_normals );
	glBindTexture( GL_TEXTURE_2D, gpubtt->tex_normals );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB32F, ohm->hm.width, ohm->hm.height, 0, GL_RGB, GL_FLOAT, normals );

	float * const horizons = memarena_push_many( arena, float, ohm->hm.width * ohm->hm.height );

	for( u32 i = 0; i < ohm->hm.height; i++ ) {
		compute_horizons( arena, &ohm->hm, horizons, glm::ivec2( 0, i ), glm::ivec2( 1, 0 ) );
	}

	glGenTextures( 1, &gpubtt->tex_horizons );
	glBindTexture( GL_TEXTURE_2D, gpubtt->tex_horizons );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R32F, ohm->hm.width, ohm->hm.height, 0, GL_RED, GL_FLOAT, horizons );

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
