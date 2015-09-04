#include <iostream>
#include <fstream>
#include <math.h>

#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "game.h"
#include "intrinsics.h"
#include "gl.h"
#include "bsp.h"
#include "bsp_renderer.h"
#include "shitty_glsl.h"

static ImmediateTriangle triangles[ 512000 ];
static ImmediateContext imm;

static bool fix = false;
static glm::vec3 fix_start;
static glm::vec3 fix_end;

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

float point_plane_distance( const glm::vec3 point, const glm::vec3 normal, const float d ) {
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

// Adapted from RTCD, which means I need this:
//
// from Real-Time Collision Detection by Christer Ericson, published by Morgan
// Kaufmann Publishers, (c) 2005 Elsevier Inc
bool BSP::trace_seg_brush( const BSP_Brush & brush, const glm::vec3 start, const glm::vec3 dir, const float tmin, const float tmax, float * tout ) const {
	const glm::vec3 end = start + dir * tmax;
	float tfar = tmin;
	bool hit = false;

	for( u32 i = 0; i < brush.num_sides; i++ ) {
		const BSP_BrushSide & side = brush_sides[ i + brush.init_side ];
		const BSP_Plane & plane = planes[ side.plane ];

		const float start_dist = point_plane_distance( start, plane.n, plane.d );
		const float end_dist = point_plane_distance( end, plane.n, plane.d );

		// both points infront of plane - we are outside the brush
		if( start_dist > 0.0f && end_dist > 0.0f ) return false;
		// both points behind plane - we intersect with another side
		if( start_dist <= 0.0f && end_dist <= 0.0f ) continue;

		const float denom = -glm::dot( plane.n, dir );
		const float t = start_dist / denom;

		if( t >= tmin && t <= tmax ) {
			glm::vec4 colour = glm::vec4( 0, 0, 1, 1 );
			// TODO: if we are exiting the brush we want the nearest collision
			if( start_dist >= 0.0f ) {
				if( t >= tfar ) {
					tfar = t;
					hit = true;
					colour = glm::vec4( 1, 1, 1, 1 );
				}
			}
			immediate_sphere( &imm, start + t * dir, 8, colour, 8 );
		}
		else
			immediate_sphere( &imm, start + t * dir, 8, glm::vec4( 0.5, 0.5, 0.5, 1 ), 8 );
	}

	if( hit ) immediate_sphere( &imm, start + tfar * dir, 16, glm::vec4( 0.5, 0.5, 0.5, 1 ) );

	if( hit ) *tout = tfar;
	return hit;
}

void BSP::trace_seg_leaf( const u32 leaf_idx, const glm::vec3 start, const glm::vec3 dir, const float tmin, const float tmax, BSP_Intersection & bis ) const {
	const BSP_Leaf & leaf = leaves[ leaf_idx ];

	for( u32 i = 0; i < leaf.num_brushes; i++ ) {
		const BSP_Brush & brush = brushes[ leaf_brushes[ i + leaf.init_brush ] ];
		const BSP_Texture & texture = textures[ brush.texture ];

		// TODO: magic number
		if( texture.content_flags & 1 ) {
			float t;
			bool hit = trace_seg_brush( brush, start, dir, tmin, tmax, &t );

			if( hit && t ) {
				bis.hit = true;
				if( t < bis.is.t ) bis.is.t = t;
			}
		}
	}
}

void BSP::trace_seg_tree( const s32 node_idx, const glm::vec3 start, const glm::vec3 dir, const float tmin, const float tmax, BSP_Intersection & bis ) const {
	if( bis.hit ) return;

	if( node_idx < 0 ) {
		trace_seg_leaf( -( node_idx + 1 ), start, dir, tmin, tmax, bis );
		return;
	}

	const BSP_Node & node = nodes[ node_idx ];
	const BSP_Plane & plane = planes[ node.plane ];

	// ( start + dir * t ) . plane.n = plane.d
	// start . plane.n + t * dir . plane.n = plane.d
	// t * dir . plane.n = plane.d - start . plane.n
	// t * denom = dist
	const float denom = glm::dot( plane.n, dir );
	const float dist = -point_plane_distance( start, plane.n, plane.d );
	bool near_child = dist > 0.0f;
	bool check_both_sides = false;
	float t = tmax;

	if( denom != 0.0f ) {
		const float unchecked_t = dist / denom;

		// if t > tmax, we hit the plane beyond the area we want to
		// check so we only need to look at stuff on the near side
		// if t < 0, we didn't even hit the plane
		if( unchecked_t >= 0 && unchecked_t <= tmax ) {
			// we hit the plane before our threshold, so the area
			// we want to check is entirely on the other side
			if( unchecked_t < tmin ) {
				near_child = !near_child;
			}
			else {
				// otherwise we straddle the plane
				check_both_sides = true;
				t = unchecked_t;
				immediate_sphere( &imm, start + t * dir, 8, glm::vec4( 0, 1, 0, 1 ), 8 );
			}
		}
	}

	// TODO: if we hit on the near side we should early out
	trace_seg_tree( node.children[ near_child ], start, dir, tmin, t, bis );

	if( check_both_sides ) {
		trace_seg_tree( node.children[ !near_child ], start, dir, t, tmax, bis );
	}
}

bool BSP::trace_seg( const glm::vec3 & start, const glm::vec3 & end, Intersection & is ) const {
	BSP_Intersection bis = { };
	bis.is.t = 1.0f;
	bis.start = start;
	bis.end = end;

	const glm::vec3 dir = end - start;

	trace_seg_tree( 0, start, dir, 0.0f, 1.0f, bis );

	if( bis.hit ) {
		bis.is.pos = start + bis.is.t * ( end - start );
	}
	is = bis.is;

	return bis.hit;
}

BSP_Leaf & BSP::position_to_leaf( const glm::vec3 & pos ) const {
	s32 node_idx = 0;

	do {
		const BSP_Node & node = nodes[ node_idx ];
		const BSP_Plane & plane = planes[ node.plane ];

		const float dist = point_plane_distance( pos, plane.n, plane.d );

		node_idx = node.children[ dist < 0 ];
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

	MemoryArena arena = memarena_push_arena( &mem->persistent_arena, megabytes( 10 ) );

	state->pos = glm::vec3( 0, -100, 450 );
	state->angles = glm::radians( ( glm::vec3( -90, 135, 0 ) ) );

	state->test_shader = compile_shader( vert_src, frag_src, "screen_colour" );
	state->test_at_position = glGetAttribLocation( state->test_shader, "position" );
	state->test_at_colour = glGetAttribLocation( state->test_shader, "colour" );
	state->test_un_VP = glGetUniformLocation( state->test_shader, "VP" );

	bspr_init( &state->bspr, &arena, &state->bsp, state->test_at_position, state->test_at_colour );

	immediate_init( &imm, triangles, array_count( triangles ) );
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

	const glm::vec3 forward = angles_to_vector( state->angles );
	state->pos += forward * 100.0f * dt * ( float ) fb;
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

	bspr_render( &state->bspr, state->pos );

	immediate_clear( &imm );
	immediate_sphere( &imm, glm::vec3( 0, 0, 0 ), 128, glm::vec4( 1, 1, 0, 1 ) );

	if( input->keys[ 't' ] ) {
		fix = true;
		fix_start = state->pos;
		fix_end = fix_start + forward * 1000.0f;
	}

	if( fix ) {
		Intersection is;
		bool hit = state->bspr.bsp->trace_seg( fix_start, fix_end, is );

		if( hit ) {
			immediate_sphere( &imm, is.pos, 16, glm::vec4( 1, 0, 0, 1 ) );
		}
	}

	immediate_render( &imm, state->test_at_position, state->test_at_colour );
}
