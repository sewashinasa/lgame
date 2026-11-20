

#ifndef LDISPLAY_H_
#define LDISPLAY_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#define LGAME_VIDEOINFO_METATABLE "lgame.VideoInfo"

#define LGAME_WINDOWSURFACE_KEY "LGAME_WINDOWSURFACE"
#define lgame_getwindowsurface(L) lua_getfield((L), LUA_REGISTRYINDEX, LGAME_WINDOWSURFACE_KEY)
#define lgame_setwindowsurface(L) lua_setfield((L), LUA_REGISTRYINDEX, LGAME_WINDOWSURFACE_KEY)

typedef struct lgame_VideoInfo {
  int hw;
  int wm;
  int video_mem;
  int bitsize;
  int bytesize;

  Uint32 Rmask, Gmask, Bmask, Amask;
  Uint32 Rshift, Gshift, Bshift, Ashift;
  Uint32 Rloss, Gloss, Bloss, Aloss;

  int blit_hw;
  int blit_hw_CC;
  int blit_hw_A;

  int blit_sw;
  int blit_sw_CC;
  int blit_sw_A;

  int current_w;
  int current_h;

  SDL_PixelFormat pixel_format;

} lgame_VideoInfo;

extern const struct luaL_Reg lgame_display_module[];

const struct luaL_Reg lgame_VideoInfo_metamethods[];

lgame_VideoInfo* lgame_pushvideoinfo(lua_State* L);
lgame_VideoInfo* lgame_checkvideoinfo(lua_State* L, int idx);

SDL_Surface* lgame_get_windowsurface(void);
SDL_Window* lgame_get_window(void);

#endif