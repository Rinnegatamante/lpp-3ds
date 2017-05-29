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
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/graphics/Graphics.h"
#include "include/Archives.h"
#include "include/utils.h"
#include "include/audio.h"
extern "C"{
	#include "include/brahma/brahma.h"
}

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define MAX_RAM_ALLOCATION	10485760

u16 OLD_3DS_CLOCK = 268;
u16 NEW_3DS_CLOCK = 804;
int current_clock = OLD_3DS_CLOCK;
extern bool isNinjhax2;
bool isTitleDatabaseInit = false;

FS_Archive main_extdata_archive;

static int lua_exit(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	luaL_dostring(L, "collectgarbage()");
	return luaL_error(L, string); // NOTE: This is a fake error
}

static int lua_dofile (lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *fname = luaL_checkstring(L, 1);
	u64 size;
	unsigned char *buffer;
	if (strncmp("romfs:/",fname,7) == 0){	
		FILE* script = fopen(fname,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (script == NULL) return luaL_error(L, "script doesn't exist.");
		#endif
		fseek(script, 0, SEEK_END);
		int size = ftell(script);
		fseek(script, 0, SEEK_SET);
		buffer = (unsigned char*)malloc(size+1);
		fread(buffer, size, 1, script);
		fclose(script);
		buffer[size]=0;		
	}else{
		Handle fileHandle;
		u32 bytesRead;
		FS_Path filePath=fsMakePath(PATH_ASCII, fname);
		FS_Archive script=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		Result ret = FSUSER_OpenFileDirectly( &fileHandle, script, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "script doesn't exist.");
		#endif
		FSFILE_GetSize(fileHandle, &size);
		buffer = (unsigned char*)malloc(size+1);
		FSFILE_Read(fileHandle, &bytesRead, 0, buffer, size);
		buffer[size]=0;
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
	}
	lua_settop(L, 1);
	if (luaL_loadbuffer(L, (const char*)buffer, strlen((const char*)buffer), NULL) != LUA_OK) return lua_error(L);
	lua_KFunction dofilecont = (lua_KFunction)(lua_gettop(L) - 1);
	lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
	return (int)dofilecont;
}

static int lua_openfile(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 2) && (argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	int type = luaL_checkinteger(L, 2);
	u32 archive_id;
	bool extdata = false;
	u32 filesize;
	if (argc >= 3){
		archive_id = luaL_checknumber(L,3);
		extdata = true;
		if (argc == 4) filesize = luaL_checkinteger(L, 4);
	}
	FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
	Handle fileHandle;
	Result ret;
	fileStream* result;
	if (strncmp("romfs:/",file_tbo,7) == 0){	
		FILE* Handle = fopen(file_tbo,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (Handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		result = (fileStream*)malloc(sizeof(fileStream));
		result->handle = (u32)Handle;
		result->isRomfs = true;
	}else if (extdata){
		FS_MediaType mtype;
		FS_ArchiveID atype;
		if (archive_id < 0x2000){
			mtype = MEDIATYPE_SD;
			atype = ARCHIVE_EXTDATA;
		}else{
			mtype = MEDIATYPE_NAND;
			atype = ARCHIVE_SHARED_EXTDATA;
		}
		u32 main_extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
		FS_Archive main_extdata_archive = (FS_Archive){atype, (FS_Path){PATH_BINARY, 0xC, (u8*)main_extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &main_extdata_archive);
		#ifndef SKIP_ERROR_HANDLING
		if(ret!=0) return luaL_error(L, "cannot access extdata archive");
		#endif
		switch(type){
			case 0:
				ret = FSUSER_OpenFile( &fileHandle, main_extdata_archive, filePath, FS_OPEN_READ, 0);
				break;
			case 1:
				ret = FSUSER_OpenFile( &fileHandle, main_extdata_archive, filePath, FS_OPEN_WRITE, 0);
				break;
			case 2:
				FSUSER_DeleteFile(main_extdata_archive, filePath);
				ret = FSUSER_CreateFile(main_extdata_archive, filePath, 0, filesize);
				#ifndef SKIP_ERROR_HANDLING
				if (ret) return luaL_error(L, "error creating the file.");
				#endif
				ret = FSUSER_OpenFile( &fileHandle, main_extdata_archive, filePath, FS_OPEN_WRITE, 0);
				break;
		}
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "error opening file.");
		#endif
		result = (fileStream*)malloc(sizeof(fileStream));
		result->handle = (u32)fileHandle;
		result->isRomfs = false;
	}else{
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		switch(type){
			case 0:
				ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
				break;
			case 1:
				ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_WRITE, 0x00000000);
				break;
			case 2:
				ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);
				break;
		}
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "file doesn't exist.");
		#endif
		result = (fileStream*)malloc(sizeof(fileStream));
		result->handle = (u32)fileHandle;
		result->isRomfs = false;
	}
	result->magic = 0xBEEFDEAD;
	lua_pushinteger(L,(u32)result);
	return 1;
}

static int lua_checkexist(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1);
	if (strncmp("romfs:/",file_tbo,7) == 0){	
		FILE* Handle = fopen(file_tbo,"r");
		bool res = true;
		if (Handle == NULL) res = false;
		else fclose(Handle);
		lua_pushboolean(L,res);
	}else{
		Handle fileHandle;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		if (!ret) FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		lua_pushboolean(L,!ret);
	}
	return 1;
}

static int lua_checkbuild(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	if (isNinjhax2) lua_pushinteger(L,2);
	else if (CIA_MODE) lua_pushinteger(L,1);
	else lua_pushinteger(L,0);
	return 1;
}

static int lua_getRegion(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8 region;
	CFGU_SecureInfoGetRegion(&region);
	lua_pushinteger(L,region);
	return 1;
}

static int lua_screenshot(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1 && argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *screenpath = luaL_checkstring(L, 1);
	bool isJPG = false;
	if (argc == 2) isJPG = lua_toboolean(L, 2);
	Handle fileHandle;
	int x, y;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	if (!isJPG){ //BMP Format
		FS_Path filePath=fsMakePath(PATH_ASCII, screenpath);
		Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);
		//if(ret) return luaL_error(L, "error opening file"); // Sometimes this gives false positives
		u32 bytesWritten;
		u8* tempbuf = (u8*)malloc(0x36+576000);
		memset(tempbuf, 0, 0x36+576000);
		tempbuf[0x36+576000]=0;
		FSFILE_SetSize(fileHandle, (u16)(0x36+576000));
		*(u16*)&tempbuf[0x0] = 0x4D42;
		*(u32*)&tempbuf[0x2] = 0x36 + 576000;
		*(u32*)&tempbuf[0xA] = 0x36;
		*(u32*)&tempbuf[0xE] = 0x28;
		*(u32*)&tempbuf[0x12] = 400;
		*(u32*)&tempbuf[0x16] = 480;
		*(u32*)&tempbuf[0x1A] = 0x00180001;
		*(u32*)&tempbuf[0x22] = 576000;
		u8* framebuf = TopLFB;
		for (y = 0; y < 240; y++){
			for (x = 0; x < 400; x++){
				int si = ((239 - y) + (x * 240)) * 3;
				int di = 0x36 + (x + ((479 - y) * 400)) * 3;
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
			}	
		}
		framebuf = BottomFB;
		for (y = 0; y < 240; y++){
			for (x = 0; x < 320; x++){
				int si = ((239 - y) + (x * 240)) * 3;
				int di = 0x36 + ((x+40) + ((239 - y) * 400)) * 3;
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
			}
			// Patch for wrong left/right borders colors
			for (x = 0; x < 40; x++){
				int di = 0x36 + (x + ((239 - y) * 400)) * 3;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
			}
			for (x = 360; x < 400; x++){
				int di = 0x36 + (x + ((239 - y) * 400)) * 3;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
			}
		}
		FSFILE_Write(fileHandle, &bytesWritten, 0, (u32*)tempbuf, 0x36 + 576000, 0x10001);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		free(tempbuf);
	}else{ // JPG Format
		u8* tempbuf = (u8*)malloc(576000);
		u8* framebuf = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
		for (y = 0; y < 240; y++){
			for (x = 0; x < 400; x++){
				int si = ((239 - y) + (x * 240)) * 3;
				int di = (x + (y*400)) * 3;
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
			}	
		}
		framebuf = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
		for (y = 0; y < 240; y++){
			for (x = 0; x < 320; x++){
				int si = ((239 - y) + (x * 240)) * 3;
				int di = ((x+40) + ((y+240)*400)) * 3;
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
				tempbuf[di++] = framebuf[si++];
			}
			// Patch for wrong left/right borders colors
			for (x = 0; x < 40; x++){
				int di = (x + ((y+240)*400)) * 3;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
			}
			for (x = 360; x < 400; x++){
				int di = (x + ((y+240)*400)) * 3;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
				tempbuf[di++] = 0;
			}
		}
		sdmcInit();
		char tmpPath2[1024];
		strcpy(tmpPath2,"sdmc:");
		strcat(tmpPath2,(char*)screenpath);
		saveJpg(tmpPath2,(u32*)tempbuf,400,480);
		sdmcExit();
		free(tempbuf);
	}
	return 0;
}

static int lua_getsize(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	fileStream* file = (fileStream*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (file->magic != 0xBEEFDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	u64 size;
	FS_GetSize(file, &size);
	lua_pushinteger(L,size);
	return 1;
}

static int lua_closefile(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	#endif
	fileStream* file = (fileStream*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (file->magic != 0xBEEFDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	u64 size;
	FS_Close(file);
	if (argc == 2) FSUSER_CloseArchive( &main_extdata_archive);
	free(file);
	return 0;
}

static int lua_readfile(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3) return luaL_error(L, "wrong number of arguments");
	#endif
	fileStream* file = (fileStream*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (file->magic != 0xBEEFDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	u64 init = luaL_checkinteger(L, 2);
	u64 size = luaL_checkinteger(L, 3);
	unsigned char *buffer = (unsigned char*)malloc(size+1);
	u32 bytesRead;
	FS_Read(file, &bytesRead, init, buffer, size);
	buffer[size] = 0;
	lua_pushlstring(L,(const char*)buffer,size);
	free(buffer);
	return 1;
}


static int lua_writefile(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 4) return luaL_error(L, "wrong number of arguments");
	#endif
	fileStream* file = (fileStream*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (file->magic != 0xBEEFDEAD) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	u64 init = luaL_checkinteger(L, 2);
	const char *text = luaL_checkstring(L, 3);
	u64 size = luaL_checkinteger(L, 4);
	if (file->isRomfs) luaL_error(L, "romFs partition is read-only");
	else{
		u32 bytesWritten;
		Result ret=FSFILE_Write(file->handle, &bytesWritten, init, text, size, FS_WRITE_FLUSH);
		//if(ret || size!=bytesWritten) return luaL_error(L, "error writing file"); // Sometimes this gives false positives
	}
	return 0;
}

static int lua_getFW(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 fw_id = osGetFirmVersion();
	lua_pushinteger(L,GET_VERSION_MAJOR(fw_id));
	lua_pushinteger(L,GET_VERSION_MINOR(fw_id));
	lua_pushinteger(L,GET_VERSION_REVISION(fw_id));
	return 3;
}

static int lua_getLang(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8 language;
	CFGU_GetSystemLanguage(&language);
	lua_pushinteger(L,language);
	return 1;
}

static int lua_getUsername(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	char username_tmp[0x1C];
	char username[0x0E];
	CFGU_GetConfigInfoBlk2(0x1C, 0xA0000, (u8*)&username_tmp);
	for (int i = 0; i < 0x0E; i++){
		username[i] = username_tmp[i<<1];
	}
	lua_pushstring(L,username);
	return 1;
}

static int lua_getBirth(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8 birthday[0x02];
	CFGU_GetConfigInfoBlk2(0x02, 0xA0001, (u8*)&birthday);
	lua_pushinteger(L,birthday[0x01]);
	lua_pushinteger(L,birthday[0x00]);
	return 2;
}

static int lua_getK(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 fw_id = osGetKernelVersion();
	lua_pushinteger(L,GET_VERSION_MAJOR(fw_id));
	lua_pushinteger(L,GET_VERSION_MINOR(fw_id));
	lua_pushinteger(L,GET_VERSION_REVISION(fw_id));
	return 3;
}

static int lua_getCurrentDirectory(lua_State *L){
	lua_pushstring(L, cur_dir);
	return 1;
}

static int lua_setCurrentDirectory(lua_State *L){
	const char *path = luaL_checkstring(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if(!path) return luaL_error(L, "System.currentDirectory(file) takes a filename as a string argument.");
	#endif
	strcpy(cur_dir,path);
	return 0;
}

static int lua_curdir(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc == 0) return lua_getCurrentDirectory(L);
	if(argc == 1) return lua_setCurrentDirectory(L);
	#ifndef SKIP_ERROR_HANDLING
	return luaL_error(L, "System.currentDirectory([file]) takes zero or one argument");
	#endif
}

static int lua_rendir(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	const char *path2 = luaL_checkstring(L, 2);
	FS_Archive sdmcArchive = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive( &sdmcArchive);
	FS_Path filePath=fsMakePath(PATH_ASCII, path);
	FS_Path filePath2=fsMakePath(PATH_ASCII, path2);
	FSUSER_RenameDirectory(sdmcArchive,filePath,sdmcArchive,filePath2);
	FSUSER_CloseArchive( &sdmcArchive);
	return 0;
}

static int lua_createdir(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1 && argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	if (argc == 2){
		u32 archive_id = luaL_checkinteger(L, 2);
		FS_MediaType mtype;
		FS_ArchiveID atype;
		if (archive_id < 0x2000){
			mtype = MEDIATYPE_SD;
			atype = ARCHIVE_EXTDATA;
		}else{
			mtype = MEDIATYPE_NAND;
			atype = ARCHIVE_SHARED_EXTDATA;
		}
		u32 main_extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
		FS_Archive archive2 = (FS_Archive){atype, (FS_Path){PATH_BINARY, 0xC, (u8*)main_extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &archive2);
		#ifndef SKIP_ERROR_HANDLING
		if(ret!=0) return luaL_error(L, "cannot access extdata archive");
		#endif
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_CreateDirectory(archive2,filePath, FS_ATTRIBUTE_DIRECTORY);
		FSUSER_CloseArchive( &archive2);
	}else{
		FS_Archive archive2 = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		Result ret = FSUSER_OpenArchive( &archive2);
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_CreateDirectory(archive2,filePath, FS_ATTRIBUTE_DIRECTORY);
		FSUSER_CloseArchive( &archive2);
	}
	return 0;
}

static int lua_deldir(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1 && argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	if (argc == 2){
		u32 archive_id = luaL_checkinteger(L, 2);
		FS_MediaType mtype;
		FS_ArchiveID atype;
		if (archive_id < 0x2000){
			mtype = MEDIATYPE_SD;
			atype = ARCHIVE_EXTDATA;
		}else{
			mtype = MEDIATYPE_NAND;
			atype = ARCHIVE_SHARED_EXTDATA;
		}
		u32 main_extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
		FS_Archive archive2 = (FS_Archive){atype, (FS_Path){PATH_BINARY, 0xC, (u8*)main_extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &archive2);
		#ifndef SKIP_ERROR_HANDLING
		if(ret!=0) return luaL_error(L, "cannot access extdata archive");
		#endif
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_DeleteDirectory(archive2,filePath);
		FSUSER_CloseArchive( &archive2);
	}else{
		FS_Archive archive2 = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FSUSER_OpenArchive( &archive2);
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_DeleteDirectory(archive2,filePath);
		FSUSER_CloseArchive( &archive2);
	}
	return 0;
}

static int lua_delfile(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1 && argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	if (argc == 2){
		u32 archive_id = luaL_checkinteger(L, 2);
		FS_MediaType mtype;
		FS_ArchiveID atype;
		if (archive_id < 0x2000){
			mtype = MEDIATYPE_SD;
			atype = ARCHIVE_EXTDATA;
		}else{
			mtype = MEDIATYPE_NAND;
			atype = ARCHIVE_SHARED_EXTDATA;
		}
		u32 main_extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
		FS_Archive archive2 = (FS_Archive){atype, (FS_Path){PATH_BINARY, 0xC, (u8*)main_extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &archive2);
		#ifndef SKIP_ERROR_HANDLING
		if(ret!=0) return luaL_error(L, "cannot access extdata archive");
		#endif
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_DeleteFile(archive2,filePath);
		FSUSER_CloseArchive( &archive2);
	}else{
		FS_Archive archive2 = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		Result ret = FSUSER_OpenArchive( &archive2);
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		FSUSER_DeleteFile(archive2,filePath);
		FSUSER_CloseArchive( &archive2);
	}
	return 0;
}

static int lua_renfile(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	const char *path2 = luaL_checkstring(L, 2);
	FS_Archive sdmcArchive = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive( &sdmcArchive);
	FS_Path filePath=fsMakePath(PATH_ASCII, path);
	FS_Path filePath2=fsMakePath(PATH_ASCII, path2);
	FSUSER_RenameFile(sdmcArchive,filePath,sdmcArchive,filePath2);
	FSUSER_CloseArchive( &sdmcArchive);
	return 0;
}

static int lua_listdir(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *path = luaL_checkstring(L, 1);
	Handle dirHandle;
	FS_dirent entry;
	FS_Path dirPath=fsMakePath(PATH_ASCII, path);
	FS_Archive sdmcArchive = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive( &sdmcArchive);
	FSUSER_OpenDirectory(&dirHandle, sdmcArchive, dirPath);
	u32 entriesRead;
	lua_newtable(L);
	int i = 1;
	static char name[1024];
	for (;;){
		entriesRead=0;
		FSDIR_Read(dirHandle, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
		if (entriesRead){
			lua_pushinteger(L, i++);
			lua_newtable(L);
			lua_pushstring(L, "name");
			utf2ascii(&name[0],entry.name);
			lua_pushstring(L, name);
			lua_settable(L, -3);
			lua_pushstring(L, "size");
			lua_pushnumber(L, entry.fileSize);
			lua_settable(L, -3);
			lua_pushstring(L, "directory");
			lua_pushboolean(L, entry.isDirectory);
			lua_settable(L, -3);
			lua_settable(L, -3);
		}else break;
	}
	FSDIR_Close(dirHandle);
	svcCloseHandle(dirHandle);
	FSUSER_CloseArchive( &sdmcArchive);
	return 1;
}

static int lua_batterylv(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8 batteryLevel;
	PTMU_GetBatteryLevel(&batteryLevel);
	lua_pushinteger(L,batteryLevel);
	return 1;
}

static int lua_batterycharge(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u8 batteryLevel;
	PTMU_GetBatteryChargeState(&batteryLevel);
	lua_pushboolean(L,batteryLevel);
	return 1;
}

static int lua_readsmdh(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* file = luaL_checkstring(L, 1);
	char name[64];
	char desc[128];
	char author[64];
	fileStream tmp;
	u32 bytesRead;
	if (strncmp("romfs:/",file,7) == 0){
		tmp.isRomfs = true;
		FILE* handle = fopen(file,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		tmp.handle = (u32)handle;
	}else{
		tmp.isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file);
		Result ret = FSUSER_OpenFileDirectly( &tmp.handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "file doesn't exist.");
		#endif
	}
	u32 magic;
	FS_Read(&tmp, &bytesRead, 0, &magic, 4);
	if (magic != 0x48444D53) return luaL_error(L, "error opening SMDH file.");
	unsigned char *buffer = (unsigned char*)(malloc((129) * sizeof (char)));
	buffer[128] = 0;
	FS_Read(&tmp, &bytesRead, 8, buffer, 128);
	int i = 0;
	while (i < 129){
		if (buffer[i<<1] == 0) break;
		else name[i] = buffer[i<<1];
		i++;
	}
	name[i] = 0;
	FS_Read(&tmp, &bytesRead, 392, buffer, 128);
	i = 0;
	while (i < 129){
		if (buffer[i<<1] == 0) break;
		else author[i] = buffer[i<<1];
		i++;
	}
	author[i] = 0;
	free(buffer);
	buffer = (unsigned char*)(malloc((257) * sizeof (char)));
	buffer[256] = 0;
	FS_Read(&tmp, &bytesRead, 136, buffer, 256);
	i = 0;
	while (i < 257){
		if (buffer[i<<1] == 0) break;
		else desc[i] = buffer[i<<1];
		i++;
	}
	desc[i] = 0;
	free(buffer);
	Bitmap* bitmap = (Bitmap*)malloc(sizeof(Bitmap));
	bitmap->width = 48;
	bitmap->height = 48;
	bitmap->pixels = (u8*)malloc(6912);
	bitmap->bitperpixel = 24;
	u16* icon_buffer = (u16*)malloc(0x1200);
	FS_Read(&tmp, &bytesRead, 0x24C0, icon_buffer, 0x1200);
	FS_Close(&tmp);
	
	// Convert RGB565 icon to a RGB24 one
	int x=0;
	int y=0;
	int tile_size = 16;
	int tile_number = 1;
	int extra_x = 0;
	int extra_y = 0;
	i=0;
	int tile_x[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};
	int tile_y[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
	while (tile_number < 37){
		while (i < (tile_size)){
			putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)<<6)] + extra_x, tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i]);
			putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)<<6)] + extra_x, tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+16]);
			putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)<<6)] + extra_x, 4+tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+32]);
			putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)<<6)] + extra_x, 4+tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+48]);
			i++;
		}
		if (tile_number % 6 == 0){
			extra_x = 0;
			extra_y = extra_y + 8;
		}else extra_x = extra_x + 8;
		tile_number++;
		tile_size = tile_size + 64;
		i = i + 48;
	}
	bitmap->magic = 0x4C494D47;
	free(icon_buffer);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushstring(L, "title");
	lua_pushstring(L, name);
	lua_settable(L, -3);
	lua_pushstring(L, "desc");
	lua_pushstring(L, desc);
	lua_settable(L, -3);
	lua_pushstring(L, "author");
	lua_pushstring(L, author);
	lua_settable(L, -3);
	lua_pushstring(L, "icon");
	lua_pushinteger(L, (u32)bitmap);
	lua_settable(L, -3);
	return 1;
}

//Boot Func
extern void (*__system_retAddr)(void);
static Handle hbHandle;
static u32 argbuffer[0x100];
static u32 argbuffer_length = 0;
void (*callBootloader)(Handle hb, Handle file);
void (*setArgs)(u32* src, u32 length);
static void launchFile(void){ callBootloader(0x00000000, hbHandle); }
void (*callBootloader_2x)(Handle file, u32* argbuf, u32 arglength) = (void*)0x00100000;
static void launchFile_2x(void){ callBootloader_2x(hbHandle, argbuffer, argbuffer_length); }

static int lua_launch(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* file = luaL_checkstring(L, 1);
	if (isNinjhax2){
		argbuffer[0] = 1;
		snprintf((char*)&argbuffer[1], sizeof(argbuffer) - 4, "sdmc:%s", file);
		argbuffer_length = 0x100;
	}else HB_GetBootloaderAddresses((void**)&callBootloader, (void**)&setArgs);
	fsExit();
	fsInit();
	FS_Archive sdmcArchive = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive( &sdmcArchive);
	Result ret = FSUSER_OpenFileDirectly( &hbHandle, sdmcArchive, fsMakePath(PATH_ASCII, file), FS_OPEN_READ, 0x00000000);
	#ifndef SKIP_ERROR_HANDLING
	if (ret) return luaL_error(L, "file doesn't exist.");
	#endif
	if (isNinjhax2) __system_retAddr = launchFile_2x;
	else{
		static u32 argbuffer_nh1[0x200];
		argbuffer_nh1[0]=1;
		snprintf((char*)&argbuffer_nh1[1], 0x800, "sdmc:%s", file);
		setArgs(argbuffer_nh1, 0x800);
		__system_retAddr = launchFile;
	}
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	luaL_dostring(L, "collectgarbage()");
	return luaL_error(L, string); // NOTE: This is a fake error
}

static int lua_listExtdata(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	lua_newtable(L);
	int z = 1;
	Handle extdata_dir;
	static char name[1024];
	u32 i;
	for (i=0; i<0x2000; ++i) {
		u32 extdata_archive_lowpathdata[3] = {MEDIATYPE_SD, i, 0};
		FS_Archive extdata_archive = (FS_Archive){ARCHIVE_EXTDATA, (FS_Path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(&extdata_dir, extdata_archive, fsMakePath(PATH_ASCII, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				utf2ascii(&name[0],entry.name);
				lua_pushstring(L, name);
				lua_settable(L, -3);
				lua_pushstring(L, "size");
				lua_pushnumber(L, entry.fileSize);
				lua_settable(L, -3);
				lua_pushstring(L, "directory");
				lua_pushboolean(L, entry.isDirectory);
				lua_settable(L, -3);
				lua_pushstring(L, "archive");
				lua_pushinteger(L, i);
				lua_settable(L, -3);
				lua_settable(L, -3);
			}else break;
		}
		FSDIR_Close(extdata_dir);
		FSUSER_CloseArchive( &extdata_archive);
	}
	for (i=0xE0000000; i<0xE0000100; ++i) {
		u32 extdata_archive_lowpathdata[3] = {MEDIATYPE_NAND, i, 0};
		FS_Archive extdata_archive = (FS_Archive){ARCHIVE_SHARED_EXTDATA, (FS_Path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(&extdata_dir, extdata_archive, fsMakePath(PATH_ASCII, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				utf2ascii(&name[0],entry.name);
				lua_pushstring(L, name);
				lua_settable(L, -3);
				lua_pushstring(L, "size");
				lua_pushnumber(L, entry.fileSize);
				lua_settable(L, -3);
				lua_pushstring(L, "directory");
				lua_pushboolean(L, entry.isDirectory);
				lua_settable(L, -3);
				lua_pushstring(L, "archive");
				lua_pushinteger(L, i);
				lua_settable(L, -3);
				lua_settable(L, -3);
			}else break;
		}
		FSDIR_Close(extdata_dir);
		FSUSER_CloseArchive( &extdata_archive);
	}
	for (i=0xF0000000; i<0xF0000100; ++i) {
		u32 extdata_archive_lowpathdata[3] = {MEDIATYPE_NAND, i, 0};
		FS_Archive extdata_archive = (FS_Archive){ARCHIVE_SHARED_EXTDATA, (FS_Path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive( &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(&extdata_dir, extdata_archive, fsMakePath(PATH_ASCII, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				utf2ascii(&name[0],entry.name);
				lua_pushstring(L, name);
				lua_settable(L, -3);
				lua_pushstring(L, "size");
				lua_pushnumber(L, entry.fileSize);
				lua_settable(L, -3);
				lua_pushstring(L, "directory");
				lua_pushboolean(L, entry.isDirectory);
				lua_settable(L, -3);
				lua_pushstring(L, "archive");
				lua_pushinteger(L, i);
				lua_settable(L, -3);
				lua_settable(L, -3);
			}else break;
		}
		FSDIR_Close(extdata_dir);
		FSUSER_CloseArchive( &extdata_archive);
	}
	svcCloseHandle(extdata_dir);
	return 1;
}

static int lua_listExtdataDir(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* path = luaL_checkstring(L, 1);
	u32 archive_id = luaL_checkinteger(L, 2);
	lua_newtable(L);
	int z = 1;
	static char name[1024];
	FS_MediaType mtype;
	FS_ArchiveID atype;
	if (archive_id < 0x2000){
		mtype = MEDIATYPE_SD;
		atype = ARCHIVE_EXTDATA;
	}else{
		mtype = MEDIATYPE_NAND;
		atype = ARCHIVE_SHARED_EXTDATA;
	}
	u32 extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
	FS_Archive extdata_archive = (FS_Archive){atype, (FS_Path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
	Result ret = FSUSER_OpenArchive( &extdata_archive);
	#ifndef SKIP_ERROR_HANDLING
	if(ret!=0) return luaL_error(L, "cannot access extdata archive");
	#endif
	Handle extdata_dir;
	FSUSER_OpenDirectory(&extdata_dir, extdata_archive, fsMakePath(PATH_ASCII, path));
	FS_dirent entry;
	for (;;){
		u32 entriesRead=0;
		FSDIR_Read(extdata_dir, &entriesRead, 1, (FS_DirectoryEntry*)&entry);
		if (entriesRead){
			lua_pushinteger(L, z++);
			lua_newtable(L);
			lua_pushstring(L, "name");
			utf2ascii(&name[0],entry.name);
			lua_pushstring(L, name);
			lua_settable(L, -3);
			lua_pushstring(L, "size");
			lua_pushnumber(L, entry.fileSize);
			lua_settable(L, -3);
			lua_pushstring(L, "directory");
			lua_pushboolean(L, entry.isDirectory);
			lua_settable(L, -3);
			lua_pushstring(L, "archive");
			lua_pushinteger(L, archive_id);
			lua_settable(L, -3);
			lua_settable(L, -3);
		}else break;
	}
	FSDIR_Close(extdata_dir);
	FSUSER_CloseArchive( &extdata_archive);
	svcCloseHandle(extdata_dir);	
	return 1;
}

static int lua_installCia(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* path = luaL_checkstring(L, 1);
	u8 mediatype = luaL_checkinteger(L, 2);
	FS_MediaType media;
	u64 size = 0;
	u32 bytes;
	if (mediatype == 1) media = MEDIATYPE_SD;
	else media = MEDIATYPE_NAND;
	fileStream tmp;
	Handle ciaHandle;
	if (strncmp("romfs:/",path,7) == 0){
		tmp.isRomfs = true;
		FILE* handle = fopen(path,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		tmp.handle = (u32)handle;
	}else{
		tmp.isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, path);
		Result ret = FSUSER_OpenFileDirectly( &tmp.handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "file doesn't exist.");
		#endif
	}
	FS_GetSize(&tmp, &size);
	amInit();
	if (!isTitleDatabaseInit){
		isTitleDatabaseInit = true;
		AM_InitializeExternalTitleDatabase(false);
	}
	AM_StartCiaInstall(media, &ciaHandle);
	if (size < MAX_RAM_ALLOCATION){
		u8* cia_buffer = (u8*)(malloc(size * sizeof (u8)));
		FS_Read(&tmp, &bytes, 0, cia_buffer, size);
		FSFILE_Write(ciaHandle, &bytes, 0, cia_buffer, size, 0);
		free(cia_buffer);
	}else{
		u64 i = 0;
		u8* cia_buffer;
		while (i < size){
			u64 bytesToRead;
			if	(i+MAX_RAM_ALLOCATION > size){
				cia_buffer = (u8*)malloc(size-i);
				bytesToRead = size - i;
			}else{
				cia_buffer = (u8*)malloc(MAX_RAM_ALLOCATION);
				bytesToRead = MAX_RAM_ALLOCATION;
			}
			FS_Read(&tmp, &bytes, i, cia_buffer, bytesToRead);
			FSFILE_Write(ciaHandle, &bytes, i, cia_buffer, bytesToRead, 0);
			i = i + bytesToRead;
			free(cia_buffer);
		}
	}
	AM_FinishCiaInstall(ciaHandle);
	FS_Close(&tmp);
	amExit();
	return 0;
}

struct TitleId{
	u32 uniqueid;
	u16 category;
	u16 platform;
};

static int lua_listCia(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	amInit();
	if (!isTitleDatabaseInit){
		isTitleDatabaseInit = true;
		AM_InitializeExternalTitleDatabase(false);
	}
	u32 cia_nums;
	u32 titlesRead;
	AM_GetTitleCount(MEDIATYPE_SD, &cia_nums);
	TitleId* TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(&titlesRead, MEDIATYPE_SD, cia_nums, (u64*)TitleIDs);
	AM_TitleEntry* entries = (AM_TitleEntry*)malloc(sizeof(AM_TitleEntry)*cia_nums);
	AM_GetTitleInfo(MEDIATYPE_SD, cia_nums, (u64*)TitleIDs, entries);
	u32 i = 1;
	lua_newtable(L);
	while (i <= cia_nums){
		lua_pushinteger(L, i);
		lua_newtable(L);
		lua_pushstring(L, "unique_id");
		lua_pushinteger(L, (TitleIDs[i-1].uniqueid));
		lua_settable(L, -3);
		lua_pushstring(L, "mediatype");
		lua_pushinteger(L, 1);
		lua_settable(L, -3);
		lua_pushstring(L, "platform");
		lua_pushinteger(L, (TitleIDs[i-1].platform));
		lua_settable(L, -3);
		u64 id = TitleIDs[i-1].uniqueid | ((u64)TitleIDs[i-1].category << 32) | ((u64)TitleIDs[i-1].platform << 48);
		char product_id[16];
		AM_GetTitleProductCode(MEDIATYPE_SD, id, product_id);
		lua_pushstring(L, "product_id");
		lua_pushstring(L, product_id);
		lua_settable(L, -3);
		lua_pushstring(L, "access_id");
		lua_pushinteger(L, i);
		lua_settable(L, -3);
		lua_pushstring(L, "version");
		lua_pushinteger(L, entries[i-1].version);
		lua_settable(L, -3);
		lua_pushstring(L, "size");
		lua_pushinteger(L, entries[i-1].size);
		lua_settable(L, -3);
		lua_pushstring(L, "category");
		if(((TitleIDs[i-1].category) & 0x8000) == 0x8000) lua_pushinteger(L, 4);
		else if (((TitleIDs[i-1].category) & 0x10) == 0x10) lua_pushinteger(L, 1);
		else if(((TitleIDs[i-1].category) & 0x6) == 0x6) lua_pushinteger(L, 3);
		else if(((TitleIDs[i-1].category) & 0x2) == 0x2) lua_pushinteger(L, 2);
		else lua_pushnumber(L, 0);
		lua_settable(L, -3);
		lua_settable(L, -3);
		i++;
	}
	free(TitleIDs);
	free(entries);
	u32 z = 1;
	AM_GetTitleCount(MEDIATYPE_NAND, &cia_nums);
	TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(&titlesRead,MEDIATYPE_NAND,cia_nums,(u64*)TitleIDs);
	entries = (AM_TitleEntry*)malloc(sizeof(AM_TitleEntry)*cia_nums);
	AM_GetTitleInfo(MEDIATYPE_NAND, cia_nums, (u64*)TitleIDs, entries);
	while (z <= cia_nums){
		lua_pushinteger(L, i);
		lua_newtable(L);
		lua_pushstring(L, "unique_id");
		lua_pushinteger(L, (TitleIDs[i-1].uniqueid));
		lua_settable(L, -3);
		lua_pushstring(L, "mediatype");
		lua_pushinteger(L, 2);
		lua_settable(L, -3);
		lua_pushstring(L, "platform");
		lua_pushinteger(L, (TitleIDs[i-1].platform));
		lua_settable(L, -3);
		u64 id = TitleIDs[i-1].uniqueid | ((u64)TitleIDs[i-1].category << 32) | ((u64)TitleIDs[i-1].platform << 48);
		char product_id[16];
		AM_GetTitleProductCode(MEDIATYPE_NAND, id, product_id);
		lua_pushstring(L, "product_id");
		lua_pushstring(L, product_id);
		lua_settable(L, -3);
		lua_pushstring(L, "access_id");
		lua_pushinteger(L, z);
		lua_settable(L, -3);
		lua_pushstring(L, "version");
		lua_pushinteger(L, entries[i-1].version);
		lua_settable(L, -3);
		lua_pushstring(L, "size");
		lua_pushinteger(L, entries[i-1].size);
		lua_settable(L, -3);
		lua_pushstring(L, "category");
		if(((TitleIDs[i-1].category) & 0x8000) == 0x8000) lua_pushinteger(L, 4);
		else if (((TitleIDs[i-1].category) & 0x10) == 0x10) lua_pushinteger(L, 1);
		else if(((TitleIDs[i-1].category) & 0x6) == 0x6) lua_pushinteger(L, 3);
		else if(((TitleIDs[i-1].category) & 0x2) == 0x2) lua_pushinteger(L, 2);
		else lua_pushinteger(L, 0);
		lua_settable(L, -3);
		lua_settable(L, -3);
		i++;
		z++;
	}
	free(TitleIDs);
	free(entries);
	amExit();
	return 1;
}

static int lua_uninstallCia(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 delete_id = luaL_checknumber(L,1);
	u32 mediatype = luaL_checknumber(L,2);
	FS_MediaType media;
	if (mediatype == 1) media = MEDIATYPE_SD;
	else media = MEDIATYPE_NAND;
	amInit();
	if (!isTitleDatabaseInit){
		isTitleDatabaseInit = true;
		AM_InitializeExternalTitleDatabase(false);
	}
	u32 cia_nums;
	u32 titlesRead;
	AM_GetTitleCount(media, &cia_nums);
	TitleId* TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(&titlesRead,media,cia_nums,(u64*)TitleIDs);
	u64 id = TitleIDs[delete_id-1].uniqueid | ((u64)TitleIDs[delete_id-1].category << 32) | ((u64)TitleIDs[delete_id-1].platform << 48);
	AM_DeleteAppTitle(media, id);
	AM_DeleteTitle(media, id);
	amExit();
	free(TitleIDs);
	return 0;
}

static int lua_ciainfo(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	const char* file = luaL_checkstring(L, 1);
	char title[16];
	Handle fileHandle;
	u32 bytesRead;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FS_Path filePath=fsMakePath(PATH_ASCII, file);
	Result ret = FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
	#ifndef SKIP_ERROR_HANDLING
	if (ret) return luaL_error(L, "file doesn't exist.");
	#endif
	AM_TitleEntry info;
	amInit();
	AM_GetCiaFileInfo(MEDIATYPE_SD, &info, fileHandle);
	FSFILE_Read(fileHandle, &bytesRead, 0x3A50, title, 16);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushstring(L, "title");
	lua_pushstring(L, title);
	lua_settable(L, -3);
	lua_pushstring(L, "unique_id");
	u64 unique_id = ((info.titleID << 32) >> 32);
	lua_pushinteger(L, unique_id);
	lua_settable(L, -3);
	lua_pushstring(L, "version");
	lua_pushinteger(L, info.version);
	lua_settable(L, -3);
	lua_pushstring(L, "install_size");
	lua_pushnumber(L, info.size);
	lua_settable(L, -3);
	lua_pushstring(L, "icon");
	char* smdh_data = (char*)malloc(0x36C0);
	if (R_SUCCEEDED(AM_GetCiaIcon((void*)smdh_data, fileHandle))){
		char name[64];
		char desc[128];
		char author[64];
		char* buffer = (char*)&smdh_data[8];
		int i = 0;
		while (i < 129){
			if (buffer[i<<1] == 0) break;
			else name[i] = buffer[i<<1];
			i++;
		}
		name[i] = 0;
		buffer = (char*)&smdh_data[392];
		i = 0;
		while (i < 129){
			if (buffer[i<<1] == 0) break;
			else author[i] = buffer[i<<1];
			i++;
		}
		author[i] = 0;
		buffer = (char*)&smdh_data[136];
		i = 0;
		while (i < 257){
			if (buffer[i<<1] == 0) break;
			else desc[i] = buffer[i<<1];
			i++;
		}
		desc[i] = 0;
		buffer = (char*)&smdh_data[0x24C0];
		u16* icon_buffer = (u16*)buffer;
		Bitmap* bitmap = (Bitmap*)malloc(sizeof(Bitmap));
		bitmap->width = 48;
		bitmap->height = 48;
		bitmap->pixels = (u8*)malloc(6912);
		bitmap->bitperpixel = 24;
		
		//convert RGB565 to RGB24
		int x=0;
		int y=0;
		int tile_size = 16;
		int tile_number = 1;
		int extra_x = 0;
		int extra_y = 0;
		i=0;
		int tile_x[16] = {0,1,0,1,2,3,2,3,0,1,0,1,2,3,2,3};
		int tile_y[16] = {0,0,1,1,0,0,1,1,2,2,3,3,2,2,3,3};
		while (tile_number < 37){
			while (i < (tile_size)){
				putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)<<6)] + extra_x, tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i]);
				putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)<<6)] + extra_x, tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+16]);
				putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)<<6)] + extra_x, 4+tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+32]);
				putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)<<6)] + extra_x, 4+tile_y[i-((tile_number-1)<<6)] + extra_y, icon_buffer[i+48]);
				i++;
			}
			if (tile_number % 6 == 0){
				extra_x = 0;
				extra_y = extra_y + 8;
			}else extra_x = extra_x + 8;
			tile_number++;
			tile_size = tile_size + 64;
			i = i + 48;
		}
		bitmap->magic = 0x4C494D47;
		lua_pushinteger(L, (u32)bitmap);
		lua_settable(L, -3);
		lua_pushstring(L, "name");
		lua_pushstring(L, name);
		lua_settable(L, -3);
		lua_pushstring(L, "desc");
		lua_pushstring(L, desc);
		lua_settable(L, -3);
		lua_pushstring(L, "author");
		lua_pushstring(L, author);
	}else{
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pushstring(L, "name");
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pushstring(L, "desc");
		lua_pushnil(L);
		lua_settable(L, -3);
		lua_pushstring(L, "author");
		lua_pushnil(L);
	}
	amExit();
	free(smdh_data);
	lua_settable(L, -3);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	return 1;
}

static int lua_ZipExtract(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 2 && argc != 3) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char *FileToExtract = luaL_checkstring(L, 1);
	const char *DirTe = luaL_checkstring(L, 2);
	const char *Password = (argc == 3) ? luaL_checkstring(L, 3) : NULL;
	FS_Archive sdmcArchive = (FS_Archive){0x9, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive( &sdmcArchive);
	FS_Path TEMP_PATH=fsMakePath(PATH_ASCII, DirTe);
	FSUSER_CreateDirectory(sdmcArchive,TEMP_PATH, FS_ATTRIBUTE_DIRECTORY);
	FSUSER_CloseArchive( &sdmcArchive);
	char tmpFile2[1024];
	char tmpPath2[1024];
	sdmcInit();
	strcpy(tmpPath2,"sdmc:");
	strcat(tmpPath2,(char*)DirTe);
	chdir(tmpPath2);
	if (strncmp("romfs:/",FileToExtract,7) == 0) strcpy(tmpFile2,FileToExtract);
	else{
		strcpy(tmpFile2,"sdmc:");
		strcat(tmpFile2,(char*)FileToExtract);
	}
	Zip *handle = ZipOpen(tmpFile2);
	#ifndef SKIP_ERROR_HANDLING
	if (handle == NULL) luaL_error(L, "error opening ZIP file.");
	#endif
	int result = ZipExtract(handle, Password);
	ZipClose(handle);
	sdmcExit();
	lua_pushinteger(L, result);
	return 1;
}

static int lua_getfilefromzip(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 3 && argc != 4 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char *FileToExtract = luaL_checkstring(L, 1);
	const char *FileToExtract2 = luaL_checkstring(L, 2);
	const char *Dest = luaL_checkstring(L, 3);
	const char *Password = (argc == 4) ? luaL_checkstring(L, 4) : NULL;
	char tmpFile2[1024];
	sdmcInit();
	if (strncmp("romfs:/",FileToExtract,7) == 0) strcpy(tmpFile2,FileToExtract);
	else{
		strcpy(tmpFile2,"sdmc:");
		strcat(tmpFile2,(char*)FileToExtract);
	}
	Zip *handle = ZipOpen(tmpFile2);
	#ifndef SKIP_ERROR_HANDLING
	if (handle == NULL) luaL_error(L, "error opening ZIP file.");
	#endif
	ZipFile* file = ZipFileRead(handle, FileToExtract2, Password);
	if (file == NULL) lua_pushboolean(L, false);
	else{
		Handle fileHandle;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, Dest);
		Result ret = FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE | FS_OPEN_WRITE, 0x00000000);
		u32 bytesWritten;
		ret=FSFILE_Write(fileHandle, &bytesWritten, 0, file->data, file->size, FS_WRITE_FLUSH);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		ZipFileFree(file);
		lua_pushboolean(L, true);
	}
	ZipClose(handle);
	sdmcExit();
	return 1;
}

static int lua_model(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	u8 model;
	CFGU_GetSystemModel(&model);
	lua_pushinteger(L,model);
	return 1;
}

static int lua_brahmaloader(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 2 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char* file = luaL_checkstring(L, 1);
	u32 offset = luaL_checkinteger(L, 2);
	brahma_init();
	load_arm9_payload_offset ((char*)file, offset, 0x10000);
	firm_reboot();
    brahma_exit();
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	luaL_dostring(L, "collectgarbage()");
	return luaL_error(L, string); // NOTE: This is a fake error
}
	
static int lua_syscall1(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	aptReturnToMenu();
	return 0;
}

static int lua_appstatus(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	APT_AppStatus status = aptGetStatus();
	lua_pushinteger(L,status);
	return 1;
}

static int lua_reboot(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	aptOpenSession();
	APT_HardwareResetAsync();
	aptCloseSession();
	for(;;){}
	return 0;
}

static int lua_startcard(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	amInit();
	char product_id[16];
	AM_GetTitleProductCode(MEDIATYPE_GAME_CARD, 0, product_id);
	amExit();
	luaL_dostring(L, "collectgarbage()");
	if (product_id[0] == 'C' and product_id[1] == 'T' and product_id[2] == 'R'){
		u8 buf0[0x300];
		u8 buf1[0x20];
		memset(buf0, 0, 0x300);
		memset(buf1, 0, 0x20);
		aptOpenSession();
		APT_PrepareToDoAppJump(0, 0, MEDIATYPE_GAME_CARD);
		APT_DoAppJump(0x300, 0x20, buf0, buf1);
		aptCloseSession();
	}else{
		nsInit();
		NS_RebootToTitle(MEDIATYPE_GAME_CARD,0);
		nsExit();
	}
	for (;;){}
	return 0;
}

static int lua_getcard(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	#endif
	amInit();
	char product_id[16];
	AM_GetTitleProductCode(MEDIATYPE_GAME_CARD, 0, product_id);
	amExit();
	if (product_id[0] == 'C' and product_id[1] == 'T' and product_id[2] == 'R') lua_pushstring(L,product_id);
	else lua_pushstring(L,"");
	return 1;
}

static int lua_freespace(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	FS_ArchiveResource sdmcData;
	FSUSER_GetSdmcArchiveResource(&sdmcData);
	lua_pushnumber(L,(u64)((u64)sdmcData.freeClusters*(u64)sdmcData.clusterSize));
	return 1;
}

static int lua_launchCia(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 unique_id = luaL_checkinteger(L,1);
	u32 mediatype = luaL_checkinteger(L,2);
	u8 buf0[0x300];
	u8 buf1[0x20];
	u64 id = unique_id | ((u64)0x00040000 << 32);
	memset(buf0, 0, 0x300);
	memset(buf1, 0, 0x20);
	luaL_dostring(L, "collectgarbage()");
	aptOpenSession();
	APT_PrepareToDoAppJump(0, id, mediatype);
	APT_DoAppJump(0x300, 0x20, buf0, buf1);
	aptCloseSession();
	for (;;){}
	return 0;
}

static int lua_gettime(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u64 time = (osGetTime() / 1000) % 86400;
	u8 hours = time / 3600;
	u8 minutes = (time % 3600) / 60;
	u8 seconds = time % 60;
	lua_pushinteger(L,hours);
	lua_pushinteger(L,minutes);
	lua_pushinteger(L,seconds);
	return 3;
}

u32 month_seconds[4] = {2592000,2419200,2678400,2505600}; // 30-28-31-29 days
u8 day_values[7] = {6,7,1,2,3,4,5};

static int lua_getdate(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	u64 time = (osGetTime() / 1000) - 3155673600; // Time from 1st January 2000
	u32 day_value = ((time / 86400) % 7);
	u32 year = 2000;
	u8 day = 1;
	u8 month = 1;
	u32 control;
	if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) control = 31622400;
	else control = 31536000;
	while (time > control){
		year++;
		time = time - control;
		if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) control = 31622400;
		else control = 31536000;
	}
	bool extended = false;
	if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) extended = true;
	control = month_seconds[2];
	while (time > control){
		month++;
		time = time - control;
		if ((month == 11) || (month == 4) || (month == 6) || (month == 9)) control = month_seconds[0];
		else if (month == 2){
			if (extended) control = month_seconds[3];
			else control = month_seconds[1];
		}else control = month_seconds[2];
	}
	day = day + time / 86400;
	lua_pushinteger(L,day_values[day_value]);
	lua_pushinteger(L,day);
	lua_pushinteger(L,month);
	lua_pushinteger(L,year);
	return 4;
}

static int lua_listnews(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	lua_newtable(L);
	u32 tot;
	u32 i = 1;
	newsInit();
	NEWS_GetTotalNotifications(&tot);
	while (i <= tot){
		NotificationHeader header;
		NEWS_GetNotificationHeader(i-1, &header);
		char title[32];
		utf2ascii(title,header.title);
		lua_pushinteger(L, i);
		lua_newtable(L);
		lua_pushstring(L, "id");
		lua_pushinteger(L, i-1);
		lua_settable(L, -3);
		lua_pushstring(L, "hasImage");
		lua_pushboolean(L, header.enableJPEG);
		lua_settable(L, -3);
		lua_pushstring(L, "unread");
		lua_pushboolean(L, header.unread);
		lua_settable(L, -3);
		lua_pushstring(L, "isSpotPass");
		lua_pushboolean(L, header.isSpotPass);
		lua_settable(L, -3);
		lua_pushstring(L, "title");
		lua_pushstring(L, title);
		lua_settable(L, -3);
		lua_settable(L, -3);
		i++;
	}
	newsExit();
	return 1;
}

static int lua_addnews(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 2) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *title = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);
	u32* image = NULL;
	u32 img_size = 0;
	bool hasImage = false;
	bool processed = false;
	Bitmap* file;
	if (argc == 4){
		bool isFile = lua_toboolean(L, 4);
		if (isFile){
			fileStream tmp;
			const char *filename = luaL_checkstring(L, 3);
			if (strncmp("romfs:/",filename,7) == 0){
				tmp.isRomfs = true;
				FILE* handle = fopen(filename,"r");
				#ifndef SKIP_ERROR_HANDLING
				if (handle == NULL) return luaL_error(L, "file doesn't exist.");
				#endif
			}else{
				tmp.isRomfs = false;
				FS_Path filePath=fsMakePath(PATH_ASCII, filename);
				FS_Archive script=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
				Result ret = FSUSER_OpenFileDirectly( &tmp.handle, script, filePath, FS_OPEN_READ, 0x00000000);
				#ifndef SKIP_ERROR_HANDLING
				if (ret) return luaL_error(L, "file doesn't exist.");
				#endif
			}
			u16 magic;
			u64 long_magic;
			u32 bytesRead;
			u64 size;
			FS_Read(&tmp, &bytesRead, 0, &magic, 2);
			if (magic == 0xD8FF){
				FS_GetSize(&tmp, &size);
				img_size = size;
				if (img_size > 0x10000){
					FS_Close(&tmp);
					return luaL_error(L, "image is too big.");
				}
				u8* buffer = (u8*)malloc(img_size);
				FS_Read(&tmp, &bytesRead, 0, buffer, img_size);
				FS_Close(&tmp);
				image = (u32*)buffer;
				processed = true;
			}else{
				if (magic == 0x5089){
					FS_Read(&tmp, &bytesRead, 0, &long_magic, 8);
					FS_Close(&tmp);
					if (long_magic == 0x0A1A0A0D474E5089) file = loadPng(filename);
				}else if (magic == 0x4D42){
					FS_Close(&tmp);
					file = LoadBitmap((char*)filename);
				}else{
					FS_Close(&tmp);
					return luaL_error(L, "wrong file format.");
				}
			}
		}else{
			file = (Bitmap*)luaL_checkinteger(L, 3);
			#ifndef SKIP_ERROR_HANDLING
			if (file->magic != 0x4C494D47) return luaL_error(L, "attempt to access wrong memory block type");
			#endif
		}
		if (!processed){
			u8 moltiplier = file->bitperpixel >> 3;
			u32 size_val = file->width * file->height * moltiplier;
			u8* flip_pixels = (u8*)malloc(size_val);
			flip_pixels = flipBitmap(flip_pixels, file);
			if (moltiplier == 4){ // 32bpp image - Need to delete alpha channel
				u8* tmp = flip_pixels;
				flip_pixels = (u8*)malloc((file->width)*(file->height)*3);
				u32 i = 0;
				u32 j = 0;
				while ((i+1) < size_val){
					flip_pixels[j++] = tmp[i];
					flip_pixels[j++] = tmp[i+1];
					flip_pixels[j++] = tmp[i+2];
					i = i + 4;
				}
				free(tmp);
			}
			image = toJpg(&img_size,(u32*)flip_pixels,file->width,file->height);
			if (isFile){
				free(file->pixels);
				free(file);
			}
			if (img_size > 0x10000){
				free(image);
				return luaL_error(L, "image is too big.");
			}
		}
		hasImage = true;
	}
	u16* uni_title = (u16*)malloc(strlen(title)*sizeof(u16));
	u16* uni_text = (u16*)malloc(strlen(text)*sizeof(u16));
	ascii2utf(uni_title,(char*)title);
	ascii2utf(uni_text,(char*)text);
	newsInit();
	NEWS_AddNotification(uni_title, strlen(title), uni_text, strlen(text), image, img_size, hasImage);
	if (image != NULL) free(image);
	newsExit();
	free(uni_title);
	free(uni_text);
	return 0;
}

static int lua_getnews(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 id = luaL_checkinteger(L, 1);
	u16 message[0x1780];
	char result[0x1780<<1];
	newsInit();
	NEWS_GetNotificationMessage(id, message);
	utf2ascii(result, message);
	newsExit();
	lua_pushstring(L, result);
	return 1;
}

static int lua_erasenews(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 id = luaL_checkinteger(L, 1);
	NotificationHeader header = { 0 };
	newsInit();
	NEWS_SetNotificationHeader(id, (const NotificationHeader*)&header);
	newsExit();
	return 0;
}

static int lua_setcpu(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");	
	#endif
	u16 cpu_clock = luaL_checkinteger(L,1);
	bool isNew;
	APT_CheckNew3DS((u8*)&isNew);
	if (cpu_clock >= NEW_3DS_CLOCK && isNew){
		osSetSpeedupEnable(1);
		current_clock = NEW_3DS_CLOCK;
	}else {
		osSetSpeedupEnable(0);
		current_clock = OLD_3DS_CLOCK;
	}
	return 0;
}

static int lua_getcpu(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	lua_pushinteger(L,current_clock);
	return 1;
}

static int lua_detectsd(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	bool isSD;
	FSUSER_IsSdmcDetected(&isSD);
	lua_pushboolean(L, isSD);
	return 1;
}

static int lua_dup(lua_State *L){ // TODO: Add Music and wav struct support
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	Music* src = (Music*)luaL_checkinteger(L,1); // Music is just a random memory block ptr type
	u8* res = NULL;
	if (src->magic == 0x4C544D52){ // Timer
		Timer* dst = (Timer*)malloc(sizeof(Timer));
		memcpy(dst, src, sizeof(Timer));
		res = (u8*)dst;
	}else if(src->magic == 0x4C464E54){ // Font
		ttf* dst = (ttf*)malloc(sizeof(ttf));
		ttf* src2 = (ttf*)src;
		memcpy(dst, src2, sizeof(ttf));
		dst->buffer = (unsigned char*)malloc(src2->bufsize);
		memcpy(dst->buffer, src2->buffer, src2->bufsize);
		res = (u8*)dst;
	}else if(src->magic == 0x4C494D47){ // CPU Render Image
		Bitmap* dst = (Bitmap*)malloc(sizeof(Bitmap));
		Bitmap* src2 = (Bitmap*)src;
		memcpy(dst, src2, sizeof(Bitmap));
		u32 bufsize = src2->width*src2->height*(src2->bitperpixel>>3);
		dst->pixels = (u8*)malloc(bufsize);
		memcpy(dst->pixels, src2->pixels, bufsize);
		res = (u8*)dst;
	}else if(src->magic == 0x4C545854){ // GPU Render Image
		gpu_text* dst = (gpu_text*)malloc(sizeof(gpu_text));
		gpu_text* src2 = (gpu_text*)src;
		memcpy(dst, src2, sizeof(gpu_text));
		dst->tex = (sf2d_texture*)(malloc(sizeof(sf2d_texture)));
		memcpy(dst->tex, src2->tex, sizeof(sf2d_texture));
		dst->tex->data = linearAlloc(src2->tex->data_size);
		memcpy(dst->tex->data, src2->tex->data, src2->tex->data_size);
		res = (u8*)dst;
	}else if(src->magic == 0xC0C0C0C0){ // Render-type Color
		color* dst = (color*)malloc(sizeof(color));
		memcpy(dst, (color*)src, sizeof(color));
		res = (u8*)dst;
	}else if(src->magic == 0xC00FFEEE){ // 3D Model
		model* dst = (model*)malloc(sizeof(model));
		model* src2 = (model*)src;
		memcpy(dst, src2, sizeof(model));
		u32 vbo_size = src2->vertex_count * sizeof(vertex);
		dst->vbo_data = (u8*)malloc(vbo_size);
		memcpy(dst->vbo_data, src2->vbo_data, vbo_size);
		dst->texture = (C3D_Tex*)linearAlloc(sizeof(C3D_Tex));
		memcpy(dst->texture, src2->texture, sizeof(C3D_Tex));
		u32 tex_size = (src2->texture->width*src2->texture->height)<<2;
		dst->texture->data = linearAlloc(tex_size);
		memcpy(dst->texture->data, src2->texture->data, tex_size);
		dst->material = (C3D_Mtx*)linearAlloc(sizeof(C3D_Mtx));
		*dst->material = {
			{
				{ { 0.0f, src2->material->r[0].c[1], src2->material->r[0].c[2], src2->material->r[0].c[3] } }, // Ambient
				{ { 0.0f, src2->material->r[1].c[1], src2->material->r[1].c[2], src2->material->r[1].c[3] } }, // Diffuse
				{ { 0.0f, src2->material->r[2].c[1], src2->material->r[2].c[2], src2->material->r[2].c[3] } }, // Specular
				{ { src2->material->r[3].c[0], 0.0f, 0.0f, 0.0f } }, // Emission
			}
		};
		res = (u8*)dst;
	}else if(src->magic == 0x4C434E53){ // Console
		Console* dst = (Console*)malloc(sizeof(Console));
		memcpy(dst, (Console*)src, sizeof(Console));
		res = (u8*)dst;
	}else return luaL_error(L, "unsupported memory block input.");
	lua_pushinteger(L, (u32)res);
	return 1;
}

//Register our System Functions
static const luaL_Reg System_functions[] = {
	{"exit",                lua_exit},
	{"getFirmware",         lua_getFW},
	{"getGWRomID",          lua_getcard},
	{"getKernel",           lua_getK},
	{"takeScreenshot",      lua_screenshot},
	{"currentDirectory",    lua_curdir},
	{"checkBuild",          lua_checkbuild},
	{"renameDirectory",     lua_rendir},
	{"createDirectory",     lua_createdir},
	{"deleteDirectory",     lua_deldir},
	{"renameFile",          lua_renfile},
	{"deleteFile",          lua_delfile},
	{"doesFileExist",       lua_checkexist},
	{"listDirectory",       lua_listdir},
	{"getBatteryLife",      lua_batterylv},
	{"isBatteryCharging",   lua_batterycharge},
	{"getLanguage",         lua_getLang},
	{"launch3DSX",          lua_launch},
	{"launchCIA",           lua_launchCia},
	{"launchPayload",       lua_brahmaloader},
	{"extractSMDH",         lua_readsmdh},
	{"scanExtdata",         lua_listExtdata},
	{"listExtdataDir",      lua_listExtdataDir},
	{"installCIA",          lua_installCia},
	{"listCIA",             lua_listCia},
	{"uninstallCIA",        lua_uninstallCia},
	{"extractCIA",          lua_ciainfo},
	{"getRegion",           lua_getRegion},
	{"extractZIP",          lua_ZipExtract},
	{"getModel",            lua_model},
	{"showHomeMenu",        lua_syscall1},
	{"checkStatus",         lua_appstatus},
	{"reboot",              lua_reboot},
	{"launchGamecard",      lua_startcard},
	{"getFreeSpace",        lua_freespace},
	{"getTime",             lua_gettime},
	{"getDate",             lua_getdate},
	{"getUsername",         lua_getUsername},
	{"getBirthday",         lua_getBirth},
	{"addNews",             lua_addnews},
	{"listNews",            lua_listnews},
	{"getNews",             lua_getnews},
	{"eraseNews",           lua_erasenews},
	{"setCpuSpeed",         lua_setcpu},
	{"getCpuSpeed",         lua_getcpu},
	{"extractFromZIP",      lua_getfilefromzip},
	{"checkSDMC",           lua_detectsd},
	{"fork",                lua_dup},
// I/O Module and Dofile Patch
	{"openFile",            lua_openfile},
	{"getFileSize",         lua_getsize},
	{"closeFile",           lua_closefile},
	{"readFile",            lua_readfile},
	{"writeFile",           lua_writefile},
	{"dofile",              lua_dofile},
// End I/O Module and Dofile Patch
	{0, 0}
};

void luaSystem_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, System_functions, 0);
	lua_setglobal(L, "System");
	VariableRegister(L,APP_EXITING);
	VariableRegister(L,APP_RUNNING);
	u8 FREAD = 0;
	u8 FWRITE = 1;
	u8 FCREATE = 2;
	u8 NAND = 0;
	u8 SDMC = 1;
	VariableRegister(L,FREAD);
	VariableRegister(L,FWRITE);
	VariableRegister(L,FCREATE);
	VariableRegister(L,NAND);
	VariableRegister(L,SDMC);
	VariableRegister(L,OLD_3DS_CLOCK);
	VariableRegister(L,NEW_3DS_CLOCK);
}
