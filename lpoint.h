
#ifndef LPOINT_H_
#define LPOINT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_rect.h>

typedef SDL_Point lgame_Point;

void lgame_pushpoint(lua_State* L, lgame_Point p);
lgame_Point lgame_checkpoint(lua_State* L, int idx);

void lgame_pushpoint2int(lua_State* L, int x, int y);
void lgame_checkpoint2int(lua_State* L, int idx, int* x, int* y);

#endif