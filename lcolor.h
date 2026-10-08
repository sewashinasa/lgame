
#ifndef LCOLOR_H_
#define LCOLOR_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_pixels.h>

#define LGAME_COLOR_METATABLE "lgame.Color"
#define LGAME_FCOLOR_METATABLE "lgame.FColor"

typedef SDL_Color lColor;
typedef SDL_FColor lFColor;

extern const struct luaL_Reg lg_Color_module[];
extern const struct luaL_Reg lg_Color_methods[];
extern const struct luaL_Reg lg_Color_metamethods[];

lColor* lg_pushcolor(lua_State* L);
lColor* lg_checkcolor(lua_State* L, int idx);
lColor lg_checkcolortable(lua_State* L, int idx);
lColor lg_unpack32(Uint32 color);
lColor lg_checkcolorarg(lua_State* L, int idx);

#define lg_checkcolorfromargs0(L) lg_checkcolorfromargs((L), 0)
#define lg_checkcolorfromargs1(L) lg_checkcolorfromargs((L), 1)


#endif