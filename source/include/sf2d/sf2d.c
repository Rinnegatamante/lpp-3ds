#include "sf2d.h"
#include "sf2d_private.h"
#include "shader_vsh_shbin.h"

#define GPU_CMD_SIZE  0x40000
#define POOL_SIZE     0x10000

static int sf2d_initialized = 0;
static u32 clear_color = RGBA8(0x00, 0x00, 0x00, 0xFF);
static u32 *gpu_cmd = NULL;
// Temporary memory pool
static void *pool_addr = NULL;
static u32 pool_index = 0;
//GPU framebuffer address
static u32 *gpu_fb_addr = NULL;
//GPU depth buffer address
static u32 *gpu_depth_fb_addr = NULL;
//Current screen/side
static gfxScreen_t cur_screen = GFX_TOP;
static gfx3dSide_t cur_side = GFX_LEFT;
//Shader stuff
static DVLB_s *dvlb = NULL;
static shaderProgram_s shader;
static u32 projection_desc = -1;
static u32 modelview_desc = -1;
//Matrix
static float ortho_matrix_top[4*4];
static float ortho_matrix_bot[4*4];

int sf2d_init()
{
	if (sf2d_initialized) return 0;
	
	gpu_fb_addr       = vramMemAlign(400*240*8, 0x100);
	gpu_depth_fb_addr = vramMemAlign(400*240*8, 0x100);
	gpu_cmd           = linearAlloc(GPU_CMD_SIZE * 4);
	pool_addr         = linearAlloc(POOL_SIZE);
	
	//gfxInitDefault();
	GPU_Init(NULL);
	//gfxSet3D(true);
	GPU_Reset(NULL, gpu_cmd, GPU_CMD_SIZE);
	GPUCMD_SetBufferOffset(0);
	
	//Setup the shader
	dvlb = DVLB_ParseFile((u32 *)shader_vsh_shbin, shader_vsh_shbin_size);
	shaderProgramInit(&shader);
	shaderProgramSetVsh(&shader, &dvlb->DVLE[0]);
	
	//Get shader uniform descriptors
	projection_desc = shaderInstanceGetUniformLocation(shader.vertexShader, "projection");
	modelview_desc = shaderInstanceGetUniformLocation(shader.vertexShader, "modelview");
	
	shaderProgramUse(&shader);
	
	matrix_init_orthographic(ortho_matrix_top, 0.0f, 400.0f, 0.0f, 240.0f, 0.0f, 1.0f);
	matrix_init_orthographic(ortho_matrix_bot, 0.0f, 320.0f, 0.0f, 240.0f, 0.0f, 1.0f);
	matrix_gpu_set_uniform(ortho_matrix_top, projection_desc);

	GPUCMD_Finalize();
	GPUCMD_FlushAndRun(NULL);
	gspWaitForP3D();

	sf2d_pool_reset();
	
	sf2d_initialized = 1;
	
	return 1;
}

int sf2d_fini()
{
	if (!sf2d_initialized) return 0;
	
	//gfxExit();
	shaderProgramFree(&shader);
	DVLB_Free(dvlb);

	linearFree(pool_addr);
	linearFree(gpu_cmd);
	vramFree(gpu_fb_addr);
	vramFree(gpu_depth_fb_addr);
	
	sf2d_initialized = 0;
	
	return 1;
}

void sf2d_start_frame(gfxScreen_t screen, gfx3dSide_t side)
{
	sf2d_pool_reset();
	GPUCMD_SetBufferOffset(0);

	// Only upload the uniform if the screen changes
	if (screen != cur_screen) {
		if (screen == GFX_TOP) {
			matrix_gpu_set_uniform(ortho_matrix_top, projection_desc);
		} else {
			matrix_gpu_set_uniform(ortho_matrix_bot, projection_desc);
		}
	}

	cur_screen = screen;
	cur_side = side;

	int screen_w;
	if (screen == GFX_TOP) {
		screen_w = 400;
	} else {
		screen_w = 320;
	}
	GPU_SetViewport((u32 *)osConvertVirtToPhys((u32)gpu_depth_fb_addr), 
		(u32 *)osConvertVirtToPhys((u32)gpu_fb_addr),
		0, 0, 240, screen_w);

	GPU_DepthMap(-1.0f, 0.0f);
	GPU_SetFaceCulling(GPU_CULL_NONE);
	GPU_SetStencilTest(false, GPU_ALWAYS, 0x00, 0xFF, 0x00);
	GPU_SetStencilOp(GPU_KEEP, GPU_KEEP, GPU_KEEP);
	GPU_SetBlendingColor(0,0,0,0);
	GPU_SetDepthTestAndWriteMask(true, GPU_ALWAYS, GPU_WRITE_ALL);
	GPUCMD_AddMaskedWrite(GPUREG_0062, 0x1, 0);
	GPUCMD_AddWrite(GPUREG_0118, 0);

	GPU_SetAlphaBlending(
		GPU_BLEND_ADD,
		GPU_BLEND_ADD,
		GPU_SRC_ALPHA, GPU_ONE_MINUS_SRC_ALPHA,
		GPU_ONE, GPU_ZERO
	);

	GPU_SetAlphaTest(false, GPU_ALWAYS, 0x00);
	
	GPU_SetDummyTexEnv(1);
	GPU_SetDummyTexEnv(2);
	GPU_SetDummyTexEnv(3);
	GPU_SetDummyTexEnv(4);
	GPU_SetDummyTexEnv(5);
}

void sf2d_end_frame()
{
	GPU_FinishDrawing();
	GPUCMD_Finalize();
	GPUCMD_FlushAndRun(NULL);
	gspWaitForP3D();

	//Draw the screen
	if (cur_screen == GFX_TOP) {
		GX_SetDisplayTransfer(NULL, gpu_fb_addr, 0x019000F0, (u32*)gfxGetFramebuffer(GFX_TOP, cur_side, NULL, NULL), 0x019000F0, 0x1000);
	} else {
		GX_SetDisplayTransfer(NULL, gpu_fb_addr, 0x014000F0, (u32*)gfxGetFramebuffer(GFX_BOTTOM, GFX_LEFT, NULL, NULL), 0x014000F0, 0x1000);
	}
	gspWaitForPPF();

	//Clear the screen
	GX_SetMemoryFill(NULL, gpu_fb_addr, clear_color, &gpu_fb_addr[0x2EE00],
		0x201, gpu_depth_fb_addr, 0x00000000, &gpu_depth_fb_addr[0x2EE00], 0x201);
	gspWaitForPSC0();
}

void sf2d_swapbuffers()
{
	gspWaitForPPF();
	gspWaitForEvent(GSPEVENT_VBlank0, true);
	gfxSwapBuffersGpu();
}

void *sf2d_pool_malloc(u32 size)
{
	if ((pool_index + size) < POOL_SIZE) {
		void *addr = (void *)((u32)pool_addr + pool_index);
		pool_index += size;
		return addr;
	}
	return NULL;
}

void *sf2d_pool_memalign(u32 size, u32 alignment)
{
	u32 new_index = (pool_index + alignment - 1) & ~(alignment - 1);
	if ((new_index + size) < POOL_SIZE) {
		void *addr = (void *)((u32)pool_addr + new_index);
		pool_index = new_index + size;
		return addr;
	}
	return NULL;
}

void sf2d_pool_reset()
{
	pool_index = 0;
}

void sf2d_set_clear_color(u32 color)
{
	clear_color = color;
}
