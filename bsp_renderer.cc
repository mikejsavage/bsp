#include <string.h>

#include "platform_opengl.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "intrinsics.h"
#include "bsp.h"
#include "bsp_renderer.h"
#include "memory_arena.h"
#include "immediate.h"

glm::vec3 bspr_face_colour( const BSP * const bsp, const BSP_Face & face ) {
	if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_olive033") == 0) {
		return glm::vec3(0, 0.33, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_olive066") == 0) {
		return glm::vec3(0, 0.66, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_olive100") == 0) {
		return glm::vec3(0, 1, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_olive100_fade") == 0) {
		return glm::vec3(0.25, 1, 0.25);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_red100_fade") == 0) {
		return glm::vec3(1, 0.25, 0.25);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_red100") == 0) {
		return glm::vec3(1, 0, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_red066") == 0) {
		return glm::vec3(0.66, 0, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_red033") == 0) {
		return glm::vec3(0.33, 0, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_purple033") == 0) {
		return glm::vec3(0.33, 0, 0.33);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_purple066") == 0) {
		return glm::vec3(0.66, 0, 0.66);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_purple100") == 0) {
		return glm::vec3(1, 0, 1);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_purple100_fade") == 0) {
		return glm::vec3(1, 0.25, 1);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_cyan033") == 0) {
		return glm::vec3(0, 0.33, 0.33);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_cyan066") == 0) {
		return glm::vec3(0, 0.66, 0.66);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_cyan100") == 0) {
		return glm::vec3(0, 1, 1);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/wall_cyan100_fade") == 0) {
		return glm::vec3(0.25, 1, 1);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/struc_lightgrey") == 0) {
		//return glm::vec3(0.85, 0.85, 0.85);
		return glm::vec3(0.15, 0.15, 0.15);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/struc_darkgrey") == 0) {
		return glm::vec3(0.5, 0.5, 0.5);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/sky_black") == 0) {
		return glm::vec3(0, 0, 0);
	} else if(strcmp(bsp->textures[face.texture].name, "textures/acidwdm2/ink") == 0) {
		//return glm::vec3(0, 0, 0);
		return glm::vec3( 0, 0, 0 );
	} else {
		//return glm::vec3( 1, 1, 1 );
		return glm::vec3( 0, 0, 0 );
	}
}

void bspr_init( BSPRenderer * const bspr, MemoryArena * const arena, const BSP * const bsp ) {
	bspr->arena = arena;
	bspr->bsp = bsp;

	bspr->vaos = memarena_push_many( arena, GLuint, bsp->num_leaves );
	bspr->vbos = memarena_push_many( arena, GLuint, bsp->num_leaves * 2 );
	bspr->ebos = memarena_push_many( arena, GLuint, bsp->num_leaves );
	bspr->vertex_counts = memarena_push_many( arena, u32, bsp->num_leaves );

	glGenVertexArrays( bsp->num_leaves, bspr->vaos );
	glGenBuffers( bsp->num_leaves * 2, bspr->vbos );
	glGenBuffers( bsp->num_leaves, bspr->ebos );

	GLfloat * pos_scratch = memarena_push_many( arena, GLfloat, 4096 );
	u32 pos_scratch_used = 0;

	GLfloat * colour_scratch = memarena_push_many( arena, GLfloat, 4096 );
	u32 colour_scratch_used = 0;

	GLuint * ebo_scratch = memarena_push_many( arena, GLuint, 4096 );
	u32 ebo_scratch_used = 0;

	for( u32 l = 0; l < bsp->num_leaves; l++ ) {
		const BSP_Leaf & leaf = bsp->leaves[ l ];

		for( u32 f = 0; f < leaf.num_faces; f++ ) {
			const BSP_LeafFace & leaf_face = bsp->leaf_faces[ f + leaf.init_face ];
			const BSP_Face & face = bsp->faces[ leaf_face ];
			const glm::vec3 colour = bspr_face_colour( bsp, face );

			const BSP_Vertex * const vertices = &bsp->vertices[ face.init_vert ];
			const i32 * const indices = &bsp->mesh_verts[ face.init_mesh_vert ];

			// for( i32 v = 0; v < face.num_verts; v++ ) {
			// 	const BSP_Vertex & vert = bsp->vertices[ face.init_vert + v ];
			// 	const glm::vec3 pos = vert.pos;
                        //
			// 	pos_scratch[ pos_scratch_used++ ] = pos.x;
			// 	pos_scratch[ pos_scratch_used++ ] = pos.y;
			// 	pos_scratch[ pos_scratch_used++ ] = pos.z;
			// 	colour_scratch[ colour_scratch_used++ ] = colour.x;
			// 	colour_scratch[ colour_scratch_used++ ] = colour.y;
			// 	colour_scratch[ colour_scratch_used++ ] = colour.z;
                        //
			// 	assert( pos_scratch_used < 4096 && colour_scratch_used < 4096 );
			// }

			for( i32 m = 0; m < face.num_mesh_verts; m++ ) {
				const glm::vec3 & pos = vertices[ indices[ m ] ].pos;

				pos_scratch[ pos_scratch_used++ ] = pos.x;
				pos_scratch[ pos_scratch_used++ ] = pos.y;
				pos_scratch[ pos_scratch_used++ ] = pos.z;
				colour_scratch[ colour_scratch_used++ ] = colour.x;
				colour_scratch[ colour_scratch_used++ ] = colour.y;
				colour_scratch[ colour_scratch_used++ ] = colour.z;

				ebo_scratch_used++;

				assert( pos_scratch_used < 4096 && colour_scratch_used < 4096 );
				// const u32 idx = bsp->mesh_verts[ face.init_mesh_vert + m ];
				// const glm::vec3 pos = bsp->vertices[ face.init_vert + idx ].pos;
                                //
				// ebo_scratch[ ebo_scratch_used++ ] = idx;
                                //
				// assert( ebo_scratch_used < 4096 );
			}
		}

		glBindVertexArray( bspr->vaos[ l ] );

		glBindBuffer( GL_ARRAY_BUFFER, bspr->vbos[ l * 2 ] );
		glBufferData( GL_ARRAY_BUFFER, pos_scratch_used * sizeof( GLfloat ), pos_scratch, GL_STATIC_DRAW );

		glBindBuffer( GL_ARRAY_BUFFER, bspr->vbos[ l * 2 + 1 ] );
		glBufferData( GL_ARRAY_BUFFER, colour_scratch_used * sizeof( GLfloat ), colour_scratch, GL_STATIC_DRAW );

		glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bspr->ebos[ l ] );
		glBufferData( GL_ELEMENT_ARRAY_BUFFER, ebo_scratch_used * sizeof( GLuint ), ebo_scratch, GL_STATIC_DRAW );

		glBindVertexArray( 0 );

		bspr->vertex_counts[ l ] = ebo_scratch_used;

		pos_scratch_used = 0;
		colour_scratch_used = 0;
		ebo_scratch_used = 0;
	}
}

static void bspr_render_leaf( const BSPRenderer * const bspr, const u32 leaf, const GLuint at_position, const GLuint at_colour ) {
	glBindVertexArray( bspr->vaos[ leaf ] );

	glBindBuffer( GL_ARRAY_BUFFER, bspr->vbos[ leaf * 2 ] );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, bspr->ebos[ leaf ] );
	glEnableVertexAttribArray( at_position );
	glVertexAttribPointer( at_position, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	glBindBuffer( GL_ARRAY_BUFFER, bspr->vbos[ leaf * 2 + 1 ] );
	glEnableVertexAttribArray( at_colour );
	glVertexAttribPointer( at_colour, 3, GL_FLOAT, GL_FALSE, 0, 0 );

	// glDrawElements( GL_TRIANGLES, bspr->vertex_counts[ leaf ], GL_UNSIGNED_INT, 0 );
	glDrawArrays( GL_TRIANGLES, 0, bspr->vertex_counts[ leaf ] );

	glBindVertexArray( 0 );
}

static const glm::mat4 P( glm::perspective( glm::radians( 120.0f ), 640.0f / 480.0f, 0.1f, 10000.0f ) );

void bspr_render( const BSPRenderer * const bspr, const glm::vec3 & pos, const GLuint at_position, const GLuint at_colour ) {
	const i32 cluster = bspr->bsp->position_to_leaf( pos ).cluster;

	if( cluster == -1 ) {
		for( u32 i = 0; i < bspr->bsp->num_leaves; i++ ) {
			bspr_render_leaf( bspr, i, at_position, at_colour );
		}

		return;
	}

	for( u32 i = 0; i < bspr->bsp->num_leaves; i++ ) {
		const BSP_Leaf & leaf = bspr->bsp->leaves[ i ];

		const i32 other_cluster = leaf.cluster;
		const i32 pvs_idx = cluster * bspr->bsp->vis->cluster_size + other_cluster / 8;

		if( bspr->bsp->vis->pvs[ pvs_idx ] & ( 1 << other_cluster % 8 ) ) {
			bspr_render_leaf( bspr, i, at_position, at_colour );
		}
	}
}

void bspr_destroy( BSPRenderer * const bspr ) {
	memarena_clear( bspr->arena );

	glDeleteBuffers( bspr->bsp->num_leaves, bspr->ebos );
	glDeleteBuffers( bspr->bsp->num_leaves * 2, bspr->vbos );
	glDeleteVertexArrays( bspr->bsp->num_leaves, bspr->vaos );
}
