
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lbase.h"
#include "lrect.h"
#include "lcolor.h"
#include "lsurface.h"
#include "ldisplay.h"

static void createmetatable(lua_State* L, 
  const char* name, 
  const struct luaL_Reg* methods, 
  const struct luaL_Reg* metamethods) {
  luaL_newmetatable(L, name);
  luaL_setfuncs(L, metamethods, 0);
  lua_newtable(L);
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "METHODS");
}

static void createmodule(lua_State* L,
  const char* name,
  const struct luaL_Reg* module) {
  lua_newtable(L);
  luaL_setfuncs(L, module, 0);
  lua_setfield(L, -2, name);
}

static void pushfunction(lua_State* L, const char* n, lua_CFunction f) {
  lua_pushcfunction(L, f);
  lua_setfield(L, -2, n);
}

static lua_CFunction getfunction(lua_State* L, const char* n, const struct luaL_Reg *r) {
  const struct luaL_Reg* pr = r;
  while (pr->name) {
    if (strcmp(pr->name, n) == 0) {
      return pr->func;
    }
    pr++;
  }
  return NULL;
}

__declspec(dllexport) int luaopen_lgame(lua_State* L) {
  createmetatable(L, LGAME_RECT_METATABLE, lg_Rect_methods, lg_Rect_metamethods);
  createmetatable(L, LGAME_COLOR_METATABLE, lg_Color_methods, lg_Color_metamethods);
  createmetatable(L, LGAME_SURFACE_METATABLE, lg_Surface_methods, lg_Surface_metamethods);

  lua_newtable(L);
  
  luaL_newmetatable(L, "LGAME_METATABLE");
  lua_pushcfunction(L, getfunction(L, "quit", lg_base_module));
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);

  createmodule(L, "rect", lg_rect_module);
  createmodule(L, "color", lg_color_module);
  createmodule(L, "surface", lg_surface_module);
  createmodule(L, "display", lg_display_module);

  pushfunction(L, "init", getfunction(L, "init", lg_base_module));
  pushfunction(L, "quit", getfunction(L, "quit", lg_base_module));
  pushfunction(L, "get_init", getfunction(L, "get_init", lg_base_module));

  pushfunction(L, "Rect", getfunction(L, "Rect", lg_rect_module));
  pushfunction(L, "Color", getfunction(L, "Color", lg_color_module));
  pushfunction(L, "Surface", getfunction(L, "Surface", lg_surface_module));

  return 1;
}