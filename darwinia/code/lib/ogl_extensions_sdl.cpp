#include "lib/universal_include.h"

#include "lib/ogl_extensions.h"

#include <SDL.h>
#include <string.h>

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
}

int IsOGLExtensionSupported(const char *extension)
{
	// From http://www.opengl.org/resources/features/OGLextensions/
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;
	extensions = glGetString(GL_EXTENSIONS);
	/* It takes a bit of care to be fool-proof about parsing the
	   OpenGL extensions string. Don't be fooled by sub-strings,
	   etc. */
	start = extensions;
	for (;;) {
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;
		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
		if (*terminator == ' ' || *terminator == '\0')
			return 1;
		start = terminator;
	}
	return 0;
}
