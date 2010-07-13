#include "lib/universal_include.h"
#include "lib/netlib/net_lib.h"
#include "app.h"

NetMutex g_openGLMutex;

#undef glNewList
void safeGlNewList( GLuint list, GLenum mode )
{
#ifndef USE_DIRECT3D
	if( NetGetCurrentThreadId() != g_app->m_mainThreadId )
	{
		AppDebugOut("glNewList called from non-mainthread (bad for OpenGL)\n");
	}
#endif

	g_openGLMutex.Lock();
	glNewList( list, mode );
}

#undef glEndList
void safeGlEndList()
{
	glEndList();
	g_openGLMutex.Unlock();
}

