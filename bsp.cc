#include <iostream>
#include <fstream>
#include <math.h>
#include <string.h>

#include <glm/glm.hpp>

#include <GL/glu.h>
#include <GLFW/glfw3.h>

#include "stb_easy_font.h"
#include "gl.h"
#include "bsp.h"
#include "int.h"

const float EPSILON = 1.0 / 32.0;

void glterrible() {
	printf( "glterrible\n" );
	GLenum err = glGetError();
	while(err!=GL_NO_ERROR) {
		const char * error;

		switch(err) {
			case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
			case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
			case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
			case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
			case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
		}

		printf( "GL error: %s\n", error );
		err=glGetError();
	}
}

// TODO: bit twiddling?
bool same_sign( const float a, const float b ) {
	return a * b >= 0;
}

float point_plane_distance( const glm::vec3 & point, const glm::vec3 & normal, float d ) {
	return glm::dot( point, normal ) - d;
}

void BSP::pick_color( const BSP_Face & face ) {
	if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_olive033") == 0) {
		glColor3f(0, 0.33, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_olive066") == 0) {
		glColor3f(0, 0.66, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_olive100") == 0) {
		glColor3f(0, 1, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_olive100_fade") == 0) {
		glColor3f(0.25, 1, 0.25);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_red100_fade") == 0) {
		glColor3f(1, 0.25, 0.25);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_red100") == 0) {
		glColor3f(1, 0, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_red066") == 0) {
		glColor3f(0.66, 0, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_red033") == 0) {
		glColor3f(0.33, 0, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_purple033") == 0) {
		glColor3f(0.33, 0, 0.33);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_purple066") == 0) {
		glColor3f(0.66, 0, 0.66);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_purple100") == 0) {
		glColor3f(1, 0, 1);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_purple100_fade") == 0) {
		glColor3f(1, 0.25, 1);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_cyan033") == 0) {
		glColor3f(0, 0.33, 0.33);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_cyan066") == 0) {
		glColor3f(0, 0.66, 0.66);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_cyan100") == 0) {
		glColor3f(0, 1, 1);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/wall_cyan100_fade") == 0) {
		glColor3f(0.25, 1, 1);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/struc_lightgrey") == 0) {
		//glColor3f(0.85, 0.85, 0.85);
		glColor3f(0.15, 0.15, 0.15);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/struc_darkgrey") == 0) {
		glColor3f(0.5, 0.5, 0.5);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/sky_black") == 0) {
		glColor3f(0, 0, 0);
	} else if(strcmp(textures[face.texture].name, "textures/acidwdm2/ink") == 0) {
		//glColor3f(0, 0, 0);
		glColor3f( 0, 0, 0 );
	} else {
		//glColor3f( 1, 1, 1 );
		glColor3f( 0, 0, 0 );
	}
}

template< typename T >
void BSP::load_lump( u32 & num_ts, T *& ts, BSP_Lump lump ) {
	const BSP_Header * const header = reinterpret_cast< BSP_Header * >( contents );
	const BSP_HeaderLump & hl = header->lumps[ lump ];

	assert( hl.len % sizeof( T ) == 0 );

	num_ts = hl.len / sizeof( T );
	ts = reinterpret_cast< T * >( contents + hl.off );

	printf( "%d: ok. off %u len %u num %u\n", lump, hl.off, hl.len, num_ts );
}

void BSP::trace_seg_brush( const BSP_Brush & brush, BSP_Intersection & bis ) {
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

void BSP::trace_seg_leaf( const i32 leaf_idx, BSP_Intersection & bis ) {
	const BSP_Leaf & leaf = leaves[ leaf_idx ];

	for( u32 i = 0; i < leaf.num_brushes; i++ ) {
		const BSP_Brush & brush = brushes[ leaf_brushes[ i + leaf.init_brush ] ];
		const BSP_Texture & texture = textures[ brush.texture ];

		if( texture.content_flags & 1 ) {
			trace_seg_brush( brush, bis );
		}
	}
}

void BSP::trace_seg_tree( const i32 node_idx, const glm::vec3 & start, const glm::vec3 & end, const float t1, const float t2, BSP_Intersection & bis ) {
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

bool BSP::trace_seg( const glm::vec3 & start, const glm::vec3 & end, Intersection & is ) {
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

BSP_Leaf & BSP::position_to_leaf( const glm::vec3 & pos ) {
	i32 node_idx = 0;

	do {
		const BSP_Node & node = nodes[ node_idx ];
		const BSP_Plane & plane = planes[ node.plane ];

		const float dist = point_plane_distance( pos, plane.n, plane.d );

		node_idx = dist >= 0 ? node.pos_child : node.neg_child;
	} while( node_idx >= 0 );

	return leaves[ -( node_idx + 1 ) ];
}

void BSP::render_leaf( const BSP_Leaf & leaf ) {
	for( u32 i = 0; i < leaf.num_faces; i++ ) {
		const BSP_LeafFace & leaf_face = leaf_faces[ i + leaf.init_face ];
		const BSP_Face & face = faces[ leaf_face ];

		pick_color( face );

		glVertexPointer( 3, GL_FLOAT, sizeof( BSP_Vertex ), &vertices[ face.init_vert ].pos );
		glDrawElements( GL_TRIANGLES, face.num_mesh_verts, GL_UNSIGNED_INT, &mesh_verts[ face.init_mesh_vert ] );
	}
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
	load_lump( num_vis, vis, LUMP_VISIBILITY );
}

BSP::~BSP() {
	delete contents;
}

void BSP::render( const glm::vec3 & camera ) {
	// const i32 cluster = position_to_leaf( camera ).cluster;	

	for( u32 i = 0; i < num_leaves; i++ ) {
		const BSP_Leaf & leaf = leaves[ i ];
		// const i32 other_cluster = leaf.cluster;
		// const i32 vis_idx = cluster * num_visdata + other_cluster / 8;

		// printf( "%d\n", vis_idx );

		render_leaf( leaf );
	}
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

int main() {
	BSP bsp( "acidwdm2.bsp" );
	GLFWwindow * const window = GL::init();

	const glm::vec3 start( 0, -100, 450 );
	const glm::vec3 angles( -90, 135, 0 );
	const glm::vec3 end = start + angles_to_vector( d2r( angles ) ) * 1000.0f;

	Intersection tt;
	bool hit = bsp.trace_seg( start, end, tt );

	printf( "%.3f: %s %.1f %.1f %.1f\n", tt.t, hit ? "yes" : "no", tt.pos.x, tt.pos.y, tt.pos.z );

	while( !glfwWindowShouldClose( window ) ) {
		float now = glfwGetTime();

		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		glLoadIdentity();

		// TODO: do matrices like a big boy
		gluPerspective( 120.0f, 800.0f / 600.0f, 0.1f, 10000.0f );
		glRotatef( angles.x, 1.0, 0.0, 0.0 );
		glRotatef( angles.y, 0.0, 0.0, 1.0 );
		glTranslatef( -start.x, -start.y, -start.z );

		bsp.render( start );

		const glm::vec3 test( 353.553, -453.553, 450.000 );
		glColor3f( 1, 1, 1 );
		glLineWidth( 2 );
		glBegin( GL_LINES );
		glVertex3f( test.x, test.y, test.z + 10 );
		glVertex3f( test.x, test.y, test.z - 10 );

		glVertex3f( test.x, test.y + 10, test.z );
		glVertex3f( test.x, test.y - 10, test.z );

		glVertex3f( test.x + 10, test.y, test.z );
		glVertex3f( test.x - 10, test.y, test.z );
		glEnd();

		glLoadIdentity();

		glBegin( GL_TRIANGLE_STRIP );
		glColor3f( 0.2, 0.2, 0.2 );
		glVertex2f( -1, 1 );
		glVertex2f( 1, 1 );
		glVertex2f( -1, 0.95 );
		glVertex2f( 1, 0.95 );
		glEnd();

		static char buffer[99999]; // ~500 chars
		int num_quads = stb_easy_font_print(2, 2, "hello", NULL, buffer, sizeof(buffer));

		glOrtho( 0, 640, 480, 0, -1, 1 );
		glColor3f(1,1,1);
		glVertexPointer(2, GL_FLOAT, 16, buffer);
		glDrawArrays(GL_QUADS, 0, num_quads*4);

		glfwSwapBuffers( window );
		glfwPollEvents();
	}

	GL::term();

	return 0;
}
