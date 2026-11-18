
#ifndef LTIME_H_
#define LTIME_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_time.h>
#include <SDL3/SDL_timer.h>

const struct luaL_Reg lgame_time_module[];

void lgame_timer_map_quit(void);

#endif