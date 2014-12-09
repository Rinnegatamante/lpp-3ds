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
#include "include/luaGraphics.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

struct BMPV{
	Handle sourceFile;
	u32 currentFrame;
	u32 width;
	u32 height;
	u32 framerate;
	u64 tot_frame;
	u64 tick;
	u32 audio_size;
	u32 samplerate;
	u8* audiobuf;
	bool isPlaying;
	int loop;
};

static int lua_loadBMPV(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
	u32 magic,width,height,framerate,frame_size,bytesRead,audio_size,samplerate;
	u64 size;
	FSFILE_GetSize(fileHandle, &size);
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x56504D42){
	BMPV* BMPV_file = (BMPV*)malloc(sizeof(BMPV));
	FSFILE_Read(fileHandle, &bytesRead, 4, &framerate, 4);
	FSFILE_Read(fileHandle, &bytesRead, 8, &width, 4);
	FSFILE_Read(fileHandle, &bytesRead, 12, &height, 4);
	FSFILE_Read(fileHandle, &bytesRead, 16, &samplerate, 4);
	FSFILE_Read(fileHandle, &bytesRead, 20, &audio_size, 4);
	BMPV_file->framerate = framerate;
	BMPV_file->width = width;
	BMPV_file->height = height;
	BMPV_file->audio_size = audio_size;
	BMPV_file->isPlaying = false;
	BMPV_file->currentFrame = 0;
	BMPV_file->sourceFile = fileHandle;
	BMPV_file->tick = 0;
	BMPV_file->audiobuf = NULL;
	BMPV_file->samplerate = samplerate;
	frame_size = width*height*3;
	int tot_frame = (size-24-audio_size)/frame_size;
	BMPV_file->tot_frame = tot_frame;
	lua_pushnumber(L, (u32)BMPV_file);
	}
	return 1;
}

//Custom CSND_playsound to prevent audio desynchronization
Result My_CSND_playsound(u32 channel, u32 looping, u32 encoding, u32 samplerate, u32 *vaddr0, u32 *vaddr1, u32 totalbytesize, u32 unk0, u32 unk1)
{
u32 physaddr0 = 0;
u32 physaddr1 = 0;
physaddr0 = osConvertVirtToPhys((u32)vaddr0);
physaddr1 = osConvertVirtToPhys((u32)vaddr1);
CSND_sharedmemtype0_cmde(channel, looping, encoding, samplerate, unk0, unk1, physaddr0, physaddr1, totalbytesize);
CSND_sharedmemtype0_cmd8(channel, samplerate);
if(looping)
{
if(physaddr1>physaddr0)totalbytesize-= (u32)physaddr1 - (u32)physaddr0;
CSND_sharedmemtype0_cmd3(channel, physaddr1, totalbytesize);
}
CSND_sharedmemtype0_cmd8(channel, samplerate);
CSND_sharedmemtype0_cmd9(channel, 0xffff);
}

static int lua_startBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	int loop = luaL_checkint(L, 2);
	src->loop = loop;
	src->isPlaying = true;
	src->currentFrame = 0;
	u32 bytesRead;
	if (src->samplerate != 0 && src->audio_size != 0){
	u8* audiobuf = (u8*)linearAlloc(src->audio_size);
	FSFILE_Read(src->sourceFile, &bytesRead, 24, audiobuf, src->audio_size);
	src->audiobuf = audiobuf;
	GSPGPU_FlushDataCache(NULL, audiobuf, src->audio_size);
	if (src->loop == 1){
		My_CSND_playsound(0x8, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)audiobuf, NULL, src->audio_size, 2, 0);
	}else{
		My_CSND_playsound(0x8, CSND_LOOP_DISABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)audiobuf, NULL, src->audio_size, 2, 0);		
	}
	src->tick = osGetTime();
	CSND_setchannel_playbackstate(0x08, 1);
	CSND_sharedmemtype0_cmdupdatestate(0);
	}
	return 0;
}

static int lua_drawBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 4) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);
	BMPV* src = (BMPV*)luaL_checkint(L, 3);
	int screen = luaL_checkint(L, 4);
	if (src->isPlaying){
		if (src->currentFrame >= src->tot_frame){
			if (src->loop == 1){
				src->currentFrame = 0;
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
			}
		}else{
			src->currentFrame =((osGetTime() - src->tick) * src->framerate / 1000);
			Bitmap bitmap;
			bitmap.width = src->width;
			bitmap.height = src->height;
			u32 frame_size = src->width * src->height * 3;
			u8* framebuf = (u8*)(malloc(frame_size));
			u32 bytesRead;
			FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(src->currentFrame*frame_size), framebuf, frame_size);
			bitmap.pixels = framebuf;
			PrintBitmap(x,y,bitmap,screen);
			free(framebuf);
		}
	}else{
		if (src->tick != 0){
			Bitmap bitmap;
			bitmap.width = src->width;
			bitmap.height = src->height;
			u32 frame_size = src->width * src->height * 3;
			u8* framebuf = (u8*)(malloc(frame_size));
			u32 bytesRead;
			FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(src->currentFrame*frame_size), framebuf, frame_size);
			bitmap.pixels = framebuf;
			PrintBitmap(x,y,bitmap,screen);
			free(framebuf);
		}
	}
	return 0;
}

static int lua_getFPS(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	lua_pushnumber(L, src->framerate);
	return 1;
}

static int lua_getCF(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	lua_pushnumber(L, src->currentFrame);
	return 1;
}

static int lua_getSize(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	lua_pushnumber(L, src->tot_frame);
	return 1;
}

static int lua_getSrate(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	lua_pushnumber(L, src->samplerate);
	return 1;
}

static int lua_isPlaying(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_unloadBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	if (src->samplerate != 0 && src->audio_size != 0){
	linearFree(src->audiobuf);
	}
	FSFILE_Close(src->sourceFile);
	svcCloseHandle(src->sourceFile);
	free(src);
	return 0;
}

static int lua_stopBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	src->isPlaying = false;
	src->currentFrame = 0;
	return 0;
}

static int lua_pauseBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	src->isPlaying = false;
	src->tick = (osGetTime() - src->tick);
	if (src->samplerate != 0 && src->audio_size != 0){
	CSND_setchannel_playbackstate(0x08, 0);
	CSND_sharedmemtype0_cmdupdatestate(0);
	}
	return 0;
}

static int lua_resumeBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	src->isPlaying = true;
	src->tick = (osGetTime() - src->tick);
	if (src->samplerate != 0 && src->audio_size != 0){
	CSND_setchannel_playbackstate(0x08, 1);
	CSND_sharedmemtype0_cmdupdatestate(0);
	}
	return 0;
}

//Register our BMPV Functions
static const luaL_Reg BMPV_functions[] = {
  {"load",				lua_loadBMPV},
  {"start",				lua_startBMPV},
  {"draw",				lua_drawBMPV},
  {"unload",			lua_unloadBMPV},
  {"getFPS",			lua_getFPS},
  {"getFrame",			lua_getCF},
  {"getSize",			lua_getSize},
  {"getSrate",			lua_getSrate},
  {"isPlaying",			lua_isPlaying},
  {"stop",				lua_stopBMPV},
  {"resume",			lua_resumeBMPV},
  {"pause",				lua_pauseBMPV},
  {0, 0}
};

void luaVideo_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, BMPV_functions, 0);
	lua_setglobal(L, "BMPV");
	int LOOP = 1;
	int NO_LOOP = 0;
	VariableRegister(L,LOOP);
	VariableRegister(L,NO_LOOP);
}
