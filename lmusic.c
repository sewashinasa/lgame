
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lmusic.h"
#include "lmixer.h"
#include "lcheckfile.h"

#define CHECK_MIXER_INIT(L) \
  if (!lgame_getmixerinit()) \
    return luaL_error(L, "mixer not initialized");

#define CHECK_MUSIC_LOAD(L) \
  if (lgame_MusicTrack == NULL || lgame_MusicAudio == NULL) \
    return luaL_error(L, "music not loaded");

static int music_load(lua_State * L);
static int music_unload(lua_State * L);
static int music_play(lua_State * L);
static int music_rewind(lua_State * L);
static int music_stop(lua_State * L);
static int music_pause(lua_State * L);
static int music_unpause(lua_State * L);
static int music_fadeout(lua_State * L);
static int music_set_volume(lua_State * L);
static int music_get_volume(lua_State * L);
static int music_get_busy(lua_State * L);
static int music_set_pos(lua_State * L);
static int music_get_pos(lua_State * L);
static int music_get_metadata(lua_State * L);

const struct luaL_Reg lgame_music_module[] = {
  {"load", music_load},
  {"unload", music_unload},
  {"play", music_play},
  {"rewind", music_rewind},
  {"stop", music_stop},
  {"pause", music_pause},
  {"unpause", music_unpause},
  {"fadeout", music_fadeout},
  {"set_volume", music_set_volume},
  {"get_volume", music_get_volume},
  {"get_busy", music_get_busy},
  {"set_pos", music_set_pos},
  {"get_pos", music_get_pos},
  {"get_metadata", music_get_metadata},
  {NULL, NULL}
};

static MIX_Track* lgame_MusicTrack = NULL;
static MIX_Audio* lgame_MusicAudio = NULL;

void lgame_freemusic(void) {
  if (lgame_MusicAudio && lgame_MusicTrack) { 
    MIX_StopTrack(lgame_MusicTrack, 0);
  }
  if (lgame_MusicAudio) {
    MIX_DestroyAudio(lgame_MusicAudio);
    lgame_MusicAudio = NULL;
  }
  if (lgame_MusicTrack) {
    MIX_DestroyTrack(lgame_MusicTrack);
    lgame_MusicTrack = NULL;
  }
}

static int music_load(lua_State* L) {
  CHECK_MIXER_INIT(L);
  SDL_IOStream* io = lgame_checkreadfile(L, 1);
  MIX_Mixer* mixer = lgame_getmixer();
  if (lgame_MusicAudio != NULL) {
    if (!MIX_StopTrack(lgame_MusicTrack, 0))
      return luaL_error(L, SDL_GetError());
    if (!MIX_SetTrackAudio(lgame_MusicTrack, NULL))
      return luaL_error(L, SDL_GetError());
    MIX_DestroyAudio(lgame_MusicAudio);
  }
  lgame_MusicAudio = MIX_LoadAudio_IO(mixer, io, false, false);
  if (lgame_MusicAudio == NULL) {
    lgame_closereadfile(L, io, 0);
    return luaL_error(L, SDL_GetError());
  }
  if (lgame_MusicTrack == NULL) {
    lgame_MusicTrack = MIX_CreateTrack(mixer);
    if (lgame_MusicTrack == NULL) {
      lgame_closereadfile(L, io, 0);
      return luaL_error(L, SDL_GetError());
    }
  }
  if (!MIX_SetTrackAudio(lgame_MusicTrack, lgame_MusicAudio)) {
    return luaL_error(L, SDL_GetError());
  }
  lgame_closereadfile(L, io, 1);
  return 0;
}

static int music_unload(lua_State* L) {
  CHECK_MIXER_INIT(L);
  lgame_freemusic();
  return 0;
}

static int music_play(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  Sint64 loops = (Sint64)luaL_optinteger(L, 1, 0);
  float start = (float)luaL_optnumber(L, 2, .0f);
  Sint64 ms = (Sint64)luaL_optinteger(L, 3, 0);
  SDL_PropertiesID props = 0;
  if (loops || start || ms) {
    props = SDL_CreateProperties();
    if (props == 0)
      return luaL_error(L, SDL_GetError());
  }
  if (loops && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, loops)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (start) {
    Sint64 start_ms = (Sint64)(start * 1000);
    if (!SDL_SetNumberProperty(props, MIX_PROP_PLAY_START_MILLISECOND_NUMBER, start_ms)) {
      SDL_DestroyProperties(props);
      return luaL_error(L, SDL_GetError());
    }
  }
  if (ms && !SDL_SetNumberProperty(props, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, ms)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  if (!MIX_PlayTrack(lgame_MusicTrack, props)) {
    SDL_DestroyProperties(props);
    return luaL_error(L, SDL_GetError());
  }
  SDL_DestroyProperties(props);
  return 0;
}

static int music_rewind(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  if (!MIX_PlayTrack(lgame_MusicTrack, 0)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_stop(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  if (!MIX_StopTrack(lgame_MusicTrack, 0)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_pause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  if (!MIX_PauseTrack(lgame_MusicTrack)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_unpause(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  if (!MIX_ResumeTrack(lgame_MusicTrack)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_fadeout(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  Sint64 ms = (Sint64)luaL_checkinteger(L, 1);
  Sint64 ms_frames = MIX_AudioMSToFrames(lgame_MusicAudio, ms);
  if (ms_frames < 0) {
    return luaL_error(L, SDL_GetError());
  }
  if (!MIX_StopTrack(lgame_MusicTrack, ms_frames)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_set_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  float volume = (float)luaL_checknumber(L, 1);
  if (volume > 1.0f)
    volume = 1.0f;
  else if (volume < 0.0f)
    return 0;
  if (!MIX_SetTrackGain(lgame_MusicTrack, volume)) {
    return luaL_error(L, SDL_GetError());
  }
  return 0;
}

static int music_get_volume(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  lua_pushnumber(L, MIX_GetTrackGain(lgame_MusicTrack));
  return 1;
}

static int music_get_busy(lua_State* L) {
  CHECK_MIXER_INIT(L);
  int busy = 0;
  if (lgame_MusicTrack) {
    if (MIX_TrackPlaying(lgame_MusicTrack))
      busy = 1;
    else if (!MIX_TrackPlaying(lgame_MusicTrack) && MIX_TrackPaused(lgame_MusicTrack))
      busy = 1;
  }
  lua_pushboolean(L, busy);
  return 1;
}

static int music_set_pos(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  float pos = (float)luaL_checknumber(L, 1);
  Sint64 pos_ms = (Sint64)(pos * 1000);
  Sint64 pos_frames = MIX_AudioMSToFrames(lgame_MusicAudio, pos_ms);
  if (pos_frames < 0)
    return luaL_error(L, SDL_GetError());
  if (!MIX_SetTrackPlaybackPosition(lgame_MusicTrack, pos_frames))
    return luaL_error(L, SDL_GetError());
  return 0;
}

static int music_get_pos(lua_State* L) {
  CHECK_MIXER_INIT(L);
  CHECK_MUSIC_LOAD(L);
  Sint64 pos_frames;
  Sint64 pos_ms;
  pos_frames = MIX_GetTrackPlaybackPosition(lgame_MusicTrack);
  if (pos_frames < 0)
    return luaL_error(L, SDL_GetError());
  pos_ms = MIX_AudioFramesToMS(lgame_MusicAudio, pos_frames);
  if (pos_ms < 0)
    return luaL_error(L, SDL_GetError());
  lua_pushinteger(L, pos_ms);
  return 1;
}

static int music_get_metadata(lua_State* L) {
  CHECK_MIXER_INIT(L);
  MIX_Audio* audio;
  int free_audio = 0;
  if (lua_isnoneornil(L, 1)) {
    CHECK_MUSIC_LOAD(L);
    audio = lgame_MusicAudio;
  }
  else {
    SDL_IOStream* io = lgame_checkreadfile(L, 1);
    MIX_Mixer* mixer = lgame_getmixer();
    audio = MIX_LoadAudio_IO(mixer, io, false, false);
    if (audio == NULL)
      return luaL_error(L, SDL_GetError());
    free_audio = 1;
    lgame_closereadfile(L, io, 1);
  }

  SDL_PropertiesID props = MIX_GetAudioProperties(audio);
  if (props == 0) {
    if (free_audio)
      MIX_DestroyAudio(audio);
    return luaL_error(L, SDL_GetError());
  }

  const char* title;
  const char* album;
  const char* artist;
  const char* copyright;

  title = SDL_GetStringProperty(props, MIX_PROP_METADATA_TITLE_STRING, "");
  album = SDL_GetStringProperty(props, MIX_PROP_METADATA_ALBUM_STRING, "");
  artist = SDL_GetStringProperty(props, MIX_PROP_METADATA_ARTIST_STRING, "");
  copyright = SDL_GetStringProperty(props, MIX_PROP_METADATA_COPYRIGHT_STRING, "");

  lua_createtable(L, 0, 4);
  lua_pushstring(L, title);
  lua_setfield(L, -2, "title");
  lua_pushstring(L, album);
  lua_setfield(L, -2, "album");
  lua_pushstring(L, artist);
  lua_setfield(L, -2, "artist");
  lua_pushstring(L, copyright);
  lua_setfield(L, -2, "copyright");

  if (free_audio)
    MIX_DestroyAudio(audio);
  SDL_DestroyProperties(props);

  return 1;
}