#ifndef _INCLUDED_MATRIX34_H
#define _INCLUDED_MATRIX34_H


//#include <string.h>
#include <stdlib.h>

#include "lib/matrix33.h"
#include "lib/vector3.h"


class Matrix33;


class Matrix34
{
public:
	Vector3 r, u, f, pos;

	static float m_openGLFormat[16];

	// Constructors
	Matrix34() {}

	Matrix34(int _ignored)
	{
		SetToIdentity();
	}


	Matrix34( Matrix34 const &_other )
	{
		memcpy(this, &_other, sizeof(Matrix34));
	}


	Matrix34( Matrix33 const &_or, Vector3 const &_pos )
	:	r(_or.r),
		u(_or.u),
		f(_or.f),
		pos(_pos)
	{
	}


	Matrix34( Vector3 const &_f, Vector3 const &_u, Vector3 const &_pos )
	:	f(_f),
		pos(_pos)
	{
		r = _u ^ f;
		r.Normalise();
		u = f ^ r;
		u.Normalise();
	}


	Matrix34( float _yaw, float _dive, float _roll )
	{
		SetToIdentity();
		RotateAroundZ( _roll );
		RotateAroundX( _dive );
		RotateAroundY( _yaw );
	}

	void OrientRU( Vector3 const & _r, Vector3 const & _u );
	void OrientRF( Vector3 const & _r, Vector3 const & _f );
	void OrientUF( Vector3 const & _u, Vector3 const & _f );
	void OrientUR( Vector3 const & _u, Vector3 const & _r );
	void OrientFR( Vector3 const & _f, Vector3 const & _r );
	void OrientFU( Vector3 const & _f, Vector3 const & _u );

	Matrix34 const &RotateAroundR( float angle );
	Matrix34 const &RotateAroundU( float angle );
	Matrix34 const &RotateAroundF( float angle );
	Matrix34 const &RotateAroundX( float angle );
	Matrix34 const &RotateAroundY( float angle );
	Matrix34 const &RotateAroundZ( float angle );
	Matrix34 const &RotateAround( Vector3 const & _rotationAxis );
	Matrix34 const &RotateAround( Vector3 const & _norm, float _angle );
	Matrix34 const &FastRotateAround( Vector3 const & _norm, float _angle );
	
	Matrix34 const &Transpose();
	
	Vector3			DecomposeToYDR	() const;	//x = dive, y = yaw, z = roll
	void			DecomposeToYDR	( float *_y, float *_d, float *_r ) const;
	void			SetToIdentity	();

    bool            IsNormalised    ();
    void			Normalise		();

	float *ConvertToOpenGLFormat() const
	{
		m_openGLFormat[0] = r.x;
		m_openGLFormat[1] = r.y;
		m_openGLFormat[2] = r.z;
		m_openGLFormat[3] = 0.0f;
		m_openGLFormat[4] = u.x;
		m_openGLFormat[5] = u.y;
		m_openGLFormat[6] = u.z;
		m_openGLFormat[7] = 0.0f;
		m_openGLFormat[8] = f.x;
		m_openGLFormat[9] = f.y;
		m_openGLFormat[10] = f.z;
		m_openGLFormat[11] = 0.0f;
		m_openGLFormat[12] = pos.x;
		m_openGLFormat[13] = pos.y;
		m_openGLFormat[14] = pos.z;
		m_openGLFormat[15] = 1.0f;

		return m_openGLFormat;
	}

	Matrix33 GetOr() const
	{
		return Matrix33(r, u, f);
	}

	Vector3			InverseMultiplyVector(Vector3 const &) const;
    void            WriteToDebugStream();

    void            Test();

	// Operators
	bool operator == (Matrix34 const &b) const;		// Uses FLT_EPSILON
//	Matrix34 const &operator =  ( Matrix34 const &_o );
//	Matrix34 const &Matrix34::operator = ( Matrix34 const &_o )
//	{
//		memcpy(this, &_o, sizeof(Matrix34));
//		return *this;
//	}


	Matrix34 const &operator *= ( Matrix34 const &_o );
	Matrix34		operator *  ( Matrix34 const &b ) const
	{
		return Matrix34(*this) *= b;
	}
};


extern Matrix34 const g_identityMatrix34;


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
