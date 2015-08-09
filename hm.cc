#include <iostream>
#include <fstream>
#include <math.h>
#include <err.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "int.h"
#include "heightmap.h"
#include "terrain_manager.h"
#include "stb_easy_font.h"
#include "stb_image.h"
#include "stb_perlin.h"

#define TEXT( x, y, form, ... ) \
	do { \
		static char buffer[ 99999 ]; \
		static char text[ 2048 ]; \
		sprintf( text, form, __VA_ARGS__ ); \
		const int num_quads = stb_easy_font_print( x, y, text, nullptr, buffer, sizeof( buffer ) ); \
		glColor3f(1,1,1); \
		glVertexPointer(2, GL_FLOAT, 16, buffer); \
		glDrawArrays(GL_QUADS, 0, num_quads*4); \
	} while( 0 )

glm::vec3 angles_to_vector( const glm::vec3 & angles ) {
	return glm::vec3(
		-sin( angles.y ) * sin( angles.x ),
		-cos( angles.y ) * sin( angles.x ),
		-cos( angles.x )
	);
}

glm::vec3 angles_to_vector_xy( const glm::vec3 & angles ) {
	return glm::vec3( sin( angles.y ), cos( angles.y ), 0 );
}

void print_vec3( const std::string & name, const glm::vec3 & v ) {
	printf( "%s: %.3f %.3f %.3f\n", name.c_str(), v.x, v.y, v.z );
}

bool init = false;
glm::vec3 pos( 15000, 3000, 50 );
glm::vec3 angles( glm::radians( glm::vec3( -90, 45, 0 ) ) );
glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );
TerrainManager tm( "Srtm_ramp2.world.21600x10800.jpg.parts" );

extern "C" GAME_FRAME( game_frame ) {
	if( !init ) {
		glClearColor( 0, 0.5, 0.7, 1 );
		tm.teleport( pos );
		init = true;
	}

	// if( glfwGetKey( window, 'Q' ) ) {
	// 	break;
	// }

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	// const int fb = glfwGetKey( window, 'W' ) - glfwGetKey( window, 'S' );
	// const int lr = glfwGetKey( window, 'A' ) - glfwGetKey( window, 'D' );
	// const int dz = glfwGetKey( window, GLFW_KEY_SPACE ) - glfwGetKey( window, GLFW_KEY_LEFT_SHIFT );

	// const int pitch = glfwGetKey( window, GLFW_KEY_UP ) - glfwGetKey( window, GLFW_KEY_DOWN );
	// const int yaw = glfwGetKey( window, GLFW_KEY_RIGHT ) - glfwGetKey( window, GLFW_KEY_LEFT );

	const int fb = 0;
	const int lr = 0;
	const int dz = 0;
	const int pitch = 0;
	const int yaw = 0;

	angles.x += pitch * dt * 2;
	angles.y += yaw * dt * 2;

	pos += angles_to_vector_xy( angles ) * 100.0f * dt * ( float ) fb;
	const glm::vec3 sideways = glm::vec3( -cosf( angles.y ), sinf( angles.y ), 0 );
	pos += sideways * 100.0f * dt * ( float ) lr;
	// pos.z = hm.height( pos.x, pos.y ) + 8;
	pos.z += dz * 50.0f * dt;

	tm.update( pos );

	const glm::mat4 VP = glm::translate(
		glm::rotate(
			glm::rotate(
				P,
				angles.x,
				glm::vec3( 1, 0, 0 )
			),
			angles.y,
			glm::vec3( 0, 0, 1 )
		),
		-pos
	);

	tm.render( VP );

	// glLoadIdentity();
	//
	// glBegin( GL_TRIANGLE_STRIP );
	// glColor3f( 0.2, 0.2, 0.2 );
	// glVertex2f( -1, 1 );
	// glVertex2f( 1, 1 );
	// glVertex2f( -1, 0.95 );
	// glVertex2f( 1, 0.95 );
	// glEnd();
	//
	// glOrtho( 0, 640, 480, 0, -1, 1 );
	// TEXT( 2, 2, "%d", ( int ) ( 1 / dt ) );
	//
	// glPopMatrix();
}
