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
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/luaGraphics.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

static int lua_print(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 5) && (argc != 6)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
	char* text = (char*)(luaL_checkstring(L, 3));
	u32 color = luaL_checknumber(L,4);
	int screen = luaL_checknumber(L,5);
	int side;
	if (argc == 6){
	side = luaL_checkint(L,6);
	}else{
	side = 0;
	}
	DrawText(x,y,text,color,screen,side);
	return 0;
}

static int lua_enable3D(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	gfxSet3D(true);
	return 0;
}

static int lua_disable3D(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	gfxSet3D(false);
	return 0;
}

static int lua_get3D(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	lua_pushnumber(L, CONFIG_3D_SLIDERSTATE);
	return 1;
}

static int lua_bitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	char* text = (char*)(luaL_checkstring(L, 1));
	Bitmap *bitmap = (Bitmap*)malloc(sizeof(Bitmap));
	*bitmap = LoadBitmap(text);
    lua_pushnumber(L, (u32)(bitmap));
	return 1;
}

static int lua_pbitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkint(L, 1);
    int y = luaL_checkint(L, 2);
	Bitmap* file = (Bitmap*)luaL_checkint(L, 3);
	int screen= luaL_checkint(L, 4);
	int side;
	if (argc == 5){
	side = luaL_checkint(L,5);
	}else{
	side = 0;
	}
	PrintBitmap(x,y,*file,screen,side);
	return 0;
}

static int lua_flipBitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	u8* not_flipped;
	Bitmap* src = (Bitmap*)luaL_checkint(L, 1);
	Bitmap* dst = (Bitmap*)luaL_checkint(L, 2);
	not_flipped = dst->pixels;
	u8* flip_pixels = (u8*)malloc((src->width)*(src->height)*3);
	dst->pixels = flipBitmap(flip_pixels, *src);
	dst->width = src->width;
	dst->height = src->height;
	free(not_flipped);
	return 0;
}

static int lua_saveimg(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkint(L, 1);
	char* text = (char*)(luaL_checkstring(L, 2));
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, text);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
	u32 bytesWritten;
	u8* tempbuf = (u8*)malloc(0x36+(src->width)*(src->height)*3);
	tempbuf[0x36+(src->width)*(src->height)*3]=0;
	FSFILE_SetSize(fileHandle, (u16)(0x36+(src->width)*(src->height)*3));
	*(u16*)&tempbuf[0x0] = 0x4D42;
	*(u32*)&tempbuf[0x2] = 0x36 + (src->width)*(src->height)*3;
	*(u32*)&tempbuf[0xA] = 0x36;
	*(u32*)&tempbuf[0xE] = 0x28;
	*(u32*)&tempbuf[0x12] = src->width;
	*(u32*)&tempbuf[0x16] = src->height;
	*(u32*)&tempbuf[0x1A] = 0x00180001;
	*(u32*)&tempbuf[0x22] = (src->width)*(src->height)*3;
	int i=0;
	while (i<((src->width)*(src->height)*3)){
	tempbuf[0x36+i] = src->pixels[i];
	i++;
	}
	FSFILE_Write(fileHandle, &bytesWritten, 0, (u32*)tempbuf, 0x36 + (src->width)*(src->height)*3, 0x10001);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	free(tempbuf);
	return 0;
}


static int lua_newBitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	int width_new = luaL_checkint(L, 1);
	int height_new = luaL_checkint(L, 2);
	Bitmap *bitmap = (Bitmap*)malloc(sizeof(Bitmap));
	bitmap->width = width_new;
	bitmap->height = height_new;
	u8* pixels_new = (u8*)malloc(width_new*height_new*3);
	int i=0;
	while (i < (width_new*height_new*3)){
	pixels_new[i] = 0;
	i++;
	}
	bitmap->pixels = pixels_new;
	lua_pushnumber(L, (u32)(bitmap));
	return 1;
}

static int lua_free(lua_State *L)
{
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkint(L, 1);
	free(src->pixels);
	free(src);
	return 0;
}

static int lua_flip(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	gfxFlushBuffers();
	gfxSwapBuffers();
	return 0;
}

static int lua_refresh(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	RefreshScreen();	
	return 0;
}

static int lua_Vblank(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	gspWaitForVBlank();
	return 0;
}

static int lua_clearScreen(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	int screen = luaL_checkint(L,1);
	ClearScreen(screen);
	return 0;
}

static int lua_fillRect(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 6) && (argc != 7)) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkint(L,1);
	int x2 = luaL_checkint(L,2);
	int y1 = luaL_checkint(L,3);
	int y2 = luaL_checkint(L,4);
	u32 color = luaL_checknumber(L,5);
	int screen = luaL_checkint(L,6);
	int side;
	if (argc == 7){
	side = luaL_checkint(L,7);
	}else{
	side = 0;
	}
	FillRect(x1,x2,y1,y2,color,screen,side);
	return 0;
}

static int lua_fillEmptyRect(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 6) && (argc != 7)) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkint(L,1);
	int x2 = luaL_checkint(L,2);
	int y1 = luaL_checkint(L,3);
	int y2 = luaL_checkint(L,4);
	u32 color = luaL_checknumber(L,5);
	int screen = luaL_checkint(L,6);
	int side;
	if (argc == 7){
	side = luaL_checkint(L,7);
	}else{
	side = 0;
	}
	FillEmptyRect(x1,x2,y1,y2,color,screen,side);
	return 0;
}

static int lua_pixel(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checknumber(L,1);
	int y = luaL_checknumber(L,2);
	u32 color = luaL_checknumber(L,3);
	int screen = luaL_checknumber(L,4);
	int side;
	if (argc == 5){
	side = luaL_checkint(L,5);
	}else{
	side = 0;
	}
	if (screen > 1){
	DrawImagePixel(x,y,color,(Bitmap*)screen);
	}else{
	DrawPixel(x,y,color,screen,side);
	}
	return 0;
}

static int lua_color(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 3) return luaL_error(L, "wrong number of arguments");
    int r = luaL_checkint(L, 1);
    int g = luaL_checkint(L, 2);
	int b = luaL_checkint(L, 3);
    u32 color = b | (g << 8) | (r << 16);
    lua_pushnumber(L,color);
    return 1;
}

static int lua_getB(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkint(L, 1);
    u32 colour = color & 0xFF;
    lua_pushnumber(L,colour);
    return 1;
}

static int lua_getG(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkint(L, 1);
    u32 colour = (color >> 8) & 0xFF;
    lua_pushnumber(L,colour);
    return 1;
}

static int lua_getR(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkint(L, 1);
    u32 colour = (color >> 16) & 0xFF;
    lua_pushnumber(L,colour);
    return 1;
}


//Register our Color Functions
static const luaL_Reg Color_functions[] = {
  {"new",                				lua_color},
  {"getR",								lua_getR},
  {"getG",								lua_getG},
  {"getB",								lua_getB},
  {0, 0}
};

//Register our Screen Functions
static const luaL_Reg Screen_functions[] = {
  {"debugPrint",					lua_print},
  {"waitVblankStart",				lua_Vblank},
  {"flip",							lua_flip},
  {"refresh",						lua_refresh},
  {"clear",							lua_clearScreen},
  {"fillRect",						lua_fillRect},
  {"fillEmptyRect",					lua_fillEmptyRect},
  {"pixel",							lua_pixel},
  {"enable3D",						lua_enable3D},
  {"get3DLevel",					lua_get3D},
  {"disable3D",						lua_disable3D},
  {"loadBitmap",					lua_bitmap},
  {"drawImage",						lua_pbitmap},
  {"freeImage",						lua_free},
  {"flipImage",						lua_flipBitmap},
  {"createImage",					lua_newBitmap},
  {"saveBitmap",					lua_saveimg},
  {0, 0}
};

void luaScreen_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Screen_functions, 0);
	lua_setglobal(L, "Screen");
	lua_newtable(L);
	luaL_setfuncs(L, Color_functions, 0);
	lua_setglobal(L, "Color");
	int TOP_SCREEN = 0;
	int BOTTOM_SCREEN = 1;
	int LEFT_EYE = 0;
	int RIGHT_EYE = 1;
	VariableRegister(L,TOP_SCREEN);
	VariableRegister(L,BOTTOM_SCREEN);
	VariableRegister(L,LEFT_EYE);
	VariableRegister(L,RIGHT_EYE);
}