#include <string.h>

#include <GL/gl.h>
#include <glm/glm.hpp>

#include "bsp.h"
#include "bsp_renderer.h"

void BSP_Renderer::pick_color( const BSP & bsp, const BSP_Face & face ) {
	if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_olive033") == 0) {
		glColor3f(0, 0.33, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_olive066") == 0) {
		glColor3f(0, 0.66, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_olive100") == 0) {
		glColor3f(0, 1, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_olive100_fade") == 0) {
		glColor3f(0.25, 1, 0.25);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_red100_fade") == 0) {
		glColor3f(1, 0.25, 0.25);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_red100") == 0) {
		glColor3f(1, 0, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_red066") == 0) {
		glColor3f(0.66, 0, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_red033") == 0) {
		glColor3f(0.33, 0, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_purple033") == 0) {
		glColor3f(0.33, 0, 0.33);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_purple066") == 0) {
		glColor3f(0.66, 0, 0.66);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_purple100") == 0) {
		glColor3f(1, 0, 1);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_purple100_fade") == 0) {
		glColor3f(1, 0.25, 1);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_cyan033") == 0) {
		glColor3f(0, 0.33, 0.33);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_cyan066") == 0) {
		glColor3f(0, 0.66, 0.66);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_cyan100") == 0) {
		glColor3f(0, 1, 1);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/wall_cyan100_fade") == 0) {
		glColor3f(0.25, 1, 1);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/struc_lightgrey") == 0) {
		//glColor3f(0.85, 0.85, 0.85);
		glColor3f(0.15, 0.15, 0.15);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/struc_darkgrey") == 0) {
		glColor3f(0.5, 0.5, 0.5);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/sky_black") == 0) {
		glColor3f(0, 0, 0);
	} else if(strcmp(bsp.textures[face.texture].name, "textures/acidwdm2/ink") == 0) {
		//glColor3f(0, 0, 0);
		glColor3f( 0, 0, 0 );
	} else {
		//glColor3f( 1, 1, 1 );
		glColor3f( 0, 0, 0 );
	}
}

void BSP_Renderer::render_leaf( const BSP & bsp, const BSP_Leaf & leaf ) {
	for( u32 i = 0; i < leaf.num_faces; i++ ) {
		const BSP_LeafFace & leaf_face = bsp.leaf_faces[ i + leaf.init_face ];
		const BSP_Face & face = bsp.faces[ leaf_face ];

		pick_color( bsp, face );

		glVertexPointer( 3, GL_FLOAT, sizeof( BSP_Vertex ), &bsp.vertices[ face.init_vert ].pos );
		glDrawElements( GL_TRIANGLES, face.num_mesh_verts, GL_UNSIGNED_INT, &bsp.mesh_verts[ face.init_mesh_vert ] );
	}
}

void BSP_Renderer::render( const BSP & bsp, const glm::vec3 & camera ) {
	// const i32 cluster = bsp.position_to_leaf( camera ).cluster;	

	for( u32 i = 0; i < bsp.num_leaves; i++ ) {
		const BSP_Leaf & leaf = bsp.leaves[ i ];
		// const i32 other_cluster = leaf.cluster;
		// const i32 vis_idx = cluster * num_visdata + other_cluster / 8;

		// printf( "%d\n", vis_idx );

		render_leaf( bsp, leaf );
	}
}
