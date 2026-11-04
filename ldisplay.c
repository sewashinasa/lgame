
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ldisplay.h"
#include "lsurface.h"
#include "lrect.h"
#include "lconsts.h"

#define DEFAULT_TITLE "lgame window"

static int display_init(lua_State* L);
static int display_quit(lua_State* L);
static int display_get_init(lua_State* L);
static int display_set_mode(lua_State* L);
static int display_get_surface(lua_State* L);
static int display_flip(lua_State* L);
static int display_update(lua_State* L);
static int display_get_driver(lua_State* L);

const struct luaL_Reg lgame_display_module[] = {
  {"init", display_init},
  {"quit", display_quit},
  {"get_init", display_get_init},
  {"set_mode", display_set_mode},
  {"get_surface", display_get_surface},
  {"flip", display_flip},
  {"update", display_update},
  {"get_driver", display_get_driver},
  {NULL, NULL}
};

static SDL_Window* lgame_Window = NULL;
static SDL_Surface* lgame_WindowSurface = NULL;

static int lgame_display_initialized = 0;
static int lgame_display_mode_set = 0;

int lgame_validate_window_surface = 0;

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
  int flags;
  int depth;
  int display;
  int vsync;
  lgame_checkpoint2int(L, 1, &dw, &dh);
  flags = (int)luaL_optinteger(L, 2, 0);
  depth = (int)luaL_optinteger(L, 3, 0); (void)depth;
  display = (int)luaL_optinteger(L, 4, 0);
  vsync = (int)luaL_optinteger(L, 5, 0); (void)vsync;

  display_init(L);

  if (dw == 0 || dh == 0) {
    const SDL_DisplayMode* dm;
    dm = SDL_GetDesktopDisplayMode((SDL_DisplayID)display);
    if (dm == NULL)
      return luaL_error(L, SDL_GetError());
    if (dw == 0) dw = dm->w;
    if (dh == 0) dh = dm->h;
  }

  if (lgame_display_mode_set) {
    SDL_SetWindowSize(lgame_Window, dw, dh);

    lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
    if (!lgame_WindowSurface) {
      return luaL_error(L, SDL_GetError());
    }

    if (flags & LGAME_HIDDEN) {
      if (!SDL_HideWindow(lgame_Window)) {
        return luaL_error(L, SDL_GetError());
      }
    }
    else {
      if (!SDL_ShowWindow(lgame_Window)) {
        return luaL_error(L, SDL_GetError());
      }
    }

    if (flags & LGAME_NOFRAME) {
      if (!SDL_SetWindowBordered(lgame_Window, false)) {
        return luaL_error(L, SDL_GetError());
      }
    }
    else {
      if (!SDL_SetWindowBordered(lgame_Window, true)) {
        return luaL_error(L, SDL_GetError());
      }
    }

    if (flags & LGAME_RESIZABLE) {
      if (!SDL_SetWindowResizable(lgame_Window, true)) {
        return luaL_error(L, SDL_GetError());
      }
    }
    else {
      if (!SDL_SetWindowResizable(lgame_Window, false)) {
        return luaL_error(L, SDL_GetError());
      }
    }

    lgame_getwindowsurface(L);
    ws = (lgame_Surface*)lua_touserdata(L, -1);
    ws->surf = lgame_WindowSurface;

    return 1;
  }

  lgame_Window = SDL_CreateWindow(DEFAULT_TITLE, dw, dh, flags);
  if (!lgame_Window)
    return luaL_error(L, SDL_GetError());

  lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
  if (!lgame_WindowSurface)
    return luaL_error(L, SDL_GetError());

  ws = lgame_pushsurface(L);
  ws->surf = lgame_WindowSurface;
  ws->isws = 1;
  lua_pushvalue(L, -1);
  lgame_setwindowsurface(L);

  lgame_display_mode_set = 1;

  return 1;
}

static int display_get_surface(lua_State* L) {
  lgame_getwindowsurface(L);
  return 1;
}

static int display_flip(lua_State* L) {
  if (!lgame_display_mode_set)
    return luaL_error(L, "window not opened");
  if (lgame_validate_window_surface) {
    lgame_Surface* ws;
    lgame_validate_window_surface = 0;
    lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
    if (!lgame_WindowSurface)
      return luaL_error(L, SDL_GetError());
    lgame_getwindowsurface(L);
    ws = (lgame_Surface*)lua_touserdata(L, -1);
    ws->surf = lgame_WindowSurface;
    lua_pop(L, 1);
  }
  if (!SDL_UpdateWindowSurface(lgame_Window))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int isrectstyleargs(lua_State* L) {
  int top = lua_gettop(L);
  if (top == 1) {
    if (lua_isuserdata(L, 1) && luaL_testudata(L, 1, LGAME_RECT_METATABLE) != NULL)
      return 1;
    if (lua_istable(L, 1)) {
      int tlen = (int)lua_rawlen(L, 1);
      if (tlen == 4) {
        int type1, type2, type3, type4;
        type1 = lua_rawgeti(L, 1, 1);
        type2 = lua_rawgeti(L, 1, 2);
        type3 = lua_rawgeti(L, 1, 3);
        type4 = lua_rawgeti(L, 1, 4);
        lua_pop(L, 4);
        if (
          type1 == LUA_TNUMBER &&
          type2 == LUA_TNUMBER &&
          type3 == LUA_TNUMBER &&
          type4 == LUA_TNUMBER) {
          return 1;
        }
        else {
          return 0;
        }
      }
      else if (tlen == 2) {
        int type1, tableidx1;
        int type2, tableidx2;
        type1 = lua_rawgeti(L, 1, 1); tableidx1 = lua_gettop(L);
        type2 = lua_rawgeti(L, 1, 2); tableidx2 = lua_gettop(L);
        if (
          type1 == LUA_TTABLE &&
          type2 == LUA_TTABLE) {
          int tlen1, tlen2;
          tlen1 = (int)lua_rawlen(L, tableidx1);
          tlen2 = (int)lua_rawlen(L, tableidx2);
          if (tlen1 == 2 && tlen2 == 2) {
            int type11, type12;
            int type21, type22;
            type11 = lua_rawgeti(L, tableidx1, 1);
            type12 = lua_rawgeti(L, tableidx1, 2);
            type21 = lua_rawgeti(L, tableidx2, 1);
            type22 = lua_rawgeti(L, tableidx2, 2);
            lua_pop(L, 6);
            if (
              type11 == LUA_TNUMBER &&
              type12 == LUA_TNUMBER &&
              type21 == LUA_TNUMBER &&
              type22 == LUA_TNUMBER) {
              return 1;
            }
            else {
              return 0;
            }
          }
          else {
            return 0;
          }
        }
        else {
          return 0;
        }
      }
      else {
        return 0;
      }
    }
  }
  else if (top == 2) {
    if (lua_istable(L, 1) && lua_istable(L, 2)) {
      int tlen1, tlen2;
      tlen1 = (int)lua_rawlen(L, 1);
      tlen2 = (int)lua_rawlen(L, 2);
      if (tlen1 == 2 && tlen2 == 2) {
        int type11, type12;
        int type21, type22;
        type11 = lua_rawgeti(L, 1, 1);
        type12 = lua_rawgeti(L, 1, 2);
        type21 = lua_rawgeti(L, 2, 1);
        type22 = lua_rawgeti(L, 2, 2);
        lua_pop(L, 4);
        if (
          type11 == LUA_TNUMBER &&
          type12 == LUA_TNUMBER &&
          type21 == LUA_TNUMBER &&
          type22 == LUA_TNUMBER) {
          return 1;
        }
        else {
          return 0;
        }
      }
      else {
        return 0;
      }
    }
    else {
      return 0;
    }
  }
  else if (top == 4) {
    if (
      lua_isnumber(L, 1) &&
      lua_isnumber(L, 2) &&
      lua_isnumber(L, 3) &&
      lua_isnumber(L, 4)) {
      return 1;
    }
    else {
      return 0;
    }
  }
  return 0;
}

static int display_update(lua_State* L) {
  SDL_Rect rects[128] = { 0 };
  int numrects;
  if (!lgame_display_mode_set)
    return luaL_error(L, "window not opened");
  if (lua_isnil(L, 1))
    return 0;
  if (lua_isnone(L, 1))
    return display_flip(L);
  if (isrectstyleargs(L)) {
    numrects = 1;
    rects[0] = lgame_checkrectfromargs0(L);
  }
  else if (lua_istable(L, 1)) {
    numrects = (int)lua_rawlen(L, 1);
    for (int i = 0; i < numrects; i++) {
      lua_rawgeti(L, 1, (lua_Integer)(i + 1));
      rects[i] = lgame_checkrectarg(L, -1);
      lua_pop(L, 1);
    }
  }
  else {
    return luaL_error(L, "expected rect-style or nil or none argument or table of rect-style values");
  }
  if (!SDL_UpdateWindowSurfaceRects(lgame_Window, rects, numrects)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int display_get_driver(lua_State* L) {
  lua_pushstring(L, SDL_GetCurrentVideoDriver());
  return 1;
}