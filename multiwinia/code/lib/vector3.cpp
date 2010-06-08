#include "lib/universal_include.h"

#include <math.h>
#include <float.h>

#include "lib/vector2.h"
#include "lib/vector3.h"
#include "lib/math_utils.h"
#include "lib/math/random_number.h"


Vector3 const g_upVector(0.0, 1.0, 0.0);
Vector3 const g_zeroVector(0.0, 0.0, 0.0);

double FromRand(const RandomFloat &frandWrapper)
{
#ifdef TRACK_SYNC_RAND
	return DebugSyncFrand((char *)frandWrapper.file, frandWrapper.line, 0.0, frandWrapper.v);
#else
	return _syncfrand(frandWrapper.v);
#endif
}

double FromRand(const SymmetricRandomFloat &sfrandWrapper)
{
#ifdef TRACK_SYNC_RAND
	double val = sfrandWrapper.v;
	return DebugSyncFrand((char *)sfrandWrapper.file, sfrandWrapper.line, -val/2.0, val/2.0);
#else
	return _syncsfrand(sfrandWrapper.v);
#endif
}

// ******************
//  Public Functions
// ******************

void Vector3::RotateAroundX(double _angle)
{
    FastRotateAround(Vector3(1,0,0), _angle);
}


void Vector3::RotateAroundY(double _angle)
{
    FastRotateAround(g_upVector, _angle);
}


void Vector3::RotateAroundZ(double _angle)
{
    FastRotateAround(Vector3(0,0,1), _angle);
}


// *** FastRotateAround
// ASSUMES that _norm is normalised
void Vector3::FastRotateAround(Vector3 const &_norm, double _angle)
{
	double dot = (*this) * _norm;
	Vector3 a = _norm * dot;
	Vector3 n1 = *this - a;
	Vector3 n2 = _norm ^ n1;
	double s = (double)iv_sin( _angle );
	double c = (double)iv_cos( _angle );

	*this = a + c*n1 + s*n2;
}


void Vector3::RotateAround(Vector3 const &_axis)
{
	double angle = _axis.MagSquared();
	if (angle < 1e-8) return;
	angle = iv_sqrt(angle);
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
