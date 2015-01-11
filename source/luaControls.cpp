/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> ---------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> -------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib -------------------------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Sean Barrett for stb_truetype ---------------------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"
#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

static int lua_readInit(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	hidScanInput();
	irrstScanInput();
	return 0;
}

static int lua_readC(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	lua_pushnumber(L, hidKeysHeld());
	return 1;
}

static int lua_check(lua_State *L)
{
        if (lua_gettop(L) != 2) return luaL_error(L, "wrong number of arguments.");
		u32 pad = luaL_checknumber(L, 1);
		u32 button = luaL_checknumber(L, 2);
        lua_pushboolean(L, ((pad & button) == button));
        return 1;
}

static int lua_circlepad(lua_State *L)
{
        if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
		circlePosition cpos;
		hidCircleRead(&cpos);
		lua_pushnumber(L, cpos.dx);
		lua_pushnumber(L, cpos.dy);
        return 2;
}

static int lua_touchpad(lua_State *L)
{
        if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
		touchPosition cpos;
		hidTouchRead(&cpos);
		lua_pushnumber(L, cpos.px);
		lua_pushnumber(L, cpos.py);
        return 2;
}

static int lua_cstickpad(lua_State *L)
{
        if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
		circlePosition cpos;
		hidCstickRead(&cpos);
		lua_pushnumber(L, cpos.dx);
		lua_pushnumber(L, cpos.dy);
        return 2;
}

static int lua_volume(lua_State *L)
{
        if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
		angularRate cpos;
		hidGyroRead(&cpos);
		u8 value;
		HIDUSER_GetSoundVolume(&value);
		lua_pushnumber(L, value);
		return 1;
}

//Register our Controls Functions
static const luaL_Reg Controls_functions[] = {
  {"init",                				lua_readInit},
  {"read",								lua_readC},		  
  {"check",								lua_check},	
  {"readCirclePad",						lua_circlepad},	
  {"readTouch",							lua_touchpad},	
  {"readCstickPad",						lua_cstickpad},	
  {"getVolume",							lua_volume},	
  {0, 0}
};

void luaControls_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Controls_functions, 0);
	lua_setglobal(L, "Controls");
	VariableRegister(L,KEY_A);
	VariableRegister(L,KEY_B);
	VariableRegister(L,KEY_SELECT);
	VariableRegister(L,KEY_START);
	VariableRegister(L,KEY_DRIGHT);
	VariableRegister(L,KEY_DLEFT);
	VariableRegister(L,KEY_DUP);
	VariableRegister(L,KEY_DDOWN);
	VariableRegister(L,KEY_R);
	VariableRegister(L,KEY_L);
	VariableRegister(L,KEY_X);
	VariableRegister(L,KEY_Y);
	VariableRegister(L,KEY_ZL);
	VariableRegister(L,KEY_ZR);
	VariableRegister(L,KEY_TOUCH);
}