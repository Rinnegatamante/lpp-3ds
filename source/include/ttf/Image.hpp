// Image.hpp
// @author: Nanni

#ifndef IMAGE_HPP_INC
#define IMAGE_HPP_INC

#include "Common.hpp"
#include "Color.hpp"

class Image
{
public:
	enum Format
	{
		Format_Png,
		Format_Bmp,
		Format_Tga
	};

	Image();

	Image(int width, int height, Color fill_color);

	Image(const std::string& filename);

	Image(const std::vector<unsigned char>& buffer);

	Image(const unsigned char* buffer, unsigned int buffer_size);

	bool create(int width, int height, Color fill_color);

	bool loadFromFile(const std::string& filename);

	bool loadFromMemory(const unsigned char* buffer, unsigned int buffer_size);

	bool loadFromMemory(const std::vector<unsigned char>& buffer);

	bool saveToFile(const std::string& filename, Format format);

	bool saveToMemory(std::vector<unsigned char>& out_buffer);

	bool saveToMemory(unsigned char** out_buffer, unsigned int* out_buffer_size);

	int getWidth() const { return m_width; }

	int getHeight() const { return m_height; }

	unsigned int getSize() const { return m_pixels.size(); }

	void putPixel(int x, int y, Color color);

	Color getPixel(int x, int y);

	void setPixels(const unsigned char* pixels, unsigned int pixels_size);

	bool isLoaded() const;

	void draw(int x, int y, bool top_screen = true, bool side = true);

	std::vector<unsigned char>& getPixels() { return m_pixels; }

	unsigned char* getPixelsPointer();

private:
	int m_width;
	int m_height;
	std::vector<unsigned char> m_pixels;
};

#endif // IMAGE_HPP_INC
