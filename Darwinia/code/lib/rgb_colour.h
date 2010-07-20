#ifndef INCLUDED_RGB_COLOUR_H
#define INCLUDED_RGB_COLOUR_H

#include "debug_utils.h"

class RGBAColour
{
public:
	unsigned char r, g, b, a;

	RGBAColour();
	RGBAColour(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
    RGBAColour(int _col);

	void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255);
    void Set(int _col);

	RGBAColour operator + (RGBAColour const &b) const;
	RGBAColour operator - (RGBAColour const &b) const;
	RGBAColour operator * (float const b) const;
	RGBAColour operator / (float const b) const;

    RGBAColour const &operator = (RGBAColour const &b)
	{
		// only this one is inlined, because only this inline has visible effect on fps (1%)
		// clean code:
		//r = _b.r;
		//g = _b.g;
		//b = _b.b;
		//a = _b.a;
		// faster code:
		//  but not completely safe (c++ aliasing rules)
		DarwiniaDebugAssert(sizeof(int)==4);
		*(int*)this = *(int*)&b;
		return *this;
	}
	RGBAColour const &operator *= (float const b);
	RGBAColour const &operator /= (float const b);
	RGBAColour const &operator += (RGBAColour const &b);
	RGBAColour const &operator -= (RGBAColour const &b);

	bool operator == (RGBAColour const &b) const;
	bool operator != (RGBAColour const &b) const;

	unsigned char const *GetData() const;

    void AddWithClamp( RGBAColour const &b);
	void MultiplyWithClamp(float _scale);
};

// Operator * between float and RGBAColour
inline RGBAColour operator * ( float _scale, RGBAColour const &_v )
{
	return _v * _scale;
}


extern RGBAColour g_colourBlack;
extern RGBAColour g_colourWhite;


#endif
