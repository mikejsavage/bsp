#ifndef _TERRAIN_MANAGER_H_
#define _TERRAIN_MANAGER_H_

#include <string>
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "memory_arena.h"
#include "heightmap.h"

static const u32 TILE_SIZE = 128;

static const u32 VIEW_SIZE = 5;
static const u32 VIEW_HALF = VIEW_SIZE / 2;

struct TerrainManager {
	char dir[ 128 ];

	MemoryArena * arena;

	u32 width, height;

	GLuint shader;

	GLint at_pos;
	GLint at_normal;
	GLint at_lit;

	GLint un_vp;
	GLint un_sun;

	bool first_teleport;

	// tile_x and tile_y are the coordinates of the tile we are centered on
	// view_x and view_y are indices into tiles of the tile we are centered on
	u32 tile_x, tile_y;
	u32 view_left, view_top;
	Heightmap tiles[ VIEW_SIZE ][ VIEW_SIZE ];
};

void terrain_init( TerrainManager * const tm, const char * const tiles_dir,
	MemoryArena * const arena );
void terrain_teleport( TerrainManager * const tm, const glm::vec3 position );
void terrain_update( TerrainManager * const tm, const glm::vec3 position );
void terrain_render( const TerrainManager * const tm, const glm::mat4 VP, const float sun_slope );
float terrain_height( const TerrainManager * const tm, const glm::vec3 position );

#endif // _TERRAIN_MANAGER_H_
