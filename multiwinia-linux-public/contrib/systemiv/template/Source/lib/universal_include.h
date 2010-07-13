#ifndef INCLUDED_UNIVERSAL_INCLUDE_H
#define INCLUDED_UNIVERSAL_INCLUDE_H

#define APP_NAME        "StressTest"
#define APP_VERSION     "v0.1 [Internal]"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>



#if defined WIN32
#pragma warning( disable : 4244 4305 4800 4018 )
#define for if (0) ; else for
// Defines that will enable you to double click on a #pragma message
// in the Visual Studio output window.
#define MESSAGE_LINENUMBERTOSTRING(linenumber)	#linenumber
#define MESSAGE_LINENUMBER(linenumber)			MESSAGE_LINENUMBERTOSTRING(linenumber)
#define MESSAGE(x) message (__FILE__ "(" MESSAGE_LINENUMBER(__LINE__) "): "x) 

#include <crtdbg.h>
#define snprintf _snprintf

#define NOMINMAX

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif // WIN32

#include <GL/gl.h>
#include <GL/glu.h>


#ifdef TARGET_OS_LINUX
#define stricmp strcasecmp

//#define _POSIX_C_SOURCE 199309
#include <time.h>

inline void Sleep( unsigned _milliseconds ) 
{
	struct timespec delay = { _milliseconds / 1000, (_milliseconds % 1000) * 1000000 };
	nanosleep( &delay, NULL );
}
#endif // TARGET_OS_LINUX

#endif // INCLUDED_UNIVERSAL_INCLUDE_H
