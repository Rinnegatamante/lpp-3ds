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
#- xerpi for sf2dlib ---------------------------------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/audio.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

u32* micbuf = NULL;
u32 micsize;
u32 cur_offset;
MICU_SampleRate srate;
extern bool csndAccess;

u32 sampleSize(MICU_SampleRate smplrt){
	if (smplrt == MICU_SAMPLE_RATE_8180) return 8180;
	else if (smplrt == MICU_SAMPLE_RATE_10910) return 10910;
	else if (smplrt == MICU_SAMPLE_RATE_16360) return 16360;
	else return 32730;
}

static int lua_regsound(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 time = luaL_checkinteger(L, 1);
	u32 samplerate = luaL_checkinteger(L, 2);
	u32 mem_base;
	if (samplerate <= 8200){
		srate = MICU_SAMPLE_RATE_8180;
		mem_base = 0x4000;
	}else if (samplerate <= 12000){
		srate = MICU_SAMPLE_RATE_10910;
		mem_base = 0x6000;
	}else if (samplerate <= 18000){
		srate = MICU_SAMPLE_RATE_16360;
		mem_base = 0x8000;
	}else{
		srate = MICU_SAMPLE_RATE_32730; 
		mem_base = 0x10000;
	}
	u32 micbuf_pos = 0;
	u32 mem_size = mem_base * time;
	micbuf = (u32*)memalign(0x1000, mem_size);
	micInit((u8*)micbuf, mem_size);
	micsize = micGetSampleDataSize();
	bool power;
	cur_offset = 0;
	svcSleepThread(1000000000);
	MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, srate, 0, micsize, false);
	return 0;
}

static int lua_micplaying(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	bool isSampling;
	MICU_IsSampling(&isSampling);
	lua_pushboolean(L, isSampling);
	return 1;
}

static int lua_stoprec_CSND(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	bool isSampling;
	MICU_IsSampling(&isSampling);
	u32 offset;
	if (isSampling){
		offset = micGetLastSampleOffset();
		MICU_StopSampling();
	}else offset = micsize;
	u32 samplerate = sampleSize(srate);
	u32 mute_size = samplerate<<1;
	u8* bytebuf = (u8*)linearAlloc(offset);
	memcpy(bytebuf, (u8*)micbuf, offset);
	micExit();
	free(micbuf);
	wav* songFile = (wav*)malloc(sizeof(wav));
	songFile->audiobuf = bytebuf;
	songFile->audiobuf2 = NULL;
	songFile->big_endian = false;
	songFile->mem_size = 0;
	songFile->ch = 0xDEADBEEF;
	songFile->size = offset;
	songFile->samplerate = samplerate;
	songFile->encoding = CSND_ENCODING_PCM16;	
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->isPlaying = false;
	songFile->bytepersample = 2;
	songFile->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)songFile);
	return 1;
}

static int lua_stoprec_DSP(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	bool isSampling;
	MICU_IsSampling(&isSampling);
	u32 offset;
	if (isSampling){
		offset = micGetLastSampleOffset();
		MICU_StopSampling();
	}else offset = micsize;
	u32 samplerate = sampleSize(srate);
	u32 mute_size = samplerate<<1;
	u8* bytebuf = (u8*)linearAlloc(offset);
	memcpy(bytebuf, (u8*)micbuf, offset);
	micExit();
	free(micbuf);
	Music* songFile = (Music*)malloc(sizeof(Music));
	songFile->audiobuf = bytebuf;
	songFile->audiobuf2 = NULL;
	songFile->big_endian = false;
	songFile->mem_size = 0;
	songFile->ch = 0xDEADBEEF;
	songFile->size = offset;
	songFile->samplerate = samplerate;
	songFile->encoding = CSND_ENCODING_PCM16;	
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->isPlaying = false;
	songFile->bytepersample = 2;
	songFile->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)songFile);
	return 1;
}

static int lua_pausemic(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	bool isSampling;
	MICU_IsSampling(&isSampling);
	if (isSampling){
		cur_offset = micGetLastSampleOffset();
		MICU_StopSampling();
	}
	return 0;
}

static int lua_resumemic(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	bool isSampling;
	MICU_IsSampling(&isSampling);
	u32 offset;
	if (!isSampling) MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED, srate, cur_offset, micsize - cur_offset, false);
	return 0;
}

//Register our Mic Functions
static const luaL_Reg Mic_DSP_functions[] = {
	{"start",        lua_regsound},
	{"isRecording",  lua_micplaying},	
	{"stop",         lua_stoprec_DSP},	
	{"pause",        lua_pausemic},	
	{"resume",       lua_resumemic},	
	{0, 0}
};

static const luaL_Reg Mic_CSND_functions[] = {
	{"start",        lua_regsound},
	{"isRecording",  lua_micplaying},
	{"stop",         lua_stoprec_CSND},	
	{"pause",        lua_pausemic},	
	{"resume",       lua_resumemic},	
	{0, 0}
};

void luaMic_init(lua_State *L) {
	lua_newtable(L);
	if (csndAccess) luaL_setfuncs(L, Mic_CSND_functions, 0);
	else luaL_setfuncs(L, Mic_DSP_functions, 0);
	lua_setglobal(L, "Mic");
}