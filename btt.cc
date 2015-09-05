#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "game.h"
#include "intrinsics.h"
#include "btt.h"
#include "heightmap.h"
#include "gl.h"
#include "shitty_glsl.h"

static const GLchar * const vert_src = GLSL(
	in vec3 position;
	in vec3 colour;

	out vec3 frag_colour;

	uniform mat4 VP;

	void main() {
		gl_Position = VP * vec4( position, 1.0 );
		frag_colour = colour;
	}
);

static const GLchar * frag_src = GLSL(
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
	printf( "linking. left %p right %p bottom %p left_sibling %p right_sibling %p\n", node->left, node->right, node->bottom, node->left_sibling, node->right_sibling );

	printf( "alloc left/right\n" );
	node->left = memarena_push_type( arena, BTT );
	node->right = memarena_push_type( arena, BTT );

	printf( "zero left/right\n" );
	*node->left = { };
	*node->right = { };

	printf( "link left and right\n" );
	node->left->right_sibling = node->right;
	node->right->left_sibling = node->left;

	printf( "link left and right bottoms\n" );
	node->left->bottom = node->left_sibling;
	node->right->bottom = node->right_sibling;

	if( node->left_sibling ) {
		printf( "point left sibling at us\n" );
		BTT * const ls = node->left_sibling;
		if( ls->left_sibling == node ) ls->left_sibling = node->left;
		if( ls->right_sibling == node ) ls->right_sibling = node->left;
		if( ls->bottom == node ) ls->bottom = node->left;
	}

	if( node->right_sibling ) {
		printf( "point right sibling at us\n" );
		BTT * const rs = node->right_sibling;
		if( rs->left_sibling == node ) rs->left_sibling = node->right;
		if( rs->right_sibling == node ) rs->right_sibling = node->right;
		if( rs->bottom == node ) rs->bottom = node->right;
	}
}

static void btt_split( MemoryArena * const arena, BTT * const node ) {
	printf( "btt_split\n" );
	assert( node );
	// assert( !node->left && !node->right );

	printf( "do we split bottom? %p\n", node->bottom );
	if( node->bottom ) printf( "%p\n", node->bottom->bottom );
	if( node->bottom && node->bottom->bottom != node ) {
		assert( !node->bottom->left && !node->bottom->right );
		// assert( node->bottom->level == node->level - 1 );

		printf( "yes\n" );
		btt_split( arena, node->bottom );
	}

	printf( "link node\n" );
	btt_link_diamond( arena, node );

	if( node->bottom ) {
		printf( "link bottom\n" );
		btt_link_diamond( arena, node->bottom );

		printf( "touch up bottom\n" );
		node->left->left_sibling = node->bottom->right;
		node->right->left_sibling = node->bottom->left;

		node->bottom->left->right_sibling = node->right;
		node->bottom->right->left_sibling = node->left;
	}
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

	btt_build( hm, arena, roots.left_root, glm::ivec2( 0, 0 ), glm::ivec2( 0, 1 ), glm::ivec2( 1, 1 ) );
	btt_build( hm, arena, roots.right_root, glm::ivec2( 1, 1 ), glm::ivec2( 1, 0 ), glm::ivec2( 0, 0 ) );

	return roots;
}

static const glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

extern "C" GAME_INIT( game_init ) {
	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_at_colour = glGetAttribLocation( state->test_shader, "colour" );
	state->test_at_normal = glGetAttribLocation( state->test_shader, "normal" );
	state->test_at_lit = glGetAttribLocation( state->test_shader, "lit" );
	state->test_un_VP = glGetUniformLocation( state->test_shader, "VP" );

	Heightmap hm;
	hm.load( "mountains512.png", 0, 0, state->test_at_position, state->test_at_normal, state->test_at_lit );
	state->btt = btt_from_heightmap( &hm, &mem->persistent_arena );
}

extern "C" GAME_FRAME( game_frame ) {
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}
