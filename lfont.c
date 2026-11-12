
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lfont.h"
#include "lcolor.h"
#include "lsurface.h"
#include "lmethod.h"
#include "lcheckfile.h"

#define OPENED_FONTS_FIELD "_OPENED_FONTS"

#define SET_STYLE(lfont, enabled, flag) \
  do { \
    if (enabled) { \
        (lfont)->style |= flag; \
    } \
    else { \
        (lfont)->style &= ~flag; \
    } \
    TTF_SetFontStyle((lfont)->font, (lfont)->style); \
  } while (0)

#define GET_STYLE(lfont, flag) \
  TTF_GetFontStyle((lfont)->font) & (flag)

static int font_init(lua_State * L);
static int font_quit(lua_State * L);
static int font_get_init(lua_State * L);
static int font_get_sdl_ttf_version(lua_State * L);

static int Font_new(lua_State * L);
static int Font_render(lua_State * L);
static int Font_size(lua_State * L);
static int Font_set_underline(lua_State * L);
static int Font_get_underline(lua_State * L);
static int Font_set_strikethrough(lua_State * L);
static int Font_get_strikethrough(lua_State * L);
static int Font_set_bold(lua_State * L);
static int Font_get_bold(lua_State * L);
static int Font_set_italic(lua_State * L);
static int Font_get_italic(lua_State * L);
static int Font_metrics(lua_State * L);
static int Font__index(lua_State * L);
static int Font__newindex(lua_State * L);
static int Font__gc(lua_State * L);

const struct luaL_Reg lgame_font_module[] = {
  {"init", font_init},
  {"quit", font_quit},
  {"get_init", font_get_init},
  {"get_sdl_ttf_version", font_get_sdl_ttf_version},
  {"Font", Font_new},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Font_methods[] = {
  {"render", Font_render},
  {"size", Font_size},
  {"set_underline", Font_set_underline},
  {"get_underline", Font_get_underline},
  {"set_strikethrough", Font_set_strikethrough},
  {"get_strikethrough", Font_get_strikethrough},
  {"set_bold", Font_set_bold},
  {"get_bold", Font_get_bold},
  {"set_italic", Font_set_italic},
  {"get_italic", Font_get_italic},
  {"metrics", Font_metrics},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Font_metamethods[] = {
  {"__index", Font__index},
  {"__newindex", Font__newindex},
  {"__gc", Font__gc},
  {NULL, NULL}
};

static int lgame_font_initialized = 0;

lgame_Font* lgame_pushfont(lua_State* L) {
  lgame_Font* font;
  font = lua_newuserdata(L, sizeof(lgame_Font));
  font->font = NULL;
  font->font_src = NULL;
  font->style = TTF_STYLE_NORMAL;
  luaL_getmetatable(L, LGAME_FONT_METATABLE);
  lua_setmetatable(L, -2);
  if (lua_getfield(L, LUA_REGISTRYINDEX, OPENED_FONTS_FIELD) == LUA_TTABLE) {
    lua_pushvalue(L, -2);
    lua_rawsetp(L, -2, font);
  }
  lua_pop(L, 1);
  return font;
}

lgame_Font* lgame_checkfont(lua_State* L, int idx) {
  lgame_Font* font = (lgame_Font*)luaL_checkudata(L, idx, LGAME_FONT_METATABLE);
  if (font->font == NULL) luaL_error(L, "font is NULL");
  return font;
}

lgame_Font* lgame_testfont(lua_State* L, int idx) {
  return (lgame_Font*)luaL_testudata(L, idx, LGAME_FONT_METATABLE);
}

Uint32 utf8_to_utf32(const char* utf8, int *codesize) {
  const unsigned char* bytes = (const unsigned char*)utf8;
  Uint32 code;
  if (bytes[0] < 128) {
    *codesize = 1;
    code = bytes[0];
  }
  else if ((bytes[0] & 0xE0) == 0xC0) {
    *codesize = 2;
    code = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
  }
  else if ((bytes[0] & 0xF0) == 0xE0) {
    *codesize = 3;
    code = ((bytes[0] & 0x0F) << 12) | ((bytes[1] & 0x3F) << 6) | (bytes[2] & 0x3F);
  }
  else if ((bytes[0] & 0xF8) == 0xF0) {
    *codesize = 4;
    code = ((bytes[0] & 0x07) << 18) | ((bytes[1] & 0x3F) << 12) | ((bytes[2] & 0x3F) << 6) | (bytes[3] & 0x3F);
  }
  else {
    *codesize = 0;
    return 0;
  }
  return code;
}

static int font_init(lua_State* L) {
  if (lgame_font_initialized) {
    return 0;
  }
  if (!TTF_Init()) {
    return luaL_error(L, SDL_GetError());
  }
  lua_newtable(L);
  lua_setfield(L, LUA_REGISTRYINDEX, OPENED_FONTS_FIELD);
  lgame_font_initialized = 1;
  return 0;
}

static int font_quit(lua_State* L) {
  if (!lgame_font_initialized) {
    return 0;
  }

  if (lua_getfield(L, LUA_REGISTRYINDEX, OPENED_FONTS_FIELD) == LUA_TTABLE) {
    lua_pushnil(L);
    while (lua_next(L, -2)) {
      lgame_Font* font = lgame_testfont(L, -1);
      if (font) {
        if (font->font) {
          TTF_CloseFont(font->font);
          font->font = NULL;
        }
        if (font->font_src) {
          lgame_closereadfile(L, font->font_src, 0);
          font->font_src = NULL;
        }
      }
      lua_pop(L, 1);
    }
  }
  lua_pop(L, 1);

  lua_pushnil(L);
  lua_setfield(L, LUA_REGISTRYINDEX, OPENED_FONTS_FIELD);

  TTF_Quit();
  lgame_font_initialized = 0;

  return 0;
}

static int font_get_init(lua_State* L) {
  lua_pushboolean(L, lgame_font_initialized);
  return 1;
}

static int font_get_sdl_ttf_version(lua_State* L) {
  int linked = 1;
  if (!lua_isnoneornil(L, 1))
    linked = lua_toboolean(L, 1);
  int version;
  if (linked)
    version = TTF_Version();
  else
    version = SDL_TTF_VERSION;
  int major, minor, patch;
  major = version / 1000000;
  minor = (version / 1000) % 1000;
  patch = version % 1000;
  lua_createtable(L, 3, 0);
  lua_pushinteger(L, major);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, minor);
  lua_rawseti(L, -2, 2);
  lua_pushinteger(L, patch);
  lua_rawseti(L, -2, 3);
  return 1;
}

static int Font_new(lua_State* L) {
  SDL_IOStream* io = lgame_checkreadfile(L, 1);
  int ptsize = (int)luaL_optinteger(L, 2, 20);
  lgame_Font* self = lgame_pushfont(L);
  self->font = TTF_OpenFontIO(io, false, (float)ptsize);
  if (self->font == NULL) {
    lgame_closereadfile(L, io, 0);
    return luaL_error(L, SDL_GetError());
  }
  self->font_src = io;
  return 1;
}

static int Font_render(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  size_t text_len = 0;
  const char* text = luaL_checklstring(L, 2, &text_len);
  int antialias = lua_toboolean(L, 3);
  lgame_Color fgcolor,* pfgcolor;
  lgame_Color bgcolor,* pbgcolor;
  fgcolor = lgame_checkcolorarg(L, 4);
  pfgcolor = &fgcolor;
  if (!lua_isnoneornil(L, 5)) {
    bgcolor = lgame_checkcolorarg(L, 5);
    pbgcolor = &bgcolor;
  }
  else {
    pbgcolor = NULL;
  }
  int wrap = (int)luaL_optinteger(L, 6, 0);
  
  lgame_Surface* surface_text = lgame_pushsurface(L);
  if (antialias) {
    if (wrap > 0) {
      if (pbgcolor) {
        surface_text->surf = TTF_RenderText_Shaded_Wrapped(self->font, text, text_len, *pfgcolor, *pbgcolor, wrap);
      }
      else {
        surface_text->surf = TTF_RenderText_Blended_Wrapped(self->font, text, text_len, *pfgcolor, wrap);
      }
    }
    else {
      if (pbgcolor) {
        surface_text->surf = TTF_RenderText_Shaded(self->font, text, text_len, *pfgcolor, *pbgcolor);
      }
      else {
        surface_text->surf = TTF_RenderText_Blended(self->font, text, text_len, *pfgcolor);
      }
    }
  }
  else {
    if (wrap > 0) {
      surface_text->surf = TTF_RenderText_Solid_Wrapped(self->font, text, text_len, *pfgcolor, wrap);
    }
    else {
      surface_text->surf = TTF_RenderText_Solid(self->font, text, text_len, *pfgcolor);
    }
    if (surface_text->surf && pbgcolor) {
      if (!SDL_SetSurfaceColorKey(surface_text->surf, false, 0)) {
        SDL_DestroySurface(surface_text->surf);
        return luaL_error(L, SDL_GetError());
      }
      SDL_Palette* pal = SDL_GetSurfacePalette(surface_text->surf);
      if (pal) {
        pal->colors[0].r = pbgcolor->r;
        pal->colors[0].g = pbgcolor->g;
        pal->colors[0].b = pbgcolor->b;
      }
    }
  }

  if (surface_text->surf == NULL)
    return luaL_error(L, SDL_GetError());

  return 1;
}

static int Font_size(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  size_t text_len;
  const char* text = luaL_checklstring(L, 2, &text_len);
  int w, h;
  if (!TTF_GetStringSize(self->font, text, text_len, &w, &h))
    return luaL_error(L, SDL_GetError());
  lua_createtable(L, 2, 0);
  lua_pushinteger(L, w);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, h);
  lua_rawseti(L, -2, 2);
  return 1;
}

static int Font_set_underline(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int enabled = lua_toboolean(L, 2);
  SET_STYLE(self, enabled, TTF_STYLE_UNDERLINE);
  return 0;
}

static int Font_get_underline(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_UNDERLINE));
  return 1;
}

static int Font_set_strikethrough(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int enabled = lua_toboolean(L, 2);
  SET_STYLE(self, enabled, TTF_STYLE_STRIKETHROUGH);
  return 0;
}

static int Font_get_strikethrough(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_STRIKETHROUGH));
  return 1;
}

static int Font_set_bold(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int enabled = lua_toboolean(L, 2);
  SET_STYLE(self, enabled, TTF_STYLE_BOLD);
  return 0;
}

static int Font_get_bold(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_BOLD));
  return 1;
}

static int Font_set_italic(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int enabled = lua_toboolean(L, 2);
  SET_STYLE(self, enabled, TTF_STYLE_ITALIC);
  return 0;
}

static int Font_get_italic(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_ITALIC));
  return 1;
}

static int Font_metrics(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  size_t text_len;
  const char* text = luaL_checklstring(L, 2, &text_len);
  Uint32 code;
  int codesize;
  int i = 1;
  lua_newtable(L);
  while (*text) {
    code = utf8_to_utf32(text, &codesize);
    if (code == 0)
      return luaL_error(L, "invalid utf-8 character");
    
    int minx, maxx, miny, maxy, advance;
    if (!TTF_GetGlyphMetrics(self->font, code, &minx, &maxx, &miny, &maxy, &advance))
      return luaL_error(L, SDL_GetError());

    lua_createtable(L, 5, 0);
    lua_pushinteger(L, minx);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, maxx);
    lua_rawseti(L, -2, 2);
    lua_pushinteger(L, miny);
    lua_rawseti(L, -2, 3);
    lua_pushinteger(L, maxy);
    lua_rawseti(L, -2, 4);
    lua_pushinteger(L, advance);
    lua_rawseti(L, -2, 5);
    lua_rawseti(L, -2, i);

    i++;
    text += codesize;
  }

  return 1;
}

static int Font_get_linesize(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushinteger(L, TTF_GetFontLineSkip(self->font));
  return 1;
}

static int Font_set_linesize(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int linesize = (int)luaL_checkinteger(L, 2);
  TTF_SetFontLineSkip(self->font, linesize);
  return 0;
}

static int Font_get_height(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushinteger(L, TTF_GetFontHeight(self->font));
  return 1;
}

static int Font_set_point_size(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  int ptsize = (int)luaL_checkinteger(L, 2);
  if (!TTF_SetFontSize(self->font, (float)ptsize))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Font_get_point_size(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  float ptsize = TTF_GetFontSize(self->font);
  if (ptsize == 0.0f)
    return luaL_error(L, SDL_GetError());
  lua_pushinteger(L, (lua_Integer)ptsize);
  return 1;
}

static int Font_get_ascent(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushinteger(L, TTF_GetFontAscent(self->font));
  return 1;
}

static int Font_get_descent(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  lua_pushinteger(L, TTF_GetFontDescent(self->font));
  return 1;
}

static int Font_set_direction(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  TTF_Direction direction = (TTF_Direction)luaL_checkinteger(L, 2);
  if (!TTF_SetFontDirection(self->font, direction))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Font__index(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  const char* key = luaL_checkstring(L, 2);
  if (strcmp(key, "bold") == 0)
    lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_BOLD));
  else if (strcmp(key, "italic") == 0)
    lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_ITALIC));
  else if (strcmp(key, "underline") == 0)
    lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_UNDERLINE));
  else if (strcmp(key, "strikethrough") == 0)
    lua_pushboolean(L, GET_STYLE(self, TTF_STYLE_STRIKETHROUGH));
  else if (strcmp(key, "point_size") == 0) {
    float ptsize = TTF_GetFontSize(self->font);
    if (ptsize == .0f)
      return luaL_error(L, SDL_GetError());
    lua_pushinteger(L, (lua_Integer)ptsize);
  }
  else
    lgame_getmethod(L, 1, key);
  return 1;
}

static int Font__newindex(lua_State* L) {
  lgame_Font* self = lgame_checkfont(L, 1);
  const char* key = luaL_checkstring(L, 2);
  if (
    strcmp(key, "bold")          == 0 ||
    strcmp(key, "italic")        == 0 ||
    strcmp(key, "underline")     == 0 ||
    strcmp(key, "strikethrough") == 0) {
    int enabled = lua_toboolean(L, 3);
    if (strcmp(key, "bold") == 0)
      SET_STYLE(self, enabled, TTF_STYLE_BOLD);
    else if (strcmp(key, "italic") == 0)
      SET_STYLE(self, enabled, TTF_STYLE_ITALIC);
    else if (strcmp(key, "underline") == 0)
      SET_STYLE(self, enabled, TTF_STYLE_UNDERLINE);
    else if (strcmp(key, "strikethrough") == 0)
      SET_STYLE(self, enabled, TTF_STYLE_STRIKETHROUGH);
  }
  else if (strcmp(key, "point_size") == 0) {
    int ptsize = (int)luaL_checkinteger(L, 3);
    if (!TTF_SetFontSize(self->font, (float)ptsize))
      return luaL_error(L, SDL_GetError());
  }
  else
    return luaL_error(L, "unknow '%s' key", key);
  return 0;
}

static int Font__gc(lua_State* L) {
  lgame_Font* self = lgame_testfont(L, 1);
  if (self) {
    if (lua_getfield(L, LUA_REGISTRYINDEX, OPENED_FONTS_FIELD) == LUA_TTABLE) {
      lua_pushnil(L);
      lua_rawsetp(L, -2, self);
    }
    lua_pop(L, 1);

    if (self->font) {
      TTF_CloseFont(self->font);
      self->font = NULL;
    }

    if (self->font_src) {
      lgame_closereadfile(L, self->font_src, 0);
      self->font_src = NULL;
    }
  }
  return 0;
}