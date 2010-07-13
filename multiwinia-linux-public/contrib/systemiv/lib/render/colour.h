#ifndef INCLUDED_COLOUR_H
#define INCLUDED_COLOUR_H


class Colour
{
public:
	unsigned char m_r;
    unsigned char m_g;
    unsigned char m_b;
    unsigned char m_a;

public:
	Colour();
    Colour(int _col);
	Colour(unsigned char r, 
           unsigned char g, 
           unsigned char b, 
           unsigned char a = 255);

    void Set(int _col);
	void Set(unsigned char r, 
             unsigned char g, 
             unsigned char b, 
             unsigned char a = 255);

	Colour operator + (Colour const &b) const;
	Colour operator - (Colour const &b) const;
	Colour operator * (float const b) const;
	Colour operator / (float const b) const;

    Colour const &operator  = (Colour const &b);
	Colour const &operator *= (float const b);
	Colour const &operator /= (float const b);
	Colour const &operator += (Colour const &b);
	Colour const &operator -= (Colour const &b);

	bool operator == (Colour const &b) const;
	bool operator != (Colour const &b) const;

	unsigned char const *GetData() const;

    void GetHexValue( char *_hex );
    void LoadFromHex( char *_hex );

    void AddWithClamp       ( Colour const &b);
	void MultiplyWithClamp  (float _scale);
};



// Operator * between float and RGBAColour
inline Colour operator * ( float _scale, Colour const &_v )
{
	return _v * _scale;
}


#endif
