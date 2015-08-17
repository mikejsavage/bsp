#include <iostream>
#include <fstream>
#include <math.h>

#include "platform_opengl.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "intrinsics.h"
#include "gl.h"
#include "bsp.h"
#include "bsp_renderer.h"
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

static const float EPSILON = 1.0 / 32.0;

// TODO: bit twiddling?
bool same_sign( const float a, const float b ) {
	return a * b >= 0;
}

float point_plane_distance( const glm::vec3 & point, const glm::vec3 & normal, float d ) {
	return glm::dot( point, normal ) - d;
}

BSP::BSP( const std::string filename ) {
	std::ifstream file( filename, std::ifstream::binary );

	assert( file.is_open() );

	file.seekg( 0, file.end );
	ssize_t len = file.tellg();
	file.seekg( 0, file.beg );

	contents = new char[ len ];
	file.read( contents, len );

	assert( ( file.rdstate() & std::ifstream::failbit ) == 0 );

	file.close();

	load_lump( num_textures, textures, LUMP_TEXTURES );
	load_lump( num_planes, planes, LUMP_PLANES );
	load_lump( num_nodes, nodes, LUMP_NODES );
	load_lump( num_leaves, leaves, LUMP_LEAVES );
	load_lump( num_leaf_faces, leaf_faces, LUMP_LEAFFACES );
	load_lump( num_leaf_brushes, leaf_brushes, LUMP_LEAFBRUSHES );
	load_lump( num_brushes, brushes, LUMP_BRUSHES );
	load_lump( num_brush_sides, brush_sides, LUMP_BRUSHSIDES );
	load_lump( num_vertices, vertices, LUMP_VERTICES );
	load_lump( num_mesh_verts, mesh_verts, LUMP_MESHVERTS );
	load_lump( num_faces, faces, LUMP_FACES );
	load_vis();
}

BSP::~BSP() {
	delete contents;
}

template< typename T >
void BSP::load_lump( u32 & num_ts, T *& ts, const BSP_Lump lump ) {
	const BSP_Header * const header = reinterpret_cast< BSP_Header * >( contents );
	const BSP_HeaderLump & hl = header->lumps[ lump ];

	assert( hl.len % sizeof( T ) == 0 );

	num_ts = hl.len / sizeof( T );
	ts = reinterpret_cast< T * >( contents + hl.off );

	printf( "%d: ok. off %u len %u num %u\n", lump, hl.off, hl.len, num_ts );
}

void BSP::load_vis() {
	const BSP_Header * const header = reinterpret_cast< BSP_Header * >( contents );
	const BSP_HeaderLump & hl = header->lumps[ LUMP_VISIBILITY ];

	vis = reinterpret_cast< BSP_Vis * >( contents + hl.off );

	assert( hl.len == 2 * sizeof( u32 ) + vis->num_clusters * vis->cluster_size );

	printf( "%d: ok. off %u len %u num %u\n", LUMP_VISIBILITY, hl.off, hl.len, vis->num_clusters * vis->cluster_size );
}

void BSP::trace_seg_brush( const BSP_Brush & brush, BSP_Intersection & bis ) const {
	float near = -1.0;
	float far = 1.0;

	for( u32 i = 0; i < brush.num_sides; i++ ) {
		const BSP_BrushSide & side = brush_sides[ i + brush.init_side ];
		const BSP_Plane & plane = planes[ side.plane ];

		const float sd = point_plane_distance( bis.start, plane.n, plane.d );
		const float ed = point_plane_distance( bis.end, plane.n, plane.d );

		// both points infront of plane - we are outside the brush
		if( sd > 0 && ed > 0 ) {
			return;
		}

		// both points behind plane - we intersect with another side
		if( sd <= 0 && ed <= 0 ) {
			continue;
		}

		const bool entering = sd > ed;
		const float eps = entering ? -EPSILON : EPSILON;
		const float t = ( sd + eps ) / ( sd - ed );

		if( entering ) {
			if( t > near ) {
				near = t;
				bis.is.plane = &plane;
				bis.hit = true;
			}
		}
		else {
			far = fminf( t, far );
		}
	}

	// TODO if near < 0, near = 0?
	if( near < 0 ) printf( "near < 0: %.3f\n", near );

	if( near < far && near > -1.0 && near < bis.is.t ) {
		if( near >= 0 ) {
			bis.is.t = near;
		}
	}
}

void BSP::trace_seg_leaf( const i32 leaf_idx, BSP_Intersection & bis ) const {
	const BSP_Leaf & leaf = leaves[ leaf_idx ];

	for( u32 i = 0; i < leaf.num_brushes; i++ ) {
		const BSP_Brush & brush = brushes[ leaf_brushes[ i + leaf.init_brush ] ];
		const BSP_Texture & texture = textures[ brush.texture ];

		if( texture.content_flags & 1 ) {
			trace_seg_brush( brush, bis );
		}
	}
}

void BSP::trace_seg_tree( const i32 node_idx, const glm::vec3 & start, const glm::vec3 & end, const float t1, const float t2, BSP_Intersection & bis ) const {
	if( node_idx < 0 ) {
		trace_seg_leaf( -( node_idx + 1 ), bis );
		return;
	}

	const BSP_Node & node = nodes[ node_idx ];
	const BSP_Plane & plane = planes[ node.plane ];

	const float sd = point_plane_distance( start, plane.n, plane.d );
	const float ed = point_plane_distance( end, plane.n, plane.d );

	if( same_sign( sd, ed ) ) {
		trace_seg_tree( sd >= 0 ? node.pos_child : node.neg_child, start, end, t1, t2, bis );
		return;
	}

	const bool pos_to_neg = sd > ed;
	const float id = 1 / ( sd - ed );
	const float f1 = ( sd + EPSILON ) * id;
	float f2;

	if( pos_to_neg ) {
		f2 = ( sd - EPSILON ) * id;
	}
	else {
		f2 = ( sd + EPSILON ) * id;
	}

	// clamp f1/f2?
	if( f1 < 0 || f1 > 1 || f2 < 0 || f2 > 1 ) printf( "%.2f %.2f\n", f1, f2 );

	const float m1 = t1 + f1 * ( t2 - t1 );
	const float m2 = t1 + f2 * ( t2 - t1 );

	const glm::vec3 mid1 = start + f1 * ( end - start );
	const glm::vec3 mid2 = start + f2 * ( end - start );

	trace_seg_tree( pos_to_neg ? node.pos_child : node.neg_child, start, mid1, t1, m1, bis );
	trace_seg_tree( pos_to_neg ? node.neg_child : node.pos_child, mid2, end, m2, t2, bis );
}

bool BSP::trace_seg( const glm::vec3 & start, const glm::vec3 & end, Intersection & is ) const {
	BSP_Intersection bis = { };
	bis.is.t = 1;
	bis.start = start;
	bis.end = end;

	trace_seg_tree( 0, start, end, 0, 1, bis );

	if( bis.hit ) {
		bis.is.pos = start + bis.is.t * ( end - start );
	}
	is = bis.is;

	return bis.hit;
}

BSP_Leaf & BSP::position_to_leaf( const glm::vec3 & pos ) const {
	i32 node_idx = 0;

	do {
		const BSP_Node & node = nodes[ node_idx ];
		const BSP_Plane & plane = planes[ node.plane ];

		const float dist = point_plane_distance( pos, plane.n, plane.d );

		node_idx = dist >= 0 ? node.pos_child : node.neg_child;
	} while( node_idx >= 0 );

	return leaves[ -( node_idx + 1 ) ];
}

glm::vec3 d2r( const glm::vec3 & degrees ) {
	return degrees * static_cast< float >( M_PI / 180 );
}

glm::vec3 angles_to_vector( const glm::vec3 & angles ) {
	return glm::vec3(
		-sin( angles.y ) * sin( angles.x ),
		-cos( angles.y ) * sin( angles.x ),
		-cos( angles.x )
	);
}

static const glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

extern "C" GAME_INIT( game_init ) {
	state->bsp = BSP( "acidwdm2.bsp" );

	u8 * memory = reserve_persistent( mem, megabytes( 10 ) );
	MemoryArena arena;
	memarena_init( &arena, memory, megabytes( 10 ) );

	bspr_init( &state->bspr, &arena, &state->bsp );

	state->pos = glm::vec3( 0, -100, 450 );
	state->angles = glm::radians( ( glm::vec3( -90, 135, 0 ) ) );

	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_at_colour = glGetAttribLocation( state->test_shader, "colour" );
	state->test_un_VP = glGetUniformLocation( state->test_shader, "VP" );
}

extern "C" GAME_FRAME( game_frame ) {
	GameState * state = ( GameState * ) mem.persistent;

	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	const int fb = glfwGetKey( window, 'W' ) - glfwGetKey( window, 'S' );
	const int lr = glfwGetKey( window, 'A' ) - glfwGetKey( window, 'D' );
	const int dz = glfwGetKey( window, GLFW_KEY_SPACE ) - glfwGetKey( window, GLFW_KEY_LEFT_SHIFT );

	const int pitch = glfwGetKey( window, GLFW_KEY_UP ) - glfwGetKey( window, GLFW_KEY_DOWN );
	const int yaw = glfwGetKey( window, GLFW_KEY_RIGHT ) - glfwGetKey( window, GLFW_KEY_LEFT );

	state->angles.x += pitch * dt * 2;
	state->angles.y += yaw * dt * 2;

	state->pos += angles_to_vector( state->angles ) * 100.0f * dt * ( float ) fb;
	const glm::vec3 sideways = glm::vec3( -cosf( state->angles.y ), sinf( state->angles.y ), 0 );
	state->pos += sideways * 100.0f * dt * ( float ) lr;
	state->pos.z += ( float ) dz * 100.0f * dt;

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

	glUseProgram( state->test_shader );
	glUniformMatrix4fv( state->test_un_VP, 1, GL_FALSE, glm::value_ptr( VP ) );

	bspr_render( &state->bspr, state->pos, state->test_at_position, state->test_at_colour );
}
