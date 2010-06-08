#include "lib/universal_include.h"

#include <math.h>

#include "math_utils.h"



float Log2(float x)
{
	static float oneOverLogOf2 = 1.0f / logf(2.0f);
	return logf(x) * oneOverLogOf2;
}


double RampUpAndDown(double _startTime, double _duration, double _timeNow)
{
    // Imagine that you have a stationary object that you want to move from
    // A to B in _duration. This function helps you do that with linear acceleration
    // to the midpoint, then linear deceleration to the end point. It returns the
    // fractional distance along the route. Used in the camera MoveToTarget routine.

	if (_timeNow > _startTime + _duration) return 1.0001f;

	double fractionalTime = (_timeNow - _startTime) / _duration;

	if (fractionalTime < 0.5)
	{
		double t = fractionalTime * 2.0;
		t *= t;
		t *= 0.5;
		return t;
	}
	else
	{
		double t = (1.0 - fractionalTime) * 2.0;
		t *= t;
		t *= 0.5;
		t = 1.0 - t;
		return t;
	}
}


void TruncateFloatPrecision( float &f )
{
    // See http://en.wikipedia.org/wiki/IEEE_754#Single-precision_32_bit [^]
    // Zero the last 3 bits 

    unsigned &x = (unsigned &) f;
    x &= ~0x0007;
}