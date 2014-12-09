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
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"

struct wav{
u32 samplerate;
u8* audiobuf;
u32 size;
};

static int lua_openwav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
	u32 samplerate,bytesRead;
	u64 size;
	FSFILE_GetSize(fileHandle, &size);
	u8* audiobuf = (u8*)linearAlloc(size-44);
	FSFILE_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
	FSFILE_Read(fileHandle, &bytesRead, 44, audiobuf, size-44);
	wav *wav_file = (wav*)malloc(sizeof(wav));
	wav_file->audiobuf = audiobuf;
	wav_file->samplerate = samplerate;
	wav_file->size = size-44;
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	lua_pushnumber(L,(u32)wav_file);
	return 1;
}

static int lua_soundinit(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	CSND_initialize(NULL);
	return 0;
}

static int lua_playWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	u32 ch = luaL_checkint(L, 2);
	GSPGPU_FlushDataCache(NULL, src->audiobuf, src->size);
	CSND_playsound(ch, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, NULL, src->size, 2, 0);
	return 0;
}

static int lua_pause(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	u32 ch = luaL_checkint(L, 1);
	CSND_setchannel_playbackstate(ch, 0);
	CSND_sharedmemtype0_cmdupdatestate(0);
	return 0;
}

static int lua_resume(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	u32 ch = luaL_checkint(L, 1);
	CSND_setchannel_playbackstate(ch, 1);
	CSND_sharedmemtype0_cmdupdatestate(0);
	return 0;
}

static int lua_soundend(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	CSND_shutdown();
	return 0;
}

//Register our Sound Functions
static const luaL_Reg Sound_functions[] = {
  {"openWav",				lua_openwav},
  {"playWav",				lua_playWav},
  {"init",					lua_soundinit},
  {"term",					lua_soundend},
  {"pause",					lua_pause},
  {"resume",				lua_resume},
  {0, 0}
};

void luaSound_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Sound_functions, 0);
	lua_setglobal(L, "Sound");
}