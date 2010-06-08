#include "lib/universal_include.h"

#include <math.h>
#include <float.h>

#include "lib/vector2.h"
#include "lib/vector3.h"
#include "lib/math_utils.h"


Vector3 const g_upVector(0.0f, 1.0f, 0.0f);
Vector3 const g_zeroVector(0.0f, 0.0f, 0.0f);


// ******************
//  Public Functions
// ******************

void Vector3::RotateAroundX(float _angle)
{
    FastRotateAround(Vector3(1,0,0), _angle);
}


void Vector3::RotateAroundY(float _angle)
{
    FastRotateAround(g_upVector, _angle);
}


void Vector3::RotateAroundZ(float _angle)
{
    FastRotateAround(Vector3(0,0,1), _angle);
}


// *** FastRotateAround
// ASSUMES that _norm is normalised
void Vector3::FastRotateAround(Vector3 const &_norm, float _angle)
{
	float dot = (*this) * _norm;
	Vector3 a = _norm * dot;
	Vector3 n1 = *this - a;
	Vector3 n2 = _norm ^ n1;
	float s = (float)sin( _angle );
	float c = (float)cos( _angle );

	*this = a + c*n1 + s*n2;
}


void Vector3::RotateAround(Vector3 const &_axis)
{
	float angle = _axis.MagSquared();
	if (angle < 1e-8) return;
	angle = sqrtf(angle);
	Vector3 norm(_axis / angle);
	FastRotateAround(norm, angle);
}


// *** Operator ==
bool Vector3::operator == (Vector3 const &b) const
{
	return Compare(b);
}


// *** Operator !=
bool Vector3::operator != (Vector3 const &b) const
{
	return !Compare(b);
}
