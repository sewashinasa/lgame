

#ifndef LDISPLAY_H_
#define LDISPLAY_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>


#define LGAME_WINDOWSURFACE_KEY "LGAME_WINDOWSURFACE"
#define lg_getwindowsurface(L) lua_getfield((L), LUA_REGISTRYINDEX, LGAME_WINDOWSURFACE_KEY)
#define lg_setwindowsurface(L) lua_setfield((L), LUA_REGISTRYINDEX, LGAME_WINDOWSURFACE_KEY)

extern const struct luaL_Reg lg_display_module[];

SDL_Surface* lg_get_windowsurface(void);
SDL_Window* lg_get_window(void);

#endif