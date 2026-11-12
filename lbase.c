
#include "lbase.h"
#include "ldisplay.h"
#include "lmixer.h"
#include "lfont.h"
#include "lpoint.h"

static int num_pass = 0;
static int num_fail = 0;

static int lgame_initialized = 0;



static int base_init(lua_State* L);
static int base_quit(lua_State* L);
static int base_get_init(lua_State* L);

const struct luaL_Reg lgame_base_module[] = {
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
  if (lgame_initialized) {
    lgame_pushpoint2int(L, num_pass, num_fail);
    return 1;
  }

  lua_CFunction display_init;
  lua_CFunction font_init;
  lua_CFunction mixer_init;
  display_init = getfunction(L, "init", lgame_display_module);
  font_init = getfunction(L, "init", lgame_font_module);
  mixer_init = getfunction(L, "init", lgame_mixer_module);
  
  lua_pushcfunction(L, display_init);
  if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
    num_pass++;
  }
  else {
    lua_pop(L, 1); num_fail++;
  }

  lua_pushcfunction(L, font_init);
  if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
    num_pass++;
  }
  else {
    lua_pop(L, 1); num_fail++;
  }

  lua_pushcfunction(L, mixer_init);
  if (lua_pcall(L, 0, 0, 0) == LUA_OK) {
    num_pass++;
  }
  else {
    lua_pop(L, 1); num_fail++;
  }

  lgame_pushpoint2int(L, num_pass, num_fail);
  lgame_initialized = 1;
  return 1;
}

static int base_quit(lua_State* L) {
  if (!lgame_initialized) return 0;

  lua_CFunction mixer_quit;
  lua_CFunction font_quit;
  lua_CFunction display_quit;
  mixer_quit = getfunction(L, "quit", lgame_mixer_module);
  font_quit = getfunction(L, "quit", lgame_font_module);
  display_quit = getfunction(L, "quit", lgame_display_module);

  mixer_quit(L);
  font_quit(L);
  display_quit(L);

  lgame_initialized = 0;
  num_pass = 0;
  num_fail = 0;

  return 0;
}

static int base_get_init(lua_State* L) {
  lua_pushboolean(L, lgame_initialized);
  return 1;
}