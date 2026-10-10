
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lsurface.h"
#include "ldisplay.h"
#include "lrect.h"
#include "lconsts.h"
#include "lmethod.h"


static int Surface_new(lua_State* L);
static int Surface_blit(lua_State* L);
static int Surface_get_size(lua_State* L);
static int Surface_get_width(lua_State* L);
static int Surface_get_height(lua_State* L);
static int Surface_get_bytesize(lua_State* L);
static int Surface_get_bitsize(lua_State* L);
static int Surface_get_masks(lua_State* L);
static int Surface_get_shifts(lua_State* L);
static int Surface__index(lua_State* L);
static int Surface__tostring(lua_State* L);
static int Surface__gc(lua_State* L);

const struct luaL_Reg lg_surface_module[] = {
  {"Surface", Surface_new},
  {NULL, NULL}
};

const struct luaL_Reg lg_Surface_methods[] = {
  {"blit", Surface_blit},
  {"get_size", Surface_get_size},
  {"get_width", Surface_get_width},
  {"get_height", Surface_get_height},
  {"get_bytesize", Surface_get_bytesize},
  {"get_bitsize", Surface_get_bitsize},
  {"get_masks", Surface_get_masks},
  {"get_shifts",Surface_get_shifts},
  {NULL, NULL}
};

const struct luaL_Reg lg_Surface_metamethods[] = {
  {"__index", Surface__index},
  {"__tostring", Surface__tostring},
  {"__gc", Surface__gc},
  { NULL, NULL }
};


lSurface* lg_pushsurface(lua_State* L) {
  lSurface* s;
  s = (lSurface*)lua_newuserdata(L, sizeof(lSurface));
  s->surf = NULL;
  s->isws = 0;
  luaL_getmetatable(L, LGAME_SURFACE_METATABLE);
  lua_setmetatable(L, -2);
  return s;
}

lSurface* lg_checksurface(lua_State* L, int idx) {
  lSurface *s = (lSurface*)luaL_checkudata(L, idx, LGAME_SURFACE_METATABLE);
  if (!s->surf) luaL_error(L, "invalid lgame.Surface: surface is NULL");
  return s;
}

lSurface* lg_testsurface(lua_State* L, int idx) {
  return (lSurface*)luaL_testudata(L, idx, LGAME_SURFACE_METATABLE);
}

static int counttruebits(Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask) {
  Uint32 masks = Rmask | Gmask | Bmask | Amask;
  int count = 0;
  while (masks) {
    count += (masks & 1);
    masks >>= 1;
  }
  return count;
}

static SDL_PixelFormat dispfmtalpha(void)
{
  SDL_Surface* dispsurf;
  SDL_PixelFormat dispfmt;
  Uint32 dispfmtRmask, dispfmtGmask, dispfmtBmask, dispfmtAmask;
  Uint32 pfe;
  Uint32 Amask = 0xff000000;
  Uint32 Rmask = 0x00ff0000;
  Uint32 Gmask = 0x0000ff00;
  Uint32 Bmask = 0x000000ff;
  int _empty;

  dispsurf = lg_get_windowsurface();
  if (!dispsurf) {
    SDL_SetError("no video mode has been set");
    return SDL_PIXELFORMAT_UNKNOWN;
  }

  dispfmt = dispsurf->format;
  SDL_GetMasksForPixelFormat(dispfmt, &_empty, &dispfmtRmask, &dispfmtGmask, &dispfmtBmask, &dispfmtAmask);

  switch (SDL_BYTESPERPIXEL(dispfmt)) {
  case 2:
    /* same behavior as SDL1 */
    if ((dispfmtRmask == 0x1f) &&
      (dispfmtBmask == 0xf800 || dispfmtBmask == 0x7c00)) {
      Rmask = 0xff;
      Bmask = 0xff0000;
    }
    break;
  case 3:
  case 4:
    /* keep the format if the high bits are free */
    if ((dispfmtRmask == 0xff) && (dispfmtBmask == 0xff0000)) {
      Rmask = 0xff;
      Bmask = 0xff0000;
    }
    else if (dispfmtRmask == 0xff00 &&
      (dispfmtBmask == 0xff000000)) {
      Amask = 0x000000ff;
      Rmask = 0x0000ff00;
      Gmask = 0x00ff0000;
      Bmask = 0xff000000;
    }
    break;
  default: /* ARGB8888 */
    break;
  }

  pfe = SDL_GetPixelFormatForMasks(32, Rmask, Gmask, Bmask, Amask);
  if (pfe == SDL_PIXELFORMAT_UNKNOWN) {
    SDL_SetError("unknown pixel format");
    return SDL_PIXELFORMAT_UNKNOWN;
  }

  return pfe;
}

static int Surface_new(lua_State* L) {
  int depth_passed = 0;
  int masks_passed = 0;
  int sw, sh;
  Uint32 flags;
  int depth;
  Uint32 Rmask, Gmask, Bmask, Amask;
  lSurface* self;
  SDL_PixelFormat pf;

  lg_checkpoint2int(L, 1, &sw, &sh);
  flags = (Uint32)luaL_optinteger(L, 2, 0);
  depth = (int)luaL_optinteger(L, 3, -1);
  if (depth != -1) depth_passed = 1;
  if (!lua_isnoneornil(L, 4)) {
    luaL_checktype(L, 4, LUA_TTABLE); 
    lua_rawgeti(L, 4, 1);
    Rmask = (Uint32)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, 4, 2);
    Gmask = (Uint32)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, 4, 3);
    Bmask = (Uint32)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    lua_rawgeti(L, 4, 4);
    Amask = (Uint32)luaL_checkinteger(L, -1);
    lua_pop(L, 1);
    masks_passed = 1;
  }

  if (flags & LGAME_SRCALPHA) {
    pf = dispfmtalpha();
  }
  else {
    if (depth_passed && !masks_passed) {
      Rmask = 0; Gmask = 0; Bmask = 0; Amask = 0;
    }
    else if (!depth_passed && masks_passed) {
      depth = counttruebits(Rmask, Gmask, Bmask, Amask);
      if (Rmask == 0xFF000000 && /* RGBX8888 only 32-bit */
          Gmask == 0x00FF0000 &&
          Bmask == 0x0000FF00 &&
          Amask == 0x00000000)
          depth += 8;
      if (Rmask == 0x0000FF00 && /* BGRX8888 only 32-bit */
          Gmask == 0x00FF0000 &&
          Bmask == 0xFF000000 &&
          Amask == 0x00000000)
          depth += 8;
    }
    else if (!depth_passed && !masks_passed) {
      depth = 32;
      Rmask = 0; Gmask = 0; Bmask = 0; Amask = 0;
    }
    pf = SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask);
  }

  if (pf == 0) return luaL_error(L, SDL_GetError());

  self = lg_pushsurface(L);
  self->surf = SDL_CreateSurface(sw, sh, pf);
  if (!self->surf) return luaL_error(L, SDL_GetError());

  if (SDL_ISPIXELFORMAT_ALPHA(pf)) {
    if (!SDL_SetSurfaceBlendMode(self->surf, SDL_BLENDMODE_BLEND)) {
      SDL_DestroySurface(self->surf);
      return luaL_error(L, SDL_GetError());
    }
  }

  return 1;
}

static int Surface_blit(lua_State* L) {
  lSurface* self;
  lSurface* src;
  SDL_Surface* _src;
  SDL_Surface* _dst;
  int dstx, dsty;
  SDL_Rect* pdstrect, * psrcrect;
  SDL_Rect dstrect, srcrect;
  self = lg_checksurface(L, 1); 
  src = lg_checksurface(L, 2);
  _src = src->surf;
  _dst = self->surf;
  lg_checkpoint2int(L, 3, &dstx, &dsty);
  dstrect.x = dstx; 
  dstrect.y = dsty;
  dstrect.w = 0; 
  dstrect.h = 0;
  pdstrect = &dstrect;
  if (!lua_isnoneornil(L, 4)) {
    srcrect = lg_checkrectarg(L, 4);
    psrcrect = &srcrect;
  }
  else {
    psrcrect = NULL;
  }
  if (!SDL_BlitSurface(_src, psrcrect, _dst, pdstrect)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int Surface_get_size(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  lg_pushpoint2int(L, self->surf->w, self->surf->h);
  return 1;
}

static int Surface_get_width(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  lua_pushinteger(L, self->surf->w);
  return 1;
}

static int Surface_get_height(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  lua_pushinteger(L, self->surf->h);
  return 1;
}

static int Surface_get_bytesize(lua_State *L) {
  lSurface* self = lg_checksurface(L, 1);
  lua_pushinteger(L, SDL_BYTESPERPIXEL(self->surf->format));
  return 1;
}

static int Surface_get_bitsize(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  lua_pushinteger(L, SDL_BITSPERPIXEL(self->surf->format));
  return 1;
}

#define push4ints(L,a,b,c,d)\
  lua_createtable((L), 4, 0);\
  lua_pushinteger((L), (a));\
  lua_rawseti((L), -2, 1);\
  lua_pushinteger((L), (b));\
  lua_rawseti((L), -2, 2);\
  lua_pushinteger((L), (c));\
  lua_rawseti((L), -2, 3);\
  lua_pushinteger((L), (d));\
  lua_rawseti((L), -2, 4);\

static int Surface_get_masks(lua_State* L) {
  Uint32 Rmask, Gmask, Bmask, Amask;
  int _empty;
  lSurface* self = lg_checksurface(L, 1);
  if (!SDL_GetMasksForPixelFormat(self->surf->format, &_empty, &Rmask, &Gmask, &Bmask, &Amask))
    return luaL_error(L, SDL_GetError());
  push4ints(L, Rmask, Gmask, Bmask, Amask);
  return 1;
}

static int Surface_get_shifts(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  const SDL_PixelFormatDetails *pfd = SDL_GetPixelFormatDetails(self->surf->format);
  push4ints(L, pfd->Rshift, pfd->Gshift, pfd->Bshift, pfd->Ashift);
  return 1;
}

static int Surface__index(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  const char* key = luaL_checkstring(L, 2);
  lg_getmethod(L, 1, key);
  return 1;
}

static int Surface__tostring(lua_State* L) {
  lSurface* self = lg_checksurface(L, 1);
  int sw = self->surf->w;
  int sh = self->surf->h;
  int sbpp = SDL_BITSPERPIXEL(self->surf->format);
  lua_pushfstring(L, "<Surface(%dx%dx%d)>", sw, sh, sbpp);
  return 1;
}

static int Surface__gc(lua_State* L) {
  lSurface* self = lg_testsurface(L, 1);
  if (self->surf && !self->isws) {
    SDL_DestroySurface(self->surf);
    self->surf = NULL;
  }
  return 0;
}