
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lrect.h"
#include "lcolor.h"

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

__declspec(dllexport) int luaopen_lgame(lua_State* L) {
  createmetatable(L, LGAME_RECT_METATABLE, lg_Rect_methods, lg_Rect_metamethods);
  createmetatable(L, LGAME_COLOR_METATABLE, lg_Color_methods, lg_Color_metamethods);

  lua_newtable(L);
  createmodule(L, "rect", lg_Rect_module);
  createmodule(L, "color", lg_Color_module);

  return 1;
}