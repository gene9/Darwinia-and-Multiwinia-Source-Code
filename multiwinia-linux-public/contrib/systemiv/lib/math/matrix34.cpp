#include "universal_include.h"

#include <math.h>

#include "matrix33.h"
#include "matrix34.h"
#include "invert_matrix.h"

#include "lib/debug_utils.h"


static float s_openGLFormat[16];

Matrix34 const g_identityMatrix34(0);


Matrix34::Matrix34()
{
    SetToIdentity();
}


Matrix34::Matrix34(int _ignored)
{
    SetToIdentity();
}


Matrix34::Matrix34( Matrix34 const &_other )
{
    memcpy(this, &_other, sizeof(Matrix34));
}


Matrix34::Matrix34( Matrix33 const &_or, Vector3 const &_pos )
:	r(_or.r),
    u(_or.u),
    f(_or.f),
    pos(_pos)
{
}


Matrix34::Matrix34( Vector3 const &_f, Vector3 const &_u, Vector3 const &_pos )
:	f(_f),
    pos(_pos)
{
    r = _u ^ f;
    r.Normalise();
    u = f ^ r;
    u.Normalise();
}


Matrix34::Matrix34( float _yaw, float _dive, float _roll )
{
    SetToIdentity();
    RotateAroundZ( _roll );
    RotateAroundX( _dive );
    RotateAroundY( _yaw );
}


// ****************************************************************************
// Functions
// ****************************************************************************

void Matrix34::OrientRU( Vector3 const & _r, Vector3 const & _u )
{
	Vector3 tr = _r;
	tr.Normalise();
	f = tr ^ _u;
	r = tr;
	f.Normalise();
	u = f ^ r;
}

void Matrix34::OrientRF( Vector3 const & _r, Vector3 const & _f )
{
	Vector3 tr = _r;
	tr.Normalise();
	u = _f ^ tr;
	r = tr;
	u.Normalise();
	f = r ^ u;
}

void Matrix34::OrientUF( Vector3 const & _u, Vector3 const & _f )
{
	Vector3 tu = _u;
	tu.Normalise();
	r = tu ^ _f;
	u = tu;
	r.Normalise();
	f = r ^ u;
}

void Matrix34::OrientUR( Vector3 const & _u, Vector3 const & _r )
{
	Vector3 tu = _u;
	tu.Normalise();
	f = _r ^ tu;
	u = tu;
	f.Normalise();
	r = u ^ f;
}

void Matrix34::OrientFR( Vector3 const & _f, Vector3 const & _r )
{
	Vector3 tf = _f;
	tf.Normalise();
	u = tf ^ _r;
	f = tf;
	u.Normalise();
	r = u ^ f;
}

void Matrix34::OrientFU( Vector3 const & _f, Vector3 const & _u )
{
	Vector3 tf = _f;
	tf.Normalise();
	r = _u ^ tf;
	f = tf;
	r.Normalise();
	u = f ^ r;
}

Matrix34 const &Matrix34::RotateAroundR( float angle )
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

Matrix34 const &Matrix34::RotateAroundU( float angle )
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

Matrix34 const &Matrix34::RotateAroundF( float angle )
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

Matrix34 const &Matrix34::RotateAroundX( float angle )
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

Matrix34 const &Matrix34::RotateAroundY( float angle )
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

Matrix34 const &Matrix34::RotateAroundZ( float angle )
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
Matrix34 const &Matrix34::RotateAround( Vector3 const &_rotationAxis )
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
Matrix34 const &Matrix34::FastRotateAround( Vector3 const & _norm, float _angle )
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


Matrix34 const &Matrix34::Scale( float factor )
{
    r *= factor;
    f *= factor;
    u *= factor;

    return *this;
}


Matrix34 const &Matrix34::RotateAround( Vector3 const & _onorm, float _angle )
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

Matrix34 const &Matrix34::Transpose()
{
	pos = *this * -pos;

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


Matrix34 const &Matrix34::Invert()
{
    // Use the general case InvertMatrix function
    // Note: Could almost certainly be optimised by writing
    // a dedicated 4x4 inverter

    double matrixIn[16];
    double matrixOut[16];

    matrixIn[0] = r.x;
    matrixIn[1] = r.y;
    matrixIn[2] = r.z;
    matrixIn[3] = 0;
    matrixIn[4] = u.x;
    matrixIn[5] = u.y;
    matrixIn[6] = u.z;
    matrixIn[7] = 0;
    matrixIn[8] = f.x;
    matrixIn[9] = f.y;
    matrixIn[10] = f.z;
    matrixIn[11] = 0;
    matrixIn[12] = pos.x;
    matrixIn[13] = pos.y;
    matrixIn[14] = pos.z;
    matrixIn[15] = 1;

    InvertMatrix( (double *)&matrixIn, (double *)&matrixOut, 4, 4 );

    r.x = matrixOut[0];
    r.y = matrixOut[1];
    r.z = matrixOut[2];
    u.x = matrixOut[4];
    u.y = matrixOut[5];
    u.z = matrixOut[6];
    f.x = matrixOut[8];
    f.y = matrixOut[9];
    f.z = matrixOut[10];
    pos.x = matrixOut[12];
    pos.y = matrixOut[13];
    pos.z = matrixOut[14];

    return *this;
}


Vector3 Matrix34::DecomposeToYDR() const
{
	//x = dive, y = yaw, z = roll
	float x,y,z;
	DecomposeToYDR( &x, &y, &z );
	return Vector3( x, y, z );
}

void Matrix34::DecomposeToYDR( float *_y, float *_d, float *_r ) const
{
	//work with a copy..
	Matrix34 workingMat( *this );

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

void Matrix34::SetToIdentity()
{
	memset(this, 0, sizeof(Matrix34));
	r.x = 1.0f;
	u.y = 1.0f;
	f.z = 1.0f;
}

bool Matrix34::IsNormalised()
{
    float lenR = r.MagSquared();
    if (lenR < 0.999f || lenR > 1.001f) return false;
    float lenU = u.MagSquared();
    if (lenU < 0.999f || lenU > 1.001f) return false;
    float lenF = f.MagSquared();
    if (lenF < 0.999f || lenF > 1.001f) return false;

    if (fabsf(r * u) > 0.001f) return false;
    if (fabsf(r * f) > 0.001f) return false;
    if (fabsf(f * u) > 0.001f) return false;

    return true;
}

void Matrix34::Normalise()
{
	f.Normalise();
	r = u ^ f;
	r.Normalise();
	u = f ^ r;
}


Vector3	Matrix34::InverseMultiplyVector(Vector3 const &s) const
{
	Vector3 v = s - pos;
	return Vector3(r.x*v.x + u.x*v.y + f.x*v.z,
				   r.y*v.x + u.y*v.y + f.y*v.z,
				   r.z*v.x + u.z*v.y + f.z*v.z);
}


void Matrix34::WriteToDebugStream()
{
    AppDebugOut("%5.2f %5.2f %5.2f\n", r.x, r.y, r.z);
    AppDebugOut("%5.2f %5.2f %5.2f\n", u.x, u.y, u.z);
    AppDebugOut("%5.2f %5.2f %5.2f\n", f.x, f.y, f.z);
    AppDebugOut("%5.2f %5.2f %5.2f\n\n", pos.x, pos.y, pos.z);
}


void Matrix34::Test()
{
    Matrix34 a(Vector3(0,0,1), g_upVector, g_zeroVector);
    Matrix34 b(Vector3(0,0,1), g_upVector, g_zeroVector);
    Matrix34 c=a*b;
    AppDebugOut("c = a * b\n");
    c.WriteToDebugStream();

    Vector3 front(10,20,2);
    front.Normalise();
    Vector3 up(0,1,0);
    Vector3 right = up ^ front;
    right.Normalise();
    up = front ^ right;
    up.Normalise();
    Matrix34 d(front, up, Vector3(-1,2,-3));
    AppDebugOut("d = \n");
    d.WriteToDebugStream();

    Matrix34 e = d * d;
    AppDebugOut("e = d * d\n");
    e.WriteToDebugStream();
}


float *Matrix34::ConvertToOpenGLFormat() const
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
    s_openGLFormat[12] = pos.x;
    s_openGLFormat[13] = pos.y;
    s_openGLFormat[14] = pos.z;
    s_openGLFormat[15] = 1.0f;

    return s_openGLFormat;
}


Matrix33 Matrix34::GetOr() const
{
    Matrix33 orientation;
    orientation.f = f;
    orientation.r = r;
    orientation.u = u;

    return orientation;
}


// ****************************************************************************
// Operators
// ****************************************************************************

bool Matrix34::operator == (Matrix34 const &_b) const
{
	if (r == _b.r &&
		u == _b.u &&
		f == _b.f &&
		pos == _b.pos)
	{
		return true;
	}

	return false;
}


// Multiply this matrix by another and write the result back to ourself
Matrix34 const &Matrix34::operator *= ( Matrix34 const &o )
{
	Matrix34 mat;

	mat.r.x  =  r.x * o.r.x  +  r.y * o.u.x  +  r.z * o.f.x;
	mat.r.y  =  r.x * o.r.y  +  r.y * o.u.y  +  r.z * o.f.y;
	mat.r.z  =  r.x * o.r.z  +  r.y * o.u.z  +  r.z * o.f.z;

	mat.u.x  =  u.x * o.r.x  +  u.y * o.u.x  +  u.z * o.f.x;
	mat.u.y  =  u.x * o.r.y  +  u.y * o.u.y  +  u.z * o.f.y;
	mat.u.z  =  u.x * o.r.z  +  u.y * o.u.z  +  u.z * o.f.z;

	mat.f.x  =  f.x * o.r.x  +  f.y * o.u.x  +  f.z * o.f.x;
	mat.f.y  =  f.x * o.r.y  +  f.y * o.u.y  +  f.z * o.f.y;
	mat.f.z  =  f.x * o.r.z  +  f.y * o.u.z  +  f.z * o.f.z;

	mat.pos.x  =  pos.x * o.r.x  +  pos.y * o.u.x  +  pos.z * o.f.x  +  o.pos.x;
	mat.pos.y  =  pos.x * o.r.y  +  pos.y * o.u.y  +  pos.z * o.f.y  +  o.pos.y;
	mat.pos.z  =  pos.x * o.r.z  +  pos.y * o.u.z  +  pos.z * o.f.z  +  o.pos.z;

	*this = mat;
	return *this;
}


Matrix34 Matrix34::operator *  ( Matrix34 const &b ) const
{
    return Matrix34(*this) *= b;
}
