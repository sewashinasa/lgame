
#ifndef LEVENT_H_
#define LEVENT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_events.h>

#define LGAME_EVENT_METATABLE "lgame.Event"

extern const struct luaL_Reg lgame_event_module[];
extern const struct luaL_Reg lgame_Event_methods[];
extern const struct luaL_Reg lgame_Event_metamethods[];

void lgame_pushevent(lua_State* L);
void lgame_pushnoevent(lua_State* L);
void lgame_checkevent(lua_State* L, int idx);

#endif