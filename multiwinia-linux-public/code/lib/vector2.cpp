#include "lib/universal_include.h"

#include "vector2.h"
#include "vector3.h"

#include <math.h>
#include <float.h>



// *******************
//  Private Functions
// *******************

// *** Compare
bool Vector2::Compare(Vector2 const &b) const
{
	return ( (fabsf(x - b.x) < FLT_EPSILON) &&
			 (fabsf(y - b.y) < FLT_EPSILON) );
}


// ******************
//  Public Functions
// ******************

// Constructor
Vector2::Vector2()
:	x(0.0),
	y(0.0) 
{
}


// Constructor
Vector2::Vector2(Vector3 const &v)
:	x(v.x),
	y(v.z)
{
}


// Constructor
Vector2::Vector2(double _x, double _y)
:	x(_x),
	y(_y) 
{
}


void Vector2::Zero()
{
	x = y = 0.0;
}


void Vector2::Set(double _x, double _y)
{
	x = _x;
	y = _y;
}


// Cross Product
double Vector2::operator ^ (Vector2 const &b) const
{
	return x*b.y - y*b.x;
}


// Dot Product
double Vector2::operator * (Vector2 const &b) const
{
	return (x * b.x + y * b.y);
}


// Negate
Vector2 Vector2::operator - () const
{
	return Vector2(-x, -y);
}


Vector2 Vector2::operator + (Vector2 const &b) const
{
	return Vector2(x + b.x, y + b.y);
}


Vector2 Vector2::operator - (Vector2 const &b) const
{
	return Vector2(x - b.x, y - b.y);
}


Vector2 Vector2::operator * (double const b) const
{
	return Vector2(x * b, y * b);
}


Vector2 Vector2::operator / (double const b) const
{
	double multiplier = 1.0 / b;
	return Vector2(x * multiplier, y * multiplier);
}


void Vector2::operator = (Vector2 const &b)
{
	x = b.x;
	y = b.y;
}


// Assign from a Vector3 - throws away Y value of Vector3
void Vector2::operator = (Vector3 const &b)
{
	x = b.x;
	y = b.z;
}


// Scale
void Vector2::operator *= (double const b)
{
	x *= b;
	y *= b;
}


// Scale
void Vector2::operator /= (double const b)
{
	double multiplier = 1.0 / b;
	x *= multiplier;
	y *= multiplier;
}


void Vector2::operator += (Vector2 const &b)
{
	x += b.x;
	y += b.y;
}


void Vector2::operator -= (Vector2 const &b)
{
	x -= b.x;
	y -= b.y;
}


Vector2 const &Vector2::Normalise()
{
	double lenSqrd = x*x + y*y;
	if (lenSqrd > 0.0)
	{
		double invLen = 1.0 / iv_sqrt(lenSqrd);
		x *= invLen;
		y *= invLen;
	}
	else
	{
		x = 0.0;
        y = 1.0;
	}

	return *this;
}


void Vector2::SetLength(double _len)
{
	double scaler = _len / Mag();
	x *= scaler;
	y *= scaler;
}


double Vector2::Mag() const
{
    return iv_sqrt(x * x + y * y);
}


double Vector2::MagSquared() const
{
    return x * x + y * y;
}


bool Vector2::operator == (Vector2 const &b) const
{
	return Compare(b);
}


bool Vector2::operator != (Vector2 const &b) const
{
	return !Compare(b);
}


double *Vector2::GetData()
{
	return &x;
}
