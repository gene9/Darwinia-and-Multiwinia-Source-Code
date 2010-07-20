#ifndef MATRIX33_H
#define MATRIX33_H


#include "vector3.h"
#include "stdlib.h"	// For "NULL"

class Matrix33
{
public:
	Vector3 r, u, f;

	static float m_openGLFormat[16];

	// Constructors
	Matrix33();
	Matrix33( int _ignored );
	Matrix33( Matrix33 const &_other );
	Matrix33( Vector3 const &_r, Vector3 const &_u, Vector3 const &_f );
	Matrix33( float _yaw, float _dive, float _roll );
	inline Matrix33( float _rx, float _ry, float _rz, float _ux, float _uy, float _uz, float _fx, float _fy, float _fz );

	void OrientRU( Vector3 const & _r, Vector3 const & _u );
	void OrientRF( Vector3 const & _r, Vector3 const & _f );
	void OrientUF( Vector3 const & _u, Vector3 const & _f );
	void OrientUR( Vector3 const & _u, Vector3 const & _r );
	void OrientFR( Vector3 const & _f, Vector3 const & _r );
	void OrientFU( Vector3 const & _f, Vector3 const & _u );
	Matrix33 const &RotateAroundR( float angle );
	Matrix33 const &RotateAroundU( float angle );
	Matrix33 const &RotateAroundF( float angle );
	Matrix33 const &RotateAroundX( float angle );
	Matrix33 const &RotateAroundY( float angle );
	Matrix33 const &RotateAroundZ( float angle );
	Matrix33 const &RotateAround( Vector3 const & _rotationAxis );
	Matrix33 const &RotateAround( Vector3 const & _norm, float _angle );
	Matrix33 const &FastRotateAround( Vector3 const & _norm, float _angle );
	Matrix33 const &Transpose();
	Matrix33 const &Invert();
	Vector3 DecomposeToYDR() const;	//x = dive, y = yaw, z = roll
	void DecomposeToYDR( float *_y, float *_d, float *_r ) const;
	void SetToIdentity();
	void Normalise();

	Vector3			InverseMultiplyVector(Vector3 const &) const;

	void OutputToDebugStream();
	float *ConvertToOpenGLFormat(Vector3 const *_pos = NULL);

	// Operators
	Matrix33 const &operator =  ( Matrix33 const &_o );
	Matrix33 const &operator *= ( Matrix33 const &_o );
	Matrix33		operator *  (Matrix33 const &b) const;
};


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

