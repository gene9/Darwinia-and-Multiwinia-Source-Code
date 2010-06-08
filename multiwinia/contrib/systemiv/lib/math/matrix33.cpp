#include "universal_include.h"

#include <math.h>

#include "matrix33.h"

#include "lib/debug_utils.h"


static float s_openGLFormat[16];

Matrix33 const g_identityMatrix33(0);


// ****************************************************************************
// Constructors
// ****************************************************************************

Matrix33::Matrix33()
{
}


Matrix33::Matrix33(int _ignored)
{
	SetToIdentity();
}


Matrix33::Matrix33( Matrix33 const &_other )
{
	memcpy(this, &_other, sizeof(Matrix33));
}


Matrix33::Matrix33( Vector3 const &_f, Vector3 const &_u )
:	f(_f),
	u(_u),
	r(_u ^ _f)
{
}


Matrix33::Matrix33( float _yaw, float _dive, float _roll )
{
	SetToIdentity();
	RotateAroundZ( _roll );
	RotateAroundX( _dive );
	RotateAroundY( _yaw );
}


Matrix33::Matrix33( float _rx, float _ry, float _rz, float _ux, float _uy, float _uz, float _fx, float _fy, float _fz )
:	f(_fx,_fy,_fz),
	u(_ux,_uy,_uz),
	r(_rx,_ry,_rz)	
{
}


// ****************************************************************************
// Functions
// ****************************************************************************

void Matrix33::OrientRU( Vector3 const & _r, Vector3 const & _u )
{
	Vector3 tr = _r;
	tr.Normalise();
	f = tr ^ _u;
	r = tr;
	f.Normalise();
	u = f ^ r;
}

void Matrix33::OrientRF( Vector3 const & _r, Vector3 const & _f )
{
	Vector3 tr = _r;
	tr.Normalise();
	u = _f ^ tr;
	r = tr;
	u.Normalise();
	f = r ^ u;
}

void Matrix33::OrientUF( Vector3 const & _u, Vector3 const & _f )
{
	Vector3 tu = _u;
	tu.Normalise();
	r = tu ^ _f;
	u = tu;
	r.Normalise();
	f = r ^ u;
}

void Matrix33::OrientUR( Vector3 const & _u, Vector3 const & _r )
{
	Vector3 tu = _u;
	tu.Normalise();
	f = _r ^ tu;
	u = tu;
	f.Normalise();
	r = u ^ f;
}

void Matrix33::OrientFR( Vector3 const & _f, Vector3 const & _r )
{
	Vector3 tf = _f;
	tf.Normalise();
	u = tf ^ _r;
	f = tf;
	u.Normalise();
	r = u ^ f;
}

void Matrix33::OrientFU( Vector3 const & _f, Vector3 const & _u )
{
	Vector3 tf = _f;
	tf.Normalise();
	r = _u ^ tf;
	f = tf;
	r.Normalise();
	u = f ^ r;
}

Matrix33 const &Matrix33::RotateAroundR( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t = u.x *  c + f.x * s;
	f.x = u.x * -s + f.x * c;
	u.x = t;

	t =	u.y * c + f.y * s;
	f.y = u.y * -s + f.y * c;
	u.y = t;

	t =	u.z * c + f.z * s;
	f.z = u.z * -s + f.z * c;
	u.z = t;

	return *this;
}

Matrix33 const &Matrix33::RotateAroundU( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t =	f.x * c + r.x * s;
	r.x = f.x * -s + r.x * c;
	f.x = t;

	t =	f.y * c + r.y * s;
	r.y = f.y * -s + r.y * c;
	f.y = t;

	t =	f.z * c + r.z * s;
	r.z = f.z * -s + r.z * c;
	f.z = t;

	return *this;
}

Matrix33 const &Matrix33::RotateAroundF( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t =	  r.x *  c + u.x * s;
	u.x = r.x * -s + u.x * c;
	r.x = t;

	t =	  r.y *  c + u.y * s;
	u.y = r.y * -s + u.y * c;
	r.y = t;

	t =	  r.z *  c + u.z * s;
	u.z = r.z * -s + u.z * c;
	r.z = t;

	return *this;
}

Matrix33 const &Matrix33::RotateAroundX( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t = r.y *  c + r.z * s;
	r.z = r.y * -s + r.z * c;
	r.y = t;

	t = u.y *  c + u.z * s;
	u.z = u.y * -s + u.z * c;
	u.y = t;

	t = f.y *  c + f.z * s;
	f.z = f.y * -s + f.z * c;
	f.y = t;

	return *this;
}

Matrix33 const &Matrix33::RotateAroundY( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t = r.z *  c + r.x * s;
	r.x = r.z * -s + r.x * c;
	r.z = t;

	t = u.z *  c + u.x * s;
	u.x = u.z * -s + u.x * c;
	u.z = t;

	t = f.z *  c + f.x * s;
	f.x = f.z * -s + f.x * c;
	f.z = t;

	return *this;
}

Matrix33 const &Matrix33::RotateAroundZ( float angle )
{
	float c = (float)cos( angle );
	float s = (float)sin( angle );

	float t;

	t = r.x *  c + r.y * s;
	r.y = r.x * -s + r.y * c;
	r.x = t;

	t = u.x *  c + u.y * s;
	u.y = u.x * -s + u.y * c;
	u.x = t;

	t = f.x *  c + f.y * s;
	f.y = f.x * -s + f.y * c;
	f.x = t;

	return *this;
}

// Mag of vector determines the amount to rotate by. This function is useful
// when you want to do a rotation based on the result of a cross product
Matrix33 const &Matrix33::RotateAround( Vector3 const &_rotationAxis )
{
	float magSquared = _rotationAxis.MagSquared();
	if (magSquared < 0.00001f * 0.00001f)
	{
		return *this;
	}

	float mag = sqrtf(magSquared);
	return FastRotateAround(_rotationAxis/mag, mag);
}

// Assumes normal is already normalised
Matrix33 const &Matrix33::FastRotateAround( Vector3 const & _norm, float _angle )
{
	float s = (float)sin(_angle);
	float c = (float)cos(_angle);

	float dot = r * _norm;
	Vector3 a = _norm * dot;
	Vector3 n1 = r - a;
	Vector3 n2 = _norm ^ n1;
	r = a + c*n1 + s*n2;

	dot = u * _norm;
	a = _norm * dot;
	n1 = u - a;
	n2 = _norm ^ n1;
	u = a + c*n1 + s*n2;

	dot = f * _norm;
	a = _norm * dot;
	n1 = f - a;
	n2 = _norm ^ n1;
	f = a + c*n1 + s*n2;

	return *this;
}

Matrix33 const &Matrix33::RotateAround( Vector3 const & _onorm, float _angle )
{
	Vector3 norm = _onorm;
	norm.Normalise();
	float s = (float)sin(_angle);
	float c = (float)cos(_angle);

	float dot = r * norm;
	Vector3 a = norm * dot;
	Vector3 n1 = r - a;
	Vector3 n2 = norm ^ n1;
	r = a + c*n1 + s*n2;

	dot = u * norm;
	a = norm * dot;
	n1 = u - a;
	n2 = norm ^ n1;
	u = a + c*n1 + s*n2;

	dot = f * norm;
	a = norm * dot;
	n1 = f - a;
	n2 = norm ^ n1;
	f = a + c*n1 + s*n2;

	return *this;
}

Matrix33 const &Matrix33::Transpose()
{
	float t = r.y;
	r.y = u.x;
	u.x = t;
	t = r.z;
	r.z = f.x;
	f.x = t;
	t = u.z;
	u.z = f.y;
	f.y = t;

	return *this;
}

Matrix33 const &Matrix33::Invert()
{
    // Note by Chris :
    // We could just call the general case InvertMatrix function here
    // But this method is much quicker in the 3x3 case

	// pick up values
	float a1 = r.x;
	float a2 = r.y;
	float a3 = r.z;
	float b1 = u.x;
	float b2 = u.y;
	float b3 = u.z;
	float c1 = f.x;
	float c2 = f.y;
	float c3 = f.z;

	// generate adjoint of matrix
	r.x = (b2*c3-c2*b3);
	u.x =-(b1*c3-c1*b3);
	f.x = (b1*c2-c1*b2);
	
	r.y =-(a2*c3-c2*a3);
	u.y = (a1*c3-c1*a3);
	f.y =-(a1*c2-c1*a2);

	r.z = (a2*b3-b2*a3);
	u.z =-(a1*b3-b1*a3);
	f.z = (a1*b2-b1*a2);

	// calculate determinant (adjoint is useful at this stage)
	float det = a1 * r.x + a2 * u.x + a3 * f.x;

	// scale result by 1/det
	det = 1.0f / det;
	r.x = r.x * det ;
	r.y = r.y * det ;
	r.z = r.z * det ;

	u.x = u.x * det ;
	u.y = u.y * det ;
	u.z = u.z * det ;

	f.x = f.x * det ;
	f.y = f.y * det ;
	f.z = f.z * det ;

	// generate determinant of matrix
	return *this;
}

Vector3 Matrix33::DecomposeToYDR() const
{
	//x = dive, y = yaw, z = roll
	float x,y,z;
	DecomposeToYDR( &x, &y, &z );
	return Vector3( x, y, z );
}

void Matrix33::DecomposeToYDR( float *_y, float *_d, float *_r ) const
{
	//work with a copy..
	Matrix33 workingMat( *this );

	//get the yaw
	*_y = -(float)atan2( workingMat.f.x, workingMat.f.z );

	//get the dive
	//	can't use asin( workingMat.f.y ) 'cos occasionally we get
	//	blasted precision problems. this sorts us out..
	*_d = (float)atan2( workingMat.f.y, workingMat.u.y );

	//rotate the matrix back by -yaw and -dive (one at a time)
	workingMat.RotateAroundY( -*_y );
	workingMat.RotateAroundX( -*_d );

	//now the roll
	*_r = -(float)atan2( workingMat.r.y, workingMat.r.x );
}

void Matrix33::SetToIdentity()
{
	memset(this, 0, sizeof(Matrix33));
	r.x = 1.0f;
	u.y = 1.0f;
	f.z = 1.0f;
}

void Matrix33::Normalise()
{
	f.Normalise();
	r = u ^ f;
	r.Normalise();
	u = f ^ r;
}

Vector3	Matrix33::InverseMultiplyVector(Vector3 const &s) const
{
	Vector3 const &v = s;
	return Vector3(r.x*v.x + u.x*v.y + f.x*v.z,
				   r.y*v.x + u.y*v.y + f.y*v.z,
				   r.z*v.x + u.z*v.y + f.z*v.z);
}

void Matrix33::OutputToDebugStream()
{
	AppDebugOut("%4.1f %4.1f %4.1f\n", r.x, r.y, r.z );	
	AppDebugOut("%4.1f %4.1f %4.1f\n", f.x, f.y, f.z );	
	AppDebugOut("%4.1f %4.1f %4.1f\n", u.x, u.y, u.z );	
}

float *Matrix33::ConvertToOpenGLFormat(Vector3 const *_pos)
{
	s_openGLFormat[0] = r.x;
	s_openGLFormat[1] = r.y;
	s_openGLFormat[2] = r.z;
	s_openGLFormat[3] = 0.0f;
	s_openGLFormat[4] = u.x;
	s_openGLFormat[5] = u.y;
	s_openGLFormat[6] = u.z;
	s_openGLFormat[7] = 0.0f;
	s_openGLFormat[8] = f.x;
	s_openGLFormat[9] = f.y;
	s_openGLFormat[10] = f.z;
	s_openGLFormat[11] = 0.0f;

	if (_pos)
	{
		s_openGLFormat[12] = _pos->x;
		s_openGLFormat[13] = _pos->y;
		s_openGLFormat[14] = _pos->z;
	}
	else
	{
		s_openGLFormat[12] = 0.0f;
		s_openGLFormat[13] = 0.0f;
		s_openGLFormat[14] = 0.0f;
	}
	s_openGLFormat[15] = 1.0f;

	return s_openGLFormat;
}


// ****************************************************************************
// Operators
// ****************************************************************************

Matrix33 const &Matrix33::operator = ( Matrix33 const &_o )
{
	memcpy(this, &_o, sizeof(Matrix33));
	return *this;
}


// Multiply this matrix by another and write the result back to ourself
Matrix33 const &Matrix33::operator *= ( Matrix33 const &_o )
{
	Matrix33 mat(
			r.x*_o.r.x + r.y*_o.u.x + r.z*_o.f.x,
			r.x*_o.r.y + r.y*_o.u.y + r.z*_o.f.y,
			r.x*_o.r.z + r.y*_o.u.z + r.z*_o.f.z,

			u.x*_o.r.x + u.y*_o.u.x + u.z*_o.f.x,
			u.x*_o.r.y + u.y*_o.u.y + u.z*_o.f.y,
			u.x*_o.r.z + u.y*_o.u.z + u.z*_o.f.z,

			f.x*_o.r.x + f.y*_o.u.x + f.z*_o.f.x,
			f.x*_o.r.y + f.y*_o.u.y + f.z*_o.f.y,
			f.x*_o.r.z + f.y*_o.u.z + f.z*_o.f.z);
	*this = mat;
	return *this;
}


// Multiply matrix by matrix
Matrix33 Matrix33::operator * (Matrix33 const &_o) const
{
	return Matrix33(
			r.x*_o.r.x + r.y*_o.u.x + r.z*_o.f.x,
			r.x*_o.r.y + r.y*_o.u.y + r.z*_o.f.y,
			r.x*_o.r.z + r.y*_o.u.z + r.z*_o.f.z,

			u.x*_o.r.x + u.y*_o.u.x + u.z*_o.f.x,
			u.x*_o.r.y + u.y*_o.u.y + u.z*_o.f.y,
			u.x*_o.r.z + u.y*_o.u.z + u.z*_o.f.z,

			f.x*_o.r.x + f.y*_o.u.x + f.z*_o.f.x,
			f.x*_o.r.y + f.y*_o.u.y + f.z*_o.f.y,
			f.x*_o.r.z + f.y*_o.u.z + f.z*_o.f.z);
}


bool Matrix33::operator == (Matrix33 const &_b) const
{
    if (r == _b.r &&
        u == _b.u &&
        f == _b.f )
    {
        return true;
    }

    return false;
}