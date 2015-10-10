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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "Graphics.h"
#include "font.h"
#include "../lodepng/lodepng.h"
#include "../libjpeg/jpeglib.h"
#include <setjmp.h>

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
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
				DrawImagePixel(xp+x,yp+y,color,(Bitmap*)screen);
			}
		}
			}else{
			for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				DrawAlphaImagePixel(xp+x,yp+y,color,(Bitmap*)screen);
			}
			
		}
	}
}

void PrintGpuBitmap(int xp,int yp, Bitmap* result,int screen){
	int x, y;
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
			sf2d_set_pixel(((gpu_text*)screen)->tex, xp+x, yp+y, color);
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
			u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
			DrawPixel(buffer,xp+x,yp+y,color);
			}
		}
}else{
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				DrawAlphaPixel(buffer,xp+x,yp+y,color);
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
			u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
			DrawPixel(buffer,xp+x-st_x,yp+y-st_y,color);
		}
	}
}else{
	for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				DrawAlphaPixel(buffer,xp+x-st_x,yp+y-st_y,color);
			}			
		}
	}
}

void PrintPartialImageBitmap(int xp,int yp,int st_x,int st_y,int width,int height, Bitmap* result,int screen){
	if(!result)
		return;
	int x, y;
		if (((Bitmap*)screen)->bitperpixel == 32){
		if (result->bitperpixel == 24){
			for (y = st_y; y < st_y + height; y++){
				for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
				color = (color & 0x00FFFFFF) | (0xFF << 24);
				Draw32bppImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen);
				}
				}
		}else{
		for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				Draw32bppImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen);
			}
		}
		}
		}else{
			if (result->bitperpixel == 24){
	for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
				DrawImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen);
				}
				}
			}else{
				for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				DrawAlphaImagePixel(xp+x-st_x,yp+y-st_y,color,(Bitmap*)screen);
			}			
		}
	}
	}
}

void PrintPartialGpuBitmap(int xp,int yp,int st_x,int st_y,int width,int height, Bitmap* result,int screen){
	if(!result)
		return;
	int x, y;
		if (result->bitperpixel == 24){
			for (y = st_y; y < st_y + height; y++){
				for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]);
				color = (color & 0x00FFFFFF) | (0xFF << 24);
				sf2d_set_pixel(((gpu_text*)screen)->tex,xp+x-st_x,yp+y-st_y,color);
				}
				}
		}else{
		for (y = st_y; y < st_y + height; y++){
		for (x = st_x; x < st_x + width; x++){
				u32 color = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
				sf2d_set_pixel(((gpu_text*)screen)->tex,xp+x-st_x,yp+y-st_y,color);
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
			int idx = (x+y * result->width)*3;
			*(u32*)(&(flip_bitmap[idx])) = ((*(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*3]) & 0x00FFFFFF) | (*(u32*)(&(flip_bitmap[idx])) & 0xFF000000));
		}
	}
}else if(result->bitperpixel == 32){
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			*(u32*)(&(flip_bitmap[(x+y * result->width)*4])) = *(u32*)&(result->pixels[(x + (result->height - y - 1) * result->width)*4]);
		}
	}
}
	return flip_bitmap;
}

void DrawPixel(u8* screen, int x,int y, u32 color){
	int idx = (((x)*240) + (239-(y)))*3;
	*(u32*)&(screen[idx]) = (((color) & 0x00FFFFFF) | ((*(u32*)&(screen[idx])) & 0xFF000000));
}

void DrawAlphaPixel(u8* screen, int x,int y, u32 color){
	u8 alpha = (((color) >> 24) & 0xFF);
	int idx = ((x)*240) + (239-(y));
	float ratio = alpha / 255.0f;
	screen[idx*3+0] = ((color & 0xFF) * ratio) + (screen[idx*3+0] * (1.0 - ratio));
	screen[idx*3+1] = ((((color) >> 8) & 0xFF) * ratio) + (screen[idx*3+1] * (1.0 - ratio));
	screen[idx*3+2] = ((((color) >> 16) & 0xFF) * ratio) + (screen[idx*3+2] * (1.0 - ratio));
}

u32 GetPixel(int x,int y,int screen,int side){
	int idx = (((x)*240) + (239-(y))) * 3;
	u32 color;
	if (screen == 0){
		if (side == 0) color = (TopLFB[idx] & 0x00FFFFFF) | (0xFFFFFFFF & 0xFF000000);
		else color = (TopRFB[idx] & 0x00FFFFFF) | (0xFFFFFFFF & 0xFF000000);
	}else if(screen == 1) color = (BottomFB[idx] & 0x00FFFFFF) | (0xFFFFFFFF & 0xFF000000);
	return color;
}

u32 GetImagePixel(int x,int y,Bitmap* screen){
	u32 color;
	if (screen->bitperpixel == 24){
		int idx = (x + (screen->height - y) * screen->width)*3;
		color = ((*(u32*)&(screen->pixels[idx])) & 0x00FFFFFF) | (0xFFFFFFFF & 0xFF000000);
	}else{ 
		int idx = (x + (screen->height - y) * screen->width)*4;
		color = *(u32*)&(screen->pixels[idx]);
	}
return color;
}

void DrawAlphaImagePixel(int x,int y,u32 color,Bitmap* screen){
u8 alpha = (((color) >> 24) & 0xFF);
int idx = (x + (screen->height - y) * screen->width);
float ratio = alpha / 255.0f;
screen->pixels[idx*(screen->bitperpixel / 8)+0] = ((color & 0xFF) * ratio) + (screen->pixels[idx*(screen->bitperpixel / 8)+0] * (1.0 - ratio));
screen->pixels[idx*(screen->bitperpixel / 8)+1] = ((((color) >> 8) & 0xFF) * ratio) + (screen->pixels[idx*(screen->bitperpixel / 8)+1] * (1.0 - ratio));
screen->pixels[idx*(screen->bitperpixel / 8)+2] = ((((color) >> 16) & 0xFF) * ratio) + (screen->pixels[idx*(screen->bitperpixel / 8)+2] * (1.0 - ratio));
}

void DrawImagePixel(int x,int y,u32 color,Bitmap* screen){
	int idx = (x + (screen->height - y) * screen->width)*3;
	*(u32*)&(screen->pixels[idx]) = (((color) & 0x00FFFFFF) | ((*(u32*)&(screen->pixels[idx])) & 0xFF000000));
}

void Draw32bppImagePixel(int x,int y,u32 color,Bitmap* screen){
u8 alpha = (((color) >> 24) & 0xFF);
int idx = (x + (screen->height - y) * screen->width);
float srcA = alpha / 255.0f;
float outA = srcA + (screen->pixels[idx*4+3] / 255.0f) * (1 - srcA);
screen->pixels[idx*4+0] = (((color & 0xFF) * srcA)  + (screen->pixels[idx*4+0] * (screen->pixels[idx*4+3] / 255.0f) * (1.0 - srcA))) / outA;
screen->pixels[idx*4+1] = ((((color >> 8) & 0xFF) * srcA)  + (screen->pixels[idx*4+1] * (screen->pixels[idx*4+3] / 255.0f) * (1.0 - srcA))) / outA;
screen->pixels[idx*4+2] = (((((color) >> 16) & 0xFF) * srcA)  + (screen->pixels[idx*4+2] * (screen->pixels[idx*4+3] / 255.0f) * (1.0 - srcA))) / outA;
screen->pixels[idx*4+3] = outA * 255.0f;
}

void RefreshScreen(){
TopLFB = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
if (CONFIG_3D_SLIDERSTATE != 0) TopRFB = gfxGetFramebuffer(GFX_TOP, GFX_RIGHT, NULL, NULL);
BottomFB = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
}

void DrawScreenText(int x, int y, char* str, u32 color, int screen,int side){
u8* buffer;
u16 max_x = 320;
if (screen == 0){
	if (side == 0) buffer = TopLFB;
	else buffer = TopRFB;
	max_x = 400;
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
if ((x+cx) >= max_x) return;
if (val & (1 << cx))
DrawPixel(buffer, x+cx, y+cy, color);
}
}
x += glyphsize;
x++;
}
}

void DrawAlphaScreenText(int x, int y, char* str, u32 color, int screen,int side){
u8* buffer;
u16 max_x = 320;
if (screen == 0){
max_x = 400;
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
if ((x+cx) >= max_x) return;
if (val & (1 << cx))
DrawAlphaPixel(buffer, x+cx, y+cy, color);
}
}
x += glyphsize;
x++;
}
}

void DrawAlphaImageText(int x, int y, char* str, u32 color, int screen){
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
DrawAlphaImagePixel(x+cx, y+cy, color, (Bitmap*)screen);
}
}
x += glyphsize;
x++;
}
}

void Draw32bppImageText(int x, int y, char* str, u32 color, int screen){
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
Draw32bppImagePixel(x+cx, y+cy, color, (Bitmap*)screen);
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

void DrawGpuText(int x, int y, char* str, u32 color, int screen){
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
sf2d_set_pixel(((gpu_text*)screen)->tex,x+cx, y+cy, color);
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

void FillGpuRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			sf2d_set_pixel(((gpu_text*)screen)->tex,x1,y1,color);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillAlphaImageRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			DrawAlphaImagePixel(x1,y1,color,(Bitmap*)screen);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void Fill32bppImageRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			Draw32bppImagePixel(x1,y1,color,(Bitmap*)screen);
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

void FillAlphaScreenRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side){
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
			DrawAlphaPixel(buffer,x1,y1,color);
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
		x1++;
	}
}

void FillGpuEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			sf2d_set_pixel(((gpu_text*)screen)->tex,x1,y1,color);
			sf2d_set_pixel(((gpu_text*)screen)->tex,x2,y1,color);
			y1++;
		}
	while (x1 <= x2){
		sf2d_set_pixel(((gpu_text*)screen)->tex,x1,base_y,color);
		sf2d_set_pixel(((gpu_text*)screen)->tex,x1,y2,color);
		x1++;
	}
}

void FillAlphaImageEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			DrawAlphaImagePixel(x1,y1,color,(Bitmap*)screen);
			DrawAlphaImagePixel(x2,y1,color,(Bitmap*)screen);
			y1++;
		}
	while (x1 <= x2){
		DrawAlphaImagePixel(x1,base_y,color,(Bitmap*)screen);
		DrawAlphaImagePixel(x1,y2,color,(Bitmap*)screen);
		x1++;
	}
}

void Fill32bppImageEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
		Draw32bppImagePixel(x1,y1,color,(Bitmap*)screen);
		Draw32bppImagePixel(x2,y1,color,(Bitmap*)screen);
		y1++;
		}
	while (x1 <= x2){
		Draw32bppImagePixel(x1,base_y,color,(Bitmap*)screen);
		Draw32bppImagePixel(x1,y2,color,(Bitmap*)screen);
		x1++;
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

void FillAlphaScreenEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen,int side){
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
		DrawAlphaPixel(buffer,x1,y1,color);
		DrawAlphaPixel(buffer,x2,y1,color);
		y1++;
	}
	while (x1 <= x2){
		DrawAlphaPixel(buffer,x1,base_y,color);
		DrawAlphaPixel(buffer,x1,y2,color);
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

void DrawScreenLine(int x0, int y0, int x1, int y1, u32 color, int screen, int side)
{
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    DrawPixel(buffer, x0, y0, color);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            DrawPixel(buffer, x0, y0, color);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            DrawPixel(buffer, x0, y0, color);
        }
    }
}

void DrawAlphaScreenLine(int x0, int y0, int x1, int y1, u32 color, int screen, int side)
{
	u8* buffer;
	if (screen == 0){
		if (side == 0) buffer = TopLFB;
		else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    DrawAlphaPixel(buffer, x0, y0, color);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            DrawAlphaPixel(buffer, x0, y0, color);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            DrawAlphaPixel(buffer, x0, y0, color);
        }
    }
}

void DrawAlphaImageLine(int x0, int y0, int x1, int y1, u32 color, int screen)
{
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    DrawAlphaImagePixel(x0, y0, color, (Bitmap*)screen);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            DrawAlphaImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            DrawAlphaImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    }
}

void Draw32bppImageLine(int x0, int y0, int x1, int y1, u32 color, int screen)
{
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    Draw32bppImagePixel(x0, y0, color, (Bitmap*)screen);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            Draw32bppImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            Draw32bppImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    }
}

void DrawImageLine(int x0, int y0, int x1, int y1, u32 color, int screen)
{
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    DrawImagePixel(x0, y0, color, (Bitmap*)screen);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            DrawImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            DrawImagePixel(x0, y0, color, (Bitmap*)screen);
        }
    }
}

void DrawGpuLine(int x0, int y0, int x1, int y1, u32 color, int screen)
{
    int dy = y1 - y0;
    int dx = x1 - x0;
    int stepx, stepy;
   
    if (dy < 0) { dy = -dy;  stepy = -1; } else { stepy = 1; }
    if (dx < 0) { dx = -dx;  stepx = -1; } else { stepx = 1; }
    dy <<= 1;
    dx <<= 1;
   
    y0 *= 1;
    y1 *= 1;
    DrawImagePixel(x0, y0, color, (Bitmap*)screen);
    if (dx > dy) {
        int fraction = dy - (dx >> 1);
        while (x0 != x1) {
            if (fraction >= 0) {
                y0 += stepy;
                fraction -= dx;
            }
            x0 += stepx;
            fraction += dy;
            sf2d_set_pixel(((gpu_text*)screen)->tex,x0, y0, color);
        }
    } else {
        int fraction = dx - (dy >> 1);
        while (y0 != y1) {
            if (fraction >= 0) {
                x0 += stepx;
                fraction -= dy;
            }
            y0 += stepy;
            fraction += dx;
            sf2d_set_pixel(((gpu_text*)screen)->tex,x0, y0, color);
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

Bitmap* decodePNGfile(const char* filename)
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
	return result;
}

void linecpy(u8* screen,u16 x,u16 y,u16 width,u16 height, u8* image,u16 x_img,u16 y_img){
	for (int i=y_img; i<y_img+height; i++){
		for (int j=x_img; j<x_img+width; j++){
			u32 idx = (j+i*width)*3;
			*(u32*)&(screen[idx]) = (((*(u32*)&(image[idx])) & 0x00FFFFFF) | ((*(u32*)&(screen[idx])) & 0xFF000000));
		}
	}
}

void RAW2FB(int xp,int yp, Bitmap* result,int screen,int side){
	if(!result) return;
	u8* buffer = 0;
	if (screen == 0){
	if (side == 0) buffer = TopLFB;
	else buffer = TopRFB;
	}else if (screen == 1) buffer = BottomFB;
	int x=0, y=0;
	linecpy(buffer,xp,yp,result->width,result->height, result->pixels,x,y);
}

struct my_error_mgr {
struct jpeg_error_mgr pub;
jmp_buf setjmp_buffer;
};
typedef struct my_error_mgr * my_error_ptr;
METHODDEF(void)
my_error_exit (j_common_ptr cinfo)
{
my_error_ptr myerr = (my_error_ptr) cinfo->err;
(*cinfo->err->output_message) (cinfo);
longjmp(myerr->setjmp_buffer, 1);
}

Bitmap* OpenJPG(const char* filename)
{
    Bitmap* result = (Bitmap*)malloc(sizeof(Bitmap));
	if (result == NULL) return 0;
	u64 size;
	u32 bytesRead;
	Handle fileHandle;
    struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jpeg_create_decompress(&cinfo);
	FS_path filePath = FS_makePath(PATH_CHAR, filename);
	FS_archive archive = (FS_archive) { ARCH_SDMC, (FS_path) { PATH_EMPTY, 1, (u8*)"" }};
	FSUSER_OpenFileDirectly(NULL, &fileHandle, archive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);	
	FSFILE_GetSize(fileHandle, &size);
	unsigned char* in = (unsigned char*)malloc(size);
	if(!in) {
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		return 0;
	}
	FSFILE_Read(fileHandle, &bytesRead, 0x00, in, size);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
    jpeg_mem_src(&cinfo, in, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_bytes = width * cinfo.num_components;
    u8* bgr_buffer = (u8*) malloc(width*height*cinfo.num_components);
    while (cinfo.output_scanline < cinfo.output_height) {
        u8* buffer_array[1];
        buffer_array[0] = bgr_buffer + (cinfo.output_scanline) * row_bytes;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    result->bitperpixel = 24;
    result->width = width;
    result->height = height;
    result->pixels = bgr_buffer;
	u8* flipped = (u8*)malloc(width*height*3);
	flipped = flipBitmap(flipped, result);
	free(bgr_buffer);
	free(in);
	result->pixels = flipped;
    return result;
}

Bitmap* decodeJPGfile(const char* filename)
{
    Bitmap* result = (Bitmap*)malloc(sizeof(Bitmap));
	if (result == NULL) return 0;
	u64 size;
	u32 bytesRead;
	Handle fileHandle;
    struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jpeg_create_decompress(&cinfo);
	FS_path filePath = FS_makePath(PATH_CHAR, filename);
	FS_archive archive = (FS_archive) { ARCH_SDMC, (FS_path) { PATH_EMPTY, 1, (u8*)"" }};
	FSUSER_OpenFileDirectly(NULL, &fileHandle, archive, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);	
	FSFILE_GetSize(fileHandle, &size);
	unsigned char* in = (unsigned char*)malloc(size);
	if(!in) {
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		return 0;
	}
	FSFILE_Read(fileHandle, &bytesRead, 0x00, in, size);
	FSFILE_Close(fileHandle);
	svcCloseHandle(fileHandle);
    jpeg_mem_src(&cinfo, in, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_bytes = width * cinfo.num_components;
    u8* bgr_buffer = (u8*) malloc(width*height*cinfo.num_components);
    while (cinfo.output_scanline < cinfo.output_height) {
        u8* buffer_array[1];
        buffer_array[0] = bgr_buffer + (cinfo.output_scanline) * row_bytes;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    result->bitperpixel = 24;
    result->width = width;
    result->height = height;
    result->pixels = bgr_buffer;
	int i = 0;
	while (i < (width*height*3)){
		u8 tmp = result->pixels[i];
		result->pixels[i] = result->pixels[i+2];
		result->pixels[i+2] = tmp;
		i=i+3;
	}
	free(in);
    return result;
}

Bitmap* decodeBMPfile(const char* fname){
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
	u8* flipped = (u8*)malloc(result->width*result->height*result->bitperpixel);
	flipped = flipBitmap(flipped, result);
	free(result->pixels);
	result->pixels = flipped;
	int i = 0;
	while (i < (result->width*result->height*(result->bitperpixel / 8))){
		u8 tmp = result->pixels[i];
		result->pixels[i] = result->pixels[i+2];
		result->pixels[i+2] = tmp;
		i= i + (result->bitperpixel / 8);
	}
	
	return result;
}

Bitmap* decodeJpg(unsigned char* in,u64 size)
{
    Bitmap* result = (Bitmap*)malloc(sizeof(Bitmap));
    struct jpeg_decompress_struct cinfo;
	struct my_error_mgr jerr;
	cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, in, size);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);
    int width = cinfo.output_width;
    int height = cinfo.output_height;
    int row_bytes = width * cinfo.num_components;
    u8* bgr_buffer = (u8*) malloc(width*height*cinfo.num_components);
    while (cinfo.output_scanline < cinfo.output_height) {
        u8* buffer_array[1];
        buffer_array[0] = bgr_buffer + (cinfo.output_scanline) * row_bytes;
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    result->bitperpixel = 24;
    result->width = width;
    result->height = height;
    result->pixels = bgr_buffer;
    return result;
}

void saveJpg(char *filename, u32 *pixels, u32 width, u32 height){
	FILE *outfile = fopen(filename, "wb");
	struct jpeg_error_mgr jerr;
	struct jpeg_compress_struct cinfo;
	JSAMPROW row_pointer[1];
	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, outfile);
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;
	jpeg_set_defaults(&cinfo);
	cinfo.num_components = 3;
	cinfo.dct_method = JDCT_FLOAT;
	jpeg_set_quality(&cinfo, 100, TRUE);
	jpeg_start_compress(&cinfo, TRUE);
	while( cinfo.next_scanline < cinfo.image_height ){
		row_pointer[0] = (unsigned char*)&pixels[ (cinfo.next_scanline * cinfo.image_width * cinfo.input_components) / 4];
		jpeg_write_scanlines( &cinfo, row_pointer, 1 );
	}
	jpeg_finish_compress( &cinfo );
	jpeg_destroy_compress( &cinfo );
	fclose(outfile);
}