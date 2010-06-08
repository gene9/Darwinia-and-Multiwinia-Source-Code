#include "lib/universal_include.h"

#ifdef USE_DIRECT3D
#include "lib/opengl_trace.h"

const GLubyte * glGetString (GLenum name)
{
	GL_TRACE("glGetString(%s)", glEnumToString(name))
	return 0;
}

void glDeleteTextures (GLsizei n, const GLuint *textures)
{
	GL_TRACE("glDeleteTextures(%d, (const unsigned *)%p)", n, textures)
}

void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	GL_TRACE("glFrustum(%10.4g, %10.4g, %10.4g, %10.4g, %10.4g, %10.4g)", left, right, bottom, top, zNear, zFar)
}

void glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params)
{
	GL_TRACE("glGetMaterialfv(%s, %s, (float *)%p)", glEnumToString(face), glEnumToString(pname), params)
}

void glEnableClientState (GLenum array)
{
	GL_TRACE("glEnableClientState(%s)", glEnumToString(array))
}

void glDisableClientState (GLenum array)
{
	GL_TRACE("glDisableClientState(%s)", glEnumToString(array))
}

void glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_TRACE("glVertexPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
}

void glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_TRACE("glColorPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
}

void glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_TRACE("glNormalPointer(%s, %d, %p)", glEnumToString(type), stride, pointer)
}

void glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	GL_TRACE("glTexCoordPointer(%d, %s, %d, %p)", size, glEnumToString(type), stride, pointer)
}

void glDrawArrays (GLenum mode, GLint first, GLsizei count)
{
	GL_TRACE("glDrawArrays(%s, %d, %d)", glEnumToString(mode), first, count)
}

#endif // USE_DIRECT3D
