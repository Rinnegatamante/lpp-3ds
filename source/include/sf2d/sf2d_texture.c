#include <stdlib.h>
#include <string.h>
#include "sf2d.h"
#include "sf2d_private.h"

sf2d_texture *sf2d_create_texture(int width, int height, GPU_TEXCOLOR pixel_format, sf2d_place place)
{
	int pow2_w = next_pow2(width);
	int pow2_h = next_pow2(height);
	int data_size;

	switch (pixel_format) {
	case GPU_RGBA8:
	default:
		data_size = pow2_w * pow2_h * 4;
		break;
	case GPU_RGB8:
		data_size = pow2_w * pow2_h * 3;
		break;
	case GPU_RGBA5551:
	case GPU_RGB565:
		data_size = pow2_w * pow2_h * 2;
		break;
	}

	sf2d_texture *texture;

	if (place == SF2D_PLACE_RAM) {
		// If there's not enough linear heap space, return
		if (linearSpaceFree() < data_size) {
			return NULL;
		}
		texture = malloc(sizeof(*texture));
		texture->data = linearMemAlign(data_size, 0x80);

	} else if (place == SF2D_PLACE_VRAM) {
		// If there's not enough VRAM heap space, return
		if (vramSpaceFree() < data_size) {
			return NULL;
		}
		texture = malloc(sizeof(*texture));
		texture->data = vramMemAlign(data_size, 0x80);

	} else {
		//wot?
		return NULL;
	}

	texture->place = place;
	texture->pixel_format = pixel_format;
	texture->width = width;
	texture->height = height;
	texture->pow2_w = pow2_w;
	texture->pow2_h = pow2_h;
	texture->data_size = data_size;

	return texture;
}

void sf2d_free_texture(sf2d_texture *texture)
{
	if (texture) {
		if (texture->place == SF2D_PLACE_RAM) {
			linearFree(texture->data);
		} else if (texture->place == SF2D_PLACE_VRAM) {
			vramFree(texture->data);
		}
		free(texture);
	}
}

void sf2d_fill_texture_from_RGBA8(sf2d_texture *dst, const void *rgba8, int source_w, int source_h)
{
	// TODO: add support for non-RGBA8 textures

	u8 *tmp = linearAlloc(dst->pow2_w * dst->pow2_h * 4);
	int i, j;
	for (i = 0; i < source_h; i++) {
		for (j = 0; j < source_w; j++) {
			((u32 *)tmp)[i*dst->pow2_w + j] = ((u32 *)rgba8)[i*source_w + j];
		}
	}
	memcpy(dst->data, tmp, dst->pow2_w*dst->pow2_h*4);
	linearFree(tmp);
}

void sf2d_bind_texture(const sf2d_texture *texture, GPU_TEXUNIT unit)
{
	GPU_SetTextureEnable(unit);

	GPU_SetTexEnv(
		0,
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
		GPU_TEVSOURCES(GPU_TEXTURE0, GPU_TEXTURE0, GPU_TEXTURE0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_TEVOPERANDS(0, 0, 0),
		GPU_MODULATE, GPU_MODULATE,
		0xFFFFFFFF
	);

	GPU_SetTexture(
		unit,
		(u32 *)osConvertVirtToPhys((u32)texture->data),
		// width and height swapped?
		texture->pow2_h,
		texture->pow2_w,
		GPU_TEXTURE_MAG_FILTER(GPU_NEAREST) | GPU_TEXTURE_MIN_FILTER(GPU_NEAREST),
		texture->pixel_format
	);
}

void sf2d_draw_texture(const sf2d_texture *texture, int x, int y)
{
	sf2d_vertex_pos_tex *vertices = sf2d_pool_malloc(4 * sizeof(sf2d_vertex_pos_tex));

	int w = texture->width;
	int h = texture->height;

	vertices[0].position = (sf2d_vector_3f){(float)x,   (float)y,   0.5f};
	vertices[1].position = (sf2d_vector_3f){(float)x+w, (float)y,   0.5f};
	vertices[2].position = (sf2d_vector_3f){(float)x,   (float)y+h, 0.5f};
	vertices[3].position = (sf2d_vector_3f){(float)x+w, (float)y+h, 0.5f};

	float u = texture->width/(float)texture->pow2_w;
	float v = texture->height/(float)texture->pow2_h;

	vertices[0].texcoord = (sf2d_vector_2f){0.0f, 0.0f};
	vertices[1].texcoord = (sf2d_vector_2f){u,    0.0f};
	vertices[2].texcoord = (sf2d_vector_2f){0.0f, v};
	vertices[3].texcoord = (sf2d_vector_2f){u,    v};

	sf2d_bind_texture(texture, GPU_TEXUNIT0);

	GPU_SetAttributeBuffers(
		2, // number of attributes
		(u32*)osConvertVirtToPhys((u32)vertices),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
		0xFFFC, //0b1100
		0x10,
		1, //number of buffers
		(u32[]){0x0}, // buffer offsets (placeholders)
		(u64[]){0x10}, // attribute permutations for each buffer
		(u8[]){2} // number of attributes for each buffer
	);

	GPU_DrawArray(GPU_TRIANGLE_STRIP, 4);
}

void sf2d_draw_texture_rotate(const sf2d_texture *texture, int x, int y, float rad)
{
	sf2d_vertex_pos_tex *vertices = sf2d_pool_malloc(4 * sizeof(sf2d_vertex_pos_tex));

	int w2 = texture->width/2.0f;
	int h2 = texture->height/2.0f;

	vertices[0].position = (sf2d_vector_3f){(float)-w2, (float)-h2, 0.5f};
	vertices[1].position = (sf2d_vector_3f){(float) w2, (float)-h2, 0.5f};
	vertices[2].position = (sf2d_vector_3f){(float)-w2, (float) h2, 0.5f};
	vertices[3].position = (sf2d_vector_3f){(float) w2, (float) h2, 0.5f};

	float u = texture->width/(float)texture->pow2_w;
	float v = texture->height/(float)texture->pow2_h;

	vertices[0].texcoord = (sf2d_vector_2f){0.0f, 0.0f};
	vertices[1].texcoord = (sf2d_vector_2f){u,    0.0f};
	vertices[2].texcoord = (sf2d_vector_2f){0.0f, v};
	vertices[3].texcoord = (sf2d_vector_2f){u,    v};

	float m[4*4];
	matrix_set_z_rotation(m, rad);
	sf2d_vector_3f rot[4];

	int i;
	for (i = 0; i < 4; i++) {
		vector_mult_matrix4x4(m, &vertices[i].position, &rot[i]);
	}
	for (i = 0; i < 4; i++) {
		vertices[i].position = (sf2d_vector_3f){rot[i].x + x + w2, rot[i].y + y + h2, rot[i].z};
	}

	sf2d_bind_texture(texture, GPU_TEXUNIT0);

	GPU_SetAttributeBuffers(
		2, // number of attributes
		(u32*)osConvertVirtToPhys((u32)vertices),
		GPU_ATTRIBFMT(0, 3, GPU_FLOAT) | GPU_ATTRIBFMT(1, 2, GPU_FLOAT),
		0xFFFC, //0b1100
		0x10,
		1, //number of buffers
		(u32[]){0x0}, // buffer offsets (placeholders)
		(u64[]){0x10}, // attribute permutations for each buffer
		(u8[]){2} // number of attributes for each buffer
	);

	GPU_DrawArray(GPU_TRIANGLE_STRIP, 4);
}

static const u8 tile_order[] = {
	0,1,8,9,2,3,10,11,16,17,24,25,18,19,26,27,4,5,
	12,13,6,7,14,15,20,21,28,29,22,23,30,31,32,33,
	40,41,34,35,42,43,48,49,56,57,50,51,58,59,36,
	37,44,45,38,39,46,47,52,53,60,61,54,55,62,63
};

//Stolen from smealum's portal3DS
void sf2d_texture_tile32(sf2d_texture *texture)
{
	// TODO: add support for non-RGBA8 textures

	u8 *tmp = linearAlloc(texture->pow2_w * texture->pow2_h * 4);

	int i, j, k, l = 0;
	for (j = 0; j < texture->pow2_h; j+=8) {
		for (i = 0; i < texture->pow2_w; i+=8) {
			for (k = 0; k < 8*8; k++) {
				int x = i + tile_order[k]%8;
				int y = j + (tile_order[k] - (x-i))/8;
				u32 v = ((u32 *)texture->data)[x + (texture->pow2_h-1-y)*texture->pow2_w];
				((u32 *)tmp)[l++] = __builtin_bswap32(v);
			}
		}
	}
	memcpy(texture->data, tmp, texture->pow2_w*texture->pow2_h*4);
	linearFree(tmp);
}
