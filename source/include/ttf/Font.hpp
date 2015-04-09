// Font.hpp
// @author: Nanni

#ifndef FONT_HPP_INC
#define FONT_HPP_INC

#include "Common.hpp"
#include "Image.hpp"
#include "stb_truetype.h"

class Font
{
public:
	Font();

	Font(const std::string& filename);

	Font(const unsigned char* buffer, unsigned int buffer_size);

	Font(const std::vector<unsigned char>& buffer);

	unsigned char* loadFromFile(const std::string& filename);

	bool loadFromMemory(const unsigned char* buffer, unsigned int buffer_size);

	bool loadFromMemory(const std::vector<unsigned char>& buffer);

	void setSize(int pixels);

	void setScale(float scale);
	
	float measureText(const std::string& text);
	
	bool isLoaded();

	float getScale() const { return m_scale; }
	
	void drawString(int x, int y, const std::string& str, Color color, bool top_screen = true, bool side = true);
	
	void drawStringUnicode(int x, int y, const std::wstring& str, Color color, bool top_screen = true, bool side = true);

private:
	stbtt_fontinfo m_info;
	float m_scale;
};

#endif // FONT_HPP_INC
