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
:	x(0.0f),
	y(0.0f)
{
}


// Constructor
Vector2::Vector2(Vector3 const &v)
:	x(v.x),
	y(v.z)
{
}


// Constructor
Vector2::Vector2(float _x, float _y)
:	x(_x),
	y(_y)
{
}


void Vector2::Zero()
{
	x = y = 0.0f;
}


void Vector2::Set(float _x, float _y)
{
	x = _x;
	y = _y;
}


// Cross Product
float Vector2::operator ^ (Vector2 const &b) const
{
	return x*b.y - y*b.x;
}


// Dot Product
float Vector2::operator * (Vector2 const &b) const
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


Vector2 Vector2::operator * (float const b) const
{
	return Vector2(x * b, y * b);
}


Vector2 Vector2::operator / (float const b) const
{
	float multiplier = 1.0f / b;
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
void Vector2::operator *= (float const b)
{
	x *= b;
	y *= b;
}


// Scale
void Vector2::operator /= (float const b)
{
	float multiplier = 1.0f / b;
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
	float lenSqrd = x*x + y*y;
	if (lenSqrd > 0.0f)
	{
		float invLen = 1.0f / sqrtf(lenSqrd);
		x *= invLen;
		y *= invLen;
	}
	else
	{
		x = 0.0f;
        y = 1.0f;
	}

	return *this;
}


void Vector2::SetLength(float _len)
{
	float scaler = _len / Mag();
	x *= scaler;
	y *= scaler;
}


float Vector2::Mag() const
{
    return sqrtf(x * x + y * y);
}


float Vector2::MagSquared() const
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


float *Vector2::GetData()
{
	return &x;
}
