#ifndef _TERRAIN_MANAGER_H_
#define _TERRAIN_MANAGER_H_

#include <string>
#include <glm/glm.hpp>

#include "int.h"
#include "heightmap.h"

class TerrainManager {
private:
	const static int TILE_SIZE = 128;
	const static int REGION_SIZE = 11;
	const static int REGION_HALF = REGION_SIZE / 2;

	std::string dir;

	int w, h;

	Heightmap tiles[ REGION_SIZE ][ REGION_SIZE ];
	Heightmap alt_tiles[ REGION_SIZE ][ REGION_SIZE ];

	u8 lods[ REGION_SIZE ][ REGION_SIZE ];

	int cx, cy;
	int ptx, pty;

public:
	TerrainManager( const std::string & dir );
	void use( const std::string & dir );

	std::string tp( const int tx, const int ty ) const;
	void update( const glm::vec3 & position );

	// maybe these aren't needed
	void prepare_teleport( const glm::vec3 & position );
	bool teleport_ready();
	void teleport( const glm::vec3 & position );

	void render( const glm::mat4 & VP );
};

#endif // _TERRAIN_MANAGER_H_
