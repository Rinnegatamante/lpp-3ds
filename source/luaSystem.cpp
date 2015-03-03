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
#include "include/Archives.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

int FREAD = 0;
int FWRITE = 1;
int FCREATE = 2;
int NAND = 0;
int SDMC = 1;

FS_archive main_extdata_archive;

void unicodeToChar(char* dst, u16* src)
{
if(!src || !dst)return;
while(*src)*(dst++)=(*(src++))&0xFF;
*dst=0x00;
}

static int lua_exit(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	return luaL_error(L, string); // NOTE: This is a fake error
}

static int lua_dofile (lua_State *L) {
  int argc = lua_gettop(L);
  if (argc != 1) return luaL_error(L, "wrong number of arguments");
  const char *fname = luaL_checkstring(L, 1);
  Handle fileHandle;
  u64 size;
  u32 bytesRead;
  unsigned char *buffer;
  FS_path filePath=FS_makePath(PATH_CHAR, fname);
  FS_archive script=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
  FSUSER_OpenFileDirectly(NULL, &fileHandle, script, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
  FSFILE_GetSize(fileHandle, &size);
  buffer = (unsigned char*)(malloc((size+1) * sizeof (char)));
  FSFILE_Read(fileHandle, &bytesRead, 0x0, buffer, size);
  buffer[size]=0;
  FSFILE_Close(fileHandle);
  svcCloseHandle(fileHandle);
  lua_settop(L, 1);
  if (luaL_loadbuffer(L, (const char*)buffer, strlen((const char*)buffer), NULL) != LUA_OK)
    return lua_error(L);
  lua_KFunction dofilecont = (lua_KFunction)(lua_gettop(L) - 1);
  lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return (int)dofilecont;
}

static int lua_openfile(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 2) && (argc != 3)) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	int type = luaL_checkinteger(L, 2);
	u64 archive_id;
	bool extdata = false;
	if (argc == 3){
		archive_id = luaL_checknumber(L,3);
		extdata = true;
	}
	Handle fileHandle;
	Result ret;
	if (extdata){
		mediatypes_enum mtype;
		FS_archiveIds atype;
		if (archive_id < 0x2000){
			mtype = mediatype_SDMC;
			atype = ARCH_EXTDATA;
		}else{
			mtype = mediatype_NAND;
			atype = ARCH_SHARED_EXTDATA;
		}
		u32 main_extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
		FS_archive main_extdata_archive = (FS_archive){atype, (FS_path){PATH_BINARY, 0xC, (u8*)main_extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive(NULL, &main_extdata_archive);
		if(ret!=0) return luaL_error(L, "cannot access extdata archive");
		switch(type){
			case 0:
				ret = FSUSER_OpenFile(NULL, &fileHandle, main_extdata_archive, FS_makePath(PATH_CHAR, file_tbo), FS_OPEN_READ, 0);
				break;
			case 1:
				ret = FSUSER_OpenFile(NULL, &fileHandle, main_extdata_archive, FS_makePath(PATH_CHAR, file_tbo), FS_OPEN_WRITE, 0);
				break;
		}
	}else{
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
		switch(type){
			case 0:
				ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
				break;
			case 1:
				ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
				break;
			case 2:
				ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
				break;
		}
		if(ret) return luaL_error(L, "error opening file");
	}
	lua_pushinteger(L,fileHandle);
	return 1;
}

static int lua_checkexist(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	if (!ret){
	FSFILE_Close(fileHandle);
	}
	svcCloseHandle(fileHandle);
	lua_pushboolean(L,!ret);
	return 1;
}

static int lua_isGW(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushboolean(L,GW_MODE);
	return 1;
}

static int lua_is3DSX(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushboolean(L,is3DSX);
	return 1;
}

static int lua_getRegion(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8 region;
	CFGU_SecureInfoGetRegion(&region);
	lua_pushinteger(L,region);
	return 1;
}

static int lua_screenshot(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *screenpath = luaL_checkstring(L, 1);
	Handle fileHandle;
	int x, y;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, screenpath);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
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
	u8* framebuf = (u8*)gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 400; x++)
		{
			int si = ((239 - y) + (x * 240)) * 3;
			int di = 0x36 + (x + ((479 - y) * 400)) * 3;
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
			tempbuf[di++] = framebuf[si++];
		}
	}
	framebuf = (u8*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
	for (y = 0; y < 240; y++)
	{
		for (x = 0; x < 320; x++)
		{
		int si = ((239 - y) + (x * 240)) * 3;
		int di = 0x36 + ((x+40) + ((239 - y) * 400)) * 3;
		tempbuf[di++] = framebuf[si++];
		tempbuf[di++] = framebuf[si++];
		tempbuf[di++] = framebuf[si++];
		}
	}
	FSFILE_Write(fileHandle, &bytesWritten, 0, (u32*)tempbuf, 0x36 + 576000, 0x10001);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	free(tempbuf);
	return 0;
}

static int lua_getsize(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 size;
	Result ret=FSFILE_GetSize(fileHandle, &size);
	if(ret) return luaL_error(L, "error getting size");
	lua_pushnumber(L,size);
	return 1;
}

static int lua_closefile(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 1) && (argc != 2)) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	Result ret=FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	if (argc == 2) FSUSER_CloseArchive(NULL, &main_extdata_archive);
	if(ret) return luaL_error(L, "error closing file");
	return 0;
}

static int lua_readfile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 init = luaL_checknumber(L, 2);
	u64 size = luaL_checknumber(L, 3);
	u32 bytesRead;
	unsigned char *buffer = (unsigned char*)(malloc((size+1) * sizeof (char)));
	Result ret=FSFILE_Read(fileHandle, &bytesRead, init, buffer, size);
	buffer[size] = 0;
	if(ret || size!=bytesRead) return luaL_error(L, "error reading file");
	lua_pushlstring(L,(const char*)buffer,size);
	free(buffer);
	return 1;
}


static int lua_writefile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 4) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	u64 init = luaL_checknumber(L, 2);
	const char *text = luaL_checkstring(L, 3);
	u64 size = luaL_checknumber(L, 4);
	u32 bytesWritten;
	Result ret=FSFILE_Write(fileHandle, &bytesWritten, init, text, size, FS_WRITE_FLUSH);
	if(ret || size!=bytesWritten) return luaL_error(L, "error writing file");
	return 0;
}

static int lua_getFW(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushinteger(L,osGetFirmVersion());
    return 1;
}

static int lua_getLang(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8 language;
	CFGU_GetSystemLanguage(&language);
	lua_pushinteger(L,language);
    return 1;
}

static int lua_getK(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushinteger(L,osGetKernelVersion());
    return 1;
}

static int lua_getCurrentDirectory(lua_State *L)
{
    lua_pushstring(L, cur_dir);
    return 1;
}

static int lua_setCurrentDirectory(lua_State *L)
{
    const char *path = luaL_checkstring(L, 1);
    if(!path) return luaL_error(L, "System.currentDirectory(file) takes a filename as a string argument.");
    strcpy(cur_dir,path);
    return 0;
}

static int lua_curdir(lua_State *L) {
    int argc = lua_gettop(L);
    if(argc == 0) return lua_getCurrentDirectory(L);
    if(argc == 1) return lua_setCurrentDirectory(L);
    return luaL_error(L, "System.currentDirectory([file]) takes zero or one argument");
}

static int lua_rendir(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	const char *path2 = luaL_checkstring(L, 2);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FS_path filePath2=FS_makePath(PATH_CHAR, path2);
	FSUSER_RenameDirectory(NULL,sdmcArchive,filePath,sdmcArchive,filePath2);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
    return 0;
}

static int lua_createdir(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FSUSER_CreateDirectory(NULL,sdmcArchive,filePath);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
    return 0;
}

static int lua_deldir(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FSUSER_DeleteDirectory(NULL,sdmcArchive,filePath);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
    return 0;
}

static int lua_delfile(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FSUSER_DeleteFile(NULL,sdmcArchive,filePath);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
    return 0;
}

static int lua_renfile(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	const char *path2 = luaL_checkstring(L, 2);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FS_path filePath2=FS_makePath(PATH_CHAR, path2);
	FSUSER_RenameFile(NULL,sdmcArchive,filePath,sdmcArchive,filePath2);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
    return 0;
}

static int lua_listdir(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char *path = luaL_checkstring(L, 1);
	Handle dirHandle;
	FS_dirent entry;
	FS_path dirPath=FS_makePath(PATH_CHAR, path);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FSUSER_OpenDirectory(NULL, &dirHandle, sdmcArchive, dirPath);
	u32 entriesRead;
	lua_newtable(L);
	int i = 1;
	static char name[1024];
	for (;;){
		entriesRead=0;
		FSDIR_Read(dirHandle, &entriesRead, 1, &entry);
		if (entriesRead){
			lua_pushinteger(L, i++);
			lua_newtable(L);
			lua_pushstring(L, "name");
			unicodeToChar(&name[0],entry.name);
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
	FSUSER_CloseArchive(NULL, &sdmcArchive);
	return 1;
}

static int lua_batterylv(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8 batteryLevel;
	PTMU_GetBatteryLevel(NULL, &batteryLevel);
	lua_pushinteger(L,batteryLevel);
	return 1;
}

static int lua_batterycharge(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8 batteryLevel;
	PTMU_GetBatteryChargeState(NULL, &batteryLevel);
	lua_pushboolean(L,batteryLevel);
	return 1;
}

static int lua_keyboard(lua_State *L){
	int argc = lua_gettop(L);
	if ((argc != 0) && (argc != 1)) return luaL_error(L, "wrong number of arguments");
	Console* console = (Console*)malloc(sizeof(Console));
	console->screen = 0;
	if (argc == 1) strcpy(console->text, luaL_checkstring(L, 1));
	else strcpy(console->text,"");
	int key_pos = 0;
	bool maiusc = true;
	char* keychar1[4] = {"1234567890()","QWERTYUIOP[]","ASDFGHJKL+- ","ZXCVBNM,.   "}; // Last 3 are Maiusc/Del/Enter
	char* keychar2[4] = {"1234567890!$","qwertyuiop{}","asdfghjkl@=_","zxcvbnm;%   "}; // Last 3 are Maiusc/Del/Enter
	char letter[2];
	for (;;){
		gspWaitForVBlank();
		RefreshScreen();
		ClearScreen(0);
		ClearScreen(1);
		FillScreenRect(6,199,6,82,0xFFFFFF,1,0);
		ConsoleOutput(console);
		if (maiusc){
		int i=0;
		while (i < 4){
			int j=0;
			while (j < 12){
				u32 color;
				if (key_pos == (j+(i*12))){
				if (key_pos < 46) FillScreenRect(6+j*15,j*15+16,i*15+6,i*15+19,0xFFFF00,1,0);
				else if (key_pos == 46) FillScreenRect(26,36,(i+1)*15+6,(i+1)*15+19,0xFFFF00,1,0);
				else FillScreenRect(76,86,(i+1)*15+6,(i+1)*15+19,0xFFFF00,1,0);
				color = 0xFF0000;
				}
				else color = 0x000000;
				if ((j < 9) || (i < 3)){
				strncpy(letter,&keychar1[i][j],1);
				letter[1] = 0;
				DrawScreenText(8+(j*15),8+(i*15),letter,color,1,0);
				}else{
					if (j == 9) DrawScreenText(8+(j*15),8+(i*15),"DEL",color,1,0);
					if (j == 10) DrawScreenText(28,8+((i+1)*15),"CAPS",color,1,0);
					if (j == 11) DrawScreenText(78,8+((i+1)*15),"ENTER",color,1,0);
				}
				j++;
			}
			i++;
		}		
		}else{
		int i=0;
		while (i < 4){
			int j=0;
			while (j < 12){
				u32 color;
				if (key_pos == (j+(i*12))){
				if (key_pos < 46) FillScreenRect(6+j*15,j*15+16,i*15+6,i*15+19,0xFFFF00,1,0);
				else if (key_pos == 46) FillScreenRect(26,36,(i+1)*15+6,(i+1)*15+19,0xFFFF00,1,0);
				else FillScreenRect(76,86,(i+1)*15+6,(i+1)*15+19,0xFFFF00,1,0);
				color = 0xFF0000;
				}
				else color = 0x000000;
				if ((j < 9) || (i < 3)){
				strncpy(letter,&keychar2[i][j],1);
				letter[1] = 0;
				DrawScreenText(8+(j*15),8+(i*15),letter,color,1,0);
				}else{
					if (j == 9) DrawScreenText(8+(j*15),8+(i*15),"DEL",color,1,0);
					if (j == 10) DrawScreenText(28,8+((i+1)*15),"CAPS",color,1,0);
					if (j == 11) DrawScreenText(78,8+((i+1)*15),"ENTER",color,1,0);
				}
				j++;
			}
			i++;
		}
		}
		hidScanInput();
		irrstScanInput();
		if ((hidKeysDown() & KEY_A) == KEY_A){
		if (key_pos == 47) break;
		else if (key_pos == 46) maiusc = !maiusc;
		else if ((key_pos == 45) && (strlen(console->text) > 0)){
			console->text[strlen(console->text)-1] = 0;
		}else{
			if (maiusc) strncpy(letter,&keychar1[key_pos/12][key_pos%12],1);
			else strncpy(letter,&keychar2[key_pos/12][key_pos%12],1);
			letter[1] = 0;
			strcat(console->text,letter);
		}
		}else if ((hidKeysDown() & KEY_DUP) == KEY_DUP){
			if ((key_pos < 46) && (key_pos > 11)) key_pos = key_pos - 12;
			else if (key_pos == 46) key_pos = 37;
			else if (key_pos < 5) key_pos = 46;
			else if (key_pos < 12) key_pos = 47;
			else key_pos = 41;
		}else if ((hidKeysDown() & KEY_DDOWN) == KEY_DDOWN){
			if (key_pos < 34) key_pos = key_pos + 12;
			else if ((key_pos == 35) || (key_pos == 34)) key_pos = 45;
			else if (key_pos == 45) key_pos = 9;
			else if (key_pos == 46) key_pos = 1;
			else if (key_pos == 47) key_pos = 5;
			else if (key_pos < 41) key_pos = 46;
			else key_pos = 47;
		}
		else if ((hidKeysDown() & KEY_DLEFT) == KEY_DLEFT){
		key_pos = key_pos - 1;
		}
		else if ((hidKeysDown() & KEY_DRIGHT) == KEY_DRIGHT){
		key_pos = key_pos + 1;
		}
		if (key_pos > 47) key_pos = 0;
		else if (key_pos < 0) key_pos = 47;
		gfxFlushBuffers();
		gfxSwapBuffers();
	}
	char result[256];
	strcpy(result,console->text);
	free(console);
	lua_pushstring(L, result);
	return 1;
}

static inline void putPixel565(u8* dst, u8 x, u8 y, u16 v)
{
dst[(x+(47-y)*48)*3+0]=(v&0x1F)<<3;
dst[(x+(47-y)*48)*3+1]=((v>>5)&0x3F)<<2;
dst[(x+(47-y)*48)*3+2]=((v>>11)&0x1F)<<3;
}

static int lua_readsmdh(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char* file = luaL_checkstring(L, 1);
	char name[64];
	char desc[128];
	char author[64];
	Handle fileHandle;
	u32 bytesRead;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file);
	FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	u32 magic;
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 4);
	if (magic != 0x48444D53) return luaL_error(L, "error opening SMDH file");
	unsigned char *buffer = (unsigned char*)(malloc((129) * sizeof (char)));
	buffer[128] = 0;
	FSFILE_Read(fileHandle, &bytesRead, 8, buffer, 128);
	int i = 0;
	while (i < 129){
	if (buffer[i*2] == 0) break;
	else name[i] = buffer[i*2];
	i++;
	}
	name[i] = 0;
	FSFILE_Read(fileHandle, &bytesRead, 392, buffer, 128);
	i = 0;
	while (i < 129){
	if (buffer[i*2] == 0) break;
	else author[i] = buffer[i*2];
	i++;
	}
	author[i] = 0;
	free(buffer);
	buffer = (unsigned char*)(malloc((257) * sizeof (char)));
	buffer[256] = 0;
	FSFILE_Read(fileHandle, &bytesRead, 136, buffer, 256);
	i = 0;
	while (i < 257){
	if (buffer[i*2] == 0) break;
	else desc[i] = buffer[i*2];
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
	FSFILE_Read(fileHandle, &bytesRead, 0x24C0, icon_buffer, 0x1200);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
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
			putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)*64)] + extra_x, tile_y[i-((tile_number-1)*64)] + extra_y, icon_buffer[i]);
			putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)*64)] + extra_x, tile_y[i-((tile_number-1)*64)] + extra_y, icon_buffer[i+16]);
			putPixel565(bitmap->pixels, tile_x[i-((tile_number-1)*64)] + extra_x, 4+tile_y[i-((tile_number-1)*64)] + extra_y, icon_buffer[i+32]);
			putPixel565(bitmap->pixels, 4+tile_x[i-((tile_number-1)*64)] + extra_x, 4+tile_y[i-((tile_number-1)*64)] + extra_y, icon_buffer[i+48]);
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
void (*callBootloader)(Handle hb, Handle file);
void (*setArgs)(u32* src, u32 length);
static void launchFile(void){ callBootloader(0x00000000, hbHandle); }

static int lua_launch(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char* file = luaL_checkstring(L, 1);
	HB_GetBootloaderAddresses((void**)&callBootloader, (void**)&setArgs);
	fsExit();
	fsInit();
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FSUSER_OpenFileDirectly(NULL, &hbHandle, sdmcArchive, FS_makePath(PATH_CHAR, file), FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	static u32 argbuffer[0x200];
	argbuffer[0]=1;
	snprintf((char*)&argbuffer[1], 0x200*4, "sdmc:%s", file);
	setArgs(argbuffer, 0x200*4);
	__system_retAddr = launchFile;
	char string[20];
	strcpy(string,"lpp_exit_0456432");
	return luaL_error(L, string); // NOTE: This is a fake error
}

static int lua_listExtdata(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_newtable(L);
	int z = 1;
	Handle extdata_dir;
	static char name[1024];
	u64 i;
	for (i=0; i<0x2000; ++i) {
		u32 extdata_archive_lowpathdata[3] = {mediatype_SDMC, i, 0};
		FS_archive extdata_archive = (FS_archive){ARCH_EXTDATA, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, &entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				unicodeToChar(&name[0],entry.name);
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
		FSUSER_CloseArchive(NULL, &extdata_archive);
	}
	for (i=0xE0000000; i<0xE0000100; ++i) {
		u32 extdata_archive_lowpathdata[3] = {mediatype_NAND, i, 0};
		FS_archive extdata_archive = (FS_archive){ARCH_SHARED_EXTDATA, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, &entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				unicodeToChar(&name[0],entry.name);
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
		FSUSER_CloseArchive(NULL, &extdata_archive);
	}
	for (i=0xF0000000; i<0xF0000100; ++i) {
		u32 extdata_archive_lowpathdata[3] = {mediatype_NAND, i, 0};
		FS_archive extdata_archive = (FS_archive){ARCH_SHARED_EXTDATA, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
		Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
		if(ret!=0) continue;
		FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, "/"));
		FS_dirent entry;
		for (;;){
			u32 entriesRead=0;
			FSDIR_Read(extdata_dir, &entriesRead, 1, &entry);
			if (entriesRead){
				lua_pushinteger(L, z++);
				lua_newtable(L);
				lua_pushstring(L, "name");
				unicodeToChar(&name[0],entry.name);
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
		FSUSER_CloseArchive(NULL, &extdata_archive);
	}
	svcCloseHandle(extdata_dir);
	return 1;
}

static int lua_listExtdataDir(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char* path = luaL_checkstring(L, 1);
	u64 archive_id = luaL_checknumber(L, 2);
	lua_newtable(L);
	int z = 1;
	static char name[1024];
	mediatypes_enum mtype;
	FS_archiveIds atype;
	if (archive_id < 0x2000){
		mtype = mediatype_SDMC;
		atype = ARCH_EXTDATA;
	}else{
		mtype = mediatype_NAND;
		atype = ARCH_SHARED_EXTDATA;
	}
	u32 extdata_archive_lowpathdata[3] = {mtype, archive_id, 0};
	FS_archive extdata_archive = (FS_archive){atype, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
	Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
	if(ret!=0) return luaL_error(L, "cannot access extdata archive");
	Handle extdata_dir;
	FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, path));
	FS_dirent entry;
	for (;;){
		u32 entriesRead=0;
		FSDIR_Read(extdata_dir, &entriesRead, 1, &entry);
		if (entriesRead){
			lua_pushinteger(L, z++);
			lua_newtable(L);
			lua_pushstring(L, "name");
			unicodeToChar(&name[0],entry.name);
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
	FSUSER_CloseArchive(NULL, &extdata_archive);
	svcCloseHandle(extdata_dir);	
	return 1;
}


//AM service support, partially stolen by libctru
static Handle amHandle = 0;

Result amInit()
{
	if(srvGetServiceHandle(&amHandle, "am:net") == 0)
		return (Result)0;
	else
		return srvGetServiceHandle(&amHandle, "am:u");
}

Result amExit()
{
	return svcCloseHandle(amHandle);
}

Result AM_StartCiaInstall(u8 mediatype, Handle *ciahandle)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x04020040;
	cmdbuf[1] = mediatype;

	if((ret = svcSendSyncRequest(amHandle))!=0) return ret;

	*ciahandle = cmdbuf[3];
	
	return (Result)cmdbuf[1];
}

Result AM_FinishCiaInstall(u8 mediatype, Handle *ciahandle)
{
	Result ret = 0;
	u32 *cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x04050002;
	cmdbuf[1] = 0x10;
	cmdbuf[2] = *ciahandle;

	if((ret = svcSendSyncRequest(amHandle))!=0) return ret;

	return (Result)cmdbuf[1];
}

Result AM_GetTitleCount(u8 mediatype, u32 *count)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x00010040;
cmdbuf[1] = mediatype;
if((ret = svcSendSyncRequest(amHandle))!=0) return ret;
*count = cmdbuf[2];
return (Result)cmdbuf[1];
}

Result AM_GetTitleList(u8 mediatype, u32 count, void *buffer)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x00020082;
cmdbuf[1] = count;
cmdbuf[2] = mediatype;
cmdbuf[3] = ((count*8) << 4) | 12;
cmdbuf[4] = (u32)buffer;
if((ret = svcSendSyncRequest(amHandle))!=0) return ret;
return (Result)cmdbuf[1];
}

Result AM_GetTitleProductCode(u8 mediatype, u64 titleid, char* product_code)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x000500C0;
cmdbuf[1] = mediatype;
cmdbuf[2] = titleid & 0xffffffff;
cmdbuf[3] = (titleid >> 32) & 0xffffffff;
if((ret = svcSendSyncRequest(amHandle))!=0) return ret;
sprintf(product_code,"%s",(char*)(&cmdbuf[2]));
return (Result)cmdbuf[1];
}

Result AM_DeleteTitle(u8 mediatype, u64 titleid)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x041000C0;
cmdbuf[1] = mediatype;
cmdbuf[2] = titleid & 0xffffffff;
cmdbuf[3] = (titleid >> 32) & 0xffffffff;
if((ret = svcSendSyncRequest(amHandle))!=0) return ret;
return (Result)cmdbuf[1];
}

Result AM_DeleteAppTitle(u8 mediatype, u64 titleid)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x000400C0;
cmdbuf[1] = mediatype;
cmdbuf[2] = titleid & 0xffffffff;
cmdbuf[3] = (titleid >> 32) & 0xffffffff;
if((ret = svcSendSyncRequest(amHandle))!=0) return ret;
return (Result)cmdbuf[1];
}
//Finish AM support

static int lua_installCia(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char* path = luaL_checkstring(L, 1);
	Handle fileHandle;
	Handle ciaHandle;
	u64 size;
	int MAX_RAM_ALLOCATION = 10485760;
	u32 bytes;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, path);
	FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	amInit();
	AM_StartCiaInstall(mediatype_SDMC, &ciaHandle);
	FSFILE_GetSize(fileHandle, &size);
	if (size < MAX_RAM_ALLOCATION){
		char* cia_buffer = (char*)(malloc((size) * sizeof (char)));
		FSFILE_Read(fileHandle, &bytes, 0x0, cia_buffer, size);
		FSFILE_Write(ciaHandle, &bytes, 0, cia_buffer, size, 0x10001);
		free(cia_buffer);
	}else{
		u64 i = 0;
		char* cia_buffer;
		while (i < size){
			u64 bytesToRead;
			if	(i+MAX_RAM_ALLOCATION > size){
				cia_buffer = (char*)(malloc((size-i) * sizeof(char)));
				bytesToRead = size - i;
			}else{
				cia_buffer = (char*)(malloc((MAX_RAM_ALLOCATION) * sizeof(char)));
				bytesToRead = MAX_RAM_ALLOCATION;
			}
			FSFILE_Read(fileHandle, &bytes, i, cia_buffer, bytesToRead);
			FSFILE_Write(ciaHandle, &bytes, i, cia_buffer, bytesToRead, 0x10001);
			i = i + bytesToRead;
			free(cia_buffer);
		}
	}
	AM_FinishCiaInstall(mediatype_SDMC, &ciaHandle);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	amExit();
	return 0;
}

struct TitleId{
	u32 uniqueid;
	u16 category;
	u16 platform;
};
/* CIA categories
0 = Application
1 = System
2 = Demo
3 = Patch
4 = TWL
*/
static int lua_listCia(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	amInit();
	u32 cia_nums;
	AM_GetTitleCount(mediatype_SDMC, &cia_nums);
	TitleId* TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(mediatype_SDMC,cia_nums,TitleIDs);
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
		AM_GetTitleProductCode(mediatype_SDMC, id, product_id);
		lua_pushstring(L, "product_id");
		lua_pushstring(L, product_id);
		lua_settable(L, -3);
		lua_pushstring(L, "access_id");
		lua_pushinteger(L, i);
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
	u32 z = 1;
	AM_GetTitleCount(mediatype_NAND, &cia_nums);
	TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(mediatype_NAND,cia_nums,TitleIDs);
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
		AM_GetTitleProductCode(mediatype_NAND, id, product_id);
		lua_pushstring(L, "product_id");
		lua_pushstring(L, product_id);
		lua_settable(L, -3);
		lua_pushstring(L, "access_id");
		lua_pushinteger(L, z);
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
	amExit();
	return 1;
}

static int lua_uninstallCia(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	u32 delete_id = luaL_checknumber(L,1);
	u32 mediatype = luaL_checknumber(L,2);
	mediatypes_enum media;
	if (mediatype == 1) media = mediatype_SDMC;
	else media = mediatype_NAND;
	amInit();
	u32 cia_nums;
	AM_GetTitleCount(media, &cia_nums);
	TitleId* TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(media,cia_nums,TitleIDs);
	u64 id = TitleIDs[delete_id-1].uniqueid | ((u64)TitleIDs[delete_id-1].category << 32) | ((u64)TitleIDs[delete_id-1].platform << 48);
	AM_DeleteAppTitle(media, id);
	AM_DeleteTitle(media, id);
	amExit();
	free(TitleIDs);
	return 0;
}

static u32 Endian_UInt32_Conversion(u32 value){
   return ((value >> 24) & 0x000000FF) | ((value >> 8) & 0x0000FF00) | ((value << 8) & 0x00FF0000) | ((value << 24) & 0xFF000000);
}

static int lua_ciainfo(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	const char* file = luaL_checkstring(L, 1);
	char title[16];
	Handle fileHandle;
	u32 bytesRead;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file);
	FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	u32 unique_id;
	FSFILE_Read(fileHandle, &bytesRead, 0x3A50, title, 16);
	FSFILE_Read(fileHandle, &bytesRead, 0x2C20, &unique_id, 4);
	lua_newtable(L);
	lua_newtable(L);
	lua_pushstring(L, "title");
	lua_pushstring(L, title);
	lua_settable(L, -3);
	lua_pushstring(L, "unique_id");
	lua_pushinteger(L, Endian_UInt32_Conversion(unique_id));
	lua_settable(L, -3);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	return 1;
}

static int lua_ZipExtract(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 2 && argc != 3)
		return luaL_error(L, "wrong number of arguments.");
	const char *FileToExtract = luaL_checkstring(L, 1);
	const char *DirTe = luaL_checkstring(L, 2);
	const char *Password = (argc == 3) ? luaL_checkstring(L, 3) : NULL;
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path TEMP_PATH=FS_makePath(PATH_CHAR, DirTe);
	FSUSER_CreateDirectory(NULL,sdmcArchive,TEMP_PATH);
	FSUSER_CloseArchive(NULL, &sdmcArchive);
	char tmpFile2[1024];
	char tmpPath2[1024];
	sdmcInit();
	strcpy(tmpPath2,"sdmc:");
	strcat(tmpPath2,(char*)DirTe);
	chdir(tmpPath2);
	strcpy(tmpFile2,"sdmc:");
	strcat(tmpFile2,(char*)FileToExtract);
	Zip *handle = ZipOpen(tmpFile2);
	if (handle == NULL) luaL_error(L, "error opening ZIP file.");
	int result = ZipExtract(handle, Password);
	ZipClose(handle);
	sdmcExit();
	lua_pushinteger(L, result);
	return 1;
}

static int lua_RarExtract(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 2)
		return luaL_error(L, "wrong number of arguments.");
	const char *FileToExtract = luaL_checkstring(L, 1);
	const char *DirTe = luaL_checkstring(L, 2);
	FS_archive sdmcArchive = (FS_archive){0x9, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenArchive(NULL, &sdmcArchive);
	FS_path TEMP_PATH=FS_makePath(PATH_CHAR, DirTe);
	FSUSER_CreateDirectory(NULL,sdmcArchive,TEMP_PATH);
	char tmpFile2[1024];
	char tmpPath2[1024];
	sdmcInit();
	strcpy(tmpPath2,"sdmc:");
	strcat(tmpPath2,(char*)DirTe);
	chdir(tmpPath2);
	strcpy(tmpFile2,"sdmc:");
	strcat(tmpFile2,(char*)FileToExtract);
	unrar_t* inp;
	unrar_open( &inp, tmpFile2 );
	Handle fileHandle;
	u32 bytesWritten;
	while (!unrar_done(inp)){
		unrar_info_t* file_info = (unrar_info_t*)malloc(sizeof(unrar_info_t));
		file_info = (unrar_info_t*)unrar_info(inp);
		u8* buffer = (u8*)malloc(file_info->size);
		unrar_extract(inp, buffer, file_info->size);
		char fname[256];
		strcpy(fname,DirTe);
		strcat(fname,file_info->name);
		FS_path filePath=FS_makePath(PATH_CHAR, fname);
		FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
		FSFILE_Write(fileHandle, &bytesWritten, 0, buffer, file_info->size, 0x10001);
		FSFILE_Close(fileHandle);
		free(buffer);
		free(file_info);
		unrar_next(inp);
	}
	unrar_close(inp);
	sdmcExit();
	FSUSER_CloseArchive(NULL, &sdmcArchive);
	return 0;
}

static int lua_model(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	u8 model;
	CFGU_GetSystemModel(&model);
	lua_pushinteger(L,model);
	return 1;
}
	
static int lua_syscall1(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	aptReturnToMenu();
	return 0;
}

static int lua_appstatus(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	APP_STATUS status = aptGetStatus();
	lua_pushinteger(L,status);
	return 1;
}

static int lua_reboot(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	aptOpenSession();
	APT_HardwareResetAsync(NULL);
	aptCloseSession();
	return 0;
}

static Handle nsHandle;

Result NS_RebootToTitle(u8 mediatype, u64 titleid)
{
Result ret = 0;
u32 *cmdbuf = getThreadCommandBuffer();
cmdbuf[0] = 0x00100180;
cmdbuf[1] = 0x1;
cmdbuf[2] = titleid & 0xffffffff;
cmdbuf[3] = (titleid >> 32) & 0xffffffff;
cmdbuf[4] = mediatype;
cmdbuf[5] = 0x0; // reserved
cmdbuf[6] = 0x0;
if((ret = svcSendSyncRequest(nsHandle))!=0)return ret;
return (Result)cmdbuf[1];
}

static int lua_startcard(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	amInit();
	char product_id[16];
	AM_GetTitleProductCode(mediatype_GAMECARD, 0, product_id);
	amExit();
	if (product_id[0] == 'C' and product_id[1] == 'T' and product_id[2] == 'R'){
		u8 buf0[0x300];
		u8 buf1[0x20];
		memset(buf0, 0, 0x300);
		memset(buf1, 0, 0x20);
		aptOpenSession();
		APT_PrepareToDoAppJump(NULL, 0, 0, mediatype_GAMECARD);
		APT_DoAppJump(NULL, 0x300, 0x20, buf0, buf1);
		aptCloseSession();
	}else{
		srvGetServiceHandle(&nsHandle, "ns:s"); 
		NS_RebootToTitle(mediatype_GAMECARD,0);
		svcCloseHandle(nsHandle);
	}
	for (;;){}
	return 0;
}

static int lua_getcard(lua_State *L) {
	int argc = lua_gettop(L);
	if(argc != 0 ) return luaL_error(L, "wrong number of arguments.");
	amInit();
	char product_id[16];
	AM_GetTitleProductCode(mediatype_GAMECARD, 0, product_id);
	amExit();
	if (product_id[0] == 'C' and product_id[1] == 'T' and product_id[2] == 'R') lua_pushstring(L,product_id);
	else lua_pushstring(L,"");
	return 1;
}

static int lua_freespace(lua_State *L) {
int argc = lua_gettop(L);
if (argc != 0) return luaL_error(L, "wrong number of arguments");
u32 freeBlocks;
u32 blockSize;
FSUSER_GetSdmcArchiveResource(NULL, NULL, &blockSize, NULL, &freeBlocks);
lua_pushnumber(L,freeBlocks*blockSize);
return 1;
}

static int lua_launchCia(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	u32 delete_id = luaL_checkinteger(L,1);
	u32 mediatype = luaL_checkinteger(L,2);
	mediatypes_enum media;
	if (mediatype == 1) media = mediatype_SDMC;
	else if (mediatype == 0) media = mediatype_NAND;
	else media = mediatype_GAMECARD;
	amInit();
	u32 cia_nums;
	AM_GetTitleCount(media, &cia_nums);
	TitleId* TitleIDs = (TitleId*)malloc(cia_nums * sizeof(TitleId));
	AM_GetTitleList(media,cia_nums,TitleIDs);
	u64 id = TitleIDs[delete_id-1].uniqueid | ((u64)TitleIDs[delete_id-1].category << 32) | ((u64)TitleIDs[delete_id-1].platform << 48);
	free(TitleIDs);
	amExit();
	u8 buf0[0x300];
	u8 buf1[0x20];
	memset(buf0, 0, 0x300);
	memset(buf1, 0, 0x20);
	aptOpenSession();
	APT_PrepareToDoAppJump(NULL, 0, id, mediatype);
	APT_DoAppJump(NULL, 0x300, 0x20, buf0, buf1);
	aptCloseSession();
	for (;;){}
	return 0;
}

static int lua_gettime(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
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
u8 day_values[7] = {4,5,6,7,1,2,3};

static int lua_getdate(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u64 time = (osGetTime() / 1000) - 3629059200; // Time from 1st January 2015
	u32 day_value = ((time / 86400) % 7);
	u32 year = 2015;
	u8 day = 1;
	u8 month = 1;
	u32 control;
	if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) control = 31622400;
	else control = 31536000;
	while (time > control){
		year = year + 1;
		time = time - control;
		if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) control = 31622400;
		else control = 31536000;
	}
	bool extended = false;
	if (year % 400 == 0 || (year % 100 != 0 && year % 4 == 0)) extended = true;
	control = month_seconds[2];
	while (time > control){
		month = month + 1;
		time = time - control;
		if ((month == 11) || (month == 4) || (month == 6) || (month == 9)) control = month_seconds[0];
		else if (month == 2){
			if (extended) control = month_seconds[3];
			else control = month_seconds[1];
		}else{
		control = month_seconds[2];
		}
	}
	day = day + time / 86400;
	lua_pushinteger(L,day_values[day_value]);
	lua_pushinteger(L,day);
	lua_pushinteger(L,month);
	lua_pushinteger(L,year);
	return 4;
}

//Register our System Functions
static const luaL_Reg System_functions[] = {
  {"exit",					lua_exit},
  {"getFirmware",			lua_getFW},
  {"getGWRomID",			lua_getcard},
  {"isGWMode",				lua_isGW},
  {"is3DSXMode",			lua_is3DSX},
  {"getKernel",				lua_getK},
  {"takeScreenshot",		lua_screenshot},
  {"currentDirectory",		lua_curdir},
  {"renameDirectory",		lua_rendir},
  {"createDirectory",		lua_createdir},
  {"deleteDirectory",		lua_deldir},
  {"renameFile",			lua_renfile},
  {"deleteFile",			lua_delfile},
  {"doesFileExist",			lua_checkexist},
  {"listDirectory",			lua_listdir},
  {"getBatteryLife",		lua_batterylv},
  {"isBatteryCharging",		lua_batterycharge},
  {"getLanguage",			lua_getLang},
  {"startKeyboard",			lua_keyboard},
  {"launch3DSX",			lua_launch},
  {"launchCIA",				lua_launchCia},
  {"extractSMDH",			lua_readsmdh},
  {"scanExtdata",			lua_listExtdata},
  {"listExtdataDir",		lua_listExtdataDir},
  {"installCIA",			lua_installCia},
  {"listCIA",				lua_listCia},
  {"uninstallCIA",			lua_uninstallCia},
  {"extractCIA",			lua_ciainfo},
  {"getRegion",				lua_getRegion},
  {"extractZIP",			lua_ZipExtract},
  {"extractRAR",			lua_RarExtract},
  {"getModel",				lua_model},
  {"showHomeMenu",			lua_syscall1},
  {"checkStatus",			lua_appstatus},
  {"reboot",				lua_reboot},
  {"launchGamecard",		lua_startcard},
  {"getFreeSpace",			lua_freespace},
  {"getTime",				lua_gettime},
  {"getDate",				lua_getdate},
// I/O Module and Dofile Patch
  {"openFile",				lua_openfile},
  {"getFileSize",			lua_getsize},
  {"closeFile",				lua_closefile},
  {"readFile",				lua_readfile},
  {"writeFile",				lua_writefile},
  {"dofile",				lua_dofile},
// End I/O Module and Dofile Patch
  {0, 0}
};

void luaSystem_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, System_functions, 0);
	lua_setglobal(L, "System");
	VariableRegister(L,APP_EXITING);
	VariableRegister(L,APP_RUNNING);
	VariableRegister(L,FREAD);
	VariableRegister(L,FWRITE);
	VariableRegister(L,FCREATE);
	VariableRegister(L,NAND);
	VariableRegister(L,SDMC);
}