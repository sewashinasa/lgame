
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ldisplay.h"
#include "lsurface.h"
#include "lpoint.h"

#define DEFAULT_TITLE "lgame window"

static int display_init(lua_State* L);
static int display_quit(lua_State* L);
static int display_get_init(lua_State* L);
static int display_set_mode(lua_State* L);

const struct luaL_Reg lgame_display_module[] = {
  {"init", display_init},
  {"quit", display_quit},
  {"get_init", display_get_init},
  {"set_mode", display_set_mode},
  {NULL, NULL}
};

static SDL_Window* lgame_Window = NULL;
static SDL_Surface* lgame_WindowSurface = NULL;

static int lgame_display_initialized = 0;
static int lgame_display_mode_set = 0;

SDL_Surface* lgame_get_windowsurface(void) { return lgame_WindowSurface; }
SDL_Window* lgame_get_window(void) { return lgame_Window; }

static int display_init(lua_State* L) {
  if (lgame_display_initialized) return 0;
  if (!SDL_WasInit(SDL_INIT_VIDEO)) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
      return luaL_error(L, SDL_GetError());
    }
  }
  if (!SDL_WasInit(SDL_INIT_EVENTS)) {
    if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
      SDL_QuitSubSystem(SDL_INIT_VIDEO);
      return luaL_error(L, SDL_GetError());
    }
  }
  lgame_display_initialized = 1;
  return 0;
}

static int display_quit(lua_State* L) {
  if (!lgame_display_initialized) return 0;
  if (lgame_Window) {
    SDL_DestroyWindow(lgame_Window);
    lgame_Window = NULL;
    lgame_WindowSurface = NULL;
  }
  lgame_getwindowsurface(L);
  if (!lua_isnil(L, 1)) {
    lua_pushnil(L);
    lgame_setwindowsurface(L);
    lua_pop(L, 1);
  }
  if (SDL_WasInit(SDL_INIT_EVENTS))
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
  if (SDL_WasInit(SDL_INIT_VIDEO))
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  lgame_display_initialized = 0;
  lgame_display_mode_set = 0;
  return 0;
}

static int display_get_init(lua_State* L) {
  lua_pushboolean(L, lgame_display_initialized);
  return 1;
}

static int display_set_mode(lua_State* L) {
  lgame_Surface* ws;
  int dw, dh;
  lgame_checkpoint2int(L, 1, &dw, &dh);

  if (lgame_display_mode_set) {
    SDL_SetWindowSize(lgame_Window, dw, dh);

    lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
    if (!lgame_WindowSurface) {
      SDL_DestroyWindow(lgame_Window);
      return luaL_error(L, SDL_GetError());
    }

    lgame_getwindowsurface(L);
    ws = (lgame_Surface*)lua_touserdata(L, -1);
    ws->surf = lgame_WindowSurface;

    return 1;
  }

  lgame_Window = SDL_CreateWindow(DEFAULT_TITLE, dw, dh, 0);
  if (!lgame_Window)
    return luaL_error(L, SDL_GetError());

  lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
  if (!lgame_WindowSurface) {
    SDL_DestroyWindow(lgame_Window);
    return luaL_error(L, SDL_GetError());
  }

  ws = lgame_pushsurface(L);
  ws->surf = lgame_WindowSurface;
  ws->isws = 1;
  lua_pushvalue(L, -1);
  lgame_setwindowsurface(L);

  lgame_display_mode_set = 1;

  return 1;
}