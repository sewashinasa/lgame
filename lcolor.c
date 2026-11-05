
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lcolor.h"
#include "lnamedcolors.h"

#define chkrgbval(v) \
  if ((v) < 0 || (v) > 255) \
    luaL_error(L, "invalid color value: expected 0-255, got %d", (int)(v));

static int Color_new(lua_State * L);
static int Color__tostring(lua_State * L);
static int Color__index(lua_State * L);
static int Color__newindex(lua_State * L);

const struct luaL_Reg lgame_color_module[] = {
  {"Color", Color_new},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Color_methods[] = {
  {NULL, NULL}
};

const struct luaL_Reg lgame_Color_metamethods[] = {
  {"__tostring", Color__tostring},
  {"__index", Color__index},
  {"__newindex", Color__newindex},
  { NULL, NULL }
};

lgame_Color* lgame_pushcolor(lua_State* L) {
  lgame_Color* c;
  c = (lgame_Color*)lua_newuserdata(L, sizeof(lgame_Color));
  c->r = 0; c->g = 0; c->b = 0; c->a = 0;
  luaL_getmetatable(L, LGAME_COLOR_METATABLE);
  lua_setmetatable(L, -2);
  return c;
}

lgame_Color* lgame_checkcolor(lua_State* L, int idx) {
  return (lgame_Color*)luaL_checkudata(L, idx, LGAME_COLOR_METATABLE);
}

lgame_Color lgame_checkcolortable(lua_State* L, int idx) {
  lua_Integer r, g, b, a;
  idx = lua_absindex(L, idx);
  lgame_Color c;
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

lgame_Color lgame_unpack32(Uint32 color) {
  lgame_Color c;
  c.r = (Uint8)((color >> 24) & 0xFF);
  c.g = (Uint8)((color >> 16) & 0xFF);
  c.b = (Uint8)((color >> 8) & 0xFF);
  c.a = (Uint8)(color & 0xFF);
  return c;
}

static int hex2num(const char* hex) {
  int res = 0;
  int dec = 0;
  for (int i = 0; i < strlen(hex); i++) {
    switch (hex[i]) {
    case ('0'): dec = 0; break;
    case ('1'): dec = 1; break;
    case ('2'): dec = 2; break;
    case ('3'): dec = 3; break;
    case ('4'): dec = 4; break;
    case ('5'): dec = 5; break;
    case ('6'): dec = 6; break;
    case ('7'): dec = 7; break;
    case ('8'): dec = 8; break;
    case ('9'): dec = 9; break;
    case ('a'): case ('A'): dec = 10; break;
    case ('b'): case ('B'): dec = 11; break;
    case ('c'): case ('C'): dec = 12; break;
    case ('d'): case ('D'): dec = 13; break;
    case ('e'): case ('E'): dec = 14; break;
    case ('f'): case ('F'): dec = 15; break;
    default: return -1;
    }
    res += dec * (int)SDL_pow(16., (double)(strlen(hex) - i - 1));
  }
  return res;
}

#define ISHEXPREFIX(s) ((s[0] == '#') || (s[0] == '0' && s[1] == 'x'))
#define ISHEX_0X(s) ((s[0] == '0') && (s[1] == 'x'))
#define ISHEX_HASH(s) ((s[0] == '#'))

lgame_Color lgame_checkcolorarg(lua_State* L, int idx) {
  idx = lua_absindex(L, idx);
  lgame_Color c;
  if (lua_type(L, idx) == LUA_TTABLE)
    c = lgame_checkcolortable(L, idx);
  else if (lua_type(L, idx) == LUA_TSTRING) {
    const char* stringval = lua_tostring(L, idx);
    if (ISHEXPREFIX(stringval)) {
      int colorval; 
      int hasalpha;
      int stringvallen;

      if (ISHEX_0X(stringval))
        stringval += 2;
      else if (ISHEX_HASH(stringval))
        stringval += 1;

      stringvallen = (int)strlen(stringval);

      if (stringvallen != 8 && stringvallen != 6)
        luaL_error(L, "invalid hex color length");

      if (stringvallen > 6)
        hasalpha = 1;
      else
        hasalpha = 0;

      colorval = hex2num(stringval);

      if (hasalpha) {
        c.r = (Uint8)((colorval >> 24) & 0xFF);
        c.g = (Uint8)((colorval >> 16) & 0xFF);
        c.b = (Uint8)((colorval >> 8) & 0xFF);
        c.a = (Uint8)(colorval & 0xFF);
      }
      else {
        c.r = (Uint8)((colorval >> 16) & 0xFF);
        c.g = (Uint8)((colorval >> 8) & 0xFF);
        c.b = (Uint8)(colorval & 0xFF);
        c.a = 255;
      }
    }
    else {
      const SDL_Color* color = lgame_getcolorfromname(stringval);
      if (color) {
        c.r = color->r;
        c.g = color->g;
        c.b = color->b;
        c.a = color->a;
      }
      else {
        luaL_error(L, "unknown color name");
      }
    }
  }
  else if (lua_type(L, idx) == LUA_TNUMBER)
    c = lgame_unpack32((Uint32)luaL_checkinteger(L, idx));
  else if (lua_type(L, idx) == LUA_TUSERDATA)
    c = *lgame_checkcolor(L, idx);
  else
    luaL_typeerror(L, idx, "colorlike");
  return c;
}

lgame_Color lgame_checkcolorfromargs(lua_State* L, int offset) {
  lgame_Color c;
  int top = lua_gettop(L) - offset;
  if (top == 1) {
    c = lgame_checkcolorarg(L, 1 + offset);
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
    luaL_error(L, "invalid number of arguments");
  }
  return c;
}

static int Color_new(lua_State* L) {
  lgame_Color color = lgame_checkcolorfromargs0(L);
  lgame_Color* self = lgame_pushcolor(L);
  *self = color;
  return 1;
}

static int Color__tostring(lua_State* L) {
  lgame_Color* self = lgame_checkcolor(L, 1);
  lua_pushfstring(L, "(%d, %d, %d, %d)", self->r, self->g, self->b, self->a);
  return 1;
}

static int Color__index(lua_State* L) {
  lgame_Color* self = lgame_checkcolor(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'r': lua_pushinteger(L, self->r); return 1;
    case 'g': lua_pushinteger(L, self->g); return 1;
    case 'b': lua_pushinteger(L, self->b); return 1;
    case 'a': lua_pushinteger(L, self->a); return 1;
    default:
      return luaL_error(L, "invalid '%s' key", key);
    }
  }
  return luaL_typeerror(L, 2, "string");
}

static int Color__newindex(lua_State* L) {
  lgame_Color* self = lgame_checkcolor(L, 1);
  if (lua_type(L, 2) == LUA_TSTRING) {
    const char* key = lua_tostring(L, 2);
    char fc = key[0];
    switch (fc) {
    case 'r': self->r = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'g': self->g = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'b': self->b = (Uint8)luaL_checkinteger(L, 3); return 0;
    case 'a': self->a = (Uint8)luaL_checkinteger(L, 3); return 0;
    default:
      return luaL_error(L, "invalid '%s' key", key);
    }
  }
  return luaL_typeerror(L, 2, "string");
}