#include <stdio.h>
#include <err.h>
#include <time.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "platform_opengl.h"
#include <GLFW/glfw3.h>

#include "game.h"
#include "intrinsics.h"
#include "gl.h"

struct Game {
	void * lib;
	GameInit * init;
	GameFrame * frame;

	time_t lib_write_time;
};

time_t file_last_write_time( const char * const path ) {
	struct stat buf;
	if( stat( path, &buf ) == -1 ) {
		return 0;
	}

	return buf.st_mtime;
}

Game load_game( const char * const path ) {
	Game game = { };

	game.lib = dlopen( path, RTLD_NOW );
	if( game.lib ) {
		game.init = ( GameInit * ) dlsym( game.lib, "game_init" );
		game.frame = ( GameFrame * ) dlsym( game.lib, "game_frame" );

		if( !game.init || !game.frame ) {
			warnx( "load_game: couldn't find game functions (init = %p, frame = %p)", game.init, game.frame );

			game.init = nullptr;
			game.frame = nullptr;
		}
	}

	game.lib_write_time = file_last_write_time( path );

	const char * const error = dlerror();
	if( error ) {
		warnx( "load_game: %s", error );
	}

	return game;
}

void unload_game( Game * game ) {
	if( game->lib ) {
		dlclose( game->lib );
	}

	game->init = nullptr;
	game->frame = nullptr;
}

bool should_reload_game( const char * const path, const time_t lib_write_time ) {
	return file_last_write_time( path ) > lib_write_time;
}

int main( int argc, char ** argv ) {
	const char * const game_library_path = argc == 2 ? argv[ 1 ] : "./hm.so";

	Game game = load_game( game_library_path );
	GameMemory mem = { };
	mem.persistent_size = megabytes( 64 );
	mem.persistent = new u8[ mem.persistent_size ];

	GameState * state = ( GameState * ) reserve_persistent( mem, sizeof( GameState ) );

	GLFWwindow * const window = GL::init();

	game.init( state, mem );

	float last_frame_time = glfwGetTime();

	while( !glfwWindowShouldClose( window ) ) {
		const float current_frame_time = glfwGetTime();
		const float dt = current_frame_time - last_frame_time;

		if( glfwGetKey( window, GLFW_KEY_Q ) ) {
			break;
		}

		if( ( i32 ) current_frame_time != ( i32 ) last_frame_time ) {
			if( should_reload_game( game_library_path, game.lib_write_time ) ) {
				unload_game( &game );
				game = load_game( game_library_path );
			}
		}

		// TODO: it would be nice to do input handling out here instead of
		// just passing in window.
		if( game.frame ) {
			game.frame( mem, window, dt );
		}

		glfwSwapBuffers( window );
		glfwPollEvents();

		last_frame_time = current_frame_time;
	}

	unload_game( &game );

	GL::term();

	return 0;
}
