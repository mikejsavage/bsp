#ifndef _GAME_H_
#define _GAME_H_

#include <glm/glm.hpp>

#include "intrinsics.h"
#include "assets.h"
#include "terrain_manager.h"
#include "bsp.h"
#include "bsp_renderer.h"
#include "immediate.h"
#include "work_queue.h"
#include "memory_arena.h"
#include "keys.h"
#include "stb_truetype.h"

struct GameState {
	glm::vec3 pos;
	glm::vec3 angles;
	TerrainManager tm;

	BSP bsp;
	BSPRenderer bspr;

	WorkQueue background_tasks;

	GLuint test_shader;
	GLint test_at_position;
	GLint test_at_colour;
	GLint test_un_VP;

	GLuint test_tex_shader;
	GLint test_tex_at_pos;
	GLint test_tex_at_colour;
	GLint test_tex_at_uv;
	GLint test_tex_un_tex;

	ImmediateContext test_immediate;

	Asset assets[ ASSET_COUNT ];
	stbtt_bakedchar test_chars[ 256 ];
};

struct GameMemory {
	MemoryArena persistent_arena;
	GameState * state;
};

struct GameInput {
	bool keys[ KEY_COUNT ];
};

#define GAME_INIT( name ) void name( GameState * const state, GameMemory * const mem )
typedef GAME_INIT( GameInit );

#define GAME_FRAME( name ) void name( GameState * const state, GameMemory * const mem, const GameInput * const input, const float dt )
typedef GAME_FRAME( GameFrame );

#endif // _GAME_H_
