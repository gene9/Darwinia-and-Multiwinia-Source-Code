#ifndef VECTOR3_H
#define VECTOR3_H


#include "math_utils.h"
#include "vector2.h"

class Vector3;

extern Vector3 const g_upVector;
extern Vector3 const g_zeroVector;


class Vector3
{
private:
	bool Compare(Vector3 const &b) const
	{
		return ( NearlyEquals(x, b.x) &&
				 NearlyEquals(y, b.y) &&
				 NearlyEquals(z, b.z) );
	}

public:
	double x, y, z;

	// *** Constructors ***
	Vector3()
	:	x(0.0f), y(0.0f), z(0.0f) 
	{
	}

	Vector3(double _x, double _y, double _z)
	:	x(_x), y(_y), z(_z) 
	{
	}

	Vector3(Vector2 const &_b)
	:	x(_b.x), y(0.0f), z(_b.y)
	{
	}

	void Zero()
	{
		x = y = z = 0.0f;
	}

	void Set(double _x, double _y, double _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}

	Vector3 operator ^ (Vector3 const &b) const
	{
		return Vector3(y*b.z - z*b.y, z*b.x - x*b.z, x*b.y - y*b.x); 
	}

	double operator * (Vector3 const &b) const
	{
		return x*b.x + y*b.y + z*b.z;
	}

	Vector3 operator - () const
	{
		return Vector3(-x, -y, -z);
	}

	Vector3 operator + (Vector3 const &b) const
	{
		return Vector3(x + b.x, y + b.y, z + b.z);
	}

	Vector3 operator - (Vector3 const &b) const
	{
		return Vector3(x - b.x, y - b.y, z - b.z);
	}

	Vector3 operator * (double const b) const
	{
		return Vector3(x * b, y * b, z * b);
	}

	Vector3 operator / (double const b) const
	{
		double multiplier = 1.0f / b;
		return Vector3(x * multiplier, y * multiplier, z * multiplier);
	}

	Vector3 const &operator = (Vector3 const &b)
	{
		x = b.x;
		y = b.y;
		z = b.z;
		return *this;
	}

	Vector3 const &operator *= (double const b)
	{
		x *= b;
		y *= b;
		z *= b;
		return *this;
	}

	Vector3 const &operator /= (double const b)
	{
		double multiplier = 1.0f / b;
		x *= multiplier;
		y *= multiplier;
		z *= multiplier;
		return *this;
	}

	Vector3 const &operator += (Vector3 const &b)
	{
		x += b.x;
		y += b.y;
		z += b.z;
		return *this;
	}

	Vector3 const &operator -= (Vector3 const &b)
	{
		x -= b.x;
		y -= b.y;
		z -= b.z;
		return *this;
	}

	Vector3 const &Normalise()
	{
		double lenSqrd = x*x + y*y + z*z;
		if (lenSqrd > 0.0f)
		{
			double invLen = 1.0f / sqrtf(lenSqrd);
			x *= invLen;
			y *= invLen;
			z *= invLen;
		}
		else
		{
			x = y = 0.0f;
			z = 1.0f;
		}

		return *this;
	}

	Vector3 const &SetLength(double _len)
	{
		double mag = Mag();
		if (NearlyEquals(mag, 0.0f))
		{	
			x = _len;
			return *this;
		}

		double scaler = _len / Mag();
		*this *= scaler;
		return *this;
	}


	bool operator == (Vector3 const &b) const;		// Uses FLT_EPSILON
	bool operator != (Vector3 const &b) const;		// Uses FLT_EPSILON

	void	RotateAroundX	(double _angle);
	void	RotateAroundY	(double _angle);
	void	RotateAroundZ	(double _angle);
	void	FastRotateAround(Vector3 const &_norm, double _angle);
	void	RotateAround	(Vector3 const &_norm);

	double Mag() const
	{
		return sqrtf(x * x + y * y + z * z);
	}

	double MagSquared() const
	{
		return x * x + y * y + z * z;
	}

	void HorizontalAndNormalise()
	{
		y = 0.0f;
		double invLength = 1.0f / sqrtf(x * x + z * z);
		x *= invLength;
		z *= invLength;
	}

	double *GetData()
	{
		return &x;
	}

	double const *GetDataConst() const
	{
		return &x;
	}
};


// Operator * between double and Vector3
inline Vector3 operator * (	double _scale, Vector3 const &_v )
{
	return _v * _scale;
}


#endif
