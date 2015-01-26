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
#include "include/luaGraphics.h"
#include "include/font.h"
#define LODEPNG_COMPILE_PNG
#include "include/lodepng/lodepng.h"

#define CONFIG_3D_SLIDERSTATE (*(float*)0x1FF81080)

typedef unsigned short u16;
u8* TopLFB;
u8* TopRFB;
u8* BottomFB;

Bitmap* LoadBitmap(char* fname){
	Handle fileHandle;
	u64 size;
	u32 bytesRead;
	FS_path filePath=FS_makePath(PATH_CHAR, fname);
	FS_archive script=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
	FSUSER_OpenFileDirectly(NULL, &fileHandle, script, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	FSFILE_GetSize(fileHandle, &size);
	Bitmap* result = (Bitmap*)malloc(sizeof(Bitmap));
	
	if(!result) {
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		return 0;
	}
	
	result->pixels = (u8*)malloc(size-0x36);
	FSFILE_Read(fileHandle, &bytesRead, 0x36, result->pixels, size-0x36);
	FSFILE_Read(fileHandle, &bytesRead, 0x12, &(result->width), 4);
	FSFILE_Read(fileHandle, &bytesRead, 0x16, &(result->height), 4);
	FSFILE_Read(fileHandle, &bytesRead, 0x1C, &(result->bitperpixel), 2);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
	
	return result;
}

void PrintImageBitmap(int xp,int yp, Bitmap* result,int screen){
	if(!result)
		return;
	int x, y;
if (result->bitperpixel == 24){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*3];
				u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 1];
				u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 2];
				u32 color = B + G*256 + R*256*256;
				DrawImagePixel(xp+x,yp+y,color,(Bitmap*)screen);
			}
		}
			}else{
			for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*4];
				u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 1];
				u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 2];
				u8 A = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 3];
				u32 color = B + G*256 + R*256*256;
				DrawAlphaImagePixel(xp+x,yp+y,color,(Bitmap*)screen, A);
			}
			
		}
	}
}

void PrintScreenBitmap(int xp,int yp, Bitmap* result,int screen,int side){
if(!result) return;
u8* buffer = 0;
if (screen == 0){
if (side == 0) buffer = TopLFB;
else buffer = TopRFB;
}else if (screen == 1) buffer = BottomFB;
int x, y;
if (result->bitperpixel == 24){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*3];
			u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 1];
			u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 2];
			u32 color = B + G*256 + R*256*256;
			DrawPixel(buffer,xp+x,yp+y,color);
			}
		}
}else{
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*4];
				u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 1];
				u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 2];
				u8 A = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 3];
				u32 color = B + G*256 + R*256*256;
				DrawAlphaPixel(buffer,xp+x,yp+y,color,A);
			}			
		}
	}
}

void PrintPartialScreenBitmap(int xp,int yp,int st_x,int st_y,int width,int height, Bitmap* result,int screen,int side){
if(!result)
	return;
u8* buffer = 0;
if (screen == 0){
if (side == 0) buffer = TopLFB;
else buffer = TopRFB;
}else if (screen == 1) buffer = BottomFB;
int x, y;
if (result->bitperpixel == 24){
	for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
			u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*3];
			u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 1];
			u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 2];
			u32 color = B + G*256 + R*256*256;
			DrawPixel(buffer,xp+x-st_x,yp+y-st_y,color);
		}
	}
}else{
	for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
			u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*4];
			u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 1];
			u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 2];
			u8 A = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 3];
				u32 color = B + G*256 + R*256*256;
				DrawAlphaPixel(buffer,xp+x-st_x,yp+y-st_y,color,A);
			}			
		}
	}
}

void PrintPartialImageBitmap(int xp,int yp,int st_x,int st_y,int width,int height, Bitmap* result,int screen){
	if(!result)
		return;
	int x, y;
			if (result->bitperpixel == 24){
	for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*3];
				u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 1];
				u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 2];
				u32 color = B + G*256 + R*256*256;
				DrawImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen);
				}
				}
			}else{
				for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u8 B = result->pixels[(x + (result->height - y - 1) * result->width)*4];
				u8 G = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 1];
				u8 R = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 2];
				u8 A = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 3];
				u32 color = B + G*256 + R*256*256;
				DrawAlphaImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen, A);
			}
			
		}
	}
}

u8* flipBitmap(u8* flip_bitmap, Bitmap* result){
if(!result)
	return 0;
int x, y;
if (result->bitperpixel == 24){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			flip_bitmap[(x + y * result->width)*3] = result->pixels[(x + (result->height - y - 1) * result->width)*3];
			flip_bitmap[(x + y * result->width)*3 + 1] = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 1];
			flip_bitmap[(x + y * result->width)*3 + 2] = result->pixels[(x + (result->height - y - 1) * result->width)*3 + 2];
		}
	}
}else if(result->bitperpixel == 32){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			flip_bitmap[(x + y * result->width)*4] = result->pixels[(x + (result->height - y - 1) * result->width)*4];
			flip_bitmap[(x + y * result->width)*4 + 1] = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 1];
			flip_bitmap[(x + y * result->width)*4 + 2] = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 2];
			flip_bitmap[(x + y * result->width)*4 + 3] = result->pixels[(x + (result->height - y - 1) * result->width)*4 + 3];
		}
	}
}
	return flip_bitmap;
}

void DrawPixel(u8* screen, int x,int y, u32 color){
	int idx = ((x)*240) + (239-(y));
	screen[idx*3+0] = (color);
	screen[idx*3+1] = (color) >> 8;
	screen[idx*3+2] = (color) >> 16;
}

void DrawAlphaPixel(u8* screen, int x,int y, u32 color, u8 alpha){
	int idx = ((x)*240) + (239-(y));
	float ratio = alpha / 255.0f;
	screen[idx*3+0] = ((color & 0xFF) * ratio) + (screen[idx*3+0] * (1.0 - ratio));
	screen[idx*3+1] = ((((color) >> 8) & 0xFF) * ratio) + (screen[idx*3+1] * (1.0 - ratio));
	screen[idx*3+2] = ((((color) >> 16) & 0xFF) * ratio) + (screen[idx*3+2] * (1.0 - ratio));
}

u32 GetPixel(int x,int y,int screen,int side){
int idx = ((x)*240) + (239-(y));
u32 color;
if ((screen == 0) && (x < 400) && (y < 240) && (x >= 0) && (y >= 0)){
if (side == 0){
color = TopLFB[idx*3+0] + TopLFB[idx*3+1] * 256 + TopLFB[idx*3+2] * 256 * 256;
}else{
color = TopRFB[idx*3+0] + TopRFB[idx*3+1] * 256 + TopRFB[idx*3+2] * 256 * 256;
}
}else if((screen == 1) && (x < 320) && (y < 240) && (x >= 0) && (y >= 0)){
color = BottomFB[idx*3+0] + BottomFB[idx*3+1] * 256 + BottomFB[idx*3+2] * 256 * 256;
}
return color;
}

u32 GetImagePixel(int x,int y,Bitmap* screen){
int idx = (x + (screen->height - y) * screen->width);
u32 color = screen->pixels[idx*3+0] + screen->pixels[idx*3+1] * 256 + screen->pixels[idx*3+2] * 256 * 256;
return color;
}

void DrawAlphaImagePixel(int x,int y,u32 color,Bitmap* screen, u8 alpha){
int idx = (x + (screen->height - y) * screen->width);
float ratio = alpha / 255.0f;
screen->pixels[idx*3+0] = ((color & 0xFF) * ratio) + (screen->pixels[idx*3+0] * (1.0 - ratio));
screen->pixels[idx*3+1] = ((((color) >> 8) & 0xFF) * ratio) + (screen->pixels[idx*3+1] * (1.0 - ratio));
screen->pixels[idx*3+2] = ((((color) >> 16) & 0xFF) * ratio) + (screen->pixels[idx*3+2] * (1.0 - ratio));
}

void DrawImagePixel(int x,int y,u32 color,Bitmap* screen){
int idx = (x + (screen->height - y) * screen->width);
screen->pixels[idx*3+0] = (color);
screen->pixels[idx*3+1] = (color) >> 8;
screen->pixels[idx*3+2] = (color) >> 16;
}

void RefreshScreen(){
TopLFB = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
if (CONFIG_3D_SLIDERSTATE != 0) TopRFB = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
BottomFB = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
}

void DrawScreenText(int x, int y, char* str, u32 color, int screen,int side){
u8* buffer;
if (screen == 0){
if (side == 0) buffer = TopLFB;
else buffer = TopRFB;
}else if (screen == 1) buffer = BottomFB;
unsigned short* ptr;
unsigned short glyphsize;
int i, cx, cy;
for (i = 0; str[i] != '\0'; i++)
{
if (str[i] < 0x21)
{
x += 6;
continue;
}
u16 ch = str[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
continue;
}
x++;
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if (val & (1 << cx))
DrawPixel(buffer, x+cx, y+cy, color);
}
}
x += glyphsize;
x++;
}
}

void DrawAlphaScreenText(int x, int y, char* str, u32 color, int screen,int side,u8 alpha){
u8* buffer;
if (screen == 0){
if (side == 0) buffer = TopLFB;
else buffer = TopRFB;
}else if (screen == 1) buffer = BottomFB;
unsigned short* ptr;
unsigned short glyphsize;
int i, cx, cy;
for (i = 0; str[i] != '\0'; i++)
{
if (str[i] < 0x21)
{
x += 6;
continue;
}
u16 ch = str[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
continue;
}
x++;
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if (val & (1 << cx))
DrawAlphaPixel(buffer, x+cx, y+cy, color, alpha);
}
}
x += glyphsize;
x++;
}
}

void DrawAlphaImageText(int x, int y, char* str, u32 color, int screen, u8 alpha){
unsigned short* ptr;
unsigned short glyphsize;
int i, cx, cy;
for (i = 0; str[i] != '\0'; i++)
{
if (str[i] < 0x21)
{
x += 6;
continue;
}
u16 ch = str[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
continue;
}
x++;
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if (val & (1 << cx))
DrawAlphaImagePixel(x+cx, y+cy, color, (Bitmap*)screen, alpha);
}
}
x += glyphsize;
x++;
}
}

void DrawImageText(int x, int y, char* str, u32 color, int screen){
unsigned short* ptr;
unsigned short glyphsize;
int i, cx, cy;
for (i = 0; str[i] != '\0'; i++)
{
if (str[i] < 0x21)
{
x += 6;
continue;
}
u16 ch = str[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
continue;
}
x++;
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if (val & (1 << cx))
DrawImagePixel(x+cx, y+cy, color, (Bitmap*)screen);
}
}
x += glyphsize;
x++;
}
}

void DebugOutput(char* str){
unsigned short* ptr;
unsigned short glyphsize;
int i, cx, cy;
int x=0;
int y=0;
for (i = 0; str[i] != '\0'; i++)
{
if (str[i] == 0x0A){
x=0;
y=y+15;
continue;
}else if(str[i] == 0x0D){
continue;
}
if (str[i] < 0x21)
{
x += 6;
continue;
}
u16 ch = str[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
continue;
}
x++;
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if ((x+cx) >= 320){
x=0;
y=y+15;
}
if (val & (1 << cx))
DrawPixel(BottomFB, x+cx, y+cy, 0xFFFFFF);
}
}
x += glyphsize;
x++;
}
}

int ConsoleOutput(Console* console){
unsigned short* ptr;
unsigned short glyphsize;
int max_x;
u8* buffer;
if (console->screen == 0){
max_x = 400;
buffer = TopLFB;
}else{
max_x = 320;
buffer = BottomFB;
}
int i, cx, cy;
int x=0;
int y=0;
int res = 0;
for (i = 0; console->text[i] != '\0'; i++)
{
if (y > 230) break;
if (console->text[i] == 0x0A){
x=0;
y=y+15;
res++;
continue;
}else if(console->text[i] == 0x0D){
res++;
continue;
}
if (console->text[i] < 0x21)
{
x += 6;
res++;
continue;
}
u16 ch = console->text[i];
if (ch > 0x7E) ch = 0x7F;
ptr = &font[(ch-0x20) << 4];
glyphsize = ptr[0];
if (!glyphsize)
{
x += 6;
res++;
continue;
}
x++;
if (x >= max_x - 10){
x=0;
y=y+15;
if (y > 230) break;
}
for (cy = 0; cy < 12; cy++)
{
unsigned short val = ptr[4+cy];
for (cx = 0; cx < glyphsize; cx++)
{
if (val & (1 << cx))
DrawPixel(buffer, x+cx, y+cy, 0xFFFFFF);
}
}
x += glyphsize;
x++;
res++;
}
x = 0;
y = 0;
return res;
}

void FillImageRect(int x1,int x2,int y1,int y2,u32 color,int screen){
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (x1 <= x2){
		while (y1 <= y2){
			DrawImagePixel(x1,y1,color,(Bitmap*)screen);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillAlphaImageRect(int x1,int x2,int y1,int y2,u32 color,int screen,u8 alpha){
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (x1 <= x2){
		while (y1 <= y2){
			DrawAlphaImagePixel(x1,y1,color,(Bitmap*)screen,alpha);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillScreenRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side){
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (x1 <= x2){
		while (y1 <= y2){
			DrawPixel(buffer,x1,y1,color);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillAlphaScreenRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side,u8 alpha){
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (x1 <= x2){
		while (y1 <= y2){
			DrawAlphaPixel(buffer,x1,y1,color,alpha);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillImageEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen){
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (y1 <= y2){
			DrawImagePixel(x1,y1,color,(Bitmap*)screen);
			DrawImagePixel(x2,y1,color,(Bitmap*)screen);
			y1++;
		}
	while (x1 <= x2){
		DrawImagePixel(x1,base_y,color,(Bitmap*)screen);
		DrawImagePixel(x1,y2,color,(Bitmap*)screen);
	}
}

void FillAlphaImageEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen,u8 alpha){
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (y1 <= y2){
			DrawAlphaImagePixel(x1,y1,color,(Bitmap*)screen,alpha);
			DrawAlphaImagePixel(x2,y1,color,(Bitmap*)screen,alpha);
			y1++;
		}
	while (x1 <= x2){
		DrawAlphaImagePixel(x1,base_y,color,(Bitmap*)screen,alpha);
		DrawAlphaImagePixel(x1,y2,color,(Bitmap*)screen,alpha);
	}
}

void FillScreenEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side){
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (y1 <= y2){
		DrawPixel(buffer,x1,y1,color);
		DrawPixel(buffer,x2,y1,color);
		y1++;
	}
	while (x1 <= x2){
		DrawPixel(buffer,x1,base_y,color);
		DrawPixel(buffer,x1,y2,color);
		x1++;
	}
}

void FillAlphaScreenEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side,u8 alpha){
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	if (x1 > x2){
	int temp_x = x1;
	x1 = x2;
	x2 = temp_x;
	}
	if (y1 > y2){
	int temp_y = y1;
	y1 = y2;
	y2 = temp_y;
	}
	int base_y = y1;
	while (y1 <= y2){
		DrawAlphaPixel(buffer,x1,y1,color,alpha);
		DrawAlphaPixel(buffer,x2,y1,color,alpha);
		y1++;
	}
	while (x1 <= x2){
		DrawAlphaPixel(buffer,x1,base_y,color,alpha);
		DrawAlphaPixel(buffer,x1,y2,color,alpha);
		x1++;
	}
}

void ClearScreen(int screen){
	if (screen==1){
		memset(BottomFB,0x00,230400);
	}else{
		memset(TopLFB,0x00,288000);
	if (CONFIG_3D_SLIDERSTATE != 0){
		memset(TopRFB,0x00,288000);
	}
	}
}

Bitmap* loadPng(const char* filename)
{
	Handle fileHandle;
	Bitmap* result;
	u64 size;
	u32 bytesRead;
	unsigned char* out;
	unsigned char* in;
	unsigned int w, h;
	
	FS_path filePath = FS_makePath(PATH_CHAR, filename);
	FS_archive archive = (FS_archive) { ARCH_SDMC, (FS_path) { PATH_EMPTY, 1, (u8*)"" }};
	FSUSER_OpenFileDirectly(NULL, &fileHandle, archive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
	
	FSFILE_GetSize(fileHandle, &size);
	
	in = (unsigned char*)malloc(size);
	
	if(!in) {
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		return 0;
	}
	
	FSFILE_Read(fileHandle, &bytesRead, 0x00, in, size);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
		
		if(lodepng_decode32(&out, &w, &h, in, size) != 0) {
			free(in);
			return 0;
		}
	
	free(in);
	
	result = (Bitmap*)malloc(sizeof(Bitmap));
	if(!result) {
		free(out);
	}
	
	result->pixels = out;
	result->width = w;
	result->height = h;
	result->bitperpixel = 32;
	
	u8* flipped = (u8*)malloc(w*h*4);
	flipped = flipBitmap(flipped, result);
	u32 i = 0;
	while (i < (w*h*4)){
	u8 tmp = flipped[i];
	flipped[i] = flipped[i+2];
	flipped[i+2] = tmp;
	i=i+4;
	}
	free(out);
	result->pixels = flipped;
	
	return result;
}

/*void PrintScreenPNG(int xp,int yp, Bitmap* result,int screen,int side){
if(!result) return;
u8* buffer = 0;
if (screen == 0){
if (side == 0) buffer = TopLFB;
else buffer = TopRFB;
}else if (screen == 1) buffer = BottomFB;
int x, y;
if (result->bitperpixel == 24){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			u8 R = result->pixels[(x + y * result->width)*3];
			u8 G = result->pixels[(x + y * result->width)*3 + 1];
			u8 B = result->pixels[(x + y * result->width)*3 + 2];
			u32 color = B + G*256 + R*256*256;
			DrawPixel(buffer,xp+x,yp+y,color);
			}
			}
			}else if (result->bitperpixel == 32){
			for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u8 R = result->pixels[(x + y * result->width)*4];
				u8 G = result->pixels[(x + y * result->width)*4 + 1];
				u8 B = result->pixels[(x + y * result->width)*4 + 2];
				u8 A = result->pixels[(x + y * result->width)*4 + 3];
				u32 color = B + G*256 + R*256*256;
				DrawAlphaPixel(buffer,xp+x,yp+y,color,A);
			}			
		}
	}
}

Bitmap* decodePng(unsigned char* in,u64 size)
{
	Handle fileHandle;
	Bitmap* result;
	u32 bytesRead;
	unsigned char* out;
	unsigned int w, h;
		
	if(lodepng_decode24(&out, &w, &h, in, size) != 0) return 0;
	
	result = (Bitmap*)malloc(sizeof(Bitmap));
	if(!result) {
		free(out);
	}
	
	result->pixels = out;
	result->width = w;
	result->height = h;
	result->bitperpixel = 24;
	
	return result;
}*/