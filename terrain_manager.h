#ifndef _TERRAIN_MANAGER_H_
#define _TERRAIN_MANAGER_H_

#include <string>
#include <glm/glm.hpp>

#include "intrinsics.h"
#include "heightmap.h"

static const u32 TILE_SIZE = 128;
static const u32 MAX_TILES_DIM = 256;

static const u32 VIEW_SIZE = 5;
static const u32 VIEW_HALF = VIEW_SIZE / 2;

struct TerrainManager {
	char dir[ 128 ];

	u32 width, height;
	u32 last_tx, last_ty;

	GLuint shader;

	GLint at_pos;
	GLint at_normal;
	GLint at_lit;

	GLint un_vp;
	GLint un_sun;

	bool first_teleport;

	Heightmap tiles[ MAX_TILES_DIM ][ MAX_TILES_DIM ];
};

void terrain_init( TerrainManager * const tm, const char * const tiles_dir );
void terrain_teleport( TerrainManager * const tm, const glm::vec3 position );
void terrain_update( TerrainManager * const tm, const glm::vec3 position );
void terrain_render( const TerrainManager * const tm, const glm::mat4 VP, const float sun_slope );

#endif // _TERRAIN_MANAGER_H_
