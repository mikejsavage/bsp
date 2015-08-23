#include <iostream>
#include <fstream>
#include <math.h>
#include <err.h>

#include "platform_opengl.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "intrinsics.h"
#include "heightmap.h"
#include "terrain_manager.h"
#include "work_queue.h"
#include "stb_image.h"
#include "stb_perlin.h"

#include "shitty_glsl.h"

static const GLchar * const vert_src = GLSL(
	in vec3 position;
	in vec4 colour;

	out vec4 frag_colour;

	void main() {
		gl_Position = vec4( position, 1.0 );
		frag_colour = colour;
	}
);

static const GLchar * frag_src = GLSL(
	in vec4 frag_colour;

	out vec4 screen_colour;

	void main() {
		screen_colour = frag_colour;
	}
);

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

glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

static WORK_QUEUE_CALLBACK( testwq ) {
	u32 i = *( u32 * ) data;
	printf( "the thread got called %u\n", i );
}

extern "C" GAME_INIT( game_init ) {
	state->pos = glm::vec3( 15000, 3000, 50 );
	state->angles = glm::radians( glm::vec3( -90, 45, 0 ) );
	state->tm.use( "Srtm_ramp2.world.21600x10800.jpg.parts" );
	state->tm.teleport( state->pos );

	workqueue_init( &state->background_tasks, 2 );
	u32 nums[ 10 ];
	for( u32 i = 0; i < 10; i++ ) {
		nums[ i ] = i;
		workqueue_enqueue( &state->background_tasks, testwq, &nums[ i ] );
	}
	workqueue_exhaust( &state->background_tasks );

	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_at_colour = glGetAttribLocation( state->test_shader, "colour" );

	// TODO: persistent memory should be an arena
	const size_t triangles = 65536;
	ImmediateTriangle * immediate_memory = ( ImmediateTriangle * ) reserve_persistent( mem, sizeof( ImmediateTriangle ) * triangles );
	immediate_init( &state->test_immediate, immediate_memory, triangles );

	const float aspect = 640.0f / 480.0f;
	const float crosshair_thickness = 0.005;
	const float crosshair_length = 0.02;

	const glm::vec4 red( 1, 0, 0, 1 );
	immediate_triangle( &state->test_immediate,
		glm::vec3( -crosshair_length, -crosshair_thickness, 0 ),
		glm::vec3( -crosshair_length,  crosshair_thickness, 0 ),
		glm::vec3(  crosshair_length,  crosshair_thickness, 0 ),
		red
	);
	immediate_triangle( &state->test_immediate,
		glm::vec3(  crosshair_length,  crosshair_thickness, 0 ),
		glm::vec3(  crosshair_length, -crosshair_thickness, 0 ),
		glm::vec3( -crosshair_length, -crosshair_thickness, 0 ),
		red
	);
	immediate_triangle( &state->test_immediate,
		glm::vec3( -crosshair_thickness / aspect,  crosshair_length * aspect , 0 ),
		glm::vec3(  crosshair_thickness / aspect,  crosshair_length * aspect , 0 ),
		glm::vec3(  crosshair_thickness / aspect, -crosshair_length * aspect , 0 ),
		red
	);
	immediate_triangle( &state->test_immediate,
		glm::vec3(  crosshair_thickness / aspect, -crosshair_length * aspect , 0 ),
		glm::vec3( -crosshair_thickness / aspect, -crosshair_length * aspect , 0 ),
		glm::vec3( -crosshair_thickness / aspect,  crosshair_length * aspect , 0 ),
		red
	);

	glClearColor( 0, 0.5, 0.7, 1 );
}

extern "C" GAME_FRAME( game_frame ) {
	GameState * state = ( GameState * ) mem.persistent;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	const int fb = input->keys[ 'w' ] - input->keys[ 's' ];
	const int lr = input->keys[ 'a' ] - input->keys[ 'd' ];
	const int dz = input->keys[ KEY_SPACE ] - input->keys[ KEY_LEFTSHIFT ];

	const int pitch = input->keys[ KEY_UPARROW ] - input->keys[ KEY_DOWNARROW ];
	const int yaw = input->keys[ KEY_RIGHTARROW ] - input->keys[ KEY_LEFTARROW ];

	state->angles.x += pitch * dt * 2;
	state->angles.y += yaw * dt * 2;

	state->pos += angles_to_vector_xy( state->angles ) * 100.0f * dt * ( float ) fb;
	const glm::vec3 sideways = glm::vec3( -cosf( state->angles.y ), sinf( state->angles.y ), 0 );
	state->pos += sideways * 100.0f * dt * ( float ) lr;
	// pos.z = hm.height( pos.x, pos.y ) + 8;
	state->pos.z += dz * 50.0f * dt;

	state->tm.update( state->pos );

	const glm::mat4 VP = glm::translate(
		glm::rotate(
			glm::rotate(
				P,
				state->angles.x,
				glm::vec3( 1, 0, 0 )
			),
			state->angles.y,
			glm::vec3( 0, 0, 1 )
		),
		-state->pos
	);

	state->tm.render( VP );

	glDisable( GL_DEPTH_TEST );
	glUseProgram( state->test_shader );
	immediate_render( &state->test_immediate, state->test_at_position, state->test_at_colour );
	glEnable( GL_DEPTH_TEST );
}
