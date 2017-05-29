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
#include "include/graphics/Graphics.h"

#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)
#define WAIT_TIMEOUT 300000000ULL

u8* cam_buf;
u32 SCREEN_SIZE;
u32 bufSize;
int cam_type;
bool is3D;
CAMU_PhotoMode pmode = PHOTO_MODE_NORMAL;

void initCam(CAMU_Size res, bool enable3D, bool isVideo){
	u16 width;
	u16 height;
	if (res == SIZE_VGA){
		width = 640;
		height = 480;
	}else if (res == SIZE_QVGA){
		width = 320;
		height = 240;
	}else if (res == SIZE_QQVGA){
		width = 160;
		height = 120;
	}else if (res == SIZE_CIF){
		width = 352;
		height = 288;
	}else if (res == SIZE_QCIF){
		width = 176;
		height = 144;
	}else if (res == SIZE_DS_LCD){
		width = 256;
		height = 192;
	}else if (res == SIZE_DS_LCDx4){
		width = 512;
		height = 384;
	}else if (res == SIZE_CTR_TOP_LCD){
		width = 400;
		height = 240;
	}
	CAMU_SetSize(cam_type, res, CONTEXT_A);
	CAMU_SetOutputFormat(cam_type, OUTPUT_RGB_565, CONTEXT_A);
	if (isVideo) CAMU_SetFrameRate(cam_type, FRAME_RATE_10);
	CAMU_SetNoiseFilter(cam_type, true);
	CAMU_SetAutoExposure(cam_type, true);
	CAMU_SetAutoWhiteBalance(cam_type, true);
	if (enable3D) CAMU_SetTrimming(PORT_BOTH, false);
	else CAMU_SetTrimming(PORT_CAM1, false);
	CAMU_SetPhotoMode(cam_type, pmode);
	if (enable3D) CAMU_SetTrimming(PORT_CAM2, false);
	CAMU_GetMaxBytes(&bufSize, width, height);
	if (enable3D) CAMU_SetTransferBytes(PORT_BOTH, bufSize, width, height);
	else CAMU_SetTransferBytes(PORT_CAM1, bufSize, width, height);
	CAMU_Activate(cam_type);
	if (enable3D){
		CAMU_ClearBuffer(PORT_BOTH);
		CAMU_SynchronizeVsyncTiming(SELECT_OUT1, SELECT_OUT2);
		CAMU_StartCapture(PORT_BOTH);
	}else{
		CAMU_ClearBuffer(PORT_CAM1);
		CAMU_StartCapture(PORT_CAM1);
	}
}

static int lua_caminit(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3 && argc != 4) return luaL_error(L, "wrong number of arguments.");
	#endif
	int screen = luaL_checkinteger(L,1);
	cam_type = luaL_checkinteger(L,2);
	pmode = (CAMU_PhotoMode)luaL_checkinteger(L,3);
	is3D = false;
	if (argc == 3) is3D = lua_toboolean(L, 4);
	u16 width;
	u16 height = 240;
	camInit();
	CAMU_Size res = SIZE_CTR_TOP_LCD;
	if (is3D){
		screen = 0;
		cam_type = SELECT_OUT1_OUT2;
	}
	if (screen == 0) width = 400;
	else{
		width = 320;
		res = SIZE_QVGA;
	}
	initCam(res, is3D, true);
	SCREEN_SIZE = (width * height) << 1;
	if (is3D) cam_buf = (u8*)malloc(SCREEN_SIZE << 1);
	else cam_buf = (u8*)malloc(SCREEN_SIZE);
	return 0;
}

static int lua_camoutput(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	Handle camReceiveEvent = 0;
	Handle camReceiveEvent2 = 0;
	CAMU_SetReceiving(&camReceiveEvent, cam_buf, PORT_CAM1, SCREEN_SIZE, (s16)bufSize);
	if (is3D) CAMU_SetReceiving(&camReceiveEvent2, cam_buf + SCREEN_SIZE, PORT_CAM2, SCREEN_SIZE, (s16) bufSize);
	svcWaitSynchronization(camReceiveEvent, WAIT_TIMEOUT);
	if (is3D) svcWaitSynchronization(camReceiveEvent2, WAIT_TIMEOUT);
	if (SCREEN_SIZE == 153600) DrawRGB565Screen(BottomFB, (u16*)cam_buf);
	else{
		if (is3D){
			DrawRGB565Screen(TopLFB, (u16*)(cam_buf + SCREEN_SIZE));
			DrawRGB565Screen(TopRFB, (u16*)(cam_buf));
		}else DrawRGB565Screen(TopLFB, (u16*)cam_buf);
	}
	svcCloseHandle(camReceiveEvent);
	if (is3D) svcCloseHandle(camReceiveEvent2);
	return 0;
}

static int lua_camimage(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	Bitmap* result = (Bitmap*)malloc(sizeof(Bitmap));
	if (SCREEN_SIZE == 153600) result->width = 320;
	else result->width = 400;
	result->height = 240;
	result->magic = 0x4C494D47;
	result->bitperpixel = 24;
	u32 bytes_to_process = SCREEN_SIZE >> 1;
	result->pixels = (u8*)malloc(bytes_to_process * 3);
	u16* frame_buf = (u16*)cam_buf;
	u32 i = 0;
	u32 x;
	u32 y;
	for (y = 0; y < result->height; y++){
		for (x = 0; x < result->width; x++){
			int di = (x + (((result->height - 1) - y) * result->width)) * 3;
			u8 b = (frame_buf[i]&0x1F)<<3;
			u8 g = ((frame_buf[i]>>5)&0x3F)<<2;
			u8 r = ((frame_buf[i]>>11)&0x1F)<<3;
			result->pixels[di++] = b;
			result->pixels[di++] = g;
			result->pixels[di++] = r;
			i++;
		}
	}
	lua_pushinteger(L,(u32)result);
	return 1;
}

static int lua_camshot(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2 && argc != 3) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char *screenpath = luaL_checkstring(L, 1);
	CAMU_Size res = (CAMU_Size)luaL_checkinteger(L, 2);
	int width;
	int height;
	if (res == SIZE_VGA){
		width = 640;
		height = 480;
	}else if (res == SIZE_QVGA){
		width = 320;
		height = 240;
	}else if (res == SIZE_QQVGA){
		width = 160;
		height = 120;
	}else if (res == SIZE_CIF){
		width = 352;
		height = 288;
	}else if (res == SIZE_QCIF){
		width = 176;
		height = 144;
	}else if (res == SIZE_DS_LCD){
		width = 256;
		height = 192;
	}else if (res == SIZE_DS_LCDx4){
		width = 512;
		height = 384;
	}else if (res == SIZE_CTR_TOP_LCD){
		width = 400;
		height = 240;
	}
	bool isJPG = false;
	if (argc == 3) isJPG = lua_toboolean(L, 3);
	if (is3D) CAMU_StopCapture(PORT_BOTH);
	else CAMU_StopCapture(PORT_CAM1);
	camExit();
	camInit();
	initCam(res, false, true);
	u32 BUFFER_SIZE = width*height*3;
	u32 framebuf_size = (width*height)<<1;
	u8* framebuf = (u8*)malloc(framebuf_size);
	u16* frame_buf = (u16*)framebuf;
	Handle camReceiveEvent = 0;
	CAMU_SetReceiving(&camReceiveEvent, framebuf, PORT_CAM1, framebuf_size, (s16)bufSize);
	svcWaitSynchronization(camReceiveEvent, WAIT_TIMEOUT);
	CAMU_PlayShutterSound(SHUTTER_SOUND_TYPE_NORMAL);
	Handle fileHandle;
	int x, y;
	FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
	if (!isJPG){ //BMP Format
		FS_Path filePath=fsMakePath(PATH_ASCII, screenpath);
		Result ret=FSUSER_OpenFileDirectly( &fileHandle, sdmcArchive, filePath, FS_OPEN_CREATE|FS_OPEN_WRITE, 0x00000000);
		u32 bytesWritten;
		u8* tempbuf = (u8*)malloc(0x36 + BUFFER_SIZE);
		memset(tempbuf, 0, 0x36+BUFFER_SIZE);
		FSFILE_SetSize(fileHandle, (u16)(0x36 + BUFFER_SIZE));
		*(u16*)&tempbuf[0x0] = 0x4D42;
		*(u32*)&tempbuf[0x2] = 0x36 + BUFFER_SIZE;
		*(u32*)&tempbuf[0xA] = 0x36;
		*(u32*)&tempbuf[0xE] = 0x28;
		*(u32*)&tempbuf[0x12] = width;
		*(u32*)&tempbuf[0x16] = height;
		*(u32*)&tempbuf[0x1A] = 0x00180001;
		*(u32*)&tempbuf[0x22] = BUFFER_SIZE;
		int si = 0;
		for (y = 0; y < height; y++){
			for (x = 0; x < width; x++){
				int di = 0x36 + (x + (((height - 1) - y) * width)) * 3;
				u8 b = (frame_buf[si]&0x1F)<<3;
				u8 g = ((frame_buf[si]>>5)&0x3F)<<2;
				u8 r = ((frame_buf[si]>>11)&0x1F)<<3;
				tempbuf[di++] = b;
				tempbuf[di++] = g;
				tempbuf[di++] = r;
				si++;
			}	
		}
		FSFILE_Write(fileHandle, &bytesWritten, 0, (u32*)tempbuf, 0x36 + BUFFER_SIZE, 0x10001);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		free(tempbuf);
	}else{
		u8* tempbuf = (u8*)malloc(BUFFER_SIZE);
		int si = 0;
		for (y = 0; y < height; y++){
			for (x = 0; x < width; x++){
				int di = (x + (y*width)) * 3;
				u8 b = (frame_buf[si]&0x1F)<<3;
				u8 g = ((frame_buf[si]>>5)&0x3F)<<2;
				u8 r = ((frame_buf[si]>>11)&0x1F)<<3;
				tempbuf[di++] = b;
				tempbuf[di++] = g;
				tempbuf[di++] = r;
				si++;
			}	
		}
		sdmcInit();
		char tmpPath2[1024];
		strcpy(tmpPath2,"sdmc:");
		strcat(tmpPath2,(char*)screenpath);
		saveJpg(tmpPath2,(u32*)tempbuf,width,height);
		sdmcExit();
		free(tempbuf);
	}
	svcCloseHandle(camReceiveEvent);
	CAMU_StopCapture(PORT_CAM1);
	CAMU_Activate(SELECT_NONE);
	free(framebuf);
	camExit();
	camInit();
	res = SIZE_CTR_TOP_LCD;
	if (SCREEN_SIZE == 153600) res = SIZE_QVGA;
	initCam(res, is3D, true);
	return 0;
}

static int lua_camexit(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments.");
	#endif
	if (is3D) CAMU_StopCapture(PORT_BOTH);
	else CAMU_StopCapture(PORT_CAM1);
	CAMU_Activate(SELECT_NONE);
	free(cam_buf);
	camExit();
	return 0;
}

//Register our Camera Functions
static const luaL_Reg Camera_functions[] = {
	{"init",       lua_caminit},
	{"getOutput",  lua_camoutput},
	{"takePhoto",  lua_camshot},
	{"takeImage",  lua_camimage},
	{"term",       lua_camexit},
	{0, 0}
};

void luaCamera_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Camera_functions, 0);
	lua_setglobal(L, "Camera");
	
	// Camera combinations
	u8 INNER_CAM = SELECT_IN1;
	u8 OUTER_CAM = SELECT_OUT1;
	VariableRegister(L,INNER_CAM);
	VariableRegister(L,OUTER_CAM);
	
	// Camera Resolutions
	u8 VGA_RES = SIZE_VGA;
	u8 QVGA_RES = SIZE_QVGA;
	u8 QQVGA_RES = SIZE_QVGA;
	u8 CIF_RES = SIZE_CIF;
	u8 QCIF_RES = SIZE_QCIF;
	u8 DS_RES = SIZE_DS_LCD;
	u8 HDS_RES = SIZE_DS_LCDx4;
	u8 CTR_RES = SIZE_CTR_TOP_LCD;
	VariableRegister(L,VGA_RES);
	VariableRegister(L,QVGA_RES);
	VariableRegister(L,QQVGA_RES);
	VariableRegister(L,CIF_RES);
	VariableRegister(L,QCIF_RES);
	VariableRegister(L,DS_RES);
	VariableRegister(L,HDS_RES);
	VariableRegister(L,CTR_RES);
	VariableRegister(L,PHOTO_MODE_NORMAL);
	VariableRegister(L,PHOTO_MODE_PORTRAIT);
	VariableRegister(L,PHOTO_MODE_LANDSCAPE);
	VariableRegister(L,PHOTO_MODE_NIGHTVIEW);
	VariableRegister(L,PHOTO_MODE_LETTER);
}