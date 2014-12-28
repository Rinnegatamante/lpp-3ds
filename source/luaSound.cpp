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
#include <malloc.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/luaAudio.h"

struct wav{
u32 samplerate;
u16 bytepersample;
u8* audiobuf;
u8* audiobuf2;
u32 size;
u32 mem_size;
Handle sourceFile;
u32 startRead;
u32 moltiplier;
u64 tick;
bool isPlaying;
u32 ch;
u32 ch2;
bool streamLoop;
};

static int lua_openwav(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	u32 mem_size = 0;
	if (argc == 2) mem_size = luaL_checkint(L, 2);
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
	wav_file->mem_size = mem_size;
	wav_file->samplerate = samplerate;
	FSFILE_Read(fileHandle, &bytesRead, 32, &(wav_file->bytepersample), 2);
	if (audiotype == 1){
	if (mem_size > 0){
	wav_file->moltiplier = 1;
	wav_file->isPlaying = false;
	wav_file->mem_size = (size-(pos+4))/mem_size;
	wav_file->sourceFile = fileHandle;
	wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))/mem_size);
	wav_file->startRead = (pos+4);
	FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, wav_file->audiobuf, (size-(pos+4))/mem_size);
	wav_file->audiobuf2 = NULL;
	wav_file->size = size;
	}else{
	wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4));
	FSFILE_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
	wav_file->audiobuf2 = NULL;
	wav_file->size = size-(pos+4);
	}
	}else{
	// I must reordinate my buffer in order to play stereo sound (Thanks CSND/FS libraries .-.)
	u16 bytepersample;
	u32 size_tbp;
	u8* tmp_buffer;
	if (mem_size > 0){
	wav_file->moltiplier = 1;
	wav_file->sourceFile = fileHandle;
	wav_file->startRead = (pos+4);
	wav_file->size = size;
	wav_file->mem_size = (size-(pos+4))/mem_size;
	tmp_buffer = (u8*)linearAlloc((size-(pos+4))/mem_size);
	wav_file->audiobuf = (u8*)linearAlloc(((size-(pos+4))/mem_size)/2);
	wav_file->audiobuf2 = (u8*)linearAlloc(((size-(pos+4))/mem_size)/2);
	FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, tmp_buffer, (size-(pos+4))/mem_size);
	size_tbp = (size-(pos+4))/mem_size;
	}else{
	tmp_buffer = (u8*)linearAlloc((size-(pos+4)));
	wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))/2);
	wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))/2);
	size_tbp = size-(pos+4);
	wav_file->size = (size_tbp)/2;
	FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_buffer, size-(pos+4));
	}
	u32 off=0;
	u32 i=0;
	u16 z;
	while (i < size_tbp){
	z=0;
	while (z < (bytepersample/2)){
	wav_file->audiobuf[off+z] = tmp_buffer[i+z];
	wav_file->audiobuf2[off+z] = tmp_buffer[i+z+(bytepersample/2)];
	z++;
	}
	z=0;
	i=i+bytepersample;
	off=off+(bytepersample/2);
	}
	linearFree(tmp_buffer);
	}
	lua_pushnumber(L,(u32)wav_file);
	}
	if (mem_size == 0){
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	}
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
    if ((argc != 3) && (argc != 4))	return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	int loop = luaL_checkint(L, 2);
	u32 ch = luaL_checkint(L, 3);
	u32 ch2;
	if (argc == 4) ch2 = luaL_checkint(L, 4);
	if (src->audiobuf2 == NULL){
		if (src->mem_size > 0){
		if (loop == 0) src->streamLoop = false;
		else src->streamLoop = true;
		GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
		My_CSND_playsound(ch, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size, 0xFFFF, 0xFFFF);
		}else{
		GSPGPU_FlushDataCache(NULL, src->audiobuf, src->size);
		My_CSND_playsound(ch, loop, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 0xFFFF, 0xFFFF);
		}
		src->ch = ch;
		src->tick = osGetTime();
		CSND_setchannel_playbackstate(ch, 1);
		CSND_sharedmemtype0_cmdupdatestate(0);
	}else{
		if (src->mem_size > 0){
		if (loop == 0) src->streamLoop = false;
		else src->streamLoop = true;
		GSPGPU_FlushDataCache(NULL, src->audiobuf, (src->mem_size)/2);
		GSPGPU_FlushDataCache(NULL, src->audiobuf2, (src->mem_size)/2);
		My_CSND_playsound(ch, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), (src->mem_size)/2, 0xFFFF, 0);
		My_CSND_playsound(ch2, CSND_LOOP_ENABLE, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), (src->mem_size)/2, 0, 0xFFFF);
		}else{
		GSPGPU_FlushDataCache(NULL, src->audiobuf, src->size);
		GSPGPU_FlushDataCache(NULL, src->audiobuf2, src->size);
		My_CSND_playsound(ch, loop, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 0xFFFF, 0);
		My_CSND_playsound(ch2, loop, CSND_ENCODING_PCM16, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->size, 0, 0xFFFF);
		}
		src->ch = ch;
		src->ch2 = ch2;
		src->tick = osGetTime();
		CSND_setchannel_playbackstate(ch, 1);
		CSND_setchannel_playbackstate(ch2, 1);
		CSND_sharedmemtype0_cmdupdatestate(0);
	}
	src->isPlaying = true;
	return 0;
}

static int lua_streamWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	u32 bytesRead;
	if ((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000) >= (src->size - src->startRead)) && (src->isPlaying)){
		if (src->streamLoop){
			src->tick = osGetTime();
			src->moltiplier = 1;
		}else{
			src->isPlaying = false;
			src->moltiplier = 1;
			CSND_setchannel_playbackstate(src->ch, 0);
			if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 0);
			CSND_sharedmemtype0_cmdupdatestate(0);
		}
		if (src->audiobuf2 == NULL){
			FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, src->audiobuf, src->mem_size);
			GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
		}else{
			u8* tmp_buffer = (u8*)linearAlloc(src->mem_size);
			FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, tmp_buffer, src->mem_size);
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
	}else if (((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000)) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying)){
		if ((src->moltiplier % 2) == 1){
			//Update and flush first half-buffer
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, src->audiobuf, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				}
				src->moltiplier = src->moltiplier + 1;
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, tmp_buffer, (src->mem_size)/2);
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
					z=0;
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
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				GSPGPU_FlushDataCache(NULL, src->audiobuf, src->mem_size);
			}else{
				u8* tmp_buffer = (u8*)linearAlloc((src->mem_size)/2);
				FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(src->mem_size/2)*(src->moltiplier + 1), tmp_buffer, (src->mem_size)/2);
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
	return 0;
}

static int lua_closeWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	linearFree(src->audiobuf);
	if (src->audiobuf2 != NULL) linearFree(src->audiobuf2);
	free(src);
	return 0;
}

static int lua_pause(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	CSND_setchannel_playbackstate(src->ch, 0);
	if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 0);
	CSND_sharedmemtype0_cmdupdatestate(0);
	src->isPlaying = false;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_resume(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	CSND_setchannel_playbackstate(src->ch, 1);
	if (src->audiobuf2 != NULL) CSND_setchannel_playbackstate(src->ch2, 1);
	CSND_sharedmemtype0_cmdupdatestate(0);
	src->isPlaying = true;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_wisPlaying(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_soundend(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	CSND_shutdown();
	return 0;
}

static int lua_regsound(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	u32 time = luaL_checkint(L, 1);
	u32 mem_size = (time + 1) * 32000; // time + 1 because first second is mute
	u8* audiobuf = (u8*)linearAlloc(mem_size);
	u32 audiobuf_pos = 0;
	u32* sharedmem = (u32*)memalign(0x1000, (0x8000) * (time + 1));
	MIC_Initialize(sharedmem, (0x8000) * (time + 1), 0x40, 0, 3, 1, 1);
	MIC_SetRecording(1);
	u64 startRegister = osGetTime();
	while ((osGetTime() - startRegister) <= ((time + 1) * 1000)){
	audiobuf_pos+= MIC_ReadAudioData(&audiobuf[audiobuf_pos], mem_size-audiobuf_pos, 1);
	}
	MIC_SetRecording(0);
	MIC_Shutdown();
	free(sharedmem);
	
	//Prevent first mute second to be allocated in wav struct
	u8* nomute_audiobuf =  (u8*)linearAlloc(mem_size - 32000);
	memcpy(nomute_audiobuf,&audiobuf[32000],mem_size - 32000);
	linearFree(audiobuf);
	
	wav* wav_file = (wav*)malloc(sizeof(wav));
	wav_file->audiobuf = nomute_audiobuf;
	wav_file->audiobuf2 = NULL;
	wav_file->mem_size = 0;
	wav_file->size = mem_size - 32000;
	wav_file->samplerate = 16000;
	wav_file->isPlaying = false;
	wav_file->bytepersample = 2;
	lua_pushnumber(L,(u32)wav_file);
	return 1;
}

static int lua_savemono(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkint(L, 1);
	const char* file = luaL_checkstring(L, 2);
	Handle fileHandle;
	u32 bytesWritten;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file);
	FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	u32 four_bytes;
	u16 two_bytes;
	FSFILE_Write(fileHandle, &bytesWritten, 0, "RIFF", 4, FS_WRITE_FLUSH);
	four_bytes = src->size + 36;
	FSFILE_Write(fileHandle, &bytesWritten, 4, &four_bytes, 8, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 8, "WAVEfmt ", 8, FS_WRITE_FLUSH);
	four_bytes = 16;
	FSFILE_Write(fileHandle, &bytesWritten, 16, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = 1;
	FSFILE_Write(fileHandle, &bytesWritten, 20, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 22, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 24, &(src->samplerate), 4, FS_WRITE_FLUSH);
	four_bytes = src->samplerate * src->bytepersample;
	FSFILE_Write(fileHandle, &bytesWritten, 28, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = src->bytepersample*8;
	FSFILE_Write(fileHandle, &bytesWritten, 32, &(src->bytepersample), 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 34, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 36, "data", 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 40, &(src->size), 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 44, src->audiobuf, src->size, FS_WRITE_FLUSH);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	return 0;
}

//Register our Sound Functions
static const luaL_Reg Sound_functions[] = {
  {"openWav",				lua_openwav},
  {"closeWav",				lua_closeWav},
  {"play",					lua_playWav},
  {"init",					lua_soundinit},
  {"term",					lua_soundend},
  {"pause",					lua_pause},
  {"resume",				lua_resume},
  {"isPlaying",				lua_wisPlaying},
  {"updateStream",			lua_streamWav},
  {"register",				lua_regsound},
  {"saveWav",				lua_savemono},
  {0, 0}
};

void luaSound_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Sound_functions, 0);
	lua_setglobal(L, "Sound");
}