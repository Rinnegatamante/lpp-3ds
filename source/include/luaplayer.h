#ifndef __LUAPLAYER_H
#define __LUAPLAYER_H

#include <stdlib.h>
//#include <tdefs.h> //Not needed for compilation via Ubuntu (complains it's missing)
#include "lua/lua.hpp"

extern void luaC_collectgarbage (lua_State *L);

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(val, min, max) ((val)>(max)?(max):((val)<(min)?(min):(val)))

#define UserdataStubs(HANDLE, DATATYPE) \
DATATYPE *to##HANDLE (lua_State *L, int index) \
{ \
  DATATYPE* handle  = (DATATYPE*)lua_touserdata(L, index); \
  if (handle == NULL) luaL_typerror(L, index, #HANDLE); \
  return handle; \
} \
DATATYPE* push##HANDLE(lua_State *L) { \
	DATATYPE * newvalue = (DATATYPE*)lua_newuserdata(L, sizeof(DATATYPE)); \
	luaL_getmetatable(L, #HANDLE); \
	lua_pushvalue(L, -1); \
	lua_setmetatable(L, -3); \
	lua_pushstring(L, "__index"); \
	lua_pushstring(L, #HANDLE); \
	lua_gettable(L, LUA_GLOBALSINDEX); \
	lua_settable(L, -3); \
	lua_pop(L, 1); \
	return newvalue; \
}

#define UserdataRegister(HANDLE, METHODS, METAMETHODS) \
int HANDLE##_register(lua_State *L) { \
	luaL_newmetatable(L, #HANDLE);  /* create new metatable for file handles */ \
	lua_pushliteral(L, "__index"); \
	lua_pushvalue(L, -2);  /* push metatable */ \
	lua_rawset(L, -3);  /* metatable.__index = metatable */ \
	\
	luaL_openlib(L, 0, METAMETHODS, 0); \
	luaL_openlib(L, #HANDLE, METHODS, 0); \
	\
	lua_pushstring(L, #HANDLE); \
	lua_gettable(L, LUA_GLOBALSINDEX); \
	luaL_getmetatable(L, #HANDLE); \
	lua_setmetatable(L, -2); \
	return 1; \
}

const char *runScript(const char* script, bool isStringBuffer);
void luaC_collectgarbage (lua_State *L);

void luaScreen_init(lua_State *L);

void stackDump (lua_State *L);

#endif
