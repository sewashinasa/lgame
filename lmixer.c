
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lmixer.h"
#include "lmusic.h"
#include "lmethod.h"
#include "lcheckfile.h"

#define DEFAULT_FREQUENCY 44100
#define DEFAULT_SIZE -16
#define DEFAULT_CHANNELS 2
#define DEFAULT_CHUNKSIZE 512
#define DEFAULT_DEVNAME NULL

#define DEFAULT_CHANNEL_COUNT 8

#define CHECK_MIXER_INIT(L) \
  if (!lgame_mixer_initialized) \
    return luaL_error(L, "mixer not initialized");

static int mixer_init(lua_State* L);
static int mixer_pre_init(lua_State* L);
static int mixer_quit(lua_State* L);
static int mixer_get_init(lua_State* L);
static int mixer_get_driver(lua_State* L);
static int mixer_stop(lua_State* L);
static int mixer_pause(lua_State* L);
static int mixer_unpause(lua_State* L);
static int mixer_fadeout(lua_State* L);
static int mixer_set_num_channels(lua_State* L);
static int mixer_get_num_channels(lua_State* L);
static int mixer_find_channel(lua_State* L);
static int mixer_get_busy(lua_State* L);
static int image_get_sdl_mixer_version(lua_State * L);

static int Sound_new(lua_State* L);
static int Sound_play(lua_State* L);
static int Sound_stop(lua_State* L);
static int Sound_fadeout(lua_State* L);
static int Sound_set_volume(lua_State* L);
static int Sound_get_volume(lua_State* L);
static int Sound_get_num_channels(lua_State* L);
static int Sound_get_length(lua_State* L);
static int Sound__index(lua_State* L);
static int Sound__gc(lua_State* L);

static int Channel_new(lua_State* L);
static int Channel_play(lua_State* L);
static int Channel_stop(lua_State* L);
static int Channel_pause(lua_State* L);
static int Channel_unpause(lua_State* L);
static int Channel_fadeout(lua_State* l);
static int Channel_set_volume(lua_State* L);
static int Channel_get_volume(lua_State* L);
static int Channel_get_busy(lua_State* L);
static int Channel_get_sound(lua_State* L);
static int Channel__index(lua_State* L);

const struct luaL_Reg lgame_mixer_module[] = {
  {"init", mixer_init},
  {"pre_init", mixer_pre_init},
  {"quit", mixer_quit},
  {"get_init", mixer_get_init},
  {"get_driver", mixer_get_driver},
  {"stop", mixer_stop},
  {"pause", mixer_pause},
  {"unpause", mixer_unpause},
  {"fadeout", mixer_fadeout},
  {"set_num_channels", mixer_set_num_channels},
  {"get_num_channels", mixer_get_num_channels},
  {"find_channel", mixer_find_channel},
  {"get_busy", mixer_get_busy},
  {"get_sdl_mixer_version", image_get_sdl_mixer_version},
  {"Sound", Sound_new},
  {"Channel", Channel_new},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Sound_methods[] = {
  {"play", Sound_play},
  {"stop", Sound_stop},
  {"fadeout", Sound_fadeout},
  {"set_volume", Sound_set_volume},
  {"get_volume", Sound_get_volume},
  {"get_num_channels", Sound_get_num_channels},
  {"get_length", Sound_get_length},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Sound_metamethods[] = {
  {"__index", Sound__index},
  {"__gc", Sound__gc},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Channel_methods[] = {
  {"play", Channel_play},
  {"stop", Channel_stop},
  {"pause", Channel_pause},
  {"unpause", Channel_unpause},
  {"fadeout", Channel_fadeout},
  {"set_volume", Channel_set_volume},
  {"get_volume", Channel_get_volume},
  {"get_busy", Channel_get_busy},
  {"get_sound", Channel_get_sound},
  {NULL, NULL}
};

const struct luaL_Reg lgame_Channel_metamethods[] = {
  {"__index", Channel__index},
  {NULL, NULL}
};

static MIX_Mixer* lgame_Mixer = NULL;
static int lgame_mixer_initialized = 0;

typedef struct lgame_InternalChannel {
  MIX_Track* track;
  Uint64 start_time;
  int reserved;
} lgame_InternalChannel;

static struct lgame_AllocatedChannels {
  int nchannels;
  lgame_InternalChannel* channels;
} lgame_AllocatedChannels = {
  .nchannels = 0,
  .channels = NULL,
};

static struct lgame_mixer_init_args {
  int frequency;
  int size;
  int channels;
  int chunksize;
  const char* devname;
} lgame_mixer_default_init_args = { 
  .frequency = DEFAULT_FREQUENCY,
  .size = DEFAULT_SIZE,
  .channels = DEFAULT_CHANNELS,
  .chunksize = DEFAULT_CHUNKSIZE,
  .devname = DEFAULT_DEVNAME,
};

lgame_Sound* lgame_pushsound(lua_State* L, MIX_Audio *audio) {
  lgame_Sound* s;
  s = (lgame_Sound*)lua_newuserdata(L, sizeof(lgame_Sound));
  s->audio = audio;
  s->volume = 1.0f;
  luaL_getmetatable(L, LGAME_SOUND_METATABLE);
  lua_setmetatable(L, -2);
  
  if (audio) {
    lua_pushvalue(L, -1);
    lua_rawsetp(L, LUA_REGISTRYINDEX, (const void*)audio);
  }

  return s;
}

lgame_Sound* lgame_checksound(lua_State* L, int idx) {
  return (lgame_Sound*)luaL_checkudata(L, idx, LGAME_SOUND_METATABLE);
}

lgame_Sound* lgame_testsound(lua_State* L, int idx) {
  return (lgame_Sound*)luaL_testudata(L, idx, LGAME_SOUND_METATABLE);
}

lgame_Channel* lgame_pushchannel(lua_State* L) {
  lgame_Channel* c;
  c = (lgame_Channel*)lua_newuserdata(L, sizeof(lgame_Channel));
  c->channel = LGAME_CHANNEL_NONE;
  luaL_getmetatable(L, LGAME_CHANNEL_METATABLE);
  lua_setmetatable(L, -2);
  return c;
}

lgame_Channel* lgame_checkchannel(lua_State* L, int idx) {
  lgame_Channel* channel = (lgame_Channel*)luaL_checkudata(L, idx, LGAME_CHANNEL_METATABLE);
  int nchannels = lgame_AllocatedChannels.nchannels;
  if (channel->channel < 0 || channel->channel >= nchannels)
    luaL_error(L, "out of channel range");
  return channel;
}

lgame_Channel* lgame_testchannel(lua_State* L, int idx) {
  return (lgame_Channel*)luaL_testudata(L, idx, LGAME_CHANNEL_METATABLE);
}

MIX_Mixer* lgame_getmixer(void) {
  return lgame_Mixer;
}

int lgame_getmixerinit(void) {
  return lgame_mixer_initialized;
}

#define CHANNELS_SUC_ERROR  0
#define CHANNELS_OOM_ERROR -1
#define CHANNELS_SDL_ERROR -2

static int alloc_channels(int count) {
  if (count == lgame_AllocatedChannels.nchannels)
    return CHANNELS_SUC_ERROR;

  if (count == 0 && lgame_AllocatedChannels.channels) {
    for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
      if (lgame_AllocatedChannels.channels[i].track) {
        MIX_DestroyTrack(lgame_AllocatedChannels.channels[i].track);
        lgame_AllocatedChannels.channels[i].track = NULL;
        lgame_AllocatedChannels.channels[i].start_time = 0;
        lgame_AllocatedChannels.channels[i].reserved = 0;
      }
    }
    free(lgame_AllocatedChannels.channels);
    lgame_AllocatedChannels.channels = NULL;
    lgame_AllocatedChannels.nchannels = 0;
    return CHANNELS_SUC_ERROR;
  }
  
  lgame_AllocatedChannels.nchannels = count;
  lgame_AllocatedChannels.channels = (lgame_InternalChannel*)calloc(count, sizeof(lgame_InternalChannel));
  if (!lgame_AllocatedChannels.channels)
    return CHANNELS_OOM_ERROR;

  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    lgame_AllocatedChannels.channels[i].start_time = 0;
    lgame_AllocatedChannels.channels[i].reserved = 0;
    lgame_AllocatedChannels.channels[i].track = MIX_CreateTrack(lgame_Mixer);
    if (lgame_AllocatedChannels.channels[i].track == NULL) {
      for (int j = 0; j < i; j++) {
        if (lgame_AllocatedChannels.channels[j].track) {
          MIX_DestroyTrack(lgame_AllocatedChannels.channels[j].track);
          lgame_AllocatedChannels.channels[j].track = NULL;
        }
      }
      return CHANNELS_SDL_ERROR;
    }
  }

  return CHANNELS_SUC_ERROR;
}

static void free_channels(void) {
  alloc_channels(0);
}

static int realloc_channels(int count) {
  int last_nchannels = lgame_AllocatedChannels.nchannels;
  if (count == last_nchannels)
    return CHANNELS_SUC_ERROR;

  if (last_nchannels > count) {
    for (int i = count; i < last_nchannels; i++) {
      if (lgame_AllocatedChannels.channels[i].track) {
        MIX_DestroyTrack(lgame_AllocatedChannels.channels[i].track);
        lgame_AllocatedChannels.channels[i].track = NULL;
      }
    }
  }

  lgame_InternalChannel* newchannels;
  newchannels = (lgame_InternalChannel*)realloc(lgame_AllocatedChannels.channels, count * sizeof(lgame_InternalChannel));
  if (!newchannels)
    return CHANNELS_OOM_ERROR;
  
  lgame_AllocatedChannels.channels = newchannels;
  lgame_AllocatedChannels.nchannels = count;

  for (int i = last_nchannels; i < count; i++) {
    lgame_AllocatedChannels.channels[i].start_time = 0;
    lgame_AllocatedChannels.channels[i].reserved = 0;
    lgame_AllocatedChannels.channels[i].track = MIX_CreateTrack(lgame_Mixer);
    if (lgame_AllocatedChannels.channels[i].track == NULL) {
      lgame_AllocatedChannels.nchannels = 0;
      for (int j = last_nchannels; j < i; j++) {
        if (lgame_AllocatedChannels.channels[j].track) {
          MIX_DestroyTrack(lgame_AllocatedChannels.channels[j].track);
          lgame_AllocatedChannels.channels[j].track = NULL;
        }
      }
      return CHANNELS_SDL_ERROR;
    }
  }

  return CHANNELS_SUC_ERROR;
}

static int setaudiovolume_channels(MIX_Audio *audio, float volume) {
  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    if (MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track) == audio) {
      if (!MIX_SetTrackGain(lgame_AllocatedChannels.channels[i].track, volume)) {
        return CHANNELS_SDL_ERROR;
      }
    }
  }
  return CHANNELS_SUC_ERROR;
}

static int find_channel(int force) {
  if (lgame_AllocatedChannels.nchannels == 0)
    if (force)
      return LGAME_CHANNEL_ERROR;
    else
      return LGAME_CHANNEL_NONE;

  if (lgame_AllocatedChannels.nchannels == 1)
    return 0;

  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    if (!MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track)) {
      return i;
    }
  }

  if (force) {
    Uint64 start_time = lgame_AllocatedChannels.channels[0].start_time;
    Uint64 current_time = SDL_GetTicks();
    Uint64 delay = current_time - start_time;
    int channel = 0;
    for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
      start_time = lgame_AllocatedChannels.channels[i].start_time;
      if ((current_time - start_time) > delay) {
        delay = current_time - start_time;
        channel = i;
      }
    }
    return channel;
  }

  return LGAME_CHANNEL_NONE;
}

static SDL_AudioFormat size2fmt(int size) {
  switch (size) {
  case 8:   return SDL_AUDIO_U8;
  case -8:  return SDL_AUDIO_S8;
  case 16:  return SDL_AUDIO_S16;
  case -16: return SDL_AUDIO_S16;
  case 32:  return SDL_AUDIO_S32;
  case -32: return SDL_AUDIO_S32;
  default:  return 0;
  }
}

static SDL_AudioDeviceID getadevicebyname(const char* devname) {
  if (!devname) 
    return SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
  SDL_AudioDeviceID* adevs = SDL_GetAudioPlaybackDevices(NULL);
  int i = 0;
  while (adevs[i]) {
    const char* name = SDL_GetAudioDeviceName(adevs[i]);
    if (name && devname && strcmp(name, devname) == 0) {
      SDL_free(adevs);
      return adevs[i];
    }
    i++;
  }
  SDL_free(adevs);
  return 0;
}

static int mixer_init(lua_State* L) {
  int freq = (int)luaL_optinteger(L, 1, lgame_mixer_default_init_args.frequency);
  int size = (int)luaL_optinteger(L, 2, lgame_mixer_default_init_args.size);
  int channels = (int)luaL_optinteger(L, 3, lgame_mixer_default_init_args.channels);
  int chunksize = (int)luaL_optinteger(L, 4, lgame_mixer_default_init_args.chunksize); (void)chunksize;
  const char* devname = luaL_optstring(L, 5, lgame_mixer_default_init_args.devname);

  if (lgame_mixer_initialized)
    return 0;

  if (!MIX_Init())
    return luaL_error(L, SDL_GetError());
  
  SDL_AudioSpec aspec;
  SDL_zero(aspec);
  aspec.channels = channels;
  aspec.format = size2fmt(size);
  aspec.freq = freq;

  SDL_AudioDeviceID devid;
  devid = getadevicebyname(devname);
  if (!devid) return luaL_error(L, "unknown '%s' device", devname);

  lgame_Mixer = MIX_CreateMixerDevice(devid, &aspec);
  if (!lgame_Mixer)
    return luaL_error(L, SDL_GetError());

  int err = alloc_channels(DEFAULT_CHANNEL_COUNT);
  if (err < 0) {
    if (err == CHANNELS_OOM_ERROR)
      return luaL_error(L, "out of memory");
    else if (err == CHANNELS_SDL_ERROR)
      return luaL_error(L, SDL_GetError());
  }

  lgame_mixer_initialized = 1;

  return 0;
}

static int mixer_pre_init(lua_State* L) {
  int freq = (int)luaL_optinteger(L, 1, DEFAULT_FREQUENCY);
  int size = (int)luaL_optinteger(L, 2, DEFAULT_SIZE);
  int channels = (int)luaL_optinteger(L, 3, DEFAULT_CHANNELS);
  int chunksize = (int)luaL_optinteger(L, 4, DEFAULT_CHUNKSIZE); (void)chunksize;
  const char* devname = luaL_optstring(L, 5, DEFAULT_DEVNAME);

  lgame_mixer_default_init_args.frequency = freq;
  lgame_mixer_default_init_args.size = size;
  lgame_mixer_default_init_args.channels = channels;
  lgame_mixer_default_init_args.chunksize = chunksize;
  lgame_mixer_default_init_args.devname = devname;

  return 0;
}

static int mixer_quit(lua_State* L) {
  if (!lgame_mixer_initialized)
    return 0;
  lgame_freemusic();
  free_channels();
  if (lgame_Mixer) {
    MIX_DestroyMixer(lgame_Mixer);
    lgame_Mixer = NULL;
  }
  MIX_Quit();
  lgame_mixer_initialized = 0;
  return 0;
}

static int mixer_get_init(lua_State* L) {
  if (!lgame_mixer_initialized) {
    lua_pushnil(L);
    return 1;
  }

  SDL_AudioSpec aspec;
  if (!MIX_GetMixerFormat(lgame_Mixer, &aspec))
    return luaL_error(L, SDL_GetError());

  int freq = aspec.freq;
  int size = SDL_AUDIO_BITSIZE(aspec.format);
  int channels = aspec.channels;

  lua_createtable(L, 3, 0);
  lua_pushinteger(L, freq);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, size);
  lua_rawseti(L, -2, 2);
  lua_pushinteger(L, channels);
  lua_rawseti(L, -2, 3);
  return 1;
}

static int mixer_get_driver(lua_State* L) {
  lua_pushstring(L, SDL_GetCurrentAudioDriver());
  return 1;
}

static int mixer_stop(lua_State* L) {
  CHECK_MIXER_INIT(L);
  if (!MIX_StopAllTracks(lgame_Mixer, 0))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int mixer_pause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  if (!MIX_PauseAllTracks(lgame_Mixer))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int mixer_unpause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  if (!MIX_ResumeAllTracks(lgame_Mixer))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int mixer_fadeout(lua_State* L) {
  CHECK_MIXER_INIT(L);
  Sint64 ms = (Sint64)luaL_checkinteger(L, 1);
  if (!MIX_StopAllTracks(lgame_Mixer, ms))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int mixer_set_num_channels(lua_State* L) {
  CHECK_MIXER_INIT(L);
  int count = (int)luaL_checkinteger(L, 1);
  if (count < 0)
    return 0;
  else if (count == lgame_AllocatedChannels.nchannels)
    return 0;
  else if (count == 0) {
    free_channels();
    return 0;
  }

  if (lgame_AllocatedChannels.nchannels == 0) {
    int err = alloc_channels(count);
    if (err < 0) {
      if (err == CHANNELS_OOM_ERROR)
        return luaL_error(L, "out of memory");
      else if (err == CHANNELS_SDL_ERROR)
        return luaL_error(L, SDL_GetError());
    }
  }

  int err = realloc_channels(count);
  if (err < 0) {
    if (err == CHANNELS_OOM_ERROR)
      return luaL_error(L, "out of memory");
    else if (err == CHANNELS_SDL_ERROR)
      return luaL_error(L, SDL_GetError());
  }
  
  return 0;
}

static int mixer_get_num_channels(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lua_pushinteger(L, lgame_AllocatedChannels.nchannels);
  return 1;
}

static int mixer_find_channel(lua_State* L) {
  CHECK_MIXER_INIT(L);
  int force = lua_toboolean(L, 1);
  lgame_Channel* channel = lgame_pushchannel(L);
  channel->channel = find_channel(force);
  return 1;
}

static int mixer_get_busy(lua_State* L) {
  int busy = 0;
  if (!lgame_mixer_initialized) {
    lua_pushboolean(L, busy);
    return 1;
  }
  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    MIX_Track* track = lgame_AllocatedChannels.channels[i].track;
    if (MIX_TrackPlaying(track)) {
      busy = 1;
      break;
    }
    else if (!MIX_TrackPlaying(track) && MIX_TrackPaused(track)) {
      busy = 1;
      break;
    }
  }
  lua_pushboolean(L, busy);
  return 1;
}

static int image_get_sdl_mixer_version(lua_State* L) {
  int linked = 1;
  if (!lua_isnoneornil(L, 1))
    linked = lua_toboolean(L, 1);
  int version;
  if (linked)
    version = MIX_Version();
  else
    version = SDL_MIXER_VERSION;
  int major, minor, patch;
  major = version / 1000000;
  minor = (version / 1000) % 1000;
  patch = version % 1000;
  lua_createtable(L, 3, 0);
  lua_pushinteger(L, major);
  lua_rawseti(L, -2, 1);
  lua_pushinteger(L, minor);
  lua_rawseti(L, -2, 2);
  lua_pushinteger(L, patch);
  lua_rawseti(L, -2, 3);
  return 1;
}

static int Sound_new(lua_State* L) {
  CHECK_MIXER_INIT(L);
  SDL_IOStream* io;
  MIX_Audio* audio;
  io = lgame_checkreadfile(L, 1);
  audio = MIX_LoadAudio_IO(lgame_Mixer, io, true, false);
  if (audio == NULL) {
    lgame_closereadfile(L, io, 0);
    return luaL_error(L, SDL_GetError());
  }
  lgame_pushsound(L, audio);
  lgame_closereadfile(L, io, 1);
  return 1;
}

static int Sound_play(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  Sint64 loops = (Sint64)luaL_optinteger(L, 2, 0);
  int maxtime = (int)luaL_optinteger(L, 3, 0);
  Sint64 ms = (Sint64)luaL_optinteger(L, 4, 0);
  SDL_PropertiesID props = 0;
  if (loops|| maxtime|| ms) {
    props = SDL_CreateProperties();
    if (!props)
      return luaL_error(L, SDL_GetError());
  }
  if (loops && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (maxtime && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_MAX_MILLISECONDS_NUMBER, maxtime)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (ms && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, ms)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  int channelidx = find_channel(1);
  if (channelidx == LGAME_CHANNEL_ERROR) {
    SDL_DestroyProperties(props);
    return luaL_error(L, "count of channels is zero");
  }
  lgame_InternalChannel* channels = lgame_AllocatedChannels.channels;
  MIX_Track *track = channels[channelidx].track;
  channels[channelidx].start_time = SDL_GetTicks();
  if (!MIX_SetTrackAudio(track, self->audio)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (setaudiovolume_channels(self->audio, self->volume) < 0) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (!MIX_PlayTrack(track, props)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  lgame_Channel* channel = lgame_pushchannel(L);
  channel->channel = channelidx;
  return 1;
}

static int Sound_stop(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    if (MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track) == self->audio) {
      if (!MIX_StopTrack(lgame_AllocatedChannels.channels[i].track, 0)) {
        return luaL_error(L, SDL_GetError());
      }
    }
  }
  return 0;
}

static int Sound_fadeout(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  Sint64 ms = (Sint64)luaL_checkinteger(L, 2);
  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    if (MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track) == self->audio) {
      Sint64 fadeout_frames = MIX_AudioMSToFrames(self->audio, ms);
      if (!MIX_StopTrack(lgame_AllocatedChannels.channels[i].track, fadeout_frames)) {
        return luaL_error(L, SDL_GetError());
      }
    }
  }
  return 0;
}

static int Sound_set_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  float volume = (float)luaL_checknumber(L, 2);
  if (volume > 1.0)
    volume = 1.0;
  else if (volume < 0.0)
    return 0;
  if (setaudiovolume_channels(self->audio, volume) < 0)
    return luaL_error(L, SDL_GetError());
  self->volume = volume;
  return 0;
}

static int Sound_get_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lua_pushnumber(L, lgame_checksound(L, 1)->volume);
  return 1;
}

static int Sound_get_num_channels(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  int count = 0;
  for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
    if (MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track) == self->audio) {
      count++;
    }
  }
  lua_pushinteger(L, count);
  return 1;
}

static int Sound_get_length(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Sound* self = lgame_checksound(L, 1);
  Sint64 length = MIX_GetAudioDuration(self->audio);
  if (length == MIX_DURATION_UNKNOWN)
    return luaL_error(L, "unknown length");
  else if (length == MIX_DURATION_INFINITE)
    return luaL_error(L, "infinite length");
  else {
    length = MIX_AudioFramesToMS(self->audio, length);
    if (length < 0)
      return luaL_error(L, SDL_GetError());
  }
  lua_pushnumber(L, length / 1000.0);
  return 1;
}

static int Sound__index(lua_State* L) {
  lgame_checksound(L, 1);
  const char* key = luaL_checkstring(L, 2);
  lgame_getmethod(L, 1, key);
  return 1;
}

static int Sound__gc(lua_State* L) {
  lgame_Sound* self = lgame_checksound(L, 1);
  if (self && self->audio) {
    for (int i = 0; i < lgame_AllocatedChannels.nchannels; i++) {
      if (MIX_GetTrackAudio(lgame_AllocatedChannels.channels[i].track) == self->audio) {
        MIX_SetTrackAudio(lgame_AllocatedChannels.channels[i].track, NULL);
        lgame_AllocatedChannels.channels[i].start_time = 0;
      }
    }

    lua_pushnil(L);
    lua_rawsetp(L, LUA_REGISTRYINDEX, (const void*)self->audio);

    MIX_DestroyAudio(self->audio);
    self->audio = NULL;
  }
  return 0;
}


static int Channel_new(lua_State* L) {
  CHECK_MIXER_INIT(L);
  int channel;
  int nchannels;
  lgame_Channel* self;
  channel = (int)luaL_checkinteger(L, 1);
  nchannels = lgame_AllocatedChannels.nchannels;
  if (channel < 0 || channel >= nchannels)
    luaL_error(L, "out of channel range");
  self = lgame_pushchannel(L);
  self->channel = channel;
  return 1;
}

static int Channel_play(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  lgame_Sound* sound = lgame_checksound(L, 2);
  Sint64 loops = (Sint64)luaL_optinteger(L, 3, 0);
  Sint64 maxtime = (Sint64)luaL_optinteger(L, 4, 0);
  Sint64 ms = (Sint64)luaL_optinteger(L, 5, 0);
  SDL_PropertiesID props = 0;
  if (loops || maxtime || ms) {
    props = SDL_CreateProperties();
    if (!props)
      return luaL_error(L, SDL_GetError());
  }
  if (loops && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (maxtime && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_MAX_MILLISECONDS_NUMBER, maxtime)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (ms && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, ms)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  lgame_InternalChannel* channels = lgame_AllocatedChannels.channels;
  MIX_Track* track = channels[self->channel].track;
  channels[self->channel].start_time = SDL_GetTicks();
  if (!MIX_SetTrackAudio(track, sound->audio)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (!MIX_PlayTrack(track, props)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int Channel_stop(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  if (!MIX_StopTrack(track, 0))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Channel_pause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  if (!MIX_PauseTrack(track))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Channel_unpause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  if (!MIX_ResumeTrack(track))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Channel_fadeout(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  Sint64 ms = (Sint64)luaL_checkinteger(L, 2);
  MIX_Audio* audio = MIX_GetTrackAudio(lgame_AllocatedChannels.channels[self->channel].track);
  if (audio == NULL)
    return 0;
  Sint64 fadeout_frames = MIX_AudioMSToFrames(audio, ms);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  if (!MIX_StopTrack(track, fadeout_frames))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Channel_set_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  float volume = (float)luaL_checknumber(L, 2);
  if (volume < 0.0f)
    return 0;
  else if (volume > 1.0f)
    volume = 1.0f;
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  if (!MIX_SetTrackGain(track, volume))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int Channel_get_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  lua_pushnumber(L, MIX_GetTrackGain(track));
  return 1;
}

static int Channel_get_busy(lua_State* L) {
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  int busy = 0;
  if (lgame_mixer_initialized) {
    if (MIX_TrackPlaying(track))
      busy = 1;
    else if (!MIX_TrackPlaying(track) && MIX_TrackPaused(track))
      busy = 1;
  }
  lua_pushboolean(L, busy);
  return 1;
}

static int Channel_get_sound(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_Channel* self = lgame_checkchannel(L, 1);
  MIX_Track* track = lgame_AllocatedChannels.channels[self->channel].track;
  MIX_Audio* audio = MIX_GetTrackAudio(track);
  if (audio)
    lua_rawgetp(L, LUA_REGISTRYINDEX, (const void*)audio);
  else
    lua_pushnil(L);
  return 1;
}

static int Channel__index(lua_State* L) {
  lgame_Channel* self = lgame_checkchannel(L, 1);
  const char* key = luaL_checkstring(L, 2);
  if (strcmp(key, "id") == 0)
    lua_pushinteger(L, self->channel);
  else
    lgame_getmethod(L, 1, key);
  return 1;
}