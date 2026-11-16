
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "ltransform.h"
#include "lscale2x.h"
#include "lsurface.h"
#include "lpoint.h"

static int transform_flip(lua_State* L);
static int transform_scale(lua_State* L);
static int transform_scale_by(lua_State* L);
static int transform_scale2x(lua_State* L);

const struct luaL_Reg lgame_transform_module[] = {
  {"flip", transform_flip},
  {"scale", transform_scale},
  {"scale_by", transform_scale_by},
  {"scale2x", transform_scale2x},
  {NULL, NULL}
};

static bool copysurfsettings(SDL_Surface* src, SDL_Surface* dst) {
  if (SDL_SurfaceHasRLE(src)) {
    if (!SDL_SetSurfaceRLE(dst, true)) {
      return false;
    }
  }

  if (SDL_SurfaceHasColorKey(src)) {
    Uint32 key;
    if (!SDL_GetSurfaceColorKey(src, &key)) {
      return false;
    }
    if (!SDL_SetSurfaceColorKey(dst, true, key)) {
      return false;
    }
  }

  Uint8 alpha;
  if (!SDL_GetSurfaceAlphaMod(src, &alpha)) {
    return false;
  }
  if (!SDL_SetSurfaceAlphaMod(dst, alpha)) {
    return false;
  }

  SDL_BlendMode blend;
  if (!SDL_GetSurfaceBlendMode(src, &blend)) {
    return false;
  }
  if (!SDL_SetSurfaceBlendMode(dst, blend)) {
    return false;
  }

  return true;
}

static SDL_Surface* dupsurf(SDL_Surface* src) {
  SDL_Surface* dup = SDL_DuplicateSurface(src);
  if (dup == NULL)
    return NULL;

  if (!copysurfsettings(src, dup)) {
    SDL_DestroySurface(dup);
    return NULL;
  }

  return dup;
}

static lgame_Surface* optsurface(lua_State* L, int idx, lgame_Surface* def) {
  if (lua_isnoneornil(L, idx))
    return def;
  return lgame_checksurface(L, idx);
}

static void checktwoflt(lua_State* L, int idx, float* x, float* y) {
  idx = lua_absindex(L, idx);
  luaL_checktype(L, idx, LUA_TTABLE);
  lua_rawgeti(L, idx, 1);
  *x = (float)luaL_checknumber(L, -1);
  lua_pop(L, 1);
  lua_rawgeti(L, idx, 2);
  *y = (float)luaL_checknumber(L, -1);
  lua_pop(L, 1);
}

static int transform_flip(lua_State* L) {
  lgame_Surface* src = lgame_checksurface(L, 1);
  int xflip = lua_toboolean(L, 2);
  int yflip = lua_toboolean(L, 3);

  SDL_Surface* dst = dupsurf(src->surf);
  if (dst == NULL)
    return luaL_error(L, SDL_GetError());

  SDL_FlipMode flip = SDL_FLIP_NONE;
  if (xflip) flip |= SDL_FLIP_HORIZONTAL;
  if (yflip) flip |= SDL_FLIP_VERTICAL;

  lgame_Surface* res = lgame_pushsurface(L);
  res->surf = dst;

  if (flip == SDL_FLIP_NONE)
    return 1;

  if (!SDL_FlipSurface(res->surf, flip)) {
    SDL_DestroySurface(res->surf);
    return luaL_error(L, SDL_GetError());
  }

  return 1;
}

static int transform_scale(lua_State* L) {
  lgame_Surface* src = lgame_checksurface(L, 1);
  int scale_w, scale_h;
  lgame_checkpoint2int(L, 2, &scale_w, &scale_h);
  lgame_Surface* dst = optsurface(L, 3, NULL);
  SDL_Surface* dst_surface = NULL;
  SDL_Surface* src_surface = src->surf;
  int dst_from_arg = 0;
  if (dst) {
    int src_bpp, dst_bpp;
    int dst_w, dst_h;
    src_bpp = SDL_BITSPERPIXEL(src->surf->format);
    dst_bpp = SDL_BITSPERPIXEL(dst->surf->format);
    dst_w = dst->surf->w;
    dst_h = dst->surf->h;
    if (src_bpp != dst_bpp)
      return luaL_error(L, "the depth of the source and destionation surfaces should match");
    if (scale_w != dst_w || scale_h != dst_h)
      return luaL_error(L, "the size of the source and destination surfaces should match");
    dst_surface = dst->surf;
    dst_from_arg = 1;
  }
  else {
    dst_surface = SDL_CreateSurface(scale_w, scale_h, src_surface->format);
    if (dst_surface == NULL)
      return luaL_error(L, SDL_GetError());
    if (!copysurfsettings(src_surface, dst_surface)) {
      SDL_DestroySurface(dst_surface);
      return luaL_error(L, SDL_GetError());
    }
  }
  SDL_Rect dst_rect;
  SDL_zero(dst_rect);
  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.w = scale_w;
  dst_rect.h = scale_h;
  if (!SDL_BlitSurfaceScaled(src_surface, NULL, dst_surface, &dst_rect, SDL_SCALEMODE_NEAREST)) {
    if (!dst_from_arg)
      SDL_DestroySurface(dst_surface);
    return luaL_error(L, SDL_GetError());
  }
  if (dst_from_arg) {
    lua_pushvalue(L, 3);
  }
  else {
    lgame_Surface* res = lgame_pushsurface(L);
    res->surf = dst_surface;
  }
  return 1;
}

static int transform_scale_by(lua_State* L) {
  lgame_Surface* src = lgame_checksurface(L, 1);
  float factor_w, factor_h;
  checktwoflt(L, 2, &factor_w, &factor_h);
  lgame_Surface* dst = optsurface(L, 3, NULL);
  SDL_Surface* dst_surface = NULL;
  SDL_Surface* src_surface = src->surf;
  int scale_w = (int)(src_surface->w * factor_w);
  int scale_h = (int)(src_surface->h * factor_h);
  int dst_from_arg = 0;
  if (dst) {
    int src_bpp, dst_bpp;
    int dst_w, dst_h;
    src_bpp = SDL_BITSPERPIXEL(src->surf->format);
    dst_bpp = SDL_BITSPERPIXEL(dst->surf->format);
    dst_w = dst->surf->w;
    dst_h = dst->surf->h;
    if (src_bpp != dst_bpp)
      return luaL_error(L, "the depth of the source and destionation surfaces should match");
    if (scale_w != dst_w || scale_h != dst_h)
      return luaL_error(L, "the size of the source and destination surfaces should match");
    dst_surface = dst->surf;
    dst_from_arg = 1;
  }
  else {
    dst_surface = SDL_CreateSurface(scale_w, scale_h, src_surface->format);
    if (dst_surface == NULL)
      return luaL_error(L, SDL_GetError());
    if (!copysurfsettings(src_surface, dst_surface)) {
      SDL_DestroySurface(dst_surface);
      return luaL_error(L, SDL_GetError());
    }
  }
  SDL_Rect dst_rect;
  SDL_zero(dst_rect);
  dst_rect.x = 0;
  dst_rect.y = 0;
  dst_rect.w = scale_w;
  dst_rect.h = scale_h;
  if (!SDL_BlitSurfaceScaled(src_surface, NULL, dst_surface, &dst_rect, SDL_SCALEMODE_NEAREST)) {
    if (!dst_from_arg)
      SDL_DestroySurface(dst_surface);
    return luaL_error(L, SDL_GetError());
  }
  if (dst_from_arg) {
    lua_pushvalue(L, 3);
  }
  else {
    lgame_Surface* res = lgame_pushsurface(L);
    res->surf = dst_surface;
  }
  return 1;
}

static int transform_scale2x(lua_State* L) {
  lgame_Surface* src = lgame_checksurface(L, 1);
  lgame_Surface* dst = optsurface(L, 2, NULL);

  lgame_scale2x_function scale2x_function = NULL;
  switch (SDL_BYTESPERPIXEL(src->surf->format)) {
  case (4):
    scale2x_function = lgame_scale2x_4bpp;
    break;
  case (3):
    scale2x_function = lgame_scale2x_3bpp;
    break;
  case (2):
    scale2x_function = lgame_scale2x_2bpp;
    break;
  case (1):
    scale2x_function = lgame_scale2x_1bpp;
    break;
  default:
    return luaL_error(L, "unsupport pixel format");
  }
  lgame_Surface* res = lgame_pushsurface(L);
  res->surf = scale2x_function(src->surf, dst ? dst->surf : NULL);
  if (res->surf == NULL)
    return luaL_error(L, SDL_GetError());

  return 1;
}