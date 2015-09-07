#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "intrinsics.h"
#include "btt.h"
#include "heightmap.h"
#include "gl.h"
#include "shitty_glsl.h"

static const GLchar * const vert_src = GLSL(
	in vec3 position;
	in vec3 normal;
	in float lit;

	out vec3 n;
	out float depth;
	out float l;

	uniform mat4 vp;

	void main() {
		n = normal;
		l = lit;
		gl_Position = vp * vec4( position, 1.0 );
		depth = gl_Position.z;
	}
);

static const GLchar * frag_src = GLSL(
	in vec3 n;
	in float depth;
	in float l;

	out vec4 colour;

	uniform vec3 sun;

	void main() {
		vec3 ground;
		if( n.z > 0.9 ) {
			ground = vec3( 0.4, 1.0, 0.4 );
		}
		else {
			ground = vec3( 0.7, 0.7, 0.5 );
		}

		float d = max( 0, -dot( n, sun ) );
		float light = max( 0.2, l * d );

		vec3 fog = vec3( 0.6, 0.6, 0.6 );

		float t = smoothstep( 400, 600, depth );

		colour = vec4( ( 1.0 - t ) * ground * light + t * fog, 1.0 );
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
	// printf( "linking. left %p right %p bottom %p left_sibling %p right_sibling %p\n", node->left, node->right, node->bottom, node->left_sibling, node->right_sibling );

	// printf( "alloc left/right\n" );
	node->left = memarena_push_type( arena, BTT );
	node->right = memarena_push_type( arena, BTT );

	// printf( "zero left/right\n" );
	*node->left = { };
	*node->right = { };

	// printf( "link left and right\n" );
	node->left->right_sibling = node->right;
	node->right->left_sibling = node->left;

	// printf( "link left and right bottoms\n" );
	node->left->bottom = node->left_sibling;
	node->right->bottom = node->right_sibling;

	if( node->left_sibling ) {
		// printf( "point left sibling at us\n" );
		BTT * const ls = node->left_sibling;
		if( ls->left_sibling == node ) ls->left_sibling = node->left;
		if( ls->right_sibling == node ) ls->right_sibling = node->left;
		if( ls->bottom == node ) ls->bottom = node->left;
	}

	if( node->right_sibling ) {
		// printf( "point right sibling at us\n" );
		BTT * const rs = node->right_sibling;
		if( rs->left_sibling == node ) rs->left_sibling = node->right;
		if( rs->right_sibling == node ) rs->right_sibling = node->right;
		if( rs->bottom == node ) rs->bottom = node->right;
	}
}

static void btt_split( MemoryArena * const arena, BTT * const node ) {
	// printf( "btt_split\n" );
	assert( node );
	// assert( !node->left && !node->right );

	// printf( "do we split bottom? %p\n", node->bottom );
	// if( node->bottom ) printf( "%p\n", node->bottom->bottom );
	if( node->bottom && node->bottom->bottom != node ) {
		assert( !node->bottom->left && !node->bottom->right );
		// assert( node->bottom->level == node->level - 1 );

		// printf( "yes\n" );
		btt_split( arena, node->bottom );
	}

	// printf( "link node\n" );
	btt_link_diamond( arena, node );

	if( node->bottom ) {
		// printf( "link bottom\n" );
		btt_link_diamond( arena, node->bottom );

		// printf( "touch up bottom\n" );
		node->left->left_sibling = node->bottom->right;
		node->right->left_sibling = node->bottom->left;

		node->bottom->left->right_sibling = node->right;
		node->bottom->right->left_sibling = node->left;
	}
}

static int iabs( const int x ) {
	return x < 0 ? -x : x;
}

// TODO: this should be recursive
static bool btt_should_split(
	const Heightmap * const hm,
	const glm::ivec2 v0, const glm::ivec2 v2, const glm::ivec2 mid
) {
	const float avg_height = hm->point( v0.x, v0.y ).z + hm->point( v2.x, v2.y ).z * 0.5f;
	const float error = fabsf( avg_height - hm->point( mid.x, mid.y ).z );

	return error > 2.0f;
}

static void btt_build(
	const Heightmap * const hm, MemoryArena * const arena,
	BTT * const node,
	const glm::ivec2 v0, const glm::ivec2 v1, const glm::ivec2 v2
) {
	if( iabs( v0.x - v2.x ) < 4 ) return;

	const glm::ivec2 mid = ( v0 + v2 ) / 2;

	if( !node->left ) {
		assert( !node->right );

		if( btt_should_split( hm, v0, v2, mid ) ) {
			btt_split( arena, node );
			// btt_split( arena, node->bottom );
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

	btt_build( hm, arena, roots.left_root, glm::ivec2( 0, 0 ), glm::ivec2( 0, hm->h ), glm::ivec2( hm->w, hm->h ) );
	btt_build( hm, arena, roots.right_root, glm::ivec2( hm->w, hm->h ), glm::ivec2( hm->w, 0 ), glm::ivec2( 0, 0 ) );

	return roots;
}

static const glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

extern "C" GAME_INIT( game_init ) {
	state->pos = glm::vec3( 100, 100, 50 );
	state->angles = glm::radians( glm::vec3( -90, 45, 0 ) );

	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_at_colour = glGetAttribLocation( state->test_shader, "colour" );
	state->test_at_normal = glGetAttribLocation( state->test_shader, "normal" );
	state->test_at_lit = glGetAttribLocation( state->test_shader, "lit" );
	state->test_un_VP = glGetUniformLocation( state->test_shader, "vp" );
	state->test_un_sun = glGetUniformLocation( state->test_shader, "sun" );

	state->hm.load( "mountains512.png", 0, 0, state->test_at_position,
		state->test_at_normal, state->test_at_lit );
	state->btt = btt_from_heightmap( &state->hm, &mem->persistent_arena );
}

static glm::vec3 angles_to_vector_xy( const glm::vec3 & angles ) {
	return glm::vec3( sin( angles.y ), cos( angles.y ), 0 );
}

extern "C" GAME_FRAME( game_frame ) {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	const int fb = input->keys[ 'w' ] - input->keys[ 's' ];
	const int lr = input->keys[ 'a' ] - input->keys[ 'd' ];
	const int dz = input->keys[ KEY_SPACE ] - input->keys[ KEY_LEFTSHIFT ];

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

	glUseProgram( state->test_shader );
	glUniformMatrix4fv( state->test_un_VP, 1, GL_FALSE, glm::value_ptr( VP ) );
	glUniform3fv( state->test_un_sun, 1, glm::value_ptr( sun ) );
	state->hm.render();
	glUseProgram( 0 );
}
