#include "lib/universal_include.h"

#include "lib/ogl_extensions.h"

#ifdef TARGET_OS_LINUX
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#else
#include <SDL.h>
#include <OpenGL/glu.h>
#endif
#include <string.h>
#include "lib/netlib/net_thread.h"
#include "app.h"

const GLubyte *g_openGLExtensions = NULL;

MultiTexCoord2fARB gglMultiTexCoord2fARB = NULL;
ActiveTextureARB gglActiveTextureARB = NULL;
	
glBindBufferARB                 gglBindBufferARB = NULL;
glDeleteBuffersARB              gglDeleteBuffersARB = NULL;
glGenBuffersARB                 gglGenBuffersARB = NULL;
glIsBufferARB                   gglIsBufferARB = NULL;
glBufferDataARB                 gglBufferDataARB = NULL;
glBufferSubDataARB              gglBufferSubDataARB = NULL;
glGetBufferSubDataARB           gglGetBufferSubDataARB = NULL;
glMapBufferARB                  gglMapBufferARB = NULL;
glUnmapBufferARB                gglUnmapBufferARB = NULL;
glGetBufferParameterivARB       gglGetBufferParameterivARB = NULL;
glGetBufferPointervARB          gglGetBufferPointervARB = NULL;

	
void InitialiseOGLExtensions()
{
	AppReleaseAssert(NetGetCurrentThreadId() == g_app->m_mainThreadId,
					 "can't initialize OpenGL on a secondary thread");
	
	gglMultiTexCoord2fARB = (MultiTexCoord2fARB)SDL_GL_GetProcAddress("glMultiTexCoord2fARB");
	gglActiveTextureARB = (ActiveTextureARB)SDL_GL_GetProcAddress("glActiveTextureARB");

	gglBindBufferARB                = (glBindBufferARB)             SDL_GL_GetProcAddress("glBindBufferARB");
	gglDeleteBuffersARB             = (glDeleteBuffersARB)          SDL_GL_GetProcAddress("glDeleteBuffersARB");
	gglGenBuffersARB                = (glGenBuffersARB)             SDL_GL_GetProcAddress("glGenBuffersARB");
	gglIsBufferARB                  = (glIsBufferARB)               SDL_GL_GetProcAddress("glIsBufferARB");
	gglBufferDataARB                = (glBufferDataARB)             SDL_GL_GetProcAddress("glBufferDataARB");
	gglBufferSubDataARB             = (glBufferSubDataARB)          SDL_GL_GetProcAddress("glBufferSubDataARB");
	gglGetBufferSubDataARB          = (glGetBufferSubDataARB)       SDL_GL_GetProcAddress("glGetBufferSubDataARB");
	gglMapBufferARB                 = (glMapBufferARB)              SDL_GL_GetProcAddress("glMapBufferARB");
	gglUnmapBufferARB               = (glUnmapBufferARB)            SDL_GL_GetProcAddress("glUnmapBufferARB");
	gglGetBufferParameterivARB      = (glGetBufferParameterivARB)   SDL_GL_GetProcAddress("glGetBufferParameterivARB");
	gglGetBufferPointervARB         = (glGetBufferPointervARB)      SDL_GL_GetProcAddress("glGetBufferPointervARB");
	
	g_openGLExtensions = glGetString(GL_EXTENSIONS);
}

int IsOGLExtensionSupported(const char *extension)
{
	return gluCheckExtension((const GLubyte *)extension, g_openGLExtensions);
}

// The Windows code has a blacklist for VBOs on certain Intel hardware, which we aren't
// worrying about yet.
int IsOGLExtensionPerfectlySupported(const char *extension)
{
	return IsOGLExtensionSupported(extension);
}
