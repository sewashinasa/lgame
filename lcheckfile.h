
#ifndef LCHECKFILE_H_
#define LCHECKFILE_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_iostream.h>

SDL_IOStream* lgame_checkreadfile(lua_State* L, int fi);
bool lgame_closereadfile(lua_State* L, SDL_IOStream* io, int err);

SDL_IOStream* lgame_checkwritefile(lua_State* L, int fi, int* dyn);
bool lgame_closewritefile(lua_State* L, SDL_IOStream* io, int fi, int dyn, int err);

#endif