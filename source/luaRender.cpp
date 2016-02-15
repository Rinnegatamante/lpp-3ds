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
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#- fincs for citro3D ---------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------*/
#include <3ds.h>
#include <citro3d.h>
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
	
typedef struct{
	float x;
	float y;
	float z;
	float t1;
	float t2;
	float n1;
	float n2;
	float n3;
} vertex;

typedef struct{
	u32 magic;
	u8* vbo_data;
	u32 vertex_count;
	C3D_Tex* texture;
	C3D_Mtx* material;
} model;

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
	uLoc_modelView    = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");
	uLoc_lightVec     = shaderInstanceGetUniformLocation(program.vertexShader, "lightVec");
	uLoc_lightHalfVec = shaderInstanceGetUniformLocation(program.vertexShader, "lightHalfVec");
	uLoc_lightClr     = shaderInstanceGetUniformLocation(program.vertexShader, "lightClr");
	uLoc_material     = shaderInstanceGetUniformLocation(program.vertexShader, "material");
	
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

static int lua_loadModel(lua_State *L){
	int argc = lua_gettop(L);
    #ifndef SKIP_ERROR_HANDLING
		if (argc != 6) return luaL_error(L, "wrong number of arguments");
	#endif
	luaL_checktype(L, 1, LUA_TTABLE);
	int len = lua_rawlen(L, 1);
	gpu_text* tex = (gpu_text*)luaL_checkinteger(L, 2);
	color* ambient = (color*)luaL_checkinteger(L, 3);
	color* diffuse = (color*)luaL_checkinteger(L, 4);
	color* specular = (color*)luaL_checkinteger(L, 5);
	float emission = luaL_checknumber(L, 6);
	#ifndef SKIP_ERROR_HANDLING
		if (tex->magic != 0x4C545854) return luaL_error(L, "attempt to access wrong memory block type");
		if (ambient->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
		if (diffuse->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
		if (specular->magic != 0xC0C0C0C0) return luaL_error(L, "attempt to access wrong memory block type");
	#endif
	
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
	
	// Load the texture
	C3D_Tex* texture = (C3D_Tex*)linearAlloc(sizeof(C3D_Tex));
	C3D_TexInit(texture , tex->tex->pow2_w, tex->tex->pow2_h, GPU_RGBA8);
	C3D_TexUpload(texture, tex->tex->data);
	C3D_TexSetFilter(texture, GPU_LINEAR, GPU_NEAREST);
	
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
	C3D_FVUnifSet(GPU_VERTEX_SHADER, uLoc_lightVec,     light_x, light_y, light_z, 0.0f);
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
	{"createVertex", 	lua_newVertex},
	{"loadModel", 		lua_loadModel},
	{"unloadModel", 	lua_unloadModel},
	{"init", 			lua_init},
	{"drawModel", 		lua_blend},
	{"initBlend", 		lua_initblend},
	{"termBlend", 		lua_termblend},
	{"setLightColor", 	lua_setlightc},
	{"setLightSource", 	lua_lightdir},
	{"createColor", 	lua_newcolor},
	{"convertColorFrom",lua_convert},
	{"term", 			lua_term},
	{0, 0}
};
	
void luaRender_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Render_functions, 0);
	lua_setglobal(L, "Render");
}