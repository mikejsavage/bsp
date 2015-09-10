/*
 * I use the following naming conventions in this file::
 * TODO: no i dont
 *
 *   - tx/ty are used to denote tile coordinates
 *   - ptx/pty are used to denote the player's tile coordinates
 *   - x/y are used to denote local coordinates within a tile
 *   - wx/wy are used to denote global coordinates
 */

#include <string.h>

#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "intrinsics.h"
#include "heightmap.h"
#include "terrain_manager.h"

static const GLchar * const vert_src = GLSL(
	in vec3 position;
	in vec3 normal;
	in float lit;

	out vec3 n;
	out float depth;
	out float l;

	uniform mat4 vp;

	void main() {
		n = normal;
		l = lit;
		gl_Position = vp * vec4( position, 1.0 );
		depth = gl_Position.z;
	}
);

static const GLchar * frag_src = GLSL(
	in vec3 n;
	in float depth;
	in float l;

	out vec4 colour;

	uniform vec3 sun;

	void main() {
		vec3 ground;
		if( n.z > 0.9 ) {
			ground = vec3( 0.4, 1.0, 0.4 );
		}
		else {
			ground = vec3( 0.7, 0.7, 0.5 );
		}

		float d = max( 0, -dot( n, sun ) );
		float light = max( 0.2, l * d );

		vec3 fog = vec3( 0.6, 0.6, 0.6 );

		float t = smoothstep( 400, 600, depth );

		colour = vec4( ( 1.0 - t ) * ground * light + t * fog, 1.0 );
	}
);

static void terrain_load(
	TerrainManager * const tm, const u32 tx, const u32 ty,
	const u32 vx, const u32 vy
) {
	char path[ 256 ];
	sprintf( path, "%s/%d_%d.tga", tm->dir, tx, ty );

	Heightmap * const hm = &tm->tiles[ tx ][ ty ];

	if( hm->vbo_verts != 0 ) hm->unload();
	hm->load( path, tx * TILE_SIZE, ty * TILE_SIZE, tm->at_pos, tm->at_normal, tm->at_lit );
}

void terrain_init( TerrainManager * const tm, const char * const tiles_dir ) {
	assert( strlen( tiles_dir ) < array_count( tm->dir ) );
	strcpy( tm->dir, tiles_dir );

	char dims_path[ 256 ];
	sprintf( dims_path, "%s/dims.txt", tm->dir );

	FILE * dims = fopen( dims_path, "r" );
	fscanf( dims, "%d %d", &tm->width, &tm->height );
	fclose( dims );
	printf( "%d %d\n", tm->width, tm->height );

	tm->shader = compile_shader( vert_src, frag_src, "colour" );

	tm->at_pos = glGetAttribLocation( tm->shader, "position" );
	tm->at_normal = glGetAttribLocation( tm->shader, "normal" );
	tm->at_lit = glGetAttribLocation( tm->shader, "lit" );

	tm->un_vp = glGetUniformLocation( tm->shader, "vp" );
	tm->un_sun = glGetUniformLocation( tm->shader, "sun" );

	tm->first_teleport = true;
	tm->view_left = tm->view_top = 0;
}

void terrain_teleport( TerrainManager * const tm, const glm::vec3 position ) {
	const u32 new_tx = position.x / TILE_SIZE;
	const u32 new_ty = position.y / TILE_SIZE;

	// TODO: this is a cheap hack
	if( !tm->first_teleport ) {
		for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
			for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
				tm->tiles[ tm->last_tx + tx - VIEW_HALF ][ tm->last_ty + ty - VIEW_HALF ].unload();
			}
		}
	}

	tm->first_teleport = false;

	tm->last_tx = new_tx;
	tm->last_ty = new_ty;

	for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
		for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
			terrain_load( tm, new_tx + tx - VIEW_HALF, new_ty + ty - VIEW_HALF, tx, ty );
		}
	}
}

void terrain_update( TerrainManager * const tm, const glm::vec3 position ) {
	const u32 new_tx = position.x / TILE_SIZE;
	const u32 new_ty = position.y / TILE_SIZE;

	if( new_tx != tm->last_tx ) {
		if( new_tx > tm->last_tx ) {
			// +x boundary
			for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
				terrain_load( tm, tm->last_tx + VIEW_HALF + 1, tm->last_ty + ty - VIEW_HALF, tm->view_left, tm->view_top );
			}
			tm->last_tx++;
			tm->view_left = ( tm->view_left + 1 ) % VIEW_SIZE;
		}
		else {
			// -x boundary
			const u32 new_view_left = tm->view_left == 0 ? VIEW_SIZE - 1 : tm->view_left - 1;
			for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
				terrain_load( tm, tm->last_tx - VIEW_HALF - 1, tm->last_ty + ty - VIEW_HALF, new_view_left, tm->view_top );
			}
			tm->last_tx--;
			tm->view_left = new_view_left;
		}
	}

	if( new_ty != tm->last_ty ) {
		if( new_ty > tm->last_ty ) {
			// +y boundary
			for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
				terrain_load( tm, tm->last_tx + tx - VIEW_HALF, tm->last_ty + VIEW_HALF + 1, tm->view_left, tm->view_top );
			}
			tm->last_ty++;
			tm->view_top = ( tm->view_top + 1 ) % VIEW_SIZE;
		}
		else {
			// -y boundary
			const u32 new_view_top = tm->view_top == 0 ? VIEW_SIZE - 1 : tm->view_top - 1;
			for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
				terrain_load( tm, tm->last_tx + tx - VIEW_HALF, tm->last_ty - VIEW_HALF - 1, tm->view_left, new_view_top );
			}
			tm->last_ty--;
			tm->view_top = new_view_top;
		}
	}
}

void terrain_render( const TerrainManager * const tm, const glm::mat4 VP, const float sun_slope ) {
	const glm::vec3 sun = glm::normalize( glm::vec3( 1, 0, -sun_slope ) );

	glUseProgram( tm->shader );

	glUniformMatrix4fv( tm->un_vp, 1, GL_FALSE, glm::value_ptr( VP ) );
	glUniform3fv( tm->un_sun, 1, glm::value_ptr( sun ) );

	for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
		for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
			tm->tiles[ tm->last_tx + tx - VIEW_HALF ][ tm->last_ty + ty - VIEW_HALF ].render();
		}
	}

	glUseProgram( 0 );
}

float terrain_height( const TerrainManager * const tm, const float x, const float y ) {
	const u32 tx = x / TILE_SIZE;
	const u32 ty = y / TILE_SIZE;

	const Heightmap * const hm = &tm->tiles[ tx ][ ty ];

	return hm->height( x - tx * TILE_SIZE, y - ty * TILE_SIZE );
}
