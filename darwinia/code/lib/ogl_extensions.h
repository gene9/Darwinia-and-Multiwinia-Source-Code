#ifndef _included_ogl_extensions_h
#define _included_ogl_extensions_h


#include <stddef.h>


// Extension 1: GL_ARB_multitexture
#define GL_COMBINE_EXT                    0x8570
#define GL_COMBINE_RGB_EXT                0x8571
#define GL_COMBINE_ALPHA_EXT              0x8572
#define GL_ADD_SIGNED_EXT                 0x8574
#define GL_INTERPOLATE_EXT                0x8575
#define GL_CONSTANT_EXT                   0x8576
#define GL_SOURCE0_ALPHA_ARB              0x8588
#define GL_TEXTURE0_ARB                   0x84C0
#define GL_TEXTURE1_ARB                   0x84C1
typedef void (__stdcall *MultiTexCoord2fARB) (int, float, float);
typedef void (__stdcall *ActiveTextureARB) (int);
extern MultiTexCoord2fARB gglMultiTexCoord2fARB;
extern ActiveTextureARB gglActiveTextureARB;

// Extension 11: WGL_ARB_pbuffer
#define WGL_DRAW_TO_PBUFFER_ARB		   0x202D
#define WGL_DRAW_TO_PBUFFER_ARB		   0x202D
#define WGL_MAX_PBUFFER_PIXELS_ARB	   0x202E
#define WGL_MAX_PBUFFER_WIDTH_ARB	   0x202F
#define WGL_MAX_PBUFFER_HEIGHT_ARB	   0x2030
#define WGL_PBUFFER_LARGEST_ARB		   0x2033
#define WGL_PBUFFER_WIDTH_ARB		   0x2034
#define WGL_PBUFFER_HEIGHT_ARB		   0x2035
#define WGL_PBUFFER_LOST_ARB		   0x2036
//DECLARE_HANDLE(HPBUFFERARB);
//typedef HPBUFFERARB glCreatePbufferARB(HDC hDC,
//				int iPixelFormat,
//				int iWidth,
//				int iHeight,
//				const int *piAttribList);
//HDC wglGetPbufferDCARB(HPBUFFERARB hPbuffer);
//int wglReleasePbufferDCARB(HPBUFFERARB hPbuffer, HDC hDC);
//BOOL wglDestroyPbufferARB(HPBUFFERARB hPbuffer);
//BOOL wglQueryPbufferARB(HPBUFFERARB hPbuffer,	int iAttribute,	int *piValue);


// Extension 28: GL_ARB_vertex_buffer_object
#define GL_ARRAY_BUFFER_ARB                             0x8892
#define GL_ELEMENT_ARRAY_BUFFER_ARB                     0x8893

#define GL_ARRAY_BUFFER_BINDING_ARB                     0x8894
#define GL_ELEMENT_ARRAY_BUFFER_BINDING_ARB             0x8895
#define GL_VERTEX_ARRAY_BUFFER_BINDING_ARB              0x8896
#define GL_NORMAL_ARRAY_BUFFER_BINDING_ARB              0x8897
#define GL_COLOR_ARRAY_BUFFER_BINDING_ARB               0x8898
#define GL_INDEX_ARRAY_BUFFER_BINDING_ARB               0x8899
#define GL_TEXTURE_COORD_ARRAY_BUFFER_BINDING_ARB       0x889A
#define GL_EDGE_FLAG_ARRAY_BUFFER_BINDING_ARB           0x889B
#define GL_SECONDARY_COLOR_ARRAY_BUFFER_BINDING_ARB     0x889C
#define GL_FOG_COORDINATE_ARRAY_BUFFER_BINDING_ARB      0x889D
#define GL_WEIGHT_ARRAY_BUFFER_BINDING_ARB              0x889E

#define GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING_ARB       0x889F

#define GL_STREAM_DRAW_ARB                              0x88E0
#define GL_STREAM_READ_ARB                              0x88E1
#define GL_STREAM_COPY_ARB                              0x88E2
#define GL_STATIC_DRAW_ARB                              0x88E4
#define GL_STATIC_READ_ARB                              0x88E5
#define GL_STATIC_COPY_ARB                              0x88E6
#define GL_DYNAMIC_DRAW_ARB                             0x88E8
#define GL_DYNAMIC_READ_ARB                             0x88E9
#define GL_DYNAMIC_COPY_ARB                             0x88EA

#define GL_READ_ONLY_ARB                                0x88B8
#define GL_WRITE_ONLY_ARB                               0x88B9
#define GL_READ_WRITE_ARB                               0x88BA

#define GL_BUFFER_SIZE_ARB                              0x8764
#define GL_BUFFER_USAGE_ARB                             0x8765
#define GL_BUFFER_ACCESS_ARB                            0x88BB
#define GL_BUFFER_MAPPED_ARB                            0x88BC

#define GL_BUFFER_MAP_POINTER_ARB                       0x88BD

typedef ptrdiff_t GLintptrARB;
typedef ptrdiff_t GLsizeiptrARB;

typedef void (__stdcall *glBindBufferARB) (GLenum, GLuint);
typedef void (__stdcall *glDeleteBuffersARB) (GLsizei, const GLuint *);
typedef void (__stdcall *glGenBuffersARB) (GLsizei, GLuint *);
typedef GLboolean (__stdcall *glIsBufferARB) (GLuint);
typedef void (__stdcall *glBufferDataARB) (GLenum, GLsizeiptrARB, const GLvoid *, GLenum);
typedef void (__stdcall *glBufferSubDataARB) (GLenum, GLintptrARB, GLsizeiptrARB, const GLvoid *);
typedef void (__stdcall *glGetBufferSubDataARB) (GLenum, GLintptrARB, GLsizeiptrARB, GLvoid *);
typedef GLvoid* (__stdcall *glMapBufferARB) (GLenum, GLenum);
typedef GLboolean (__stdcall *glUnmapBufferARB) (GLenum);
typedef void (__stdcall *glGetBufferParameterivARB) (GLenum, GLenum, GLint *);
typedef void (__stdcall *glGetBufferPointervARB) (GLenum, GLenum, GLvoid* *);

extern glBindBufferARB				gglBindBufferARB;
extern glDeleteBuffersARB			gglDeleteBuffersARB;
extern glGenBuffersARB				gglGenBuffersARB;
extern glIsBufferARB				gglIsBufferARB;
extern glBufferDataARB				gglBufferDataARB;
extern glBufferSubDataARB			gglBufferSubDataARB;
extern glGetBufferSubDataARB		gglGetBufferSubDataARB;
extern glMapBufferARB				gglMapBufferARB;
extern glUnmapBufferARB				gglUnmapBufferARB;
extern glGetBufferParameterivARB	gglGetBufferParameterivARB;
extern glGetBufferPointervARB		gglGetBufferPointervARB;

// Extension for full-screen anti-aliasing
typedef bool (__stdcall *ChoosePixelFormatARB) (HDC, const int *, const float *, unsigned int, int *, unsigned int *);
extern ChoosePixelFormatARB gglChoosePixelFormatARB;
#define WGL_SAMPLE_BUFFERS_ARB              0x2041
#define WGL_SAMPLES_ARB                     0x2042
#define WGL_DOUBLE_BUFFER_ARB               0x2011
#define WGL_ACCELERATION_ARB                0x2003
#define WGL_FULL_ACCELERATION_ARB           0x2027
#define GL_MULTISAMPLE_ARB                  0x809D
#define WGL_DRAW_TO_WINDOW_EXT         0x2001
#define WGL_SUPPORT_OPENGL_EXT         0x2010
#define WGL_COLOR_BITS_EXT             0x2014
#define WGL_ALPHA_BITS_EXT             0x201B
#define WGL_DEPTH_BITS_EXT             0x2022
#define WGL_STENCIL_BITS_EXT           0x2023

void InitialiseOGLExtensions();
int IsOGLExtensionSupported(const char *extension);

#endif
