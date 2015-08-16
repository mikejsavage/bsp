#include <string>
#include <iostream>
#include <fstream>

#include <stdio.h>
#include <sys/stat.h>

#include "intrinsics.h"
#include "stb_image.h"
#include "stb_image_write.h"

const int TILE_SIZE = 128;
const int OUT_SIZE = TILE_SIZE + 1;

int w, h;
u8 * pixels;

void write_tile( const std::string & dir, const int tx, const int ty ) {
	u8 tile[ OUT_SIZE * OUT_SIZE ];

	for( int y = 0; y <= TILE_SIZE; y++ ) {
		for( int x = 0; x <= TILE_SIZE; x++ ) {
			const int gy = ty * TILE_SIZE + y;
			const int gx = tx * TILE_SIZE + x;

			if( gy >= h || gx >= w ) {
				tile[ y * OUT_SIZE + x ] = 0;
			}
			else {
				tile[ y * OUT_SIZE + x ] = pixels[ gy * w + gx ];
			}
		}
	}

	const std::string file = dir + "/" + std::to_string( tx ) + "_" + std::to_string( ty ) + ".tga";
	printf( "writing tile %s\n", file.c_str() );
	stbi_write_tga( file.c_str(), OUT_SIZE, OUT_SIZE, 1, tile );
}

int main( int argc, char ** argv ) {
	const std::string path = argc == 2 ? argv[ 1 ] : "mountains512.png";
	pixels = stbi_load( path.c_str(), &w, &h, nullptr, 1 );

	const std::string dir = path + ".parts";

	mkdir( dir.c_str(), 0755 );

	for( int ty = 0; ty < h / TILE_SIZE; ty++ ) {
		for( int tx = 0; tx < w / TILE_SIZE; tx++ ) {
			write_tile( dir, tx, ty );
		}
	}

	std::ofstream dims( dir + "/dims.txt" );
	dims << std::to_string( w ) + " " + std::to_string( h );
	dims.close();

	stbi_image_free( pixels );

	return 0;
}
