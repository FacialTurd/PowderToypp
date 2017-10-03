#include "../LuaCompat.h"
void luaopen_socket(lua_State *l);
#ifndef NO_SCRIPT_MANAGER
void luacon_loadscriptmanager(lua_State *l);
#endif