#include "lib/universal_include.h"

#include "texture_uv.h"


// *** Constructor
TextureUV::TextureUV()
{
}


// *** Constructor
TextureUV::TextureUV(float _u, float _v)
:	u(_u),
	v(_v)
{
}


// *** Set
void TextureUV::Set(float _u, float _v)
{
	u = _u;
	v = _v;
}


// *** Operator +
TextureUV TextureUV::operator + (TextureUV const &_b) const
{
	return TextureUV(u + _b.u, v + _b.v);
}


// *** Operator -
TextureUV TextureUV::operator - (TextureUV const &_b) const
{
	return TextureUV(u - _b.u, v - _b.v);
}


// *** Operator *
TextureUV TextureUV::operator * (float const _b) const
{
	return TextureUV(u * _b, v * _b);
}


// *** Operator /
TextureUV TextureUV::operator / (float const _b) const
{
	float multiplier = 1.0f / _b;
	return TextureUV(u * multiplier, v * multiplier);
}


// *** Operator =
TextureUV const &TextureUV::operator = (TextureUV const &_b)
{
	u = _b.u;
	v = _b.v;
	return *this;
}


// *** Operator *=
TextureUV const &TextureUV::operator *= (float const _b)
{
	u *= _b;
	v *= _b;
	return *this;
}


// *** Operator /=
TextureUV const &TextureUV::operator /= (float const _b)
{
	float multiplier = 1.0f / _b;
	u *= multiplier;
	v *= multiplier;
	return *this;
}


// *** Operator +=
TextureUV const &TextureUV::operator += (TextureUV const &_b)
{
	u += _b.u;
	v += _b.v;
	return *this;
}


// *** Operator -=
TextureUV const &TextureUV::operator -= (TextureUV const &_b)
{
	u -= _b.u;
	v -= _b.v;
	return *this;
}


// *** GetData
float *TextureUV::GetData()
{
	return &u;
}
