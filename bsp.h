#ifndef _BSP_H_
#define _BSP_H_

#include <string>
#include "intrinsics.h"

enum BSP_Lump {
	LUMP_ENTITIES = 0,
	LUMP_TEXTURES,
	LUMP_PLANES,
	LUMP_NODES,
	LUMP_LEAVES,
	LUMP_LEAFFACES,
	LUMP_LEAFBRUSHES,
	LUMP_MODELS,
	LUMP_BRUSHES,
	LUMP_BRUSHSIDES,
	LUMP_VERTICES,
	LUMP_MESHVERTS,
	LUMP_FOGS,
	LUMP_FACES,
	LUMP_LIGHTING,
	LUMP_LIGHTGRID,
	LUMP_VISIBILITY,
	LUMP_LIGHTARRAY,
	LUMP_MAX,
};

struct BSP_HeaderLump {
	u32 off;
	u32 len;
};

struct BSP_Header {
	char ibsp[ 4 ];
	u32 version;

	BSP_HeaderLump lumps[ LUMP_MAX ];
};

struct BSP_Texture {
	char name[ 64 ];
	s32 surface_flags;
	s32 content_flags;
};

struct BSP_Plane {
	glm::vec3 n;
	float d;
};

struct BSP_Node {
	u32 plane;

	union {
		s32 children[ 2 ];
		struct {
			s32 pos_child;
			s32 neg_child;
		};
	};

	s32 mins[ 3 ];
	s32 maxs[ 3 ];
};

struct BSP_Leaf {
	s32 cluster;
	s32 area;

	s32 mins[ 3 ];
	s32 maxs[ 3 ];

	u32 init_face;
	u32 num_faces;

	u32 init_brush;
	u32 num_brushes;
};

typedef u32 BSP_LeafFace;
typedef u32 BSP_LeafBrush;

// LUMP_MODELS

struct BSP_Brush {
	u32 init_side;
	u32 num_sides;

	s32 texture;
};

struct BSP_BrushSide {
	u32 plane;
	s32 texture;
};

struct BSP_Vertex {
	glm::vec3 pos;
	float uv[ 2 ][ 2 ];
	glm::vec3 normal;
	u8 rgba[ 4 ];
};

typedef s32 BSP_MeshVert;

// LUMP_FOG

struct BSP_Face {
	s32 texture;
	s32 effect;
	s32 type;

	s32 init_vert;
	s32 num_verts;
	s32 init_mesh_vert;
	s32 num_mesh_verts;

	s32 lightmap;
	s32 lmpos[ 2 ];
	s32 lmsize[ 2 ];
	glm::vec3 lmorigin;
	glm::vec3 a;
	glm::vec3 b;

	glm::vec3 normal;

	s32 c; s32 d;
};

// LUMP_LIGHTING
// LUMP_LIGHTGRID

struct BSP_Vis {
	u32 num_clusters;
	u32 cluster_size;

	u8 pvs[];
};

// LUMP_LIGHTARRAY

struct Intersection {
	const BSP_Plane * plane;
	glm::vec3 pos;
	float t;
};

struct BSP_Intersection {
	Intersection is;
	glm::vec3 start;
	glm::vec3 end;
	bool hit;
};

class BSP {
private:
public: // TODO
	char * contents;

	u32 num_textures;
	BSP_Texture * textures;

	u32 num_planes;
	BSP_Plane * planes;

	u32 num_nodes;
	BSP_Node * nodes;

	u32 num_leaves;
	BSP_Leaf * leaves;

	u32 num_leaf_faces;
	BSP_LeafFace * leaf_faces;

	u32 num_leaf_brushes;
	BSP_LeafBrush * leaf_brushes;

	u32 num_brushes;
	BSP_Brush * brushes;

	u32 num_brush_sides;
	BSP_BrushSide * brush_sides;

	u32 num_vertices;
	BSP_Vertex * vertices;

	u32 num_mesh_verts;
	BSP_MeshVert * mesh_verts;

	u32 num_faces;
	BSP_Face * faces;

	BSP_Vis * vis;

	template< typename T > void load_lump( u32 & num_ts, T *& ts, const BSP_Lump lump );
	void load_vis();

	void trace_seg_brush( const BSP_Brush & brush, BSP_Intersection & bis ) const;
	void trace_seg_leaf( const s32 leaf_idx, BSP_Intersection & bis ) const;
	void trace_seg_tree( const s32 node_idx, const glm::vec3 & start, const glm::vec3 & end, const float t1, const float t2, BSP_Intersection & bis ) const;

	BSP_Leaf & position_to_leaf( const glm::vec3 & pos ) const;

public:
	BSP( std::string filename );
	~BSP();

	bool trace_seg( const glm::vec3 & start, const glm::vec3 & end, Intersection & is ) const;
};

#endif // _BSP_H_
