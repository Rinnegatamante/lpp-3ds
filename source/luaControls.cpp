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

int KEY_HOME = 0xFFFF;
int KEY_POWER = 0xFFFE;
extern bool isTopLCDOn;
extern bool isBottomLCDOn;

static int lua_readC(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	hidScanInput();
	irrstScanInput();
	lua_pushinteger(L, hidKeysHeld());
	return 1;
}

static int lua_check(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 pad = luaL_checkinteger(L, 1);
	u32 button = luaL_checkinteger(L, 2);
	if (button == KEY_HOME){
		APT_AppStatus status = aptGetStatus();
		lua_pushboolean(L,((status == APP_SUSPENDING) && aptGetStatusPower() == 0));
	}else if (button == KEY_POWER){
		APT_AppStatus status = aptGetStatus();
		lua_pushboolean(L,((status == APP_SUSPENDING) && aptGetStatusPower() == 1));
	}else lua_pushboolean(L, ((pad & button) == button));
	return 1;
}

static int lua_circlepad(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	circlePosition cpos;
	hidScanInput();
	hidCircleRead(&cpos);
	lua_pushinteger(L, cpos.dx);
	lua_pushinteger(L, cpos.dy);
	return 2;
}

static int lua_headset(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	lua_pushboolean(L,*(u8*)0x1FF810C0);
	return 1;
}

static int lua_touchpad(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (lua_gettop(L) != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	touchPosition cpos;
	hidScanInput();
	hidTouchRead(&cpos);
	lua_pushinteger(L, cpos.px);
	lua_pushinteger(L, cpos.py);
	return 2;
}

static int lua_gyro(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	angularRate cpos;
	hidScanInput();
	hidGyroRead(&cpos);
	lua_pushnumber(L, cpos.x);
	lua_pushnumber(L, cpos.y);
	lua_pushnumber(L, cpos.z);
	return 3;
}

static int lua_accel(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	accelVector cpos;
	hidScanInput();
	hidAccelRead(&cpos);
	lua_pushnumber(L, cpos.x);
	lua_pushnumber(L, cpos.y);
	lua_pushnumber(L, cpos.z);
	return 3;
}

static int lua_cstickpad(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	circlePosition cpos;
	hidScanInput();
	hidCstickRead(&cpos);
	lua_pushinteger(L, cpos.dx);
	lua_pushinteger(L, cpos.dy);
	return 2;
}

static int lua_eg(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	HIDUSER_EnableGyroscope();
	return 0;
}

static int lua_ea(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	HIDUSER_EnableAccelerometer();
	return 0;
}

static int lua_dg(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	HIDUSER_DisableGyroscope();
	return 0;
}

static int lua_da(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	HIDUSER_DisableAccelerometer();
	return 0;
}

static int lua_volume(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	u8 value;
	HIDUSER_GetSoundVolume(&value);
	lua_pushnumber(L, value);
	return 1;
}

static int lua_lcdon(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 screen = luaL_checkinteger(L, 1);
	u32 flag;
	if (screen == 0) flag = GSPLCD_SCREEN_TOP;
	else if (screen==1) flag = GSPLCD_SCREEN_BOTTOM;
	else return luaL_error(L, "wrong parameter.");
	if (gspLcdInit() == 0){
		GSPLCD_PowerOnBacklight(flag);
		gspLcdExit();
		if (flag == GSPLCD_SCREEN_TOP) isTopLCDOn = true;
		else isBottomLCDOn = true;
	}
	return 0;
}

static int lua_lcdoff(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 screen = luaL_checkinteger(L, 1);
	u32 flag;
	if (screen == 0) flag = GSPLCD_SCREEN_TOP;
	else if (screen==1) flag = GSPLCD_SCREEN_BOTTOM;
	else return luaL_error(L, "wrong parameter.");
	if (gspLcdInit() == 0){
		GSPLCD_PowerOffBacklight(flag);
		gspLcdExit();
		if (flag == GSPLCD_SCREEN_TOP) isTopLCDOn = false;
		else isBottomLCDOn = false;
	}
	return 0;
}

static int lua_shell(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	u8 shell;
	PTMU_GetShellState(&shell);
	lua_pushboolean(L, shell);
	return 1;
}

//Register our Controls Functions
static const luaL_Reg Controls_functions[] = {
  {"read",             lua_readC},		  
  {"check",            lua_check},	
  {"enableGyro",       lua_eg},	
  {"enableAccel",      lua_ea},	
  {"disableGyro",      lua_dg},	
  {"disableAccel",     lua_da},	
  {"readCirclePad",    lua_circlepad},	
  {"readTouch",        lua_touchpad},	
  {"readGyro",         lua_gyro},
  {"readAccel",        lua_accel},	
  {"readCstickPad",    lua_cstickpad},	
  {"getVolume",        lua_volume},
  {"headsetStatus",    lua_headset},
  {"shellStatus",      lua_shell},
  {"enableScreen",     lua_lcdon},
  {"disableScreen",    lua_lcdoff},
  {0, 0}
};

void luaControls_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Controls_functions, 0);
	lua_setglobal(L, "Controls");
	VariableRegister(L,KEY_HOME);
	VariableRegister(L,KEY_POWER);
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