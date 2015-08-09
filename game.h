#ifndef _ASDF_H_
#define _ASDF_H_

struct GameMemory {
};

struct GameInput {
};

#define GAME_FRAME( name ) void name( GameMemory & mem, const GameInput & input, const float dt )
typedef GAME_FRAME( GameFrame );

#endif // _ASDF_H_
