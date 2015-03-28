// Image.cpp
// @author: Nanni

#include "Image.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <3ds.h>

Image::Image()
{
	m_width = 0;
	m_height = 0;
}

Image::Image(int width, int height, Color fill_color)
{
	create(width, height, fill_color);
}

Image::Image(const std::string& filename)
{
	loadFromFile(filename);
}

Image::Image(const std::vector<unsigned char>& buffer)
{
	loadFromMemory(buffer);
}

Image::Image(const unsigned char* buffer, unsigned int buffer_size)
{
	loadFromMemory(buffer, buffer_size);
}

bool Image::create(int width, int height, Color fill_color)
{
	m_width = width;
	m_height = height;
	m_pixels.resize(width * height * 4);

	for (int x = 0; x < m_width; x++) {
		for (int y = 0; y < m_height; y++) {
			putPixel(x, y, fill_color);
		}
	}

	return true;
}

bool Image::loadFromFile(const std::string& filename)
{
	int loaded_width = 0;
	int loaded_height = 0;
	unsigned char* loaded_buffer = stbi_load(filename.c_str(), &loaded_width,
		&loaded_height, 0, 4);
	if (!loaded_buffer || loaded_width == 0 || loaded_height == 0)
		return false;

	m_width = loaded_width;
	m_height = loaded_height;

	unsigned int size = m_width * m_height * 4;

	setPixels(loaded_buffer, size);

	stbi_image_free(loaded_buffer);

	return true;
}

bool Image::loadFromMemory(const std::vector<unsigned char>& buffer)
{
	if (buffer.size() == 0)
		return false;
	return loadFromMemory(&buffer[0], buffer.size());
}

bool Image::loadFromMemory(const unsigned char* buffer, unsigned int buffer_size)
{
	if (!buffer || buffer_size == 0)
		return false;

	int loaded_width = 0;
	int loaded_height = 0;

	unsigned char* loaded_buffer = stbi_load_from_memory(buffer,
		buffer_size, &loaded_width, &loaded_height, 0, 4);

	if (!loaded_buffer || loaded_width == 0 || loaded_height == 0)
		return false;

	m_width = loaded_width;
	m_height = loaded_height;

	unsigned int size = m_width * m_height * 4;

	setPixels(loaded_buffer, size);

	stbi_image_free(loaded_buffer);

	return true;
}

void Image::putPixel(int x, int y, Color color)
{
	if (x < 0 || y < 0 || x >= m_width || y >= m_height)
		return;
	
	unsigned int index = x + y * m_width;

	m_pixels[4 * index] = color.r;
	m_pixels[4 * index + 1] = color.g;
	m_pixels[4 * index + 2] = color.b;
	m_pixels[4 * index + 3] = color.a;
}

Color Image::getPixel(int x, int y)
{
	Color color;

	if (x >= 0 && y >= 0 && x < m_width && y < m_height)
	{
		unsigned int index = x + y * m_width;

		color.r = m_pixels[4 * index];
		color.g = m_pixels[4 * index + 1];
		color.b = m_pixels[4 * index + 2];
		color.a = m_pixels[4 * index + 3];
	}

	return color;
}

void Image::setPixels(const unsigned char* pixels, unsigned int pixels_size)
{
	if (!pixels)
	{
		m_pixels.resize(0);
		return;
	}

	m_pixels.resize(pixels_size);

	for (unsigned int i = 0; i < pixels_size; i++)
	{
		m_pixels[i] = pixels[i];
	}
}

bool Image::isLoaded() const
{
	return m_width > 0 && m_height > 0 &&
		m_pixels.size() > 0;
}

unsigned char* Image::getPixelsPointer()
{
	if (!isLoaded())
		return 0;

	return &m_pixels[0];
}

void Image::draw(int x, int y, bool top_screen, bool side)
{
	if (!isLoaded())
		return;

	u16 buffer_width = 0;
	u16 buffer_height = 0;

	u8* frameBuffer = gfxGetFramebuffer(top_screen ? GFX_TOP : GFX_BOTTOM, side ? GFX_LEFT : GFX_RIGHT,
		&buffer_height, &buffer_width);
	u8* pixelsPointer = getPixelsPointer();

	for (int ix = 0; ix < getWidth(); ix++) {
		for (int iy = 0; iy < getHeight(); iy++) {
			int xpos = ix + x;
			int ypos = iy + y;

			if (xpos < 0 || ypos < 0 || xpos >= buffer_width || ypos >= buffer_height)
				continue;

			unsigned char bg_r = frameBuffer[3 * (xpos * 240 + 239 - ypos) + 2];
			unsigned char bg_g = frameBuffer[3 * (xpos * 240 + 239 - ypos) + 1];
			unsigned char bg_b = frameBuffer[3 * (xpos * 240 + 239 - ypos) + 0];

			unsigned char fg_r = pixelsPointer[4 * (ix + iy * getWidth()) + 0];
			unsigned char fg_g = pixelsPointer[4 * (ix + iy * getWidth()) + 1];
			unsigned char fg_b = pixelsPointer[4 * (ix + iy * getWidth()) + 2];
			unsigned char fg_a = pixelsPointer[4 * (ix + iy * getWidth()) + 3];

			unsigned int alpha = fg_a + 1;
			unsigned int inv_alpha = 256 - fg_a;

			unsigned char res_r = (unsigned char)((alpha * fg_r + inv_alpha * bg_r) >> 8);
			unsigned char res_g = (unsigned char)((alpha * fg_g + inv_alpha * bg_g) >> 8);
			unsigned char res_b = (unsigned char)((alpha * fg_b + inv_alpha * bg_b) >> 8);

			frameBuffer[3 * (xpos * 240 + 239 - ypos) + 2] = res_r;
			frameBuffer[3 * (xpos * 240 + 239 - ypos) + 1] = res_g;
			frameBuffer[3 * (xpos * 240 + 239 - ypos) + 0] = res_b;
		}
	}
}

bool Image::saveToFile(const std::string& filename, Format format)
{
	switch (format)
	{
	case Format_Png:
		return stbi_write_png(filename.c_str(), getWidth(), getHeight(), 4,
			getPixelsPointer(), getWidth());
	case Format_Bmp:
		return stbi_write_bmp(filename.c_str(), getWidth(), getHeight(), 4,
			getPixelsPointer());
	default:
		return stbi_write_tga(filename.c_str(), getWidth(), getHeight(), 4,
			getPixelsPointer());
	}
}

bool Image::saveToMemory(unsigned char** buffer, unsigned int* buffer_size)
{
	if (!buffer || !buffer_size)
		return false;

	int out_len = 0;
	*buffer = stbi_write_png_to_mem(getPixelsPointer(), getWidth(), getWidth(), getHeight(), 4, &out_len);

	if (!*buffer)
		return false;

	*buffer_size = out_len;

	return true;
}

bool Image::saveToMemory(std::vector<unsigned char>& out_buffer)
{
	unsigned char* buffer = 0;
	unsigned int buffer_size = 0;

	if (!saveToMemory(&buffer, &buffer_size))
		return false;

	out_buffer.resize(buffer_size);

	for (unsigned int i = 0; i < buffer_size; i++)
	{
		out_buffer[i] = buffer[i];
	}

	free(buffer);

	return true;
}
