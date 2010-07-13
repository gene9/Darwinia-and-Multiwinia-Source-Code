#ifndef VECTOR3_H
#define VECTOR3_H


#include "lib/math_utils.h"
#include "lib/vector2.h"

class Vector3;

extern Vector3 const g_upVector;
extern Vector3 const g_zeroVector;

// C++ does not specify the order of evaluation for a method with multiple arguments,
// which includes constructors. But for frand() and sfrand() to stay synchronized
// we need to guarantee order, and the Vector3 constructor often takes arguments
// wrapped in frand() and sfrand() calls.
//
// The solution is the FRAND and SFRAND classes, which we use as markers to indicate
// that the Vector3 constructor should invoke frand() or sfrand() on behalf of the
// caller, in a left-to-right order.
#ifdef TRACK_SYNC_RAND
	#define FRAND(x) RandomFloat(x, __FILE__, __LINE__)
	#define SFRAND(x) SymmetricRandomFloat(x, __FILE__, __LINE__)
#else
	#define FRAND(x) RandomFloat(x)
	#define SFRAND(x) SymmetricRandomFloat(x)
#endif

struct RandomFloat
{
	double v;
	const char *file;
	int line;
	
	RandomFloat(double _v, const char *_file = NULL, int _line = 0)
	{
#ifdef TRACK_SYNC_RAND
		file = _file;
		line = _line;
#endif
		v = _v;
	}
};

struct SymmetricRandomFloat
{
	double v;
	const char *file;
	int line;
	
	SymmetricRandomFloat(double _v, const char *_file = NULL, int _line = 0)
	{
#ifdef TRACK_SYNC_RAND
		file = _file;
		line = _line;
#endif
		v = _v;
	}
};

double FromRand(const RandomFloat &frandWrapper);
double FromRand(const SymmetricRandomFloat &sfrandWrapper);
inline double FromRand(const double d) { return d; }

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
	:	x(0.0), y(0.0), z(0.0) 
	{
	}

	Vector3(double _x, double _y, double _z)
	:	x(_x), y(_y), z(_z) 
	{
	}

	Vector3(Vector2 const &_b)
	:	x(_b.x), y(0.0), z(_b.y)
	{
	}
	
	template <class X, class Y, class Z>
	Vector3(X _x, Y _y, Z _z)
	{
		x = FromRand(_x);
		y = FromRand(_y);
		z = FromRand(_z);
	}

	void Zero()
	{
		x = y = z = 0.0;
	}

	void Set(double _x, double _y, double _z)
	{
		x = _x;
		y = _y;
		z = _z;
	}
	
	template <class X, class Y, class Z>
	void Set(X _x, Y _y, Z _z)
	{
		x = FromRand(_x);
		y = FromRand(_y);
		z = FromRand(_z);
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
		double multiplier = 1.0 / b;
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
		double multiplier = 1.0 / b;
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
		// volatile for sync
		volatile double lenSqrd = x*x + y*y + z*z;

		// Make sure lenSqrd not too small, otherwise +/- INF results
		if( lenSqrd > 0.0000000001 ) 
		{
			double invLen = 1.0 / iv_sqrt(lenSqrd);
			x *= invLen;
			y *= invLen;
			z *= invLen;
		}
		else
		{
			x = y = 0.0;
			z = 1.0;
		}

		return *this;
	}

	Vector3 const &SetLength(double _len)
	{
		double mag = Mag();
		if (NearlyEquals(mag, 0.0))
		{	
			x = _len;
			return *this;
		}

		double scaler = _len / mag;
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
		return iv_sqrt(x * x + y * y + z * z);
	}

	double MagSquared() const
	{
		return x * x + y * y + z * z;
	}

	void HorizontalAndNormalise()
	{
		y = 0.0;
		double invLength = 1.0 / iv_sqrt(x * x + z * z);
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
