#ifndef INCLUDED_VECTOR2_H
#define INCLUDED_VECTOR2_H

class Vector3;

class Vector2
{
private:
	bool Compare(Vector2 const &b) const;

public:
	double x, y;

	Vector2();
	Vector2(Vector3 const &);
	Vector2(double _x, double _y);

	void	Zero();
    void	Set	(double _x, double _y);

	double	operator ^ (Vector2 const &b) const;	// Cross product
	double   operator * (Vector2 const &b) const;	// Dot product

	Vector2 operator - () const;					// Negate
	Vector2 operator + (Vector2 const &b) const;
	Vector2 operator - (Vector2 const &b) const;
	Vector2 operator * (double const b) const;		// Scale
	Vector2 operator / (double const b) const;

    void	operator = (Vector2 const &b);
    void	operator = (Vector3 const &b);
	void	operator *= (double const b);
	void	operator /= (double const b);
	void	operator += (Vector2 const &b);
	void	operator -= (Vector2 const &b);

	bool operator == (Vector2 const &b) const;		// Uses FLT_EPSILON
	bool operator != (Vector2 const &b) const;		// Uses FLT_EPSILON

	Vector2 const &Normalise();
	void	SetLength		(double _len);
	
    double	Mag				() const;
	double	MagSquared		() const;

	double  *GetData			();
};


// Operator * between double and Vector2
inline Vector2 operator * (	double _scale, Vector2 const &_v )
{
	return _v * _scale;
}


#endif
