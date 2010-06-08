#ifndef INCLUDED_TEXTURE_UV_H
#define INCLUDED_TEXTURE_UV_H

class TextureUV
{
public:
	float u, v;

	TextureUV();
	TextureUV(float u, float v);

	void Set(float u, float v);

	TextureUV operator + (TextureUV const &b) const;
	TextureUV operator - (TextureUV const &b) const;
	TextureUV operator * (float const b) const;
	TextureUV operator / (float const b) const;

    TextureUV const &operator = (TextureUV const &b);
	TextureUV const &operator *= (float const b);
	TextureUV const &operator /= (float const b);
	TextureUV const &operator += (TextureUV const &b);
	TextureUV const &operator -= (TextureUV const &b);

	bool operator == (TextureUV const &b) const;
	bool operator != (TextureUV const &b) const;

	float *GetData();
};

// Operator * between float and TextureUV
inline TextureUV operator * ( float _scale, TextureUV const &_v )
{
	return _v * _scale;
}

#endif
