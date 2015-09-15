// easy = one day
// medium = one week
// hard = many weeks
// hard++ = significant study required
//
// rendering
// 	textures - easy/medium
// 	unify the renderers - hard
// 	multithreading - medium
// 	particles - medium
// 	fonts - medium/hard
// 	ui - medium/hard
// 	allocate vao/vbo on immediate renderer initialization? - easy - probably not worth doing
//
// lighting
// 	deferred shading - medium/hard
// 	shadow volumes - medium/hard
// 	ambient aperture lighting - medium
//
// terrain
// 	multithreading - medium
// 	binary triangle trees - medium
// 	compute proper horizon - medium
// 	load bsps - hard
// 	load models? - hard
//
// assets
// 	ram virtual memory - medium
// 	vram virtual memory - hard
// 	lock critical assets in memory to avoid graphical glitches - hard
//
// collision
// 	bsp - medium
// 	heightmap - medium
// 	bodyblocking/bvh - hard
// 	models on heightmap - hard
// 	union - medium
//
// sound
// 	all of it - hard
//
// networking
// 	all of it - hard++
//
// 	set up communication client->server with no sync - low/medium
// 	also simulate movement on server - low/medium
// 	give server authority when they disagree - low/medium
// 	go back in time by some amount - low/medium
// 	rtt estimation - low/medium
//
// demo playback/ingame tv
// 	all of it - hard++
// 	fp determinism....
//
// 	would be nice to have recent gameplay recorded automatically
// 	mindlink through tv servers
//
// animation
// 	all of it - hard++

#ifndef _GAME_H_
#define _GAME_H_

#include <glm/glm.hpp>

#include "intrinsics.h"
#include "assets.h"
#include "terrain_manager.h"
#include "btt.h"
#include "gpubtt.h"
#include "heightmap.h"
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

	GLint test_at_normal;
	GLint test_at_lit;
	GLint test_un_sun;

	GLuint test_outline_shader;
	GLint test_outline_at_position;
	GLint test_outline_at_colour;
	GLint test_outline_un_vp;

	BTTs btt;
	GPUBTT gpubtt;
	Heightmap hm;

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
