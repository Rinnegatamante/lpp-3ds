#include "sf2d.h"

void sf2d_draw_line(int x0, int y0, int x1, int y1, u32 color)
{
	sf2d_vertex_pos_col *vertices = sf2d_pool_malloc(4 * sizeof(sf2d_vertex_pos_col));

	vertices[0].position = (sf2d_vector_3f){(float)x0+0.5f, (float)y0+0.5f, 0.5f};
	vertices[1].position = (sf2d_vector_3f){(float)x0-0.5f, (float)y0-0.5f, 0.5f};
	vertices[2].position = (sf2d_vector_3f){(float)x1+0.5f, (float)y1+0.5f, 0.5f};
	vertices[3].position = (sf2d_vector_3f){(float)x1-0.5f, (float)y1-0.5f, 0.5f};

	u8 r = (color>>24) & 0xFF;
	u8 g = (color>>16) & 0xFF;
	u8 b = (color>>8) & 0xFF;
	u8 a = color & 0xFF;

	vertices[0].color = (sf2d_vector_4f){r/255.0f,  g/255.0f, b/255.0f, a/255.0f};
	vertices[1].color = vertices[0].color;
	vertices[2].color = vertices[0].color;
	vertices[3].color = vertices[0].color;

	GPU_SetTexEnv(
		0,
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF
	);

	GPU_SetAttributeBuffers(
		2, // number of attributes
		(u32*)osConvertVirtToPhys((u32)vertices),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_FLOAT),
		0xFFFC, //0b1100
		0x10,
		1, //number of buffers
		(u32[]){0x0}, // buffer offsets (placeholders)
		(u64[]){0x10}, // attribute permutations for each buffer
		(u8[]){2} // number of attributes for each buffer
	);

	GPU_DrawArray(GPU_TRIANGLE_STRIP, 4);
}

void sf2d_draw_rectangle(int x, int y, int w, int h, u32 color)
{
	sf2d_vertex_pos_col *vertices = sf2d_pool_malloc(4 * sizeof(sf2d_vertex_pos_col));

	vertices[0].position = (sf2d_vector_3f){(float)x,   (float)y,   0.5f};
	vertices[1].position = (sf2d_vector_3f){(float)x+w, (float)y,   0.5f};
	vertices[2].position = (sf2d_vector_3f){(float)x,   (float)y+h, 0.5f};
	vertices[3].position = (sf2d_vector_3f){(float)x+w, (float)y+h, 0.5f};

	u8 r = (color>>24) & 0xFF;
	u8 g = (color>>16) & 0xFF;
	u8 b = (color>>8) & 0xFF;
	u8 a = color & 0xFF;

	vertices[0].color = (sf2d_vector_4f){r/255.0f,  g/255.0f, b/255.0f, a/255.0f};
	vertices[1].color = vertices[0].color;
	vertices[2].color = vertices[0].color;
	vertices[3].color = vertices[0].color;

	GPU_SetTexEnv(
		0,
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVSOURCES(GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR, GPU_PRIMARY_COLOR),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF
	);

	GPU_SetAttributeBuffers(
		2, // number of attributes
		(u32*)osConvertVirtToPhys((u32)vertices),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 4, GPU_FLOAT),
		0xFFFC, //0b1100
		0x10,
		1, //number of buffers
		(u32[]){0x0}, // buffer offsets (placeholders)
		(u64[]){0x10}, // attribute permutations for each buffer
		(u8[]){2} // number of attributes for each buffer
	);

	GPU_DrawArray(GPU_TRIANGLE_STRIP, 4);
}

