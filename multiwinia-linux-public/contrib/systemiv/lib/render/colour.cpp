#define _CRT_SECURE_NO_DEPRECATE
#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>
#include <algorithm>

#include "lib/debug_utils.h"
#include "colour.h"


// *** Constructor
Colour::Colour()
{
    Set(0,0,0,255);
}


// *** Constructor
Colour::Colour(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
{
    Set( _r, _g, _b, _a );
}

Colour::Colour(int _col)
{
    Set( _col);
}

// *** Set
void Colour::Set(unsigned char _r, unsigned char _g, unsigned char _b, unsigned char _a)
{
	m_r = _r;
	m_g = _g;
	m_b = _b;
	m_a = _a;
}


void Colour::Set(int _col)
{
    m_r = (_col & 0xff000000) >> 24;
    m_g = (_col & 0x00ff0000) >> 16;
    m_b = (_col & 0x0000ff00) >> 8;
    m_a = (_col & 0x000000ff) >> 0;
}


// *** Operator +
Colour Colour::operator + (Colour const &_b) const
{
	return Colour((m_r + _b.m_r),
					  (m_g + _b.m_g),
					  (m_b + _b.m_b),
					  (m_a + _b.m_a));
}


// *** Operator -
Colour Colour::operator - (Colour const &_b) const
{
	return Colour((m_r - _b.m_r), 
					  (m_g - _b.m_g), 
					  (m_b - _b.m_b),
					  (m_a - _b.m_a));
}


// *** Operator *
Colour Colour::operator * (float const _b) const
{
	return Colour((unsigned char) ((float)m_r * _b),
					  (unsigned char) ((float)m_g * _b),
					  (unsigned char) ((float)m_b * _b),
					  (unsigned char) ((float)m_a * _b));
}


// *** Operator /
Colour Colour::operator / (float const _b) const
{
	float multiplier = 1.0f / _b;
	return Colour((unsigned char) ((float)m_r * multiplier),
					  (unsigned char) ((float)m_g * multiplier),
					  (unsigned char) ((float)m_b * multiplier),
					  (unsigned char) ((float)m_a * multiplier));
}


// *** Operator =
Colour const &Colour::operator = (Colour const &_b)
{
	m_r = _b.m_r;
	m_g = _b.m_g;
	m_b = _b.m_b;
	m_a = _b.m_a;
	return *this;
}


// *** Operator *=
Colour const &Colour::operator *= (float const _b)
{
	m_r = (unsigned char)((float)m_r * _b);
	m_g = (unsigned char)((float)m_g * _b);
	m_b = (unsigned char)((float)m_b * _b);
	m_a = (unsigned char)((float)m_a * _b);
	return *this;
}


// *** Operator /=
Colour const &Colour::operator /= (float const _b)
{
	float multiplier = 1.0f / _b;
	m_r = (unsigned char)((float)m_r * multiplier);
	m_g = (unsigned char)((float)m_g * multiplier);
	m_b = (unsigned char)((float)m_b * multiplier);
	m_a = (unsigned char)((float)m_a * multiplier);
	return *this;
}


// *** Operator +=
Colour const &Colour::operator += (Colour const &_b)
{
	m_r += _b.m_r;
	m_g += _b.m_g;
	m_b += _b.m_b;
	m_a += _b.m_a;
	return *this;
}


// *** Operator -=
Colour const &Colour::operator -= (Colour const &_b)
{
	m_r -= _b.m_r;
	m_g -= _b.m_g;
	m_b -= _b.m_b;
	m_a -= _b.m_a;
	return *this;
}


// *** Operator ==
bool Colour::operator == (Colour const &_b) const
{
	return (m_r == _b.m_r && m_g == _b.m_g && m_b == _b.m_b && m_a == _b.m_a);
}


// *** Operator !=
bool Colour::operator != (Colour const &_b) const
{
	return (m_r != _b.m_r || m_g != _b.m_g || m_b != _b.m_b || m_a != _b.m_a);
}


// *** GetData
unsigned char const *Colour::GetData() const
{
	return &m_r;
}


void Colour::GetHexValue( char *_hex )
{
    sprintf( _hex, "%02x%02x%02x%02x", m_r, m_g, m_b, m_a );
}   


void Colour::LoadFromHex( char *_hex )
{
    // Do a sanity check first

    bool error = false;

    if( strlen(_hex) != 8 ) error = true;

    for( int i = 0; i < 8; ++i )
    {
        bool isNumber = (_hex[i] >= '0' && _hex[i] <= '9');
        bool isChar   = (_hex[i] >= 'a' && _hex[i] <= 'f');

        if( !isNumber && !isChar ) error = true;
    }

    if( error )
    {
        AppDebugOut( "Error parsing colour hex '%s'\n", _hex );
        return;
    }


    //
    // Looks ok
    
    int red, green, blue, alpha;
    sscanf( _hex, "%2x%2x%2x%2x", &red, &green, &blue, &alpha );   

    m_r = red;
    m_g = green;
    m_b = blue;
    m_a = alpha;
}


// *** AddWithClamp
void Colour::AddWithClamp( Colour const &_b)
{
    float alpha = (float) _b.m_a / 255.0f;;

    int newR = (int)m_r + int( _b.m_r * alpha );
    int newG = (int)m_g + int( _b.m_g * alpha );
    int newB = (int)m_b + int( _b.m_b * alpha );

    newR = std::max(newR, 0);
    newG = std::max(newG, 0);
    newB = std::max(newB, 0);

    newR = std::min(newR, 255);
    newG = std::min(newG, 255);
    newB = std::min(newB, 255);

    m_r = newR;
    m_g = newG;
    m_b = newB;
}


// *** MultiplyWithClamp
void Colour::MultiplyWithClamp(float _scale)
{
	if ((float)m_r * _scale < 255.0f)
		m_r = (unsigned char) ((float)m_r * _scale);
	else
		m_r = 255;

	if ((float)m_g * _scale < 255.0f)	
		m_g = (unsigned char) ((float)m_g * _scale);
	else
		m_g = 255;
	
	if ((float)m_b * _scale < 255.0f)	
		m_b = (unsigned char) ((float)m_b * _scale);
	else
		m_b = 255;
	
	if ((float)m_a * _scale < 255.0f)	
		m_a = (unsigned char)((float)m_a * _scale);
	else 
		m_a = 255;
}
