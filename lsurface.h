
#ifndef LSURFACE_H_
#define LSURFACE_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_surface.h>

#define LGAME_SURFACE_METATABLE "lgame.Surface"

typedef struct lSurface {
  SDL_Surface* surf;
  int isws;
} lSurface;

extern const struct luaL_Reg lg_surface_module[];
extern const struct luaL_Reg lg_Surface_methods[];
extern const struct luaL_Reg lg_Surface_metamethods[];

lSurface* lg_pushsurface(lua_State* L);
lSurface* lg_checksurface(lua_State* L, int idx);
lSurface* lg_testsurface(lua_State* L, int idx);

#endif