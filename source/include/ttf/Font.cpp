// Font.cpp
// @author: Nanni

#include <3ds.h>
#include "Font.hpp"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

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

unsigned char* Font::loadFromFile(const std::string& filename)
{
	FILE* fp = fopen(filename.c_str(), "rb");
	if (!fp)
		return false;
	fseek(fp, 0, SEEK_END);
	unsigned int buffer_size = ftell(fp);
	unsigned char* buffer = (unsigned char*)malloc(buffer_size);
	if (!buffer) {
		fclose(fp);
		return false;
	}
	rewind(fp);
	fread(buffer, 1, buffer_size, fp);
	fclose(fp);

	bool ret = loadFromMemory(buffer, buffer_size);

	//free(buffer);

	return buffer;
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

void Font::drawStringUnicode(int x, int y, const std::wstring& str, Color color, bool top_screen, bool side)
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

	for (unsigned int i = 0; i < str.length(); i++)
	{
		if(str[i + 1] == L'\n') {
			sy += lineGap * m_scale;
			sx = 0;
			continue;
		}
		
		stbtt_GetCodepointBitmapBox(&m_info, str[i], m_scale, m_scale,
			&bx, &by, &bw, &bh);

		int width = bw - bx;
		int height = bh - by;

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

void Font::drawString(int x, int y, const std::string& str, Color color, bool top_screen, bool side)
{
	if(str.length() > 0)
	{
		std::wstring unicode_str = std::wstring(str.begin(), str.end());
		drawStringUnicode(x, y, unicode_str, color, top_screen, side);
	}
}

bool Font::isLoaded()
{
	return m_info.data != 0;
}

float Font::measureText(const std::string& text)
{	
	float length = 0.0f;
	float lineLength = 0.0f;
	
	if(isLoaded())
	{
		for(unsigned int i = 0; i < text.length(); i++)
		{
			if(text[i] == '\n')
			{
				if(lineLength > length) {
					length = lineLength;
				}
				
				lineLength = 0.0f;
				continue;
			}
			
			int advance = 0;
			stbtt_GetCodepointHMetrics(&m_info, text[i], &advance, 0);
			
			lineLength += advance;
		}
		
		if(lineLength > length)
			length = lineLength;
	}
	
	return length;
}
