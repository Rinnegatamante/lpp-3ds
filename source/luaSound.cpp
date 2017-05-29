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
#include "include/utils.h"
#include "include/luaplayer.h"
#include "include/graphics/Graphics.h"
#include "include/audio.h"
#include "include/ogg/ogg.h"
#include "include/ogg/codec.h"
#include "include/ogg/vorbisfile.h"
#include "include/audiodec/audio_decoder.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define STREAM_MAX_ALLOC 524288
#define DECODER_MAX_ALLOC 786432

extern bool audioChannels[32];
extern bool isNinjhax2;
extern bool csndAccess;
volatile bool closeStream = false;
Handle updateStream;
Handle streamThread;
char pcmout[2048];
extern u8* tmp_buf;

void streamOGG_CSND(void* arg){
	wav* src = (wav*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		u32 bytesRead;
		
		if(closeStream){
			closeStream = false;
			threadExit(0);
		}
			
		// Initializing libogg and vorbisfile
		int eof=0;
		static int current_section;
	
		u32 control;
		u32 total = src->size;
		u32 block_size;
		u32 package_max_size;
		if (src->package_size == 0){
			block_size = src->mem_size>>3;
			package_max_size = block_size;
		}else{
			block_size = src->total_packages_size / (src->moltiplier - 1);
			package_max_size = src->mem_size>>3;
		}
		if (src->audiobuf2 == NULL) control = (src->samplerate<<1) * ((osGetTime() - src->tick) / 1000);
		else{
			control = (src->samplerate<<2) * ((osGetTime() - src->tick) / 1000);
			total = total<<1;
		}
		if ((src->streamLoop) && (control >= total * src->loop_index)) src->loop_index = src->loop_index + 1;
		if ((control >= total) && (src->isPlaying) && (!src->streamLoop)){
			src->isPlaying = false;
			src->tick = (osGetTime()-src->tick);
			CSND_SetPlayState(src->ch, 0);
			if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
			CSND_UpdateInfo(0);
			ov_raw_seek((OggVorbis_File*)src->sourceFile,0);
			if (src->audiobuf2 == NULL){ //Mono file
				int i = 0;
				while(!eof){
					long ret=ov_read((OggVorbis_File*)src->sourceFile,pcmout,sizeof(pcmout),0,2,1,&current_section);
					if (ret == 0) {

						// EOF
						eof=1;

					}else{

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
					else{

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
					if (j >= src->mem_size>>1) j = 0;
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
					}else{
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
					}else{
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
					if (j >= src->mem_size>>1) j = 0;
				}
				src->audio_pointer = j;
				src->total_packages_size = src->total_packages_size + src->package_size;
			}
			src->moltiplier = src->moltiplier + 1;
		}
	}
}

void streamWAV_CSND(void* arg){
	wav* src = (wav*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		u32 bytesRead;
		u32 control;
		if(closeStream){
			closeStream = false;
			threadExit(0);
		}
		u8 half_bps = src->bytepersample >> 1;
		u32 half_mem = src->mem_size>>1;
		if (src->encoding == CSND_ENCODING_ADPCM) control = (src->samplerate>>1) * ((osGetTime() - src->tick) / 1000);
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
				CSND_UpdateInfo(0);
			}
			if (src->audiobuf2 == NULL){
				FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead, src->audiobuf, src->mem_size);
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
				FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead, tmp_buf, src->mem_size);
				u32 size_tbp = src->mem_size;
				u32 off=0;
				u32 i=0;
				u16 z;
				if (src->big_endian){
					while (i < size_tbp){
						z=0;
						while (z < half_bps){
							src->audiobuf[off+z] = tmp_buf[i+half_bps-z-1];
							src->audiobuf2[off+z] = tmp_buf[i+half_bps-z-1];
							z++;
						}
						i=i+src->bytepersample;
						off=off+half_bps;
					}
				}else{
					while (i < size_tbp){
						z=0;
						while (z < half_bps){
							src->audiobuf[off+z] = tmp_buf[i+z];
							src->audiobuf2[off+z] = tmp_buf[i+z+half_bps];
							z++;
						}
						i=i+src->bytepersample;
						off=off+half_bps;
					}
				}
			}
		}else if (((control) > (half_mem * src->moltiplier)) && (src->isPlaying)){
			if ((src->moltiplier % 2) == 1){
				//Update and flush first half-buffer
				if (src->audiobuf2 == NULL){
					if (src->encoding == CSND_ENCODING_ADPCM){ //ADPCM Decoding TODO
						u32 buffer_headers_num = half_mem / src->bytepersample;
						u8* tmp_audiobuf = tmp_buf;
						FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+(half_mem*(src->moltiplier + 1)), tmp_audiobuf, half_mem);
						int z=0,i=0;
						while (i < half_mem){
							src->audiobuf[z] = tmp_audiobuf[i];
							z++;
							i++;
							if ((i % src->bytepersample) == 0) i=i+4;
						}
					}else{ //PCM-16 Decoding
						FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+(half_mem*(src->moltiplier + 1)), src->audiobuf, half_mem);
						u64 i = 0;
						if (bytesRead != half_mem){
							FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead, src->audiobuf, half_mem);
							src->moltiplier = src->moltiplier + 1;
						}
						if (src->big_endian){
							while (i < half_mem){
								u8 tmp = src->audiobuf[i];
								src->audiobuf[i] = src->audiobuf[i+1];
								src->audiobuf[i+1] = tmp;
								i=i+2;
							}
						}
					}
					src->moltiplier = src->moltiplier + 1;
				}else{
					FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+half_mem*(src->moltiplier + 1), tmp_buf, half_mem);
					if (bytesRead != half_mem){
						FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead, tmp_buf, half_mem);
						src->moltiplier = src->moltiplier + 1;
					}
					src->moltiplier = src->moltiplier + 1;
					u32 size_tbp = half_mem;
					u32 off=0;
					u32 i=0;
					u16 z;
					if (src->big_endian){
						while (i < size_tbp){
							z=0;
							while (z < half_bps){
								src->audiobuf[off+z] = tmp_buf[i+half_bps-z-1];
								src->audiobuf2[off+z] = tmp_buf[i+(src->bytepersample)-z-1];
								z++;
							}
							i=i+src->bytepersample;
							off=off+half_bps;
						}
					}else{
						while (i < size_tbp){
							z=0;
							while (z < half_bps){
								src->audiobuf[off+z] = tmp_buf[i+z];
								src->audiobuf2[off+z] = tmp_buf[i+z+half_bps];
								z++;
							}
							i=i+src->bytepersample;
							off=off+half_bps;
						}
					}
				}
			}else{
				u32 bytesRead;
				//Update and flush second half-buffer
				if (src->audiobuf2 == NULL){
					if (src->encoding == CSND_ENCODING_ADPCM){ // ADPCM Decoding TODO
						u32 buffer_headers_num = half_mem / src->bytepersample;
						u8* tmp_audiobuf = tmp_buf;
						FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+(half_mem*(src->moltiplier + 1)), tmp_audiobuf, half_mem);
						int z=0,i=0;
						while (i < half_mem){
							src->audiobuf[z+half_mem] = tmp_audiobuf[i];
							z++;
							i++;
							if ((i % src->bytepersample) == 0) i=i+4;
						}
					}else{ // PCM-16 Decoding
						FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+(((src->mem_size)/2)*(src->moltiplier + 1)), src->audiobuf+((src->mem_size)/2), (src->mem_size)/2);
						if (src->big_endian){
							u64 i = 0;
							while (i < ((src->mem_size)/2)){
								u8 tmp = src->audiobuf[i+half_mem];
								src->audiobuf[i+half_mem] = src->audiobuf[i+half_mem+1];
								src->audiobuf[i+half_mem+1] = tmp;
								i=i+2;
							}
						}
					}
					src->moltiplier = src->moltiplier + 1;
				}else{
					int quart_mem = half_mem >> 1;
					FS_Read((fileStream*)src->sourceFile, &bytesRead, src->startRead+half_mem*(src->moltiplier + 1), tmp_buf, half_mem);
					src->moltiplier = src->moltiplier + 1;
					u32 size_tbp = half_mem;
					u32 off=0;
					u32 i=0;
					u16 z;
					if (src->big_endian){
						while (i < size_tbp){
							z=0;
							while (z < half_bps){
								src->audiobuf[quart_mem+off+z] = tmp_buf[i+half_bps-z-1];
								src->audiobuf2[quart_mem+off+z] = tmp_buf[i+(src->bytepersample)-z-1];
								z++;
							}
							z=0;
							i=i+src->bytepersample;
							off=off+half_bps;
						}
					}else{
						while (i < size_tbp){
							z=0;
							while (z < half_bps){
								src->audiobuf[quart_mem+off+z] = tmp_buf[i+z];
								src->audiobuf2[quart_mem+off+z] = tmp_buf[i+z+half_bps];
								z++;
							}
							i=i+src->bytepersample;
							off=off+half_bps;
						}
					}
				}
			}
		}
	}		
}

void streamDEC_CSND(void* arg){
	wav* src = (wav*)arg;
	while(1) {
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		if(closeStream){
			closeStream = false;
			threadExit(0);
		}
		u32 control = src->samplerate * src->bytepersample * ((osGetTime() - src->tick) / 1000);
		u32 blockmem = src->mem_size>>1;
		if (control > src->moltiplier * blockmem){
			src->moltiplier = src->moltiplier + 1;
			u8 half_check = (src->moltiplier)%2;
			AudioDecoder* audio_decoder = (AudioDecoder*)src->misc;
			if (src->audiobuf2 == NULL){
				audio_decoder->Decode(src->audiobuf+(half_check*blockmem), blockmem);
			}else{
				audio_decoder->DecodeAsMono(src->audiobuf+(half_check*blockmem), src->audiobuf2+(half_check*blockmem), blockmem);
			}
		}
	}
}

static int lua_openogg_old(lua_State *L) {	
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1); //Filename
	
	// Streaming support
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Using stdio instead of FS
	char myFile[512];
	if (strncmp("romfs:/",file_tbo,7) == 0) strcpy(myFile,file_tbo);
	else{
		strcpy(myFile,"sdmc:");
		strcat(myFile,file_tbo);
	}
	sdmcInit();
	
	// Initializing libogg and vorbisfile
	int eof=0;
	OggVorbis_File* vf = (OggVorbis_File*)malloc(sizeof(OggVorbis_File));
	static int current_section;
	
	// Opening a valid OGG music file
	FILE* fp = fopen(myFile,"r");
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
	wav_file->size = ov_time_total(vf,-1) * (my_info->rate<<1); // TODO: Find a more accurated calculation method
	wav_file->startRead = 0;
	wav_file->total_packages_size = 0;
	wav_file->loop_index = 1;
	wav_file->ch = 0xDEADBEEF;
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
		wav_file->bytepersample = 1;
		if (mem_size){
			wav_file->moltiplier = 1;
			wav_file->mem_size = wav_file->size>>1;
			while (wav_file->mem_size > STREAM_MAX_ALLOC){
				wav_file->mem_size = wav_file->mem_size>>1;
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
				#ifndef SKIP_ERROR_HANDLING
				if(ret==OV_EBADLINK) return luaL_error(L, "corrupt bitstream section.");
				#endif
				
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
			wav_file->mem_size = wav_file->size>>1;
			u8 molt = 2;
			if (wav_file->samplerate <= 30000) molt = 4; // Temporary patch for low samplerates
			while (wav_file->mem_size > STREAM_MAX_ALLOC * molt){
				wav_file->mem_size = wav_file->mem_size>>1;
			}
			size_tbp = wav_file->mem_size;
			tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
			wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size>>1);
			wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size>>1);
		}else{
			wav_file->bytepersample = 4;
			size_tbp = wav_file->size * 2;
			tmp_buf = (u8*)linearAlloc(wav_file->size<<1);
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
				#ifndef SKIP_ERROR_HANDLING
				if(ret==OV_EBADLINK) return luaL_error(L, "corrupt bitstream section.");
				#endif
				
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

static int lua_openmp3_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1); //Filename
	
	// Streaming support
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Using stdio instead of FS
	char myFile[512];
	if (strncmp("romfs:/",file_tbo,7) == 0) strcpy(myFile,file_tbo);
	else{
		strcpy(myFile,"sdmc:");
		strcat(myFile,file_tbo);
	}
	sdmcInit();
	
	// Initializing audio decoder
	FILE* fp = fopen(myFile,"rb");
	if (fp == NULL) return luaL_error(L, "error opening file.");
	AudioDecoder* audio_decoder;
	audio_decoder = AudioDecoder::Create(fp, myFile);
	if (audio_decoder == NULL) return luaL_error(L, "error opening audio decoder.");
	audio_decoder->Open(fp);
	AudioDecoder::Format int_format;
	
	// Allocating music info
	int audiotype;
	wav *wav_file = (wav*)malloc(sizeof(wav));
	wav_file->magic = 0x4C534E44;
	int samplerate;
	audio_decoder->GetFormat(samplerate, int_format, audiotype);
	if (audiotype > 2) audio_decoder->SetFormat(samplerate, AudioDecoder::Format::S16, 2);
	wav_file->samplerate = samplerate;
	wav_file->big_endian = false;
	wav_file->encoding = CSND_ENCODING_MPEG;
	wav_file->size = 0; // TODO: Add a proper size detection
	wav_file->ch = 0xDEADBEEF;
	wav_file->moltiplier = 1;
	wav_file->bytepersample = audiotype<<1;
	wav_file->misc = (u32*)audio_decoder;
	strcpy(wav_file->author,"");
	strcpy(wav_file->title,"");
	
	//Extracting metadatas
	// TODO: Add metadatas extraction
		
	wav_file->isPlaying = false;
	
	// Decoding MP3 buffer
	if (audiotype == 1){ //Mono buffer
		wav_file->mem_size = DECODER_MAX_ALLOC;
		wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
		wav_file->audiobuf2 = NULL;
		audio_decoder->Decode(wav_file->audiobuf, wav_file->mem_size);
	}else{ //Stereo buffer
		wav_file->mem_size = DECODER_MAX_ALLOC;
		wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
		wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size);
		audio_decoder->DecodeAsMono(wav_file->audiobuf, wav_file->audiobuf2, wav_file->mem_size);
	}
	
	// Push wav struct offset to LUA stack
	lua_pushinteger(L,(u32)wav_file);
	
	return 1;
}


static int lua_openwav_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	fileStream* fileHandle = (fileStream*)malloc(sizeof(fileStream));
	if (strncmp("romfs:/",file_tbo,7) == 0){
		fileHandle->isRomfs = true;
		FILE* handle = fopen(file_tbo,"rb");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle->handle = (u32)handle;
	}else{
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly(&fileHandle->handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if(ret) return luaL_error(L, "error opening file");
		#endif
	}
	u32 magic,samplerate,bytesRead,jump,chunk=0x00000000;
	u16 audiotype;
	FS_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x46464952){
		wav *wav_file = (wav*)malloc(sizeof(wav));
		wav_file->ch = 0xDEADBEEF;
		strcpy(wav_file->author,"");
		strcpy(wav_file->title,"");
		u64 size;
		wav_file->big_endian = false;
		u32 pos = 16;	
		while (chunk != 0x61746164){
			FS_Read(fileHandle, &bytesRead, pos, &jump, 4);
			pos=pos+4+jump;
			FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
			pos=pos+4;
	
			//Chunk LIST detection
			if (chunk == 0x5453494C){
				u32 chunk_size;
				u32 subchunk;
				u32 subchunk_size;
				u32 sub_pos = pos+4;
				FS_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
				if (subchunk == 0x4F464E49){
					sub_pos = sub_pos+4;
					FS_Read(fileHandle, &bytesRead, pos, &chunk_size, 4);
					while (sub_pos < (chunk_size + pos + 4)){
						FS_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
						FS_Read(fileHandle, &bytesRead, sub_pos+4, &subchunk_size, 4);
						if (subchunk == 0x54524149){
							char* author = (char*)malloc(subchunk_size);
							FS_Read(fileHandle, &bytesRead, sub_pos+8, author, subchunk_size);
							strcpy(wav_file->author,author);
							wav_file->author[subchunk_size+1] = 0;
							free(author);
						}else if (subchunk == 0x4D414E49){
							char* title = (char*)malloc(subchunk_size);
							FS_Read(fileHandle, &bytesRead, sub_pos+8, title, subchunk_size);
							strcpy(wav_file->title,title);
							wav_file->title[subchunk_size+1] = 0;
							free(title);
						}
						sub_pos = sub_pos + 8 + subchunk_size;
						u8 checksum;
						FS_Read(fileHandle, &bytesRead, sub_pos, &checksum, 1); //Prevent errors switching subchunks
						if (checksum == 0) sub_pos++;
					}
				}
			}
	
		}
		FS_GetSize(fileHandle, &size);
		FS_Read(fileHandle, &bytesRead, 22, &audiotype, 2);
		FS_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
		u16 raw_enc;
		FS_Read(fileHandle, &bytesRead, 20, &raw_enc, 2);
		FS_Read(fileHandle, &bytesRead, 32, &(wav_file->bytepersample), 2);
		if (raw_enc == 0x01) wav_file->encoding = CSND_ENCODING_PCM16;
		else if (raw_enc == 0x11) wav_file->encoding = CSND_ENCODING_ADPCM;
		wav_file->samplerate = samplerate;
		wav_file->sourceFile = (u32)fileHandle;
		if (audiotype == 1){
			if (mem_size){
				wav_file->moltiplier = 1;
				wav_file->isPlaying = false;
				wav_file->mem_size = (size-(pos+4));
				while (wav_file->mem_size > STREAM_MAX_ALLOC){
					wav_file->mem_size = wav_file->mem_size>>1;
				}
				wav_file->startRead = (pos+4);
				if (raw_enc == 0x11){ // ADPCM Decoding
					u32 buffer_headers_num = (wav_file->mem_size) / wav_file->bytepersample;
					u8* tmp_audiobuf = (u8*)linearAlloc(wav_file->mem_size);
					FS_Read(fileHandle, &bytesRead, pos+4, tmp_audiobuf, wav_file->mem_size);
					wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size-(buffer_headers_num<<2));
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
					FS_Read(fileHandle, &bytesRead, wav_file->startRead, wav_file->audiobuf, wav_file->mem_size);
				}
				wav_file->audiobuf2 = NULL;
				wav_file->size = size;
			}else{
				if (raw_enc == 0x11){ // ADPCM Decoding
					u32 headers_num = (size-(pos+4)) / wav_file->bytepersample;
					u8* tmp_audiobuf = (u8*)linearAlloc(size-(pos+4));
					FS_Read(fileHandle, &bytesRead, pos+4, tmp_audiobuf, size-(pos+4));
					wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4)-(headers_num<<2));
					int z=0,i=0;
					while (i < size){
						wav_file->audiobuf[z] = tmp_audiobuf[i];
						z++;
						i++;
						if ((i % wav_file->bytepersample) == 0) i=i+4;
					}
					tmp_buf = tmp_audiobuf;
					size = size-(pos+4) - (headers_num<<2);
				}else{ // PCM-16 Decoding
					wav_file->audiobuf = (u8*)linearAlloc(size-(pos+4));
					FS_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
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
					wav_file->mem_size = wav_file->mem_size>>1;
				}
				tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
				wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size>>1);
				wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size>>1);
				FS_Read(fileHandle, &bytesRead, wav_file->startRead, tmp_buf, wav_file->mem_size);
				size_tbp = wav_file->mem_size;
			}else{
				tmp_buf = (u8*)linearAlloc((size-(pos+4)));
				size_tbp = size-(pos+4);
				wav_file->startRead = 0;
				wav_file->size = (size_tbp)>>1;
				FS_Read(fileHandle, &bytesRead, pos+4, tmp_buf, size-(pos+4));
				wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))>>1);
				wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))>>1);
			}
			u32 off=0;
			u32 i=0;
			u16 z;
			if (raw_enc == 0x01){ //PCM16 Decoding				
				while (i < size_tbp){
					z=0;
					while (z < (wav_file->bytepersample>>1)){
						wav_file->audiobuf[off+z] = tmp_buf[i+z];
						wav_file->audiobuf2[off+z] = tmp_buf[i+z+(wav_file->bytepersample>>1)];
						z++;
					}
					i=i+wav_file->bytepersample;
					off=off+(wav_file->bytepersample>>1);
				}
			}else if (raw_enc == 0x11){ //ADPCM Decoding
				u32 headers_num = (size_tbp) / wav_file->bytepersample;
				wav_file->audiobuf = (u8*)linearAlloc((size_tbp-(headers_num<<3))>>1);
				wav_file->audiobuf2 = (u8*)linearAlloc((size_tbp-(headers_num<<3))>>1);
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
	if (!mem_size) FS_Close(fileHandle);
	return 1;
}

void streamOGG_DSP(void* arg){
	
	// Fetching Music struct from main thread
	Music* src = (Music*)arg;
	while(1) {	
		
		// Waiting for updateStream event
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		
		// Close the thread if closeStream event received
		if(closeStream){
			closeStream = false;
			threadExit(0);
		}
		
		// Check if the current stream is paused or not
		if (src->isPlaying){
		
			// Check if a free buffer is available
			if (src->wavebuf2 == NULL){
			
				// Check if file reached EOF
				if (src->audio_pointer >= src->size){
				
					// Looping feature (TODO: Seems to not work)
					if (src->streamLoop){
						ov_raw_seek((OggVorbis_File*)src->sourceFile,0);
						src->audio_pointer = 0;
					}
				
					// Check if playback ended
					if (!ndspChnIsPlaying(src->ch)){
						src->isPlaying = false;
						src->tick = (osGetTime()-src->tick);
					}
					
					continue;
				}
				
				// Initializing libogg and vorbisfile
				int eof=0;
				static int current_section;
				OggVorbis_File* vf = (OggVorbis_File*)src->sourceFile;
				
				// Swap audiobuffers
				u8* tmp = src->audiobuf;
				src->audiobuf = src->audiobuf2;
				src->audiobuf2 = tmp;
				
				// Create a new block for DSP service
				u32 bytesRead;
				src->wavebuf2 = (ndspWaveBuf*)calloc(1,sizeof(ndspWaveBuf));
				createDspBlock(src->wavebuf2, src->bytepersample, src->mem_size, false, (u32*)src->audiobuf);
				populatePurgeTable(src, src->wavebuf2);
				ndspChnWaveBufAdd(src->ch, src->wavebuf2);
				src->audio_pointer = src->audio_pointer + src->mem_size;
				
				// Decoding Vorbis audiobuffer
				int i = 0;
				while(!eof){
					long ret=ov_read(vf,(char*)&src->audiobuf[i],2048,0,2,1,&current_section);
					if (ret == 0) eof=1;
					else{
						i = i + ret;
						if ((i >= src->mem_size)) break;
			
					}
				}
			
			}
			
			// Check if a block playback is finished
			u32 curSample = ndspChnGetSamplePos(src->ch);
			if (src->lastCheck > curSample){
			
				// Prepare next block
				src->wavebuf = src->wavebuf2;
				src->wavebuf2 = NULL;
			
			}
			
			// Update sample position tick
			src->lastCheck = curSample;
		
		}
			
	}
}

static int lua_openaiff_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	fileStream* fileHandle = (fileStream*)malloc(sizeof(fileStream));
	if (strncmp("romfs:/",file_tbo,7) == 0){
		fileHandle->isRomfs = true;
		FILE* handle = fopen(file_tbo,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle->handle = (u32)handle;
	}else{
		fileHandle->isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly(&fileHandle->handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "error opening file.");
		#endif
	}
	u32 magic,bytesRead,jump,chunk=0x00000000;
	u16 samplerate;
	u16 audiotype;
	FS_Read(fileHandle, &bytesRead, 8, &magic, 4);
	if (magic == 0x46464941){
		wav *wav_file = (wav*)malloc(sizeof(wav));
		wav_file->ch = 0xDEADBEEF;
		strcpy(wav_file->author,"");
		strcpy(wav_file->title,"");
		wav_file->encoding = CSND_ENCODING_PCM16;
		u64 size;
		wav_file->big_endian = true;
		u32 pos = 12;	
		FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);
		while (chunk != 0x444E5353){
	
			//Chunks detection
			if (chunk == 0x454D414E){ //NAME Chunk
				u32 chunk_size;
				FS_Read(fileHandle, &bytesRead, pos+4, &chunk_size, 4);
				chunk_size = Endian_UInt32_Conversion(chunk_size);
				char* title = (char*)malloc(chunk_size * sizeof(char));
				FS_Read(fileHandle, &bytesRead, pos+8, title, chunk_size);
				strcpy(wav_file->title,title);
				wav_file->title[chunk_size+1] = 0;
				free(title);
			}else if (chunk == 0x4D4D4F43){ //COMM Chunk
				FS_Read(fileHandle, &bytesRead, pos+8, &audiotype, 2);
				audiotype = audiotype>>8;
				FS_Read(fileHandle, &bytesRead, pos+14, &(wav_file->bytepersample), 2);
				wav_file->bytepersample = (wav_file->bytepersample>>11) * audiotype;
				FS_Read(fileHandle, &bytesRead, pos+18, &samplerate, 2);
				samplerate = Endian_UInt16_Conversion(samplerate);
			}
	
			pos=pos+4;
			FS_Read(fileHandle, &bytesRead, pos, &jump, 4);
			pos=pos+4+Endian_UInt32_Conversion(jump);
			FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
		}
		pos=pos+4;
		FS_GetSize(fileHandle, &size);
		wav_file->mem_size = mem_size;
		wav_file->samplerate = samplerate;
		if (audiotype == 1){
			if (mem_size){
				wav_file->moltiplier = 1;
				wav_file->isPlaying = false;
				wav_file->mem_size = (size-(pos+4));
				while (wav_file->mem_size > STREAM_MAX_ALLOC<<1){
					wav_file->mem_size = wav_file->mem_size>>1;
				}
				wav_file->sourceFile = (u32)fileHandle;
				wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size);
				wav_file->startRead = (pos+4);
				FS_Read(fileHandle, &bytesRead, wav_file->startRead, wav_file->audiobuf, wav_file->mem_size);
	
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
				FS_Read(fileHandle, &bytesRead, pos+4, wav_file->audiobuf, size-(pos+4));
	
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
				wav_file->sourceFile = (u32)fileHandle;
				wav_file->isPlaying = false;
				wav_file->startRead = (pos+4);
				wav_file->size = size;
				wav_file->mem_size = (size-(pos+4));
				while (wav_file->mem_size > STREAM_MAX_ALLOC * 10){
					wav_file->mem_size = wav_file->mem_size>>1;
				}
				tmp_buf = (u8*)linearAlloc(wav_file->mem_size);
				wav_file->audiobuf = (u8*)linearAlloc(wav_file->mem_size>>1);
				wav_file->audiobuf2 = (u8*)linearAlloc(wav_file->mem_size>>1);
				FS_Read(fileHandle, &bytesRead, wav_file->startRead, tmp_buf, wav_file->mem_size);
				size_tbp = wav_file->mem_size;
			}else{
				tmp_buf = (u8*)linearAlloc((size-(pos+4)));
				wav_file->audiobuf = (u8*)linearAlloc((size-(pos+4))>>1);
				wav_file->audiobuf2 = (u8*)linearAlloc((size-(pos+4))>>1);
				wav_file->startRead = 0;
				size_tbp = size-(pos+4);
				wav_file->size = size_tbp>>1;
				FS_Read(fileHandle, &bytesRead, pos+4, tmp_buf, size-(pos+4));
			}
			u32 off=0;
			u32 i=0;
			u16 z;
			while (i < size_tbp){
				z=0;
				while (z < (wav_file->bytepersample>>1)){
					wav_file->audiobuf[off+z] = tmp_buf[i+(wav_file->bytepersample>>1)-z-1];
					wav_file->audiobuf2[off+z] = tmp_buf[i+(wav_file->bytepersample)-z-1];
					z++;
				}
				i=i+wav_file->bytepersample;
				off=off+(wav_file->bytepersample>>1);
			}
		}
		wav_file->magic = 0x4C534E44;
		lua_pushinteger(L,(u32)wav_file);
	}
	if (!mem_size){
		FS_Close(fileHandle);
		free(fileHandle);
	}
	return 1;
}

void streamWAV_DSP(void* arg){

	// Fetching Music struct from main thread
	Music* src = (Music*)arg;
	while(1) {
	
		// Waiting for updateStream event
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		
		// Close the thread if closeStream event received
		if(closeStream){
			closeStream = false;
			threadExit(0);
		}
		
		// Check if the current stream is paused or not
		if (src->isPlaying){
		
			// Check if a free buffer is available
			if (src->wavebuf2 == NULL){
			
				// Check if file reached EOF
				if (src->audio_pointer >= src->size){
					
					// Looping feature
					if (src->streamLoop) src->audio_pointer = src->startRead;
					
					// Check if playback ended
					if (!ndspChnIsPlaying(src->ch)){
						src->isPlaying = false;
						src->tick = (osGetTime()-src->tick);
					}
					
					continue;
				}
				
				// Swap audiobuffers
				u8* tmp = src->audiobuf;
				src->audiobuf = src->audiobuf2;
				src->audiobuf2 = tmp;
				
				// Create a new block for DSP service
				u32 bytesRead;
				src->wavebuf2 = (ndspWaveBuf*)calloc(1,sizeof(ndspWaveBuf));
				createDspBlock(src->wavebuf2, src->bytepersample, src->mem_size, false, (u32*)src->audiobuf);
				populatePurgeTable(src, src->wavebuf2);
				ndspChnWaveBufAdd(src->ch, src->wavebuf2);
				FS_Read((fileStream*)src->sourceFile, &bytesRead, src->audio_pointer, src->audiobuf, src->mem_size);
				src->audio_pointer = src->audio_pointer + src->mem_size;

				// Changing endianess if Big Endian
				if (src->big_endian){
					u64 i = 0;
					while (i < src->mem_size){
						u8 tmp = src->audiobuf[i];
						src->audiobuf[i] = src->audiobuf[i+1];
						src->audiobuf[i+1] = tmp;
						i=i+2;	
					}
				}
			
			}
			
			// Check if a block playback is finished
			u32 curSample = ndspChnGetSamplePos(src->ch);
			if (src->lastCheck > curSample){
			
				// Prepare next block
				src->wavebuf = src->wavebuf2;
				src->wavebuf2 = NULL;
			
			}
			
			// Update sample position tick
			src->lastCheck = curSample;
		
		}
	}		
}

static int lua_openogg(lua_State *L) 
{	
	
	// Init function
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1); //Filename
	
	// Check for streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Init stdio
	char myFile[512];
	if (strncmp("romfs:/",file_tbo,7) == 0) strcpy(myFile,file_tbo);
	else{
		strcpy(myFile,"sdmc:");
		strcat(myFile,file_tbo);
	}
	sdmcInit();
	
	// Init libogg and vorbisfile
	int eof=0;
	OggVorbis_File* vf = (OggVorbis_File*)malloc(sizeof(OggVorbis_File));
	static int current_section;
	
	// Opening a valid OGG music file
	FILE* fp = fopen(myFile,"r");
	if(ov_open(fp, vf, NULL, 0) != 0)
	{
		fclose(fp);
		return luaL_error(L, "corrupt OGG file.");
	}
	
	// Allocating music info
	Music* songFile = (Music*)malloc(sizeof(Music));
	songFile->ch = 0xDEADBEEF;
	songFile->magic = 0x4C534E44;
	vorbis_info* my_info = ov_info(vf,-1);
	songFile->samplerate = my_info->rate;
	songFile->big_endian = false;
	songFile->encoding = CSND_ENCODING_VORBIS;
	songFile->size = ov_time_total(vf,-1) * (my_info->rate<<1); // TODO: Find a more accurated calculation method
	songFile->startRead = 0;
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->wavebuf = NULL;
	songFile->wavebuf2 = NULL;
	songFile->blocks = NULL;
	
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
			fread(&songFile->author,info_size - 7,1,fp);
			songFile->author[info_size - 7] = 0;
		}else if ((strcmp((const char*)&info_type,"TITLE=") == 0) || (strcmp((const char*)&info_type,"title=") == 0)){
			fread(&songFile->title,info_size - 6,1,fp);
			songFile->title[info_size - 6] = 0;
		}
		fseek(fp,offset+info_size, SEEK_SET);
		fread(&info_size,4,1,fp);
	}
	fseek(fp, restore, SEEK_SET); // Restore libOgg offset
	
	
	
	songFile->isPlaying = false;
	songFile->bytepersample = 2;
	songFile->audiotype = my_info->channels;
	
	// Decoding OGG buffer
	i = 0;
	if (mem_size){
		songFile->mem_size = songFile->size;
		u32 BUFFER_SIZE = STREAM_MAX_ALLOC;
		if (songFile->audiotype == 2) BUFFER_SIZE = BUFFER_SIZE * 10; // Temporary patch for stereo files
		while (songFile->mem_size > BUFFER_SIZE){
			if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
			songFile->mem_size = songFile->mem_size>>1;
		}
		if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
		songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
		songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
		songFile->audio_pointer = songFile->mem_size;
	}else{
		songFile->audiobuf = (u8*)linearAlloc(songFile->size);
		songFile->mem_size = 0;
		songFile->audiobuf2 = NULL;
	}		
	int offs = 0;
	while(!eof){
		long ret=ov_read(vf,(char*)&songFile->audiobuf[offs],2048,0,2,1,&current_section);
		if (ret == 0) eof=1;
		else if (ret < 0) {
			
			// Error handling
			if(ret==OV_EBADLINK) return luaL_error(L, "corrupt bitstream section.");
				
		} else {
			
			// Copying decoded block to PCM16 audiobuffer
			offs = offs + ret;
			if ((mem_size) && (offs >= songFile->mem_size)) break;
			
		}
	}
	
	if (!mem_size){
	
		// Deallocate OGG decoder resources and close file if not streaming
		ov_clear(vf);
		sdmcExit();
		
	}else songFile->sourceFile = (u32)vf;
	
	// Push wav struct offset to LUA stack
	lua_pushinteger(L,(u32)songFile);
	
	return 1;
}

static int lua_openwav(lua_State *L){

	// Init function
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	
	// Check for Streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Open file
	fileStream* fileHandle = (fileStream*)malloc(sizeof(fileStream));
	if (strncmp("romfs:/",file_tbo,7) == 0){
		fileHandle->isRomfs = true;
		FILE* handle = fopen(file_tbo,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle->handle = (u32)handle;
	}else{
		fileHandle->isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly( &fileHandle->handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "error opening file.");
		#endif
	}
	
	// Check for magic
	u32 magic,samplerate,bytesRead,jump,chunk=0x00000000;
	u16 audiotype;
	FS_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x46464952){
	
		// Init wav struct
		Music* songFile = (Music*)malloc(sizeof(Music));
		songFile->ch = 0xDEADBEEF;
		strcpy(songFile->author,"");
		strcpy(songFile->title,"");
		u64 size;
		songFile->big_endian = false;
		
		// Metadatas extraction
		u32 pos = 16;	
		while (chunk != 0x61746164){
			FS_Read(fileHandle, &bytesRead, pos, &jump, 4);
			pos=pos+4+jump;
			FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
			pos=pos+4;
	
			//Chunk LIST detection
			if (chunk == 0x5453494C){
				u32 chunk_size;
				u32 subchunk;
				u32 subchunk_size;
				u32 sub_pos = pos+4;
				FS_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
				if (subchunk == 0x4F464E49){
					sub_pos = sub_pos+4;
					FS_Read(fileHandle, &bytesRead, pos, &chunk_size, 4);
					while (sub_pos < (chunk_size + pos + 4)){
						FS_Read(fileHandle, &bytesRead, sub_pos, &subchunk, 4);
						FS_Read(fileHandle, &bytesRead, sub_pos+4, &subchunk_size, 4);
						if (subchunk == 0x54524149){
							char* author = (char*)malloc(subchunk_size);
							FS_Read(fileHandle, &bytesRead, sub_pos+8, author, subchunk_size);
							strcpy(songFile->author,author);
							songFile->author[subchunk_size+1] = 0;
							free(author);
						}else if (subchunk == 0x4D414E49){
							char* title = (char*)malloc(subchunk_size);
							FS_Read(fileHandle, &bytesRead, sub_pos+8, title, subchunk_size);
							strcpy(songFile->title,title);
							songFile->title[subchunk_size+1] = 0;
							free(title);
						}
						sub_pos = sub_pos + 8 + subchunk_size;
						u8 checksum;
						FS_Read(fileHandle, &bytesRead, sub_pos, &checksum, 1); //Prevent errors switching subchunks
						if (checksum == 0) sub_pos++;
					}
				}
			}
	
		}
		
		// Music info extraction
		FS_GetSize(fileHandle, &size);
		FS_Read(fileHandle, &bytesRead, 22, &audiotype, 2);
		FS_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
		u16 raw_enc;
		FS_Read(fileHandle, &bytesRead, 20, &raw_enc, 2);
		FS_Read(fileHandle, &bytesRead, 32, &(songFile->bytepersample), 2);
		if (raw_enc == 0x01) songFile->encoding = CSND_ENCODING_PCM16;
		else if (raw_enc == 0x11) songFile->encoding = CSND_ENCODING_ADPCM;
		songFile->samplerate = samplerate;
		songFile->audiotype = audiotype;
		songFile->wavebuf = NULL;
		songFile->wavebuf2 = NULL;
		songFile->blocks = NULL;
		
		// Audiobuffer extraction
		songFile->isPlaying = false;
		songFile->sourceFile = (u32)fileHandle;
		if (mem_size){
			songFile->mem_size = size-(pos+4);
			while (songFile->mem_size > STREAM_MAX_ALLOC){
				if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
				songFile->mem_size = songFile->mem_size>>1;
			}
			if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
			songFile->startRead = pos+4;
			songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
			songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
			FS_Read(fileHandle, &bytesRead, songFile->startRead, songFile->audiobuf, songFile->mem_size);
			songFile->audio_pointer = songFile->startRead + songFile->mem_size;
			songFile->size = size;
		}else{
			songFile->audiobuf = (u8*)linearAlloc(size-(pos+4));
			FS_Read(fileHandle, &bytesRead, pos+4, songFile->audiobuf, size-(pos+4));
			size = size-(pos+4);
			songFile->size = size;
			songFile->startRead = 0;
			songFile->mem_size = 0;
			songFile->audiobuf2 = NULL;
		}
		
		// Pushing memory block to LUA stack
		songFile->magic = 0x4C534E44;
		lua_pushinteger(L,(u32)songFile);
		
	}
	
	// Closing file if no streaming feature usage
	if (!mem_size){
		FS_Close(fileHandle);
		free(fileHandle);
	}
	
	return 1;
}

static int lua_openaiff(lua_State *L){
	
	// Init function
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	
	// Check for streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Opening file
	fileStream* fileHandle = (fileStream*)malloc(sizeof(fileStream));
	if (strncmp("romfs:/",file_tbo,7) == 0){
		fileHandle->isRomfs = true;
		FILE* handle = fopen(file_tbo,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle->handle = (u32)handle;
	}else{
		fileHandle->isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly( &fileHandle->handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
	}
	
	// Init wav struct
	u32 magic,bytesRead,jump,chunk=0x00000000;
	u16 samplerate;
	u16 audiotype;
	FS_Read(fileHandle, &bytesRead, 8, &magic, 4);
	if (magic != 0x46464941) return luaL_error(L, "corrupt AIFF file");
	Music* songFile = (Music*)malloc(sizeof(Music));
	songFile->ch = 0xDEADBEEF;
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->encoding = CSND_ENCODING_PCM16;
	u64 size;
	songFile->big_endian = true;
	u32 pos = 12;	
	FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);
	
	//Extracting Metadatas
	while (chunk != 0x444E5353){
	
		//Chunks detection
		if (chunk == 0x454D414E){ //NAME Chunk
			u32 chunk_size;
			FS_Read(fileHandle, &bytesRead, pos+4, &chunk_size, 4);
			chunk_size = Endian_UInt32_Conversion(chunk_size);
			char* title = (char*)malloc(chunk_size);
			FS_Read(fileHandle, &bytesRead, pos+8, title, chunk_size);
			strcpy(songFile->title,title);
			songFile->title[chunk_size+1] = 0;
			free(title);
		}else if (chunk == 0x4D4D4F43){ //COMM Chunk
			FS_Read(fileHandle, &bytesRead, pos+8, &audiotype, 2);
			audiotype = audiotype>>8;
			FS_Read(fileHandle, &bytesRead, pos+14, &(songFile->bytepersample), 2);
			songFile->bytepersample = (songFile->bytepersample>>11) * audiotype;
			FS_Read(fileHandle, &bytesRead, pos+18, &samplerate, 2);
			samplerate = Endian_UInt16_Conversion(samplerate);
		}	
	
		pos=pos+4;
		FS_Read(fileHandle, &bytesRead, pos, &jump, 4);
		pos=pos+4+Endian_UInt32_Conversion(jump);
		FS_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
	}
	
	//Extracting music info
	pos=pos+4;
	FS_GetSize(fileHandle, &size);
	songFile->mem_size = 0;
	songFile->samplerate = samplerate;
	songFile->audiotype = audiotype;
	songFile->audiobuf2 = NULL;
	songFile->wavebuf = NULL;
	songFile->wavebuf2 = NULL;
	songFile->isPlaying = false;
	songFile->size = size-(pos+4);
	songFile->startRead = 0;
	songFile->blocks = NULL;
	
	// Extracting audiobuffer
	int stbp = 0;
	if (mem_size){
		songFile->mem_size = (size-(pos+4));
		while (songFile->mem_size > STREAM_MAX_ALLOC){
			if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
			songFile->mem_size = songFile->mem_size>>1;
		}
		if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
		songFile->sourceFile = (u32)fileHandle;
		songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
		songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
		songFile->startRead = (pos+4);
		FS_Read(fileHandle, &bytesRead, songFile->startRead, songFile->audiobuf, songFile->mem_size);	
		songFile->audio_pointer = songFile->startRead + songFile->mem_size;
		stbp = songFile->mem_size;
	}else{
		songFile->audiobuf = (u8*)linearAlloc(songFile->size);
		songFile->audiobuf2 = NULL;
		FS_Read(fileHandle, &bytesRead, pos+4, songFile->audiobuf, songFile->size);	
		stbp = songFile->size;
	}
	
	// Changing endianess
	u64 i = 0;
	while (i < stbp){
		u8 tmp = songFile->audiobuf[i];
		songFile->audiobuf[i] = songFile->audiobuf[i+1];
		songFile->audiobuf[i+1] = tmp;
		i=i+2;	
	}
	
	// Pushing memory block to LUA stack
	songFile->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)songFile);
	
	// Closing file if streaming feature used
	if (!mem_size){
		FS_Close(fileHandle);
		free(fileHandle);
	}
	
	return 1;
}

static int lua_soundinit(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	if (!isCSND){
		if (csndAccess){
			csndInit();
			isCSND = true;
		}else if (ndspInit() == 0){
			isCSND = true;
			ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		}else return luaL_error(L, "cannot initialize Sound module.");
	}
	return 0;
}

static int lua_play(lua_State *L){
	
	// Init Function
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2 && argc != 3)	return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	closeStream = false;
	bool loop = lua_toboolean(L, 2);
	int interp = NDSP_INTERP_LINEAR;
	if (argc == 3) interp = luaL_checkinteger(L, 3);
	
	// Selecting a free channel
	int ch = src->ch;
	if (ch == 0xDEADBEEF){
		ch = 0;
		while (audioChannels[ch]){
			ch++;
			if (ch >= 24) return luaL_error(L, "audio device is busy");
		}
		audioChannels[ch] = true;
	}
	src->ch = ch;
	
	// Non native Audiocodecs support
	ThreadFunc streamFunction = streamWAV_DSP;
	if (src->encoding == CSND_ENCODING_VORBIS) streamFunction = streamOGG_DSP;
	
	// Initializing DSP channel
	int raw_format;
	if (src->encoding == CSND_ENCODING_ADPCM) raw_format = NDSP_FORMAT_MONO_ADPCM;
	else if (src->audiotype == 1) raw_format = NDSP_FORMAT_MONO_PCM16;
	else raw_format = NDSP_FORMAT_STEREO_PCM16;
	ndspChnReset(ch);
	ndspChnWaveBufClear(ch);
	ndspChnSetInterp(ch, (ndspInterpType)interp);
	ndspChnSetRate(ch, float(src->samplerate));
	ndspChnSetFormat(ch, raw_format);
	ndspWaveBuf* waveBuf = (ndspWaveBuf*)calloc(1, sizeof(ndspWaveBuf));
	if (src->mem_size > 0) createDspBlock(waveBuf, src->bytepersample, src->mem_size, false, (u32*)src->audiobuf);
	else createDspBlock(waveBuf, src->bytepersample, src->size, loop, (u32*)src->audiobuf);
	populatePurgeTable(src, waveBuf);
	ndspChnWaveBufAdd(ch, waveBuf);
	src->tick = osGetTime();
	src->wavebuf = waveBuf;
	src->ch = ch;
	src->isPlaying = true;
	
	// Initializing streaming feature if used
	if (src->mem_size > 0){
		src->lastCheck = ndspChnGetSamplePos(ch);
		src->streamLoop = loop;
		svcCreateEvent(&updateStream,0);
		svcSignalEvent(updateStream);
		threadCreate(streamFunction, src, 32768, 0x18, 0, true);
	}
	
	return 0;
}

static int lua_play_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 2) && (argc != 3))	return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	closeStream = false;
	bool loop = lua_toboolean(L, 2);
	
	// Selectable interpolation type
	int interp = SOUND_LINEAR_INTERP;
	if (argc == 3) interp = luaL_checkinteger(L, 3);
	if (interp == NDSP_INTERP_NONE) interp = 0xDEADBEEF;
	else interp = SOUND_LINEAR_INTERP;
	
	// Selecting free channels
	int ch = src->ch;
	int ch2 = src->ch2;
	if (src->ch == 0xDEADBEEF){
		ch = 0x08;
		while (audioChannels[ch]){
			ch++;
			if (ch > 32) return luaL_error(L, "audio device is busy");
		}
		audioChannels[ch] = true;
		ch2 = ch + 1;
		if (src->audiobuf2 != NULL){
			while (audioChannels[ch2]){
				ch2++;
				if (ch2 > 32) return luaL_error(L, "audio device is busy");
			}
			audioChannels[ch2] = true;
		}
	}
	src->ch = ch;
	src->ch2 = ch2;
	
	bool non_native_encode = false;
	ThreadFunc streamFunction = streamWAV_CSND;
	u8 tmp_encode;
	if (argc == 4) ch2 = luaL_checkinteger(L, 4);
	if (src->encoding == CSND_ENCODING_VORBIS){
		streamFunction = streamOGG_CSND;
		tmp_encode = src->encoding;
		src->encoding = CSND_ENCODING_PCM16;
		non_native_encode = true;
	}else if (src->encoding == CSND_ENCODING_MPEG){
		streamFunction = streamDEC_CSND;
		AudioDecoder* audiodec = (AudioDecoder*)src->misc;
		audiodec->SetLooping(loop);
		tmp_encode = src->encoding;
		src->encoding = CSND_ENCODING_PCM16;
		non_native_encode = true;
	}
	if (src->audiobuf2 == NULL){
		if (src->mem_size > 0){
			src->streamLoop = loop;
			svcCreateEvent(&updateStream,0);
			svcSignalEvent(updateStream);
			threadCreate(streamFunction, src, 32768, 0x18, 1, true);
			if (interp != 0xDEADBEEF) My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size, 1.0, 2.0);
			else My_CSND_playsound(ch, SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size, 1.0, 2.0);
		}else{
			u32 looping = loop ? SOUND_REPEAT : SOUND_ONE_SHOT;
			if (interp != 0xDEADBEEF) My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, 2.0);
			else My_CSND_playsound(ch, SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, 2.0);
		}
		src->tick = osGetTime();
		CSND_SetPlayState(src->ch, 1);
		CSND_UpdateInfo(0);
	}else{
		if (src->mem_size > 0){
			src->streamLoop = loop;
			svcCreateEvent(&updateStream,0);
			svcSignalEvent(updateStream);
			threadCreate(streamFunction, src, 32768, 0x18, 1, true);
			if (interp != 0xDEADBEEF){
				My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size>>1, 1.0, -1.0);
				My_CSND_playsound(ch2, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->mem_size>>1, 1.0, 1.0);
			}else{
				My_CSND_playsound(ch, SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->mem_size>>1, 1.0, -1.0);
				My_CSND_playsound(ch2, SOUND_FORMAT(src->encoding) | SOUND_REPEAT, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->mem_size>>1, 1.0, 1.0);
			}
		}else{
			u32 looping = loop ? SOUND_REPEAT : SOUND_ONE_SHOT;
			if (interp != 0xDEADBEEF){
				My_CSND_playsound(ch, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, -1.0);
				My_CSND_playsound(ch2, SOUND_LINEAR_INTERP | SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->size, 1.0, 1.0);
			}else{
				My_CSND_playsound(ch, SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf, (u32*)(src->audiobuf), src->size, 1.0, -1.0);
				My_CSND_playsound(ch2, SOUND_FORMAT(src->encoding) | looping, src->samplerate, (u32*)src->audiobuf2, (u32*)(src->audiobuf2), src->size, 1.0, 1.0);
			}
		}
		src->tick = osGetTime();
		CSND_SetPlayState(ch, 1);
		CSND_SetPlayState(ch2, 1);
		CSND_UpdateInfo(0);
	}
	if (non_native_encode){
		src->encoding = tmp_encode;
	}
	src->isPlaying = true;
	return 0;
}


static int lua_closesong(lua_State *L){
	
	// Init function
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	// Checking if streaming feature is used
	if (src->mem_size > 0 && src->ch != 0xDEADBEEF){
		
		// Closing stream thread
		closeStream = true;
		svcSignalEvent(updateStream);
		while (closeStream){} // Wait for thread exiting...
		
		// Closing opened file
		if (src->encoding == CSND_ENCODING_VORBIS){
			ov_clear((OggVorbis_File*)src->sourceFile);
			sdmcExit();
		}else{
			FS_Close((fileStream*)src->sourceFile);
			free((fileStream*)src->sourceFile);
		}
		svcCloseHandle(updateStream);
		
	}
	
	// Freeing DSP queue and audio-device channels
	if (src->ch != 0xDEADBEEF){
		ndspChnWaveBufClear(src->ch);
		audioChannels[src->ch] = false;
	}
	
	// Deallocating audio buffers and freeing structs
	purgeTable(src->blocks);
	linearFree(src->audiobuf);
	if (src->audiobuf2 != NULL) linearFree(src->audiobuf2);
	if (tmp_buf != NULL) linearFree(tmp_buf);
	free(src);
	
	return 0;
}

static int lua_close_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	// Checking if audio playback started at least once
	if (src->ch != 0xDEADBEEF){
	
		// Freeing audio-device channels
		audioChannels[src->ch] = false;
		if (src->audiobuf2 != NULL) audioChannels[src->ch2] = false;
		
		// Stopping audio playback if still playing
		CSND_SetPlayState(src->ch, 0);
		if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
		CSND_UpdateInfo(0);
		
		// Checking if streaming feature is used
		if (src->mem_size > 0){
		
			// Closing stream thread
			closeStream = true;
			svcSignalEvent(updateStream);
			while (closeStream){} // Wait for thread exiting...
			
			// Closing opened file
			if (src->encoding == CSND_ENCODING_VORBIS){
				ov_clear((OggVorbis_File*)src->sourceFile);
				sdmcExit();
			}else if (src->encoding == CSND_ENCODING_MPEG){
				free(src->misc);
				sdmcExit();
			}else{
				FS_Close((fileStream*)src->sourceFile);
				free((fileStream*)src->sourceFile);
			}
			svcCloseHandle(updateStream);
			
		}
		
	}
	
	// Deallocating audio buffers and freeing structs
	linearFree(src->audiobuf);
	if (src->audiobuf2 != NULL) linearFree(src->audiobuf2);
	if (tmp_buf != NULL) linearFree(tmp_buf);
	free(src);
	
	return 0;
}

static int lua_pause_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	CSND_SetPlayState(src->ch, 0);
	if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 0);
	CSND_UpdateInfo(0);
	src->isPlaying = false;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_resume_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	CSND_SetPlayState(src->ch, 1);
	if (src->audiobuf2 != NULL) CSND_SetPlayState(src->ch2, 1);
	CSND_UpdateInfo(0);
	src->isPlaying = true;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_pause(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->isPlaying){
		ndspChnSetPaused(src->ch, true);
		src->isPlaying = false;
		src->tick = (osGetTime()-src->tick);
	}
	return 0;
}

static int lua_resume(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (!src->isPlaying){
		ndspChnSetPaused(src->ch, false);
		src->lastCheck = 0;
		src->isPlaying = true;
		src->tick = (osGetTime()-src->tick);
	}
	return 0;
}

static int lua_wisPlaying(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (csndAccess) lua_pushboolean(L, ((wav*)src)->isPlaying);
	else lua_pushboolean(L, src->isPlaying);
	return 1;
}

static int lua_soundend(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	if (isCSND){
		if (csndAccess) csndExit();
		else ndspExit();
		isCSND = false;
	}
	return 0;
}

static int lua_save(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->mem_size != 0) return luaL_error(L, "cannot save a stream");
	const char* file = luaL_checkstring(L, 2);
	Handle fileHandle;
	u32 bytesWritten;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FS_Path filePath=fsMakePath(PATH_ASCII, file);
	FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);
	u32 four_bytes;
	u16 two_bytes;
	FSFILE_Write(fileHandle, &bytesWritten, 0, "RIFF", 4, FS_WRITE_FLUSH);
	four_bytes = src->size + 36;
	FSFILE_Write(fileHandle, &bytesWritten, 4, &four_bytes, 8, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 8, "WAVEfmt ", 8, FS_WRITE_FLUSH);
	four_bytes = 0x10;
	FSFILE_Write(fileHandle, &bytesWritten, 16, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = 0x01;
	if (src->encoding == CSND_ENCODING_ADPCM) two_bytes = 0x11;
	FSFILE_Write(fileHandle, &bytesWritten, 20, &two_bytes, 2, FS_WRITE_FLUSH);
	two_bytes = src->audiotype;
	FSFILE_Write(fileHandle, &bytesWritten, 22, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 24, &src->samplerate, 4, FS_WRITE_FLUSH);
	four_bytes = src->samplerate * src->bytepersample;
	FSFILE_Write(fileHandle, &bytesWritten, 28, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = 0x10;
	FSFILE_Write(fileHandle, &bytesWritten, 32, &src->bytepersample, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 34, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 36, "data", 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 40, &src->size, 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 44, src->audiobuf, src->size, FS_WRITE_FLUSH);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	return 0;
}

static int lua_save_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->mem_size != 0) return luaL_error(L, "cannot save a stream");
	if (src->audiobuf2 != NULL) return luaL_error(L, "csnd:SND system cannot save stereo files");
	const char* file = luaL_checkstring(L, 2);
	Handle fileHandle;
	u32 bytesWritten;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FS_Path filePath=fsMakePath(PATH_ASCII, file);
	FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);
	u32 four_bytes;
	u16 two_bytes;
	FSFILE_Write(fileHandle, &bytesWritten, 0, "RIFF", 4, FS_WRITE_FLUSH);
	four_bytes = src->size + 36;
	FSFILE_Write(fileHandle, &bytesWritten, 4, &four_bytes, 8, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 8, "WAVEfmt ", 8, FS_WRITE_FLUSH);
	four_bytes = 0x10;
	FSFILE_Write(fileHandle, &bytesWritten, 16, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = 0x01;
	if (src->encoding == CSND_ENCODING_ADPCM) two_bytes = 0x11;
	FSFILE_Write(fileHandle, &bytesWritten, 20, &two_bytes, 2, FS_WRITE_FLUSH);
	two_bytes = 1;
	FSFILE_Write(fileHandle, &bytesWritten, 22, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 24, &src->samplerate, 4, FS_WRITE_FLUSH);
	four_bytes = src->samplerate * src->bytepersample;
	FSFILE_Write(fileHandle, &bytesWritten, 28, &four_bytes, 4, FS_WRITE_FLUSH);
	two_bytes = 0x10;
	FSFILE_Write(fileHandle, &bytesWritten, 32, &src->bytepersample, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 34, &two_bytes, 2, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 36, "data", 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 40, &src->size, 4, FS_WRITE_FLUSH);
	FSFILE_Write(fileHandle, &bytesWritten, 44, src->audiobuf, src->size, FS_WRITE_FLUSH);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	return 0;
}

static int lua_getSrate(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (csndAccess) lua_pushinteger(L, ((wav*)src)->samplerate);
	else lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_getTime(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->isPlaying) lua_pushinteger(L, (osGetTime() - src->tick) / 1000);
	else lua_pushinteger(L, src->tick / 1000);
	return 1;
}

static int lua_getTime_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if ((src->encoding == CSND_ENCODING_ADPCM) && (src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,(src->size - src->startRead) / (src->samplerate>>1));
	else if (src->encoding == CSND_ENCODING_VORBIS) lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample) * src->samplerate * src->audiotype));
	else if ((src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample>>1) * src->samplerate));
	else lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample) * src->samplerate));
	return 1;
}

static int lua_getTotalTime_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	wav* src = (wav*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if ((src->encoding == CSND_ENCODING_ADPCM) && (src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,(src->size - src->startRead) / (src->samplerate>>1));
	else if (src->encoding == CSND_ENCODING_VORBIS && src->bytepersample > 1) lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample) * (src->samplerate<<1)));
	else if ((src->audiobuf2 != NULL) && (src->mem_size == 0)) lua_pushinteger(L,((src->size) - src->startRead) / ((src->bytepersample>>1) * src->samplerate));
	else lua_pushinteger(L,(src->size - src->startRead) / ((src->bytepersample) * src->samplerate));
	return 1;
}


static int lua_getTitle(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (csndAccess) lua_pushstring(L, ((wav*)src)->title);
	else lua_pushstring(L, src->title);
	return 1;
}

static int lua_getAuthor(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (csndAccess) lua_pushstring(L, ((wav*)src)->author);
	else lua_pushstring(L, src->author);
	return 1;
}

static int lua_getType(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->audiotype);
	return 1;
}

static int lua_getType_old(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	svcSignalEvent(updateStream);
	return 0;
}

static int lua_service(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	char srv[9];
	if (csndAccess) sprintf(srv,"csnd:SND");
	else sprintf(srv,"dsp::DSP");
	lua_pushstring(L, srv);
	return 1;
}

//Register our Sound Functions
static const luaL_Reg Sound_DSP_functions[] = {
	{"openOgg",           lua_openogg},
	{"openWav",           lua_openwav},
	{"openAiff",          lua_openaiff},	
	{"close",             lua_closesong},
	{"play",              lua_play},
	{"init",              lua_soundinit},
	{"term",              lua_soundend},
	{"pause",             lua_pause},
	{"getSrate",          lua_getSrate},
	{"getTime",           lua_getTime},
	{"getTitle",          lua_getTitle},
	{"getAuthor",         lua_getAuthor},
	{"getType",           lua_getType},  
	{"getTotalTime",      lua_getTotalTime},
	{"resume",            lua_resume},
	{"isPlaying",         lua_wisPlaying},
	{"updateStream",      lua_updatestream},
	{"saveWav",           lua_save},
	{"getService",        lua_service},
	{0, 0}
};

static const luaL_Reg Sound_CSND_functions[] = {
	{"openMp3",           lua_openmp3_old},
	{"openOgg",           lua_openogg_old},
	{"openWav",           lua_openwav_old},
	{"openAiff",          lua_openaiff_old},	
	{"close",             lua_close_old},
	{"play",              lua_play_old},
	{"init",              lua_soundinit},
	{"term",              lua_soundend},
	{"pause",             lua_pause_old},
	{"getSrate",          lua_getSrate},
	{"getTime",           lua_getTime_old},
	{"getTitle",          lua_getTitle},
	{"getAuthor",         lua_getAuthor},
	{"getType",           lua_getType_old},  
	{"getTotalTime",      lua_getTotalTime_old},
	{"resume",            lua_resume_old},
	{"isPlaying",         lua_wisPlaying},
	{"updateStream",      lua_updatestream},
	{"saveWav",           lua_save_old},
	{"getService",        lua_service},
	{0, 0}
};

void luaSound_init(lua_State *L) {
	lua_newtable(L);
	if (csndAccess) luaL_setfuncs(L, Sound_CSND_functions, 0);
	else luaL_setfuncs(L, Sound_DSP_functions, 0);
	lua_setglobal(L, "Sound");
	int LINEAR_INTERP = NDSP_INTERP_LINEAR;
	int POLYPHASE_INTERP = NDSP_INTERP_POLYPHASE;
	int NO_INTERP = NDSP_INTERP_NONE;
	VariableRegister(L, LINEAR_INTERP);
	VariableRegister(L, POLYPHASE_INTERP);
	VariableRegister(L, NO_INTERP);
}