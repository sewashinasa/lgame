
#ifndef LRECT_H_
#define LRECT_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_rect.h>

#define LGAME_RECT_METATABLE "lgame.Rect"

typedef SDL_Rect lRect;
typedef SDL_Point lPoint;

extern const struct luaL_Reg lg_Rect_module[];
extern const struct luaL_Reg lg_Rect_methods[];
extern const struct luaL_Reg lg_Rect_metamethods[];

lRect* lg_pushrect(lua_State* L);
lRect* lg_checkrect(lua_State* L, int idx);
void lg_pushpoint(lua_State* L, lPoint p);
lPoint lg_checkpoint(lua_State* L, int idx);
lRect lg_checkrecttable(lua_State* L, int idx);
lRect lg_checkrectarg(lua_State* L, int idx);
lRect lg_checkrectfromargs(lua_State* L, int offset);

#define lg_checkrectfromargs0(L) lg_checkrectfromargs((L), 0)
#define lg_checkrectfromargs1(L) lg_checkrectfromargs((L), 1)

#endif