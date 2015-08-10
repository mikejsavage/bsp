#ifndef _GAME_H_
#define _GAME_H_

struct GameMemory {
};

struct GameInput {
};

#define GAME_FRAME( name ) void name( GameMemory & mem, const GameInput & input, const float dt )
typedef GAME_FRAME( GameFrame );

#endif // _GAME_H_
