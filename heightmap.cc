#include <stdio.h>
#include <err.h>

#include <string>

#include "platform_opengl.h"
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "heightmap.h"
#include "memory_arena.h"
#include "stb_image.h"
#include "stb_perlin.h"

const float SLOPE = 0.3;

static float lerp( const float a, const float b, const float t ) {
	return a * ( 1 - t ) + b * t;
}

static float bilinear_interpolation(
	const glm::vec3 & v1,
	const glm::vec3 & v2,
	const glm::vec3 & v3,
	const glm::vec3 & v4,
	const glm::vec2 & p
) {
	const float tx = ( p.x - v1.x ) / ( v2.x - v1.x );

	const float mx1 = lerp( v1.z, v2.z, tx );
	const float mx2 = lerp( v3.z, v4.z, tx );

	const float ty = ( p.y - v1.y ) / ( v3.y - v1.y );

	return lerp( mx1, mx2, ty );
}

// CCW winding points towards the camera
static glm::vec3 triangle_normal_ccw( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c ) {
	return glm::normalize( glm::cross( b - a, c - a ) );
}

static glm::vec3 triangle_perp_ccw( const glm::vec3 & a, const glm::vec3 & b, const glm::vec3 & c ) {
	return glm::cross( b - a, c - a );
}

void heightmap_init(
	Heightmap * const hm, MemoryArena * const mem,
	u8 * const pixels, const u32 width, const u32 height,
	const float ox, const float oy,
	const GLint at_pos, const GLint at_normal, const GLint at_lit
) {
	hm->pixels = pixels;
	hm->width = width;
	hm->height = height;

	MemoryArenaCheckpoint cp = memarena_checkpoint( mem );

	GLfloat * const vertices = memarena_push_many( mem, GLfloat, width * height * 3 );
	GLfloat * const normals = memarena_push_many( mem, GLfloat, width * height * 3 );
	GLuint * const indices = memarena_push_many( mem, GLuint, width * height * 6 );
	GLfloat * const lit = memarena_push_many( mem, GLfloat, width * height );

	for( u32 i = 0; i < width * height; i++ ) {
		lit[ i ] = 0;
	}

	for( u32 y = 0; y < height; y++ ) {
		lit[ y * width ] = -1;

		for( u32 x = 1; x < width; x++ ) {
			const float h = hm->point( x, y ).z;
			const float al = lit[ y * width + x - 1 ];
			float dh;

			if( al == -1 ) {
				dh = hm->point( x - 1, y ).z - ( h + SLOPE );
			}
			else {
				dh = al - ( h + SLOPE );
			}

			if( dh > 0 ) {
				lit[ y * width + x ] = h + dh;
			}
			else {
				lit[ y * width + x ] = -1;
			}
		}
	}

	for( u32 y = 0; y < height; y++ ) {
		for( u32 x = 0; x < width; x++ ) {
			const u32 i = y * width + x;

			lit[ i ] = lit[ i ] == -1 ? 1 : 0;
		}
	}

	for( u32 y = 0; y < height; y++ ) {
		for( u32 x = 0; x < width; x++ ) {
			const u32 base = 3 * ( y * width + x );
			const float height = hm->point( x, y ).z;

			vertices[ base ] = x + ox;
			vertices[ base + 1 ] = y + oy;
			vertices[ base + 2 ] = height;

			const glm::vec3 normal = hm->point_normal( x, y );

			normals[ base ] = normal.x;
			normals[ base + 1 ] = normal.y;
			normals[ base + 2 ] = normal.z;
		}
	}

	for( u32 y = 0; y < height - 1; y++ ) {
		for( u32 x = 0; x < width - 1; x++ ) {
			const u32 base = 6 * ( y * width + x );

			indices[ base + 0 ] = ( y + 0 ) * width + ( x + 0 );
			indices[ base + 1 ] = ( y + 1 ) * width + ( x + 0 );
			indices[ base + 2 ] = ( y + 0 ) * width + ( x + 1 );
			indices[ base + 3 ] = ( y + 1 ) * width + ( x + 1 );
			indices[ base + 4 ] = ( y + 0 ) * width + ( x + 1 );
			indices[ base + 5 ] = ( y + 1 ) * width + ( x + 0 );
		}
	}

	glGenVertexArrays( 1, &hm->vao );
	glBindVertexArray( hm->vao );

	glGenBuffers( 1, &hm->vbo_verts );
	glBindBuffer( GL_ARRAY_BUFFER, hm->vbo_verts );
	glBufferData( GL_ARRAY_BUFFER, width * height * sizeof( GLfloat ) * 3, vertices, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_pos );
	glVertexAttribPointer( at_pos, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glGenBuffers( 1, &hm->vbo_normals );
	glBindBuffer( GL_ARRAY_BUFFER, hm->vbo_normals );
	glBufferData( GL_ARRAY_BUFFER, width * height * sizeof( GLfloat ) * 3, normals, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_normal );
	glVertexAttribPointer( at_normal, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glGenBuffers( 1, &hm->vbo_lit );
	glBindBuffer( GL_ARRAY_BUFFER, hm->vbo_lit );
	glBufferData( GL_ARRAY_BUFFER, width * height * sizeof( GLfloat ), lit, GL_STATIC_DRAW );
	glEnableVertexAttribArray( at_lit );
	glVertexAttribPointer( at_lit, 1, GL_FLOAT, GL_FALSE, 0, 0 );

	glGenBuffers( 1, &hm->ebo );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, hm->ebo );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, width * height * sizeof( GLuint ) * 6, indices, GL_STATIC_DRAW );

	glBindVertexArray( 0 );

	memarena_restore( mem, &cp );
}

void heightmap_destroy( Heightmap * const hm ) {
	if( hm->vbo_verts != 0 ) {
		glDeleteBuffers( 1, &hm->vbo_verts );
		glDeleteBuffers( 1, &hm->vbo_normals );
		glDeleteBuffers( 1, &hm->vbo_lit );
		glDeleteBuffers( 1, &hm->ebo );
		glDeleteVertexArrays( 1, &hm->vao );

		stbi_image_free( hm->pixels );

		hm->vbo_verts = 0;
	}
}

glm::vec3 Heightmap::point( u32 x, u32 y ) const {
	return glm::vec3( x, y, pixels[ y * width + x ] / 4.0f );
}

glm::vec3 Heightmap::point_normal( u32 x, u32 y ) const {
	glm::vec3 bent_normal = glm::vec3( 0 );

	/*
	 * A vertex looks like this:
	 *
	 *      +y
	 *   \ 6 |
	 *    \  |
	 * 5   \ |  1
	 *      \|
	 * ------X------ +x
	 *       |\
	 *    2  | \   4
	 *       |  \
	 *       | 3 \
	 *
	 * We consider up to six triangles for the bent normal
	 */

	if( x > 0 ) {
		if( y > 0 ) { // bottom left
			const glm::vec3 tri2a = point( x, y );
			const glm::vec3 tri2b = point( x - 1, y );
			const glm::vec3 tri2c = point( x, y - 1 );

			bent_normal += triangle_normal_ccw( tri2a, tri2b, tri2c );
		}
		if( y < height - 1 ) { // top left
			const glm::vec3 tri5a = point( x, y );
			const glm::vec3 tri5b = point( x - 1, y + 1 );
			const glm::vec3 tri5c = point( x - 1, y );

			const glm::vec3 tri6a = point( x, y );
			const glm::vec3 tri6b = point( x, y + 1 );
			const glm::vec3 tri6c = point( x - 1, y + 1 );

			bent_normal += triangle_normal_ccw( tri5a, tri5b, tri5c );
			bent_normal += triangle_normal_ccw( tri6a, tri6b, tri6c );
		}
	}

	if( x < width - 1 ) {
		if( y > 0 ) { // bottom right
			const glm::vec3 tri3a = point( x, y );
			const glm::vec3 tri3b = point( x, y - 1 );
			const glm::vec3 tri3c = point( x + 1, y - 1 );

			const glm::vec3 tri4a = point( x, y );
			const glm::vec3 tri4b = point( x + 1, y - 1 );
			const glm::vec3 tri4c = point( x + 1, y );

			bent_normal += triangle_normal_ccw( tri3a, tri3b, tri3c );
			bent_normal += triangle_normal_ccw( tri4a, tri4b, tri4c );
		}
		if( y < height - 1 ) { // top right
			const glm::vec3 tri1a = point( x, y );
			const glm::vec3 tri1b = point( x + 1, y );
			const glm::vec3 tri1c = point( x, y + 1 );

			bent_normal += triangle_normal_ccw( tri1a, tri1b, tri1c );
		}
	}

	return glm::normalize( bent_normal );
}

float Heightmap::bilerp_height( const float x, const float y ) const {
	const float ix = floorf( x );
	const float iy = floorf( y );

	return bilinear_interpolation(
		point( ix, iy ),
		point( ix + 1, iy ),
		point( ix, iy + 1 ),
		point( ix + 1, iy + 1 ),
		glm::vec2( x, y )
	);
}

void Heightmap::render() const {
	assert( vbo_verts != 0 );

	glBindVertexArray( vao );
	glDrawElements( GL_TRIANGLES, width * height * 6, GL_UNSIGNED_INT, 0 );
	glBindVertexArray( 0 );
}
