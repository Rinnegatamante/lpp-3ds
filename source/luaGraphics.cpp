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
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/Graphics/Graphics.h"
extern "C"{
	#include "include/sf2d/sf2d.h"
}

struct gpu_text{
	u32 magic;
	u16 width;
	u16 height;
	sf2d_texture* tex;
};

int cur_screen;

static int lua_init(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");	
    sf2d_init();
	cur_screen = 2;
	sf2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
    return 0;
}

static int lua_term(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
	cur_screen = 2;
    sf2d_fini();
    return 0;
}

static int lua_refresh(lua_State *L) {
    int argc = lua_gettop(L);
	if ((argc != 1) && (argc != 2))  return luaL_error(L, "wrong number of arguments");
	int screen = luaL_checkinteger(L,1);
	int side=0;
	if (argc == 2) side = luaL_checkinteger(L,2);
	gfxScreen_t my_screen;
	gfx3dSide_t eye;
	cur_screen = screen;
	if (screen == 0) my_screen = GFX_TOP;
	else my_screen = GFX_BOTTOM;
	if (side == 0) eye = GFX_LEFT;
	else eye = GFX_RIGHT;
    sf2d_start_frame(my_screen,eye);
    return 0;
}

static int lua_end(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
    sf2d_end_frame();
    return 0;
}

static int lua_flip(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 0) return luaL_error(L, "wrong number of arguments");
    sf2d_swapbuffers();
    return 0;
}

static int lua_rect(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 5 && argc != 6) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	float radius = 0;
	if (x2 < x1){
		int tmp = x2;
		x2 = x1;
		x1 = tmp;
	}
	if (y2 < y1){
		int tmp = y2;
		y2 = y1;
		y1 = tmp;
	}
	u32 color = luaL_checkinteger(L,5);
	if (argc == 6) radius = luaL_checknumber(L,6);
	if (radius == 0){
		#ifndef SKIP_ERROR_HANDLING
			if ((x1 < 0) || (y1 < 0) || (x2 < 0) || (y2 < 0)) return luaL_error(L, "out of bounds");
			if ((cur_screen == 0) && ((x1 > 400) || (x2 > 400))) return luaL_error(L, "out of framebuffer bounds");
			if ((cur_screen == 1) && ((x1 > 320) || (x2 > 320))) return luaL_error(L, "out of framebuffer bounds");
			if (y1 > 240 || y2 > 240) return luaL_error(L, "out of framebuffer bounds");
			if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
		#endif
		sf2d_draw_rectangle(x1, y1, x2-x1, y2-y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
    }else{
		#ifndef SKIP_ERROR_HANDLING
			if ((x1 < 0) || (y1 < 0)) return luaL_error(L, "out of bounds");
			if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
		#endif
		sf2d_draw_rectangle_rotate(x1, y1, x2-x1, y2-y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF), radius);
	}
	return 0;
}

static int lua_fillcircle(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 5 && argc != 6) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	int radius = luaL_checkinteger(L,3);
	u32 color = luaL_checkinteger(L,4);
	#ifndef SKIP_ERROR_HANDLING
		if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
		if ((cur_screen == 0) && (x > 400)) return luaL_error(L, "out of framebuffer bounds");
		if ((cur_screen == 1) && (x > 320)) return luaL_error(L, "out of framebuffer bounds");
		if (y > 240) return luaL_error(L, "out of framebuffer bounds");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	sf2d_draw_fill_circle(x, y, radius, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
}


static int lua_line(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 5) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	#ifndef SKIP_ERROR_HANDLING
		if ((x1 < 0) || (y1 < 0) || (x2 < 0) || (y2 < 0)) return luaL_error(L, "out of bounds");
		if ((cur_screen == 0) && ((x1 > 400) || (x2 > 400))) return luaL_error(L, "out of framebuffer bounds");
		if ((cur_screen == 1) && ((x1 > 320) || (x2 > 320))) return luaL_error(L, "out of framebuffer bounds");
		if (y1 > 240 || y2 > 240) return luaL_error(L, "out of framebuffer bounds");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	u32 color = luaL_checkinteger(L,5);
    sf2d_draw_line(x1, y1, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
    return 0;
}

static int lua_emptyrect(lua_State *L) {
    int argc = lua_gettop(L);
    if (argc != 5) return luaL_error(L, "wrong number of arguments");
	int x1 = luaL_checkinteger(L,1);
	int x2 = luaL_checkinteger(L,2);
	int y1 = luaL_checkinteger(L,3);
	int y2 = luaL_checkinteger(L,4);
	#ifndef SKIP_ERROR_HANDLING
		if ((x1 < 0) || (y1 < 0) || (x2 < 0) || (y2 < 0)) return luaL_error(L, "out of bounds");
		if ((cur_screen == 0) && ((x1 > 400) || (x2 > 400))) return luaL_error(L, "out of framebuffer bounds");
		if ((cur_screen == 1) && ((x1 > 320) || (x2 > 320))) return luaL_error(L, "out of framebuffer bounds");
		if (y1 > 240 || y2 > 240) return luaL_error(L, "out of framebuffer bounds");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	u32 color = luaL_checkinteger(L,5);
    sf2d_draw_line(x1, y1, x1, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
    sf2d_draw_line(x2, y1, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	sf2d_draw_line(x1, y2, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	sf2d_draw_line(x1, y1, x2, y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
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
	Bitmap* bitmap;
	if (magic == 0x5089){
		FSFILE_Read(fileHandle, &bytesRead, 0, &long_magic, 8);
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		if (long_magic == 0x0A1A0A0D474E5089) bitmap = decodePNGfile(text);
	}else if (magic == 0x4D42){
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		bitmap = decodeBMPfile(text);
	}else if (magic == 0xD8FF){
		FSFILE_Close(fileHandle);
		svcCloseHandle(fileHandle);
		bitmap = decodeJPGfile(text);
	}
	if(!bitmap) return luaL_error(L, "Error loading image");
	if (bitmap->bitperpixel == 24){
		int length = bitmap->width*bitmap->height;
		u8* real_pixels = (u8*)malloc(length * 4);
		int i = 0;
		int z = 0;
		while (i < length){
			real_pixels[i] = bitmap->pixels[i-z];
			real_pixels[i+1] = bitmap->pixels[i-z+1];
			real_pixels[i+2] = bitmap->pixels[i-z+2];
			real_pixels[i+3] = 0xFF;
			i = i + 4;
			z++;
		}
		free(bitmap->pixels);
		bitmap->pixels = real_pixels;
	}
	sf2d_texture *tex = sf2d_create_texture(bitmap->width, bitmap->height, GPU_RGBA8, SF2D_PLACE_RAM);
	sf2d_fill_texture_from_RGBA8(tex, (u32*)bitmap->pixels, bitmap->width, bitmap->height);
	sf2d_texture_tile32(tex);
	gpu_text* result = (gpu_text*)malloc(sizeof(gpu_text));
	result->magic = 0x4C545854;
	result->tex = tex;
	result->width = bitmap->width;
	result->height = bitmap->height;
	free(bitmap->pixels);
	free(bitmap);
    lua_pushinteger(L, (u32)(result));
	return 1;
}

static int lua_drawimg(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 3 && argc != 4 && argc != 5) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,3);
	float scale_x = 1;
	float scale_y = 1;
	if (argc > 4){
		scale_x = luaL_checknumber(L,4);
		if (argc == 5) scale_y = luaL_checknumber(L,5);
	}
	if (scale_y != 1 or scale_x != 1){
		#ifndef SKIP_ERROR_HANDLING
		if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
		#endif
		sf2d_draw_texture_scale(texture->tex, x, y, scale_x, scale_y);
	}else{
		#ifndef SKIP_ERROR_HANDLING
			if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
			if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
			if ((cur_screen == 0) && (x + texture->width > 400)) return luaL_error(L, "out of framebuffer bounds");
			if ((cur_screen == 1) && (x + texture->width > 320)) return luaL_error(L, "out of framebuffer bounds");
			if (y + texture->height > 240) return luaL_error(L, "out of framebuffer bounds");
			if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
		#endif
		sf2d_draw_texture(texture->tex, x, y);
	}
	return 0;
}

static int lua_drawimg_full(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 10) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
	int st_x = luaL_checkinteger(L, 3);
    int st_y = luaL_checkinteger(L, 4);
	int width = luaL_checkinteger(L, 5);
    int height = luaL_checkinteger(L, 6);
	float radius = luaL_checknumber(L, 7);
	float scale_x = luaL_checknumber(L, 8);
	float scale_y = luaL_checknumber(L, 9);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L, 10);
	#ifndef SKIP_ERROR_HANDLING
		if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	sf2d_draw_texture_rotate_cut_scale(texture->tex, x, y, radius, st_x, st_y, width, height, scale_x, scale_y);
	return 0;
}

static int lua_partial(lua_State *L){
	int argc = lua_gettop(L);
	if (argc != 7) return luaL_error(L, "wrong number of arguments");
	int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
	int st_x = luaL_checkinteger(L, 3);
    int st_y = luaL_checkinteger(L, 4);
	int width = luaL_checkinteger(L, 5);
    int height = luaL_checkinteger(L, 6);
	gpu_text* file = (gpu_text*)luaL_checkinteger(L, 7);
	#ifndef SKIP_ERROR_HANDLING
		if (file->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
		if ((st_x < 0) || (st_y < 0)) return luaL_error(L, "out of image bounds");
		if (((st_x + width) > file->width) || (((st_y + height) > file->height))) return luaL_error(L, "out of image bounds");
		if ((cur_screen == 0) && ((x+width) > 400)) return luaL_error(L, "out of framebuffer bounds");
		if ((cur_screen == 1) && ((x+width) > 320)) return luaL_error(L, "out of framebuffer bounds");
		if ((cur_screen <= 1) && ((y+height) > 240)) return luaL_error(L, "out of framebuffer bounds");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	sf2d_draw_texture_part(file->tex, x, y, st_x, st_y, width, height);
	return 0;
}

static int lua_free(lua_State *L)
{
    int argc = lua_gettop(L);
    if (argc != 1) return luaL_error(L, "wrong number of arguments");
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,1);
	#ifndef SKIP_ERROR_HANDLING
		if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	sf2d_free_texture(texture->tex);
	free(texture);
	return 0;
}

//Register our Graphics Functions
static const luaL_Reg Graphics_functions[] = {
  {"init",					lua_init},
  {"term",					lua_term},
  {"initBlend",				lua_refresh},
  {"loadImage",				lua_loadimg},
  {"drawImage",				lua_drawimg},
  {"drawPartialImage",		lua_partial},
  {"drawImageExtended",		lua_drawimg_full},
  {"fillRect",				lua_rect},
  {"fillEmptyRect",			lua_emptyrect},
  {"drawCircle",			lua_fillcircle},
  {"drawLine",				lua_line},
  {"termBlend",				lua_end},
  {"flip",					lua_flip},
  {"freeImage",				lua_free},
  {0, 0}
};

void luaGraphics_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Graphics_functions, 0);
	lua_setglobal(L, "Graphics");
}