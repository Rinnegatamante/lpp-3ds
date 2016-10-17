// Font.hpp
// @author: Nanni

#ifndef FONT_HPP_INC
#define FONT_HPP_INC

#include "Common.hpp"
#include "Image.hpp"
#include "stb_truetype.h"
#include <cstring>

class Font
{
public:
	Font();

	Font(const std::string& filename);

	Font(const unsigned char* buffer, unsigned int buffer_size);

	Font(const std::vector<unsigned char>& buffer);

	void* loadFromFile(const std::string& filename);

	bool loadFromMemory(const unsigned char* buffer, unsigned int buffer_size);

	bool loadFromMemory(const std::vector<unsigned char>& buffer);

	void setSize(int pixels);

	void setScale(float scale);
	
	bool isLoaded();

	float getScale() const { return m_scale; }
	
	void drawString(int x, int y, const std::string& str, Color color, bool top_screen = true, bool side = true, int max_width = 0);
	
	void drawStringUnicode(int x, int y, const std::wstring& str, Color color, bool top_screen = true, bool side = true, int max_width = 0);

	void drawStringToBuffer(int x, int y, const std::wstring& str, Color color, unsigned char* buffer, int buffer_width, int buffer_height, int bitsperpixel, int max_width = 0);

	void measureText(const std::wstring str, int& width, int& height, int max_width = 0);

	static std::wstring utf8_to_UCS2(char * code);

private:
	stbtt_fontinfo m_info;
	float m_scale;
};

#endif // FONT_HPP_INC
