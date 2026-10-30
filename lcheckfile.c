

#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lcheckfile.h"



SDL_IOStream* lgame_checkreadfile(lua_State* L, int fi) {
  SDL_IOStream* io = NULL;
  fi = lua_absindex(L, fi);
  if (lua_type(L, fi) == LUA_TSTRING) {
    const char* filename;
    filename = lua_tostring(L, fi);
    io = SDL_IOFromFile(filename, "rb");
  }
  else if (lua_type(L, fi) == LUA_TUSERDATA) {
    luaL_Stream* stream;
    const char* bytes;
    size_t szbytes;
    stream = (luaL_Stream*)luaL_checkudata(L, fi, LUA_FILEHANDLE);
    if (stream->closef == NULL)
      luaL_error(L, "file is closed");
    if (stream->f == NULL)
      luaL_error(L, "incompletely created file");
    lua_getfield(L, fi, "read");
    lua_pushvalue(L, fi);
    lua_pushstring(L, "a");
    if (lua_pcall(L, 2, 1, 0) != LUA_OK)
      luaL_error(L, lua_tostring(L, -1));
    bytes = luaL_checklstring(L, -1, &szbytes); lua_pop(L, 1);
    io = SDL_IOFromConstMem(bytes, szbytes);
  }
  else {
    luaL_error(L, "expected file like argument (string or FILE*)");
  }
  if (io == NULL)
    luaL_error(L, SDL_GetError());
  return io;
}

bool lgame_closereadfile(lua_State *L, SDL_IOStream *io, int err) {
  bool res = SDL_CloseIO(io);
  if (!res && err)
    return luaL_error(L, SDL_GetError());
  return res;
}

SDL_IOStream* lgame_checkwritefile(lua_State* L, int fi, int *dyn) {
  SDL_IOStream* io = NULL;
  fi = lua_absindex(L, fi);
  if (dyn) *dyn = 0;
  if (lua_type(L, fi) == LUA_TSTRING) {
    const char* filename;
    filename = lua_tostring(L, fi);
    io = SDL_IOFromFile(filename, "wb");
  }
  else if (lua_type(L, fi) == LUA_TUSERDATA) {
    io = SDL_IOFromDynamicMem();
    if (dyn) *dyn = 1;
  }
  else {
    luaL_error(L, "expected file like argument (string or FILE*)");
  }
  if (io == NULL)
    luaL_error(L, SDL_GetError());
  return io;
}

bool lgame_closewritefile(lua_State *L, SDL_IOStream* io, int fi, int dyn, int err) {
  if (dyn) {
    SDL_PropertiesID prop;
    void* mem;
    size_t memsz;
    fi = lua_absindex(L, fi);
    prop = SDL_GetIOProperties(io);
    if (prop == 0) {
      SDL_CloseIO(io);
      if (err) luaL_error(L, SDL_GetError());
      return false;
    }
    mem = SDL_GetPointerProperty(prop, SDL_PROP_IOSTREAM_DYNAMIC_MEMORY_POINTER, NULL);
    if (mem == NULL) {
      SDL_CloseIO(io);
      if (err) luaL_error(L, SDL_GetError());
      return false;
    }
    memsz = SDL_GetIOSize(io);
    if (memsz < 0) {
      SDL_CloseIO(io);
      if (err) luaL_error(L, SDL_GetError());
      return false;
    }
    lua_getfield(L, fi, "write");
    lua_pushvalue(L, fi);
    lua_pushlstring(L, (const char*)mem, memsz);
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
      SDL_CloseIO(io);
      luaL_error(L, lua_tostring(L, -1));
    }
    lua_pop(L, 1);
  }
  SDL_CloseIO(io);
  return true;
}