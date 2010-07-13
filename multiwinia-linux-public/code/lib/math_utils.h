#ifndef MATHUTILS_H
#define MATHUTILS_H

#include <stdlib.h>

class Vector3;
class Vector2;
class Matrix34;
class Plane;

#ifdef M_PI
#undef M_PI
#endif
#define M_PI 3.1415926535897932384626

#define sign(a)				((a) < 0 ? -1 : 1) 
#define signf(a)			((a) < 0.0 ? -1.0 : 1.0) 


// Performs fast double to int conversion. It may round up or down, depending on
// the current CPU rounding mode - so DON'T USE IT if you want repeatable results
//#ifdef _DEBUG
	#define Round(a) (int)(a)
//#else
//	#define Round(a) RoundFunc(a)
//	inline int RoundFunc(double a) 
//	{
//		int retval;
//
//		__asm
//		{
//			fld a
//	//		fist retval
//			fistp retval
//		}
//
//		return retval;
//	}
//#endif


#define clamp(a, low, high)	if (a>high) a = high; else if (a<low) a = low;

#define NearlyEquals(a, b)	(fabs((a) - (b)) < 1e-6 ? 1 : 0)

double Log2(double x);

#define ASSERT_FLOAT_IS_SANE(x) \
	AppDebugAssert((x + 1.0) != x);
#define ASSERT_VECTOR3_IS_SANE(v) \
	ASSERT_FLOAT_IS_SANE((v).x) \
	ASSERT_FLOAT_IS_SANE((v).y) \
	ASSERT_FLOAT_IS_SANE((v).z)

// Imagine that you have a stationary object that you want to move from
// A to B in _duration. This function helps you do that with linear acceleration
// to the midpoint, then linear deceleration to the end point. It returns the
// fractional distance along the route. Used in the camera MoveToTarget routine.
double RampUpAndDown(double _startTime, double _duration, double _timeNow);

// nearestPowerOfTwo used for backwards compat. with older g-cards that don't
// support non-power-of-two textures.
bool RequirePowerOfTwoTextures();
bool RequireSquareTextures();
unsigned nearestPowerOfTwo( unsigned _x );


// **********************
// General Geometry Utils
// **********************

void GetPlaneMatrix(Vector3 const &t1, Vector3 const &t2, Vector3 const &t3, Matrix34 *mat);
double ProjectPointOntoPlane(Vector3 const &point, Matrix34 const &planeMat, Vector3 *result);
void ConvertWorldSpaceIntoPlaneSpace(Vector3 const &point, Matrix34 const &plane, Vector2 *result);
void ConvertPlaneSpaceIntoWorldSpace(Vector2 const &point, Matrix34 const &plane, Vector3 *result);
double CalcTriArea(Vector2 const &t1, Vector2 const &t2, Vector2 const &t3);


// *********************
// 2D Intersection Tests
// *********************

bool IsPointInTriangle(Vector2 const &pos, Vector2 const &t1, Vector2 const &t2, Vector2 const &t3);
double PointSegDist2D(Vector2 const &p,	// Point
				     Vector2 const &l0, Vector2 const &l1, // Line seg
				     Vector2 *result=NULL);
bool SegRayIntersection2D(Vector2 const &_lineStart, Vector2 const &_lineEnd,
						  Vector2 const &_rayStart, Vector2 const &_rayDir,
                          Vector2 *_result = NULL);

// *********************
// 3D Intersection Tests
// *********************

double RayRayDist(Vector3 const &a, Vector3 const &aDir,
				 Vector3 const &b, Vector3 const &bDir,
				 Vector3 *posOnA=NULL, Vector3 *posOnB=NULL);

double RaySegDist(Vector3 const &pointOnLine, Vector3 const &lineDir,
				 Vector3 const &segStart, Vector3 const &segEnd,
				 Vector3 *posOnRay=NULL, Vector3 *posInSeg=NULL);

bool RayTriIntersection         (Vector3 const &orig, Vector3 const &dir,
						         Vector3 const &vert0, Vector3 const &vert1, Vector3 const &vert2,
						         double _rayLen=1e10, Vector3 *result=NULL);

int RayPlaneIntersection(Vector3 const &pOnLine, Vector3 const &lineDir,
						 Plane const &plane, Vector3 *intersectionPoint=NULL);
//bool RayPlaneIntersection       (Vector3 const &rayStart, Vector3 const &rayDir,
//                                  Vector3 const &planePos, Vector3 const &planeNormal,
//						         double _rayLen=1e10, Vector3 *pos=NULL );

bool RaySphereIntersection      ( Vector3 const &rayStart, Vector3 const &rayDir, 
	                              Vector3 const &spherePos, double sphereRadius,
			                      double _rayLen=1e10, Vector3 *pos=NULL, Vector3 *normal=NULL );

bool SphereSphereIntersection   ( Vector3 const &_sphere1Pos, double _sphere1Radius,
                                  Vector3 const &_sphere2Pos, double _sphere2Radius );

bool SphereTriangleIntersection(Vector3 const &sphereCentre, double sphereRadius,
								Vector3 const &t1, Vector3 const &t2, Vector3 const &t3);

bool TriTriIntersection(Vector3 const &v0, Vector3 const &v1, Vector3 const &v2,
						Vector3 const &u0, Vector3 const &u1, Vector3 const &u2);

#endif

