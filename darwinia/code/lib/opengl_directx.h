#if !defined __OPENGL_DIRECTX
#define __OPENGL_DIRECTX

// We are forced to declare our own prototypes because the default OpenGL.h
// includes APIENTRY which means that the function is to be linked in from a DLL
// and so we get an error when we try to define the function ourselves.

// Error codes returned by glGetError
#define GL_NO_ERROR                       0
#define GL_INVALID_ENUM                   0x0500
#define GL_INVALID_VALUE                  0x0501
#define GL_INVALID_OPERATION              0x0502
#define GL_STACK_OVERFLOW                 0x0503
#define GL_STACK_UNDERFLOW                0x0504
#define GL_OUT_OF_MEMORY                  0x0505

// Mode declarations for glBegin
#define GL_POINTS                         0x0000
#define GL_LINES                          0x0001
#define GL_LINE_LOOP                      0x0002
#define GL_TRIANGLES                      0x0004
#define GL_TRIANGLE_STRIP                 0x0005
#define GL_TRIANGLE_FAN                   0x0006
#define GL_QUADS                          0x0007
#define GL_QUAD_STRIP                     0x0008

// Shading Model
#define GL_FLAT							  0x1D00
#define GL_SMOOTH						  0x1D01

// Targets
#define GL_TEXTURE_2D					  0x0DE1

// Alpha function
#define GL_LEQUAL                         0x0203
#define GL_GREATER                        0x0204

// Accum op
#define GL_MULT                           0x0103

// Texture Paramater Names
#define GL_TEXTURE_MAG_FILTER             0x2800
#define GL_TEXTURE_MIN_FILTER             0x2801
#define GL_TEXTURE_WRAP_S                 0x2802
#define GL_TEXTURE_WRAP_T                 0x2803

// Texture env
#define GL_TEXTURE_ENV                    0x2300

// Texture env parameter
#define GL_TEXTURE_ENV_MODE               0x2200
#define GL_TEXTURE_ENV_COLOR              0x2201

// Texture env mode
#define GL_MODULATE                       0x2100
#define GL_DECAL                          0x2101
#define GL_REPLACE                        0x1E01

// Texture Min Filter
#define GL_NEAREST_MIPMAP_LINEAR          0x2702
#define GL_LINEAR_MIPMAP_LINEAR           0x2703

// Texture Mag Filter
#define GL_NEAREST                        0x2600
#define GL_LINEAR                         0x2601

// Texture Wrap Mode
#define GL_CLAMP                          0x2900
#define GL_REPEAT                         0x2901

// Material Face
#define GL_FRONT                          0x0404
#define GL_BACK                           0x0405
#define GL_FRONT_AND_BACK                 0x0408

// Material parameters
#define GL_SHININESS                      0x1601
#define GL_AMBIENT_AND_DIFFUSE            0x1602

// Polygon mode
#define GL_LINE                           0x1B01
#define GL_FILL                           0x1B02

// Hints
#define GL_POLYGON_SMOOTH_HINT            0x0C53
#define GL_FOG_HINT                       0x0C54

// Hint mode
#define GL_DONT_CARE                      0x1100

// Blending Factors
#define GL_ZERO                           0
#define GL_ONE                            1
#define GL_SRC_COLOR                      0x0300
#define GL_ONE_MINUS_SRC_COLOR            0x0301
#define GL_SRC_ALPHA                      0x0302
#define GL_ONE_MINUS_SRC_ALPHA            0x0303

// Pixel Format
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908

// Data types
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_FLOAT                          0x1406

// Attrib Mask
#define GL_CURRENT_BIT                    0x00000001
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_ENABLE_BIT                     0x00002000
#define GL_COLOR_BUFFER_BIT               0x00004000
#define GL_TEXTURE_BIT                    0x00040000

// Current values
#define GL_CURRENT_COLOR                  0x0B00

// Enable
#define GL_LINE_SMOOTH                    0x0B20
#define GL_CULL_FACE                      0x0B44
#define GL_DEPTH_TEST                     0x0B71
#define GL_BLEND                          0x0BE2
#define GL_LIGHTING                       0x0B50
#define GL_COLOR_MATERIAL                 0x0B57
#define GL_POLYGON_MODE                   0x0B40
#define GL_FRONT_FACE                     0x0B46
#define GL_NORMALIZE                      0x0BA1
#define GL_FOG                            0x0B60
#define GL_ALPHA_TEST                     0x0BC0
#define GL_SCISSOR_TEST                   0x0C11
#define GL_POINT_SMOOTH                   0x0B10

// Blends
#define GL_BLEND_DST                      0x0BE0
#define GL_BLEND_SRC                      0x0BE1

// Integers
#define GL_MATRIX_MODE                    0x0BA0
#define GL_VIEWPORT                       0x0BA2
#define GL_SHADE_MODEL                    0x0B54
#define GL_COLOR_MATERIAL_FACE            0x0B55
#define GL_COLOR_MATERIAL_PARAMETER       0x0B56
#define GL_DEPTH_FUNC                     0x0B74

// State Integers
#define GL_ALPHA_TEST_FUNC                0x0BC1
#define GL_DEPTH_FUNC                     0x0B74
#define GL_DEPTH_WRITEMASK                0x0B72

// State floats
#define GL_ALPHA_TEST_REF                 0x0BC2

// Float vectors
#define GL_LIGHT_MODEL_AMBIENT            0x0B53

// Front face direction
#define GL_CW                             0x0900
#define GL_CCW                            0x0901

// Fog parameters
#define GL_FOG_DENSITY                    0x0B62
#define GL_FOG_START                      0x0B63
#define GL_FOG_END                        0x0B64
#define GL_FOG_MODE                       0x0B65
#define GL_FOG_COLOR                      0x0B66

// Light names
#define GL_LIGHT0                         0x4000
#define GL_LIGHT1                         0x4001
#define GL_LIGHT2                         0x4002
#define GL_LIGHT3                         0x4003
#define GL_LIGHT4                         0x4004
#define GL_LIGHT5                         0x4005
#define GL_LIGHT6                         0x4006
#define GL_LIGHT7                         0x4007

// Light parameters
#define GL_AMBIENT                        0x1200
#define GL_DIFFUSE                        0x1201
#define GL_SPECULAR                       0x1202
#define GL_POSITION                       0x1203

// Matrix Mode
#define GL_MODELVIEW                      0x1700
#define GL_PROJECTION                     0x1701

// Matrix types
#define GL_MODELVIEW_MATRIX               0x0BA6
#define GL_PROJECTION_MATRIX              0x0BA7

// Clip plane
#define GL_CLIP_PLANE0                    0x3000
#define GL_CLIP_PLANE1                    0x3001
#define GL_CLIP_PLANE2                    0x3002

// List mode
#define GL_COMPILE                        0x1300

// String Name
#define GL_VENDOR                         0x1F00
#define GL_RENDERER                       0x1F01
#define GL_VERSION                        0x1F02
#define GL_EXTENSIONS                     0x1F03

// Client state arrays
#define GL_VERTEX_ARRAY                   0x8074
#define GL_NORMAL_ARRAY                   0x8075
#define GL_COLOR_ARRAY                    0x8076
#define GL_TEXTURE_COORD_ARRAY            0x8078

// Type definitions
typedef unsigned int GLenum;
typedef unsigned char GLubyte;
typedef int GLint;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef int GLsizei;
typedef unsigned char GLboolean;
typedef void GLvoid;
typedef unsigned int GLbitfield;
typedef double GLdouble;
typedef float GLclampf;

void glBegin ( GLenum mode );
void glEnd ();

void glEnable ( GLenum cap );
void glDisable ( GLenum cap );
GLboolean glIsEnabled ( GLenum cap );

GLenum glGetError ();
const GLubyte * glGetString (GLenum name);

void glHint (GLenum target, GLenum mode);

void glFrontFace (GLenum mode);

void glGetIntegerv (GLenum pname, GLint *params);
void glGetFloatv (GLenum pname, GLfloat *params);
void glGetDoublev (GLenum pname, GLdouble *params);

void glShadeModel ( GLenum mode );

void glBlendFunc ( GLenum sfactor, GLenum dfactor );
void glAlphaFunc (GLenum func, GLclampf ref);
void glDepthFunc (GLenum func);

void glClipPlane (GLenum plane, const GLdouble *equation);

void glClear (GLbitfield mask);

void glFinish ();

void glColor3ub ( GLubyte red, GLubyte green, GLubyte blue );
void glColor3ubv ( const GLubyte *v );
void glColor3f ( GLfloat red, GLfloat green, GLfloat blue );
void glColor4ub ( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha );
void glColor4ubv (const GLubyte *v);
void glColor4f ( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha );

void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);

void glVertex2i ( GLint x, GLint y );
void glVertex2f ( GLfloat x, GLfloat y );
void glVertex2fv (const GLfloat *v);
void glVertex3d (GLdouble x, GLdouble y, GLdouble z);
void glVertex3f (GLfloat x, GLfloat y, GLfloat z);
void glVertex3fv (const GLfloat *v);

void glDepthMask ( GLboolean flag );
void glLineWidth ( GLfloat width );

// Lights
void glLightfv (GLenum light, GLenum pname, const GLfloat *params);
void glGetLightfv (GLenum light, GLenum pname, GLfloat *params);
void glLightModelfv (GLenum pname, const GLfloat *params);

// Fog
void glFogfv (GLenum pname, const GLfloat *params);
void glFogi (GLenum pname, GLint param);
void glFogf (GLenum pname, GLfloat param);

// Textures
void glBindTexture ( GLenum target, GLuint texture );
void glTexImage2D (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
void glTexParameteri (GLenum target, GLenum pname, GLint param);
void glTexParameterf ( GLenum target, GLenum pname, GLfloat param );
void glTexCoord2i (GLint s, GLint t);
void glTexCoord2f ( GLfloat s, GLfloat t );
void glGenTextures (GLsizei n, GLuint *textures);
void glDeleteTextures (GLsizei n, const GLuint *textures);
void glCopyTexImage2D (GLenum target, GLint level, GLenum internalFormat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void glReadPixels (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);

// Texture Env
void glTexEnvf (GLenum target, GLenum pname, GLfloat param);
void glTexEnvi (GLenum target, GLenum pname, GLint param);
void glTexEnviv (GLenum target, GLenum pname, const GLint *params);
void glGetTexEnviv (GLenum target, GLenum pname, GLint *params);

// Display Lists
GLuint glGenLists (GLsizei range);
void glDeleteLists (GLuint list, GLsizei range);
void glCallList (GLuint list);
void glNewList (GLuint list, GLenum mode);
void glEndList ();

// Polygon Mode
void glPolygonMode (GLenum face, GLenum mode);

// Matrices
void glMatrixMode (GLenum mode);
void glPushMatrix ();
void glPopMatrix ();
void glLoadIdentity ();
void glLoadMatrixd (const GLdouble *m);
void glMultMatrixf (const GLfloat *m);
void glTranslatef (GLfloat x, GLfloat y, GLfloat z);
void glScalef (GLfloat x, GLfloat y, GLfloat z);
void glFrustum (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

// Normals
void glNormal3fv (const GLfloat *v);
void glNormal3f (GLfloat nx, GLfloat ny, GLfloat nz);

// Materials
void glMaterialfv (GLenum face, GLenum pname, const GLfloat *params);
void glGetMaterialfv (GLenum face, GLenum pname, GLfloat *params);
void glColorMaterial (GLenum face, GLenum mode);

// Points
void glPointSize (GLfloat size);
void glRotatef (GLfloat angle, GLfloat x, GLfloat y, GLfloat z);

void glViewport (GLint x, GLint y, GLsizei width, GLsizei height);

// Client state
void glEnableClientState (GLenum array);
void glDisableClientState (GLenum array);
void glVertexPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glColorPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glNormalPointer (GLenum type, GLsizei stride, const GLvoid *pointer);
void glTexCoordPointer (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void glDrawArrays (GLenum mode, GLint first, GLsizei count);

// GLU functions
void gluOrtho2D (GLdouble left, GLdouble right, GLdouble bottom, GLdouble top);
void gluPerspective (GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar);
void gluLookAt (GLdouble eyex, GLdouble eyey, GLdouble eyez, GLdouble centerx, GLdouble centery, GLdouble centerz, GLdouble upx, GLdouble upy, GLdouble upz);
int gluBuild2DMipmaps (GLenum target, GLint components, GLint width, GLint height, GLenum format, GLenum type, const void *data);
const GLubyte* gluErrorString (GLenum errCode);
int gluProject (GLdouble objx, GLdouble objy, GLdouble objz,  const GLdouble modelMatrix[16], const GLdouble  projMatrix[16], const GLint     viewport[4], GLdouble *winx, GLdouble *winy, GLdouble *winz);
int gluUnProject (GLdouble winx, GLdouble winy, GLdouble winz, const GLdouble modelMatrix[16], const GLdouble projMatrix[16], const GLint    viewport[4], GLdouble *objx, GLdouble *objy, GLdouble *objz);

#include "opengl_directx_inline.h"

#endif // __OPENGL_DIRECTX