
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ltime.h"
#include "levent.h"

static int time_get_ticks(lua_State* L);
static int time_wait(lua_State* L);
static int time_delay(lua_State* L);

const struct luaL_Reg lgame_time_module[] = {
  {"get_ticks", time_get_ticks},
  {"wait", time_wait},
  {"delay", time_delay},
  {NULL, NULL}
};

static int time_get_ticks(lua_State* L) {
  lua_pushinteger(L, SDL_GetTicks());
  return 1;
}

static int time_wait(lua_State* L) {
  Uint32 ms = (Uint32)luaL_checkinteger(L, 1);
  Uint64 start_time = SDL_GetTicks();
  SDL_Delay(ms);
  lua_pushinteger(L, SDL_GetTicks() - start_time);
  return 0;
}

static int time_delay(lua_State* L) {
  return time_wait(L);
}

