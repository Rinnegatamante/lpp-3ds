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
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
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

int MAX_RAM_ALLOCATION = 524288;
int MAX_RAM_ALLOCATION_44100 = 524288*2;

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

struct JPGV{
	Handle sourceFile;
	u32 currentFrame;
	u32 tot_frame;
	u32 framerate;
	u64 tick;
	u32 audio_size;
	u16 bytepersample;
	u16 audiotype;
	u32 samplerate;
	u8* audiobuf;
	u8* audiobuf2;
	Bitmap* framebuf;
	int ch1;
	int ch2;
	bool isPlaying;
	int loop;
	u32 mem_size;
	u32 moltiplier;
};

static int lua_loadJPGV(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
	u32 magic,bytesRead;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x5647504A){
	JPGV* JPGV_file = (JPGV*)malloc(sizeof(JPGV));
	FSFILE_Read(fileHandle, &bytesRead, 4, &(JPGV_file->framerate), 4);
	if (!GW_MODE) FSFILE_Read(fileHandle, &bytesRead, 8,&(JPGV_file->audiotype), 2);
	FSFILE_Read(fileHandle, &bytesRead, 10,&(JPGV_file->bytepersample), 2);
	FSFILE_Read(fileHandle, &bytesRead, 12,&(JPGV_file->samplerate), 4);
	FSFILE_Read(fileHandle, &bytesRead, 16,&(JPGV_file->tot_frame), 4);
	FSFILE_Read(fileHandle, &bytesRead, 20,&(JPGV_file->audio_size), 4);
	JPGV_file->isPlaying = false;
	JPGV_file->currentFrame = 0;
	JPGV_file->sourceFile = fileHandle;
	JPGV_file->tick = 0;
	JPGV_file->audiobuf = NULL;
	JPGV_file->audiobuf2 = NULL;
	JPGV_file->mem_size = JPGV_file->audio_size;
	lua_pushinteger(L, (u32)JPGV_file);
	}
	return 1;
}

static int lua_startJPGV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	int loop = luaL_checkinteger(L, 2);
	int ch1 = luaL_checkinteger(L, 3);
	if (argc == 4){
	int ch2 = luaL_checkinteger(L, 4);
	src->ch2 = ch2;
	}
	src->loop = loop;
	src->isPlaying = true;
	src->ch1 = ch1;
	src->currentFrame = 0;
	u32 bytesRead;
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
		u32 BLOCK_SIZE;
		if (src->samplerate >= 44100) BLOCK_SIZE = MAX_RAM_ALLOCATION_44100;
		else BLOCK_SIZE = MAX_RAM_ALLOCATION;
		while(src->mem_size > BLOCK_SIZE){
			src->mem_size = src->mem_size / 2;
		}
		if (src->audiotype == 1){
			FSFILE_Read(src->sourceFile, &bytesRead, 24, src->audiobuf, src->mem_size);
			GSPGPU_FlushDataCache(NULL, src->audiobuf, src->audio_size);
			My_CSND_playsound(ch1, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)src->audiobuf, src->mem_size, 0xFFFF, 0xFFFF);
		}else{
			u8* audiobuf = (u8*)linearAlloc(src->mem_size);
			FSFILE_Read(src->sourceFile, &bytesRead, 24, audiobuf, src->mem_size);
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
	lua_pushinteger(L, (u32)BMPV_file);
	}
	return 1;
}

static int lua_startBMPV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	int loop = luaL_checkinteger(L, 2);
	int ch1 = luaL_checkinteger(L, 3);
	if (argc == 4){
	int ch2 = luaL_checkinteger(L, 4);
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

static int lua_drawJPGV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	JPGV* src = (JPGV*)luaL_checkinteger(L, 3);
	u32 bytesRead;
	int screen = luaL_checkinteger(L, 4);
	int side = 0;
	if (argc == 5){
	side = luaL_checkinteger(L,5);
	}
	if (src->isPlaying){
		if (src->currentFrame >= (src->tot_frame - 5)){
			if (src->loop == 1){
				src->currentFrame = 0;
				if (!GW_MODE){
					src->moltiplier = 1;
				}
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				if (!GW_MODE){
					src->moltiplier = 1;
					CSND_setchannel_playbackstate(src->ch1, 0);
					if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 0);
					CSND_sharedmemtype0_cmdupdatestate(0);
				}
			}
			if (!GW_MODE){
			if (src->audiobuf2 == NULL){				
					FSFILE_Read(src->sourceFile, &bytesRead, 24, src->audiobuf, src->mem_size);
					GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc(src->mem_size);
				FSFILE_Read(src->sourceFile, &bytesRead, 24, tmp_buffer, src->mem_size);
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
			}
		}else{
			if (((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000)) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying) && (!GW_MODE)){
			if ((src->moltiplier % 2) == 1){
			//Update and flush first half-buffer
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24, src->audiobuf, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				}
				src->moltiplier = src->moltiplier + 1;
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24, tmp_buffer, (src->mem_size)/2);
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
					FSFILE_Read(src->sourceFile, &bytesRead, 24+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
					src->moltiplier = src->moltiplier + 1;
					GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
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
			if (src->currentFrame >= (src->tot_frame-5)) return 0;
			else{
				u32 bytesRead;
				u64 offset;
				u64 size;
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(src->currentFrame*8), &offset, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame+1)*8), &size, 8);
				size = size - offset;
				unsigned char* frame = (unsigned char*)malloc(size);
				FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 8), frame, size);
				src->framebuf = decodeJpg(frame, size);
				free(frame);
				if (screen > 1) PrintImageBitmap(x,y,src->framebuf,screen);
				else RAW2FB(x,y,src->framebuf,screen,side);
			}
		}
	}else{
		if (src->tick != 0){	
			u32 bytesRead;
			u64 offset;
			u64 size;
			if (src->currentFrame >= (src->tot_frame-5)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->tot_frame-5) *8), &offset, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->tot_frame-4) *8), &size, 8);
			}else{
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(src->currentFrame*8), &offset, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame+1)*8), &size, 8);
			}
			size = size - offset;
			unsigned char* frame = (unsigned char*)malloc(size);
			FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 8), frame, size);
			src->framebuf = decodeJpg(frame, size);
			free(frame);
			if (screen > 1) PrintImageBitmap(x,y,src->framebuf,screen);
			else RAW2FB(x,y,src->framebuf,screen,side);
		}
	}
	return 0;
}

static int lua_drawBMPV(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	BMPV* src = (BMPV*)luaL_checkinteger(L, 3);
	u32 bytesRead;
	int screen = luaL_checkinteger(L, 4);
	int side = 0;
	if (argc == 5){
	side = luaL_checkinteger(L,5);
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
				if (!GW_MODE){
					src->moltiplier = 1;
					CSND_setchannel_playbackstate(src->ch1, 0);
					if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 0);
					CSND_sharedmemtype0_cmdupdatestate(0);
				}
			}
			if (!GW_MODE){
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
			}
		}else{
			if (((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000)) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying) && (!GW_MODE)){
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

static int lua_JPGVshowFrame(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 5) && (argc != 6)) return luaL_error(L, "wrong number of arguments");
	u16 x = luaL_checkinteger(L, 1);
	u16 y = luaL_checkinteger(L, 2);
	JPGV* src = (JPGV*)luaL_checkinteger(L, 3);
	u32 frame_index = luaL_checkinteger(L, 4);
	u8 screen = luaL_checkinteger(L, 5);
	int side = 0;
	if (argc == 6){
	side = luaL_checkinteger(L,6);
	}
	u32 bytesRead;
	u64 offset;
	u64 size;
	FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(frame_index*8), &offset, 8);
	FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((frame_index+1)*8), &size, 8);
	size = size - offset;
	unsigned char* frame = (unsigned char*)malloc(size);
	FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 8), frame, size);
	Bitmap* tmp_framebuf = decodeJpg(frame, size);
	free(frame);
	RAW2FB(x,y,tmp_framebuf,screen,side);
	return 0;
}

static int lua_BMPVshowFrame(lua_State *L){
int argc = lua_gettop(L);
    if ((argc != 5) && (argc != 6)) return luaL_error(L, "wrong number of arguments");
	u16 x = luaL_checkinteger(L, 1);
	u16 y = luaL_checkinteger(L, 2);
	BMPV* src = (BMPV*)luaL_checkinteger(L, 3);
	u32 frame_index = luaL_checkinteger(L, 4);
	u8 screen = luaL_checkinteger(L, 5);
	int side = 0;
	if (argc == 6){
	side = luaL_checkinteger(L,6);
	}
	u32 bytesRead;
	Bitmap bitmap;
	bitmap.width = src->width;
	bitmap.height = src->height;
	bitmap.bitperpixel = 24;
	u32 frame_size = src->width * src->height * 3;
	u8* tmp_buffer;
	FSFILE_Read(src->sourceFile, &bytesRead, 28+src->audio_size+(src->currentFrame*frame_size), tmp_buffer, frame_size);
	bitmap.pixels = tmp_buffer;
	PrintScreenBitmap(x,y,&bitmap,screen,side);
	free(bitmap.pixels);
	return 0;
}

static int lua_getFPS(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->framerate);
	return 1;
}

static int lua_getFPS2(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->framerate);
	return 1;
}

static int lua_getCF(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->currentFrame);
	return 1;
}

static int lua_getCF2(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->currentFrame);
	return 1;
}

static int lua_getSize(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->tot_frame);
	return 1;
}

static int lua_getSize2(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->tot_frame);
	return 1;
}

static int lua_getSrate(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_getSrate2(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_isPlaying(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_isPlaying2(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_unloadBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
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

static int lua_unloadJPGV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	if (src->samplerate != 0 && src->audio_size != 0 && !GW_MODE){
	linearFree(src->audiobuf);
	if (src->audiotype == 2){
		linearFree(src->audiobuf2);
	}
	}
	FSFILE_Close(src->sourceFile);
	svcCloseHandle(src->sourceFile);
	free(src);
	return 0;
}

static int lua_stopBMPV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
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

static int lua_stopJPGV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
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
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
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

static int lua_pauseJPGV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
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
	BMPV* src = (BMPV*)luaL_checkinteger(L, 1);
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

static int lua_resumeJPGV(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
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
  {"showFrame",			lua_BMPVshowFrame},
  {"getSize",			lua_getSize},
  {"getSrate",			lua_getSrate},
  {"isPlaying",			lua_isPlaying},
  {"stop",				lua_stopBMPV},
  {"resume",			lua_resumeBMPV},
  {"pause",				lua_pauseBMPV},
  {0, 0}
};

//Register our JPGV Functions
static const luaL_Reg JPGV_functions[] = {
  {"load",				lua_loadJPGV},
  {"start",				lua_startJPGV},
  {"draw",				lua_drawJPGV},
  {"unload",			lua_unloadJPGV},
  {"getFPS",			lua_getFPS2},
  {"getFrame",			lua_getCF2},
  {"showFrame",			lua_JPGVshowFrame},
  {"getSize",			lua_getSize2},
  {"getSrate",			lua_getSrate2},
  {"isPlaying",			lua_isPlaying2},
  {"stop",				lua_stopJPGV},
  {"resume",			lua_resumeJPGV},
  {"pause",				lua_pauseJPGV},
  {0, 0}
};

void luaVideo_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, BMPV_functions, 0);
	lua_setglobal(L, "BMPV");
	lua_newtable(L);
	luaL_setfuncs(L, JPGV_functions, 0);
	lua_setglobal(L, "JPGV");
	int LOOP = 1;
	int NO_LOOP = 0;
	VariableRegister(L,LOOP);
	VariableRegister(L,NO_LOOP);
}
