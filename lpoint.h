
#ifndef LPOINT_H_
#define LPOINT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_rect.h>

typedef SDL_Point lPoint;

void lg_pushpoint(lua_State* L, lPoint p);
lPoint lg_checkpoint(lua_State* L, int idx);

void lg_pushpoint2int(lua_State* L, int x, int y);
void lg_checkpoint2int(lua_State* L, int idx, int* x, int* y);

#endif