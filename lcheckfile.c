

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lcheckfile.h"


static size_t stdio_fwrite(
  void* userdata, 
  const void* ptr, 
  size_t size, 
  SDL_IOStatus* status) {
  size_t res;
  FILE* fp = (FILE*)userdata;
  *status = SDL_IO_STATUS_READY;
  res = fwrite(ptr, size, 1, fp);
  if (res == 0) 
    *status = SDL_IO_STATUS_ERROR;
  return res;
}

static size_t stdio_fread(
  void* userdata,
  void* ptr,
  size_t size,
  SDL_IOStatus* status) {
  size_t res;
  FILE* fp = (FILE*)userdata;
  *status = SDL_IO_STATUS_READY;
  res = fread(ptr, size, 1, fp);
  if (res == 0)
    *status = SDL_IO_STATUS_ERROR;
  return res;
}

static bool stdio_flush(
  void* userdata,
  SDL_IOStatus* status) {
  int res;
  FILE* fp = (FILE*)userdata;
  res = fflush(fp);
  *status = SDL_IO_STATUS_READY;
  return res == 0;
}

static Sint64 stdio_fseek(
  void *userdata, 
  Sint64 offset, 
  SDL_IOWhence whence) {
  Sint64 res;
  FILE* fp = (FILE*)userdata;
  res = (Sint64)fseek(fp, (long)offset, (int)whence);
  return res;
}

static bool stdio_fclose(void* userdata) {
  int res;
  FILE* fp = (FILE*)userdata;
  res = fclose(fp);
  return res == 0;
}

static Sint64 stdio_fsize(void* userdata) {
  (void)userdata;
  return 0;
}

static SDL_IOStream* iostreamfromstdio(FILE* fp) {
  SDL_IOStream* io;
  SDL_IOStreamInterface iosi;
  SDL_INIT_INTERFACE(&iosi);
  iosi.close = stdio_fclose;
  iosi.flush = stdio_flush;
  iosi.read = stdio_fread;
  iosi.write = stdio_fwrite;
  iosi.seek = stdio_fseek;
  iosi.size = stdio_fsize;
  
  io = SDL_OpenIO(&iosi, fp);
  if (io == NULL)
    return NULL;
  return io;
}

SDL_IOStream* lgame_checkfile(lua_State* L, int fi, const char *mode) {
  SDL_IOStream* io = NULL;
  fi = lua_absindex(L, fi);
  if (lua_type(L, fi) == LUA_TSTRING) {
    const char* filename;
    filename = lua_tostring(L, fi);
    io = SDL_IOFromFile(filename, mode);
  }
  else if (lua_type(L, fi) == LUA_TUSERDATA) {
    luaL_Stream* stream;
    stream = (luaL_Stream*)luaL_checkudata(L, fi, LUA_FILEHANDLE);
    if (stream->closef == NULL)
      luaL_error(L, "file is closed");
    if (stream->f == NULL)
      luaL_error(L, "incompletely created file");
    io = iostreamfromstdio(stream->f);
  }
  else {
    luaL_error(L, "expected file like argument (string or FILE*)");
  }
  if (io == NULL)
    luaL_error(L, SDL_GetError());
  return io;
}

bool lgame_closefile(SDL_IOStream* io) {
  return SDL_CloseIO(io);
}