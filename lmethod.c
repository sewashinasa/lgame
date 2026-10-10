
#include "lmethod.h"

void lg_getmethod(lua_State* L, int obji, const char* mn) {
  obji = lua_absindex(L, obji);
  lua_getmetatable(L, obji);
  if (lua_isnil(L, -1))
    return;
  lua_getfield(L, -1, "METHODS");
  lua_remove(L, -2);
  if (lua_isnil(L, -1))
    return;
  lua_getfield(L, -1, mn);
  lua_remove(L, -2);
  if (lua_isnil(L, -1))
    return;
}