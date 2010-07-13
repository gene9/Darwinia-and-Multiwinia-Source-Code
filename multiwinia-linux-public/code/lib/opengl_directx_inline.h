// COLORS

#include "opengl_trace.h"

#ifdef TARGET_MSVC
#	define INLINE __forceinline
#else
#	define INLINE inline
#endif

void glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );

INLINE
void glColor4f ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
{
	GL_TRACE_IMP(" glColor4f(%10.4g, %10.4g, %10.4g, %10.4g)", (double) red, (double) green, (double) blue, (double) alpha)

	#define CLAMPF2B(x) ((x<0)?0:((x>1)?255:(unsigned char)(x*255)))
	glColor4ub(CLAMPF2B(red),CLAMPF2B(green),CLAMPF2B(blue),CLAMPF2B(alpha));
	#undef CLAMPF2B
}

INLINE
void glColor4fv ( const GLfloat *v )
{
	glColor4f( v[0], v[1], v[2], v[3] );
}

INLINE
void glColor3f ( GLfloat red, GLfloat green, GLfloat blue )
{
	GL_TRACE_IMP(" glColor3f(%10.4g, %10.4g, %10.4g)", (double) red, (double) green, (double) blue)

	glColor4f( red, green, blue, 1.0f );
}

INLINE
void glColor3ub ( GLubyte red, GLubyte green, GLubyte blue )
{
	GL_TRACE_IMP(" glColor3ub(0x%02x, 0x%02x, 0x%02x)", red, green, blue)

	glColor4ub( red, green, blue, 255 );
}

INLINE
void glColor3ubv ( const GLubyte *v )
{
	GL_TRACE_IMP(" glColor3ubv((const unsigned char *)%p)", v)

	glColor4ub( v[0], v[1], v[2], 255 );
}

INLINE
void glColor4ubv (const GLubyte *v)
{
	GL_TRACE_IMP(" glColor4ubv((const unsigned char *)%p)", glArrayToString(v, 3))

	glColor4ub( v[0], v[1], v[2], v[3] );
}

// NORMALS

void glNormal3f_impl (GLfloat nx, GLfloat ny, GLfloat nz);

INLINE
void glNormal3dv (const GLdouble *v)
{
	GL_TRACE_IMP(" glNormal3dv(%s)", glArrayToString(v, 3));

	glNormal3f_impl( v[0], v[1], v[2] );
}

INLINE
void glNormal3fv (const GLfloat *v)
{
	GL_TRACE_IMP(" glNormal3fv(%s)", glArrayToString(v, 3));

	glNormal3f_impl( v[0], v[1], v[2] );
}

INLINE
void glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz)
{
	GL_TRACE_IMP(" glNormal3f(%10.4g, %10.4g, %10.4g)", (double) nx, (double) ny, (double) nz)

	glNormal3f_impl( nx, ny, nz );
}

// VERTICES

void glVertex3f_impl ( GLfloat x, GLfloat y, GLfloat z );

INLINE
void glVertex3dv (const GLdouble *v)
{
	GL_TRACE_IMP(" glVertex3dv(%s)", glArrayToString(v, 3))

	glVertex3f_impl ( v[0], v[1], v[2] );
}


INLINE
void glVertex3fv (const GLfloat *v)
{
	GL_TRACE_IMP(" glVertex3fv(%s)", glArrayToString(v, 3))

	glVertex3f_impl ( v[0], v[1], v[2] );
}

INLINE
void glVertex3d (GLdouble x, GLdouble y, GLdouble z)
{
	GL_TRACE_IMP(" glVertex3d(%10.4g, %10.4g, %10.4g)", x, y, z)

	glVertex3f_impl ( x, y, z );
}

INLINE
void glVertex3f (GLfloat x, GLfloat y, GLfloat z)
{
	GL_TRACE_IMP(" glVertex3f(%10.4g, %10.4g, %10.4g)", (double) x, (double) y, (double) z)

	glVertex3f_impl ( x, y, z );
}

INLINE
void glVertex2dv (const GLdouble *v)
{
	GL_TRACE_IMP(" glVertex2dv((const double *)%p)", v)

	glVertex3f_impl ( v[0], v[1], 0.0f );
}

INLINE
void glVertex2fv (const GLfloat *v)
{
	GL_TRACE_IMP(" glVertex2fv((const float *)%p)", v)

	glVertex3f_impl ( v[0], v[1], 0.0f );
}

INLINE
void glVertex2f ( GLfloat x, GLfloat y )
{
	GL_TRACE_IMP(" glVertex2f(%10.4g, %10.4g)", (double) x, (double) y)

	glVertex3f_impl ( x, y, 0.0f );
}

INLINE
void glVertex2i ( GLint x, GLint y )
{
	GL_TRACE_IMP(" glVertex2i(%d, %d)", x, y)

	glVertex3f_impl ( (float) x, (float) y, 0.0f );
}

// TEXTURE COORDS

void glTexCoord2f_impl ( GLfloat u, GLfloat v );


INLINE
void glTexCoord2f ( GLfloat s, GLfloat t )
{
	GL_TRACE_IMP(" glTexCoord2f(%10.4g, %10.4g)", (double) s, (double) t)
	glTexCoord2f_impl ( s, t );
}

INLINE
void glTexCoord2dv( GLdouble *v )
{
	glTexCoord2f_impl ( v[0], v[1] );
}

INLINE
void glTexCoord2fv( GLfloat *v )
{
	glTexCoord2f_impl ( v[0], v[1] );
}

INLINE
void glTexCoord2i (GLint s, GLint t)
{
	GL_TRACE_IMP(" glTexCoord2i(%d, %d)", s, t)
	glTexCoord2f_impl ( (float) s, (float) t );
}

// FOG

void glFogf_impl (GLenum pname, GLfloat param);
void glFogi_impl (GLenum pname, GLint param);

INLINE
void glFogf (GLenum pname, GLfloat param)
{
	GL_TRACE_IMP(" glFogf(%s, %10.4g)", glEnumToString(pname), (double) param)

	glFogf_impl (pname, param);
}

INLINE
void glFogi(GLenum pname, GLint param)
{
	GL_TRACE_IMP(" glFogi(%s, %d)", glEnumToString(pname), param)

	glFogi_impl( pname, param );
}

template <class DoubleFloat>
INLINE
void openGLMatrixToDirect3D( const DoubleFloat *_in, D3DMATRIX &_out )
{
	// See comment in direct3DMatrixToOpenGL

	_out._11 = _in[0];
	_out._12 = _in[1];
	_out._13 = _in[2];
	_out._14 = _in[3];

	_out._21 = _in[4];
	_out._22 = _in[5];
	_out._23 = _in[6];
	_out._24 = _in[7];

	_out._31 = _in[8];
	_out._32 = _in[9];
	_out._33 = _in[10];
	_out._34 = _in[11];

	_out._41 = _in[12];
	_out._42 = _in[13];
	_out._43 = _in[14];
	_out._44 = _in[15];
}