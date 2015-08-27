#include <stdlib.h>
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
#include "stb_truetype.h"

#include "platform_opengl.h"
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

static const GLchar * const textured_vert_src = GLSL(
	in vec3 position;
	in vec4 colour;
	in vec2 uv;

	out vec4 frag_colour;
	out vec2 frag_uv;

	void main() {
		gl_Position = vec4( position, 1.0 );
		frag_colour = colour;
		frag_uv = uv;
	}
);

static const GLchar * textured_frag_src = GLSL(
	in vec4 frag_colour;
	in vec2 frag_uv;

	uniform sampler2D tex;

	out vec4 screen_colour;

	void main() {
		screen_colour = texture( tex, frag_uv ) * frag_colour;
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
	terrain_init( &state->tm, "Srtm_ramp2.world.21600x10800.jpg.parts" );
	terrain_teleport( &state->tm, state->pos );

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
		glm::vec3( -crosshair_thickness / aspect,  crosshair_length * aspect, 0 ),
		glm::vec3(  crosshair_thickness / aspect,  crosshair_length * aspect, 0 ),
		glm::vec3(  crosshair_thickness / aspect, -crosshair_length * aspect, 0 ),
		red
	);
	immediate_triangle( &state->test_immediate,
		glm::vec3(  crosshair_thickness / aspect, -crosshair_length * aspect, 0 ),
		glm::vec3( -crosshair_thickness / aspect, -crosshair_length * aspect, 0 ),
		glm::vec3( -crosshair_thickness / aspect,  crosshair_length * aspect, 0 ),
		red
	);

	glClearColor( 0, 0.5, 0.7, 1 );

	// TODO
	u8 * const arial = file_get_contents( "Arial.ttf" );

	u8 * const font_memory = reserve_persistent( mem, 512 * 256 );
	const int offset = stbtt_GetFontOffsetForIndex( arial, 0 );
	const int ok = stbtt_BakeFontBitmap( arial, offset, 48, font_memory, 512, 512, ' ', 127 - ' ', state->test_chars );
	assert( ok > 0 );

	free( arial );

	Bitmap font_bitmap = { };
	font_bitmap.width = 512;
	font_bitmap.height = 256;
	font_bitmap.data = font_memory;

	Asset font_asset = { };
	font_asset.type = AT_BITMAP;
	font_asset.bitmap = font_bitmap;

	glGenTextures( 1, &font_asset.texture );
	glBindTexture( GL_TEXTURE_2D, font_asset.texture );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, font_bitmap.width, font_bitmap.height, 0, GL_RED, GL_UNSIGNED_BYTE, font_memory );

	state->assets[ ASSET_FONT ] = font_asset;

	state->test_tex_shader = compile_shader( textured_vert_src, textured_frag_src, "screen_colour" );
	state->test_tex_at_pos = glGetAttribLocation( state->test_tex_shader, "position" );
	state->test_tex_at_colour = glGetAttribLocation( state->test_tex_shader, "colour" );
	state->test_tex_at_uv = glGetAttribLocation( state->test_tex_shader, "uv" );
	state->test_tex_un_tex = glGetUniformLocation( state->test_tex_shader, "tex" );
}

static void draw_string( const GameState * state,
	float x, float y,
	const char * str,
	const GLint at_pos, const GLint at_colour, const GLint at_uv, const GLint un_tex
) {
	const float scale = 1.0f / 32.0f / 5.0f;
	const glm::vec4 white( 1, 1, 1, 1 );

	ImmediateContext imm;
	ImmediateTriangle triangles[ 256 ];
	immediate_init( &imm, triangles, array_count( triangles ) );

	glBindTexture( GL_TEXTURE_2D, state->assets[ ASSET_FONT ].texture );

	while( *str != '\0' ) {
		stbtt_aligned_quad q;
		stbtt_GetBakedQuad( ( stbtt_bakedchar * ) state->test_chars, 512, 256, *str - ' ', &x, &y, &q, true );

		const ImmediateVertex v1A = { glm::vec3( q.x0 * scale, -q.y0 * scale, 0 ), white, glm::vec2( q.s0, q.t0 ) };
		const ImmediateVertex v2A = { glm::vec3( q.x1 * scale, -q.y0 * scale, 0 ), white, glm::vec2( q.s1, q.t0 ) };
		const ImmediateVertex v3A = { glm::vec3( q.x0 * scale, -q.y1 * scale, 0 ), white, glm::vec2( q.s0, q.t1 ) };
		const ImmediateVertex v4A = { glm::vec3( q.x1 * scale, -q.y1 * scale, 0 ), white, glm::vec2( q.s1, q.t1 ) };

		immediate_triangle( &imm, v1A, v2A, v3A );
		immediate_triangle( &imm, v3A, v2A, v4A );

		str++;
	}

	immediate_render( &imm, at_pos, at_colour, true, at_uv, un_tex );

	glBindTexture( GL_TEXTURE_2D, 0 );
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

	terrain_update( &state->tm, state->pos );

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

	terrain_render( &state->tm, VP );

	glDisable( GL_DEPTH_TEST );
	glUseProgram( state->test_shader );
	immediate_render( &state->test_immediate, state->test_at_position, state->test_at_colour );
	glEnable( GL_DEPTH_TEST );


	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glUseProgram( state->test_tex_shader );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	ImmediateContext imm;
	ImmediateTriangle asdf[ 32 ];
	immediate_init( &imm, asdf, 32 );

	GLuint tex;
	glGenTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, tex );

	const glm::vec4 checkerboard[] = {
		glm::vec4( 0, 0, 0, 0.5 ),
		glm::vec4( 1, 1, 1, 0.5 ),
		glm::vec4( 1, 1, 1, 0.5 ),
		glm::vec4( 0, 0, 0, 0.5 ),
	};

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_FLOAT, checkerboard );

	const float aspect = 640.0f / 480.0f;
	const glm::vec4 white( 1, 1, 1, 1 );
	const int num_checks = 20;

	const ImmediateVertex v1 = { glm::vec3( 0.5,  1.0, 0 ), white, glm::vec2( 0, 0 ) };
	const ImmediateVertex v2 = { glm::vec3( 1.0,  1.0, 0 ), white, glm::vec2( num_checks, 0 ) };
	const ImmediateVertex v3 = { glm::vec3( 0.5,  0.5, 0 ), white, glm::vec2( 0, num_checks / aspect ) };
	const ImmediateVertex v4 = { glm::vec3( 1.0,  0.5, 0 ), white, glm::vec2( num_checks, num_checks / aspect ) };

	immediate_triangle( &imm, v1, v2, v3 );
	immediate_triangle( &imm, v3, v2, v4 );
	immediate_render( &imm, state->test_tex_at_pos, state->test_tex_at_colour, true, state->test_tex_at_uv, state->test_tex_un_tex );
	glDeleteTextures( 1, &tex );
	glBindTexture( GL_TEXTURE_2D, 0 );

	// TODO: the positions don't make sense
	draw_string( state, -140, -100, "hello", state->test_tex_at_pos, state->test_tex_at_colour, state->test_tex_at_uv, state->test_tex_un_tex );

	glUseProgram( 0 );
	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}
