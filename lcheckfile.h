
#ifndef LCHECKFILE_H_
#define LCHECKFILE_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_iostream.h>

SDL_IOStream* lgame_checkfile(lua_State* L, int fi, const char* mode);
bool lgame_closefile(SDL_IOStream* io);

#endif