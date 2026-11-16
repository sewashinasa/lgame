
#ifndef LSCALE2X_H_
#define LSCALE2X_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3/SDL_surface.h>

SDL_Surface* lgame_scale2x_4bpp(SDL_Surface* src, SDL_Surface* dst);
SDL_Surface* lgame_scale2x_3bpp(SDL_Surface* src, SDL_Surface* dst);
SDL_Surface* lgame_scale2x_2bpp(SDL_Surface* src, SDL_Surface* dst);
SDL_Surface* lgame_scale2x_1bpp(SDL_Surface* src, SDL_Surface* dst);

typedef SDL_Surface*(*lgame_scale2x_function)(SDL_Surface*, SDL_Surface*);

#endif