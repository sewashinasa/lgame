
#ifndef LCOLOR_H_
#define LCOLOR_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_pixels.h>

#define LGAME_COLOR_METATABLE "lgame.Color"
#define LGAME_FCOLOR_METATABLE "lgame.FColor"

typedef SDL_Color lgame_Color;
typedef SDL_FColor lgame_FColor;

extern const struct luaL_Reg lgame_color_module[];
extern const struct luaL_Reg lgame_Color_methods[];
extern const struct luaL_Reg lgame_Color_metamethods[];

lgame_Color* lgame_pushcolor(lua_State* L);
lgame_Color* lgame_checkcolor(lua_State* L, int idx);
lgame_Color lgame_checkcolortable(lua_State* L, int idx);
lgame_Color lgame_unpack32(Uint32 color);
lgame_Color lgame_checkcolorarg(lua_State* L, int idx);

#define lgame_checkcolorfromargs0(L) lgame_checkcolorfromargs((L), 0)
#define lgame_checkcolorfromargs1(L) lgame_checkcolorfromargs((L), 1)


#endif