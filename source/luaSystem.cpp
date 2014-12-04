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
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib -------------------------------------------------------------------------------------------------#
#- Aurelio for testing & bug-fixing ------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

int FREAD = 0;
int FWRITE = 1;
int FCREATE = 2;

static int lua_exit(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	return luaL_error(L, string); // NOTE: This is a fake error
}

static int lua_openfile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	int type = luaL_checkint(L, 2);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret;
	switch(type){
		case 0:
			ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
			break;
		case 1:
			ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
			break;
		case 2:
			ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
			break;
	}
	if(ret) return luaL_error(L, "error opening file");
	lua_pushnumber(L,fileHandle);
	return 1;
}

static int lua_getsize(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 size;
	Result ret=FSFILE_GetSize(fileHandle, &size);
	if(ret) return luaL_error(L, "error getting size");
	lua_pushnumber(L,size);
	return 1;
}

static int lua_closefile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	Result ret=FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	if(ret) return luaL_error(L, "error closing file");
	return 0;
}

static int lua_readfile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 init = luaL_checknumber(L, 2);
	u64 size = luaL_checknumber(L, 3);
	u32 bytesRead;
	unsigned char *buffer = (unsigned char*)(malloc((size+1) * sizeof (char)));
	Result ret=FSFILE_Read(fileHandle, &bytesRead, init, buffer, size);
	buffer[size] = 0;
	if(ret || size!=bytesRead) return luaL_error(L, "error reading file");
	lua_pushstring(L,(const char*)buffer);
	free(buffer);
	return 1;
}

static int lua_writefile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 init = luaL_checknumber(L, 2);
	const char *text = luaL_checkstring(L, 3);
	u64 size = strlen(text);
	u32 bytesWritten;
	Result ret=FSFILE_Write(fileHandle, &bytesWritten, init, text, size, FS_WRITE_FLUSH);
	if(ret || size!=bytesWritten) return luaL_error(L, "error writing file");
	return 0;
}

static int lua_getFW(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushnumber(L,osGetFirmVersion());
    return 1;
}

static int lua_getK(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushnumber(L,osGetKernelVersion());
    return 1;
}

//Register our Screen Functions
static const luaL_Reg System_functions[] = {
  {"exit",					lua_exit},
  {"getFirmware",			lua_getFW},
  {"getKernel",				lua_getK},
  {"openFile",				lua_openfile},
  {"getFileSize",			lua_getsize},
  {"closeFile",				lua_closefile},
  {"readFile",				lua_readfile},
  {"writeFile",				lua_writefile},
  {0, 0}
};

void luaSystem_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, System_functions, 0);
	lua_setglobal(L, "System");
	VariableRegister(L,FREAD);
	VariableRegister(L,FWRITE);
	VariableRegister(L,FCREATE);
}