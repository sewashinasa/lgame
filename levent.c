
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "levent.h"
#include "lconsts.h"

#define BUFEVENTSZ 128

static int Event_new(lua_State* L);
static int Event__tostring(lua_State* L);
static int Event__index(lua_State* L);
static int Event__newindex(lua_State* L);

static int event_pump(lua_State* L);
static int event_get(lua_State* L);
static int event_poll(lua_State* L);
static int event_wait(lua_State* L);
static int event_peek(lua_State* L);

const struct luaL_Reg lgame_event_module[] = {
  {"Event", Event_new},
  {"pump", event_pump},
  {"get", event_get},
  {"poll", event_poll},
  {"wait", event_wait},
  {"peek", event_peek},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Event_methods[] = {
  {NULL, NULL}
};

const struct luaL_Reg lgame_Event_metamethods[] = {
  {"__index", Event__index},
  {"__newindex", Event__newindex},
  {"__tostring", Event__tostring},
  { NULL, NULL }
};

static const char* get_event_name(Uint32 type);

void lgame_pushevent(lua_State* L) {
  lua_newuserdata(L, sizeof(void*));
  luaL_getmetatable(L, LGAME_EVENT_METATABLE);
  lua_setmetatable(L, -2);
  lua_newtable(L);
  lua_setuservalue(L, -2);
}

void lgame_pushnoevent(lua_State* L) {
  lgame_pushevent(L);
  lua_getuservalue(L, -1);
  lua_pushinteger(L, LGAME_NOEVENT);
  lua_setfield(L, -2, "type");
  lua_pop(L, 1);
}

void lgame_checkevent(lua_State* L, int idx) {
  luaL_checkudata(L, idx, LGAME_EVENT_METATABLE);
}

static void lgame_make_keyboard_event(lua_State* L, int evobj, SDL_Event* ev) {
  evobj = lua_absindex(L, evobj);
  lua_getuservalue(L, evobj);
  lua_pushinteger(L, ev->key.key);
  lua_setfield(L, -2, "key");
  lua_pushinteger(L, ev->key.mod);
  lua_setfield(L, -2, "mod");
  lua_pushinteger(L, ev->key.scancode);
  lua_setfield(L, -2, "scancode");
  lua_pop(L, 1);
}

static void lgame_make_mousemotion_event(lua_State* L, int evobj, SDL_Event* ev) {
  evobj = lua_absindex(L, evobj);
  lua_getuservalue(L, evobj);

  lua_newtable(L);
  lua_pushinteger(L, (lua_Integer)ev->motion.x);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, (lua_Integer)ev->motion.y);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "pos");

  lua_newtable(L);
  lua_pushinteger(L, (lua_Integer)ev->motion.xrel);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, (lua_Integer)ev->motion.yrel);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "rel");

  lua_newtable(L);
  lua_pushboolean(L, ev->motion.state & SDL_BUTTON_LMASK);
  lua_rawseti(L, -2, 1);
  lua_pushboolean(L, ev->motion.state & SDL_BUTTON_MMASK);
  lua_rawseti(L, -2, 2);
  lua_pushboolean(L, ev->motion.state & SDL_BUTTON_RMASK);
  lua_rawseti(L, -2, 3);
  lua_pushboolean(L, ev->motion.state & SDL_BUTTON_X1MASK);
  lua_rawseti(L, -2, 4);
  lua_pushboolean(L, ev->motion.state & SDL_BUTTON_X2MASK);
  lua_rawseti(L, -2, 5);
  lua_setfield(L, -2, "buttons");

  lua_pushboolean(L, ev->motion.which == SDL_TOUCH_MOUSEID);
  lua_setfield(L, -2, "touch");

  lua_pop(L, 1);
}

static void lgame_make_mousebutton_event(lua_State* L, int evobj, SDL_Event *ev) {
  evobj = lua_absindex(L, evobj);
  lua_getuservalue(L, evobj);

  lua_newtable(L);
  lua_pushinteger(L, (lua_Integer)ev->button.x);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, (lua_Integer)ev->button.y);
  lua_rawseti(L, -2, 2);
  lua_setfield(L, -2, "pos");

  lua_pushinteger(L, ev->button.button);
  lua_setfield(L, -2, "button");

  lua_pushboolean(L, ev->button.which == SDL_TOUCH_MOUSEID);
  lua_setfield(L, -2, "touch");

  lua_pop(L, 1);
}

static void lgame_make_mousewheel_event(lua_State* L, int evobj, SDL_Event* ev) {
  evobj = lua_absindex(L, evobj);
  lua_getuservalue(L, evobj);

  lua_pushboolean(L, ev->wheel.direction == SDL_MOUSEWHEEL_FLIPPED);
  lua_setfield(L, -2, "flipped");

  lua_pushinteger(L, (lua_Integer)ev->wheel.x);
  lua_setfield(L, -2, "x");

  lua_pushinteger(L, (lua_Integer)ev->wheel.y);
  lua_setfield(L, -2, "y");

  lua_pushinteger(L, (lua_Integer)ev->wheel.x);
  lua_setfield(L, -2, "preciseX");

  lua_pushinteger(L, (lua_Integer)ev->wheel.y);
  lua_setfield(L, -2, "preciseY");

  lua_pushboolean(L, ev->wheel.which == SDL_TOUCH_MOUSEID);
  lua_setfield(L, -2, "touch");

  lua_pop(L, 1);
}

static void lgame_make_event(lua_State* L, int evobj, SDL_Event *ev) {
  evobj = lua_absindex(L, evobj);
  lua_getuservalue(L, evobj);
  lua_pushinteger(L, ev->type);
  lua_setfield(L, -2, "type");
  lua_pop(L, 1);

  switch (ev->type) {
  case (LGAME_KEYDOWN):
  case (LGAME_KEYUP):
    lgame_make_keyboard_event(L, evobj, ev);
    break;
  case (LGAME_MOUSEMOTION):
    lgame_make_mousemotion_event(L, evobj, ev);
    break;
  case (LGAME_MOUSEBUTTONDOWN):
  case (LGAME_MOUSEBUTTONUP):
    lgame_make_mousebutton_event(L, evobj, ev);
    break;
  case (LGAME_MOUSEWHEEL):
    lgame_make_mousewheel_event(L, evobj, ev);
    break;
  }
}

static void checkpump(lua_State *L, int b) {
  int p = 1;
  if (!lua_isnoneornil(L, b))
    b = lua_toboolean(L, b);
  if (b)
    SDL_PumpEvents();
}

static int Event_new(lua_State* L) {
  int type = (int)luaL_checkinteger(L, 1);

  lgame_pushevent(L);
  lua_getuservalue(L, -1);
  lua_pushinteger(L, type);
  lua_setfield(L, -2, "type");
  lua_pop(L, 1);

  return 1;
}

static int Event__index(lua_State* L) {
  lgame_checkevent(L, 1);
  luaL_checkany(L, 2);
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_gettable(L, -2);
  return 1;
}

static int Event__newindex(lua_State* L) {
  const char* k = NULL;
  lgame_checkevent(L, 1);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  if (lua_type(L, 2) == LUA_TSTRING) {
    k = lua_tostring(L, 2);
    if (!strcmp(k, "type") && !lua_isinteger(L, 3)) {
      return luaL_error(L, "the type field should be an integer");
    }
  }
  lua_getuservalue(L, 1);
  lua_pushvalue(L, 2);
  lua_pushvalue(L, 3);
  lua_settable(L, -3);
  return 0;
}

static int Event__tostring(lua_State* L) {
  Uint32 type;
  const char* name;
  lgame_checkevent(L, 1);
  lua_getuservalue(L, -1);
  lua_getfield(L, -1, "type");
  type = (Uint32)lua_tointeger(L, -1);
  name = get_event_name(type);
  lua_pushfstring(L, "<Event(%d-%s)>", type, name);
  return 1;
}

static int event_pump(lua_State* L) {
  (void)L; 
  SDL_PumpEvents(); 
  return 0;
}

static int next_event(lua_State* L) {
  int uvi1 = lua_upvalueindex(1);
  int uvi2 = lua_upvalueindex(2);
  int i;
  lua_getfield(L, uvi1, "i");
  i = (int)lua_tointeger(L, -1);
  lua_pop(L, 1);
  lua_rawgeti(L, uvi2, i);
  if (lua_isnil(L, -1)) {
    return 0;
  }
  lua_pushinteger(L, ++i);
  lua_setfield(L, uvi1, "i");
  return 1;
}

static int event_get(lua_State* L) {
  lua_settop(L, 2);
  int nevents;
  int tidx;
  SDL_Event events[BUFEVENTSZ];
  if (!lua_isnoneornil(L, 1)) 
    luaL_checktype(L, 1, LUA_TTABLE);
  checkpump(L, 2);
  lua_newtable(L);
  tidx = lua_gettop(L);
  if (lua_isnoneornil(L, 1)) {
    nevents = SDL_PeepEvents(events, BUFEVENTSZ, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    for (int i = 0; i < nevents; i++) {
      lgame_pushevent(L);
      lgame_make_event(L, -1, &events[i]);
      lua_rawseti(L, tidx, (lua_Integer)(i + 1));
    }
  }
  else {
    Uint32 type;
    int tlen = (int)lua_rawlen(L, 1);
    for (int i = 0; i < tlen; i++) {
      lua_rawgeti(L, 1, (lua_Integer)(i + 1));
      type = (Uint32)luaL_checkinteger(L, -1);
      lua_pop(L, 1);
      nevents = SDL_PeepEvents(events, BUFEVENTSZ, SDL_GETEVENT, type, type);
      for (int j = 0; j < nevents; j++) {
        lgame_pushevent(L);
        lgame_make_event(L, -1, &events[j]);
        lua_rawseti(L, tidx, (lua_Integer)(j + 1));
      }
    }
  }

  lua_newtable(L);
  lua_pushinteger(L, 1);
  lua_setfield(L, -2, "i");
  lua_pushvalue(L, tidx);
  lua_pushcclosure(L, next_event, 2);
  return 1;
}

static int event_poll(lua_State* L) {
  SDL_Event ev;
  if (!SDL_PollEvent(&ev)) {
    lgame_pushnoevent(L);
    return 1;
  }
  lgame_pushevent(L);
  lgame_make_event(L, -1, &ev);
  return 1;
}

static int event_wait(lua_State* L) {
  SDL_Event ev;
  Sint32 timeout = (Sint32)luaL_optinteger(L, 1, 0);
  if (timeout > 0) {
    if (!SDL_WaitEventTimeout(&ev, timeout)) {
      lgame_pushnoevent(L);
      return 1;
    }
    lgame_pushevent(L);
    lgame_make_event(L, -1, &ev);
  }
  else {
    if (!SDL_WaitEvent(&ev)) {
      lgame_pushnoevent(L);
      return 1;
    }
    lgame_pushevent(L);
    lgame_make_event(L, -1, &ev);
  }
  return 1;
}

static int event_peek(lua_State* L) {
  lua_settop(L, 2);
  int nevents;
  int hasevs = 0;
  SDL_Event events[BUFEVENTSZ];
  if (!lua_isnoneornil(L, 1))
    luaL_checktype(L, 1, LUA_TTABLE);
  checkpump(L, 2);
  if (lua_isnoneornil(L, 1)) {
    nevents = SDL_PeepEvents(events, BUFEVENTSZ, SDL_PEEKEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST);
    if (nevents > 0) hasevs = 1;
  }
  else {
    Uint32 type;
    int tlen = (int)lua_rawlen(L, 1);
    for (int i = 0; i < tlen; i++) {
      lua_rawgeti(L, 1, (lua_Integer)(i + 1));
      type = (Uint32)luaL_checkinteger(L, -1);
      lua_pop(L, 1);
      nevents = SDL_PeepEvents(events, BUFEVENTSZ, SDL_PEEKEVENT, type, type);
      if (nevents > 0) {
        hasevs = 1;
        break;
      }
    }
  }
  
  lua_pushboolean(L, hasevs);
  return 1;
}

static const char* get_event_name(Uint32 type) {
  switch (type) {
  case (LGAME_QUIT):
    return "Quit";
  case (LGAME_KEYDOWN):
    return "KeyDown";
  case (LGAME_KEYUP):
    return "KeyUp";
  case (LGAME_MOUSEBUTTONDOWN):
    return "MouseButtonDown";
  case (LGAME_MOUSEBUTTONUP):
    return "MouseButtonUp";
  case (LGAME_MOUSEMOTION):
    return "MouseMotion";
  case (LGAME_MOUSEWHEEL):
    return "MouseWheel";
  default:
    return "Unknown";
  }
}

static int event_event_name(lua_State* L) {
  lua_pushstring(L, get_event_name((Uint32)luaL_checkinteger(L, 1)));
  return 1;
}