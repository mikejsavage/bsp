/*
 * I use the following naming conventions in this file::
 * TODO: no i dont
 *
 *   - tx/ty are used to denote tile coordinates
 *   - ptx/pty are used to denote the player's tile coordinates
 *   - x/y are used to denote local coordinates within a tile
 *   - wx/wy are used to denote global coordinates
 */

#include <string>

#include <glm/glm.hpp>

#include "int.h"
#include "heightmap.h"
#include "terrain_manager.h"

int iabs( const int x ) {
	return x < 0 ? -x : x;
}

TerrainManager::TerrainManager( const std::string & dir ) {
	use( dir );
}

void TerrainManager::use( const std::string & dir ) {
	this->dir = dir;

	FILE * dims = fopen( ( dir + "/dims.txt" ).c_str(), "r" );
	fscanf( dims, "%d %d", &w, &h );
	fclose( dims );
	printf( "%d %d\n", w, h );

	for( int ty = 0; ty < REGION_SIZE; ty++ ) {
		for( int tx = 0; tx < REGION_SIZE; tx++ ) {
			const int rx = tx - REGION_HALF;
			const int ry = ty - REGION_HALF;

			if( rx == ry || rx == -ry ) {
				lods[ tx ][ ty ] = std::max( 1, iabs( rx ) + iabs( ry ) - 1 );
			}
			else {
				lods[ tx ][ ty ] = std::max( iabs( rx ), iabs( ry ) );
			}

			tiles[ tx ][ ty ].init();
		}
	}

	for( int ty = 0; ty < REGION_SIZE; ty++ ) {
		for( int tx = 0; tx < REGION_SIZE; tx++ ) {
			printf( "%d ", lods[ tx ][ ty ] );
		}
		printf( "\n" );
	}
}

std::string TerrainManager::tp( const int tx, const int ty ) const {
	return dir + "/" + std::to_string( tx ) + "_" + std::to_string( ty ) + ".tga";
}

void TerrainManager::update( const glm::vec3 & position ) {
	const int new_ptx = position.x / TILE_SIZE;
	const int new_pty = position.y / TILE_SIZE;

	if( new_ptx != ptx ) {
		if( new_ptx > ptx ) {
			printf( "boundary +x\n" );
			for( int ty = 0; ty < REGION_SIZE; ty++ ) {
				tiles[ 0 ][ ty ].unload();

				for( int tx = 1; tx < REGION_SIZE; tx++ ) {
					tiles[ tx - 1 ][ ty ] = std::move( tiles[ tx ][ ty ] );
				}

				tiles[ REGION_SIZE - 1 ][ ty ].load(
					tp( ptx + REGION_HALF + 1, pty + ty - REGION_HALF ),
					( ptx + REGION_HALF + 1 ) * TILE_SIZE,
					( pty + ty - REGION_HALF ) * TILE_SIZE );
			}

			ptx++;
		}
		else {
			printf( "boundary -x\n" );
			for( int ty = 0; ty < REGION_SIZE; ty++ ) {
				tiles[ REGION_SIZE - 1 ][ ty ].unload();

				for( int tx = REGION_SIZE - 2; tx >= 0; tx-- ) {
					tiles[ tx + 1 ][ ty ] = std::move( tiles[ tx ][ ty ] );
				}

				tiles[ 0 ][ ty ].load(
					tp( ptx - REGION_HALF - 1, pty + ty - REGION_HALF ),
					( ptx - REGION_HALF - 1 ) * TILE_SIZE,
					( pty + ty - REGION_HALF ) * TILE_SIZE );
			}

			ptx--;
		}
	}

	if( new_pty != pty ) {
		if( new_pty > pty ) {
			printf( "boundary +y\n" );
			for( int tx = 0; tx < REGION_SIZE; tx++ ) {
				tiles[ tx ][ 0 ].unload();

				for( int ty = 1; ty < REGION_SIZE; ty++ ) {
					tiles[ tx ][ ty - 1 ] = std::move( tiles[ tx ][ ty ] );
				}

				tiles[ tx ][ REGION_SIZE - 1 ].load(
					tp( ptx + tx - REGION_HALF, pty + REGION_HALF + 1 ),
					( ptx + tx - REGION_HALF ) * TILE_SIZE,
					( pty + REGION_HALF + 1 ) * TILE_SIZE );
			}
			pty++;
		}
		else {
			printf( "boundary -y\n" );
			for( int tx = 0; tx < REGION_SIZE; tx++ ) {
				tiles[ tx ][ REGION_SIZE - 1 ].unload();

				for( int ty = REGION_SIZE - 2; ty >= 0; ty-- ) {
					tiles[ tx ][ ty + 1 ] = std::move( tiles[ tx ][ ty ] );
				}

				tiles[ tx ][ 0 ].load(
					tp( ptx + tx - REGION_HALF, pty - REGION_HALF - 1 ),
					( ptx + tx - REGION_HALF ) * TILE_SIZE,
					( pty - REGION_HALF - 1 ) * TILE_SIZE );
			}
			pty--;
		}
	}
}

void TerrainManager::prepare_teleport( const glm::vec3 & position ) {
}

bool TerrainManager::teleport_ready() {
	return false;
}

void TerrainManager::teleport( const glm::vec3 & position ) {
	cx = cy = REGION_HALF;
	ptx = position.x / TILE_SIZE;
	pty = position.y / TILE_SIZE;

	for( int ty = 0; ty < REGION_SIZE; ty++ ) {
		for( int tx = 0; tx < REGION_SIZE; tx++ ) {
			const int ltx = ptx + tx - REGION_HALF;
			const int lty = pty + ty - REGION_HALF;

			if( ltx >= 0 && ltx < w && lty >= 0 && lty < h ) {
				tiles[ tx ][ ty ].load(
					dir + "/" + std::to_string( ltx ) + "_" + std::to_string( lty ) + ".tga",
					ltx * TILE_SIZE,
					lty * TILE_SIZE
				);
			}
			else {
				tiles[ tx ][ ty ].unload();
			}
		}
	}

}

void TerrainManager::render( const glm::mat4 & VP ) {
	for( int ty = 0; ty < REGION_SIZE; ty++ ) {
		for( int tx = 0; tx < REGION_SIZE; tx++ ) {
			tiles[ tx ][ ty ].render( VP );
		}
	}
}
