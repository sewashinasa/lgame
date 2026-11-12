
#ifndef LFONT_H_
#define LFONT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_ttf/SDL_textengine.h>

#define LGAME_FONT_METATABLE "lgame.Font"
#define LGAME_TEXTENGINE_METATABLE "lgame.TextEngine"

typedef struct lgame_Font {
  TTF_Font* font;
  int style;
  SDL_IOStream* font_src;
} lgame_Font;

lgame_Font* lgame_pushfont(lua_State* L);
lgame_Font* lgame_checkfont(lua_State* L, int idx);
lgame_Font* lgame_testfont(lua_State* L, int idx);

const struct luaL_Reg lgame_font_module[];
const struct luaL_Reg lgame_Font_methods[];
const struct luaL_Reg lgame_Font_metamethods[];

#endif