
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ltime.h"
#include "levent.h"
#include "lmethod.h"
#include "lconsts.h"

#define TIMER_MAP_MAX 128

static int time_get_ticks(lua_State* L);
static int time_wait(lua_State* L);
static int time_delay(lua_State* L);
static int time_set_timer(lua_State* L);

static int Clock_new(lua_State* L);
static int Clock_tick(lua_State* L);
static int Clock_tick_busy_loop(lua_State* L);
static int Clock_get_time(lua_State* L);
static int Clock_get_rawtime(lua_State* L);
static int Clock_get_fps(lua_State* L);
static int Clock__index(lua_State* L);
static int Clock__tostring(lua_State* L);

const struct luaL_Reg lgame_time_module[] = {
  {"get_ticks", time_get_ticks},
  {"wait", time_wait},
  {"delay", time_delay},
  {"set_timer", time_set_timer},
  {"Clock", Clock_new},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Clock_methods[] = {
  {"tick", Clock_tick},
  {"tick_busy_loop", Clock_tick_busy_loop},
  {"get_time", Clock_get_time},
  {"get_rawtime", Clock_get_rawtime},
  {"get_fps", Clock_get_fps},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Clock_metamethods[] = {
  {"__index", Clock__index},
  {"__tostring", Clock__tostring},
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

lgame_Clock* lgame_pushclock(lua_State* L) {
  lgame_Clock* clock = lua_newuserdata(L, sizeof(lgame_Clock));
  clock->fps = 0.0f;
  clock->fps_count = 0;
  clock->fps_tick = 0;
  clock->last_tick = 0;
  clock->raw_passed = 0;
  clock->time_passed = 0;
  luaL_getmetatable(L, LGAME_CLOCK_METATABLE);
  lua_setmetatable(L, -2);
  return clock;
}

lgame_Clock* lgame_checkclock(lua_State* L, int idx) {
  return luaL_checkudata(L, idx, LGAME_CLOCK_METATABLE);
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

static int Clock_new(lua_State* L) {
  lgame_pushclock(L)->last_tick = SDL_GetTicks();
  return 1;
}

static Sint64 Clock__tick_core(lgame_Clock *self, float frame_rate) {
  if (frame_rate > .0f) {
    Sint64 end_time, delay_time;
    end_time = (Sint64)(1000.f / frame_rate);
    self->raw_passed = (Sint64)(SDL_GetTicks() - self->last_tick);
    delay_time = end_time - self->raw_passed;
    delay_time = delay_time > 0 ? delay_time : 0;
    SDL_Delay((Uint32)delay_time);
  }

  Uint64 now = SDL_GetTicks();
  self->time_passed = (Sint64)(now - self->last_tick);
  self->fps_count++;
  self->last_tick = now;

  if (frame_rate <= .0f)
    self->raw_passed = self->time_passed;

  if (!self->fps_tick) {
    self->fps_count = 0;
    self->fps_tick = now;
  } 
  else if (self->fps_count >= 10) {
    float t = (now - self->fps_tick) / 1000.0f;
    if (t == .0f)
      self->fps = 0.f;
    else
      self->fps = (float)(self->fps_count / t);
    self->fps_count = 0;
    self->fps_tick = now;
  }

  return (Sint64)self->time_passed;
}

static int Clock_tick(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  float frame_rate = (float)luaL_optnumber(L, 2, 0);
  lua_pushinteger(L, Clock__tick_core(self, frame_rate));
  return 1;
}

static int Clock_tick_busy_loop(lua_State* L) {
  return Clock_tick(L);
}

static int Clock_get_time(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  lua_pushinteger(L, self->time_passed);
  return 1;
}

static int Clock_get_rawtime(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  lua_pushinteger(L, self->raw_passed);
  return 1;
}

static int Clock_get_fps(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  lua_pushnumber(L, self->fps);
  return 1;
}

static int Clock__index(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  const char* key = luaL_checkstring(L, 2);
  lgame_getmethod(L, 1, key);
  return 1;
}

static int Clock__tostring(lua_State* L) {
  lgame_Clock* self = lgame_checkclock(L, 1);
  lua_pushfstring(L, "<Clock(fps=%.2f)>", self->fps);
  return 1;
}