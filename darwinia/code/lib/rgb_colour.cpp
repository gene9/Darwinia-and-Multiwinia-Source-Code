#include "lib/universal_include.h"

#include "rgb_colour.h"

RGBAColour g_colourBlack(0,0,0);
RGBAColour g_colourWhite(255,255,255);


// *** Constructor
RGBAColour::RGBAColour()
{
}


// *** Constructor
RGBAColour::RGBAColour(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
{
    Set( _r, _g, _b, _a );
}

RGBAColour::RGBAColour(int _col)
{
    Set( _col);
}

// *** Set
void RGBAColour::Set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
{
	r = _r;
	g = _g;
	b = _b;
	a = _a;
}


void RGBAColour::Set(int _col)
{
    r = (_col & 0xff000000) >> 24;
    g = (_col & 0x00ff0000) >> 16;
    b = (_col & 0x0000ff00) >> 8;
    a = (_col & 0x000000ff) >> 0;
}


// *** Operator +
RGBAColour RGBAColour::operator + (RGBAColour const &_b) const
{
	return RGBAColour((r + _b.r),
					  (g + _b.g),
					  (b + _b.b),
					  (a + _b.a));
}


// *** Operator -
RGBAColour RGBAColour::operator - (RGBAColour const &_b) const
{
	return RGBAColour((r - _b.r), 
					  (g - _b.g), 
					  (b - _b.b),
					  (a - _b.a));
}


// *** Operator *
RGBAColour RGBAColour::operator * (float const _b) const
{
	return RGBAColour((unsigned char) ((float)r * _b),
					  (unsigned char) ((float)g * _b),
					  (unsigned char) ((float)b * _b),
					  (unsigned char) ((float)a * _b));
}


// *** Operator /
RGBAColour RGBAColour::operator / (float const _b) const
{
	float multiplier = 1.0f / _b;
	return RGBAColour((unsigned char) ((float)r * multiplier),
					  (unsigned char) ((float)g * multiplier),
					  (unsigned char) ((float)b * multiplier),
					  (unsigned char) ((float)a * multiplier));
}


// *** Operator *=
RGBAColour const &RGBAColour::operator *= (float const _b)
{
	r = (unsigned char)((float)r * _b);
	g = (unsigned char)((float)g * _b);
	b = (unsigned char)((float)b * _b);
	a = (unsigned char)((float)a * _b);
	return *this;
}


// *** Operator /=
RGBAColour const &RGBAColour::operator /= (float const _b)
{
	float multiplier = 1.0f / _b;
	r = (unsigned char)((float)r * multiplier);
	g = (unsigned char)((float)g * multiplier);
	b = (unsigned char)((float)b * multiplier);
	a = (unsigned char)((float)a * multiplier);
	return *this;
}


// *** Operator +=
RGBAColour const &RGBAColour::operator += (RGBAColour const &_b)
{
	r += _b.r;
	g += _b.g;
	b += _b.b;
	a += _b.a;
	return *this;
}


// *** Operator -=
RGBAColour const &RGBAColour::operator -= (RGBAColour const &_b)
{
	r -= _b.r;
	g -= _b.g;
	b -= _b.b;
	a -= _b.a;
	return *this;
}


// *** Operator ==
bool RGBAColour::operator == (RGBAColour const &_b) const
{
	return (r == _b.r && g == _b.g && b == _b.b && a == _b.a);
}


// *** Operator !=
bool RGBAColour::operator != (RGBAColour const &_b) const
{
	return (r != _b.r || g != _b.g || b != _b.b || a != _b.a);
}


// *** GetData
unsigned char const *RGBAColour::GetData() const
{
	return &r;
}


// *** AddWithClamp
void RGBAColour::AddWithClamp( RGBAColour const &_b)
{
    float alpha = (float) _b.a / 255.0f;;

    int newR = (int)r + int( _b.r * alpha );
    int newG = (int)g + int( _b.g * alpha );
    int newB = (int)b + int( _b.b * alpha );
    
    newR = max(newR, 0);
    newG = max(newG, 0);
    newB = max(newB, 0);

    newR = min(newR, 255);
    newG = min(newG, 255);
    newB = min(newB, 255);

    r = newR;
    g = newG;
    b = newB;
}


// *** MultiplyWithClamp
void RGBAColour::MultiplyWithClamp(float _scale)
{
	if ((float)r * _scale < 255.0f)
		r = (unsigned char) ((float)r * _scale);
	else
		r = 255;

	if ((float)g * _scale < 255.0f)	
		g = (unsigned char) ((float)g * _scale);
	else
		g = 255;
	
	if ((float)b * _scale < 255.0f)	
		b = (unsigned char) ((float)b * _scale);
	else
		b = 255;
	
	if ((float)a * _scale < 255.0f)	
		a = (unsigned char)((float)a * _scale);
	else 
		a = 255;
}
