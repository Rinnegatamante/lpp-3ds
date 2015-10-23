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
#include "include/graphics/Graphics.h"
#include "include/luaAudio.h"
#include "include/ogg/ogg.h"
#include "include/ogg/codec.h"
#include "include/ogg/vorbisfile.h"

struct wav{
	u32 magic;
	u32 samplerate;
	u16 bytepersample;
	u8* audiobuf;
	u8* audiobuf2;
	u32 size;
	u32 mem_size;
	Handle sourceFile;
	u32 startRead;
	char author[256];
	char title[256];
	u32 moltiplier;
	u64 tick;
	bool isPlaying;
	u32 ch;
	u32 ch2;
	bool streamLoop;
	bool big_endian;
	u8 encoding;
	u32* thread;
	u32 audio_pointer;
	u32 package_size;
	u32 total_packages_size;
	u32 loop_index;
};

volatile bool closeStream = false;
Handle updateStream;
Handle streamThread;

int STREAM_MAX_ALLOC = 524288;
char pcmout[2048];
extern u8* tmp_buf;

void streamOGG(void* arg){ //TODO: Solve looping sound issues
	wav* src = (wav*)arg;
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
			u32 total = src->size;
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
			if ((src->streamLoop) && (control >= total * src->loop_index)) src->loop_index = src->loop_index + 1;
			if ((control >= total) && (src->isPlaying) && (!src->streamLoop)){
					src->isPlaying = false;
					src->tick = (osGetTime()-src->tick);
					src->moltiplier = 1;
					CSND_SetPlayState(src->ch, 0);
					if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
					csndExecCmds(0);
					ov_raw_seek((OggVorbis_File*)src->sourceFile,0);
					if (src->audiobuf2 == NULL){ //Mono file
					int i = 0;
					while(!eof){
						long ret=ov_read((OggVorbis_File*)src->sourceFile,pcmout,sizeof(pcmout),0,2,1,&current_section);
						if (ret == 0) {
		
							// EOF
							eof=1;
				
						} else {
					
							// Copying decoded block to PCM16 audiobuffer
							memcpy(&src->audiobuf[i],pcmout,ret);
							i = i + ret;
							if (i >= src->mem_size) break;
						}
					}
				}else{ //Stereo file
					int i = 0;
					while(!eof){
						long ret=ov_read((OggVorbis_File*)src->sourceFile,pcmout,sizeof(pcmout),0,2,1,&current_section);
						if (ret == 0) eof=1;
						else {
		
							// Copying decoded block to PCM16 audiobuffer
							memcpy(&tmp_buf[i],pcmout,ret);
							i = i + ret;
							if (i >= src->mem_size) break;	
						}
					}
				
						// Separating left and right channels
					int z;
					int j = 0;
					for (z=0; z < src->mem_size; z=z+4){
						src->audiobuf[j] = tmp_buf[z];
						src->audiobuf[j+1] = tmp_buf[z+1];
						src->audiobuf2[j] = tmp_buf[z+2];
						src->audiobuf2[j+1] = tmp_buf[z+3];
						j=j+2;
						if (j >= src->mem_size / 2) j = 0;
					}
				}
				src->moltiplier = 1;
			}
			if ((control >= (block_size * src->moltiplier)) && (src->isPlaying)){
				if (src->audiobuf2 == NULL){ //Mono file
						int i = 0;
						int j = src->audio_pointer;
						while(!eof){
							long ret=ov_read((OggVorbis_File*)src->sourceFile,pcmout,sizeof(pcmout),0,2,1,&current_section);
							if (ret == 0) {
								if (!src->streamLoop) eof=1;
								else ov_raw_seek((OggVorbis_File*)src->sourceFile,0);
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
						int i = 0;
						while(!eof){
							long ret=ov_read((OggVorbis_File*)src->sourceFile,pcmout,sizeof(pcmout),0,2,1,&current_section);
							if (ret == 0) {
								if (!src->streamLoop) eof=1;
								else ov_raw_seek((OggVorbis_File*)src->sourceFile,0);
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

void streamWAV(void* arg){
	wav* src = (wav*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		u32 bytesRead;
		u32 control;
			if(closeStream){
				closeStream = false;
				svcExitThread();
			}
			if (src->encoding == CSND_ENCODING_ADPCM) control = (src->samplerate / 2) * ((osGetTime() - src->tick) / 1000);
			else control = src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000);
			if (((control) >= (src->size - src->startRead)) && (src->isPlaying)){
				if (src->streamLoop){
					src->tick = osGetTime();
					src->moltiplier = 1;
				}else{
					src->isPlaying = false;
					src->tick = (osGetTime()-src->tick);
					src->moltiplier = 1;
					CSND_SetPlayState(src->ch, 0);
					if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
					csndExecCmds(0);
				}
				if (src->audiobuf2 == NULL){
					FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, src->audiobuf, src->mem_size);
					u64 i = 0;
					if (src->big_endian){
						while (i < (src->mem_size)){
							u8 tmp = src->audiobuf[i];
							src->audiobuf[i] = src->audiobuf[i+1];
							src->audiobuf[i+1] = tmp;
							i=i+2;
						}
					}
				}else{
					FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, tmp_buf, src->mem_size);
					u32 size_tbp = src->mem_size;
					u32 off=0;
					u32 i=0;
					u16 z;
					if (src->big_endian){
						while (i < size_tbp){
							z=0;
							while (z < (src->bytepersample/2)){
								src->audiobuf[off+z] = tmp_buf[i+(src->bytepersample/2)-z-1];
								src->audiobuf2[off+z] = tmp_buf[i+(src->bytepersample)-z-1];
								z++;
							}
							i=i+src->bytepersample;
							off=off+(src->bytepersample/2);
						}
					}else{
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
				}
			}else if (((control) > ((src->mem_size / 2) * src->moltiplier)) && (src->isPlaying)){
				if ((src->moltiplier % 2) == 1){
					//Update and flush first half-buffer
					if (src->audiobuf2 == NULL){
						if (src->encoding == CSND_ENCODING_ADPCM){ //ADPCM Decoding TODO
							u32 buffer_headers_num = ((src->mem_size)/2) / src->bytepersample;
							u8* tmp_audiobuf = tmp_buf;
							FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), tmp_audiobuf, (src->mem_size)/2);
							int z=0,i=0;
							while (i < (src->mem_size/2)){
								src->audiobuf[z] = tmp_audiobuf[i];
								z++;
								i++;
								if ((i % src->bytepersample) == 0) i=i+4;
							}
						}else{ //PCM-16 Decoding
							FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf, (src->mem_size)/2);
							u64 i = 0;
							if (bytesRead != ((src->mem_size)/2)){
								FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, src->audiobuf, (src->mem_size)/2);
								src->moltiplier = src->moltiplier + 1;
							}
							if (src->big_endian){
								while (i < ((src->mem_size)/2)){
									u8 tmp = src->audiobuf[i];
									src->audiobuf[i] = src->audiobuf[i+1];
									src->audiobuf[i+1] = tmp;
									i=i+2;
								}
							}
						}
						src->moltiplier = src->moltiplier + 1;
					}else{
						FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(src->mem_size/2)*(src->moltiplier + 1), tmp_buf, (src->mem_size)/2);
						if (bytesRead != ((src->mem_size)/2)){
							FSFILE_Read(src->sourceFile, &bytesRead, src->startRead, tmp_buf, (src->mem_size)/2);
							src->moltiplier = src->moltiplier + 1;
						}
						src->moltiplier = src->moltiplier + 1;
						u32 size_tbp = (src->mem_size)/2;
						u32 off=0;
						u32 i=0;
						u16 z;
						if (src->big_endian){
							while (i < size_tbp){
								z=0;
								while (z < (src->bytepersample/2)){
									src->audiobuf[off+z] = tmp_buf[i+(src->bytepersample/2)-z-1];
									src->audiobuf2[off+z] = tmp_buf[i+(src->bytepersample)-z-1];
									z++;
								}
								i=i+src->bytepersample;
								off=off+(src->bytepersample/2);
							}
						}else{
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
					}
				}else{
					u32 bytesRead;
					//Update and flush second half-buffer
					if (src->audiobuf2 == NULL){
						if (src->encoding == CSND_ENCODING_ADPCM){ // ADPCM Decoding TODO
							u32 buffer_headers_num = ((src->mem_size)/2) / src->bytepersample;
							u8* tmp_audiobuf = tmp_buf;
							FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), tmp_audiobuf, (src->mem_size)/2);
							int z=0,i=0;
							while (i < (src->mem_size/2)){
								src->audiobuf[z+(src->mem_size/2)] = tmp_audiobuf[i];
								z++;
								i++;
								if ((i % src->bytepersample) == 0) i=i+4;
							}
						}else{ // PCM-16 Decoding
							FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
							if (src->big_endian){
								u64 i = 0;
								while (i < ((src->mem_size)/2)){
									u8 tmp = src->audiobuf[i+((src->mem_size)/2)];
									src->audiobuf[i+((src->mem_size)/2)] = src->audiobuf[i+((src->mem_size)/2)+1];
									src->audiobuf[i+((src->mem_size)/2)+1] = tmp;
									i=i+2;
								}
							}
						}
						src->moltiplier = src->moltiplier + 1;
					}else{
						FSFILE_Read(src->sourceFile, &bytesRead, src->startRead+(src->mem_size/2)*(src->moltiplier + 1), tmp_buf, (src->mem_size)/2);
						src->moltiplier = src->moltiplier + 1;
						u32 size_tbp = (src->mem_size)/2;
						u32 off=0;
						u32 i=0;
						u16 z;
						if (src->big_endian){
							while (i < size_tbp){
								z=0;
								while (z < (src->bytepersample/2)){
									src->audiobuf[(src->mem_size)/4+off+z] = tmp_buf[i+(src->bytepersample/2)-z-1];
									src->audiobuf2[(src->mem_size)/4+off+z] = tmp_buf[i+(src->bytepersample)-z-1];
									z++;
								}
								z=0;
								i=i+src->bytepersample;
								off=off+(src->bytepersample/2);
							}
						}else{
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
}

static int lua_openogg(lua_State *L) 
{	
    int argc = lua_gettop(L);
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1); //Filename
	
	// Streaming support
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Using stdio instead of FS
	char myFile[512];
	strcpy(myFile,"sdmc:");
	strcat(myFile,file_tbo);
	sdmcInit();
	
	// Initializing libogg and vorbisfile
	int eof=0;
	OggVorbis_File* vf = (OggVorbis_File*)malloc(sizeof(OggVorbis_File));
	static int current_section;
	
	// Opening a valid OGG music file
	FILE* fp = fopen(myFile,"rb");
	if(ov_open(fp, vf, NULL, 0) != 0)
	{
		fclose(fp);
		return luaL_error(L, "corrupt OGG file.");
	}
	
	// Allocating music info
	wav *wav_file = (wav*)malloc(sizeof(wav));
	wav_file->magic = 0x4C534E44;
	vorbis_info* my_info = ov_info(vf,-1);
	wav_file->samplerate = my_info->rate;
	wav_file->big_endian = false;
	wav_file->encoding = CSND_ENCODING_VORBIS;
	wav_file->size = ov_time_total(vf,-1) * 2 * my_info->rate;
	wav_file->startRead = 0;
	wav_file->total_packages_size = 0;
	wav_file->loop_index = 1;
	wav_file->package_size = 0;
	wav_file->audio_pointer = 0;
	strcpy(wav_file->author,"");
	strcpy(wav_file->title,"");
	
	//Extracting metadatas
	u32 restore = ftell(fp); //Save offset for libOgg
	fseek(fp, 0x60, SEEK_SET);
	u32 half_magic = 0xDEADBEEF;
	u32 info_size = 0xDEADBEEF;
	u32 offset;
	char info_type[7];
	int i = 0;
	while (half_magic != 0x726F7603){
		i++;
		fread(&half_magic,4,1,fp);
		fseek(fp,0x60+i, SEEK_SET);
	}
	fseek(fp, 0x06, SEEK_CUR);
	fread(&info_size,4,1,fp);
	fseek(fp, info_size + 4, SEEK_CUR);
	fread(&info_size,4,1,fp);
	while (info_size != 0x6F760501){
		offset = ftell(fp);
		if (offset > 0x200) break; // Temporary patch for files without COMMENT section
		fread(&info_type,6,1,fp);
		if ((strcmp((const char*)&info_type,"ARTIST") == 0) || (strcmp((const char*)&info_type,"artist") == 0)){
			fseek(fp,0x01,SEEK_CUR);
			fread(&wav_file->author,info_size - 7,1,fp);
			wav_file->author[info_size - 7] = 0;
		}else if ((strcmp((const char*)&info_type,"TITLE=") == 0) || (strcmp((const char*)&info_type,"title=") == 0)){
			fread(&wav_file->title,info_size - 6,1,fp);
			wav_file->title[info_size - 6] = 0;
		}
		fseek(fp,offset+info_size, SEEK_SET);
		fread(&info_size,4,1,fp);
	}
	fseek(fp, restore, SEEK_SET); // Restore libOgg offset
	
	
	
	wav_file->isPlaying = false;
	wav_file->bytepersample = 2;
	
	// Decoding OGG buffer
	i = 0;
	if (my_info->channels == 1){ //Mono buffer
		if (mem_size){
			wav_file->moltiplier = 1;
			wav_file->mem_size = wav_file->size / 2;
			while (wav_file->mem_size > STREAM_MAX_ALLOC){
				wav_file->mem_size = wav_file->mem_size / 2;
			}
			wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
		}else{
			wav_file->audiobuf = (u8*)linearAlloc(wav_file->size);
			wav_file->mem_size = mem_size;
		}
		wav_file->audiobuf2 = NULL;
		while(!eof){
			long ret=ov_read(vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
			if (ret == 0) {
			
				// EOF
				eof=1;
				
			} else if (ret < 0) {
			
				// Error handling
				if(ret==OV_EBADLINK){
					return luaL_error(L, "corrupt bitstream section.");
				}
				
			} else {
			
				// Copying decoded block to PCM16 audiobuffer
				memcpy(&wav_file->audiobuf[i],pcmout,ret);
				i = i + ret;
				if ((mem_size) && (i >= wav_file->mem_size)) break;
			}
		}
	}else{ //Stereo buffer
		u32 size_tbp;
		if (mem_size){
			wav_file->moltiplier = 1;
			wav_file->bytepersample = 2;
			wav_file->mem_size = wav_file->size / 2;
			u8 molt = 2;
			if (wav_file->samplerate <= 30000) molt = 4; // Temporary patch for low samplerates
			while (wav_file->mem_size > STREAM_MAX_ALLOC * molt){
				wav_file->mem_size = wav_file->mem_size / 2;
			}
			size_tbp = wav_file->mem_size;
			tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
			wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size / 2);
			wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size / 2);
		}else{
			wav_file->bytepersample = 4;
			size_tbp = wav_file->size * 2;
			tmp_buf = (u8*)linearAlloc(wav_file->size * 2);
			wav_file->audiobuf = (u8*)linearAlloc(wav_file->size);
			wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->size);
			wav_file->mem_size = mem_size;
		}
		
		while(!eof){
			long ret=ov_read(vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
			if (ret == 0) {
			
				// EOF
				eof=1;
				
			} else if (ret < 0) {
			
				// Error handling
				if(ret==OV_EBADLINK){
					return luaL_error(L, "corrupt bitstream section.");
				}
				
			} else {
			
				// Copying decoded block to PCM16 audiobuffer
				memcpy(&tmp_buf[i],pcmout,ret);
				i = i + ret;
				if ((mem_size) && (i >= wav_file->mem_size)) break;	
			}
		}
		
		// Separating left and right channels
		int z;
		int j=0;
		for (z=0; z < size_tbp; z=z+4){
			wav_file->audiobuf[j] = tmp_buf[z];
			wav_file->audiobuf[j+1] = tmp_buf[z+1];
			wav_file->audiobuf2[j] = tmp_buf[z+2];
			wav_file->audiobuf2[j+1] = tmp_buf[z+3];
			j=j+2;
		}
		
	}
	
	if (!mem_size){
	
		// Deallocate OGG decoder resources and close file if not streaming
		ov_clear(vf);
		sdmcExit();
		
	}else{
		wav_file->sourceFile = (u32)vf;
	}
	
	// Push wav struct offset to LUA stack
	lua_pushinteger(L,(u32)wav_file);
	
	return 1;
}

static int lua_openwav(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	//if(ret) return luaL_error(L, "error opening file");
	u32 magic,samplerate,bytesRead,jump,chunk=0x00000000;
	u16 audiotype;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x46464952){
		wav *wav_file = (wav*)malloc(sizeof(wav));
		strcpy(wav_file->author,"");
		strcpy(wav_file->title,"");
		u64 size;
		wav_file->big_endian = false;
		u32 pos = 16;	
		while (chunk != 0x61746164){
			FSFILE_Read(fileHandle, &bytesRead, pos, &jump, 4);
			pos=pos+4+jump;
			FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
			pos=pos+4;
	
			//Chunk LIST detection
			if (chunk == 0x5453494C){
				u32 chunk_size;
				u32 subchunk;
				u32 subchunk_size;
				u32 sub_pos = pos+4;
				FSFILE_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
				if (subchunk == 0x4F464E49){
					sub_pos = sub_pos+4;
					FSFILE_Read(fileHandle, &bytesRead, pos, &chunk_size, 4);
					while (sub_pos < (chunk_size + pos + 4)){
						FSFILE_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
						FSFILE_Read(fileHandle, &bytesRead, sub_pos+4, &subchunk_size, 4);
						if (subchunk == 0x54524149){
							char* author = (char*)malloc(subchunk_size * sizeof(char));
							FSFILE_Read(fileHandle, &bytesRead, sub_pos+8, author, subchunk_size);
							strcpy(wav_file->author,author);
							wav_file->author[subchunk_size+1] = 0;
							free(author);
						}else if (subchunk == 0x4D414E49){
							char* title = (char*)malloc(subchunk_size * sizeof(char));
							FSFILE_Read(fileHandle, &bytesRead, sub_pos+8, title, subchunk_size);
							strcpy(wav_file->title,title);
							wav_file->title[subchunk_size+1] = 0;
							free(title);
						}
						sub_pos = sub_pos + 8 + subchunk_size;
						u8 checksum;
						FSFILE_Read(fileHandle, &bytesRead, sub_pos, &checksum, 1); //Prevent errors switching subchunks
						if (checksum == 0) sub_pos++;
					}
				}
			}
	
		}
		FSFILE_GetSize(fileHandle, &size);
		FSFILE_Read(fileHandle, &bytesRead, 22, &audiotype, 2);
		FSFILE_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
		u16 raw_enc;
		FSFILE_Read(fileHandle, &bytesRead, 20, &raw_enc, 2);
		FSFILE_Read(fileHandle, &bytesRead, 32, &(wav_file->bytepersample), 2);
		if (raw_enc == 0x01) wav_file->encoding = CSND_ENCODING_PCM16;
		else if (raw_enc == 0x11) wav_file->encoding = CSND_ENCODING_ADPCM;
		wav_file->samplerate = samplerate;
		wav_file->sourceFile = fileHandle;
		if (audiotype == 1){
			if (mem_size){
				wav_file->moltiplier = 1;
				wav_file->isPlaying = false;
				wav_file->mem_size = (size-(pos+4));
				while (wav_file->mem_size > STREAM_MAX_ALLOC){
					wav_file->mem_size = wav_file->mem_size / 2;
				}
				wav_file->startRead = (pos+4);
				if (raw_enc == 0x11){ // ADPCM Decoding
					u32 buffer_headers_num = (wav_file->mem_size) / wav_file->bytepersample;
					u8* tmp_audiobuf = (u8*)linearAlloc(wav_file->mem_size);
					FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_audiobuf, wav_file->mem_size);
					wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size-buffer_headers_num*4);
					int z=0,i=0;
					while (i < wav_file->mem_size){
						wav_file->audiobuf[z] = tmp_audiobuf[i];
						z++;
						i++;
						if ((i % wav_file->bytepersample) == 0) i=i+4;
					}
					tmp_buf = tmp_audiobuf;
				}else{ // PCM-16 Decoding
					wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
					FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, wav_file->audiobuf, wav_file->mem_size);
				}
				wav_file->audiobuf2 = NULL;
				wav_file->size = size;
			}else{
				if (raw_enc == 0x11){ // ADPCM Decoding
					u32 headers_num = (size-(pos+4)) / wav_file->bytepersample;
					u8* tmp_audiobuf = (u8*)linearAlloc(size-(pos+4));
					FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_audiobuf, size-(pos+4));
					wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4)-headers_num*4);
					int z=0,i=0;
					while (i < size){
						wav_file->audiobuf[z] = tmp_audiobuf[i];
						z++;
						i++;
						if ((i % wav_file->bytepersample) == 0) i=i+4;
					}
					tmp_buf = tmp_audiobuf;
					size = size-(pos+4) - headers_num * 4;
				}else{ // PCM-16 Decoding
					wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4));
					FSFILE_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
					size = size-(pos+4);
				}
				wav_file->audiobuf2 = NULL;
				wav_file->size = size;
				wav_file->startRead = 0;
			}
		}else{
			// I must reordinate my buffer in order to play stereo sound (Thanks CSND/FS libraries .-.)
			u32 size_tbp;
			if (mem_size){
				wav_file->moltiplier = 1;
				wav_file->isPlaying = false;
				wav_file->startRead = (pos+4);
				wav_file->size = size;
				wav_file->mem_size = (size-(pos+4));
				while (wav_file->mem_size > STREAM_MAX_ALLOC){
					wav_file->mem_size = wav_file->mem_size / 2;
				}
				tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
				wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size/2);
				wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size/2);
				FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, tmp_buf, wav_file->mem_size);
				size_tbp = wav_file->mem_size;
			}else{
				tmp_buf = (u8*)linearAlloc((size-(pos+4)));
				size_tbp = size-(pos+4);
				wav_file->startRead = 0;
				wav_file->size = (size_tbp)/2;
				FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_buf, size-(pos+4));
				wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))/2);
				wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))/2);
			}
			u32 off=0;
			u32 i=0;
			u16 z;
			if (raw_enc == 0x01){ //PCM16 Decoding				
				while (i < size_tbp){
					z=0;
					while (z < (wav_file->bytepersample/2)){
						wav_file->audiobuf[off+z] = tmp_buf[i+z];
						wav_file->audiobuf2[off+z] = tmp_buf[i+z+(wav_file->bytepersample/2)];
						z++;
					}
					i=i+wav_file->bytepersample;
					off=off+(wav_file->bytepersample/2);
				}
			}else if (raw_enc == 0x11){ //ADPCM Decoding
				u32 headers_num = (size_tbp) / wav_file->bytepersample;
				wav_file->audiobuf = (u8*)linearAlloc((size_tbp-headers_num*8)/2);
				wav_file->audiobuf2 = (u8*)linearAlloc((size_tbp-headers_num*8)/2);
				int z=0,i=0;
				while (i < size_tbp){
					wav_file->audiobuf[z] = tmp_buf[i++];
					wav_file->audiobuf2[z++] = tmp_buf[i+3];
					wav_file->audiobuf[z] = tmp_buf[i++];
					wav_file->audiobuf2[z++] = tmp_buf[i+3];
					wav_file->audiobuf[z] = tmp_buf[i++];
					wav_file->audiobuf2[z++] = tmp_buf[i+3];
					wav_file->audiobuf[z] = tmp_buf[i++];
					wav_file->audiobuf2[z++] = tmp_buf[i+3];
					i=i+4;
					if ((i % wav_file->bytepersample) == 0) i=i+8;
				}
			}
		}
		wav_file->magic = 0x4C534E44;
		lua_pushinteger(L,(u32)wav_file);
	}
	if (!mem_size){
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
	}
	return 1;
}

static u32 Endian_UInt32_Conversion(u32 value){
   return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000);
}

static u16 Endian_UInt16_Conversion(u16 value){
   return (u16)(((value >> 8) & 0x00FF) | ((value << 8) & 0xFF00));
}
static int lua_openaiff(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	//if(ret) return luaL_error(L, "error opening file");
	u32 magic,bytesRead,jump,chunk=0x00000000;
	u16 samplerate;
	u16 audiotype;
	FSFILE_Read(fileHandle, &bytesRead, 8, &magic, 4);
	if (magic == 0x46464941){
	wav *wav_file = (wav*)malloc(sizeof(wav));
	strcpy(wav_file->author,"");
	strcpy(wav_file->title,"");
	wav_file->encoding = CSND_ENCODING_PCM16;
	u64 size;
	wav_file->big_endian = true;
	u32 pos = 12;	
	FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);
	while (chunk != 0x444E5353){
	
	//Chunks detection
	if (chunk == 0x454D414E){ //NAME Chunk
		u32 chunk_size;
		FSFILE_Read(fileHandle, &bytesRead, pos+4, &chunk_size, 4);
		chunk_size = Endian_UInt32_Conversion(chunk_size);
		char* title = (char*)malloc(chunk_size * sizeof(char));
		FSFILE_Read(fileHandle, &bytesRead, pos+8, title, chunk_size);
		strcpy(wav_file->title,title);
		wav_file->title[chunk_size+1] = 0;
		free(title);
	}else if (chunk == 0x4D4D4F43){ //COMM Chunk
		FSFILE_Read(fileHandle, &bytesRead, pos+8, &audiotype, 2);
		audiotype = audiotype / 256;
		FSFILE_Read(fileHandle, &bytesRead, pos+14, &(wav_file->bytepersample), 2);
		wav_file->bytepersample = (wav_file->bytepersample / 2048) * audiotype;
		FSFILE_Read(fileHandle, &bytesRead, pos+18, &samplerate, 2);
		samplerate = Endian_UInt16_Conversion(samplerate);
	}
	
	pos=pos+4;
	FSFILE_Read(fileHandle, &bytesRead, pos, &jump, 4);
	pos=pos+4+Endian_UInt32_Conversion(jump);
	FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
	}
	pos=pos+4;
	FSFILE_GetSize(fileHandle, &size);
	wav_file->mem_size = mem_size;
	wav_file->samplerate = samplerate;
	if (audiotype == 1){
	if (mem_size){
	wav_file->moltiplier = 1;
	wav_file->isPlaying = false;
	wav_file->mem_size = (size-(pos+4));
	while (wav_file->mem_size > STREAM_MAX_ALLOC * 2){
		wav_file->mem_size = wav_file->mem_size / 2;
	}
	wav_file->sourceFile = fileHandle;
	wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
	wav_file->startRead = (pos+4);
	FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, wav_file->audiobuf, wav_file->mem_size);
	
	// Changing endianess
	u64 i = 0;
	while (i < wav_file->mem_size){
		u8 tmp = wav_file->audiobuf[i];
		wav_file->audiobuf[i] = wav_file->audiobuf[i+1];
		wav_file->audiobuf[i+1] = tmp;
		i=i+2;
	}
	
	wav_file->audiobuf2 = NULL;
	wav_file->size = size;
	}else{
	wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4));
	FSFILE_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
	
	// Changing endianess
	u64 i = 0;
	while (i < (size-(pos+4))){
		u8 tmp = wav_file->audiobuf[i];
		wav_file->audiobuf[i] = wav_file->audiobuf[i+1];
		wav_file->audiobuf[i+1] = tmp;
		i=i+2;
	}
	
	wav_file->audiobuf2 = NULL;
	wav_file->size = size-(pos+4);
	wav_file->startRead = 0;
	wav_file->isPlaying = false;
	}
	}else{
	// I must reordinate my buffer in order to play stereo sound and change endianess
	u32 size_tbp;
	if (mem_size){
	wav_file->moltiplier = 1;
	wav_file->sourceFile = fileHandle;
	wav_file->isPlaying = false;
	wav_file->startRead = (pos+4);
	wav_file->size = size;
	wav_file->mem_size = (size-(pos+4));
	while (wav_file->mem_size > STREAM_MAX_ALLOC * 10){
		wav_file->mem_size = wav_file->mem_size / 2;
	}
	tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
	wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size/2);
	wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size/2);
	FSFILE_Read(fileHandle, &bytesRead, wav_file->startRead, tmp_buf, wav_file->mem_size);
	size_tbp = wav_file->mem_size;
	}else{
	tmp_buf = (u8*)linearAlloc((size-(pos+4)));
	wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))/2);
	wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))/2);
	wav_file->startRead = 0;
	size_tbp = size-(pos+4);
	wav_file->size = (size_tbp)/2;
	FSFILE_Read(fileHandle, &bytesRead, pos+4, tmp_buf, size-(pos+4));
	}
	u32 off=0;
	u32 i=0;
	u16 z;
	while (i < size_tbp){
	z=0;
	while (z < (wav_file->bytepersample/2)){
	wav_file->audiobuf[off+z] = tmp_buf[i+(wav_file->bytepersample/2)-z-1];
	wav_file->audiobuf2[off+z] = tmp_buf[i+(wav_file->bytepersample)-z-1];
	z++;
	}
	i=i+wav_file->bytepersample;
	off=off+(wav_file->bytepersample/2);
	}
	}
	wav_file->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)wav_file);
	}
	if (!mem_size){
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	}
	return 1;
}

static int lua_soundinit(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	if (!isCSND){
		csndInit();
		isCSND = true;
	}
	return 0;
}

static int lua_playWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4))	return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	closeStream = false;
	int loop = luaL_checkinteger(L, 2);
	u32 ch = luaL_checkinteger(L, 3);
	u32 ch2;
	bool non_native_encode = false;
	ThreadFunc streamFunction = streamWAV;
	u8 tmp_encode;
	if (argc == 4) ch2 = luaL_checkinteger(L, 4);
	if (src->encoding == CSND_ENCODING_VORBIS){
		streamFunction = streamOGG;
		tmp_encode = src->encoding;
		src->encoding = CSND_ENCODING_PCM16;
		non_native_encode = true;
	}
	if (src->audiobuf2 == NULL){
		if (src->mem_size > 0){
			if (loop == 0) src->streamLoop = false;
			else src->streamLoop = true;
			svcCreateEvent(&updateStream,0);
			u32 *threadStack = (u32*)memalign(32, 8192);
			src->thread = threadStack;
			svcSignalEvent(updateStream);
			Result ret = svcCreateThread(&streamThread, streamFunction, (u32)src, &threadStack[2048], 0x18, 1);
			My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size, 1.0, 2.0);
		}else{
			u32 looping = loop ? SOUND_REPEAT : SOUND_ONE_SHOT;
			My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, 2.0);
		}
		src->ch = ch;
		src->tick = osGetTime();
		CSND_SetPlayState(ch, 1);
		csndExecCmds(0);
	}else{
		if (src->mem_size > 0){
			if (loop == 0) src->streamLoop = false;
			else src->streamLoop = true;
			svcCreateEvent(&updateStream,0);
			u32 *threadStack = (u32*)memalign(32, 8192);
			src->thread = threadStack;
			svcSignalEvent(updateStream);
			Result ret = svcCreateThread(&streamThread, streamFunction, (u32)src, &threadStack[2048], 0x18, 1);
			My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), (src->mem_size)/2, 1.0, 1.0);
			My_CSND_playsound(ch2, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), (src->mem_size)/2, 1.0, -1.0);
		}else{
			u32 looping = loop ? SOUND_REPEAT : SOUND_ONE_SHOT;
			My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, 1.0);
			My_CSND_playsound(ch2, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->size, 1.0, -1.0);
		}
		src->ch = ch;
		src->ch2 = ch2;
		src->tick = osGetTime();
		CSND_SetPlayState(ch, 1);
		CSND_SetPlayState(ch2, 1);
		csndExecCmds(0);
	}
	if (non_native_encode){
		src->encoding = tmp_encode;
	}
	src->isPlaying = true;
	return 0;
}

static int lua_closeWav(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->mem_size > 0){
		closeStream = true;
		svcSignalEvent(updateStream);
		while (closeStream){} // Wait for thread exiting...
		svcCloseHandle(updateStream);
		svcCloseHandle(streamThread);
		free(src->thread);
		if (src->encoding == CSND_ENCODING_VORBIS){
			ov_clear((OggVorbis_File*)src->sourceFile);
			sdmcExit();
		}else{
			FSFILE_Close(src->sourceFile);
			svcCloseHandle(src->sourceFile);
		}
	}
	linearFree(src->audiobuf);
	if (src->audiobuf2 != NULL) linearFree(src->audiobuf2);
	if (tmp_buf != NULL) linearFree(tmp_buf);
	free(src);
	return 0;
}

static int lua_pause(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	CSND_SetPlayState(src->ch, 0);
	if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
	csndExecCmds(0);
	src->isPlaying = false;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_resume(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	CSND_SetPlayState(src->ch, 1);
	if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 1);
	csndExecCmds(0);
	src->isPlaying = true;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_wisPlaying(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_soundend(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	if (isCSND){
		csndExit();
		isCSND = false;
	}
	return 0;
}

static int lua_regsound(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	u32 time = luaL_checkinteger(L, 1);
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
	wav_file->big_endian = false;
	wav_file->mem_size = 0;
	wav_file->size = mem_size - 32000;
	wav_file->samplerate = 16000;
	strcpy(wav_file->author,"");
	strcpy(wav_file->title,"");
	wav_file->isPlaying = false;
	wav_file->bytepersample = 2;
	wav_file->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)wav_file);
	return 1;
}

static int lua_savemono(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
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
	FSFILE_Write(fileHandle, &bytesWritten, 20, &(src->encoding), 2, FS_WRITE_FLUSH);
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

static int lua_getSrate(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_getTime(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->isPlaying){
		if (src->streamLoop && src->encoding == CSND_ENCODING_VORBIS) lua_pushinteger(L, ((osGetTime() - src->tick) / 1000) / src->loop_index);
		else lua_pushinteger(L, (osGetTime() - src->tick) / 1000);
	}else{
		if (src->streamLoop && src->encoding == CSND_ENCODING_VORBIS) lua_pushinteger(L, (src->tick / 1000) / src->loop_index);
		else lua_pushinteger(L, src->tick / 1000);
	}
	return 1;
}

static int lua_getTotalTime(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if ((src->encoding == CSND_ENCODING_ADPCM) && (src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,(src->size - src->startRead) / (src->samplerate / 2));
	else if ((src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,((src->size) - src->startRead) / ((src->bytepersample / 2) * src->samplerate));
	else lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample) * src->samplerate));
	return 1;
}

static int lua_getTitle(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushstring(L, src->title);
	return 1;
}

static int lua_getAuthor(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushstring(L, src->author);
	return 1;
}

static int lua_getType(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->audiobuf2 == NULL) lua_pushinteger(L, 1);
	else lua_pushinteger(L, 2);
	return 1;
}

static int lua_updatestream(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	svcSignalEvent(updateStream);
	return 0;
}

//Register our Sound Functions
static const luaL_Reg Sound_functions[] = {
  {"openOgg",				lua_openogg},
  {"openWav",				lua_openwav},
  {"openAiff",				lua_openaiff},
  {"close",					lua_closeWav},
  {"play",					lua_playWav},
  {"init",					lua_soundinit},
  {"term",					lua_soundend},
  {"pause",					lua_pause},
  {"getSrate",				lua_getSrate},
  {"getTime",				lua_getTime},
  {"getTitle",				lua_getTitle},
  {"getAuthor",				lua_getAuthor},
  {"getType",				lua_getType},  
  {"getTotalTime",			lua_getTotalTime},
  {"resume",				lua_resume},
  {"isPlaying",				lua_wisPlaying},
  {"register",				lua_regsound},
  {"updateStream",			lua_updatestream},
  {"saveWav",				lua_savemono},
  {0, 0}
};

void luaSound_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Sound_functions, 0);
	lua_setglobal(L, "Sound");
}