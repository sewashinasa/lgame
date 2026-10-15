
#ifndef LRECT_H_
#define LRECT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_rect.h>

#define LGAME_RECT_METATABLE "lgame.Rect"

typedef SDL_Rect lgame_Rect;
typedef SDL_Point lgame_Point;

extern const struct luaL_Reg lgame_rect_module[];
extern const struct luaL_Reg lgame_Rect_methods[];
extern const struct luaL_Reg lgame_Rect_metamethods[];

lgame_Rect* lgame_pushrect(lua_State* L);
lgame_Rect* lgame_checkrect(lua_State* L, int idx);
void lgame_pushpoint(lua_State* L, lgame_Point p);
void lgame_pushpoint2int(lua_State* L, int x, int y);
lgame_Point lgame_checkpoint(lua_State* L, int idx);
void lgame_checkpoint2int(lua_State* L, int idx, int* x, int* y);
lgame_Rect lgame_checkrecttable(lua_State* L, int idx);
lgame_Rect lgame_checkrectarg(lua_State* L, int idx);
lgame_Rect lgame_checkrectfromargs(lua_State* L, int offset);

#define lgame_checkrectfromargs0(L) lgame_checkrectfromargs((L), 0)
#define lgame_checkrectfromargs1(L) lgame_checkrectfromargs((L), 1)

#endif