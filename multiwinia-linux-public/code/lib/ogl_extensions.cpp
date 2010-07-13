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


static char s_glVendor[256];
static char s_glRenderer[256];
static char s_glVersion[256];
static char s_glExtensions[4096];


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

	// GL_ARB_multitexture
	gglMultiTexCoord2fARB = (MultiTexCoord2fARB)wglGetProcAddress("glMultiTexCoord2fARB");
	gglActiveTextureARB = (ActiveTextureARB)wglGetProcAddress("glActiveTextureARB");

	// GL_ARB_vertex_buffer_object
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


/*
    Because there is no way to extend WGL, these calls are defined in the
    ICD and can be called by obtaining the address with wglGetProcAddress.
    Because this extension is a WGL extension, it is not included in the
    extension string returned by glGetString. Its existence can be
    determined with the WGL_ARB_extensions_string extension.
*/

	// WGL_ARB_pixel_format
	gglChoosePixelFormatARB = (ChoosePixelFormatARB)wglGetProcAddress("wglChoosePixelFormatARB");

	
	//
	// Get useful information about the renderer

/*
	  GL_VENDOR	      Returns the company responsible for this
			      GL implementation.  This name does not
			      change from release to release.

	  GL_RENDERER	      Returns the name of the renderer.	 This
			      name is typically	specific to a
			      particular configuration of a hardware
			      platform.	 It does not change from
			      release to release.

	  GL_VERSION	      Returns a	version	or release number.

	  GL_EXTENSIONS	      Returns a	space-separated	list of
			      supported	extensions to GL.
*/

	const GLubyte *vendor = glGetString(GL_VENDOR);
	const GLubyte *renderer = glGetString(GL_RENDERER);
	const GLubyte *version = glGetString(GL_VERSION);
	const GLubyte *extensions = glGetString(GL_EXTENSIONS);

	s_glVendor[0] = '\0';
	s_glRenderer[0] = '\0';
	s_glVersion[0] = '\0';
	s_glExtensions[0] = '\0';

	if( vendor )
	{
		strncpy( s_glVendor, (const char*) vendor, sizeof(s_glVendor) );
		s_glVendor[ sizeof(s_glVendor) - 1 ] = '\0';
	}

	if( renderer )
	{
		strncpy( s_glRenderer, (const char*) renderer, sizeof(s_glRenderer) );
		s_glRenderer[ sizeof(s_glRenderer) - 1 ] = '\0';
	}

	if( version )
	{
		strncpy( s_glVersion, (const char*) version, sizeof(s_glVersion) );
		s_glVersion[ sizeof(s_glVersion) - 1 ] = '\0';
	}

	if( extensions )
	{
		strncpy( s_glExtensions, (const char*) extensions, sizeof(s_glExtensions) );
		s_glExtensions[ sizeof(s_glExtensions) - 1 ] = '\0';
	}

	AppDebugOut( "OpenGL vendor '%s', renderer '%s', version '%s', extensions '%s'\n", s_glVendor, s_glRenderer, s_glVersion, s_glExtensions );
}

static char* blacklistIntel[] = {
							 /* Reported by users */
							 "Intel 945GM",
							 "Intel 965/963",
							 "Intel Bear Lake B", /* Intel(R) G33/G31 Express Chipset Family, Intel(R) GMA 3100 */
							 /* Untested */
							 "Intel 82810",
							 "Intel 82815",
							 "Intel 82830M",
							 "Intel 82845G",
							 "Intel 82845PE",
							 "Intel 82852",
							 "Intel 82855",
							 "Intel 82865G",
							 "Intel 82910GL",
							 "Intel 82915G",
							 "Intel 82945G",
							 "Intel 910GML",
							 "Intel 915GM",
							 "Intel 945",
							 "Intel 963",
							 "Intel 965",
							 NULL
								};

/*
static char* blacklistIntel[] = {
							 "Intel(R) 82865G Graphics Controller",
							 "Intel(R) 82845G/GL/GE/PE/GV Graphics Controller",
							 "Intel(R) 82945G Express Chipset Family",
							 "Intel(R) 82852/82855 GM/GME Graphics Controller",
							 "Intel(R) 82915G/GV/910GL Express Chipset Family",
							 "Mobile Intel(R) 945GM/GU Express Chipset Family",
							 "Intel(R) 82810E Graphics Controller",
							 "Intel(R) 82815 Graphics Controller",
							 "Intel(r) 82810 Graphics Controller",
							 "Intel(R) 82845G Graphics Controller",
							 "Intel(R) 82915G Express Chipset Family",
							 "Intel(R) 82845G/GL Graphics Controller",
							 "Mobile Intel(R) 915GM/GMS/910GML Express Chipset Family (Microsoft Corporation - XDDM)",
							 "Mobile Intel(R) 945GM Express Chipset Controller 1 (Microsoft Corporation - WDDM)",
							 "Mobile Intel(R) 945GM Express Chipset Controller 0 (Microsoft Corporation - WDDM)",
							 "Intel(R) 82865G Graphics Controller (Microsoft Corporation - XDDM)",
							 "Intel(R) 82945G Express Chipset Controller 0 (Microsoft Corporation - WDDM)",
							 "Intel(R) 82915G/GV/910GL Express Chipset Family (Microsoft Corporation - XDDM)",
							 "Intel(R) 82830M Graphics Controller",
							 "Intel(R) 82852/82855 GM/GME Graphics Controller (Microsoft Corporation - XDDM)",
							 "Intel(R) 82810-DC100 Graphics Controller",
							 "Mobile Intel(R) 915GM/GMS,910GML Express Chipset Family",
							 "Mobile Intel(R) 945 Express Chipset Family",
							 "Mobile Intel(R) 945GM Express Chipset Family"
							 "Mobile Intel(R) 965 Express Chipset Family",
							 NULL
								};
*/

/* Ok Intel card?
							 "Intel(R) G965 Express Chipset Family",
							 "Intel(R) 946GZ Express Chipset Family",
							 "Intel(R) Q35 Express Chipset Family",
*/

static char* blacklistSiS[] = {
							 /* Reported by users */
							 "Mirage Graphics3", /* SiS Mirage 3 Graphics */
							 NULL
								};

static bool IsOGLExtensionBlacklisted(const char *extension)
{
	if (strcmp( extension, "GL_ARB_vertex_buffer_object" ) == 0)
	{
		if (strstr( s_glVendor, "Intel" ))
		{
			for (int i = 0; blacklistIntel[i]; i++)
			{
				if (strstr( s_glRenderer, blacklistIntel[i] ) )
				{
					return true;
				}
			}
		}
		if (strstr( s_glVendor, "SiS" ))
		{
			for (int i = 0; blacklistSiS[i]; i++)
			{
				if (strstr( s_glRenderer, blacklistSiS[i] ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}


int IsOGLExtensionPerfectlySupported(const char *extension)
{
	int ret = IsOGLExtensionSupported( extension );
	
	if ( ret )
	{
		if (strcmp( extension, "GL_ARB_vertex_buffer_object" ) == 0)
		{
			if (strstr( s_glVendor, "Intel" ) || strstr( s_glVendor, "SiS" ))
			{
				return 0;
			}
		}
	}

	return ret;
}


int IsOGLExtensionSupported(const char *extension)
{
	if (IsOGLExtensionBlacklisted( extension ))
		return 0;

	// From http://www.opengl.org/resources/features/OGLextensions/
	GLubyte *extensions = (GLubyte *) s_glExtensions;
	GLubyte *start;
	GLubyte *where, *terminator;

	/* Extension names should not have spaces. */
	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;
	//extensions = glGetString(GL_EXTENSIONS);
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
