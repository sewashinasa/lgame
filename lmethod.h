
#ifndef LMETHOD_H_
#define LMETHOD_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

void lg_getmethod(lua_State* L, int obji, const char* mn);

#endif