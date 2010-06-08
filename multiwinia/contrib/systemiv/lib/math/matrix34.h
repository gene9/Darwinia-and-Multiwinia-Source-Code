#ifndef _INCLUDED_MATRIX34_H
#define _INCLUDED_MATRIX34_H


#include <stdlib.h>

#include "lib/math/matrix33.h"
#include "lib/math/vector3.h"



class Matrix34
{
public:
	Vector3 r;
    Vector3 u;
    Vector3 f;
    Vector3 pos;

public:
	Matrix34 ();
	Matrix34 ( int ignored );
	Matrix34 ( Matrix34 const &other );
	Matrix34 ( Matrix33 const &orientation, Vector3 const &pos );
	Matrix34 ( Vector3 const &f, Vector3 const &u, Vector3 const &pos );
	Matrix34 ( float yaw, float dive, float roll );

	void OrientRU ( Vector3 const &r, Vector3 const &u );
	void OrientRF ( Vector3 const &r, Vector3 const &f );
	void OrientUF ( Vector3 const &u, Vector3 const &f );
	void OrientUR ( Vector3 const &u, Vector3 const &r );
	void OrientFR ( Vector3 const &f, Vector3 const &r );
	void OrientFU ( Vector3 const &f, Vector3 const &u );

	Matrix34 const &RotateAroundR    ( float angle );
	Matrix34 const &RotateAroundU    ( float angle );
	Matrix34 const &RotateAroundF    ( float angle );
	Matrix34 const &RotateAroundX    ( float angle );
	Matrix34 const &RotateAroundY    ( float angle );
	Matrix34 const &RotateAroundZ    ( float angle );
	Matrix34 const &RotateAround     ( Vector3 const &rotationAxis );
	Matrix34 const &RotateAround     ( Vector3 const &norm, float angle );
	Matrix34 const &FastRotateAround ( Vector3 const &norm, float angle );
	
    Matrix34 const &Scale            ( float factor );

	Matrix34 const &Transpose();
    Matrix34 const &Invert();
	
	Vector3			DecomposeToYDR	        () const;	                                            //x = dive, y = yaw, z = roll
	void			DecomposeToYDR	        ( float *y, float *d, float *r ) const;
    float           *ConvertToOpenGLFormat  () const;
	void			SetToIdentity	        ();
    bool            IsNormalised            ();
    void			Normalise		        ();
    Matrix33        GetOr                   () const;
    Vector3			InverseMultiplyVector   (Vector3 const &) const;


    void            WriteToDebugStream();
    void            Test();

    //
	// Operators

	bool            operator == ( Matrix34 const &b ) const;		        // Uses FloatEquals
	Matrix34 const &operator *= ( Matrix34 const &o );
	Matrix34		operator *  ( Matrix34 const &b ) const;
};


extern Matrix34 const g_identityMatrix34;


//
// ============================================================================



// Operator * between matrix34 and vector3
inline Vector3 operator * ( Matrix34 const &m, Vector3 const &v )
{
	return Vector3(m.r.x*v.x + m.u.x*v.y + m.f.x*v.z + m.pos.x,
    			   m.r.y*v.x + m.u.y*v.y + m.f.y*v.z + m.pos.y,
				   m.r.z*v.x + m.u.z*v.y + m.f.z*v.z + m.pos.z);
}


// Operator * between vector3 and matrix34
inline Vector3 operator * ( Vector3 const &v, Matrix34 const &m )
{
	return Vector3(m.r.x*v.x + m.u.x*v.y + m.f.x*v.z + m.pos.x,
    			   m.r.y*v.x + m.u.y*v.y + m.f.y*v.z + m.pos.y,
				   m.r.z*v.x + m.u.z*v.y + m.f.z*v.z + m.pos.z);
}


#endif
