
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
static int transform_matrixcolor(lua_State* L);

const struct luaL_Reg lgame_transform_module[] = {
  {"flip", transform_flip},
  {"scale", transform_scale},
  {"scale_by", transform_scale_by},
  {"scale2x", transform_scale2x},
  {"matrixcolor", transform_matrixcolor},
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

  SDL_Surface *dst_surface = scale2x_function(src->surf, dst ? dst->surf : NULL);
  if (dst_surface == NULL)
    return luaL_error(L, SDL_GetError());

  if (dst) {
    lua_pushvalue(L, 2);
  } 
  else {
    lgame_Surface* res = lgame_pushsurface(L);
    res->surf = dst_surface;
    if (res->surf == NULL)
      return luaL_error(L, SDL_GetError());
  }

  return 1;
}

static inline Uint8 clamp_rgba(double v) {
  return (v < 0.0) ? (Uint8)0 : (v > 1.0) ? (Uint8)255 : (Uint8)(v * 255);
}

static int transform_matrixcolor(lua_State* L) {
  lgame_Surface* src = lgame_checksurface(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  lgame_Surface* dst = optsurface(L, 3, NULL);

  double mc[4][5];
  memset(mc, 0, sizeof(mc));

  int tlen_col;
  int tlen_row;

  tlen_col = (int)lua_rawlen(L, 2);
  luaL_argcheck(L, tlen_col == 4, 2, "invalid matrix table");

  for (int i = 0; i < tlen_col; i++) {
    lua_rawgeti(L, 2, (lua_Integer)(i + 1));
    luaL_checktype(L, -1, LUA_TTABLE);
    tlen_row = (int)lua_rawlen(L, -1);
    luaL_argcheck(L, tlen_row == 5, 2, "invalid matrix table");
    for (int j = 0; j < tlen_row; j++) {
      lua_rawgeti(L, -1, (lua_Integer)(j + 1));
      mc[i][j] = luaL_checknumber(L, -1);
      lua_pop(L, 1);
    }
    lua_pop(L, 1);
  }

  SDL_Surface* dst_surface = NULL;
  SDL_Surface* src_surface = src->surf;
  int dst_from_arg = 0;

  int dst_w;
  int dst_h;

  if (dst) {
    int src_format, dst_format;
    src_format = src->surf->format;
    dst_format = dst->surf->format;
    dst_w = dst->surf->w;
    dst_h = dst->surf->h;
    if (src_format != dst_format)
      return luaL_error(L, "the format of the source and destionation surfaces should match");
    if (src->surf->w != dst_w || src->surf->h != dst_h)
      return luaL_error(L, "the size of the source and destination surfaces should match");
    dst_surface = dst->surf;
    dst_from_arg = 1;
  }
  else {
    dst_w = src->surf->w;
    dst_h = src->surf->h;
    dst_surface = SDL_CreateSurface(dst_w, dst_h, src_surface->format);
    if (dst_surface == NULL)
      return luaL_error(L, SDL_GetError());
    if (!copysurfsettings(src_surface, dst_surface)) {
      SDL_DestroySurface(dst_surface);
      return luaL_error(L, SDL_GetError());
    }
  }

  if (SDL_MUSTLOCK(src_surface)) {
    if (!SDL_LockSurface(src_surface)) {
      if (!dst_from_arg) {
        SDL_DestroySurface(dst_surface);
      }
      return luaL_error(L, SDL_GetError());
    }
  }

  if (SDL_MUSTLOCK(dst_surface)) {
    if (!SDL_LockSurface(dst_surface)) {
      if (SDL_MUSTLOCK(src_surface)) {
        SDL_UnlockSurface(src_surface);
      }
      if (!dst_from_arg) {
        SDL_DestroySurface(dst_surface);
      }
      return luaL_error(L, SDL_GetError());
    }
  }

  Uint8* src_pixels = (Uint8*)src_surface->pixels;
  Uint8* dst_pixels = (Uint8*)dst_surface->pixels;
  int src_pitch = src_surface->pitch;
  int dst_pitch = dst_surface->pitch;
  int bpp = SDL_BYTESPERPIXEL(dst_surface->format);
  int width = dst_surface->w;
  int height = dst_surface->h;
  const SDL_PixelFormatDetails* pfd = SDL_GetPixelFormatDetails(dst_surface->format);
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      Uint8* pixel_offset;
      Uint32 pixel;
      Uint8 Rcolor, Gcolor, Bcolor, Acolor;
      double Rnorm, Gnorm, Bnorm, Anorm;
      switch (bpp) {
      case (4):
        pixel = *(Uint32*)(src_pixels + y * src_pitch + x * bpp);
        SDL_GetRGBA(pixel, pfd, NULL, &Rcolor, &Gcolor, &Bcolor, &Acolor);
        break;
      case (3):
        pixel_offset = src_pixels + y * src_pitch + x *bpp;
        if (pfd->Rmask == 0xFF0000) {
          Rcolor = pixel_offset[0];
          Gcolor = pixel_offset[1];
          Bcolor = pixel_offset[2];
        }
        else {
          Rcolor = pixel_offset[2];
          Gcolor = pixel_offset[1];
          Bcolor = pixel_offset[0];
        }
        Acolor = 255;
        break;
      case (2):
        pixel = *(Uint16*)(src_pixels + y * src_pitch + x * bpp);
        SDL_GetRGBA(pixel, pfd, NULL, &Rcolor, &Gcolor, &Bcolor, &Acolor);
        break;
      case (1):
      default:
        if (SDL_MUSTLOCK(src_surface)) {
          SDL_UnlockSurface(src_surface);
        }
        if (SDL_MUSTLOCK(dst_surface)) {  
          SDL_UnlockSurface(dst_surface);
        }
        if (!dst_from_arg) {
          SDL_DestroySurface(dst_surface);
        }
        return luaL_error(L, "unknown or unsupported pixel format");
      }

      Rnorm = Rcolor / 255.0;
      Gnorm = Gcolor / 255.0;
      Bnorm = Bcolor / 255.0;
      Anorm = Acolor / 255.0;

      Rcolor = clamp_rgba(mc[0][0] * Rnorm + mc[0][1] * Gnorm + mc[0][2] * Bnorm + mc[0][3] * Anorm + mc[0][4]);
      Gcolor = clamp_rgba(mc[1][0] * Rnorm + mc[1][1] * Gnorm + mc[1][2] * Bnorm + mc[1][3] * Anorm + mc[1][4]);
      Bcolor = clamp_rgba(mc[2][0] * Rnorm + mc[2][1] * Gnorm + mc[2][2] * Bnorm + mc[2][3] * Anorm + mc[2][4]);
      Acolor = clamp_rgba(mc[3][0] * Rnorm + mc[3][1] * Gnorm + mc[3][2] * Bnorm + mc[3][3] * Anorm + mc[3][4]);

      switch (bpp) {
      case (4):
        *(Uint32*)(dst_pixels + y * dst_pitch + x * bpp)
          = SDL_MapRGBA(pfd, NULL, Rcolor, Gcolor, Bcolor, Acolor);
        break;
      case (3):
        pixel_offset = dst_pixels + y * dst_pitch + x * bpp;
        if (pfd->Rmask == 0xFF0000) {
          pixel_offset[0] = Rcolor;
          pixel_offset[1] = Gcolor;
          pixel_offset[2] = Bcolor;
        }
        else {
          pixel_offset[2] = Rcolor;
          pixel_offset[1] = Gcolor;
          pixel_offset[0] = Bcolor;
        }
        break;
      case (2):
        *(Uint16*)(dst_pixels + y * dst_pitch + x * bpp)
          = SDL_MapRGBA(pfd, NULL, Rcolor, Gcolor, Bcolor, Acolor);
        break;
      case (1):
      default:
        if (SDL_MUSTLOCK(src_surface)) {
          SDL_UnlockSurface(src_surface);
        }
        if (SDL_MUSTLOCK(dst_surface)) {
          SDL_UnlockSurface(dst_surface);
        }
        if (!dst_from_arg) {
          SDL_DestroySurface(dst_surface);
        }
        return luaL_error(L, "unknown or unsupported pixel format");
      }
    }
  }

  if (SDL_MUSTLOCK(src_surface)) {
    SDL_UnlockSurface(src_surface);
  }
  if (SDL_MUSTLOCK(dst_surface)) {
    SDL_UnlockSurface(dst_surface);
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