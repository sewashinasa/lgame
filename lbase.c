
#include "lbase.h"
#include "ldisplay.h"
#include "lpoint.h"

static int num_pass = 0;
static int num_fail = 0;

static int lg_initialized = 0;



static int base_init(lua_State* L);
static int base_quit(lua_State* L);
static int base_get_init(lua_State* L);

const struct luaL_Reg lg_base_module[] = {
  {"init", base_init},
  {"quit", base_quit},
  {"get_init", base_get_init},
  {NULL, NULL}
};

static lua_CFunction getfunction(lua_State* L, const char* n, const struct luaL_Reg* r) {
  const struct luaL_Reg* pr = r;
  while (pr->name) {
    if (strcmp(pr->name, n) == 0) {
      return pr->func;
    }
    pr++;
  }
  return NULL;
}

static int base_init(lua_State* L) {
  if (lg_initialized) {
    lg_pushpoint2int(L, num_pass, num_fail);
    return 1;
  }
  lua_CFunction display_init;
  display_init = getfunction(L, "init", lg_display_module);
  lua_pushcfunction(L, display_init);
  if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
    num_pass++;
  }
  else {
    lua_pop(L, 1); num_fail++;
  }
  lg_pushpoint2int(L, num_pass, num_fail);
  lg_initialized = 1;
  return 1;
}

static int base_quit(lua_State* L) {
  if (!lg_initialized) return 0;
  lua_CFunction display_quit;
  display_quit = getfunction(L, "quit", lg_display_module);
  display_quit(L);
  lg_initialized = 0;
  num_pass = 0;
  num_fail = 0;
  return 0;
}

static int base_get_init(lua_State* L) {
  lua_pushboolean(L, lg_initialized);
  return 1;
}