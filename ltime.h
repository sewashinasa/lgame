
#ifndef LTIME_H_
#define LTIME_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_time.h>
#include <SDL3/SDL_timer.h>

#define LGAME_CLOCK_METATABLE "lgame.Clock"

typedef struct lgame_Clock {
  Sint64 fps_tick;
  Sint64 last_tick;
  float fps;
  int fps_count;
  Sint64 raw_passed;
  Sint64 time_passed;
} lgame_Clock;

const struct luaL_Reg lgame_time_module[];

const struct luaL_Reg lgame_Clock_methods[];
const struct luaL_Reg lgame_Clock_metamethods[];

lgame_Clock* lgame_pushclock(lua_State* L);
lgame_Clock* lgame_checkclock(lua_State* L, int idx);

void lgame_timer_map_quit(void);

#endif