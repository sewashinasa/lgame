
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lcolor.h"


#define chkrgbval(v) \
  if ((v) < 0 || (v) > 255) \
    luaL_error(L, "invalid color value: expected 0-255, got %d", (int)(v));

static int Color_new(lua_State * L);
static int Color__tostring(lua_State * L);
static int Color__index(lua_State * L);
static int Color__newindex(lua_State * L);

const struct luaL_Reg lg_Color_module[] = {
  {"Color", Color_new},
  {NULL, NULL}
};

const struct luaL_Reg lg_Color_methods[] = {
  {NULL, NULL}
};

const struct luaL_Reg lg_Color_metamethods[] = {
  {"__tostring", Color__tostring},
  {"__index", Color__index},
  {"__newindex", Color__newindex},
  { NULL, NULL }
};

lColor* lg_pushcolor(lua_State* L) {
  lColor* c;
  c = (lColor*)lua_newuserdata(L, sizeof(lColor));
  c->r = 0; c->g = 0; c->b = 0; c->a = 0;
  luaL_getmetatable(L, LGAME_COLOR_METATABLE);
  lua_setmetatable(L, -2);
  return c;
}

lColor* lg_checkcolor(lua_State* L, int idx) {
  return (lColor*)luaL_checkudata(L, idx, LGAME_COLOR_METATABLE);
}

lColor lg_checkcolortable(lua_State* L, int idx) {
  lua_Integer r, g, b, a;
  idx = lua_absindex(L, idx);
  lColor c;
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_rawgeti(L, idx, 1);
  r = luaL_checkinteger(L, -1);
  chkrgbval(r);
  lua_pop(L, 1);
  lua_rawgeti(L, idx, 2);
  g = luaL_checkinteger(L, -1);
  chkrgbval(g);
  lua_pop(L, 1);
  lua_rawgeti(L, idx, 3);
  b = luaL_checkinteger(L, -1);
  chkrgbval(b);
  lua_pop(L, 1);
  lua_rawgeti(L, idx, 4);
  a = luaL_optinteger(L, -1, 255);
  chkrgbval(a);
  lua_pop(L, 1);

  c.r = (Uint8)r;
  c.g = (Uint8)g;
  c.b = (Uint8)b;
  c.a = (Uint8)a;

  return c;
}

lColor lg_unpack32(Uint32 color) {
  lColor c;
  c.r = (Uint8)((color >> 24) & 0xFF);
  c.g = (Uint8)((color >> 16) & 0xFF);
  c.b = (Uint8)((color >> 8) & 0xFF);
  c.a = (Uint8)(color & 0xFF);
  return c;
}

lColor lg_checkcolorarg(lua_State* L, int idx) {
  idx = lua_absindex(L, idx);
  lColor c;
  if (lua_type(L, idx) == LUA_TTABLE)
    c = lg_checkcolortable(L, idx);
  else if (lua_type(L, idx) == LUA_TNUMBER)
    c = lg_unpack32((Uint32)luaL_checkinteger(L, idx));
  else if (lua_type(L, idx) == LUA_TUSERDATA)
    c = *lg_checkcolor(L, idx);
  else
    luaL_error(L, "invalid argument type for lgame.Color: expected table or lgame.Color, got %s", luaL_typename(L, idx));
  return c;
}

lColor lg_checkcolorfromargs(lua_State* L, int offset) {
  lColor c;
  int top = lua_gettop(L) - offset;
  if (top == 1) {
    c = lg_checkcolorarg(L, 1 + offset);
  }
  else if (top == 3 || top == 4) {
    lua_Integer r, g, b, a;
    r = luaL_checkinteger(L, 1 + offset);
    chkrgbval(r);
    g = luaL_checkinteger(L, 2 + offset);
    chkrgbval(g);
    b = luaL_checkinteger(L, 3 + offset);
    chkrgbval(b);
    if (top == 4) {
      a = luaL_checkinteger(L, 4 + offset);
      chkrgbval(a);
    }
    else {
      a = 255;
    }

    c.r = (Uint8)r;
    c.g = (Uint8)g;
    c.b = (Uint8)b;
    c.a = (Uint8)a;
  }
  else {
    luaL_error(L, "invalid number of arguments for lgame.Color: expected 1, 3 or 4, got %d", top);
  }
  return c;
}

static int Color_new(lua_State* L) {
  lColor color = lg_checkcolorfromargs0(L);
  lColor* self = lg_pushcolor(L);
  *self = color;
  return 1;
}

static int Color__tostring(lua_State* L) {
  lColor* self = lg_checkcolor(L, 1);
  lua_pushfstring(L, "lgame.Color(%d, %d, %d, %d)", self->r, self->g, self->b, self->a);
  return 1;
}

static int Color__index(lua_State* L) {
  lColor* self = lg_checkcolor(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'r': lua_pushinteger(L, self->r); return 1;
    case 'g': lua_pushinteger(L, self->g); return 1;
    case 'b': lua_pushinteger(L, self->b); return 1;
    case 'a': lua_pushinteger(L, self->a); return 1;
    default:
      return luaL_error(L, "attempt to index lgame.Color with invalid key '%s'", key);
    }
  }
  return luaL_error(L, "attempt to index lgame.Color with non-string key");
}

static int Color__newindex(lua_State* L) {
  lColor* self = lg_checkcolor(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'r': self->r = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'g': self->g = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'b': self->b = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'a': self->a = (Uint8)luaL_checkinteger(L, 3); return 0;
    default:
      return luaL_error(L, "attempt to index lgame.Color with invalid key '%s'", key);
    }
  }
  return luaL_error(L, "attempt to index lgame.Color with non-string key");
}