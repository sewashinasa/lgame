
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lbase.h"
#include "lrect.h"
#include "lcolor.h"
#include "lsurface.h"
#include "ldisplay.h"
#include "levent.h"
#include "lmouse.h"
#include "lkey.h"
#include "lconsts.h"

#define PUSHCONST(L,name) \
  lua_pushinteger((L), LGAME_##name); \
  lua_setfield((L), -2, #name); \

static void createmetatable(lua_State* L, 
  const char* name, 
  const struct luaL_Reg* methods, 
  const struct luaL_Reg* metamethods) {
  luaL_newmetatable(L, name);
  if (metamethods) 
    luaL_setfuncs(L, metamethods, 0);
  if (methods) {
    lua_newtable(L);
    luaL_setfuncs(L, methods, 0);
    lua_setfield(L, -2, "METHODS");
  }
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
  createmetatable(L, LGAME_RECT_METATABLE, lgame_Rect_methods, lgame_Rect_metamethods);
  createmetatable(L, LGAME_COLOR_METATABLE, lgame_Color_methods, lgame_Color_metamethods);
  createmetatable(L, LGAME_SURFACE_METATABLE, lgame_Surface_methods, lgame_Surface_metamethods);
  createmetatable(L, LGAME_EVENT_METATABLE, lgame_Event_methods, lgame_Event_metamethods);
  createmetatable(L, LGAME_SCANCODEWRAPPER_METATABLE, NULL, lgame_ScancodeWrapper_methods);

  lua_newtable(L);
  
  luaL_newmetatable(L, "LGAME_METATABLE");
  lua_pushcfunction(L, getfunction(L, "quit", lgame_base_module));
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);

  createmodule(L, "rect", lgame_rect_module);
  createmodule(L, "color", lgame_color_module);
  createmodule(L, "surface", lgame_surface_module);
  createmodule(L, "display", lgame_display_module);
  createmodule(L, "event", lgame_event_module);
  createmodule(L, "mouse", lgame_mouse_module);
  createmodule(L, "key", lgame_key_module);

  pushfunction(L, "init", getfunction(L, "init", lgame_base_module));
  pushfunction(L, "quit", getfunction(L, "quit", lgame_base_module));
  pushfunction(L, "get_init", getfunction(L, "get_init", lgame_base_module));

  pushfunction(L, "Rect", getfunction(L, "Rect", lgame_rect_module));
  pushfunction(L, "Color", getfunction(L, "Color", lgame_color_module));
  pushfunction(L, "Surface", getfunction(L, "Surface", lgame_surface_module));

  PUSHCONST(L, SRCALPHA);

  PUSHCONST(L, NOEVENT);

  PUSHCONST(L, QUIT);

  PUSHCONST(L, KEYDOWN);
  PUSHCONST(L, KEYUP);

  PUSHCONST(L, MOUSEMOTION);
  PUSHCONST(L, MOUSEBUTTONDOWN);
  PUSHCONST(L, MOUSEBUTTONUP);
  PUSHCONST(L, MOUSEWHEEL);

  return 1;
}