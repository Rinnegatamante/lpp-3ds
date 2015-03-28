// Color.hpp
// @author: Nanni

#ifndef COLOR_HPP_INC
#define COLOR_HPP_INC

#include "Common.hpp"

class Color
{
public:
	Color(unsigned char r, unsigned char g, unsigned char b,
		unsigned char a = 255)
	{
		this->r = r;
		this->g = g;
		this->b = b;
		this->a = a;
	}

	Color() : r(0), b(0), a(0xFF) {}

	float getRedf() { return r / 255.0f; }

	float getBluef() { return b / 255.0f; }

	float getGreenf() { return g / 255.0f; }

	float getAlphaf() { return a / 255.0f; }

	unsigned char r, g, b, a;
};

#endif // COLOR_HPP_INC
