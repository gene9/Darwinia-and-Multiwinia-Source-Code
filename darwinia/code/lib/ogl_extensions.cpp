#include "lib/universal_include.h"

#if !defined USE_DIRECT3D
#include "lib/debug_utils.h"
#include "lib/ogl_extensions.h"


MultiTexCoord2fARB gglMultiTexCoord2fARB = NULL;
ActiveTextureARB gglActiveTextureARB = NULL;

glBindBufferARB				gglBindBufferARB = NULL;
glDeleteBuffersARB			gglDeleteBuffersARB = NULL;
glGenBuffersARB				gglGenBuffersARB = NULL;
glIsBufferARB				gglIsBufferARB = NULL;
glBufferDataARB				gglBufferDataARB = NULL;
glBufferSubDataARB			gglBufferSubDataARB = NULL;
glGetBufferSubDataARB		gglGetBufferSubDataARB = NULL;
glMapBufferARB				gglMapBufferARB = NULL;
glUnmapBufferARB			gglUnmapBufferARB = NULL;
glGetBufferParameterivARB	gglGetBufferParameterivARB = NULL;
glGetBufferPointervARB		gglGetBufferPointervARB = NULL;

ChoosePixelFormatARB gglChoosePixelFormatARB = NULL;
	
void InitialiseOGLExtensions()
{
//	char const *extensions = (char const *)glGetString(GL_EXTENSIONS);
//	int len = strlen(extensions);
//	char *c = new char[len];
//	memcpy(c, extensions, len);
//	extensions = c;
//
//	while(*c)
//	{
//		if (*c == ' ') *c = '\n'; 
//		++c;
//	}
//
//	FILE *out = fopen("blah.txt", "w");
//	fprintf(out, "%s\n", extensions);
//	fclose(out);

    gglMultiTexCoord2fARB = (MultiTexCoord2fARB)wglGetProcAddress("glMultiTexCoord2fARB");
    gglActiveTextureARB = (ActiveTextureARB)wglGetProcAddress("glActiveTextureARB");

	gglBindBufferARB			= (glBindBufferARB)				wglGetProcAddress("glBindBufferARB");
	gglDeleteBuffersARB			= (glDeleteBuffersARB)			wglGetProcAddress("glDeleteBuffersARB");
	gglGenBuffersARB			= (glGenBuffersARB)				wglGetProcAddress("glGenBuffersARB");
	gglIsBufferARB				= (glIsBufferARB)				wglGetProcAddress("glIsBufferARB");
	gglBufferDataARB			= (glBufferDataARB)				wglGetProcAddress("glBufferDataARB");
	gglBufferSubDataARB			= (glBufferSubDataARB)			wglGetProcAddress("glBufferSubDataARB");
	gglGetBufferSubDataARB		= (glGetBufferSubDataARB)		wglGetProcAddress("glGetBufferSubDataARB");
	gglMapBufferARB				= (glMapBufferARB)				wglGetProcAddress("glMapBufferARB");
	gglUnmapBufferARB			= (glUnmapBufferARB)			wglGetProcAddress("glUnmapBufferARB");
	gglGetBufferParameterivARB	= (glGetBufferParameterivARB)	wglGetProcAddress("glGetBufferParameterivARB");
	gglGetBufferPointervARB		= (glGetBufferPointervARB)		wglGetProcAddress("glGetBufferPointervARB");

    gglChoosePixelFormatARB = (ChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB");
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
#endif // !defined USE_DIRECT3D
