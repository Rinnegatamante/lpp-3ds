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
#- Smealum for ctrulib and ftpony src ----------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/utils.h"
#include "include/audio.h"
#include "include/syscalls_norm.h"

// Different kind of syscalls
typedef Result (*func_1x)();
typedef Result (*func_2x_type1)(u32 arg);
typedef Result (*func_3x_type1)(u8 arg, u32 arg2);
typedef Result (*func_3x_type2)(u8 arg, u64 arg2);
typedef Result (*func_9x_type1)(int arg, u32 arg2, u32 arg3, float arg4, float arg5, void* arg6, void* arg7, u32 arg8);

// Different kind of databases for parser optimization
typedef struct{
	char* name;
	func_1x callback;
}func_1x_db;

typedef struct{
	char* name;
	func_2x_type1 callback;
}func_2x_db;

typedef struct{
	char* name;
	func_3x_type1 callback_type1; // argsize 5 (1+4)
	func_3x_type2 callback_type2; // argsize 9 (1+4(pointer)) since Lua doesn't support 64bit integers natively
}func_3x_db;
typedef struct{
	char* name;
	func_9x_type1 callback_type1; // csndPlaysound-like calls
}func_9x_db;



// No args syscalls
func_1x_db db_1x[]={
	{"amInit", amInit},
	{"amExit", amExit_norm},
	{"csndInit", csndInit},
	{"ndspInit", ndspInit},
	{"csndExit", csndExit_norm},
	{"ndspExit", ndspExit_norm},
};

// One arg syscall
func_2x_db db_2x[]={
	// args size = 4
	{"AM_CancelCIAInstall", (func_2x_type1)AM_CancelCIAInstall}, // 0
	{"AM_FinishCiaInstall", (func_2x_type1)AM_FinishCiaInstall}, // 1
};

// Two args syscalls
func_3x_db db_3x[]={
	// type 1
	{"AM_StartCiaInstall", (func_3x_type1)AM_StartCiaInstall}, // 0
	{"AM_GetTitleCount", (func_3x_type1)AM_GetTitleCount}, // 1
	// type 2
	{"AM_DeleteTitle", NULL,(func_3x_type2)AM_DeleteTitle}, // 2
	{"AM_DeleteAppTitle", NULL,(func_3x_type2)AM_DeleteAppTitle}, // 3
};


// Two args syscalls
func_9x_db db_9x[]={
	// type 1
	{"csndPlaysound", (func_9x_type1)csndPlaySound}, // 0
}; 

static int lua_service(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char* srv = luaL_checkstring(L,1);
	Handle tmp;
	srvGetServiceHandle(&tmp, srv);
	lua_pushboolean(L, tmp);
	svcCloseHandle(tmp);
	
	// We have to manually restart a service if it's a default one of lpp-3ds
	char* handle_list[10] = {
		"ptm:u", "am", "cfg:u", "hid",
		"ir:rst", "fs", "hb",
		"apt", "srv", "ac"
	};
	func_1x restartService[10] = {
		ptmuInit, amInit, cfguInit, hidInit,
		irrstInit, fsInit, hbInit,
		aptInit, srvInit, acInit
		
	};
	for (int i=0; i<10; i++){
		if (strstr(srv,handle_list[i]) != NULL) (restartService[i])();
	}
	
	return 1;
}

static int lua_execall(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc == 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char* call = luaL_checkstring(L,1);
	
	// Parsing known syscalls
	if (argc == 1){
		u32 known_syscalls = sizeof(db_1x) / sizeof(func_1x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_1x[i].name,call) == 0){
				lua_pushinteger(L, db_1x[i].callback());
				return 1;
			}
		}
	}else if (argc == 2){
		u32 known_syscalls = sizeof(db_2x) / sizeof(func_2x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_2x[i].name,call) == 0){
				lua_pushinteger(L, db_2x[i].callback((u32)luaL_checkbuffer(L,1)));
				return 1;
			}
		}
	}else if (argc == 3){
		u32 known_syscalls = sizeof(db_3x) / sizeof(func_3x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_3x[i].name,call) == 0){
				
				// Parsing arguments according to syscall
				if (i <= 1){
					u8 arg1 = luaL_checkinteger(L, 2);
					u32 arg2 = luaL_checkinteger(L, 3);
					lua_pushinteger(L, db_3x[i].callback_type1(arg1,arg2));
				}else{
					u8 arg1 = luaL_checkinteger(L, 2);
					u64* arg2 = (u64*)luaL_checkinteger(L, 3);
					lua_pushinteger(L, db_3x[i].callback_type2(arg1,*arg2));
				
				}
				return 1;
			}
		}
	}else if (argc == 8){
		u32 known_syscalls = sizeof(db_3x) / sizeof(func_3x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_3x[i].name,call) == 0){
				int arg1 = luaL_checkinteger(L, 1);
				u32 arg2 = (u32)luaL_checkinteger(L, 2);
				u32 arg3 = (u32)luaL_checkinteger(L, 3);
				float arg4 = luaL_checknumber(L, 4);
				float arg5 = luaL_checknumber(L, 5);
				void* arg6 = luaL_checkbuffer(L, 6);
				void* arg7 = luaL_checkbuffer(L, 7);
				u32 arg8 = (u32)luaL_checkinteger(L, 8);
				// Parsing arguments according to syscall
				if (i <= 0){
					lua_pushinteger(L, db_9x[i].callback_type1(arg1,arg2,arg3,arg4,arg5,arg6,arg7,arg8));
				
				}
				return 1;
			}
		}
	}
	return luaL_error(L, "unknown syscall.");
}

static int lua_readword(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32* word = (u32*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, *word);
	return 1;
}

static int lua_storeword(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 2) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32* offs = (u32*)luaL_checkinteger(L, 1);
	u32 word = (u32)luaL_checkinteger(L, 2);
	*offs = word;
	return 0;
}

static int lua_getfh(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	Handle hdl = luaL_checkinteger(L, 1);
	fileStream* result = (fileStream*)malloc(sizeof(fileStream));
	result->isRomfs = false;
	result->handle = (u32)hdl;
	result->magic = 0xBEEFDEAD;
	lua_pushinteger(L,(u32)result);
	return 1;
}

static int lua_alloc(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 size = (u32)luaL_checkinteger(L, 1);
	u8* memblock = (u8*)malloc(size);
	lua_pushinteger(L, (u32)memblock);
	return 1;
}

static int lua_alloc2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 size = (u32)luaL_checkinteger(L, 1);
	u8* memblock = (u8*)linearAlloc(size);
	lua_pushinteger(L, (u32)memblock);
	return 1;
}

static int lua_free(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	void* offset = (void*)luaL_checkinteger(L, 1);
	free(offset);
	return 0;
}

static int lua_free2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	void* offset = (void*)luaL_checkinteger(L, 1);
	linearFree(offset);
	return 0;
}

static int lua_getraw(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	wav* block = (wav*)luaL_checkinteger(L, 1);
	if (block->magic == 0x4C534E44){
		lua_pushinteger(L, (u32)block->audiobuf);
		if (block->audiobuf2 != NULL) lua_pushinteger(L, (u32)block->audiobuf2);
		else lua_pushnil(L);
	}else return luaL_error(L, "unsupported memory block type.");
	return 2;
}

//Register our Core Functions
static const luaL_Reg Core_functions[] = {
	{"checkService",        lua_service},
	{"execCall",            lua_execall},
	{"getHandle",           lua_getfh},
	{"getRawData",          lua_getraw},
	{"readWord",            lua_readword},
	{"storeWord",           lua_storeword},
	{"free",                lua_free},
	{"alloc",               lua_alloc},
	{"linearFree",          lua_free2},
	{"linearAlloc",         lua_alloc2},
	{0, 0}
};

void luaCore_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Core_functions, 0);
	lua_setglobal(L, "Core");
}