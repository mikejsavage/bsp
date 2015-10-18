#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "intrinsics.h"
#include "btt.h"
#include "heightmap.h"
#include "gl.h"
#include "stb_image.h"

static ImmediateTriangle triangles[ 512000 ];
static ImmediateContext imm;

static const GLchar * const vert_src = GLSL(
	in vec3 position;

	out vec3 smooth_position;
	out float depth;

	uniform mat4 vp;

	void main() {
		gl_Position = vp * vec4( position, 1.0 );
		smooth_position = position;
		depth = gl_Position.z;
	}
);

static const GLchar * frag_src = GLSL(
	in vec3 smooth_position;
	in float depth;

	out vec4 colour;

	uniform float sun;
	uniform sampler2D normals;
	uniform sampler2D horizons;
	uniform vec2 dimensions;

	void main() {
		vec3 normal = normalize( texture( normals, smooth_position.xy / dimensions ).xyz );

		vec3 ground;
		if( normal.z > 0.9 ) {
			ground = vec3( 0.4, 1.0, 0.4 );
		}
		else {
			ground = vec3( 0.7, 0.7, 0.5 );
		}

		vec3 sunv = normalize( vec3( 1, 1, -sun ) );
		float l = sun > texture( horizons, smooth_position.xy / dimensions ).x ? 1.0 : 0.0;

		float d = max( 0, -dot( normal, sunv ) );
		float light = max( 0.2, l * d );

		vec3 fog = vec3( 0.6, 0.6, 0.6 );

		float t = smoothstep( 400, 600, depth );

		colour = vec4( ( 1.0 - t ) * ground * light + t * fog, 1.0 );
	}
);

static const GLchar * const vert_outline_src = GLSL(
	in vec3 position;
	in vec3 colour;

	out vec3 frag_colour;

	uniform mat4 vp;

	void main() {
		frag_colour = colour;
		gl_Position = vp * vec4( position, 1.0 );
	}
);

static const GLchar * frag_outline_src = GLSL(
	in vec3 frag_colour;

	out vec4 screen_colour;

	void main() {
		screen_colour = vec4( frag_colour, 1.0 );
	}
);

// triangles should look like this:
//
//             v1
//            /  \
//          /      \
//        /          \
//   v0 /______________\ v2
//         hypotenuse
//

static void btt_link_diamond( MemoryArena * const arena, BTT * const node ) {
	node->left = memarena_push_type( arena, BTT );
	node->right = memarena_push_type( arena, BTT );

	*node->left = { };
	*node->right = { };

	node->left->left_sibling = node->right;
	node->right->right_sibling = node->left;

	node->left->bottom = node->left_sibling;
	node->right->bottom = node->right_sibling;

	if( node->left_sibling ) {
		BTT * const ls = node->left_sibling;
		if( ls->left_sibling == node ) ls->left_sibling = node->left;
		if( ls->right_sibling == node ) ls->right_sibling = node->left;
		if( ls->bottom == node ) ls->bottom = node->left;
	}

	if( node->right_sibling ) {
		BTT * const rs = node->right_sibling;
		if( rs->left_sibling == node ) rs->left_sibling = node->right;
		if( rs->right_sibling == node ) rs->right_sibling = node->right;
		if( rs->bottom == node ) rs->bottom = node->right;
	}
}

static void btt_split( MemoryArena * const arena, BTT * const node ) {
	assert( node );
	assert( !node->left && !node->right );

	if( node->bottom && node->bottom->bottom != node ) {
		btt_split( arena, node->bottom );
	}

	btt_link_diamond( arena, node );

	if( node->bottom ) {
		btt_link_diamond( arena, node->bottom );

		node->left->right_sibling = node->bottom->right;
		node->right->left_sibling = node->bottom->left;

		node->bottom->left->right_sibling = node->right;
		node->bottom->right->left_sibling = node->left;
	}
}

static int iabs( const int x ) {
	return x < 0 ? -x : x;
}

static int square_distance( const glm::ivec2 u, const glm::ivec2 v ) {
	const glm::ivec2 d = v - u;
	return d.x * d.x + d.y * d.y;
}

static bool btt_should_split(
	const Heightmap * const hm,
	const glm::ivec2 v0, const glm::ivec2 v1, const glm::ivec2 v2,
	const glm::ivec2 mid
) {
	if( square_distance( v0, v2 ) <= 4 ) return false;

	const float avg_height = ( hm->point( v0.x, v0.y ).z + hm->point( v2.x, v2.y ).z ) * 0.5f;
	const float error = fabsf( avg_height - hm->point( mid.x, mid.y ).z );

	if( error > 2.0f ) return true;

	return btt_should_split( hm, v1, mid, v0, ( v1 + v0 ) / 2 ) || btt_should_split( hm, v2, mid, v1, ( v2 + v1 ) / 2 );
}

static void btt_build(
	const Heightmap * const hm, MemoryArena * const arena,
	BTT * const node,
	const glm::ivec2 v0, const glm::ivec2 v1, const glm::ivec2 v2
) {
	const glm::ivec2 mid = ( v0 + v2 ) / 2;

	if( !node->left ) {
		assert( !node->right );

		if( btt_should_split( hm, v0, v1, v2, mid ) ) {
			btt_split( arena, node );
		}
	}

	if( node->left ) {
		assert( node->right );

		btt_build( hm, arena, node->left, v1, mid, v0 );
		btt_build( hm, arena, node->right, v2, mid, v1 );
	}
}

BTTs btt_from_heightmap( const Heightmap * const hm, MemoryArena * const arena ) {
	BTTs roots;

	roots.left_root = memarena_push_type( arena, BTT );
	roots.right_root = memarena_push_type( arena, BTT );

	*roots.left_root = { };
	*roots.right_root = { };

	roots.left_root->bottom = roots.right_root;
	roots.right_root->bottom = roots.left_root;

	btt_build( hm, arena, roots.left_root, glm::ivec2( 0, 0 ), glm::ivec2( 0, hm->height - 1 ), glm::ivec2( hm->width - 1, hm->height - 1 ) );
	btt_build( hm, arena, roots.right_root, glm::ivec2( hm->width - 1, hm->height - 1 ), glm::ivec2( hm->width - 1, 0 ), glm::ivec2( 0, 0 ) );

	return roots;
}

static const glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

extern "C" GAME_INIT( game_init ) {
	state->pos = glm::vec3( 100, 100, 50 );
	state->angles = glm::radians( glm::vec3( -90, 45, 0 ) );

	state->test_sun = 0.3f;

	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_un_VP = glGetUniformLocation( state->test_shader, "vp" );
	state->test_un_sun = glGetUniformLocation( state->test_shader, "sun" );
	state->test_un_normals = glGetUniformLocation( state->test_shader, "normals" );
	state->test_un_dimensions = glGetUniformLocation( state->test_shader, "dimensions" );

	state->test_outline_shader = compile_shader( vert_outline_src, frag_outline_src, "screen_colour" );
	state->test_outline_at_position = glGetAttribLocation( state->test_outline_shader, "position" );
	state->test_outline_at_colour = glGetAttribLocation( state->test_outline_shader, "colour" );
	state->test_outline_un_vp = glGetUniformLocation( state->test_outline_shader, "vp" );

	int w, h;
	u8 * pixels = stbi_load( "terrains/mountains512.png", &w, &h, nullptr, 1 );
	heightmap_init( &state->hm, &mem->persistent_arena, pixels, w, h, 0, 0,
		state->test_at_position, state->test_at_normal, state->test_at_lit );

	state->btt = btt_from_heightmap( &state->hm, &mem->persistent_arena );

	const OffsetHeightmap ohm = { state->hm, 0, 0 };
	gpubtt_init( &mem->persistent_arena, &state->gpubtt, &ohm, state->btt, state->test_at_position );

	glClearColor( 0, 0.5, 0.7, 1 );
}

static glm::vec3 angles_to_vector_xy( const glm::vec3 & angles ) {
	return glm::vec3( sin( angles.y ), cos( angles.y ), 0 );
}

static void draw_btt(
	const BTT * const btt, const Heightmap * const hm,
	ImmediateContext * const imm,
	const glm::ivec2 iv0, const glm::ivec2 iv1, const glm::ivec2 iv2
) {
	const glm::vec4 white( 1, 1, 1, 1 );
	const glm::vec3 offset( 0.0f, 0.0f, 0.5f );

	const glm::vec3 v0( hm->point( iv0.x, iv0.y ) + offset );
	const glm::vec3 v1( hm->point( iv1.x, iv1.y ) + offset );
	const glm::vec3 v2( hm->point( iv2.x, iv2.y ) + offset );

	if( btt->left ) {
		assert( btt->right );

		const glm::ivec2 mid = ( iv0 + iv2 ) / 2;

		draw_btt( btt->left, hm, imm, iv1, mid, iv0 );
		draw_btt( btt->right, hm, imm, iv2, mid, iv1 );
	}
	else {
		immediate_triangle( imm, v0, v1, v2, white );
	}
}

extern "C" GAME_FRAME( game_frame ) {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	const int fb = input->keys[ 'w' ] - input->keys[ 's' ];
	const int lr = input->keys[ 'a' ] - input->keys[ 'd' ];
	const int dz = input->keys[ KEY_SPACE ] - input->keys[ KEY_LEFTSHIFT ];

	const float dsun = ( input->keys[ KEY_EQUALS ] - input->keys[ KEY_MINUS ] ) * dt;
	state->test_sun += dsun;

	const int pitch = input->keys[ KEY_UPARROW ] - input->keys[ KEY_DOWNARROW ];
	const int yaw = input->keys[ KEY_RIGHTARROW ] - input->keys[ KEY_LEFTARROW ];

	state->angles.x += pitch * dt * 2;
	state->angles.y += yaw * dt * 2;

	// const float speed = 6.0f;
	const float speed = 100.0f;
	state->pos += angles_to_vector_xy( state->angles ) * speed * dt * ( float ) fb;
	const glm::vec3 sideways = glm::vec3( -cosf( state->angles.y ), sinf( state->angles.y ), 0 );
	state->pos += sideways * speed * dt * ( float ) lr;
	state->pos.z += dz * 50.0f * dt;

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
	const glm::vec3 sun = glm::normalize( glm::vec3( 1, 0, -0.3 ) );

	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glUseProgram( state->test_shader );
	glUniformMatrix4fv( state->test_un_VP, 1, GL_FALSE, glm::value_ptr( VP ) );
	glUniform1f( state->test_un_sun, state->test_sun );
	glUniform2f( state->test_un_dimensions, state->hm.width, state->hm.height );
	gpubtt_render( &state->gpubtt, state->test_un_normals );
	glUseProgram( 0 );

	immediate_init( &imm, triangles, array_count( triangles ) );
	draw_btt( state->btt.left_root, &state->hm, &imm, glm::ivec2( 0, 0 ), glm::ivec2( 0, state->hm.height - 1 ), glm::ivec2( state->hm.width - 1, state->hm.height - 1 ) );
	draw_btt( state->btt.right_root, &state->hm, &imm, glm::ivec2( state->hm.width - 1, state->hm.height - 1 ), glm::ivec2( state->hm.width - 1, 0 ), glm::ivec2( 0, 0 ) );

	glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );

	glUseProgram( state->test_outline_shader );
	glUniformMatrix4fv( state->test_outline_un_vp, 1, GL_FALSE, glm::value_ptr( VP ) );
	immediate_render( &imm, state->test_outline_at_position, state->test_outline_at_colour );
	glUseProgram( 0 );
}
