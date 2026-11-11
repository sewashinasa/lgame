
#define _CRT_SECURE_NO_WARNINGS

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "limage.h"
#include "lsurface.h"
#include "lcheckfile.h"

static int image_load_basic(lua_State* L);
static int image_load_extended(lua_State* L);
static int image_load(lua_State* L);
static int image_save_basic(lua_State* L);
static int image_save_extended(lua_State* L);
static int image_save(lua_State* L);
static int image_get_sdl_image_version(lua_State* L);
static int image_get_extended(lua_State* L);

const struct luaL_Reg lgame_image_module[] = {
  {"load_basic", image_load_basic},
  {"load_extended", image_load_extended},
  {"load", image_load},
  {"save_basic", image_save_basic},
  {"save_extended", image_save_extended},
  {"save", image_save},
  {"get_sdl_image_version", image_get_sdl_image_version},
  {"get_extended", image_get_extended},
  {NULL, NULL}
};

#define NAMEHINT_SIZE 16

static void process_namehint(const char* s, char* buf) {
  int skip = 0;
  for (int i = (int)strlen(s) - 1; i >= 0; i--)
    if (s[i] == '.')
      skip = i;
  strncpy(buf, s + skip, NAMEHINT_SIZE);
  for (int i = 0; i < (int)strlen(buf); i++)
    buf[i] = SDL_toupper(buf[i]);
}

static int image_load_basic(lua_State* L) {
  SDL_IOStream* io = lgame_checkreadfile(L, 1);
  lgame_Surface* surface = lgame_pushsurface(L);
  surface->surf = SDL_LoadBMP_IO(io, false);
  if (!surface->surf) {
    lgame_closereadfile(L, io, 0);
    return luaL_error(L, SDL_GetError());
  }
  lgame_closereadfile(L, io, 1);
  return 1;
}

static int image_load_extended(lua_State* L) {
  SDL_IOStream* io = lgame_checkreadfile(L, 1);
  const char* namehint = luaL_optstring(L, 2, NULL);
  SDL_Surface* surface = NULL;
  lgame_Surface* lsurface = NULL;
  if (namehint) {
    char buf[NAMEHINT_SIZE];
    process_namehint(namehint, buf);
    surface = IMG_LoadTyped_IO(io, false, namehint);
  }
  else {
    surface = IMG_Load_IO(io, false);
  }
  if (!surface) {
    lgame_closereadfile(L, io, 0);
    return luaL_error(L, SDL_GetError());
  }
  lgame_closereadfile(L, io, 0);
  lsurface = lgame_pushsurface(L);
  lsurface->surf = surface;
  return 1;
}

static int image_load(lua_State* L) {
  return image_load_extended(L);
}

static int image_save_basic(lua_State* L) {
  int dyn;
  SDL_Surface* surface = lgame_checksurface(L, 1)->surf;
  SDL_IOStream* io = lgame_checkwritefile(L, 2, &dyn);
  if (!SDL_SaveBMP_IO(surface, io, false)) {
    lgame_closewritefile(L, io, 2, dyn, 0);
    return luaL_error(L, SDL_GetError());
  }
  lgame_closewritefile(L, io, 2, dyn, 1);
  return 0;
}

static int image_save_extended(lua_State* L) {
  int dyn;
  SDL_Surface* surface = lgame_checksurface(L, 1)->surf;
  SDL_IOStream* io = lgame_checkwritefile(L, 2, &dyn);
  const char* namehint = luaL_optstring(L, 3, NULL);
  if (namehint) {
    if (!IMG_SaveTyped_IO(surface, io, false, namehint)) {
      lgame_closewritefile(L, io, 2, dyn, 0);
      return luaL_error(L, SDL_GetError());
    }
  }
  else {
    if (!IMG_SaveJPG_IO(surface, io, false, 100)) {
      lgame_closewritefile(L, io, 2, dyn, 0);
      return luaL_error(L, SDL_GetError());
    }
  }
  lgame_closewritefile(L, io, 2, dyn, 1);
  return 0;
}

static int image_save(lua_State* L) {
  return image_save_extended(L);
}

static int image_get_sdl_image_version(lua_State* L) {
  int linked = 1;
  if (!lua_isnoneornil(L, 1))
    linked = lua_toboolean(L, 1);
  int version;
  if (linked)
    version = IMG_Version();
  else
    version = SDL_IMAGE_VERSION;
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

static int image_get_extended(lua_State* L) {
  lua_pushboolean(L, 1);
  return 1;
}