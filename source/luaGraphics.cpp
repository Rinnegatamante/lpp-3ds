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
#- Aurelio for testing & bug-fixing ------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaGraphics.h"
#include "include/font.h"

typedef unsigned short u16;
u8* TopFB;
u8* BottomFB;
Bitmap LoadBitmap(char* fname){
Handle fileHandle;
  u64 size;
  u32 bytesRead;
  unsigned char *buffer;
  FS_path filePath=FS_makePath(PATH_CHAR, fname);
  FS_archive script=(FS_archive){ARCH_SDMC, (FS_path){PATH_EMPTY, 1, (u8*)""}};
  FSUSER_OpenFileDirectly(NULL, &fileHandle, script, filePath, FS_OPEN_READ, FS_ATTRIBUTE_NONE);
  FSFILE_GetSize(fileHandle, &size);
  Bitmap result;
  result.pixels = (u8*)malloc(size-0x36);
  u16 header;
  FSFILE_Read(fileHandle, &bytesRead, 0, &header, 2);
  if (header == 0x4D42){
  FSFILE_Read(fileHandle, &bytesRead, 0x36, result.pixels, size-0x36);
  FSFILE_Read(fileHandle, &bytesRead, 0x12, &(result.width), 4);
  FSFILE_Read(fileHandle, &bytesRead, 0x16, &(result.height), 4);
  }
FSFILE_Close(fileHandle);
svcCloseHandle(fileHandle);
return result;
}

void PrintBitmap(int xp,int yp, Bitmap result,int screen){
int x, y;
	for (y = 0; y < result.height; y++){
		for (x = 0; x < result.width; x++){
			u8 B = result.pixels[(x + (result.height - y) * result.width)*3];
			u8 G = result.pixels[(x + (result.height - y) * result.width)*3 + 1];
			u8 R = result.pixels[(x + (result.height - y) * result.width)*3 + 2];
			u32 color = B + G*256 + R*256*256;
			DrawPixel(xp+x,yp+y,color,screen);
		}
	}
}

void DrawPixel(int x,int y,u32 color,int screen){
int idx = ((x)*240) + (239-(y));
if (screen == 0){
TopFB[idx*3+0] = (color);
TopFB[idx*3+1] = (color) >> 8;
TopFB[idx*3+2] = (color) >> 16;
}else{
BottomFB[idx*3+0] = (color);
BottomFB[idx*3+1] = (color) >> 8;
BottomFB[idx*3+2] = (color) >> 16;
}
}

void RefreshScreen(){
TopFB = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
BottomFB = gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL);
}

void DrawText(int x, int y, char* str, u32 color, int screen){
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
DrawPixel(x+cx, y+cy, color, screen);
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
DrawPixel(x+cx, y+cy, 0xFFFFFF, 1);
}
}
x += glyphsize;
x++;
}
}

void FillRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			DrawPixel(x1,y1,color,screen);
			y1++;
		}
		y1 = base_y;
		x1++;
	}
}

void FillEmptyRect(int x1,int x2,int y1,int y2,u32 color,int screen){
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
			DrawPixel(x1,y1,color,screen);
			DrawPixel(x2,y1,color,screen);
			y1++;
		}
	while (x1 <= x2){
		DrawPixel(x1,base_y,color,screen);
		DrawPixel(x1,y2,color,screen);
		x1++;
	}
}

void ClearScreen(int screen){
	if (screen==1){
		FillRect(0,319,0,239,0x000000,1);
	}else{
		FillRect(0,399,0,239,0x000000,0);
	}
}