#ifndef __OPENGL_TRACE_H
#define __OPENGL_TRACE_H

#ifdef _DEBUG
void glTrace(const char *fmt, ...);
const char *glEnumToString(GLenum id);
const char *glBooleanToString( bool value );
template <class T> const char *glArrayToString(const T *x, int length);
void glBitfieldToString1(char *buffer, GLbitfield &f, GLbitfield mask, const char *name);
const char *glBitfieldToString( GLbitfield f );

extern bool g_tracingOpenGL;

#define GL_TRACE_IMP(...) {}
// { if (g_tracingOpenGL) { glTrace(__VA_ARGS__); } else {} }
#define GL_TRACE(...) { if (g_tracingOpenGL) { glTrace(__VA_ARGS__); } else {} }
#else
#define GL_TRACE(...)		{}
#define GL_TRACE_IMP(...)	{}
#endif

#endif // OPENGL_TRACE_H
