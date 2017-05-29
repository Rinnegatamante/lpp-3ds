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
#include "vshader_shbin.h"
#include "include/graphics/Graphics.h"
#include "include/utils.h"

u32 CLEAR_COLOR = 0x68B0D8FF;
float light_r = 1.0f;
float light_g = 1.0f;
float light_b = 1.0f;
float light_a = 1.0f;
float light_x = 0.0f;
float light_y = 0.0f;
float light_z = -1.0f;

#define DISPLAY_TRANSFER_FLAGS \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO))

static DVLB_s* vshader_dvlb;
static shaderProgram_s program;
static int uLoc_projection, uLoc_modelView;
static int uLoc_lightVec, uLoc_lightHalfVec, uLoc_lightClr, uLoc_material;
static C3D_Mtx projection;
static C3D_RenderTarget* targets[3];

static int lua_newVertex(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 8) return luaL_error(L, "wrong number of arguments");
	#endif
	vertex* res = (vertex*)malloc(sizeof(vertex));
	res->x = luaL_checknumber(L, 1);
	res->y = luaL_checknumber(L, 2);
	res->z = luaL_checknumber(L, 3);
	res->t1 = luaL_checknumber(L, 4);
	res->t2 = luaL_checknumber(L, 5);
	res->n1 = luaL_checknumber(L, 6);
	res->n2 = luaL_checknumber(L, 7);
	res->n3 = luaL_checknumber(L, 8);
	lua_pushinteger(L,(u32)res);
	return 1;
}

static int lua_loadobj(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	const char *file_tbo = luaL_checkstring(L, 1); //Model filename
	const char* text = luaL_checkstring(L, 2); // Texture filename
	color* ambient = (color*)luaL_checkinteger(L, 3);
	color* diffuse = (color*)luaL_checkinteger(L, 4);
	color* specular = (color*)luaL_checkinteger(L, 5);
	#ifndef SKIP_ERROR_HANDLING
	if (ambient->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (diffuse->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (specular->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	float emission = luaL_checknumber(L, 6);
	
	// Opening texture file
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
	
	// Flipping texture
	u8* flipped = (u8*)malloc(bitmap->width*bitmap->height*(bitmap->bitperpixel >> 3));
	flipped = flipBitmap(flipped, bitmap);
	free(bitmap->pixels);
	bitmap->pixels = flipped;
	
	// Converting 24bpp texture to a 32bpp ones
	int length = (bitmap->width * bitmap->height)<<2;
	if (bitmap->bitperpixel == 24){
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
	
	// Converting texture to a tiled ones
	u8 *tmp2 = (u8*)linearAlloc(length);
	int i, j;
	for (j = 0; j < bitmap->height; j++) {
		for (i = 0; i < bitmap->width; i++) {

			u32 coarse_y = j & ~7;
			u32 dst_offset = get_morton_offset(i, j, 4) + ((coarse_y * bitmap->width) << 2);

			u32 v = ((u32 *)bitmap->pixels)[i + (bitmap->height - 1 - j)*bitmap->width];
			*(u32 *)(tmp2 + dst_offset) = __builtin_bswap32(v); /* RGBA8 -> ABGR8 */
		}
	}
	memcpy(bitmap->pixels, tmp2, length);
	
	// Opening model file
	if (strncmp("romfs:/",file_tbo,7) == 0){
		fileHandle.isRomfs = true;
		FILE* handle = fopen(file_tbo,"r");
		#ifndef SKIP_ERROR_HANDLING
		if (handle == NULL) return luaL_error(L, "file doesn't exist.");
		#endif
		fileHandle.handle = (u32)handle;
	}else{
		fileHandle.isRomfs = false;
		FS_Archive sdmcArchive=(FS_Archive){ARCHIVE_SDMC, (FS_Path){PATH_EMPTY, 1, (u8*)""}};
		FS_Path filePath=fsMakePath(PATH_ASCII, file_tbo);
		Result ret=FSUSER_OpenFileDirectly(&fileHandle.handle, sdmcArchive, filePath, FS_OPEN_READ, 0x00000000);
		#ifndef SKIP_ERROR_HANDLING
		if(ret) return luaL_error(L, "error opening file");
		#endif
	}
	
	// Loading file on RAM
	u64 size;
	FS_GetSize(&fileHandle, &size);
	char* content = (char*)malloc(size+1);
	FS_Read(&fileHandle, &bytesRead, 0, content, size);
	content[size] = 0;
	
	// Closing file
	FS_Close(&fileHandle);
	
	// Creating temp vertexList
	vertexList* vl = (vertexList*)malloc(sizeof(vertexList));
	vertexList* init = vl;
	
	// Init variables
	char* str = content;
	char* ptr = strstr(str,"v ");
	int idx;
	char float_val[16];
	char* init_val;
	char magics[3][3] = {"v ","vt","vn"};
	int magics_idx = 0;
	vertex* res;
	int v_idx = 0;
	bool skip = false;
	char* end_vert;
	char* end_val;
	float* vert_args;
	vertexList* old_vl;
	
	// Vertices extraction
	for(;;){
		
		// Check if a magic change is needed
		while (ptr == NULL){
			if (magics_idx < 2){
				res = init->vert;
				vl = init;
				magics_idx++;
				ptr = strstr(str,magics[magics_idx]);
			}else{
				skip = true;
				break;
			}
		}
		if (skip) break;
		
		// Extract vertex
		if (magics_idx == 0) idx = 0;
		else if (magics_idx == 1) idx = 3;
		else idx = 5;
		if (magics_idx == 0) init_val = ptr + 2;
		else init_val = ptr + 3;
		while (init_val[0] == ' ') init_val++;
		end_vert = strstr(init_val,"\n");
		if (magics_idx == 0) res = (vertex*)malloc(sizeof(vertex));
		end_val = strstr(init_val," ");
		vert_args = (float*)res; // Hacky way to iterate in vertex struct		
		while (init_val < end_vert){
			if (end_val > end_vert) end_val = end_vert;
			strncpy(float_val, init_val, end_val - init_val);
			float_val[end_val - init_val] = 0;
			vert_args[idx] = atof(float_val);
			idx++;
			init_val = end_val + 1;
			while (init_val[0] == ' ') init_val++;
			end_val = strstr(init_val," ");
		}
		
		// Update vertexList struct
		if (magics_idx == 0){
			vl->vert = res;
			vl->next = (vertexList*)malloc(sizeof(vertexList));
		}
		old_vl = vl;
		vl = vl->next;
		if (magics_idx == 0){
			vl->vert = NULL;
			vl->next = NULL;
		}else{
			if (vl == NULL){
				old_vl->next = (vertexList*)malloc(sizeof(vertexList));
				vl = old_vl->next;
				vl->vert = (vertex*)malloc(sizeof(vertex));
				vl->next = NULL;
			}else if(vl->vert == NULL) vl->vert = (vertex*)malloc(sizeof(vertex));
			res = vl->vert;
		}
		
		// Searching for next vertex
		str = ptr + 1;
		ptr = strstr(str,magics[magics_idx]);
		
	}

	// Creating real vertexList
	ptr = strstr(str, "f ");
	vertexList* faces = (vertexList*)malloc(sizeof(vertexList));
	vertexList* initFaces = faces;
	faces->vert = NULL;
	faces->next = NULL;
	int len = 0;
	char val[8];
	int f_idx;
	char* ptr2;
	int t_idx;
	vertexList* tmp;
	
	// Faces extraction
	while (ptr != NULL){
		
		// Skipping padding
		ptr+=2;		
		
		// Extracting face info
		f_idx = 0;
		while (f_idx < 3){
		
			// Allocating new vertex
			faces->vert = (vertex*)malloc(sizeof(vertex));
		
			// Extracting x,y,z
			ptr2 = strstr(ptr,"/");
			strncpy(val,ptr,ptr2-ptr);
			val[ptr2-ptr] = 0;
			v_idx = atoi(val);
			t_idx = 1;
			tmp = init;
			while (t_idx < v_idx){
				tmp = tmp->next;
				t_idx++;
			}
			faces->vert->x = tmp->vert->x;
			faces->vert->y = tmp->vert->y;
			faces->vert->z = tmp->vert->z;
			
			// Extracting texture info
			ptr = ptr2+1;
			ptr2 = strstr(ptr,"/");
			if (ptr2 != ptr){
				strncpy(val,ptr,ptr2-ptr);
				val[ptr2-ptr] = 0;
				v_idx = atoi(val);
				t_idx = 1;
				tmp = init;
				while (t_idx < v_idx){
					tmp = tmp->next;
					t_idx++;
				}
				faces->vert->t1 = tmp->vert->t1;
				faces->vert->t2 = tmp->vert->t2;
			}else{
				faces->vert->t1 = 0.0f;
				faces->vert->t2 = 0.0f;
			}
			
			// Extracting normals info
			ptr = ptr2+1;
			if (f_idx < 2) ptr2 = strstr(ptr," ");
			else{
				ptr2 = strstr(ptr,"\n");
				if (ptr2 == NULL) ptr2 = content + size;
			}
			strncpy(val,ptr,ptr2-ptr);
			val[ptr2-ptr] = 0;
			v_idx = atoi(val);
			t_idx = 1;
			tmp = init;
			while (t_idx < v_idx){
				tmp = tmp->next;
				t_idx++;
			}
			faces->vert->n1 = tmp->vert->n1;
			faces->vert->n2 = tmp->vert->n2;
			faces->vert->n3 = tmp->vert->n3;

			// Setting values for next vertex
			ptr = ptr2;
			faces->next = (vertexList*)malloc(sizeof(vertexList));
			faces = faces->next;
			faces->next = NULL;
			faces->vert = NULL;
			len++;
			f_idx++;
		}
		
		ptr = strstr(ptr,"f ");
		
	}
	
	// Freeing temp vertexList and allocated file
	free(content);
	vertexList* tmp_init;
	while (init != NULL){
		tmp_init = init;
		free(init->vert);
		init = init->next;
		free(tmp_init);
	}
	
	// Create the VBO (vertex buffer object)
	u32 vertex_size = (len<<3)*sizeof(float);
	u8* vbo_data = (u8*)linearAlloc(vertex_size);
	for(int i = 0; i < len; i++) {
		tmp_init = initFaces;
		memcpy(&vbo_data[i*sizeof(vertex)], initFaces->vert, sizeof(vertex));
		initFaces = initFaces->next;
		free(tmp_init->vert);
		free(tmp_init);
	}
	
	// Setting texture
	C3D_Tex* texture = (C3D_Tex*)linearAlloc(sizeof(C3D_Tex));
	C3D_TexInit(texture , bitmap->width, bitmap->height, GPU_RGBA8);
	C3D_TexUpload(texture, bitmap->pixels);
	C3D_TexSetFilter(texture, GPU_LINEAR, GPU_NEAREST);
	free(bitmap->pixels);
	free(bitmap);
	linearFree(tmp2);
	
	// Set object material attributes
	C3D_Mtx* material = (C3D_Mtx*)linearAlloc(sizeof(C3D_Mtx));
	*material = {
		{
		{ { 0.0f, ambient->r, ambient->g, ambient->b } }, // Ambient
		{ { 0.0f, diffuse->r, diffuse->g, diffuse->b } }, // Diffuse
		{ { 0.0f, specular->r, specular->g, specular->b } }, // Specular
		{ { emission, 0.0f, 0.0f, 0.0f } }, // Emission
		}
	};
	
	// Create a model object and push it into Lua stack
	model* res_m = (model*)malloc(sizeof(model));
	res_m->vertex_count = len;
	res_m->vbo_data = vbo_data;
	res_m->magic = 0xC00FFEEE;
	res_m->texture = texture;
	res_m->material = material;
	lua_pushinteger(L, (u32)res_m);
	return 1;
}

static int lua_init(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 w = luaL_checkinteger(L, 1);
	u32 h = luaL_checkinteger(L, 2);
	u32 CLEAR_COLOR = ARGB2RGBA(luaL_checkinteger(L, 3));
	
	// Initialize graphics
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);

	// Initialize the render targets
	targets[0] = C3D_RenderTargetCreate(h, w, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(targets[0], C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(targets[0], GFX_TOP, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	targets[1] = C3D_RenderTargetCreate(h, w, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(targets[1], C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(targets[1], GFX_TOP, GFX_RIGHT, DISPLAY_TRANSFER_FLAGS);
	targets[2] = C3D_RenderTargetCreate(h, w, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
	C3D_RenderTargetSetClear(targets[2], C3D_CLEAR_ALL, CLEAR_COLOR, 0);
	C3D_RenderTargetSetOutput(targets[2], GFX_BOTTOM, GFX_LEFT, DISPLAY_TRANSFER_FLAGS);
	
	// Load the vertex shader, create a shader program and bind it
	vshader_dvlb = DVLB_ParseFile((u32*)vshader_v_pica_o, vshader_v_pica_o_size);
	shaderProgramInit(&program);
	shaderProgramSetVsh(&program, &vshader_dvlb->DVLE[0]);
	C3D_BindProgram(&program);
	
	// Get the location of the uniforms
	uLoc_projection   = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
	uLoc_modelView	= shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
	uLoc_lightVec	 = shaderInstanceGetUniformLocation(program.vertexShader, "lightVec");
	uLoc_lightHalfVec = shaderInstanceGetUniformLocation(program.vertexShader, "lightHalfVec");
	uLoc_lightClr	 = shaderInstanceGetUniformLocation(program.vertexShader, "lightClr");
	uLoc_material	 = shaderInstanceGetUniformLocation(program.vertexShader, "material");
	
	// Configure attributes for use with the vertex shader
	C3D_AttrInfo* attrInfo = C3D_GetAttrInfo();
	AttrInfo_Init(attrInfo);
	AttrInfo_AddLoader(attrInfo, 0, GPU_FLOAT, 3); // v0=position
	AttrInfo_AddLoader(attrInfo, 1, GPU_FLOAT, 2); // v1=texcoord
	AttrInfo_AddLoader(attrInfo, 2, GPU_FLOAT, 3); // v2=normal
	
	// Compute the projection matrix
	Mtx_PerspTilt(&projection, 80.0f*M_PI/180.0f, float(w)/float(h), 0.01f, 1000.0f);
	
	return 0;
}

static int lua_useMaterial(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 5) return luaL_error(L, "wrong number of arguments");
	#endif
	model* mdl = (model*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (mdl->magic != 0xC00FFEEE) return luaL_error(L, "wrong number of arguments");
	#endif
	color* ambient = (color*)luaL_checkinteger(L, 2);
	color* diffuse = (color*)luaL_checkinteger(L, 3);
	color* specular = (color*)luaL_checkinteger(L, 4);
	float emission = luaL_checknumber(L, 5);
	#ifndef SKIP_ERROR_HANDLING
	if (ambient->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (diffuse->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (specular->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	// Set object material attributes
	*mdl->material = {
		{
		{ { 0.0f, ambient->r, ambient->g, ambient->b } }, // Ambient
		{ { 0.0f, diffuse->r, diffuse->g, diffuse->b } }, // Diffuse
		{ { 0.0f, specular->r, specular->g, specular->b } }, // Specular
		{ { emission, 0.0f, 0.0f, 0.0f } }, // Emission
		}
	};
	
	return 0;
}

static int lua_useTexture(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2) return luaL_error(L, "wrong number of arguments");
	#endif
	model* mdl = (model*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (mdl->magic != 0xC00FFEEE) return luaL_error(L, "wrong number of arguments");
	#endif
	char* text = (char*)luaL_checkstring(L, 2);
	
	// Freeing old texture
	C3D_TexDelete(mdl->texture);

	// Opening new texture
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
	
	// Flipping texture
	u8* flipped = (u8*)malloc(bitmap->width*bitmap->height*(bitmap->bitperpixel >> 3));
	flipped = flipBitmap(flipped, bitmap);
	free(bitmap->pixels);
	bitmap->pixels = flipped;
	
	// Converting 24bpp texture to a 32bpp ones
	int length = (bitmap->width * bitmap->height)<<2;
	if (bitmap->bitperpixel == 24){
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
	
	// Converting texture to a tiled ones
	u8 *tmp2 = (u8*)linearAlloc(length);
	int i, j;
	for (j = 0; j < bitmap->height; j++) {
		for (i = 0; i < bitmap->width; i++) {

			u32 coarse_y = j & ~7;
			u32 dst_offset = get_morton_offset(i, j, 4) + ((coarse_y * bitmap->width) << 2);

			u32 v = ((u32 *)bitmap->pixels)[i + (bitmap->height - 1 - j)*bitmap->width];
			*(u32 *)(tmp2 + dst_offset) = __builtin_bswap32(v); /* RGBA8 -> ABGR8 */
		}
	}
	memcpy(bitmap->pixels, tmp2, length);
	
	// Setting new texture
	C3D_TexInit(mdl->texture , bitmap->width, bitmap->height, GPU_RGBA8);
	C3D_TexUpload(mdl->texture, bitmap->pixels);
	C3D_TexSetFilter(mdl->texture, GPU_LINEAR, GPU_NEAREST);
	free(bitmap->pixels);
	free(bitmap);
	linearFree(tmp2);
	
	return 0;
}

static int lua_loadModel(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	luaL_checktype(L, 1, LUA_TTABLE);
	int len = lua_rawlen(L, 1);
	char* text = (char*)luaL_checkstring(L, 2);
	color* ambient = (color*)luaL_checkinteger(L, 3);
	color* diffuse = (color*)luaL_checkinteger(L, 4);
	color* specular = (color*)luaL_checkinteger(L, 5);
	float emission = luaL_checknumber(L, 6);
	#ifndef SKIP_ERROR_HANDLING
	if (ambient->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (diffuse->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	if (specular->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
	// Opening texture file
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
	
	// Flipping texture
	u8* flipped = (u8*)malloc(bitmap->width*bitmap->height*(bitmap->bitperpixel >> 3));
	flipped = flipBitmap(flipped, bitmap);
	free(bitmap->pixels);
	bitmap->pixels = flipped;
	
	// Converting 24bpp texture to a 32bpp ones
	int length = (bitmap->width * bitmap->height) << 2;
	if (bitmap->bitperpixel == 24){
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
	
	// Converting texture to a tiled ones
	u8 *tmp2 = (u8*)linearAlloc(length);
	int i, j;
	for (j = 0; j < bitmap->height; j++) {
		for (i = 0; i < bitmap->width; i++) {

			u32 coarse_y = j & ~7;
			u32 dst_offset = get_morton_offset(i, j, 4) + (coarse_y * bitmap->width) << 2;

			u32 v = ((u32 *)bitmap->pixels)[i + (bitmap->height - 1 - j)*bitmap->width];
			*(u32 *)(tmp2 + dst_offset) = __builtin_bswap32(v); /* RGBA8 -> ABGR8 */
		}
	}
	memcpy(bitmap->pixels, tmp2, length);
	
	// Create the VBO (vertex buffer object)
	u32 vertex_size = len*8*sizeof(float);
	u8* vbo_data = (u8*)linearAlloc(vertex_size);
	for(int i = 0; i < len; i++) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, -(argc+1));
		vertex* vert = (vertex*)lua_tointeger(L, -1);
		memcpy(&vbo_data[i*sizeof(vertex)], vert, sizeof(vertex));
		lua_pop(L, 1);
	}
	
	// Setting texture
	C3D_Tex* texture = (C3D_Tex*)linearAlloc(sizeof(C3D_Tex));
	C3D_TexInit(texture , bitmap->width, bitmap->height, GPU_RGBA8);
	C3D_TexUpload(texture, bitmap->pixels);
	C3D_TexSetFilter(texture, GPU_LINEAR, GPU_NEAREST);
	free(bitmap->pixels);
	free(bitmap);
	linearFree(tmp2);
	
	// Set object material attributes
	C3D_Mtx* material = (C3D_Mtx*)linearAlloc(sizeof(C3D_Mtx));
	*material = {
		{
		{ { 0.0f, ambient->r, ambient->g, ambient->b } }, // Ambient
		{ { 0.0f, diffuse->r, diffuse->g, diffuse->b } }, // Diffuse
		{ { 0.0f, specular->r, specular->g, specular->b } }, // Specular
		{ { emission, 0.0f, 0.0f, 0.0f } }, // Emission
		}
	};
	
	// Create a model object and push it into Lua stack
	model* res = (model*)malloc(sizeof(model));
	res->vertex_count = len;
	res->vbo_data = vbo_data;
	res->magic = 0xC00FFEEE;
	res->texture = texture;
	res->material = material;
	lua_pushinteger(L, (u32)res);
	
	return 1;
}

static int lua_initblend(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 2 && argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 screen = luaL_checkinteger(L, 1);
	u32 side = 0;
	if (argc == 2) side = luaL_checkinteger(L, 2);
	u8 target_idx = 0;
	if (screen == 1) target_idx = 2;
	else if (screen == 0 && side == 1) target_idx = 1;
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C3D_FrameDrawOn(targets[target_idx]);
	
	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec,	 light_x, light_y, light_z, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightHalfVec, light_x, light_y, light_z, 0.0f);
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightClr, light_r, light_g, light_b, light_a);
	
	return 0;
}

static int lua_lightdir(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 3) return luaL_error(L, "wrong number of arguments");
	#endif
	float x = luaL_checknumber(L, 1);
	float y = luaL_checknumber(L, 2);
	float z = luaL_checknumber(L, 3);
	light_x = x;
	light_y = y;
	light_z = z;
	return 0;
}

static int lua_termblend(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif
	C3D_FrameEnd(0);
	return 0;
}

static int lua_unloadModel(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	model* object = (model*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (object->magic != 0xC00FFEEE) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	linearFree(object->vbo_data);
	C3D_TexDelete(object->texture);
	linearFree(object->texture);
	linearFree(object->material);
	free(object);
	return 0;
}

static int lua_blend(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	model* object = (model*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (object->magic != 0xC00FFEEE) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	float x = luaL_checknumber(L, 2);
	float y = luaL_checknumber(L, 3);
	float z = luaL_checknumber(L, 4);
	float angleX = luaL_checknumber(L, 5);
	float angleY = luaL_checknumber(L, 6);
	
	// Configure buffers
	C3D_BufInfo* bufInfo = C3D_GetBufInfo();
	BufInfo_Init(bufInfo);
	BufInfo_Add(bufInfo, object->vbo_data, sizeof(vertex), 3, 0x210);
	
	// Bind texture
	C3D_TexBind(0, object->texture);
	
	// Configure the first fragment shading substage
	C3D_TexEnv env;
	C3D_TexEnvSrc(&env, C3D_Both, GPU_TEXTURE0, GPU_PRIMARY_COLOR, 0);
	C3D_TexEnvOp(&env, C3D_Both, 0, 0, 0);
	C3D_TexEnvFunc(&env, C3D_Both, GPU_MODULATE);
	C3D_SetTexEnv(0, &env);
	
	// Calculate the modelView matrix
	C3D_Mtx modelView;
	Mtx_Identity(&modelView);
	Mtx_Translate(&modelView, x, y, z);
	Mtx_RotateX(&modelView, angleX, true);
	Mtx_RotateY(&modelView, angleY, true);

	// Update the uniforms
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView,  &modelView);
	C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_material,   object->material);
	
	// Draw the VBO
	C3D_DrawArrays(GPU_TRIANGLES, 0, object->vertex_count);
	
	return 0;
}

static int lua_term(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 0) return luaL_error(L, "wrong number of arguments");
	#endif

	// Free the shader program
	shaderProgramFree(&program);
	DVLB_Free(vshader_dvlb);
	
	// Deinitialize graphics
	C3D_Fini();
	
	return 0;
}

static int lua_newcolor(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 4) return luaL_error(L, "wrong number of arguments");
	#endif
	float r = luaL_checknumber(L, 1);
	float g = luaL_checknumber(L, 2);
	float b = luaL_checknumber(L, 3);
	float a = luaL_checknumber(L, 4);
	
	color* res = (color*)malloc(sizeof(color));
	res->r = r;
	res->g = g;
	res->b = b;
	res->a = a;
	res->magic = 0xC0C0C0C0;
	lua_pushinteger(L,(u32)res);
	return 1;
}

static int lua_setlightc(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	color* colour = (color*)luaL_checkinteger(L, 1);
	#ifndef SKIP_ERROR_HANDLING
	if (colour->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	light_r = colour->r;
	light_g = colour->g;
	light_b = colour->b;
	light_a = colour->a;
	return 0;
}

static int lua_convert(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
	if (argc != 1) return luaL_error(L, "wrong number of arguments");
	#endif
	u32 colour = luaL_checkinteger(L, 1);
	color* res = (color*)malloc(sizeof(color));
	int2float(colour, &res->r, &res->g, &res->b, &res->a);
	res->magic = 0xC0C0C0C0;
	lua_pushinteger(L,(u32)res);
	return 1;
}
	
//Register our Render Functions
static const luaL_Reg Render_functions[] = {
	{"createVertex",    lua_newVertex},
	{"loadModel",       lua_loadModel},
	{"loadObject",      lua_loadobj},
	{"unloadModel",     lua_unloadModel},
	{"init",            lua_init},
	{"drawModel",       lua_blend},
	{"initBlend",       lua_initblend},
	{"useTexture",      lua_useTexture},
	{"useMaterial",     lua_useMaterial},
	{"termBlend",       lua_termblend},
	{"setLightColor",   lua_setlightc},
	{"setLightSource",  lua_lightdir},
	{"createColor",     lua_newcolor},
	{"convertColorFrom",lua_convert},
	{"term",            lua_term},
	{0, 0}
};
	
void luaRender_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Render_functions, 0);
	lua_setglobal(L, "Render");
}