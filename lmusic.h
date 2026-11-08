
#ifndef LMUSIC_H_
#define LMUSIC_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3_mixer/SDL_mixer.h>

void lgame_freemusic(void);

const struct luaL_Reg lgame_music_module[];

#endif