
#ifndef LSURFACE_H_
#define LSURFACE_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_surface.h>

#define LGAME_SURFACE_METATABLE "lgame.Surface"

typedef struct lgame_Surface {
  SDL_Surface* surf;
  int isws;
} lgame_Surface;

extern const struct luaL_Reg lgame_surface_module[];
extern const struct luaL_Reg lgame_Surface_methods[];
extern const struct luaL_Reg lgame_Surface_metamethods[];

lgame_Surface* lgame_pushsurface(lua_State* L);
lgame_Surface* lgame_checksurface(lua_State* L, int idx);
lgame_Surface* lgame_testsurface(lua_State* L, int idx);

#endif