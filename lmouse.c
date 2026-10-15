
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lmouse.h"
#include "lpoint.h"


static int mouse_get_pressed(lua_State* L);
static int mouse_get_pos(lua_State* L);
static int mouse_get_rel(lua_State* L);
static int mouse_set_pos(lua_State* L);

const struct luaL_Reg lgame_mouse_module[] = {
  {"get_pressed", mouse_get_pressed},
  {"get_pos", mouse_get_pos},
  {"get_rel", mouse_get_rel},
  {"set_pos", mouse_set_pos},
  {NULL, NULL}
};

static int mouse_get_pressed(lua_State* L) {
  SDL_MouseButtonFlags mbf;
  int nbts = (int)luaL_optinteger(L, 1, 3);
  int desktop = lua_toboolean(L, 2);
  if (nbts != 3 && nbts != 5)
    return luaL_error(L, "the number of buttons should be 3 or 5");
  if (desktop)
    mbf = SDL_GetGlobalMouseState(NULL, NULL);
  else
    mbf = SDL_GetMouseState(NULL, NULL);
  lua_newtable(L);
  lua_pushboolean(L, mbf & SDL_BUTTON_LMASK);
  lua_rawseti(L, -2, 1);
  lua_pushboolean(L, mbf & SDL_BUTTON_MMASK);
  lua_rawseti(L, -2, 2);
  lua_pushboolean(L, mbf & SDL_BUTTON_RMASK);
  lua_rawseti(L, -2, 3);
  if (nbts == 5) {
    lua_pushboolean(L, mbf & SDL_BUTTON_X1MASK);
    lua_rawseti(L, -2, 4);
    lua_pushboolean(L, mbf & SDL_BUTTON_X2MASK);
    lua_rawseti(L, -2, 5);
  }
  return 1;
}

static int mouse_get_pos(lua_State* L) {
  int desktop = lua_toboolean(L, 1);
  float xpos, ypos;
  if (desktop)
    SDL_GetGlobalMouseState(&xpos, &ypos);
  else
    SDL_GetMouseState(&xpos, &ypos);
  lgame_pushpoint2int(L, (int)xpos, (int)ypos);
  return 1;
}

static int mouse_get_rel(lua_State* L) {
  float xrel, yrel;
  SDL_GetRelativeMouseState(&xrel, &yrel);
  lgame_pushpoint2int(L, (int)xrel, (int)yrel);
  return 1;
}

static int mouse_set_pos(lua_State* L) {
  int xpos, ypos;
  lgame_checkpoint2intfromargs0(L, &xpos, &ypos);
  SDL_WarpMouseInWindow(NULL, (float)xpos, (float)ypos);
  return 0;
}