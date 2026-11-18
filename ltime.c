
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ltime.h"
#include "levent.h"
#include "lconsts.h"

#define TIMER_MAP_MAX 128

static int time_get_ticks(lua_State* L);
static int time_wait(lua_State* L);
static int time_delay(lua_State* L);
static int time_set_timer(lua_State* L);

const struct luaL_Reg lgame_time_module[] = {
  {"get_ticks", time_get_ticks},
  {"wait", time_wait},
  {"delay", time_delay},
  {"set_timer", time_set_timer},
  {NULL, NULL}
};

static int timer_map_initialized = 0;
struct TimerMap {
  Uint32 event;
  SDL_TimerID timer_id;
} timer_map[TIMER_MAP_MAX];

void lgame_timer_map_quit(void) {
  timer_map_initialized = 0;
}

static void timer_map_clear(void) {
  for (int i = 0; i < TIMER_MAP_MAX; i++) {
    if (timer_map[i].timer_id != 0) {
      SDL_RemoveTimer(timer_map[i].timer_id);
    }
    timer_map[i].event = 0;
    timer_map[i].timer_id = 0;
  }
}

static int timer_map_event_busy(Uint32 event) {
  for (int i = 0; i < TIMER_MAP_MAX; i++) {
    if (timer_map[i].event == event && timer_map[i].timer_id != 0) {
      return 1;
    }
  }
  return 0;
}

static int timer_map_get_event_index(Uint32 event) {
  for (int i = 0; i < TIMER_MAP_MAX; i++) {
    if (timer_map[i].event == event) {
      return i;
    }
  }
  return -1;
}

static int timer_map_find_free_slot(void) {
  for (int i = 0; i < TIMER_MAP_MAX; i++) {
    if (timer_map[i].event == 0) {
      return i;
    }
  }
  
  return -1;
}

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

static Uint32 timer_callback(void* userdata, SDL_TimerID id, Uint32 interval) {
  (void)id;
  Uint32 type = (Uint32)(uintptr_t)userdata;
  SDL_Event event;
  SDL_zero(event);
  event.type = type;
  SDL_PushEvent(&event);
  return interval;
}

static int time_set_timer(lua_State* L) {
  if (!timer_map_initialized) {
    timer_map_clear();
    timer_map_initialized = 1;
  }

  Uint32 type = (Uint32)luaL_checkinteger(L, 1);
  Uint32 ms = (Uint32)luaL_checkinteger(L, 2);

  luaL_argcheck(L, type != LGAME_NOEVENT, 1, "invalid event type");

  if (ms > 0) {
    int index;
    if (timer_map_event_busy(type)) {
      index = timer_map_get_event_index(type);
      SDL_TimerID last_id = timer_map[index].timer_id;
      SDL_RemoveTimer(last_id);
    }
    else {
      index = timer_map_find_free_slot();
      if (index == -1)
        return luaL_error(L, "too many timers have been added");
    }

    SDL_TimerID id = SDL_AddTimer(ms, timer_callback, (void*)(uintptr_t)type);
    if (id == 0)
      return luaL_error(L, SDL_GetError());
    
    timer_map[index].event = type;
    timer_map[index].timer_id = id;
  }
  else {
    if (!timer_map_event_busy(type)) {
      return 0;
    }

    int index = timer_map_get_event_index(type);
    SDL_RemoveTimer(timer_map[index].timer_id);
    timer_map[index].timer_id = 0;
    timer_map[index].event = 0;
  }

  return 0;
}