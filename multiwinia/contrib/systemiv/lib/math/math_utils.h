#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <stdlib.h>

#ifndef M_PI
#define M_PI 3.1415926535897932384626f
#endif

#ifdef max
//#error Defining macros for max and min are a bad idea. To avoid this happening by accident, define NOMINMAX before inclusion of windows.h
#endif

#include <algorithm>

using std::min;
using std::max;

#define sign(a)				    ((a) < 0 ? -1 : 1) 
#define signf(a)			    ((a) < 0.0f ? -1.0f : 1.0f) 


#define Round(a)                ((int)(a))

#ifdef TARGET_MSVC
    #define round Round
#endif

template <class T>
inline void Clamp(T &a, T low, T high)
{
    if( a > high ) 
		a = high; 
    else if( a < low ) 
		a = low;
}

#define NearlyEquals(a, b)	    (fabsf((a) - (b)) < 1e-6 ? 1 : 0)

#define ASSERT_FLOAT_IS_SANE(x)         AppDebugAssert((x + 1.0f) != x);
#define ASSERT_VECTOR3_IS_SANE(v)   	ASSERT_FLOAT_IS_SANE((v).x) \
	                                    ASSERT_FLOAT_IS_SANE((v).y) \
	                                    ASSERT_FLOAT_IS_SANE((v).z)



float   Log2                (float x);

double  RampUpAndDown       (double _startTime, double _duration, double _timeNow);

void    TruncateFloatPrecision( float &f );

#endif

