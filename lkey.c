
#include <lua/lua.h>
#include <lua/lualib.h>
#include <lua/lauxlib.h>

#include "lkey.h"


static int key_get_focused(lua_State* L);
static int key_get_pressed(lua_State* L);
static int key_get_mods(lua_State* L);
static int key_set_mods(lua_State* L);
static int key_name(lua_State* L);
static int key_key_code(lua_State* L);

static int ScancodeWrapper__index(lua_State* L);

const struct luaL_Reg lgame_key_module[] = {
  {"get_focused", key_get_focused},
  {"get_pressed", key_get_pressed},
  {"get_mods", key_get_mods},
  {"set_mods", key_set_mods},
  {"name", key_name},
  {"key_code", key_key_code},
  {NULL, NULL}
};

const struct luaL_Reg lgame_ScancodeWrapper_methods[] = {
  {"__index", ScancodeWrapper__index},
  {NULL, NULL}
};

static const struct {
  const SDL_Keycode key;
  const char* name;
} key2name[] = {
  {0, ""},                         /* K_UNKNOWN */
  {8, "backspace"},                /* K_BACKSPACE */
  {9, "tab"},                      /* K_TAB */
  {13, "return"},                  /* K_RETURN */
  {27, "escape"},                  /* K_ESCAPE */
  {32, "space"},                   /* K_SPACE */
  {33, "!"},                       /* K_EXCLAIM */
  {34, "\""},                      /* K_QUOTEDBL */
  {35, "#"},                       /* K_HASH */
  {36, "$"},                       /* K_DOLLAR */
  {37, "%"},                       /* K_PERCENT */
  {38, "&"},                       /* K_AMPERSAND */
  {39, "'"},                       /* K_QUOTE */
  {40, "("},                       /* K_LEFTPAREN */
  {41, ")"},                       /* K_RIGHTPAREN */
  {42, "*"},                       /* K_ASTERISK */
  {43, "+"},                       /* K_PLUS */
  {44, ","},                       /* K_COMMA */
  {45, "-"},                       /* K_MINUS */
  {46, "."},                       /* K_PERIOD */
  {47, "/"},                       /* K_SLASH */
  {48, "0"},                       /* K_0 */
  {49, "1"},                       /* K_1 */
  {50, "2"},                       /* K_2 */
  {51, "3"},                       /* K_3 */
  {52, "4"},                       /* K_4 */
  {53, "5"},                       /* K_5 */
  {54, "6"},                       /* K_6 */
  {55, "7"},                       /* K_7 */
  {56, "8"},                       /* K_8 */
  {57, "9"},                       /* K_9 */
  {58, ":"},                       /* K_COLON */
  {59, ";"},                       /* K_SEMICOLON */
  {60, "<"},                       /* K_LESS */
  {61, "="},                       /* K_EQUALS */
  {62, ">"},                       /* K_GREATER */
  {63, "?"},                       /* K_QUESTION */
  {64, "@"},                       /* K_AT */
  {91, "["},                       /* K_LEFTBRACKET */
  {92, "\\"},                      /* K_BACKSLASH */
  {93, "]"},                       /* K_RIGHTBRACKET */
  {94, "^"},                       /* K_CARET */
  {95, "_"},                       /* K_UNDERSCORE */
  {96, "`"},                       /* K_BACKQUOTE */
  {97, "a"},                       /* K_a */
  {98, "b"},                       /* K_b */
  {99, "c"},                       /* K_c */
  {100, "d"},                      /* K_d */
  {101, "e"},                      /* K_e */
  {102, "f"},                      /* K_f */
  {103, "g"},                      /* K_g */
  {104, "h"},                      /* K_h */
  {105, "i"},                      /* K_i */
  {106, "j"},                      /* K_j */
  {107, "k"},                      /* K_k */
  {108, "l"},                      /* K_l */
  {109, "m"},                      /* K_m */
  {110, "n"},                      /* K_n */
  {111, "o"},                      /* K_o */
  {112, "p"},                      /* K_p */
  {113, "q"},                      /* K_q */
  {114, "r"},                      /* K_r */
  {115, "s"},                      /* K_s */
  {116, "t"},                      /* K_t */
  {117, "u"},                      /* K_u */
  {118, "v"},                      /* K_v */
  {119, "w"},                      /* K_w */
  {120, "x"},                      /* K_x */
  {121, "y"},                      /* K_y */
  {122, "z"},                      /* K_z */
  {127, "delete"},                 /* K_DELETE */
  {1073741881, "caps lock"},       /* K_CAPSLOCK */
  {1073741882, "f1"},              /* K_F1 */
  {1073741883, "f2"},              /* K_F2 */
  {1073741884, "f3"},              /* K_F3 */
  {1073741885, "f4"},              /* K_F4 */
  {1073741886, "f5"},              /* K_F5 */
  {1073741887, "f6"},              /* K_F6 */
  {1073741888, "f7"},              /* K_F7 */
  {1073741889, "f8"},              /* K_F8 */
  {1073741890, "f9"},              /* K_F9 */
  {1073741891, "f10"},             /* K_F10 */
  {1073741892, "f11"},             /* K_F11 */
  {1073741893, "f12"},             /* K_F12 */
  {1073741894, "print screen"},    /* K_PRINT, K_PRINTSCREEN */
  {1073741895, "scroll lock"},     /* K_SCROLLLOCK, K_SCROLLOCK */
  {1073741896, "break"},           /* K_BREAK, K_PAUSE */
  {1073741897, "insert"},          /* K_INSERT */
  {1073741898, "home"},            /* K_HOME */
  {1073741899, "page up"},         /* K_PAGEUP */
  {1073741901, "end"},             /* K_END */
  {1073741902, "page down"},       /* K_PAGEDOWN */
  {1073741903, "right"},           /* K_RIGHT */
  {1073741904, "left"},            /* K_LEFT */
  {1073741905, "down"},            /* K_DOWN */
  {1073741906, "up"},              /* K_UP */
  {1073741907, "numlock"},         /* K_NUMLOCK, K_NUMLOCKCLEAR */
  {1073741908, "[/]"},             /* K_KP_DIVIDE */
  {1073741909, "[*]"},             /* K_KP_MULTIPLY */
  {1073741910, "[-]"},             /* K_KP_MINUS */
  {1073741911, "[+]"},             /* K_KP_PLUS */
  {1073741912, "enter"},           /* K_KP_ENTER */
  {1073741913, "[1]"},             /* K_KP1, K_KP_1 */
  {1073741914, "[2]"},             /* K_KP2, K_KP_2 */
  {1073741915, "[3]"},             /* K_KP3, K_KP_3 */
  {1073741916, "[4]"},             /* K_KP4, K_KP_4 */
  {1073741917, "[5]"},             /* K_KP5, K_KP_5 */
  {1073741918, "[6]"},             /* K_KP6, K_KP_6 */
  {1073741919, "[7]"},             /* K_KP7, K_KP_7 */
  {1073741920, "[8]"},             /* K_KP8, K_KP_8 */
  {1073741921, "[9]"},             /* K_KP9, K_KP_9 */
  {1073741922, "[0]"},             /* K_KP0, K_KP_0 */
  {1073741923, "[.]"},             /* K_KP_PERIOD */
  {1073741926, "power"},           /* K_POWER */
  {1073741927, "equals"},          /* K_KP_EQUALS */
  {1073741928, "f13"},             /* K_F13 */
  {1073741929, "f14"},             /* K_F14 */
  {1073741930, "f15"},             /* K_F15 */
  {1073741941, "help"},            /* K_HELP */
  {1073741942, "menu"},            /* K_MENU */
  {1073741978, "sys req"},         /* K_SYSREQ */
  {1073741980, "clear"},           /* K_CLEAR */
  {1073742004, "euro"},            /* K_CURRENCYUNIT, K_EURO */
  {1073742005, "CurrencySubUnit"}, /* K_CURRENCYSUBUNIT */
  {1073742048, "left ctrl"},       /* K_LCTRL */
  {1073742049, "left shift"},      /* K_LSHIFT */
  {1073742050, "left alt"},        /* K_LALT */
  {1073742051, "left meta"},       /* K_LGUI, K_LMETA, K_LSUPER */
  {1073742052, "right ctrl"},      /* K_RCTRL */
  {1073742053, "right shift"},     /* K_RSHIFT */
  {1073742054, "right alt"},       /* K_RALT */
  {1073742055, "right meta"},      /* K_RGUI, K_RMETA, K_RSUPER */
  {1073742081, "alt gr"},          /* K_MODE */
  {1073742094, "AC Back"},         /* K_AC_BACK */
};

static SDL_Keycode keycode_from_name(const char* name, bool compat) {
  if (compat) {
    int arrsz = (int)SDL_arraysize(key2name);
    for (int i = 0; i < arrsz; i++) {
      if (!strcmp(key2name[i].name, name))
        return key2name[i].key;
    }
    return SDL_GetKeyFromName(name);
  }
  else
    return SDL_GetKeyFromName(name);
}

static int ScancodeWrapper__index(lua_State* L) {
  int nkeys;
  SDL_Keycode key;
  SDL_Scancode sc;
  const bool* keys;
  luaL_checkudata(L, 1, LGAME_SCANCODEWRAPPER_METATABLE);
  key = (SDL_Keycode)luaL_checkinteger(L, 2);
  keys = SDL_GetKeyboardState(&nkeys);
  sc = SDL_GetScancodeFromKey(key, NULL);
  if (key == SDLK_UNKNOWN || sc == SDL_SCANCODE_UNKNOWN)
    return luaL_error(L, "unknown key");
  if (sc >= nkeys)
    return luaL_error(L, "key out of range");
  lua_pushboolean(L, keys[sc]);
  return 1;
}

static int key_get_focused(lua_State* L) {
  lua_pushboolean(L, SDL_GetKeyboardFocus() != NULL);
  return 1;
}

static int key_get_pressed(lua_State* L) {
  lua_newuserdata(L, sizeof(void*));
  luaL_getmetatable(L, LGAME_SCANCODEWRAPPER_METATABLE);
  lua_setmetatable(L, -2);
  return 1;
}

static int key_get_mods(lua_State* L) {
  lua_pushinteger(L, SDL_GetModState());
  return 1;
}

static int key_set_mods(lua_State* L) {
  SDL_Keymod mod = (SDL_Keymod)luaL_checkinteger(L, 1);
  SDL_SetModState(mod);
  return 0;
}

static int key_name(lua_State* L) {
  SDL_Keycode key = (SDL_Keycode)luaL_checkinteger(L, 1);
  lua_pushstring(L, SDL_GetKeyName(key));
  return 1;
}

static int key_key_code(lua_State* L) {
  const char* name;
  int compat = 1;
  SDL_Keycode key;
  name = luaL_checkstring(L, 1);
  if (!lua_isnoneornil(L, 2))
    compat = lua_toboolean(L, 2);
  key = keycode_from_name(name, compat);
  if (key == SDLK_UNKNOWN)
    return luaL_error(L, "unknown key name");
  lua_pushinteger(L, key);
  return 1;
}
