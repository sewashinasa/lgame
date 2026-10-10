
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

const struct luaL_Reg lg_display_module[] = {
  {"init", display_init},
  {"quit", display_quit},
  {"get_init", display_get_init},
  {"set_mode", display_set_mode},
  {NULL, NULL}
};

static SDL_Window* lg_Window = NULL;
static SDL_Surface* lg_WindowSurface = NULL;

static int lg_display_initialized = 0;
static int lg_display_mode_set = 0;

SDL_Surface* lg_get_windowsurface(void) { return lg_WindowSurface; }
SDL_Window* lg_get_window(void) { return lg_Window; }

static int display_init(lua_State* L) {
  if (lg_display_initialized) return 0;
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
  lg_display_initialized = 1;
  return 0;
}

static int display_quit(lua_State* L) {
  if (!lg_display_initialized) return 0;
  if (lg_Window) {
    SDL_DestroyWindow(lg_Window);
    lg_Window = NULL;
    lg_WindowSurface = NULL;
  }
  lg_getwindowsurface(L);
  if (!lua_isnil(L, 1)) {
    lua_pushnil(L);
    lg_setwindowsurface(L);
    lua_pop(L, 1);
  }
  if (SDL_WasInit(SDL_INIT_EVENTS))
    SDL_QuitSubSystem(SDL_INIT_EVENTS);
  if (SDL_WasInit(SDL_INIT_VIDEO))
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
  lg_display_initialized = 0;
  return 0;
}

static int display_get_init(lua_State* L) {
  lua_pushboolean(L, lg_display_initialized);
  return 1;
}

static int display_set_mode(lua_State* L) {
  lSurface* ws;
  int dw, dh;
  lg_checkpoint2int(L, 1, &dw, &dh);

  if (lg_display_mode_set) {
    SDL_SetWindowSize(lg_Window, dw, dh);

    lg_WindowSurface = SDL_GetWindowSurface(lg_Window);
    if (!lg_WindowSurface) {
      SDL_DestroyWindow(lg_Window);
      return luaL_error(L, SDL_GetError());
    }

    lg_getwindowsurface(L);
    ws = (lSurface*)lua_touserdata(L, -1);
    ws->surf = lg_WindowSurface;

    return 1;
  }

  lg_Window = SDL_CreateWindow(DEFAULT_TITLE, dw, dh, 0);
  if (!lg_Window)
    return luaL_error(L, SDL_GetError());

  lg_WindowSurface = SDL_GetWindowSurface(lg_Window);
  if (!lg_WindowSurface) {
    SDL_DestroyWindow(lg_Window);
    return luaL_error(L, SDL_GetError());
  }

  ws = lg_pushsurface(L);
  ws->surf = lg_WindowSurface;
  ws->isws = 1;
  lua_pushvalue(L, -1);
  lg_setwindowsurface(L);

  lg_display_mode_set = 1;

  return 1;
}