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
#- xerpi for sf2dlib ---------------------------------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <math.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/graphics/Graphics.h"
#include "include/luaAudio.h"
#include "include/ogg/ogg.h"
#include "include/ogg/codec.h"
#include "include/ogg/vorbisfile.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

#define MAX_RAM_ALLOCATION 524288
#define MAX_RAM_ALLOCATION_44100 1048576

u8* tmp_buf = NULL;
extern bool audioChannels[32];
extern bool csndAccess;

struct JPGV{
	u32 magic;
	Handle sourceFile;
	u32 currentFrame;
	u32 tot_frame;
	u16 framerate;
	u16 is3D;
	u64 tick;
	u32 audio_size;
	u16 bytepersample;
	u16 audiotype;
	u16 samplerate;
	u8* audiobuf;
	u8* audiobuf2;
	Bitmap* framebuf;
	int ch1;
	int ch2;
	bool isPlaying;
	int loop;
	u32 mem_size;
	u32 moltiplier;
	u32* thread;
	u8 audiocodec;
	u32 stdio_handle;
	u32 real_audio_size;
	u32 audio_pointer;
	u32 package_size;
	u32 total_packages_size;
};

size_t jpgv_rc(void *ptr, size_t size, size_t nmemb, void *datasource){
	return fread((FILE*)ptr,size,nmemb,(FILE*)datasource);
}

int jpgv_sk(void *datasource, ogg_int64_t offset, int whence){
	if (whence == SEEK_END){
		fseek((FILE*)datasource,0x14,SEEK_SET);
		u32 audiosize;
		fread(&audiosize,4,1,(FILE*)datasource);
		return fseek((FILE*)datasource,0x18+audiosize+offset,SEEK_SET);
	}
	else return fseek((FILE*)datasource,offset,whence);
}

long jpgv_tl(void *datasource){
	return ftell((FILE*)datasource);
}

static volatile bool closeStream = false;
static Handle updateStream;
static Handle streamThread;

static char pcmout[4096];

static void streamWAV(void* arg){
	JPGV* src = (JPGV*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		u32 bytesRead;
		u32 control;
		if(closeStream){
			closeStream = false;
			svcExitThread();
		}
		if (((src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000)) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying)){
			if ((src->moltiplier % 2) == 1){
			//Update and flush first half-buffer
			if (src->audiobuf2 == NULL){
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24, src->audiobuf, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				}
				src->moltiplier = src->moltiplier + 1;
			}else{
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(src->mem_size/2)*(src->moltiplier + 1), tmp_buf, (src->mem_size)/2);
				if (bytesRead != ((src->mem_size)/2)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24, tmp_buf, (src->mem_size)/2);
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
						src->audiobuf[off+z] = tmp_buf[i+z];
						src->audiobuf2[off+z] = tmp_buf[i+z+(src->bytepersample/2)];
						z++;
					}
					i=i+src->bytepersample;
					off=off+(src->bytepersample/2);
				}
			}
		}else{
			u32 bytesRead;
			//Update and flush second half-buffer
			if (src->audiobuf2 == NULL){
					FSFILE_Read(src->sourceFile, &bytesRead, 24+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
					src->moltiplier = src->moltiplier + 1;
			}else{
				FSFILE_Read(src->sourceFile, &bytesRead, 24+(src->mem_size/2)*(src->moltiplier + 1), tmp_buf, (src->mem_size)/2);
				src->moltiplier = src->moltiplier + 1;
				u32 size_tbp = (src->mem_size)/2;
				u32 off=0;
				u32 i=0;
				u16 z;
				while (i < size_tbp){
					z=0;
					while (z < (src->bytepersample/2)){
						src->audiobuf[(src->mem_size)/4+off+z] = tmp_buf[i+z];
						src->audiobuf2[(src->mem_size)/4+off+z] = tmp_buf[i+z+(src->bytepersample/2)];
						z++;
					}
				i=i+src->bytepersample;
				off=off+(src->bytepersample/2);
				}
			}
		}
		}

	}		
}

static void streamOGG(void* arg){
	JPGV* src = (JPGV*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		u32 bytesRead;
		
			if(closeStream){
				closeStream = false;
				svcExitThread();
			}
			
			// Initializing libogg and vorbisfile
			int eof=0;
			static int current_section;
	
			u32 control;
			u32 total = src->real_audio_size;
			u32 block_size;
			u32 package_max_size;
			if (src->package_size == 0){
				block_size = src->mem_size / 8;
				package_max_size = block_size;
			}else{ 
				block_size = src->total_packages_size / (src->moltiplier - 1);
				package_max_size = src->mem_size / 8;
			}
			if (src->audiobuf2 == NULL) control = src->samplerate * 2 * ((osGetTime() - src->tick) / 1000);
			else{
				control = src->samplerate * 4 * ((osGetTime() - src->tick) / 1000);
				total = total * 2;
			}
			if ((control >= total) && (src->isPlaying)){
				CSND_SetPlayState(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
				CSND_UpdateInfo(0);
				src->moltiplier = 1;
				ov_raw_seek((OggVorbis_File*)src->stdio_handle,0);
				if (src->audiobuf2 == NULL){
					
					int i = 0;
					while(!eof){
						long ret=ov_read((OggVorbis_File*)src->stdio_handle,pcmout,sizeof(pcmout),0,2,1,&current_section);
						if (ret == 0) {
							eof=1;
						} else {
							memcpy(&src->audiobuf[i],pcmout,ret);
							i = i + ret;
							if (i >= (package_max_size)) break;
						}
					}
				}else{
					char pcmout[2048];
					int i = 0;
					while(!eof){
						long ret=ov_read((OggVorbis_File*)src->stdio_handle,pcmout,sizeof(pcmout),0,2,1,&current_section);
						if (ret == 0) {
							eof=1;
						} else {
							memcpy(&tmp_buf[i],pcmout,ret);
							i = i + ret;
							if (i >= src->mem_size) break;	
						}
					}
		
					// Separating left and right channels
					int z;
					int j=0;
					for (z=0; z < src->mem_size; z=z+4){
						src->audiobuf[j] = tmp_buf[z];
						src->audiobuf[j+1] = tmp_buf[z+1];
						src->audiobuf2[j] = tmp_buf[z+2];
						src->audiobuf2[j+1] = tmp_buf[z+3];
						j=j+2;
					}
				}
				if (!src->loop){
					src->isPlaying = false;
					src->tick = (osGetTime()-src->tick);
				}else{
					src->tick = osGetTime();
					CSND_SetPlayState(src->ch1, 1);
					if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 1);
					CSND_UpdateInfo(0);
				}
			}else if ((control > (block_size * src->moltiplier)) && (src->isPlaying)){
				if (src->audiobuf2 == NULL){ //Mono file
						int i = 0;
						int j = src->audio_pointer;
						while(!eof){
							long ret=ov_read((OggVorbis_File*)src->stdio_handle,pcmout,sizeof(pcmout),0,2,1,&current_section);
							if (ret == 0) {
								eof=1;
							} else {
								memcpy(&tmp_buf[i],pcmout,ret);
								i = i + ret;
								src->package_size = i;
								if (i >= (package_max_size)) break;
							}
						}
						if (j + src->package_size >= src->mem_size){
							u32 frag_size = src->mem_size - j;
							u32 frag2_size = src->package_size-frag_size;
							memcpy(&src->audiobuf[j],tmp_buf,frag_size);
							memcpy(src->audiobuf,&tmp_buf[frag_size],frag2_size);
							src->audio_pointer = frag2_size;
						}else{
							memcpy(&src->audiobuf[j],tmp_buf,src->package_size);
							src->audio_pointer = j + src->package_size;
						}
						src->total_packages_size = src->total_packages_size + src->package_size;
					}else{ //Stereo file
						char pcmout[2048];
						int i = 0;
						while(!eof){
							long ret=ov_read((OggVorbis_File*)src->stdio_handle,pcmout,sizeof(pcmout),0,2,1,&current_section);
							if (ret == 0) {
								eof=1;
							} else {
								memcpy(&tmp_buf[i],pcmout,ret);
								i = i + ret;
								src->package_size = i;
								if (i >= (package_max_size)) break;
							}
						}
				
						// Separating left and right channels
						int z;
						int j = src->audio_pointer;
						for (z=0; z < src->package_size; z=z+4){
							src->audiobuf[j] = tmp_buf[z];
							src->audiobuf[j+1] = tmp_buf[z+1];
							src->audiobuf2[j] = tmp_buf[z+2];
							src->audiobuf2[j+1] = tmp_buf[z+3];
							j=j+2;
							if (j >= src->mem_size / 2) j = 0;
						}
						src->audio_pointer = j;
						src->total_packages_size = src->total_packages_size + src->package_size;
					}
					src->moltiplier = src->moltiplier + 1;
			}
	}
}

static int lua_loadJPGV(lua_State *L)
{
    int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
       if (argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	Handle fileHandle;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
	Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
	#ifndef SKIP_ERROR_HANDLING
		if(ret) return luaL_error(L, "error opening file");
	#endif
	u32 magic,bytesRead;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x5647504A){
	JPGV* JPGV_file = (JPGV*)malloc(sizeof(JPGV));
	FSFILE_Read(fileHandle, &bytesRead, 4, &(JPGV_file->framerate), 2);
	FSFILE_Read(fileHandle, &bytesRead, 6, &(JPGV_file->is3D), 2);
	FSFILE_Read(fileHandle, &bytesRead, 8,&(JPGV_file->audiotype), 2);
	FSFILE_Read(fileHandle, &bytesRead, 10,&(JPGV_file->bytepersample), 2);
	FSFILE_Read(fileHandle, &bytesRead, 12,&(JPGV_file->samplerate), 2);
	FSFILE_Read(fileHandle, &bytesRead, 14,&(JPGV_file->audiocodec), 2);
	FSFILE_Read(fileHandle, &bytesRead, 16,&(JPGV_file->tot_frame), 4);
	FSFILE_Read(fileHandle, &bytesRead, 20,&(JPGV_file->audio_size), 4);
	JPGV_file->isPlaying = false;
	JPGV_file->currentFrame = 0;
	JPGV_file->package_size = 0;
	JPGV_file->total_packages_size = 0;
	if (JPGV_file->audiocodec != 0){ // Vorbis audiocodec
		char myFile[512];
		strcpy(myFile,"sdmc:");
		strcat(myFile,file_tbo);
		sdmcInit();
		JPGV_file->stdio_handle = (u32)fopen(myFile,"rb");
	}
	JPGV_file->sourceFile = fileHandle;
	JPGV_file->tick = 0;
	JPGV_file->audiobuf = NULL;
	JPGV_file->audiobuf2 = NULL;
	JPGV_file->ch1 = 0xDEADBEEF;
	JPGV_file->ch2 = 0xDEADBEEF;
	JPGV_file->thread = NULL;
	JPGV_file->mem_size = JPGV_file->audio_size;
	JPGV_file->magic = 0x4C4A5056;
	lua_pushinteger(L, (u32)JPGV_file);
	}
	return 1;
}

static int lua_startJPGV(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	int loop = luaL_checkinteger(L, 2);
	
	// Selecting free channels
	int ch = src->ch1;
	int ch2 = src->ch2;
	if (src->ch1 == 0xDEADBEEF){
		ch = 0x08;
		while (audioChannels[ch]){
			ch++;
			if (ch > 32) return luaL_error(L, "audio device is busy");
		}
		audioChannels[ch] = true;
		ch2 = ch + 1;
		if (src->audiotype == 2){
			while (audioChannels[ch2]){
				ch2++;
				if (ch2 > 32) return luaL_error(L, "audio device is busy");
			}
			audioChannels[ch2] = true;
		}
	}
	src->ch1 = ch;
	src->ch2 = ch2;
	
	src->loop = loop;
	src->isPlaying = true;
	ThreadFunc streamFunction = streamWAV;
	if (src->audiocodec != 0){ // Vorbis audiocodec
		src->audio_pointer = 0;
		streamFunction = streamOGG;
		int eof=0;
		OggVorbis_File* vf = (OggVorbis_File*)malloc(sizeof(OggVorbis_File));
		static int current_section;
		ov_callbacks jpgv_callbacks;
		jpgv_callbacks.read_func = jpgv_rc;
		jpgv_callbacks.seek_func = jpgv_sk;
		jpgv_callbacks.tell_func = jpgv_tl;
		jpgv_callbacks.close_func = NULL;
		if (ov_open_callbacks((FILE*)src->stdio_handle, vf, NULL, 0, jpgv_callbacks) != 0)
		{
			fclose((FILE*)src->stdio_handle);
			return luaL_error(L, "corrupt OGG audiobuffer.");
		}
		vorbis_info* my_info = ov_info(vf,-1);
		src->samplerate = my_info->rate;
		src->real_audio_size = ov_time_total(vf,-1) * 2 * my_info->rate;
		src->bytepersample = 2;
		// Decoding OGG buffer
	int i=0;
	if (my_info->channels == 1){ //Mono buffer
			src->audiotype = 1;
			src->moltiplier = 1;
			src->mem_size = src->audio_size / 2;
			while (src->mem_size > MAX_RAM_ALLOCATION){
				src->mem_size = src->mem_size / 2;
			}
			src->audiobuf = (u8*)linearAlloc(src->mem_size);
		src->audiobuf2 = NULL;
		while(!eof){
			long ret=ov_read(vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
			if (ret == 0) {
			
				// EOF
				eof=1;
				
			} else if (ret < 0) {
			
				// Error handling
				#ifndef SKIP_ERROR_HANDLING
					if(ret==OV_EBADLINK){
						return luaL_error(L, "corrupt bitstream section.");
					}
				#endif
				
			} else {
			
				// Copying decoded block to PCM16 audiobuffer
				memcpy(&src->audiobuf[i],pcmout,ret);
				i = i + ret;
				if (i >= src->mem_size) break;
			}
		}
	}else{ //Stereo buffer
			src->audiotype = 2;
			tmp_buf;
			u32 size_tbp;
			src->moltiplier = 1;
			src->bytepersample = 2;
			src->mem_size = src->audio_size / 2;
			u8 molt = 1;
			if (src->samplerate <= 30000) molt = 4; // Temporary patch for low samplerates
			while (src->mem_size > MAX_RAM_ALLOCATION * molt){
				src->mem_size = src->mem_size / 2;
			}
			size_tbp = src->mem_size;
			tmp_buf = (u8*)linearAlloc(src->mem_size);
			src->audiobuf = (u8*)linearAlloc(src->mem_size / 2);
			src->audiobuf2 = (u8*)linearAlloc(src->mem_size / 2);
		
		while(!eof){
			long ret=ov_read(vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
			if (ret == 0) {
			
				// EOF
				eof=1;
				
			} else if (ret < 0) {
			
				// Error handling
				#ifndef SKIP_ERROR_HANDLING
					if(ret==OV_EBADLINK){
						return luaL_error(L, "corrupt bitstream section.");
					}
				#endif
				
			} else {
			
				// Copying decoded block to PCM16 audiobuffer
				memcpy(&tmp_buf[i],pcmout,ret);
				i = i + ret;
				if (i >= src->mem_size) break;	
			}
		}
		
		
		// Separating left and right channels
		int z;
		int j=0;
		for (z=0; z < size_tbp; z=z+4){
			src->audiobuf[j] = tmp_buf[z];
			src->audiobuf[j+1] = tmp_buf[z+1];
			src->audiobuf2[j] = tmp_buf[z+2];
			src->audiobuf2[j+1] = tmp_buf[z+3];
			j=j+2;
		}
		
	}
	src->stdio_handle = (u32)vf;
	}
	src->currentFrame = 0;
	u32 bytesRead;
	if (src->samplerate != 0 && src->audio_size != 0){
		u32 BLOCK_SIZE;
		if (src->samplerate >= 44100) BLOCK_SIZE = MAX_RAM_ALLOCATION_44100;
		else BLOCK_SIZE = MAX_RAM_ALLOCATION;
		if (src->audiocodec == 0){
			while(src->mem_size > BLOCK_SIZE){
				src->mem_size = src->mem_size / 2;
			}
		}
		if (src->audiotype == 1){			
			if (src->audiocodec == 0) FSFILE_Read(src->sourceFile, &bytesRead, 24, src->audiobuf, src->mem_size);
			My_CSND_playsound(src->ch1, SOUND_LINEAR_INTERP | SOUND_FORMAT_16BIT | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)src->audiobuf, src->mem_size, 1.0, 2.0);
		}else{
			if (src->audiocodec == 0){
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
			}
			My_CSND_playsound(src->ch1, SOUND_LINEAR_INTERP | SOUND_FORMAT_16BIT | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)src->audiobuf, src->mem_size/2, 1.0, 1.0);
			My_CSND_playsound(src->ch2, SOUND_LINEAR_INTERP | SOUND_FORMAT_16BIT | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf2, (u32*)src->audiobuf2, src->mem_size/2, 1.0, -1.0);
		}
	CSND_SetPlayState(src->ch1, 1);
	if (src->audiotype == 2) CSND_SetPlayState(src->ch2, 1);
	CSND_UpdateInfo(0);
	src->tick = osGetTime();
	src->moltiplier = 1;
	svcCreateEvent(&updateStream,0);
	u32 *threadStack = (u32*)memalign(32, 8192);
	src->thread = threadStack;
	svcSignalEvent(updateStream);
	Result ret = svcCreateThread(&streamThread, streamFunction, (u32)src, &threadStack[2048], 0x18, 1);			
	}else src->tick = osGetTime();
	return 0;
}

void draw3DJPGV(int x,int y,JPGV* src,int screen,bool use3D){
	if (src->isPlaying){
		if (src->currentFrame >= (src->tot_frame - 10)){
			if (src->loop == 1){
				src->currentFrame = 0;
				src->moltiplier = 1;
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				src->moltiplier = 1;
				CSND_SetPlayState(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
				CSND_UpdateInfo(0);
			}
		}else{
			double tmp = (double)((double)(osGetTime() - src->tick) / 1000.0) * src->framerate;
			src->currentFrame = (u32)floor(tmp);
			if (src->currentFrame >= (src->tot_frame-10)) return;
			else{
				u32 bytesRead;
				u64 offset_left;
				u64 size_left;
				u64 size_right;
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2)*8), &offset_left, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(((src->currentFrame*2)+1)*8), &size_left, 8);
				u64 offset_right = size_left;
				size_left = size_left - offset_left;
				unsigned char* frame_left = (unsigned char*)malloc(size_left);
				FSFILE_Read(src->sourceFile, &bytesRead, offset_left + (src->tot_frame * 16), frame_left, size_left);
				src->framebuf = decodeJpg(frame_left, size_left);
				free(frame_left);
				if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,0);
				free(src->framebuf->pixels);
				free(src->framebuf);
				if (use3D){
					FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(((src->currentFrame*2)+2)*8), &size_right, 8);
					size_right = size_right - offset_right;
					unsigned char* frame_right = (unsigned char*)malloc(size_right);
					FSFILE_Read(src->sourceFile, &bytesRead, offset_right + (src->tot_frame * 16), frame_right, size_right);
					src->framebuf = decodeJpg(frame_right, size_right);
					free(frame_right);
					if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,1);
					free(src->framebuf->pixels);
					free(src->framebuf);
				}
			}
		}
	}else{
		if (src->tick != 0){	
			u32 bytesRead;
			u64 offset;
			u64 size;
			u64 size_right;
			u64 offset_right;
			if (src->currentFrame < (src->tot_frame-10)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2)*8), &offset, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2+1)*8), &size, 8);
				if (use3D){
					FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2+2)*8), &size_right, 8);
					size_right = size_right - size;
					offset_right = size;
				}
			}
			size = size - offset;
			unsigned char* frame = (unsigned char*)malloc(size);
			FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 16), frame, size);
			src->framebuf = decodeJpg(frame, size);
			free(frame);
			if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,0);
			free(src->framebuf->pixels);
			free(src->framebuf);
			if (use3D){
				unsigned char* frame2 = (unsigned char*)malloc(size_right);
				FSFILE_Read(src->sourceFile, &bytesRead, offset_right + (src->tot_frame * 16), frame2, size_right);
				src->framebuf = decodeJpg(frame2, size_right);
				free(frame2);
				if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,1);
				free(src->framebuf->pixels);
				free(src->framebuf);
			}
		}
	}
}

void draw3DJPGVfast(JPGV* src, u8* framebuf, bool use3D){
	if (framebuf != TopLFB) use3D = false;
	if (src->isPlaying){
		if (src->currentFrame >= (src->tot_frame - 10)){
			if (src->loop == 1){
				src->currentFrame = 0;
				src->moltiplier = 1;
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				src->moltiplier = 1;
				CSND_SetPlayState(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
				CSND_UpdateInfo(0);
			}
		}else{
			double tmp = (double)((double)(osGetTime() - src->tick) / 1000.0) * src->framerate;
			src->currentFrame = (u32)floor(tmp);
			if (src->currentFrame >= (src->tot_frame-10)) return;
			else{
				u32 bytesRead;
				u64 offset_left;
				u64 size_left;
				u64 size_right;
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2)*8), &offset_left, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(((src->currentFrame*2)+1)*8), &size_left, 8);
				u64 offset_right = size_left;
				size_left = size_left - offset_left;
				unsigned char* frame_left = (unsigned char*)malloc(size_left);
				FSFILE_Read(src->sourceFile, &bytesRead, offset_left + (src->tot_frame * 16), frame_left, size_left);
				printJpg(frame_left, size_left, framebuf);
				free(frame_left);
				if (use3D){
					FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+(((src->currentFrame*2)+2)*8), &size_right, 8);
					size_right = size_right - offset_right;
					unsigned char* frame_right = (unsigned char*)malloc(size_right);
					FSFILE_Read(src->sourceFile, &bytesRead, offset_right + (src->tot_frame * 16), frame_right, size_right);
					printJpg(frame_right, size_right, TopRFB);
					free(frame_right);
				}
			}
		}
	}else{
		if (src->tick != 0){	
			u32 bytesRead;
			u64 offset;
			u64 size;
			u64 size_right;
			u64 offset_right;
			if (src->currentFrame < (src->tot_frame-10)){
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2)*8), &offset, 8);
				FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2+1)*8), &size, 8);
				if (use3D){
					FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((src->currentFrame*2+2)*8), &size_right, 8);
					size_right = size_right - size;
					offset_right = size;
				}
			}
			size = size - offset;
			unsigned char* frame = (unsigned char*)malloc(size);
			FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 16), frame, size);
			printJpg(frame, size, framebuf);
			free(frame);
			if (use3D){
				unsigned char* frame2 = (unsigned char*)malloc(size_right);
				FSFILE_Read(src->sourceFile, &bytesRead, offset_right + (src->tot_frame * 16), frame2, size_right);
				printJpg(frame2, size_right, TopRFB);
				free(frame2);
			}
		}
	}
}

static int lua_drawJPGVfast(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if ((argc != 2) && (argc != 3)) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	u32 bytesRead;
	u8* framebuf;
	if (luaL_checkinteger(L, 2) == 0) framebuf = TopLFB;
	else framebuf = BottomFB;
	bool use3D = false;
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	svcSignalEvent(updateStream);
	if (argc == 3) use3D = lua_toboolean(L,3);
	if (src->is3D){
		draw3DJPGVfast(src,framebuf,use3D);
		return 0;
	}
	if (src->isPlaying){
		if (src->currentFrame >= (src->tot_frame - 5)){
			if (src->loop == 1){
				src->currentFrame = 0;
				src->moltiplier = 1;
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				src->moltiplier = 1;
				CSND_SetPlayState(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
				CSND_UpdateInfo(0);
			}
		}else{
			double tmp = (double)((double)(osGetTime() - src->tick) / 1000.0) * src->framerate;
			src->currentFrame = (u32)floor(tmp);
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
				printJpg(frame, size, framebuf);
				free(frame);
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
			printJpg(frame, size, framebuf);
			free(frame);
		}
	}
	return 0;
}

static int lua_drawJPGV(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	#endif
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	JPGV* src = (JPGV*)luaL_checkinteger(L, 3);
	u32 bytesRead;
	int screen = luaL_checkinteger(L, 4);
	bool use3D = false;
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
		if ((x < 0) || (y < 0)) return luaL_error(L,"out of bounds");
	#endif
	svcSignalEvent(updateStream);
	if (argc == 5) use3D = lua_toboolean(L,5);
	if (src->is3D){
		draw3DJPGV(x,y,src,screen,use3D);
		return 0;
	}
	if (src->isPlaying){
		if (src->currentFrame >= (src->tot_frame - 5)){
			if (src->loop == 1){
				src->currentFrame = 0;
				src->moltiplier = 1;
				src->tick = osGetTime();
			}else{
				src->isPlaying = false;
				src->moltiplier = 1;
				CSND_SetPlayState(src->ch1, 0);
				if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
				CSND_UpdateInfo(0);
			}
		}else{
			double tmp = (double)((double)(osGetTime() - src->tick) / 1000.0) * src->framerate;
			src->currentFrame = (u32)floor(tmp);
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
				#ifndef SKIP_ERROR_HANDLING
					if ((screen <= 1) && (y+src->framebuf->width > 240)) return luaL_error(L,"out of framebuffer bounds");
					if ((screen == 0) && (x+src->framebuf->height > 400)) return luaL_error(L,"out of framebuffer bounds");
					if ((screen == 1) && (x+src->framebuf->height > 320)) return luaL_error(L,"out of framebuffer bounds");
				#endif
				if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,0);
				if (use3D) RAW2FB(x,y,src->framebuf,screen,1);
				free(src->framebuf->pixels);
				free(src->framebuf);
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
			#ifndef SKIP_ERROR_HANDLING
				if ((screen <= 1) && (y+src->framebuf->width > 240)) return luaL_error(L,"out of framebuffer bounds");
				if ((screen == 0) && (x+src->framebuf->height > 400)) return luaL_error(L,"out of framebuffer bounds");
				if ((screen == 1) && (x+src->framebuf->height > 320)) return luaL_error(L,"out of framebuffer bounds");
			#endif
			if (screen == 1 || screen == 0) RAW2FB(x,y,src->framebuf,screen,0);
			free(src->framebuf->pixels);
			free(src->framebuf);
		}
	}
	return 0;
}

void draw3DJPGVFrame(u16 x,u16 y,JPGV* src,u32 frame_index,u8 screen,bool is3D){
	u32 bytesRead;
	u64 offset;
	u64 size;
	u64 offset2;
	u64 size2;
	FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((frame_index*2)*8), &offset, 8);
	FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((frame_index*2+1)*8), &size, 8);
	if (is3D){
		FSFILE_Read(src->sourceFile, &bytesRead, 24+src->audio_size+((frame_index*2+2)*8), &size2, 8);
		offset2 = size;
		size2 = size2 - offset2;
	}
	size = size - offset;
	unsigned char* frame = (unsigned char*)malloc(size);
	FSFILE_Read(src->sourceFile, &bytesRead, offset + (src->tot_frame * 16), frame, size);
	Bitmap* tmp_framebuf = decodeJpg(frame, size);
	free(frame);
	RAW2FB(x,y,tmp_framebuf,screen,0);
	free(tmp_framebuf->pixels);
	free(tmp_framebuf);
	if (is3D){
		unsigned char* frame2 = (unsigned char*)malloc(size2);
		FSFILE_Read(src->sourceFile, &bytesRead, offset2 + (src->tot_frame * 16), frame2, size2);
		Bitmap* tmp_framebuf = decodeJpg(frame2, size2);
		free(frame2);
		RAW2FB(x,y,tmp_framebuf,screen,1);
		free(tmp_framebuf->pixels);
		free(tmp_framebuf);
	}
}

static int lua_JPGVshowFrame(lua_State *L){
int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if ((argc != 5) && (argc != 6)) return luaL_error(L, "wrong number of arguments");
	#endif
	u16 x = luaL_checkinteger(L, 1);
	u16 y = luaL_checkinteger(L, 2);
	JPGV* src = (JPGV*)luaL_checkinteger(L, 3);
	u32 frame_index = luaL_checkinteger(L, 4);
	u8 screen = luaL_checkinteger(L, 5);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
		if ((x < 0) || (y < 0)) return luaL_error(L,"out of bounds");
		if (frame_index > src->tot_frame) return luaL_error(L, "out of video file bounds");
	#endif
	bool is3D = false;
	if (argc == 6){
		is3D = lua_toboolean(L,6);
	}
	if (src->is3D){
		draw3DJPGVFrame(x,y,src,frame_index,screen,is3D);
		return 0;
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
	#ifndef SKIP_ERROR_HANDLING
		if ((screen <= 1) && (y+tmp_framebuf->width > 240)) return luaL_error(L,"out of framebuffer bounds");
		if ((screen == 0) && (x+tmp_framebuf->height > 400)) return luaL_error(L,"out of framebuffer bounds");
		if ((screen == 1) && (x+tmp_framebuf->height > 320)) return luaL_error(L,"out of framebuffer bounds");
	#endif
	RAW2FB(x,y,tmp_framebuf,screen,0);
	if (is3D) RAW2FB(x,y,tmp_framebuf,screen,1);
	free(tmp_framebuf->pixels);
	free(tmp_framebuf);;
	return 0;
}

static int lua_getFPS2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->framerate);
	return 1;
}

static int lua_getCF2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->currentFrame);
	return 1;
}

static int lua_getSize2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->tot_frame);
	return 1;
}

static int lua_getSrate2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_isPlaying2(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_unloadJPGV(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	audioChannels[src->ch1] = false;
	if (src->audiotype == 2) audioChannels[src->ch2] = false;
	if (src->thread != NULL){
		closeStream = true;
		svcSignalEvent(updateStream);
		while (closeStream){} // Wait for thread exiting...
		svcCloseHandle(updateStream);
		svcCloseHandle(streamThread);
		free(src->thread);
		sdmcExit();
	}
	if (src->samplerate != 0 && src->audio_size != 0 && src->audiobuf != NULL){
		linearFree(src->audiobuf);
		if (src->audiotype == 2){
			linearFree(src->audiobuf2);
		}
	}
	FSFILE_Close(src->sourceFile);
	svcCloseHandle(src->sourceFile);
	linearFree(tmp_buf);
	free(src);
	return 0;
}

static int lua_stopJPGV(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	src->isPlaying = false;
	src->currentFrame = 0;
	if (src->samplerate != 0 && src->audio_size != 0){
	CSND_SetPlayState(src->ch1, 0);
	if (src->audiotype == 2){
	CSND_SetPlayState(src->ch2, 0);
	}
	CSND_UpdateInfo(0);
	}
	return 0;
}

static int lua_pauseJPGV(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	src->isPlaying = false;
	src->tick = (osGetTime() - src->tick);
	if (src->samplerate != 0 && src->audio_size != 0){
	CSND_SetPlayState(src->ch1, 0);
	if (src->audiotype == 2){
	CSND_SetPlayState(src->ch2, 0);
	}
	CSND_UpdateInfo(0);
	}
	return 0;
}

static int lua_resumeJPGV(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	JPGV* src = (JPGV*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C4A5056) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	src->isPlaying = true;
	src->tick = (osGetTime() - src->tick);
	if (src->samplerate != 0 && src->audio_size != 0){
	CSND_SetPlayState(src->ch1, 1);
	if (src->audiotype == 2){
	CSND_SetPlayState(src->ch2, 1);
	}
	CSND_UpdateInfo(0);
	}
	return 0;
}

static int lua_dummy(lua_State *L){
	int argc = lua_gettop(L);
    return luaL_error(L, "JPGV module unavailable with dsp::DSP service.");
}

//Register our JPGV Functions
static const luaL_Reg JPGV_functions[] = {
  {"load",				lua_loadJPGV},
  {"start",				lua_startJPGV},
  {"draw",				lua_drawJPGV},
  {"drawFast",			lua_drawJPGVfast},
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

//Register our JPGV (Dummy) Functions for dsp::DSP service
static const luaL_Reg Dummy_functions[] = {
  {"load",				lua_dummy},
  {"start",				lua_dummy},
  {"draw",				lua_dummy},
  {"unload",			lua_dummy},
  {"getFPS",			lua_dummy},
  {"getFrame",			lua_dummy},
  {"showFrame",			lua_dummy},
  {"getSize",			lua_dummy},
  {"getSrate",			lua_dummy},
  {"isPlaying",			lua_dummy},
  {"stop",				lua_dummy},
  {"resume",			lua_dummy},
  {"pause",				lua_dummy},
  {0, 0}
};

void luaVideo_init(lua_State *L) {
	lua_newtable(L);
	if (csndAccess) luaL_setfuncs(L, JPGV_functions, 0);
	else luaL_setfuncs(L, Dummy_functions, 0);
	lua_setglobal(L, "JPGV");
	int LOOP = 1;
	int NO_LOOP = 0;
	VariableRegister(L,LOOP);
	VariableRegister(L,NO_LOOP);
}
