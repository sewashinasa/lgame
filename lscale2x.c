
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lscale2x.h"
#include "lsurface.h"

#define GET_PIXEL_4BPP(pixels, x, y, pitch) \
  *(Uint32*)(pixels + ((y) * (pitch)) + ((x) * 4))

#define SET_PIXEL_4BPP(pixels, x, y, pitch, pixel) \
  *(Uint32*)(pixels + ((y) * (pitch)) + ((x) * 4)) = pixel

#define GET_PIXEL_2BPP(pixels, x, y, pitch) \
  *(Uint16*)(pixels + ((y) * (pitch)) + ((x) * 2))

#define SET_PIXEL_2BPP(pixels, x, y, pitch, pixel) \
  *(Uint16*)(pixels + ((y) * (pitch)) + ((x) * 2)) = pixel

#define GET_PIXEL_1BPP(pixels, x, y, pitch) \
  *(pixels + ((y) * (pitch)) + ((x) * 1))

#define SET_PIXEL_1BPP(pixels, x, y, pitch, pixel) \
  *(pixels + ((y) * (pitch)) + ((x) * 1)) = pixel

static Uint32 GET_PIXEL_3BPP(Uint8* pixels, int x, int y, int pitch) {
  Uint8* p = pixels + (y * pitch) + (x * 3);
  Uint32 Rval = p[0] << 16;
  Uint32 Gval = p[1] << 8;
  Uint32 Bval = p[2];
  Uint32 res = Rval | Gval | Bval;
  return res;
}

static void SET_PIXEL_3BPP(Uint8* pixels, int x, int y, int pitch, Uint32 pixel) {
  Uint32 Rval = (pixel >> 16) & 0xFF;
  Uint32 Gval = (pixel >> 8) & 0xFF;
  Uint32 Bval = pixel & 0xFF;
  Uint8* p = pixels + (y * pitch) + (x * 3);
  p[0] = Rval;
  p[1] = Gval;
  p[2] = Bval;
}

SDL_Surface* lgame_scale2x_4bpp(SDL_Surface *src, SDL_Surface* dst) {
  int dst_needs_free = dst == NULL;

  if (SDL_MUSTLOCK(src)) {
    if (!SDL_LockSurface(src)) {
      return NULL;
    }
  }

  if (dst && SDL_MUSTLOCK(dst)) {
    if (!SDL_LockSurface(dst)) {
      SDL_UnlockSurface(src);
      return NULL;
    }
  }

  int src_w = src->w;
  int src_h = src->h;
  int src_pitch = src->pitch;
  Uint8* src_pixels = (Uint8*)src->pixels;
  SDL_PixelFormat src_format = src->format;

  int dst_w = 0;
  int dst_h = 0;
  int dst_pitch = 0;
  Uint8* dst_pixels = NULL;
  SDL_PixelFormat dst_format = 0;

  if (dst != NULL) {
    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }

  if (dst == NULL) {
    dst = SDL_CreateSurface(src_w * 2, src_h * 2, src_format);
    if (dst == NULL)
      return NULL;

    if (SDL_MUSTLOCK(dst)) {
      if (!SDL_LockSurface(dst)) {
        SDL_DestroySurface(dst);
        SDL_UnlockSurface(src);
        return NULL;
      }
    }

    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }
  else {
    if (dst_w != (src_w * 2) || dst_h != (src_h * 2)) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the size of the source and destination surfaces should match");
      return NULL;
    }
    else if (dst_format != src_format) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the pixel format of the source and destination surfaces should match");
      return NULL;
    }
  }

  for (int y = 0; y < src_h; y++) {
    for (int x = 0; x < src_w; x++) {

      int safe_x = x;
      int safe_y = y;
      
      if ((x + 2) >= src_w)
        safe_x -= 2;
      else if ((x + 1) >= src_w)
        safe_x -= 1;
      
      if ((y + 2) >= src_h)
        safe_y -= 2;
      else if ((y + 1) >= src_h)
        safe_y -= 1;
      
      
      Uint32 A = GET_PIXEL_4BPP(src_pixels, safe_x,   safe_y, src_pitch);
      Uint32 B = GET_PIXEL_4BPP(src_pixels, safe_x+1, safe_y, src_pitch);
      Uint32 C = GET_PIXEL_4BPP(src_pixels, safe_x+2, safe_y, src_pitch);

      Uint32 D = GET_PIXEL_4BPP(src_pixels, safe_x,   safe_y+1, src_pitch);
      Uint32 E = GET_PIXEL_4BPP(src_pixels, safe_x+1, safe_y+1, src_pitch);
      Uint32 F = GET_PIXEL_4BPP(src_pixels, safe_x+2, safe_y+1, src_pitch);

      Uint32 G = GET_PIXEL_4BPP(src_pixels, safe_x,   safe_y+2, src_pitch);
      Uint32 H = GET_PIXEL_4BPP(src_pixels, safe_x+1, safe_y+2, src_pitch);
      Uint32 I = GET_PIXEL_4BPP(src_pixels, safe_x+2, safe_y+2, src_pitch);

      Uint32 E0, E1, E2, E3;
      if (B != H && D != F) {
        E0 = D == B ? D : E;
        E1 = B == F ? F : E;
        E2 = D == H ? D : E;
        E3 = H == F ? F : E;
      }
      else {
        E0 = E;
        E1 = E;
        E2 = E;
        E3 = E;
      }

      int dst_x = x * 2;
      int dst_y = y * 2;

      SET_PIXEL_4BPP(dst_pixels, dst_x,   dst_y, dst_pitch, E0);
      SET_PIXEL_4BPP(dst_pixels, dst_x+1, dst_y, dst_pitch, E1);
      SET_PIXEL_4BPP(dst_pixels, dst_x,   dst_y+1, dst_pitch, E2);
      SET_PIXEL_4BPP(dst_pixels, dst_x+1, dst_y+1, dst_pitch, E3);
    }
  }

  if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
  if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

  return dst;
}

SDL_Surface* lgame_scale2x_3bpp(SDL_Surface* src, SDL_Surface* dst) {
  int dst_needs_free = dst == NULL;

  if (SDL_MUSTLOCK(src)) {
    if (!SDL_LockSurface(src)) {
      return NULL;
    }
  }

  if (dst && SDL_MUSTLOCK(dst)) {
    if (!SDL_LockSurface(dst)) {
      SDL_UnlockSurface(src);
      return NULL;
    }
  }

  int src_w = src->w;
  int src_h = src->h;
  int src_pitch = src->pitch;
  Uint8* src_pixels = (Uint8*)src->pixels;
  SDL_PixelFormat src_format = src->format;

  int dst_w = 0;
  int dst_h = 0;
  int dst_pitch = 0;
  Uint8* dst_pixels = NULL;
  SDL_PixelFormat dst_format = 0;

  if (dst != NULL) {
    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }

  if (dst == NULL) {
    dst = SDL_CreateSurface(src_w * 2, src_h * 2, src_format);
    if (dst == NULL)
      return NULL;

    if (SDL_MUSTLOCK(dst)) {
      if (!SDL_LockSurface(dst)) {
        SDL_DestroySurface(dst);
        SDL_UnlockSurface(src);
        return NULL;
      }
    }

    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }
  else {
    if (dst_w != (src_w * 2) || dst_h != (src_h * 2)) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the size of the source and destination surfaces should match");
      return NULL;
    }
    else if (dst_format != src_format) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the pixel format of the source and destination surfaces should match");
      return NULL;
    }
  }

  for (int y = 0; y < src_h; y++) {
    for (int x = 0; x < src_w; x++) {

      int safe_x = x;
      int safe_y = y;

      if ((x + 2) >= src_w)
        safe_x -= 2;
      else if ((x + 1) >= src_w)
        safe_x -= 1;

      if ((y + 2) >= src_h)
        safe_y -= 2;
      else if ((y + 1) >= src_h)
        safe_y -= 1;


      Uint32 A = GET_PIXEL_3BPP(src_pixels, safe_x, safe_y, src_pitch);
      Uint32 B = GET_PIXEL_3BPP(src_pixels, safe_x + 1, safe_y, src_pitch);
      Uint32 C = GET_PIXEL_3BPP(src_pixels, safe_x + 2, safe_y, src_pitch);

      Uint32 D = GET_PIXEL_3BPP(src_pixels, safe_x, safe_y + 1, src_pitch);
      Uint32 E = GET_PIXEL_3BPP(src_pixels, safe_x + 1, safe_y + 1, src_pitch);
      Uint32 F = GET_PIXEL_3BPP(src_pixels, safe_x + 2, safe_y + 1, src_pitch);

      Uint32 G = GET_PIXEL_3BPP(src_pixels, safe_x, safe_y + 2, src_pitch);
      Uint32 H = GET_PIXEL_3BPP(src_pixels, safe_x + 1, safe_y + 2, src_pitch);
      Uint32 I = GET_PIXEL_3BPP(src_pixels, safe_x + 2, safe_y + 2, src_pitch);

      Uint32 E0, E1, E2, E3;
      if (B != H && D != F) {
        E0 = D == B ? D : E;
        E1 = B == F ? F : E;
        E2 = D == H ? D : E;
        E3 = H == F ? F : E;
      }
      else {
        E0 = E;
        E1 = E;
        E2 = E;
        E3 = E;
      }

      int dst_x = x * 2;
      int dst_y = y * 2;

      SET_PIXEL_3BPP(dst_pixels, dst_x, dst_y, dst_pitch, E0);
      SET_PIXEL_3BPP(dst_pixels, dst_x + 1, dst_y, dst_pitch, E1);
      SET_PIXEL_3BPP(dst_pixels, dst_x, dst_y + 1, dst_pitch, E2);
      SET_PIXEL_3BPP(dst_pixels, dst_x + 1, dst_y + 1, dst_pitch, E3);
    }
  }

  if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
  if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

  return dst;
}

SDL_Surface* lgame_scale2x_2bpp(SDL_Surface* src, SDL_Surface* dst) {
  int dst_needs_free = dst == NULL;

  if (SDL_MUSTLOCK(src)) {
    if (!SDL_LockSurface(src)) {
      return NULL;
    }
  }

  if (dst && SDL_MUSTLOCK(dst)) {
    if (!SDL_LockSurface(dst)) {
      SDL_UnlockSurface(src);
      return NULL;
    }
  }

  int src_w = src->w;
  int src_h = src->h;
  int src_pitch = src->pitch;
  Uint8* src_pixels = (Uint8*)src->pixels;
  SDL_PixelFormat src_format = src->format;

  int dst_w = 0;
  int dst_h = 0;
  int dst_pitch = 0;
  Uint8* dst_pixels = NULL;
  SDL_PixelFormat dst_format = 0;

  if (dst != NULL) {
    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }

  if (dst == NULL) {
    dst = SDL_CreateSurface(src_w * 2, src_h * 2, src_format);
    if (dst == NULL)
      return NULL;

    if (SDL_MUSTLOCK(dst)) {
      if (!SDL_LockSurface(dst)) {
        SDL_DestroySurface(dst);
        SDL_UnlockSurface(src);
        return NULL;
      }
    }

    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }
  else {
    if (dst_w != (src_w * 2) || dst_h != (src_h * 2)) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the size of the source and destination surfaces should match");
      return NULL;
    }
    else if (dst_format != src_format) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the pixel format of the source and destination surfaces should match");
      return NULL;
    }
  }

  for (int y = 0; y < src_h; y++) {
    for (int x = 0; x < src_w; x++) {

      int safe_x = x;
      int safe_y = y;

      if ((x + 2) >= src_w)
        safe_x -= 2;
      else if ((x + 1) >= src_w)
        safe_x -= 1;

      if ((y + 2) >= src_h)
        safe_y -= 2;
      else if ((y + 1) >= src_h)
        safe_y -= 1;


      Uint32 A = GET_PIXEL_2BPP(src_pixels, safe_x, safe_y, src_pitch);
      Uint32 B = GET_PIXEL_2BPP(src_pixels, safe_x + 1, safe_y, src_pitch);
      Uint32 C = GET_PIXEL_2BPP(src_pixels, safe_x + 2, safe_y, src_pitch);

      Uint32 D = GET_PIXEL_2BPP(src_pixels, safe_x, safe_y + 1, src_pitch);
      Uint32 E = GET_PIXEL_2BPP(src_pixels, safe_x + 1, safe_y + 1, src_pitch);
      Uint32 F = GET_PIXEL_2BPP(src_pixels, safe_x + 2, safe_y + 1, src_pitch);

      Uint32 G = GET_PIXEL_2BPP(src_pixels, safe_x, safe_y + 2, src_pitch);
      Uint32 H = GET_PIXEL_2BPP(src_pixels, safe_x + 1, safe_y + 2, src_pitch);
      Uint32 I = GET_PIXEL_2BPP(src_pixels, safe_x + 2, safe_y + 2, src_pitch);

      Uint32 E0, E1, E2, E3;
      if (B != H && D != F) {
        E0 = D == B ? D : E;
        E1 = B == F ? F : E;
        E2 = D == H ? D : E;
        E3 = H == F ? F : E;
      }
      else {
        E0 = E;
        E1 = E;
        E2 = E;
        E3 = E;
      }

      int dst_x = x * 2;
      int dst_y = y * 2;

      SET_PIXEL_2BPP(dst_pixels, dst_x, dst_y, dst_pitch, E0);
      SET_PIXEL_2BPP(dst_pixels, dst_x + 1, dst_y, dst_pitch, E1);
      SET_PIXEL_2BPP(dst_pixels, dst_x, dst_y + 1, dst_pitch, E2);
      SET_PIXEL_2BPP(dst_pixels, dst_x + 1, dst_y + 1, dst_pitch, E3);
    }
  }

  if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
  if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

  return dst;
}

SDL_Surface* lgame_scale2x_1bpp(SDL_Surface* src, SDL_Surface* dst) {
  int dst_needs_free = dst == NULL;

  if (SDL_MUSTLOCK(src)) {
    if (!SDL_LockSurface(src)) {
      return NULL;
    }
  }

  if (dst && SDL_MUSTLOCK(dst)) {
    if (!SDL_LockSurface(dst)) {
      SDL_UnlockSurface(src);
      return NULL;
    }
  }

  int src_w = src->w;
  int src_h = src->h;
  int src_pitch = src->pitch;
  Uint8* src_pixels = (Uint8*)src->pixels;
  SDL_PixelFormat src_format = src->format;

  int dst_w = 0;
  int dst_h = 0;
  int dst_pitch = 0;
  Uint8* dst_pixels = NULL;
  SDL_PixelFormat dst_format = 0;

  if (dst != NULL) {
    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }

  if (dst == NULL) {
    dst = SDL_CreateSurface(src_w * 2, src_h * 2, src_format);
    if (dst == NULL)
      return NULL;

    if (SDL_MUSTLOCK(dst)) {
      if (!SDL_LockSurface(dst)) {
        SDL_DestroySurface(dst);
        SDL_UnlockSurface(src);
        return NULL;
      }
    }

    dst_w = dst->w;
    dst_h = dst->h;
    dst_pitch = dst->pitch;
    dst_pixels = (Uint8*)dst->pixels;
    dst_format = dst->format;
  }
  else {
    if (dst_w != (src_w * 2) || dst_h != (src_h * 2)) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the size of the source and destination surfaces should match");
      return NULL;
    }
    else if (dst_format != src_format) {
      if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
      if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);
      if (dst_needs_free) SDL_DestroySurface(dst);
      SDL_SetError("the pixel format of the source and destination surfaces should match");
      return NULL;
    }
  }

  for (int y = 0; y < src_h; y++) {
    for (int x = 0; x < src_w; x++) {

      int safe_x = x;
      int safe_y = y;

      if ((x + 2) >= src_w)
        safe_x -= 2;
      else if ((x + 1) >= src_w)
        safe_x -= 1;

      if ((y + 2) >= src_h)
        safe_y -= 2;
      else if ((y + 1) >= src_h)
        safe_y -= 1;


      Uint32 A = GET_PIXEL_1BPP(src_pixels, safe_x, safe_y, src_pitch);
      Uint32 B = GET_PIXEL_1BPP(src_pixels, safe_x + 1, safe_y, src_pitch);
      Uint32 C = GET_PIXEL_1BPP(src_pixels, safe_x + 2, safe_y, src_pitch);

      Uint32 D = GET_PIXEL_1BPP(src_pixels, safe_x, safe_y + 1, src_pitch);
      Uint32 E = GET_PIXEL_1BPP(src_pixels, safe_x + 1, safe_y + 1, src_pitch);
      Uint32 F = GET_PIXEL_1BPP(src_pixels, safe_x + 2, safe_y + 1, src_pitch);

      Uint32 G = GET_PIXEL_1BPP(src_pixels, safe_x, safe_y + 2, src_pitch);
      Uint32 H = GET_PIXEL_1BPP(src_pixels, safe_x + 1, safe_y + 2, src_pitch);
      Uint32 I = GET_PIXEL_1BPP(src_pixels, safe_x + 2, safe_y + 2, src_pitch);

      Uint32 E0, E1, E2, E3;
      if (B != H && D != F) {
        E0 = D == B ? D : E;
        E1 = B == F ? F : E;
        E2 = D == H ? D : E;
        E3 = H == F ? F : E;
      }
      else {
        E0 = E;
        E1 = E;
        E2 = E;
        E3 = E;
      }

      int dst_x = x * 2;
      int dst_y = y * 2;

      SET_PIXEL_1BPP(dst_pixels, dst_x, dst_y, dst_pitch, E0);
      SET_PIXEL_1BPP(dst_pixels, dst_x + 1, dst_y, dst_pitch, E1);
      SET_PIXEL_1BPP(dst_pixels, dst_x, dst_y + 1, dst_pitch, E2);
      SET_PIXEL_1BPP(dst_pixels, dst_x + 1, dst_y + 1, dst_pitch, E3);
    }
  }

  if (SDL_MUSTLOCK(src)) SDL_UnlockSurface(src);
  if (SDL_MUSTLOCK(dst)) SDL_UnlockSurface(dst);

  return dst;
}