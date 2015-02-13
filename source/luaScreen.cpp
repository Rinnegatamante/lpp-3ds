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

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

static int lua_print(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 5) && (argc != 6)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
	char* text = (char*)(luaL_checkstring(L, 3));
	u32 color = luaL_checkinteger(L,4);
	u8 alpha = (color >> 24) & 0xFF;
	int screen = luaL_checkinteger(L,5);
	int side=0;
	if (argc == 6) side = luaL_checkinteger(L,6);
	if (screen > 1){ 
	if (((Bitmap*)screen)->bitperpixel == 32) Draw32bppImageText(x,y,text,color,screen);
	else if (alpha==255) DrawImageText(x,y,text,color,screen);
	else DrawAlphaImageText(x,y,text,color,screen);
	}else{ 
	if (alpha=255) DrawScreenText(x,y,text,color,screen,side);
	else DrawAlphaScreenText(x,y,text,color,screen,side);
	}
	gfxFlushBuffers();
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

static int lua_loadimg(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	char* text = (char*)(luaL_checkstring(L, 1));
	Handle fileHandle;
	u32 bytesRead;
	u16 magic;
	u64 long_magic;
	FS_path filePath=FS_makePath(PATH_CHAR, text);
	FS_archive script=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenFileDirectly(NULL, &fileHandle, script, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	FSFILE_Read(fileHandle, &bytesRead, 0, &magic, 2);
	Bitmap *bitmap;
	if (magic == 0x5089){
		FSFILE_Read(fileHandle, &bytesRead, 0, &long_magic, 8);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		if (long_magic == 0x0A1A0A0D474E5089) bitmap = loadPng(text);
	}else if (magic == 0x4D42){
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		bitmap = LoadBitmap(text);
	}else if (magic == 0xD8FF){
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		bitmap = OpenJPG(text);
	}
	if(!bitmap) return luaL_error(L, "Error loading image");
    lua_pushinteger(L, (u32)(bitmap));
	return 1;
}

static int lua_pbitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
	Bitmap* file = (Bitmap*)luaL_checkinteger(L, 3);
	int screen= luaL_checkinteger(L, 4);
	int side = 0;
	if (argc == 5) side = luaL_checkinteger(L,5);
	if (screen > 1) PrintImageBitmap(x,y,file,screen);
	else{ 
		if (screen == 0){
			bool partial_x = false;
			bool partial_y = false;
			if (file->width > 400) partial_x = true;
			if (file->height > 240) partial_y = true;
			if (partial_x || partial_y){
				int width = file->width;
				int height = file->height;
				if (partial_x) width = 400;
				if (partial_y) height = 240;
				PrintPartialScreenBitmap(x,y,0,0,width,height,file,screen,side);
			}else PrintScreenBitmap(x,y,file,screen,side);
		}else{
			bool partial_x = false;
			bool partial_y = false;
			if (file->width > 320) partial_x = true;
			if (file->height > 240) partial_y = true;
			if (partial_x || partial_y){
				int width = file->width;
				int height = file->height;
				if (partial_x) width = 320;
				if (partial_y) height = 240;
				PrintPartialScreenBitmap(x,y,0,0,width,height,file,screen,side);
			}else PrintScreenBitmap(x,y,file,screen,side);
		}
	}
	gfxFlushBuffers();
	return 0;
}

static int lua_partial(lua_State *L){
	int argc = lua_gettop(L);
	if ((argc != 8) && (argc != 9)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
	int st_x = luaL_checkinteger(L, 3);
    int st_y = luaL_checkinteger(L, 4);
	int width = luaL_checkinteger(L, 5);
    int height = luaL_checkinteger(L, 6);
	Bitmap* file = (Bitmap*)luaL_checkinteger(L, 7);
	int screen= luaL_checkinteger(L, 8);
	int side = 0;
	if (argc == 5) side = luaL_checkinteger(L,9);
	if (screen > 1) PrintPartialImageBitmap(x,y,st_x,st_y,width,height,file,screen);
	else PrintPartialScreenBitmap(x,y,st_x,st_y,width,height,file,screen,side);
	gfxFlushBuffers();
	return 0;
}

static int lua_flipBitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	u8* not_flipped;
	Bitmap* src = (Bitmap*)luaL_checkinteger(L, 1);
	Bitmap* dst = (Bitmap*)luaL_checkinteger(L, 2);
	not_flipped = dst->pixels;
	u8* flip_pixels = (u8*)malloc((src->width)*(src->height)*3);
	dst->pixels = flipBitmap(flip_pixels, src);
	dst->width = src->width;
	dst->height = src->height;
	free(not_flipped);
	return 0;
}

static int lua_saveimg(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkinteger(L, 1);
	char* text = (char*)(luaL_checkstring(L, 2));
	Handle fileHandle;
	FS_archive sdmcArchive=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FS_path filePath=FS_makePath(PATH_CHAR, text);
	Result ret=FSUSER_OpenFileDirectly(NULL, &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, FS_ATTRIBUTE_NONE);
	if(ret) return luaL_error(L, "error opening file");
	u32 bytesWritten;
	u8 moltiplier = src->bitperpixel / 8;
	u8* tempbuf = (u8*)malloc(0x36+(src->width)*(src->height)*moltiplier);
	memset(tempbuf, 0, 0x36+(src->width)*(src->height)*moltiplier);
	tempbuf[0x36+(src->width)*(src->height)*moltiplier]=0;
	FSFILE_SetSize(fileHandle, (u16)(0x36+(src->width)*(src->height)*moltiplier));
	*(u16*)&tempbuf[0x0] = 0x4D42;
	*(u32*)&tempbuf[0x2] = 0x36 + (src->width)*(src->height)*moltiplier;
	*(u32*)&tempbuf[0xA] = 0x36;
	*(u32*)&tempbuf[0xE] = 0x28;
	*(u32*)&tempbuf[0x12] = src->width;
	*(u32*)&tempbuf[0x16] = src->height;
	if (moltiplier == 3) *(u32*)&tempbuf[0x1A] = 0x00180001;
	else *(u32*)&tempbuf[0x1A] = 0x00200001;
	*(u32*)&tempbuf[0x22] = (src->width)*(src->height)*moltiplier;
	int i=0;
	while (i<((src->width)*(src->height)*moltiplier)){
	tempbuf[0x36+i] = src->pixels[i];
	i++;
	}
	FSFILE_Write(fileHandle, &bytesWritten, 0, (u32*)tempbuf, 0x36 + (src->width)*(src->height)*moltiplier, 0x10001);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	free(tempbuf);
	return 0;
}

static int lua_newBitmap(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3) return luaL_error(L, "wrong number of arguments");
	int width_new = luaL_checkinteger(L, 1);
	int height_new = luaL_checkinteger(L, 2);
	u32 color = luaL_checkinteger(L, 3);
	Bitmap *bitmap = (Bitmap*)malloc(sizeof(Bitmap));
	bitmap->width = width_new;
	bitmap->height = height_new;
	u8* pixels_new = (u8*)malloc(width_new*height_new*4);
	int i=0;
	memset(pixels_new,color,width_new*height_new*4);
	bitmap->pixels = pixels_new;
	bitmap->bitperpixel = 32;
	lua_pushinteger(L, (u32)(bitmap));
	return 1;
}

static int lua_free(lua_State *L)
{
int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkinteger(L, 1);
	free(src->pixels);
	free(src);
	return 0;
}

static int lua_flip(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
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
	int screen = luaL_checkinteger(L,1);
	ClearScreen(screen);
	gfxFlushBuffers();
	return 0;
}

static int lua_fillRect(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 6) && (argc != 7)) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	u32 color = luaL_checkinteger(L,5);
	u8 alpha = (color >> 24) & 0xFF;
	int screen = luaL_checkinteger(L,6);
	int side=0;
	if (argc == 7) side = luaL_checkinteger(L,7);
	if (screen > 1){
	if (((Bitmap*)screen)->bitperpixel == 32) Fill32bppImageRect(x1,x2,y1,y2,color,screen);
	else if (alpha==255) FillImageRect(x1,x2,y1,y2,color,screen);
	else FillAlphaImageRect(x1,x2,y1,y2,color,screen);
	}else{
	if (alpha==255) FillScreenRect(x1,x2,y1,y2,color,screen,side);
	else FillAlphaScreenRect(x1,x2,y1,y2,color,screen,side);
	}
	gfxFlushBuffers();
	return 0;
}


static int lua_drawline(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 6) && (argc != 7)) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	u32 color = luaL_checkinteger(L,5);
	u8 alpha = (color >> 24) & 0xFF;
	int screen = luaL_checkinteger(L,6);
	int side=0;
	if (argc == 7) side = luaL_checkinteger(L,7);
	if (screen > 1){
	if (((Bitmap*)screen)->bitperpixel == 32) Draw32bppImageLine(x1,y1,x2,y2,color,screen);
	else if (alpha==255) DrawImageLine(x1,y1,x2,y2,color,screen);
	else DrawAlphaImageLine(x1,y1,x2,y2,color,screen);
	}else{
	if (alpha==255) DrawScreenLine(x1,y1,x2,y2,color,screen,side);
	else DrawAlphaScreenLine(x1,y1,x2,y2,color,screen,side);
	}
	gfxFlushBuffers();
	return 0;
}

static int lua_fillEmptyRect(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 6) && (argc != 7)) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	u32 color = luaL_checkinteger(L,5);
	u8 alpha = (color >> 24) & 0xFF;
	int screen = luaL_checkinteger(L,6);
	int side=0;
	if (argc == 7) side = luaL_checkinteger(L,7);
	if (screen > 1){ 
	if (((Bitmap*)screen)->bitperpixel == 32) Fill32bppImageEmptyRect(x1,x2,y1,y2,color,screen);
	else if (alpha == 255) FillImageEmptyRect(x1,x2,y1,y2,color,screen);
	else FillAlphaImageEmptyRect(x1,x2,y1,y2,color,screen);
	}else{
	if (alpha==255) FillScreenEmptyRect(x1,x2,y1,y2,color,screen,side);
	else FillAlphaScreenEmptyRect(x1,x2,y1,y2,color,screen,side);
	}
	gfxFlushBuffers();
	return 0;
}

static int lua_pixel(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 4) && (argc != 5)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	u32 color = luaL_checkinteger(L,3);
	u8 alpha = (color >> 24) & 0xFF;
	int screen = luaL_checkinteger(L,4);
	int side=0;
	if (argc == 5) side = luaL_checkinteger(L,5);
	if (screen > 1){
	if (((Bitmap*)screen)->bitperpixel == 32) Draw32bppImagePixel(x,y,color,(Bitmap*)screen);
	else if (alpha == 255) DrawImagePixel(x,y,color,(Bitmap*)screen);
	else DrawAlphaImagePixel(x,y,color,(Bitmap*)screen);
	}else{
	u8* buffer;
	if (screen == 0){
	if (side == 0) buffer = TopLFB;
	else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	if (alpha == 255) DrawPixel(buffer,x,y,color);
	else DrawAlphaPixel(buffer,x,y,color);
	}
	gfxFlushBuffers();
	return 0;
}

static int lua_pixel2(lua_State *L)
{
    int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	int screen = luaL_checkinteger(L,3);
	int side=0;
	if (argc == 4) side = luaL_checkinteger(L,4);
	if (screen > 1){
	lua_pushinteger(L,GetImagePixel(x,y,(Bitmap*)screen));
	}else{
	lua_pushinteger(L,GetPixel(x,y,screen,side));
	}
	return 1;
}

static int lua_getWidth(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkinteger(L, 1);
	lua_pushinteger(L,src->width);
	return 1;
}

static int lua_getHeight(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	Bitmap* src = (Bitmap*)luaL_checkinteger(L, 1);
	lua_pushinteger(L,src->height);
	return 1;
}

static int lua_color(lua_State *L) {
    int argc = lua_gettop(L);
    if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
    int r = luaL_checkinteger(L, 1);
    int g = luaL_checkinteger(L, 2);
	int b = luaL_checkinteger(L, 3);
	int a = 255;
	if (argc==4) a = luaL_checkinteger(L, 4);
    u32 color = b | (g << 8) | (r << 16) | (a << 24);
    lua_pushinteger(L,color);
    return 1;
}

static int lua_getB(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkinteger(L, 1);
    u32 colour = color & 0xFF;
    lua_pushinteger(L,colour);
    return 1;
}

static int lua_getG(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkinteger(L, 1);
    u32 colour = (color >> 8) & 0xFF;
    lua_pushinteger(L,colour);
    return 1;
}

static int lua_getR(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkinteger(L, 1);
    u32 colour = (color >> 16) & 0xFF;
    lua_pushinteger(L,colour);
    return 1;
}

static int lua_getA(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int color = luaL_checkinteger(L, 1);
    u32 colour = (color >> 24) & 0xFF;
    lua_pushinteger(L,colour);
    return 1;
}

static int lua_console(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    int screen = luaL_checkinteger(L, 1);
	Console* console = (Console*)malloc(sizeof(Console));
	console->screen = screen;
	strcpy(console->text,"");
    lua_pushinteger(L,(u32)console);
    return 1;
}

static int lua_conclear(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    Console* console = (Console*)luaL_checkinteger(L, 1);
	strcpy(console->text,"");
    return 0;
}

static int lua_condest(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    Console* console = (Console*)luaL_checkinteger(L, 1);
	free(console);
    return 0;
}

static int lua_conshow(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
    Console* console = (Console*)luaL_checkinteger(L, 1);
	int res = ConsoleOutput(console);
	lua_pushinteger(L,res);
	gfxFlushBuffers();
    return 1;
}

static int lua_conappend(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 2) return luaL_error(L, "wrong number of arguments");
    Console* console = (Console*)luaL_checkinteger(L, 1);
	char* string = (char*)luaL_checkstring(L, 2);
	strcat(console->text,string);
    return 0;
}

//Register our Console Functions
static const luaL_Reg Console_functions[] = {
  {"new",                				lua_console},
  {"clear",								lua_conclear},
  {"show",								lua_conshow},
  {"append",							lua_conappend},
  {"destroy",							lua_condest},
  {0, 0}
};

//Register our Color Functions
static const luaL_Reg Color_functions[] = {
  {"new",                				lua_color},
  {"getR",								lua_getR},
  {"getG",								lua_getG},
  {"getB",								lua_getB},
  {"getA",								lua_getA},
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
  {"drawPixel",						lua_pixel},
  {"getPixel",						lua_pixel2},
  {"enable3D",						lua_enable3D},
  {"get3DLevel",					lua_get3D},
  {"disable3D",						lua_disable3D},
  {"loadImage",						lua_loadimg},
  {"drawImage",						lua_pbitmap},
  {"freeImage",						lua_free},
  {"flipImage",						lua_flipBitmap},
  {"createImage",					lua_newBitmap},
  {"saveBitmap",					lua_saveimg},
  {"getImageWidth",					lua_getWidth},
  {"getImageHeight",				lua_getHeight},  
  {"drawPartialImage",				lua_partial},  
  {"drawLine",						lua_drawline},  
  {0, 0}
};

void luaScreen_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Screen_functions, 0);
	lua_setglobal(L, "Screen");
	lua_newtable(L);
	luaL_setfuncs(L, Color_functions, 0);
	lua_setglobal(L, "Color");
	lua_newtable(L);
	luaL_setfuncs(L, Console_functions, 0);
	lua_setglobal(L, "Console");
	int TOP_SCREEN = 0;
	int BOTTOM_SCREEN = 1;
	int LEFT_EYE = 0;
	int RIGHT_EYE = 1;
	VariableRegister(L,TOP_SCREEN);
	VariableRegister(L,BOTTOM_SCREEN);
	VariableRegister(L,LEFT_EYE);
	VariableRegister(L,RIGHT_EYE);
}