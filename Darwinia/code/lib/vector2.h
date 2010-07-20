#ifndef INCLUDED_VECTOR2_H
#define INCLUDED_VECTOR2_H

class Vector3;

class Vector2
{
private:
	bool Compare(Vector2 const &b) const;

public:
	float x, y;

	Vector2();
	Vector2(Vector3 const &);
	Vector2(float _x, float _y);

	void	Zero();
    void	Set	(float _x, float _y);

	float	operator ^ (Vector2 const &b) const;	// Cross product
	float   operator * (Vector2 const &b) const;	// Dot product

	Vector2 operator - () const;					// Negate
	Vector2 operator + (Vector2 const &b) const;
	Vector2 operator - (Vector2 const &b) const;
	Vector2 operator * (float const b) const;		// Scale
	Vector2 operator / (float const b) const;

    void	operator = (Vector2 const &b);
    void	operator = (Vector3 const &b);
	void	operator *= (float const b);
	void	operator /= (float const b);
	void	operator += (Vector2 const &b);
	void	operator -= (Vector2 const &b);

	bool operator == (Vector2 const &b) const;		// Uses FLT_EPSILON
	bool operator != (Vector2 const &b) const;		// Uses FLT_EPSILON

	Vector2 const &Normalise();
	void	SetLength		(float _len);

    float	Mag				() const;
	float	MagSquared		() const;

	float  *GetData			();
};


// Operator * between float and Vector2
inline Vector2 operator * (	float _scale, Vector2 const &_v )
{
	return _v * _scale;
}


#endif
