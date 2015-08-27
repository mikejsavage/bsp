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

#include <glm/glm.hpp>

#include "intrinsics.h"
#include "heightmap.h"
#include "terrain_manager.h"

// TODO: lol
static char tp_path[ 256 ];
static char * tp( const TerrainManager * const tm, const u32 tx, const u32 ty ) {
	sprintf( tp_path, "%s/%d_%d.tga", tm->dir, tx, ty );
	return tp_path;
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

	for( u32 ty = 0; ty < tm->height / TILE_SIZE; ty++ ) {
		printf( "init %u\n", ty );
		for( u32 tx = 0; tx < tm->width / TILE_SIZE; tx++ ) {
			tm->tiles[ tx ][ ty ].init();
		}
	}

	tm->first_teleport = true;
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
			tm->tiles[ new_tx + tx - VIEW_HALF ][ new_ty + ty - VIEW_HALF ].load(
				tp( tm, new_tx + tx - VIEW_HALF, new_ty + ty - VIEW_HALF ),
				( new_tx + tx - VIEW_HALF ) * TILE_SIZE,
				( new_ty + ty - VIEW_HALF ) * TILE_SIZE
			);
		}
	}
}

void terrain_update( TerrainManager * const tm, const glm::vec3 position ) {
	const u32 new_tx = position.x / TILE_SIZE;
	const u32 new_ty = position.y / TILE_SIZE;

	if( new_tx != tm->last_tx ) {
		if( new_tx > tm->last_tx ) {
			printf( "boundary +x\n" );
			for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
				tm->tiles[ tm->last_tx - VIEW_HALF ][ ty ].unload();

				tm->tiles[ tm->last_tx + VIEW_HALF + 1 ][ ty ].load(
					tp( tm, tm->last_tx + VIEW_HALF + 1, tm->last_ty + ty - VIEW_HALF ),
					( tm->last_tx + VIEW_HALF + 1 ) * TILE_SIZE,
					( tm->last_ty + ty - VIEW_HALF ) * TILE_SIZE );
			}

			tm->last_tx++;
		}
		else {
			printf( "boundary -x\n" );
			for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
				tm->tiles[ tm->last_tx + VIEW_HALF ][ ty ].unload();

				tm->tiles[ tm->last_tx - VIEW_HALF - 1 ][ ty ].load(
					tp( tm, tm->last_tx - VIEW_HALF - 1, tm->last_ty + ty - VIEW_HALF ),
					( tm->last_tx - VIEW_HALF - 1 ) * TILE_SIZE,
					( tm->last_ty + ty - VIEW_HALF ) * TILE_SIZE );
			}

			tm->last_tx--;
		}
	}

	if( new_ty != tm->last_ty ) {
		if( new_ty > tm->last_ty ) {
			printf( "boundary +y\n" );
			for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
				tm->tiles[ tx ][ tm->last_ty - VIEW_HALF ].unload();

				tm->tiles[ tx ][ tm->last_ty + VIEW_HALF + 1 ].load(
					tp( tm, tm->last_tx + tx - VIEW_HALF, tm->last_ty + VIEW_HALF + 1 ),
					( tm->last_tx + tx - VIEW_HALF ) * TILE_SIZE,
					( tm->last_ty + VIEW_HALF + 1 ) * TILE_SIZE );
			}
			tm->last_ty++;
		}
		else {
			printf( "boundary -y\n" );
			for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
				tm->tiles[ tx ][ tm->last_ty + VIEW_HALF ].unload();

				tm->tiles[ tx ][ tm->last_ty - VIEW_HALF - 1 ].load(
					tp( tm, tm->last_tx + tx - VIEW_HALF, tm->last_ty - VIEW_HALF - 1 ),
					( tm->last_tx + tx - VIEW_HALF ) * TILE_SIZE,
					( tm->last_ty - VIEW_HALF - 1 ) * TILE_SIZE );
			}
			tm->last_ty--;
		}
	}
}

void terrain_render( const TerrainManager * const tm, const glm::mat4 VP ) {
	for( u32 ty = 0; ty < VIEW_SIZE; ty++ ) {
		for( u32 tx = 0; tx < VIEW_SIZE; tx++ ) {
			tm->tiles[ tm->last_tx + tx - VIEW_HALF ][ tm->last_ty + ty - VIEW_HALF ].render( VP );
		}
	}
}
