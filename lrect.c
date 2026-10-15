
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lrect.h"


static int Rect_new(lua_State* L);
static int Rect__tostring(lua_State* L);
static int Rect__index(lua_State* L);
static int Rect__newindex(lua_State* L);

const struct luaL_Reg lgame_rect_module[] = {
  {"Rect", Rect_new},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Rect_methods[] = {
  {NULL, NULL}
};

const struct luaL_Reg lgame_Rect_metamethods[] = {
  { "__tostring", Rect__tostring  },
  { "__index",    Rect__index     },
  { "__newindex", Rect__newindex  },
  { NULL, NULL }
};

lgame_Rect *lgame_pushrect(lua_State* L) {
  lgame_Rect* r;
  r = (lgame_Rect*)lua_newuserdata(L, sizeof(lgame_Rect));
  r->x = 0; r->y = 0; r->w = 0; r->h = 0;
  luaL_getmetatable(L, LGAME_RECT_METATABLE);
  lua_setmetatable(L, -2);
  return r;
}

lgame_Rect* lgame_checkrect(lua_State* L, int idx) {
  return (lgame_Rect*)luaL_checkudata(L, idx, LGAME_RECT_METATABLE);
}

void lgame_pushpoint(lua_State* L, lgame_Point p) {
  lua_newtable(L);
  lua_pushinteger(L, p.x);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, p.y);
  lua_rawseti(L, -2, 2);
}

void lgame_pushpoint2int(lua_State* L, int x, int y) {
  lgame_Point p; p.x = x; p.y = y;
  lgame_pushpoint(L, p);
}

lgame_Point lgame_checkpoint(lua_State* L, int idx) {
  idx = lua_absindex(L, idx);
  lgame_Point p;
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_rawgeti(L, idx, 1);
  p.x = (int)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  lua_rawgeti(L, idx, 2);
  p.y = (int)luaL_checkinteger(L, -1);
  lua_pop(L, 1);
  return p;
}

void lgame_checkpoint2int(lua_State* L, int idx, int* x, int* y) {
  lgame_Point p = lgame_checkpoint(L, idx);
  if (x) *x = p.x;
  if (y) *y = p.y;
}

lgame_Rect lgame_checkrecttable(lua_State* L, int idx) {
  idx = lua_absindex(L, idx);
  lgame_Rect r;
  lua_Unsigned tl;
  luaL_checktype(L, idx, LUA_TTABLE);
  tl = lua_rawlen(L, idx);
  if (tl == 2) {
    lgame_Point left, right;
    lua_rawgeti(L, idx, 1);
    left = lgame_checkpoint(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, idx, 2);
    right = lgame_checkpoint(L, -1);
    lua_pop(L, 1);
    r.x = left.x; r.y = left.y;
    r.w = right.x; r.h = right.y;
  }
  else if (tl == 4) {
    lua_rawgeti(L, idx, 1);
    r.x = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, idx, 2);
    r.y = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, idx, 3);
    r.w = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, idx, 4);
    r.h = (int)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
  }
  else
    luaL_error(L, "invalid table length for lgame.Rect: expected 2 or 4, got %d", (int)tl);

  return r;
}

lgame_Rect lgame_checkrectarg(lua_State* L, int idx) {
  idx = lua_absindex(L, idx);
  lgame_Rect r;
  if (lua_type(L, idx) == LUA_TTABLE) 
    r = lgame_checkrecttable(L, idx);
  else if (lua_type(L, idx) == LUA_TUSERDATA) 
    r = *lgame_checkrect(L, idx);
  else 
    luaL_error(L, "invalid argument type for lgame.Rect: expected table or lgame.Rect, got %s", luaL_typename(L, idx));
  return r;
}

lgame_Rect lgame_checkrectfromargs(lua_State* L, int offset) {
  lgame_Rect r;
  int top = lua_gettop(L) - offset;
  if (top == 1) {
    r = lgame_checkrectarg(L, 1+offset);
  }
  else if (top == 2) {
    lgame_Point left = lgame_checkpoint(L, 1+offset);
    lgame_Point right = lgame_checkpoint(L, 2+offset);
    r.x = left.x; r.y = left.y;
    r.w = right.x; r.h = right.y;
  }
  else if (top == 4) {
    r.x = (int)luaL_checkinteger(L, 1+offset);
    r.y = (int)luaL_checkinteger(L, 2+offset);
    r.w = (int)luaL_checkinteger(L, 3+offset);
    r.h = (int)luaL_checkinteger(L, 4+offset);
  }
  else {
    luaL_error(L, "invalid number of arguments for lgame.Rect: expected 1, 2 or 4, got %d", top);
  }
  return r;
}

static int Rect_new(lua_State* L) {
  lgame_Rect rect = lgame_checkrectfromargs0(L);
  lgame_Rect* self = lgame_pushrect(L);
  *self = rect;
  return 1;
}

static int Rect__tostring(lua_State* L) {
  lgame_Rect* self = lgame_checkrect(L, 1);
  lua_pushfstring(L, "<Rect(%d, %d, %d, %d)>",
    self->x, self->y, self->w, self->h);
  return 1;
}

static int Rect__index(lua_State* L) {
  lgame_Rect* self = lgame_checkrect(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'x': lua_pushinteger(L, self->x); return 1;
    case 'y': lua_pushinteger(L, self->y); return 1;
    case 'w': lua_pushinteger(L, self->w); return 1;
    case 'h': lua_pushinteger(L, self->h); return 1;
    default:      
      return luaL_error(L, "attempt to index lgame.Rect with invalid key '%s'", key);
    }
  }
  return luaL_error(L, "attempt to index lgame.Rect with non-string key");
}

static int Rect__newindex(lua_State* L) {
  lgame_Rect* self = lgame_checkrect(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'x': self->x = (int)luaL_checkinteger(L, 3); return 0;
    case 'y': self->y = (int)luaL_checkinteger(L, 3); return 0;
    case 'w': self->w = (int)luaL_checkinteger(L, 3); return 0;
    case 'h': self->h = (int)luaL_checkinteger(L, 3); return 0;
    default:
      return luaL_error(L, "attempt to index lgame.Rect with invalid key '%s'", key);
    }
  }
  return luaL_error(L, "attempt to index lgame.Rect with non-string key");
}