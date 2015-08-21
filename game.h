#ifndef _GAME_H_
#define _GAME_H_

#include <glm/glm.hpp>

#include "intrinsics.h"
#include "terrain_manager.h"
#include "bsp.h"
#include "bsp_renderer.h"
#include "immediate.h"
#include "work_queue.h"
#include "keys.h"

struct GameState {
	glm::vec3 pos;
	glm::vec3 angles;
	TerrainManager tm;

	BSP bsp;
	BSPRenderer bspr;

	WorkQueue background_tasks;

	GLuint test_shader;
	GLuint test_at_position;
	GLuint test_at_colour;
	GLuint test_un_VP;

	ImmediateContext test_immediate;
};

struct GameMemory {
	size_t persistent_size;
	size_t persistent_used;
	u8 * persistent;
};

inline u8 * reserve_persistent( GameMemory & mem, const size_t size ) {
	assert( mem.persistent_used + size <= mem.persistent_size );
	assert( mem.persistent_used + size > mem.persistent_used );

	u8 * result = mem.persistent + mem.persistent_used;
	mem.persistent_used += size;

	return result;
}

struct GameInput {
	bool keys[ KEY_COUNT ];
};

#define GAME_INIT( name ) void name( GameState * state, GameMemory & mem )
typedef GAME_INIT( GameInit );

#define GAME_FRAME( name ) void name( GameMemory & mem, const GameInput * const input, const float dt )
typedef GAME_FRAME( GameFrame );

#endif // _GAME_H_
