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

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

int FREAD = 0;
int FWRITE = 1;
int FCREATE = 2;

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
  lua_CFunction dofilecont = (lua_CFunction)(lua_gettop(L) - 1);
  lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return (int)dofilecont;
}

static int lua_openfile(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	const char *file_tbo = luaL_checkstring(L, 1);
	int type = luaL_checkint(L, 2);
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, file_tbo);
	Result ret;
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
	lua_pushnumber(L,fileHandle);
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
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Handle fileHandle = luaL_checknumber(L, 1);
	Result ret=FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
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
	lua_pushnumber(L,osGetFirmVersion());
    return 1;
}

static int lua_getLang(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u8 language;
	CFGU_GetSystemLanguage(&language);
	lua_pushnumber(L,language);
    return 1;
}

static int lua_getK(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushnumber(L,osGetKernelVersion());
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
    return 1;
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
			lua_pushnumber(L, i++);
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
	lua_pushnumber(L,batteryLevel);
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

static int lua_wifistat(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	u32 wifiStatus;
	ACU_GetWifiStatus(NULL, &wifiStatus);
	lua_pushboolean(L,wifiStatus);
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
		FillScreenEmptyRect(5,200,5,83,0x000000,1,0);
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
			else strncpy(letter,&keychar1[key_pos/12][key_pos%12],1);
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
		gfxSwapBuffers();
	}
	char result[256];
	strcpy(result,console->text);
	free(console);
	lua_pushstring(L, result);
	return 1;
}

//Register our System Functions
static const luaL_Reg System_functions[] = {
  {"exit",					lua_exit},
  {"getFirmware",			lua_getFW},
  {"isGWMode",				lua_isGW},
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
  {"isWifiEnabled",			lua_wifistat},
  {"getLanguage",			lua_getLang},
  {"startKeyboard",			lua_keyboard},
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
	VariableRegister(L,FREAD);
	VariableRegister(L,FWRITE);
	VariableRegister(L,FCREATE);
}