#include <string.h>

#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "intrinsics.h"
#include "heightmap.h"
#include "terrain_manager.h"
#include "memory_arena.h"
#include "work_queue.h"
#include "stb_image.h"

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

static void terrain_load_tile(
	TerrainManager * const tm, const u32 tx, const u32 ty,
	const u32 vx, const u32 vy
) {
	char path[ 256 ];
	sprintf( path, "%s/%d_%d.tga", tm->dir, tx, ty );

	Heightmap * const hm = &tm->tiles[ vx ][ vy ];

	if( hm->vbo_verts != 0 ) heightmap_destroy( hm );

	size_t len;
	u8 * contents = file_get_contents( path, &len );
	assert( contents );

	int width, height;
	u8 * pixels = stbi_load_from_memory( contents, len, &width, &height, nullptr, 1 );
	free( contents );

	if( !pixels ) err( 1, "stbi_load failed (%s)", stbi_failure_reason() );

	heightmap_init( hm, tm->mem, pixels, width, height,
		tx * TILE_SIZE, ty * TILE_SIZE,
		tm->at_pos, tm->at_normal, tm->at_lit );
}

void terrain_init(
	TerrainManager * const tm, const char * const tiles_dir,
	MemoryArena * const mem
) {
	assert( strlen( tiles_dir ) < array_count( tm->dir ) );
	strcpy( tm->dir, tiles_dir );

	tm->mem = mem;

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
	if( !tm->first_teleport ) {
		for( u32 vy = 0; vy < VIEW_SIZE; vy++ ) {
			for( u32 vx = 0; vx < VIEW_SIZE; vx++ ) {
				heightmap_destroy( &tm->tiles[ vx ][ vy ] );
			}
		}
	}

	tm->first_teleport = false;

	const u32 player_tile_x = position.x / TILE_SIZE;
	const u32 player_tile_y = position.y / TILE_SIZE;

	tm->tile_x = player_tile_x;
	tm->tile_y = player_tile_y;

	for( u32 vy = 0; vy < VIEW_SIZE; vy++ ) {
		for( u32 vx = 0; vx < VIEW_SIZE; vx++ ) {
			terrain_load_tile( tm,
				vx + player_tile_x - VIEW_HALF, vy + player_tile_y - VIEW_HALF,
				vx, vy );
		}
	}
}

static u32 view_sub( u32 v, u32 o ) {
	if( v >= o ) return v - o;
	return VIEW_SIZE - ( o - v );
}

static u32 view_add( u32 v, u32 o ) {
	return ( v + o ) % VIEW_SIZE;
}

void terrain_update( TerrainManager * const tm, const glm::vec3 position ) {
	const u32 player_tile_x = position.x / TILE_SIZE;
	const u32 player_tile_y = position.y / TILE_SIZE;

	if( player_tile_x != tm->tile_x ) {
		if( player_tile_x > tm->tile_x ) {
			// +x boundary
			for( u32 vy = 0; vy < VIEW_SIZE; vy++ ) {
				terrain_load_tile( tm,
					tm->tile_x + VIEW_HALF + 1,
					tm->tile_y + vy - VIEW_HALF,
					view_add( tm->view_left, VIEW_SIZE ),
					view_add( tm->view_top, vy ) );
			}
			tm->tile_x++;
			tm->view_left = view_add( tm->view_left, 1 );
		}
		else {
			// -x boundary
			for( u32 vy = 0; vy < VIEW_SIZE; vy++ ) {
				terrain_load_tile( tm,
					tm->tile_x - VIEW_HALF - 1,
					tm->tile_y + vy - VIEW_HALF,
					view_sub( tm->view_left, 1 ),
					view_add( tm->view_top, vy ) );
			}
			tm->tile_x--;
			tm->view_left = view_sub( tm->view_left, 1 );
		}
	}

	if( player_tile_y != tm->tile_y ) {
		if( player_tile_y > tm->tile_y ) {
			// +y boundary
			for( u32 vx = 0; vx < VIEW_SIZE; vx++ ) {
				terrain_load_tile( tm,
					tm->tile_x + vx - VIEW_HALF,
					tm->tile_y + VIEW_HALF + 1,
					view_add( tm->view_left, vx ),
					view_add( tm->view_top, VIEW_SIZE ) );
			}
			tm->tile_y++;
			tm->view_top = view_add( tm->view_top, 1 );
		}
		else {
			// -y boundary
			for( u32 vx = 0; vx < VIEW_SIZE; vx++ ) {
				terrain_load_tile( tm,
					tm->tile_x + vx - VIEW_HALF,
					tm->tile_y - VIEW_HALF - 1,
					view_add( tm->view_left, vx ),
					view_sub( tm->view_top, 1 ) );
			}
			tm->tile_y--;
			tm->view_top = view_sub( tm->view_top, 1 );
		}
	}
}

void terrain_render( const TerrainManager * const tm, const glm::mat4 VP, const float sun_slope ) {
	const glm::vec3 sun = glm::normalize( glm::vec3( 1, 0, -sun_slope ) );

	glUseProgram( tm->shader );

	glUniformMatrix4fv( tm->un_vp, 1, GL_FALSE, glm::value_ptr( VP ) );
	glUniform3fv( tm->un_sun, 1, glm::value_ptr( sun ) );

	for( u32 vy = 0; vy < VIEW_SIZE; vy++ ) {
		for( u32 vx = 0; vx < VIEW_SIZE; vx++ ) {
			tm->tiles[ vx ][ vy ].render();
		}
	}

	glUseProgram( 0 );
}

float terrain_height( const TerrainManager * const tm, const float x, const float y ) {
	const u32 tx = x / TILE_SIZE;
	const u32 ty = y / TILE_SIZE;

	// TODO: totally wrong
	// convert into tile space
	const Heightmap * const hm = &tm->tiles[ tx ][ ty ];

	return hm->bilerp_height( x - tx * TILE_SIZE, y - ty * TILE_SIZE );
}
