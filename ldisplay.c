
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ldisplay.h"
#include "lsurface.h"
#include "lrect.h"
#include "ltime.h"
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
static int display_get_wm_info(lua_State* L);
static int display_get_desktop_sizes(lua_State* L);
static int display_list_modes(lua_State* L);
static int display_get_active(lua_State* L);
static int display_iconify(lua_State* L);
static int display_toggle_fullscreen(lua_State* L);
static int display_set_icon(lua_State* L);
static int display_set_caption(lua_State* L);
static int display_get_caption(lua_State* L);
static int display_is_fullscreen(lua_State* L);

static int VideoInfo_new(lua_State* L);
static int VideoInfo__index(lua_State* L);
static int VideoInfo__tostring(lua_State* L);

const struct luaL_Reg lgame_display_module[] = {
  {"init", display_init},
  {"quit", display_quit},
  {"get_init", display_get_init},
  {"set_mode", display_set_mode},
  {"get_surface", display_get_surface},
  {"flip", display_flip},
  {"update", display_update},
  {"get_driver", display_get_driver},
  {"VideoInfo", VideoInfo_new},
  {"get_wm_info", display_get_wm_info},
  {"get_desktop_sizes", display_get_desktop_sizes},
  {"list_modes", display_list_modes},
  {"get_active", display_get_active},
  {"iconify", display_iconify},
  {"toggle_fullscreen", display_toggle_fullscreen},
  {"set_icon", display_set_icon},
  {"set_caption", display_set_caption},
  {"get_caption", display_get_caption},
  {"is_fullscreen", display_is_fullscreen},
  {NULL, NULL}
};

const struct luaL_Reg lgame_VideoInfo_metamethods[] = {
  {"__index", VideoInfo__index},
  {"__tostring", VideoInfo__tostring},
  {NULL, NULL}
};

static SDL_Window* lgame_Window = NULL;
static SDL_Surface* lgame_WindowSurface = NULL;

static int lgame_display_initialized = 0;
static int lgame_display_mode_set = 0;

int lgame_validate_window_surface = 0;

static int last_window_width = 0;
static int last_window_height = 0;

lgame_VideoInfo *lgame_pushvideoinfo(lua_State* L) {
  lgame_VideoInfo* vinfo = (lgame_VideoInfo*)lua_newuserdata(L, sizeof(lgame_VideoInfo));
  luaL_getmetatable(L, LGAME_VIDEOINFO_METATABLE);
  lua_setmetatable(L, -2);

  memset(vinfo, 0, sizeof(lgame_VideoInfo));

  return vinfo;
}

lgame_VideoInfo* lgame_checkvideoinfo(lua_State* L, int idx) {
  return luaL_checkudata(L, idx, LGAME_VIDEOINFO_METATABLE);
}

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
  lgame_timer_map_quit();
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
    return luaL_error(L, "video mode not set");
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

#define MAX_RECTS 128

static int display_update(lua_State* L) {
  SDL_Rect rects[MAX_RECTS] = { 0 };
  int numrects;
  if (!lgame_display_mode_set)
    return luaL_error(L, "video mode not set");
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
    if (numrects > MAX_RECTS)
      return luaL_error(L, "the length of the table should be no more than %d", MAX_RECTS);
    for (int i = 0; i < numrects; i++) {
      lua_rawgeti(L, 1, (lua_Integer)(i + 1));
      rects[i] = lgame_checkrectarg(L, -1);
      lua_pop(L, 1);
    }
  }
  else {
    return luaL_error(L, "expected rectstyle or table of rectstyle values");
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

static int VideoInfo_new(lua_State* L) {
  lgame_VideoInfo* self = lgame_pushvideoinfo(L);

  const SDL_PixelFormatDetails* pfd;

  if (lgame_display_mode_set) {
    pfd = SDL_GetPixelFormatDetails(lgame_WindowSurface->format);

    self->current_w = lgame_WindowSurface->w;
    self->current_h = lgame_WindowSurface->h;
  }
  else {
    const SDL_DisplayMode* dm = SDL_GetCurrentDisplayMode(1);
    if (dm == NULL)
      return luaL_error(L, SDL_GetError());

    pfd = SDL_GetPixelFormatDetails(dm->format);

    self->current_w = dm->w;
    self->current_h = dm->h;
  }

  if (pfd == NULL)
    return luaL_error(L, SDL_GetError());

  self->bitsize = pfd->bits_per_pixel;
  self->bytesize = pfd->bytes_per_pixel;

  self->Rmask = pfd->Rmask;
  self->Gmask = pfd->Gmask;
  self->Bmask = pfd->Bmask;
  self->Amask = pfd->Amask;

  self->Rshift = pfd->Rshift;
  self->Gshift = pfd->Gshift;
  self->Bshift = pfd->Bshift;
  self->Ashift = pfd->Ashift;

  self->Rloss = 8 - pfd->Rbits;
  self->Gloss = 8 - pfd->Gbits;
  self->Bloss = 8 - pfd->Bbits;
  self->Aloss = 8 - pfd->Abits;

  self->pixel_format = pfd->format;

  return 1;
}

#define PUSH4INT(L, a, b, c, d) \
  do { \
    lua_createtable(L, 4, 0); \
    lua_pushinteger(L, (a)); \
    lua_rawseti(L, -2, 1); \
    lua_pushinteger(L, (b)); \
    lua_rawseti(L, -2, 2); \
    lua_pushinteger(L, (c)); \
    lua_rawseti(L, -2, 3); \
    lua_pushinteger(L, (d)); \
    lua_rawseti(L, -2, 4); \
  } while (0)

static int VideoInfo__index(lua_State* L) {
  lgame_VideoInfo* self = lgame_checkvideoinfo(L, 1);
  const char* key = luaL_checkstring(L, 2);
  if (strcmp(key, "hw") == 0)
    lua_pushinteger(L, self->hw);
  else if (strcmp(key, "wm") == 0)
    lua_pushinteger(L, self->wm);
  else if (strcmp(key, "video_mem") == 0)
    lua_pushinteger(L, self->video_mem);
  else if (strcmp(key, "bitsize") == 0)
    lua_pushinteger(L, self->bitsize);
  else if (strcmp(key, "bytesize") == 0)
    lua_pushinteger(L, self->bytesize);
  else if (strcmp(key, "masks") == 0)
    PUSH4INT(L, self->Rmask, self->Gmask, self->Bmask, self->Amask);
  else if (strcmp(key, "shifts") == 0)
    PUSH4INT(L, self->Rshift, self->Gshift, self->Bshift, self->Ashift);
  else if (strcmp(key, "losses") == 0)
    PUSH4INT(L, self->Rloss, self->Gloss, self->Bloss, self->Aloss);
  else if (strcmp(key, "blit_hw") == 0)
    lua_pushinteger(L, self->blit_hw);
  else if (strcmp(key, "blit_hw_CC") == 0)
    lua_pushinteger(L, self->blit_hw_CC);
  else if (strcmp(key, "blit_hw_A") == 0)
    lua_pushinteger(L, self->blit_hw_A);
  else if (strcmp(key, "blit_sw") == 0)
    lua_pushinteger(L, self->blit_sw);
  else if (strcmp(key, "blit_sw_CC") == 0)
    lua_pushinteger(L, self->blit_sw_CC);
  else if (strcmp(key, "blit_sw_A") == 0)
    lua_pushinteger(L, self->blit_sw_A);
  else if (strcmp(key, "current_w") == 0)
    lua_pushinteger(L, self->current_w);
  else if (strcmp(key, "current_h") == 0)
    lua_pushinteger(L, self->current_h);
  else if (strcmp(key, "pixel_format") == 0)
    lua_pushstring(L, SDL_GetPixelFormatName(self->pixel_format));
  else
    return luaL_error(L, "invalid '%s' key", key);

  return 1;
}

static int VideoInfo__tostring(lua_State* L) {
  lgame_VideoInfo* self = lgame_checkvideoinfo(L, 1);
  char buffer[512];
  snprintf(buffer, 512, 
    "<VideoInfo(hw = %d, wm = %d, video_mem = %d,\n"                    
    "           blit_hw = %d, blit_hw_CC = %d, blit_hw_A = %d,\n"
    "           blit_sw = %d, blit_sw_CC = %d, blit_sw_A = %d,\n"
    "           bitsize = %d, bytesize = %d,\n"
    "           masks = (0x%08X, 0x%08X, 0x%08X, 0x%08X),\n"
    "           shifts = (%d, %d, %d, %d),\n"
    "           losses = (%d, %d, %d, %d),\n"
    "           current_w = %d, current_h = %d,\n"
    "           pixel_format = %s)>\n",
    self->hw, self->wm, self->video_mem,
    self->blit_hw, self->blit_hw_CC, self->blit_hw_A,
    self->blit_sw, self->blit_sw_CC, self->blit_sw_A,
    self->bitsize, self->bytesize,
    self->Rmask, self->Gmask, self->Bmask, self->Amask,
    self->Rshift, self->Gshift, self->Bshift, self->Ashift,
    self->Rloss, self->Gloss, self->Bloss, self->Aloss,
    self->current_w, self->current_h,
    SDL_GetPixelFormatName(self->pixel_format));
  lua_pushstring(L, buffer);
  return 1;
}

static int display_get_wm_info(lua_State* L) {
  return 0;
}

static int display_get_desktop_sizes(lua_State* L) {
  int num_displays;
  SDL_DisplayID* displays = SDL_GetDisplays(&num_displays);
  if (displays == NULL) {
    return luaL_error(L, SDL_GetError());
  }
  SDL_DisplayID* display = displays;
  int i = 1;
  lua_createtable(L, num_displays, 0);
  while (*display != 0) {
    const SDL_DisplayMode* dm = SDL_GetDesktopDisplayMode(*display);
    if (dm == NULL) {
      SDL_free(displays);
      return luaL_error(L, SDL_GetError());
    }
    lua_createtable(L, 2, 0);
    lua_pushinteger(L, dm->w);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, dm->h);
    lua_rawseti(L, -2, 2);
    lua_rawseti(L, -2, i);
    display++;
    i++;
  }

  SDL_free(displays);
  return 1;
}

static int display_list_modes(lua_State* L) {
  int depth = (int)luaL_optinteger(L, 1, 0);
  int flags = (int)luaL_optinteger(L, 2, LGAME_FULLSCREEN); (void)flags;
  int display = (int)luaL_optinteger(L, 3, 0);

  int num_displays;
  SDL_DisplayID *displays = SDL_GetDisplays(&num_displays);
  if (displays == NULL)
    return luaL_error(L, SDL_GetError());

  if (display < 0 || display >= num_displays) {
    SDL_free(displays);
    const char* message = lua_pushfstring(L, "the display should be between 0 and %d", num_displays-1);
    return luaL_argerror(L, 3, message);
  }

  SDL_DisplayID display_id = displays[display];
  SDL_free(displays);

  if (depth == 0) {
    const SDL_DisplayMode *dm = SDL_GetCurrentDisplayMode(display_id);
    if (dm == NULL)
      return luaL_error(L, SDL_GetError());

    depth = SDL_BITSPERPIXEL(dm->format);
  }

  int num_modes;
  SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display_id, &num_modes);
  if (modes == NULL)
    return luaL_error(L, SDL_GetError());

  int i = 1;
  int last_width = -1;
  int last_height = -1;
  SDL_DisplayMode** mode = modes;
  lua_createtable(L, num_modes, 0);
  while (*mode != NULL) {
    if (SDL_BITSPERPIXEL((*mode)->format) != depth) {
      mode++;
      continue;
    }

    if (last_width == (*mode)->w && last_height == (*mode)->h) {
      mode++;
      continue;
    }

    lua_createtable(L, 2, 0);
    lua_pushinteger(L, (*mode)->w);
    lua_rawseti(L, -2, 1);
    lua_pushinteger(L, (*mode)->h);
    lua_rawseti(L, -2, 2);
    lua_rawseti(L, -2, i);

    last_width = (*mode)->w;
    last_height = (*mode)->h;

    mode++;
    i++;
  }

  SDL_free(modes);
  return 1;
}

static int display_get_active(lua_State* L) {
  if (lgame_Window == NULL) {
    lua_pushboolean(L, 0);
    return 1;
  }

  SDL_WindowFlags flags = SDL_GetWindowFlags(lgame_Window);
  lua_pushboolean(L,
    !(flags & SDL_WINDOW_HIDDEN) &&
    !(flags & SDL_WINDOW_MINIMIZED));
  return 1;
}

static int display_iconify(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  bool ok = SDL_MinimizeWindow(lgame_Window);
  lua_pushboolean(L, ok);
  return 1;
}

static int display_toggle_fullscreen(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  SDL_WindowFlags flags = SDL_GetWindowFlags(lgame_Window);
  if (flags & SDL_WINDOW_FULLSCREEN) {
    if (!SDL_SetWindowFullscreen(lgame_Window, false)) {
      return luaL_error(L, SDL_GetError());
    }

    int current_w;
    int current_h;
    if (!SDL_GetWindowSize(lgame_Window, &current_w, &current_h))
      return luaL_error(L, SDL_GetError());

    if (last_window_width != current_w || last_window_height != current_h)
      if (!SDL_SetWindowSize(lgame_Window, last_window_width, last_window_height))
        return luaL_error(L, SDL_GetError());
  }
  else {
    SDL_DisplayID display = SDL_GetDisplayForWindow(lgame_Window);
    if (display == 0)
      return luaL_error(L, SDL_GetError());

    int num_modes;
    SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(display, &num_modes);
    if (modes == NULL)
      return luaL_error(L, SDL_GetError());

    int current_w = lgame_WindowSurface->w;
    int current_h = lgame_WindowSurface->h;

    last_window_width = current_w;
    last_window_height = current_h;

    SDL_DisplayMode* smallest_mode = modes[num_modes-1];
    int smallest_w = smallest_mode->w;
    int smallest_h = smallest_mode->h;
    if (current_w < smallest_w)
      current_w = smallest_w;
    if (current_h < smallest_h)
      current_h = smallest_h;

    SDL_DisplayMode fullscreen_mode;
    SDL_zero(fullscreen_mode);

    int mode_find = 0;
    SDL_DisplayMode** mode = modes;
    while (*mode != NULL) {
      if (current_w == (*mode)->w && current_h == (*mode)->h) {
        mode_find = 1;
        memcpy(&fullscreen_mode, *mode, sizeof(fullscreen_mode));
        break;
      }
      mode++;
    }

    SDL_free(modes);
    
    if (!mode_find)
      if (!SDL_GetClosestFullscreenDisplayMode(display, current_w, current_h, .0f, false, &fullscreen_mode))
        return luaL_error(L, SDL_GetError());

    if (!SDL_SetWindowFullscreenMode(lgame_Window, &fullscreen_mode))
      return luaL_error(L, SDL_GetError());

    if (!SDL_SetWindowFullscreen(lgame_Window, true)) {
      return luaL_error(L, SDL_GetError());
    }
  }

  lgame_WindowSurface = SDL_GetWindowSurface(lgame_Window);
  if (!lgame_WindowSurface)
    return luaL_error(L, SDL_GetError());

  lgame_Surface* ws;
  lgame_getwindowsurface(L);
  ws = (lgame_Surface*)lua_touserdata(L, -1);
  ws->surf = lgame_WindowSurface;
  lua_pop(L, 1);

  lua_pushinteger(L, 1);
  return 1;
}

static int display_set_icon(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  SDL_Surface* icon = lgame_checksurface(L, 1)->surf;
  if (!SDL_SetWindowIcon(lgame_Window, icon))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int display_set_caption(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  const char* title = luaL_checkstring(L, 1);
  if (!SDL_SetWindowTitle(lgame_Window, title))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int display_get_caption(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  lua_pushstring(L, SDL_GetWindowTitle(lgame_Window));
  return 1;
}

static int display_is_fullscreen(lua_State* L) {
  if (lgame_Window == NULL) {
    return luaL_error(L, "no open window");
  }

  SDL_WindowFlags flags = SDL_GetWindowFlags(lgame_Window);
  int fullscreen = flags & SDL_WINDOW_FULLSCREEN;
  lua_pushboolean(L, fullscreen);

  return 1;
}