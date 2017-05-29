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
#include <string.h>
#include "include/luaplayer.h"
#include "include/graphics/Graphics.h"
#include "include/utils.h"
#define stringify(str) #str
#define VariableRegister(lua, value) do { lua_pushinteger(lua, value); lua_setglobal (lua, stringify(value)); } while(0)

int cur_screen;

static int lua_init(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");	
	#endif
	sf2d_init();
	cur_screen = 2;
	sf2d_set_clear_color(RGBA8(0x00, 0x00, 0x00, 0xFF));
	return 0;
}

static int lua_term(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	cur_screen = 2;
	sf2d_fini();
	return 0;
}

static int lua_refresh(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 1) && (argc != 2))  return luaL_error(L, "wrong number of arguments");
	#endif
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
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	sf2d_end_frame();
	return 0;
}

static int lua_flip(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	sf2d_swapbuffers();
	return 0;
}

static int lua_rect(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5 && argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	float x1 = luaL_checknumber(L,1);
	float x2 = luaL_checknumber(L,2);
	float y1 = luaL_checknumber(L,3);
	float y2 = luaL_checknumber(L,4);
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
	#ifndef SKIP_ERROR_HANDLING
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	if (argc == 6) radius = luaL_checknumber(L,6);
	if (radius == 0) sf2d_draw_rectangle(x1, y1, x2-x1, y2-y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	else sf2d_draw_rectangle_rotate(x1, y1, x2-x1, y2-y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF), radius);
	return 0;
}

static int lua_fillcircle(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 4) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	int radius = luaL_checkinteger(L,3);
	u32 color = luaL_checkinteger(L,4);
	#ifndef SKIP_ERROR_HANDLING
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	sf2d_draw_fill_circle(x, y, radius, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
}


static int lua_line(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5) return luaL_error(L, "wrong number of arguments");
	#endif
	float x1 = luaL_checknumber(L,1);
	float x2 = luaL_checknumber(L,2);
	float y1 = luaL_checknumber(L,3);
	float y2 = luaL_checknumber(L,4);
	#ifndef SKIP_ERROR_HANDLING
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	u32 color = luaL_checkinteger(L,5);
	sf2d_draw_line(x1, y1, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
}

static int lua_emptyrect(lua_State *L) {
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5) return luaL_error(L, "wrong number of arguments");
	#endif
	float x1 = luaL_checknumber(L,1);
	float x2 = luaL_checknumber(L,2);
	float y1 = luaL_checknumber(L,3);
	float y2 = luaL_checknumber(L,4);
	#ifndef SKIP_ERROR_HANDLING
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	u32 color = luaL_checkinteger(L,5);
	sf2d_draw_line(x1, y1, x1, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	sf2d_draw_line(x2, y1, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	sf2d_draw_line(x1, y2, x2, y2, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	sf2d_draw_line(x1, y1, x2, y1, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
}

static int lua_loadimg(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	char* text = (char*)(luaL_checkstring(L, 1));
	fileStream fileHandle;
	u32 bytesRead;
	u16 magic;
	u64 long_magic;
	if (strncmp("romfs:/",text,7) == 0){
		fileHandle.isRomfs = true;
		FILE* handle = fopen(text,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle.handle = (u32)handle;
	}else{
		fileHandle.isRomfs = false;
		FS_Path filePath = fsMakePath(PATH_ASCII, text);
		FS_Archive script=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		Result ret = FSUSER_OpenFileDirectly( &fileHandle.handle, script, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if (ret) return luaL_error(L, "file doesn't exist.");
		#endif
	}
	FS_Read(&fileHandle, &bytesRead, 0, &magic, 2);
	Bitmap* bitmap;
	if (magic == 0x5089){
		FS_Read(&fileHandle, &bytesRead, 0, &long_magic, 8);
		FS_Close(&fileHandle);
		if (long_magic == 0x0A1A0A0D474E5089) bitmap = decodePNGfile(text);
	}else if (magic == 0x4D42){
		FS_Close(&fileHandle);
		bitmap = decodeBMPfile(text);
	}else if (magic == 0xD8FF){
		FS_Close(&fileHandle);
		bitmap = decodeJPGfile(text);
	}
	#ifndef SKIP_ERROR_HANDLING
	if(!bitmap) return luaL_error(L, "Error loading image");
	#endif
	if (bitmap->bitperpixel == 24){
		int length = (bitmap->width * bitmap->height) << 2;
		u8* real_pixels = (u8*)malloc(length);
		int i = 0;
		int z = 0;
		while (i < length){
			real_pixels[i] = bitmap->pixels[z];
			real_pixels[i+1] = bitmap->pixels[z+1];
			real_pixels[i+2] = bitmap->pixels[z+2];
			real_pixels[i+3] = 0xFF;
			i = i + 4;
			z = z + 3;
		}
		free(bitmap->pixels);
		bitmap->pixels = real_pixels;
	}
	sf2d_texture *tex = sf2d_create_texture_mem_RGBA8(bitmap->pixels, bitmap->width, bitmap->height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
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

static int lua_convert(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	Bitmap* bitmap = (Bitmap*)(luaL_checkinteger(L, 1));
	#ifndef SKIP_ERROR_HANDLING
	if (bitmap->magic != 0x4C494D47) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	u8* real_pixels;
	u8* flipped = (u8*)malloc(bitmap->width * bitmap->height * (bitmap->bitperpixel >> 3));
	flipped = flipBitmap(flipped, bitmap);
	int length = (bitmap->width * bitmap->height) << 2;
	if (bitmap->bitperpixel == 24){		
		real_pixels = (u8*)malloc(length);
		int i = 0;
		int z = 0;
		while (i < length){
			real_pixels[i] = flipped[z+2];
			real_pixels[i+1] = flipped[z+1];
			real_pixels[i+2] = flipped[z];
			real_pixels[i+3] = 0xFF;
			i = i + 4;
			z = z + 3;
		}
		free(flipped);
	}else{
		real_pixels = flipped;
		int i = 0;
		while (i < length){
			u8 tmp = real_pixels[i+2];
			real_pixels[i+2] = real_pixels[i];
			real_pixels[i] = tmp;
			i = i + 4;
		}
	}
	sf2d_texture *tex = sf2d_create_texture_mem_RGBA8(real_pixels, bitmap->width, bitmap->height, TEXFMT_RGBA8, SF2D_PLACE_RAM);
	gpu_text* result = (gpu_text*)malloc(sizeof(gpu_text));
	result->magic = 0x4C545854;
	result->tex = tex;
	result->width = bitmap->width;
	result->height = bitmap->height;
	free(real_pixels);
	lua_pushinteger(L, (u32)(result));
	return 1;
}

static int lua_drawimg(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3 && argc != 4) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,3);
	u32 color = 0;
	if (argc == 4){ 
		color = luaL_checkinteger(L,4);
		#ifndef SKIP_ERROR_HANDLING
		if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
		#endif
		sf2d_draw_texture_blend(texture->tex, x, y, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	}else{
		#ifndef SKIP_ERROR_HANDLING
		if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
		#endif
		sf2d_draw_texture(texture->tex, x, y);
	}
	return 0;
}

static int lua_drawimg_scale(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5 && argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,3);
	float scale_x = luaL_checknumber(L,4);
	float scale_y = luaL_checknumber(L,5);
	u32 color;
	#ifndef SKIP_ERROR_HANDLING
	if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	if (argc == 6){
		color = luaL_checkinteger(L,6);
		sf2d_draw_texture_scale_blend(texture->tex, x, y, scale_x, scale_y, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	}else sf2d_draw_texture_scale(texture->tex, x, y, scale_x, scale_y);
	return 0;
}

static int lua_drawimg_rotate(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 4 && argc != 5) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,3);
	float radius = luaL_checknumber(L,4);
	u32 color;
	#ifndef SKIP_ERROR_HANDLING
	if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	if (argc == 5){
		color = luaL_checkinteger(L,5);
		sf2d_draw_texture_rotate_blend(texture->tex, x, y, radius, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	}else sf2d_draw_texture_rotate(texture->tex, x, y, radius);
	return 0;
}

static int lua_drawimg_full(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 10 && argc != 11) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	int st_x = luaL_checkinteger(L, 3);
	int st_y = luaL_checkinteger(L, 4);
	float width = luaL_checknumber(L, 5);
	float height = luaL_checknumber(L, 6);
	float radius = luaL_checknumber(L, 7);
	float scale_x = luaL_checknumber(L, 8);
	float scale_y = luaL_checknumber(L, 9);
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L, 10);
	u32 color;
	if (argc == 11) color = luaL_checkinteger(L, 11);
	#ifndef SKIP_ERROR_HANDLING
	if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	if (argc == 10) sf2d_draw_texture_part_rotate_scale(texture->tex, x, y, radius, st_x, st_y, width, height, scale_x, scale_y);
	else sf2d_draw_texture_part_rotate_scale_blend(texture->tex, x, y, radius, st_x, st_y, width, height, scale_x, scale_y, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	return 0;
}

static int lua_partial(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 7 && argc != 8) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L,1);
	float y = luaL_checknumber(L,2);
	int st_x = luaL_checkinteger(L, 3);
	int st_y = luaL_checkinteger(L, 4);
	float width = luaL_checknumber(L, 5);
	float height = luaL_checknumber(L, 6);
	gpu_text* file = (gpu_text*)luaL_checkinteger(L, 7);
	u32 color;
	if (argc == 8) color = luaL_checkinteger(L, 8);
	#ifndef SKIP_ERROR_HANDLING
	if (file->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	if (cur_screen != 1 && cur_screen != 0) return luaL_error(L, "you need to call initBlend to use GPU rendering");
	#endif
	if (argc == 8) sf2d_draw_texture_part_blend(file->tex, x, y, st_x, st_y, width, height, RGBA8((color >> 16) & 0xFF, (color >> 8) & 0xFF, (color) & 0xFF, (color >> 24) & 0xFF));
	else sf2d_draw_texture_part(file->tex, x, y, st_x, st_y, width, height);
	return 0;
}

static int lua_free(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	gpu_text* texture = (gpu_text*)luaL_checkinteger(L,1);
	#ifndef SKIP_ERROR_HANDLING
	if (texture->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	sf2d_free_texture(texture->tex);
	free(texture);
	return 0;
}

static int lua_getWidth(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	gpu_text* src = (gpu_text*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L,src->width);
	return 1;
}

static int lua_getHeight(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	gpu_text* src = (gpu_text*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (src->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	lua_pushinteger(L,src->height);
	return 1;
}

static int lua_viewport(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 x = luaL_checkinteger(L,1);
	u32 y = luaL_checkinteger(L,2);
	u32 w = luaL_checkinteger(L,3);
	u32 h = luaL_checkinteger(L,4);
	GPU_SCISSORMODE mode = (GPU_SCISSORMODE)luaL_checkinteger(L,5);
	sf2d_set_scissor_test(mode, x, y, w, h);
	return 0;
}

static int lua_pixel2(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if ((argc != 3) && (argc != 4)) return luaL_error(L, "wrong number of arguments");
	#endif
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	int screen = luaL_checkinteger(L,3);
	int side=0;
	if (argc == 4) side = luaL_checkinteger(L,4);
	#ifndef SKIP_ERROR_HANDLING
	if ((x < 0) || (y < 0)) return luaL_error(L, "out of bounds");
	if ((screen == 0) && (x > 400)) return luaL_error(L, "out of framebuffer bounds");
	if ((screen == 1) && (x > 320)) return luaL_error(L, "out of framebuffer bounds");
	if ((screen <= 1) && (y > 240)) return luaL_error(L, "out of framebuffer bounds");
	if ((screen > 1) && (((gpu_text*)screen)->magic != 0x4C545854)) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	if (screen > 1){
		u32 color = sf2d_get_pixel(((gpu_text*)screen)->tex, x, y);
		u32 res = (color >> 16) & 0xFF | (((color >> 8) & 0xFF) << 8) | ((color & 0xFF) << 16) | (((color >> 24) & 0xFF) << 24);
		lua_pushinteger(L,res);
	}else lua_pushinteger(L,GetPixel(x,y,screen,side));
	return 1;
}

//Register our Graphics Functions
static const luaL_Reg Graphics_functions[] = {
  {"init",                  lua_init},
  {"term",                  lua_term},
  {"initBlend",             lua_refresh},
  {"loadImage",             lua_loadimg},
  {"drawImage",             lua_drawimg},
  {"drawPartialImage",      lua_partial},
  {"drawRotateImage",       lua_drawimg_rotate},
  {"drawScaleImage",        lua_drawimg_scale},
  {"drawImageExtended",     lua_drawimg_full},
  {"fillRect",              lua_rect},
  {"fillEmptyRect",         lua_emptyrect},
  {"drawCircle",            lua_fillcircle},
  {"drawLine",              lua_line},
  {"termBlend",             lua_end},
  {"flip",                  lua_flip},
  {"freeImage",             lua_free},
  {"getImageWidth",         lua_getWidth},
  {"getImageHeight",        lua_getHeight}, 
  {"setViewport",           lua_viewport}, 
  {"getPixel",              lua_pixel2}, 
  {"convertFrom",           lua_convert},
  {0, 0}
};

void luaGraphics_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Graphics_functions, 0);
	lua_setglobal(L, "Graphics");
	u8 BORDER = 1;
	u8 CENTER = 3;
	VariableRegister(L,BORDER);
	VariableRegister(L,CENTER);
}