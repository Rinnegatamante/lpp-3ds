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
#include "include/luaGraphics.h"
#include "include/luaAudio.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

int MAX_RAM_ALLOCATION = 1048576;

struct BMPV{
	Handle sourceFile;
	u32 currentFrame;
	u32 width;
	u32 height;
	u32 framerate;
	u64 tot_frame;
	u64 tick;
	u32 audio_size;
	u16 bytepersample;
	u16 audiotype;
	u32 samplerate;
	u8* audiobuf;
	u8* audiobuf2;
	u8* framebuf;
	int ch1;
	int ch2;
	bool isPlaying;
	int loop;
	u32 mem_size;
	u32 moltiplier;
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
	u32 magic,frame_size,bytesRead;
	u64 size;
	FSFILE_GetSize(fileHandle, &size);
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x56504D42){
	BMPV* BMPV_file = (BMPV*)malloc(sizeof(BMPV));
	FSFILE_Read(fileHandle, &bytesRead, 4, &(BMPV_file->framerate), 4);
	FSFILE_Read(fileHandle, &bytesRead, 8, &(BMPV_file->width), 4);
	FSFILE_Read(fileHandle, &bytesRead, 12,&(BMPV_file->height), 4);
	if (!GW_MODE) FSFILE_Read(fileHandle, &bytesRead, 16,&(BMPV_file->audiotype), 2);
	FSFILE_Read(fileHandle, &bytesRead, 18,&(BMPV_file->bytepersample), 2);
	FSFILE_Read(fileHandle, &bytesRead, 20,&(BMPV_file->samplerate), 4);
	FSFILE_Read(fileHandle, &bytesRead, 24,&(BMPV_file->audio_size), 4);
	BMPV_file->isPlaying = false;
	BMPV_file->currentFrame = 0;
	BMPV_file->sourceFile = fileHandle;
	BMPV_file->tick = 0;
	BMPV_file->audiobuf = NULL;
	BMPV_file->audiobuf2 = NULL;
	frame_size = BMPV_file->width*BMPV_file->height*3;
	u8* framebuf = (u8*)(malloc(frame_size));
	BMPV_file->framebuf = framebuf;
	int tot_frame = (size-28-BMPV_file->audio_size)/frame_size;
	BMPV_file->mem_size = BMPV_file->audio_size;
	BMPV_file->tot_frame = tot_frame;
	lua_pushnumber(L, (u32)BMPV_file);
	}
	return 1;
}

static int lua_startBMPV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	int loop = luaL_checkint(L, 2);
	int ch1 = luaL_checkint(L, 3);
	if (argc == 4){
	int ch2 = luaL_checkint(L, 4);
	src->ch2 = ch2;
	}
	src->loop = loop;
	src->isPlaying = true;
	src->ch1 = ch1;
	src->currentFrame = 0;
	u32 bytesRead;
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
		while(src->mem_size > MAX_RAM_ALLOCATION){
			src->mem_size = src->mem_size / 2;
		}
		if (src->audiotype == 1){
			FSFILE_Read(src->sourceFile, &bytesRead, 28, src->audiobuf, src->mem_size);
			GSPGPU_FlushDataCache(NULL, src->audiobuf, src->audio_size);
			My_CSND_playsound(ch1, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)src->audiobuf, src->mem_size, 0xFFFF, 0xFFFF);
		}else{
			u8* audiobuf = (u8*)linearAlloc(src->mem_size);
			FSFILE_Read(src->sourceFile, &bytesRead, 28, audiobuf, src->mem_size);
			src->audiobuf = (u8*)linearAlloc(src->mem_size/2);
			src->audiobuf2 = (u8*)linearAlloc(src->mem_size/2);
			u32 off=0;
			u32 i=0;
			u16 z;
			while (i < (src->mem_size)){
				z=0;
				while (z < (src->bytepersample/2)){
					src->audiobuf[off+z] = audiobuf[i+z];
					src->audiobuf2[off+z] = audiobuf[i+z+(src->bytepersample/2)];
					z++;
				}
				i=i+src->bytepersample;
				off=off+(src->bytepersample/2);
			}
			linearFree(audiobuf);
			GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size/2);
			GSPGPU_FlushDataCache(NULL, src->audiobuf2, src->mem_size/2);
			My_CSND_playsound(src->ch1, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)src->audiobuf, src->mem_size/2, 0xFFFF, 0);
			My_CSND_playsound(src->ch2, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf2, (u32*)src->audiobuf2, src->mem_size/2, 0, 0xFFFF);
		}
	src->tick = osGetTime();
	CSND_setchannel_playbackstate(ch1, 1);
	if (src->audiotype == 2){
		CSND_setchannel_playbackstate(src->ch2, 1);
	}
	CSND_sharedmemtype0_cmdupdatestate(0);
	src->moltiplier = 1;
	}else{
	src->tick = osGetTime();
	}
	return 0;
}

static int lua_drawBMPV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkint(L, 1);
	int y = luaL_checkint(L, 2);
	BMPV* src = (BMPV*)luaL_checkint(L, 3);
	u32 bytesRead;
	int screen = luaL_checkint(L, 4);
	int side = 0;
	if (argc == 5){
	side = luaL_checkint(L,5);
	}
	if (src->isPlaying){
		if (src->currentFrame >= src->tot_frame){
			if (src->loop == 1){
				src->currentFrame = 0;
				if (!GW_MODE){
					src->moltiplier = 1;
				}
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				src->moltiplier = 1;
				CSND_setchannel_playbackstate(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 0);
				CSND_sharedmemtype0_cmdupdatestate(0);
			}
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, 28, src->audiobuf, src->mem_size);
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc(src->mem_size);
				FSFILE_Read(src->sourceFile, &bytesRead, 28, tmp_buffer, src->mem_size);
				u32 size_tbp = src->mem_size;
				u32 off=0;
				u32 i=0;
				u16 z;
				while (i < size_tbp){
					z=0;
					while (z < (src->bytepersample/2)){
						src->audiobuf[off+z] = tmp_buffer[i+z];
						src->audiobuf2[off+z] = tmp_buffer[i+z+(src->bytepersample/2)];
						z++;
					}
					z=0;
					i=i+src->bytepersample;
					off=off+(src->bytepersample/2);
				}
			linearFree(tmp_buffer);
			GSPGPU_FlushDataCache(NULL, src->audiobuf, (src->mem_size)/2);
			GSPGPU_FlushDataCache(NULL, src->audiobuf2, (src->mem_size)/2);
			}
		}else{
			if (((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000)) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying)){
			if ((src->moltiplier % 2) == 1){
			//Update and flush first half-buffer
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, 28+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 28, src->audiobuf, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				}
				src->moltiplier = src->moltiplier + 1;
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, 28+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 28, tmp_buffer, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				}
				src->moltiplier = src->moltiplier + 1;
				u32 size_tbp = (src->mem_size)/2;
				u32 off=0;
				u32 i=0;
				u16 z;
				while (i < size_tbp){
					z=0;
					while (z < (src->bytepersample/2)){
						src->audiobuf[off+z] = tmp_buffer[i+z];
						src->audiobuf2[off+z] = tmp_buffer[i+z+(src->bytepersample/2)];
						z++;
					}
					i=i+src->bytepersample;
					off=off+(src->bytepersample/2);
				}
				linearFree(tmp_buffer);
				GSPGPU_FlushDataCache(NULL, src->audiobuf, (src->mem_size)/2);
				GSPGPU_FlushDataCache(NULL, src->audiobuf2, (src->mem_size)/2);
			}
		}else{
			u32 bytesRead;
			//Update and flush second half-buffer
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, 28+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, 28+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				u32 size_tbp = (src->mem_size)/2;
				u32 off=0;
				u32 i=0;
				u16 z;
				while (i < size_tbp){
					z=0;
					while (z < (src->bytepersample/2)){
						src->audiobuf[(src->mem_size)/4+off+z] = tmp_buffer[i+z];
						src->audiobuf2[(src->mem_size)/4+off+z] = tmp_buffer[i+z+(src->bytepersample/2)];
						z++;
					}
				i=i+src->bytepersample;
				off=off+(src->bytepersample/2);
				}
				linearFree(tmp_buffer);
				GSPGPU_FlushDataCache(NULL, src->audiobuf, (src->mem_size)/2);
				GSPGPU_FlushDataCache(NULL, src->audiobuf2, (src->mem_size)/2);
			}
		}
		}
			src->currentFrame =((osGetTime() - src->tick) * src->framerate / 1000);
			Bitmap bitmap;
			bitmap.width = src->width;
			bitmap.height = src->height;
			bitmap.bitperpixel = 24;
			u32 frame_size = src->width * src->height * 3;
			u32 bytesRead;
			FSFILE_Read(src->sourceFile, &bytesRead, 28+src->audio_size+(src->currentFrame*frame_size), src->framebuf, frame_size);
			bitmap.pixels = src->framebuf;
			if (screen > 1) PrintImageBitmap(x,y,&bitmap,screen);
			else PrintScreenBitmap(x,y,&bitmap,screen,side);
		}
	}else{
		if (src->tick != 0){
			Bitmap bitmap;
			bitmap.width = src->width;
			bitmap.height = src->height;
			bitmap.bitperpixel = 24;
			u32 frame_size = src->width * src->height * 3;
			u32 bytesRead;
			FSFILE_Read(src->sourceFile, &bytesRead, 28+src->audio_size+(src->currentFrame*frame_size), src->framebuf, frame_size);
			bitmap.pixels = src->framebuf;
			if (screen > 1) PrintImageBitmap(x,y,&bitmap,screen);
			else PrintScreenBitmap(x,y,&bitmap,screen,side);
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
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
	linearFree(src->audiobuf);
	if (src->audiotype == 2){
		linearFree(src->audiobuf2);
	}
	}
	FSFILE_Close(src->sourceFile);
	svcCloseHandle(src->sourceFile);
	free(src->framebuf);
	free(src);
	return 0;
}

static int lua_stopBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	src->isPlaying = false;
	src->currentFrame = 0;
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
	CSND_setchannel_playbackstate(src->ch1, 0);
	if (src->audiotype == 2){
	CSND_setchannel_playbackstate(src->ch2, 0);
	}
	CSND_sharedmemtype0_cmdupdatestate(0);
	}
	return 0;
}

static int lua_pauseBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkint(L, 1);
	src->isPlaying = false;
	src->tick = (osGetTime() - src->tick);
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
	CSND_setchannel_playbackstate(src->ch1, 0);
	if (src->audiotype == 2){
	CSND_setchannel_playbackstate(src->ch2, 0);
	}
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
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
	CSND_setchannel_playbackstate(src->ch1, 1);
	if (src->audiotype == 2){
	CSND_setchannel_playbackstate(src->ch2, 1);
	}
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
