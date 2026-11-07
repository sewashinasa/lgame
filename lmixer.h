
#ifndef LMIXER_H_
#define LMIXER_H_

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include <SDL3_mixer/SDL_mixer.h>

#define LGAME_CHANNEL_NONE (-1)
#define LGAME_CHANNEL_ERROR (-2)

#define LGAME_SOUND_METATABLE "lgame.Sound"
#define LGAME_CHANNEL_METATABLE "lgame.Channel"

typedef struct lgame_Sound {
  MIX_Audio* audio;
  float volume;
} lgame_Sound;

typedef struct lgame_Channel {
  int channel;
} lgame_Channel;

extern const struct luaL_Reg lgame_mixer_module[];

extern const struct luaL_Reg lgame_Sound_methods[];
extern const struct luaL_Reg lgame_Sound_metamethods[];

extern const struct luaL_Reg lgame_Channel_methods[];
extern const struct luaL_Reg lgame_Channel_metamethods[];

lgame_Sound* lgame_pushsound(lua_State* L, MIX_Audio* audio);
lgame_Sound* lgame_checksound(lua_State* L, int idx);
lgame_Sound* lgame_testsound(lua_State* L, int idx);

lgame_Channel* lgame_pushchannel(lua_State* L);
lgame_Channel* lgame_checkchannel(lua_State* L, int idx);
lgame_Channel* lgame_testchannel(lua_State* L, int idx);

#endif