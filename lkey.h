

#ifndef LKEY_H_
#define LKEY_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_keyboard.h>

#define LGAME_SCANCODEWRAPPER_METATABLE "lgame.ScancodeWrapper"


extern const struct luaL_Reg lgame_key_module[];
extern const struct luaL_Reg lgame_ScancodeWrapper_methods[];

#endif