
#ifndef LPOINT_H_
#define LPOINT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_rect.h>

typedef SDL_Point lgame_Point;

void lgame_pushpoint(lua_State* L, lgame_Point p);
lgame_Point lgame_checkpoint(lua_State* L, int idx);
lgame_Point lgame_checkpointfromargs(lua_State* L, int offset);

void lgame_pushpoint2int(lua_State* L, int x, int y);
void lgame_checkpoint2int(lua_State* L, int idx, int* x, int* y);
void lgame_checkpoint2intfromargs(lua_State* L, int offset, int* x, int* y);

#define lgame_checkpointfromargs0(L) lgame_checkpointfromargs((L), 0)
#define lgame_checkpointfromargs1(L) lgame_checkpointfromargs((L), 1)

#define lgame_checkpoint2intfromargs0(L, x, y) lgame_checkpoint2intfromargs((L), 0, (x), (y))
#define lgame_checkpoint2intfromargs1(L, x, y) lgame_checkpoint2intfromargs((L), 1, (x), (y))

#endif