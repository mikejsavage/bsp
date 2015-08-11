#ifndef _GAME_H_
#define _GAME_H_

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include "int.h"
#include "terrain_manager.h"
#include "work_queue.h"

struct GameState {
	glm::vec3 pos;
	glm::vec3 angles;
	TerrainManager tm;

	WorkQueue background_tasks;
};

struct GameMemory {
	size_t persistent_size;
	size_t persistent_used;
	u8 * persistent;
};

u8 * reserve_persistent( GameMemory & mem, const size_t size ) {
	assert( mem.persistent_used + size <= mem.persistent_size );
	assert( mem.persistent_used + size > mem.persistent_used );

	u8 * result = mem.persistent + mem.persistent_used;
	mem.persistent_used += size;

	return result;
}

struct GameInput {
};

#define GAME_INIT( name ) void name( GameState * state )
typedef GAME_INIT( GameInit );

#define GAME_FRAME( name ) void name( GameMemory & mem, GLFWwindow * window, const float dt )
typedef GAME_FRAME( GameFrame );

#endif // _GAME_H_
