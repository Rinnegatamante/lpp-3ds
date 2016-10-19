// Font.cpp
// @author: Nanni

#include <3ds.h>
#include "Font.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "../utils.h"

Font::Font()
{
	m_scale = 1.0f;
}

Font::Font(const unsigned char* buffer, unsigned int buffer_size)
{
	loadFromMemory(buffer, buffer_size);
}

Font::Font(const std::vector<unsigned char>& buffer)
{
	loadFromMemory(buffer);
}

void* Font::loadFromFile(const std::string& filename)
{
	FILE* fp = fopen(filename.c_str(), "r");
	if (!fp)
		return NULL;
	fseek(fp, 0, SEEK_END);
	unsigned int buffer_size = ftell(fp);
	unsigned char* buffer = (unsigned char*)malloc(buffer_size);
	if (!buffer) {
		fclose(fp);
		return NULL;
	}
	rewind(fp);
	fread(buffer, 1, buffer_size, fp);
	fclose(fp);
	
	bool ret = loadFromMemory(buffer, buffer_size);

	ttf* result = (ttf*)malloc(sizeof(ttf));
	result->buffer = buffer;
	result->magic = 0x4C464E54;
	result->bufsize = buffer_size;
	//free(buffer);

	return result;
}

bool Font::loadFromMemory(const unsigned char* buffer, unsigned int buffer_size)
{
	if (!buffer || buffer_size == 0)
		return false;
	if (stbtt_InitFont(&m_info, buffer, 0) != 1)
		return false;
	m_scale = stbtt_ScaleForPixelHeight(&m_info, 16);
	return true;
}

bool Font::loadFromMemory(const std::vector<unsigned char>& buffer)
{
	if (buffer.size() == 0)
		return false;
	return loadFromMemory(&buffer[0], buffer.size());
}

void Font::setScale(float scale)
{
	m_scale = scale;
}

void Font::setSize(int pixels)
{
	m_scale = stbtt_ScaleForPixelHeight(&m_info, pixels);
}

void Font::drawStringUnicode(int x, int y, const std::wstring& str, Color color, bool top_screen, bool side, int max_width)
{
	if(!isLoaded())
	{
		return;
	}
	
	Image char_render;
	std::vector<unsigned char> char_raster;
	int bx, by, bw, bh;
	int ascent, descent, lineGap;
	int sx = 0, sy = 0;

	stbtt_GetFontVMetrics(&m_info, &ascent, &descent, &lineGap);

	ascent *= m_scale;
	descent *= m_scale;
	lineGap *= m_scale;

	for (unsigned int i = 0; i < str.length(); i++)
	{
		if(str[i] == L'\n') {
			sy += ascent - descent + lineGap;
			sx = 0;
			continue;
		}
		
		stbtt_GetCodepointBitmapBox(&m_info, str[i], m_scale, m_scale,
			&bx, &by, &bw, &bh);

		int width = bw - bx;
		int height = bh - by;

		if(max_width > 0 && sx + width > max_width) {
			sy += ascent - descent + lineGap;
			sx = 0;
		}

		int oy = ascent + by;

		char_raster.resize(width * height);

		stbtt_MakeCodepointBitmap(&m_info, &char_raster[0], width, height,
			width, m_scale, m_scale, str[i]);

		char_render.create(width, height, Color(0, 0, 0, 0));

		for (int ix = 0; ix < width; ix++) {
			for (int iy = 0; iy < height; iy++) {
				if (char_raster[ix + iy * width] != 0)
					char_render.putPixel(ix, iy, Color(color.r, color.g, color.b,
					char_raster[ix + iy * width]));
			}
		}

		char_render.draw(sx + x, sy + y + oy, top_screen, side);

		int advance;
		stbtt_GetCodepointHMetrics(&m_info, str[i], &advance, 0);

		sx += advance * m_scale;

		int kerning = stbtt_GetCodepointKernAdvance(&m_info, str[i], str[i + 1]);

		sx += kerning * m_scale;
	}
}

void Font::drawString(int x, int y, const std::string& str, Color color, bool top_screen, bool side, int max_width)
{
	if(str.length() > 0)
	{
		std::wstring unicode_str = std::wstring(str.begin(), str.end());
		drawStringUnicode(x, y, unicode_str, color, top_screen, side, max_width);
	}
}

void Font::measureText(const std::wstring str, int& width, int& height, int max_width) {
	if(!isLoaded())
	{
		return NULL;
	}

	Image char_render;
	std::vector<unsigned char> char_raster;
	int bx, by, bw, bh;
	int ascent, descent, lineGap;
	int sx = 0, sy = 0;

	stbtt_GetFontVMetrics(&m_info, &ascent, &descent, &lineGap);

	ascent *= m_scale;
	descent *= m_scale;
	lineGap *= m_scale;

	width = 0;
	height = 0;

	for(int i = 0; i < str.length(); i++) {
		if(str[i] == L'\n') {
			sy += ascent - descent + lineGap;
			sx = 0;
			continue;
		}

		stbtt_GetCodepointBitmapBox(&m_info, str[i], m_scale, m_scale,
				&bx, &by, &bw, &bh);

		int cwidth = bw - bx;
		int cheight = bh - by;
		int oy = ascent + by;

		if(max_width > 0 && sx + cwidth > max_width) {
			sy += ascent - descent + lineGap;
			sx = 0;
		}

		int advance;
		stbtt_GetCodepointHMetrics(&m_info, str[i], &advance, 0);
		if(sy + oy + cheight > height) {
			height = sy + oy + cheight;
		}
		if(sx + cwidth > width) {
			width = sx + cwidth;
		}

		sx += advance * m_scale;

		int kerning = stbtt_GetCodepointKernAdvance(&m_info, str[i], str[i + 1]);

		sx += kerning * m_scale;
	}
}

void Font::drawStringToBuffer(int x, int y, const std::wstring& str, Color color, unsigned char* buffer, int buffer_width, int buffer_height, int bitsperpixel, int max_width)
{
	std::vector<unsigned char> char_raster;
	int bx, by, bw, bh;
	int ascent, descent, lineGap;
	int sx = 0, sy = 0;
	int width, height;

	stbtt_GetFontVMetrics(&m_info, &ascent, &descent, &lineGap);

	ascent *= m_scale;
	descent *= m_scale;
	lineGap *= m_scale;

	this->measureText(str, width, height, max_width);

	for (unsigned int i = 0; i < str.length(); i++)
	{
		if(str[i] == L'\n') {
			sy += ascent - descent + lineGap;
			sx = 0;
			continue;
		}

		stbtt_GetCodepointBitmapBox(&m_info, str[i], m_scale, m_scale,
				&bx, &by, &bw, &bh);

		int char_width = bw - bx;
		int char_height = bh - by;
		int oy = ascent + by;

		if(max_width > 0 && sx + char_width > max_width) {
			sy += ascent - descent + lineGap;
			sx = 0;
		}

		char_raster.resize(char_width * char_height);

		stbtt_MakeCodepointBitmap(&m_info, &char_raster[0], char_width, char_height,
				char_width, m_scale, m_scale, str[i]);

		for (int ix = 0; ix < char_width; ix++) {
			for (int iy = 0; iy < char_height; iy++) {
				int xpos = x + sx + ix;
				int ypos = buffer_height - (y + sy + oy + iy) -1;
				if (char_raster[ix + iy * char_width] != 0 && xpos < buffer_width && ypos < buffer_height) {
					unsigned int alpha = char_raster[ix + iy * char_width];
					unsigned int inv_alpha = 255 - alpha;
					if (bitsperpixel == 24) {
						unsigned char bg_r = buffer[3 * (xpos + ypos * buffer_width) + 0];
						unsigned char bg_g = buffer[3 * (xpos + ypos * buffer_width) + 1];
						unsigned char bg_b = buffer[3 * (xpos + ypos * buffer_width) + 2];

						unsigned char r = (unsigned char)((alpha * color.r + inv_alpha * bg_r) >> 8);
						unsigned char g = (unsigned char)((alpha * color.g + inv_alpha * bg_g) >> 8);
						unsigned char b = (unsigned char)((alpha * color.b + inv_alpha * bg_b) >> 8);

						buffer[3 * (xpos + ypos * buffer_width) + 0] = b;
						buffer[3 * (xpos + ypos * buffer_width) + 1] = g;
						buffer[3 * (xpos + ypos * buffer_width) + 2] = r;
					} else {
						unsigned char bg_r = buffer[4 * (xpos + ypos * buffer_width) + 0];
						unsigned char bg_g = buffer[4 * (xpos + ypos * buffer_width) + 1];
						unsigned char bg_b = buffer[4 * (xpos + ypos * buffer_width) + 2];
						unsigned char bg_a = buffer[4 * (xpos + ypos * buffer_width) + 3];

						unsigned char a;
						if(alpha == 255 || bg_a == 255) {
							a = 255;
						} else {
							a = alpha + (inv_alpha * bg_a) / 256;
						}
						unsigned int r,g,b;
						if(a == 0) {
							r = 0;
							g = 0;
							b = 0;
						} else {
							r = (alpha * color.r + (inv_alpha * bg_a * bg_r) / 256) / a;
							b = (alpha * color.b + (inv_alpha * bg_a * bg_b) / 256) / a;
							g = (alpha * color.g + (inv_alpha * bg_a * bg_g) / 256) / a;
						}
						buffer[4 * (xpos + ypos * buffer_width) + 0] = b > 255 ? 255 : b;
						buffer[4 * (xpos + ypos * buffer_width) + 1] = g > 255 ? 255 : g;
						buffer[4 * (xpos + ypos * buffer_width) + 2] = r > 255 ? 255 : r;
						buffer[4 * (xpos + ypos * buffer_width) + 3] = a;
					}
				}
			}
		}
		int advance;
		stbtt_GetCodepointHMetrics(&m_info, str[i], &advance, 0);
		sx += advance * m_scale;

		int kerning = stbtt_GetCodepointKernAdvance(&m_info, str[i], str[i + 1]);
		sx += kerning * m_scale;
	}
}

bool Font::isLoaded()
{
	return m_info.data != 0;
}

std::wstring Font::utf8_to_UCS2(char* code)
{
	int size = strlen(code);
	int *a;
	std::wstring ucs2;
	short int com = 1 << 7;
	for (int x = 0; x <= size; ++x)
	{
		char utf = code[x];
		int size = 0;
		int i = 0;
		int index = (utf & com) != 0;
		short int binary[16];
		if (index == 0)///0xxxxxxx ==> 00000000 0xxxxxxxx
		{
			for (; i < 8; ++i)
			{
				binary[i] = 0;
			}
			for (; i < 16; ++i)
			{
				binary[i] = (utf & 1 << (15 - i)) != 0;//
			}
		}
		else if (utf&(1 << 5) == 0)// 110xxxxx 10yyyyyy ==> 00000xxx xxyyyyyy
		{
			for (; i < 5; ++i)
			{
				binary[i] = 0;
			}
			for (; i < 10; ++i)
			{
				binary[i] = (utf&(1 << (9 - i))) != 0;
			}
			x += 1;
			utf = code[x];
			for (; i < 16; ++i)
			{
				binary[i] = (utf&(1 << (15 - i))) != 0;
			}
		}
		else//1110xxxx 10yyyyyy 10zzzzzz ==> xxxxyyyy yyzzzzzz
		{
			for (; i < 4; ++i)
			{
				binary[i] = (utf & 1 << (3 - i)) != 0;
			}
			x += 1;
			utf = code[x];
			for (; i < 10; ++i)
			{
				binary[i] = (utf & 1 << (9 - i)) != 0;
			}
			x += 1;
			utf = code[x];
			for (; i < 16; ++i)
			{
				binary[i] = (utf & 1 << (15 - i)) != 0;
			}
		}
		wchar_t ch = 0;
		for (i = 0; i <16; i++)
		{
			ch <<= 1;
			ch |= binary[i];
		}
		ucs2 += ch;
	}
	return ucs2;
}
