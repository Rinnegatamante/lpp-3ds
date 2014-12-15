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
#include "include/luaAudio.h"

struct wav{
u32 samplerate;
u8* audiobuf;
u8* audiobuf2;
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
	u32 magic,samplerate,bytesRead,jump,chunk=0x00000000;
	u16 audiotype;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x46464952){
	u64 size;
	u32 pos = 16;
	while (chunk != 0x61746164){
	FSFILE_Read(fileHandle, &bytesRead, pos, &jump, 4);
	pos=pos+4+jump;
	FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);
	pos=pos+4;
	}
	FSFILE_GetSize(fileHandle, &size);
	FSFILE_Read(fileHandle, &bytesRead, 22, &audiotype, 2);
	FSFILE_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
	wav *wav_file = (wav*)malloc(sizeof(wav));
	wav_file->samplerate = samplerate;
	if (audiotype == 1){
	wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4));
	FSFILE_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
	wav_file->audiobuf2 = NULL;
	wav_file->size = size-(pos+4);
	}else{
	// I must reordinate my buffer in order to play stereo sound (Thanks CSND/FS libraries .-.)
	u16 bytepersample;
	FSFILE_Read(fileHandle, &bytesRead, 32, &bytepersample, 2);
	u8* tmp_buffer = (u8*)linearAlloc((size-(pos+4)));
	wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))/2);
	wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))/2);
	FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_buffer, size-(pos+4));
	u32 off=0;
	u32 i=0;
	u16 z;
	while (i < (size-(pos+4))){
	z=0;
	while (z < (bytepersample/2)){
	wav_file->audiobuf[off+z] = tmp_buffer[i+z];
	z++;
	}
	z=0;
	while (z < (bytepersample/2)){
	wav_file->audiobuf2[off+z] = tmp_buffer[i+z+(bytepersample/2)];
	z++;
	}
	i=i+bytepersample;
	off=off+(bytepersample/2);
	}
	linearFree(tmp_buffer);
	wav_file->size = (size-(pos+4))/2;
	}
	lua_pushnumber(L,(u32)wav_file);
	}
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
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
    if ((argc != 2) && (argc != 3))	return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	u32 ch = luaL_checkint(L, 2);
	u32 ch2 = luaL_checkint(L, 3);
	GSPGPU_FlushDataCache(NULL, src->audiobuf, src->size);
	if (src->audiobuf2 != NULL){
		My_CSND_playsound(ch, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, NULL, src->size, 0xFFFF, 0xFFFF);
		CSND_setchannel_playbackstate(ch, 1);
		CSND_sharedmemtype0_cmdupdatestate(0);
	}else{
		GSPGPU_FlushDataCache(NULL, src->audiobuf2, src->size);
		My_CSND_playsound(ch, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, NULL, src->size, 0xFFFF, 0);
		My_CSND_playsound(ch2, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf2, NULL, src->size, 0, 0xFFFF);
		CSND_setchannel_playbackstate(ch, 1);
		CSND_setchannel_playbackstate(ch2, 1);
		CSND_sharedmemtype0_cmdupdatestate(0);
	}
	return 0;
}

static int lua_closeWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	linearFree(src->audiobuf);
	free(src);
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
  {"closeWav",				lua_closeWav},
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