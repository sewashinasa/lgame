
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lkey.h"


static int key_get_focused(lua_State* L);
static int key_get_pressed(lua_State* L);
static int key_get_mods(lua_State* L);
static int key_set_mods(lua_State* L);
static int key_name(lua_State* L);

static int ScancodeWrapper__index(lua_State* L);

const struct luaL_Reg lgame_key_module[] = {
  {"get_focused", key_get_focused},
  {"get_pressed", key_get_pressed},
  {"get_mods", key_get_mods},
  {"set_mods", key_set_mods},
  {"name", key_name},
  {NULL, NULL}
};

const struct luaL_Reg lgame_ScancodeWrapper_methods[] = {
  {"__index", ScancodeWrapper__index},
  {NULL, NULL}
};

static int ScancodeWrapper__index(lua_State* L) {
  int nkeys;
  SDL_Keycode key;
  SDL_Scancode sc;
  const bool* keys;
  luaL_checkudata(L, 1, LGAME_SCANCODEWRAPPER_METATABLE);
  key = (SDL_Keycode)luaL_checkinteger(L, 2);
  keys = SDL_GetKeyboardState(&nkeys);
  sc = SDL_GetScancodeFromKey(key, NULL);
  if (key == SDLK_UNKNOWN || sc == SDL_SCANCODE_UNKNOWN)
    return luaL_error(L, "unknown key");
  if (sc >= nkeys)
    return luaL_error(L, "key out of range");
  lua_pushboolean(L, keys[sc]);
  return 1;
}

static int key_get_focused(lua_State* L) {
  lua_pushboolean(L, SDL_GetKeyboardFocus() != NULL);
  return 1;
}

static int key_get_pressed(lua_State* L) {
  lua_newuserdata(L, sizeof(void*));
  luaL_getmetatable(L, LGAME_SCANCODEWRAPPER_METATABLE);
  lua_setmetatable(L, -2);
  return 1;
}

static int key_get_mods(lua_State* L) {
  lua_pushinteger(L, SDL_GetModState());
  return 1;
}

static int key_set_mods(lua_State* L) {
  SDL_Keymod mod = (SDL_Keymod)luaL_checkinteger(L, 1);
  SDL_SetModState(mod);
  return 0;
}

static int key_name(lua_State* L) {
  SDL_Keycode key = (SDL_Keycode)luaL_checkinteger(L, 1);
  lua_pushstring(L, SDL_GetKeyName(key));
  return 1;
}


