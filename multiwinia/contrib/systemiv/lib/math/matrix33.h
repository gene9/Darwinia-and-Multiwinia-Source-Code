#ifndef MATRIX33_H
#define MATRIX33_H

#include "lib/math/vector3.h"


class Matrix33
{
public:
    Vector3 f;
    Vector3 u;
    Vector3 r;

public:
	Matrix33 ();
	Matrix33 ( int ignored );
	Matrix33 ( Matrix33 const &other );
	Matrix33 ( Vector3 const &f, Vector3 const &u );
	Matrix33 ( float yaw, float dive, float roll );
	Matrix33 ( float rx, float ry, float rz, float ux, float uy, float uz, float fx, float fy, float fz );
	
	void OrientRU ( Vector3 const &r, Vector3 const &u );
	void OrientRF ( Vector3 const &r, Vector3 const &f );
	void OrientUF ( Vector3 const &u, Vector3 const &f );
	void OrientUR ( Vector3 const &u, Vector3 const &r );
	void OrientFR ( Vector3 const &f, Vector3 const &r );
	void OrientFU ( Vector3 const &f, Vector3 const &u );

	Matrix33 const &RotateAroundR    ( float angle );
	Matrix33 const &RotateAroundU    ( float angle );
	Matrix33 const &RotateAroundF    ( float angle );
	Matrix33 const &RotateAroundX    ( float angle );
	Matrix33 const &RotateAroundY    ( float angle );
	Matrix33 const &RotateAroundZ    ( float angle );
	Matrix33 const &RotateAround     ( Vector3 const &rotationAxis );
	Matrix33 const &RotateAround     ( Vector3 const &norm, float angle );
	Matrix33 const &FastRotateAround ( Vector3 const &norm, float angle );
	
    Matrix33 const &Transpose();
	Matrix33 const &Invert();
	
    
    Vector3     DecomposeToYDR          () const;	                                //x = dive, y = yaw, z = roll
	void        DecomposeToYDR          ( float *y, float *d, float *r ) const;
	void        SetToIdentity           ();
	void        Normalise               ();
	Vector3		InverseMultiplyVector   (Vector3 const &) const;
    float       *ConvertToOpenGLFormat  (Vector3 const *pos = NULL);

	void        OutputToDebugStream     ();

    //
	// Operators

	Matrix33 const &operator =  ( Matrix33 const &o );
    bool            operator == ( Matrix33 const &b ) const;            // Uses FloatEquals
	Matrix33 const &operator *= ( Matrix33 const &o );
	Matrix33		operator *  ( Matrix33 const &b ) const;
};


extern Matrix33 const g_identityMatrix33;



//
// ============================================================================


// Operator * between matrix33 and vector3
inline Vector3 operator * ( Matrix33 const &_m, Vector3 const &_v )
{
	return Vector3(_m.r.x * _v.x + _m.r.y * _v.y + _m.r.z * _v.z,
				   _m.u.x * _v.x + _m.u.y * _v.y + _m.u.z * _v.z,
				   _m.f.x * _v.x + _m.f.y * _v.y + _m.f.z * _v.z);
}


// Operator * between vector3 and matrix33
inline Vector3 operator * (	Vector3 const & _v, Matrix33 const &_m )
{
	return Vector3(_m.r.x * _v.x + _m.r.y * _v.y + _m.r.z * _v.z,
				   _m.u.x * _v.x + _m.u.y * _v.y + _m.u.z * _v.z,
				   _m.f.x * _v.x + _m.f.y * _v.y + _m.f.z * _v.z);
}


#endif

