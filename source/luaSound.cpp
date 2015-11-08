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

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define STREAM_MAX_ALLOC 524288

extern bool audioChannels[24];
extern bool isNinjhax2;
volatile bool closeStream = false;
Handle updateStream;
Handle streamThread;
char pcmout[2048];
extern u8* tmp_buf;

void streamOGG(void* arg){ //TODO: Solve looping sound issues
	Music* src = (Music*)arg;
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
					CSND_UpdateInfo(0);
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

	// Fetching wav struct from main thread
	Music* src = (Music*)arg;
	while(1) {
	
		// Waiting for updateStream event
		svcWaitSynchronization(updateStream, U64_MAX);
		svcClearEvent(updateStream);
		
		// Close the thread if closeStream event received
		if(closeStream){
			closeStream = false;
			svcExitThread();
		}
		
		// Check if the current stream is paused or not
		if (src->isPlaying){
		
			// Check if a free buffer is available
			if (src->wavebuf2 == NULL){
			
				// Check if file reached EOF
				if (src->audio_pointer >= src->size) continue;
				
				// Swap audiobuffers
				u8* tmp = src->audiobuf;
				src->audiobuf = src->audiobuf2;
				src->audiobuf2 = tmp;
				
				// Create a new block for DSP service
				u32 bytesRead;
				src->wavebuf2 = (ndspWaveBuf*)calloc(1,sizeof(ndspWaveBuf));
				createDspBlock(src->wavebuf2, src->bytepersample, src->mem_size, 0, (u32*)src->audiobuf);
				populatePurgeTable(src, src->wavebuf2);
				ndspChnWaveBufAdd(src->ch, src->wavebuf2);
				FSFILE_Read(src->sourceFile, &bytesRead, src->audio_pointer, src->audiobuf, src->mem_size);
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
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1); //Filename
	
	// Check for streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Init stdio
	char myFile[512];
	strcpy(myFile,"sdmc:");
	strcat(myFile,file_tbo);
	sdmcInit();
	
	// Init libogg and vorbisfile
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
	Music* songFile = (Music*)malloc(sizeof(Music));
	songFile->magic = 0x4C534E44;
	vorbis_info* my_info = ov_info(vf,-1);
	songFile->samplerate = my_info->rate;
	songFile->big_endian = false;
	songFile->encoding = CSND_ENCODING_VORBIS;
	songFile->size = ov_time_total(vf,-1) * 2 * my_info->rate;
	songFile->startRead = 0;
	songFile->total_packages_size = 0;
	songFile->loop_index = 1;
	songFile->package_size = 0;
	songFile->audio_pointer = 0;
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->wavebuf = NULL;
	songFile->wavebuf2 = NULL;
	
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
		songFile->moltiplier = 1;
		songFile->mem_size = songFile->size / 2;
		while (songFile->mem_size > STREAM_MAX_ALLOC){
			songFile->mem_size = songFile->mem_size / 2;
		}
		songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
		songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
	}else{
		songFile->audiobuf = (u8*)linearAlloc(songFile->size);
		songFile->mem_size = 0;
	}
		
	songFile->audiobuf2 = NULL;
	while(!eof){
		long ret=ov_read(vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
		if (ret == 0) eof=1;
		else if (ret < 0) {
			
			// Error handling
			if(ret==OV_EBADLINK) return luaL_error(L, "corrupt bitstream section.");
				
		} else {
			
			// Copying decoded block to PCM16 audiobuffer
			memcpy(&songFile->audiobuf[i],pcmout,ret);
			i = i + ret;
			if ((mem_size) && (i >= songFile->mem_size)) break;
			
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

static int lua_openwav(lua_State *L)
{

	// Init function
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	
	// Check for Streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Open file
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	
	// Check for magic
	u32 magic,samplerate,bytesRead,jump,chunk=0x00000000;
	u16 audiotype;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic == 0x46464952){
	
		// Init wav struct
		Music* songFile = (Music*)malloc(sizeof(Music));
		strcpy(songFile->author,"");
		strcpy(songFile->title,"");
		u64 size;
		songFile->big_endian = false;
		
		// Metadatas extraction
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
							strcpy(songFile->author,author);
							songFile->author[subchunk_size+1] = 0;
							free(author);
						}else if (subchunk == 0x4D414E49){
							char* title = (char*)malloc(subchunk_size * sizeof(char));
							FSFILE_Read(fileHandle, &bytesRead, sub_pos+8, title, subchunk_size);
							strcpy(songFile->title,title);
							songFile->title[subchunk_size+1] = 0;
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
		
		// Music info extraction
		FSFILE_GetSize(fileHandle, &size);
		FSFILE_Read(fileHandle, &bytesRead, 22, &audiotype, 2);
		FSFILE_Read(fileHandle, &bytesRead, 24, &samplerate, 4);
		u16 raw_enc;
		FSFILE_Read(fileHandle, &bytesRead, 20, &raw_enc, 2);
		FSFILE_Read(fileHandle, &bytesRead, 32, &(songFile->bytepersample), 2);
		if (raw_enc == 0x01) songFile->encoding = CSND_ENCODING_PCM16;
		else if (raw_enc == 0x11) songFile->encoding = CSND_ENCODING_ADPCM;
		songFile->samplerate = samplerate;
		songFile->audiotype = audiotype;
		songFile->wavebuf = NULL;
		songFile->wavebuf2 = NULL;
		
		// Audiobuffer extraction
		songFile->isPlaying = false;
		songFile->sourceFile = fileHandle;
		if (mem_size){
			songFile->mem_size = (size-(pos+4));
			while (songFile->mem_size > STREAM_MAX_ALLOC){
				if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
				songFile->mem_size = songFile->mem_size / 2;
			}
			if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
			songFile->startRead = pos+4;
			songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
			songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
			FSFILE_Read(fileHandle, &bytesRead, songFile->startRead, songFile->audiobuf, songFile->mem_size);
			songFile->audio_pointer = songFile->startRead + songFile->mem_size;
			songFile->size = size - (pos+4);
		}else{
			songFile->audiobuf = (u8*)linearAlloc(size-(pos+4));
			FSFILE_Read(fileHandle, &bytesRead, pos+4, songFile->audiobuf, size-(pos+4));
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
	
	// Init function
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	
	// Check for streaming feature usage
	bool mem_size = false;
	if (argc == 2) mem_size = lua_toboolean(L, 2);
	
	// Opening file
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);

	// Init wav struct
	u32 magic,bytesRead,jump,chunk=0x00000000;
	u16 samplerate;
	u16 audiotype;
	FSFILE_Read(fileHandle, &bytesRead, 8, &magic, 4);
	if (magic != 0x46464941) return luaL_error(L, "corrupt AIFF file");
	Music* songFile = (Music*)malloc(sizeof(Music));
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->encoding = CSND_ENCODING_PCM16;
	u64 size;
	songFile->big_endian = true;
	u32 pos = 12;	
	FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);
	
	//Extracting Metadatas
	while (chunk != 0x444E5353){
	
		//Chunks detection
		if (chunk == 0x454D414E){ //NAME Chunk
			u32 chunk_size;
			FSFILE_Read(fileHandle, &bytesRead, pos+4, &chunk_size, 4);
			chunk_size = Endian_UInt32_Conversion(chunk_size);
			char* title = (char*)malloc(chunk_size * sizeof(char));
			FSFILE_Read(fileHandle, &bytesRead, pos+8, title, chunk_size);
			strcpy(songFile->title,title);
			songFile->title[chunk_size+1] = 0;
			free(title);
		}else if (chunk == 0x4D4D4F43){ //COMM Chunk
			FSFILE_Read(fileHandle, &bytesRead, pos+8, &audiotype, 2);
			audiotype = audiotype / 256;
			FSFILE_Read(fileHandle, &bytesRead, pos+14, &(songFile->bytepersample), 2);
			songFile->bytepersample = (songFile->bytepersample / 2048) * audiotype;
			FSFILE_Read(fileHandle, &bytesRead, pos+18, &samplerate, 2);
			samplerate = Endian_UInt16_Conversion(samplerate);
		}	
	
		pos=pos+4;
		FSFILE_Read(fileHandle, &bytesRead, pos, &jump, 4);
		pos=pos+4+Endian_UInt32_Conversion(jump);
		FSFILE_Read(fileHandle, &bytesRead, pos, &chunk, 4);	
	}
	
	//Extracting music info
	pos=pos+4;
	FSFILE_GetSize(fileHandle, &size);
	songFile->mem_size = 0;
	songFile->samplerate = samplerate;
	songFile->audiotype = audiotype;
	songFile->audiobuf2 = NULL;
	songFile->wavebuf = NULL;
	songFile->wavebuf2 = NULL;
	songFile->isPlaying = false;
	songFile->size = size-(pos+4);
	songFile->startRead = 0;
	
	// Extracting audiobuffer
	int stbp = 0;
	if (mem_size){
		songFile->mem_size = (size-(pos+4));
		while (songFile->mem_size > STREAM_MAX_ALLOC){
			if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
			songFile->mem_size = songFile->mem_size / 2;
		}
		if ((songFile->mem_size % 2) == 1) songFile->mem_size++;
		songFile->sourceFile = fileHandle;
		songFile->audiobuf = (u8*)linearAlloc(songFile->mem_size);
		songFile->audiobuf2 = (u8*)linearAlloc(songFile->mem_size);
		songFile->startRead = (pos+4);
		FSFILE_Read(fileHandle, &bytesRead, songFile->startRead, songFile->audiobuf, songFile->mem_size);	
		songFile->audio_pointer = songFile->startRead + songFile->mem_size;
		stbp = songFile->mem_size;
	}else{
		songFile->audiobuf = (u8*)linearAlloc(songFile->size);
		FSFILE_Read(fileHandle, &bytesRead, pos+4, songFile->audiobuf, songFile->size);	
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
		if (ndspInit() == 0){
			isCSND = true;
			ndspSetOutputMode(NDSP_OUTPUT_STEREO);
		}else return luaL_error(L, "cannot initialize Sound module.");
	}
	return 0;
}

static int lua_play(lua_State *L)
{
	
	// Init Function
    int argc = lua_gettop(L);
    if (argc != 2 && argc != 3)	return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	closeStream = false;
	int loop = luaL_checkinteger(L, 2);
	int interp = NDSP_INTERP_LINEAR;
	if (argc == 3) interp = luaL_checkinteger(L, 3);
	
	// Selecting a free channel
	int ch = 0x00;
	while (audioChannels[ch]){
		ch++;
		if (ch > 24) return luaL_error(L, "audio device is busy");
	}
	audioChannels[ch] = true;
	
	// Non native Audiocodecs support
	ThreadFunc streamFunction = streamWAV;
	if (src->encoding == CSND_ENCODING_VORBIS) streamFunction = streamOGG;
	
	// Initializing DSP channel
	int raw_format;
	if (src->encoding == CSND_ENCODING_ADPCM) raw_format = NDSP_FORMAT_MONO_ADPCM;
	else if (src->audiotype == 1) raw_format = NDSP_FORMAT_MONO_PCM16;
	else raw_format = NDSP_FORMAT_STEREO_PCM16;
	ndspChnReset(ch);
	ndspChnWaveBufClear(ch);
	ndspChnSetInterp(ch, interp);
	ndspChnSetRate(ch, float(src->samplerate));
	ndspChnSetFormat(ch, raw_format);
	ndspWaveBuf* waveBuf = (ndspWaveBuf*)calloc(1, sizeof(ndspWaveBuf));
	if (src->mem_size > 0) createDspBlock(waveBuf, src->bytepersample, src->mem_size, 0, (u32*)src->audiobuf);
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
		if (loop == 0) src->streamLoop = false;
		else src->streamLoop = true;
		svcCreateEvent(&updateStream,0);
		u32 *threadStack = (u32*)memalign(32, 8192);
		src->thread = threadStack;
		svcSignalEvent(updateStream);
		Result ret = svcCreateThread(&streamThread, streamFunction, (u32)src, &threadStack[2048], 0x18, 1);
	}
	
	return 0;
}

static int lua_closesong(lua_State *L)
{
	
	// Init function
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	// Closing streaming if used
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
		purgeTable(src->blocks);
	}
	
	// Purging everything
	ndspChnReset(src->ch);
	ndspChnWaveBufClear(src->ch);
	audioChannels[src->ch] = false;
	linearFree(src->audiobuf);
	if (src->mem_size == 0){
		free(src->wavebuf);
		if (src->wavebuf2 != NULL) free(src->wavebuf2);
	}
	if (src->audiobuf2 != NULL) linearFree(src->audiobuf2);
	if (tmp_buf != NULL) linearFree(tmp_buf);
	free(src);
	return 0;
}

static int lua_pause(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	src->resumeSample = ndspChnGetSamplePos(src->ch);
	ndspChnWaveBufClear(src->ch);
	src->isPlaying = false;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_resume(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	ndspChnWaveBufAdd(src->ch, src->wavebuf);
	src->isPlaying = true;
	src->tick = (osGetTime()-src->tick);
	return 0;
}

static int lua_wisPlaying(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
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
		ndspExit();
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
	
	Music* songFile = (Music*)malloc(sizeof(Music));
	songFile->audiobuf = nomute_audiobuf;
	songFile->audiobuf2 = NULL;
	songFile->big_endian = false;
	songFile->mem_size = 0;
	songFile->size = mem_size - 32000;
	songFile->samplerate = 16000;
	strcpy(songFile->author,"");
	strcpy(songFile->title,"");
	songFile->isPlaying = false;
	songFile->bytepersample = 2;
	songFile->magic = 0x4C534E44;
	lua_pushinteger(L,(u32)songFile);
	return 1;
}

static int lua_save(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (src->mem_size != 0) return luaL_error(L, "cannot save a stream");
	const char* file = luaL_checkstring(L, 2);
	Handle fileHandle;
	u32 bytesWritten;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file);
	FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
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

static int lua_getSrate(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->samplerate);
	return 1;
}

static int lua_getTime(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
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
	Music* src = (Music*)luaL_checkinteger(L, 1);
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
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushstring(L, src->title);
	return 1;
}

static int lua_getAuthor(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushstring(L, src->author);
	return 1;
}

static int lua_getType(lua_State *L){
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Music* src = (Music*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
		if (src->magic != 0x4C534E44) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L, src->audiotype);
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
  {"close",					lua_closesong},
  {"play",					lua_play},
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
  {"record",				lua_regsound},
  {"updateStream",			lua_updatestream},
  {"saveWav",				lua_save},
  {0, 0}
};

void luaSound_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Sound_functions, 0);
	lua_setglobal(L, "Sound");
	int LINEAR_INTERP = NDSP_INTERP_LINEAR;
	int POLYPHASE_INTERP = NDSP_INTERP_POLYPHASE;
	int NO_INTERP = NDSP_INTERP_NONE;
	VariableRegister(L, LINEAR_INTERP);
	VariableRegister(L, POLYPHASE_INTERP);
	VariableRegister(L, NO_INTERP);
}