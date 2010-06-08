#ifndef INCLUDED_UNIVERSAL_INCLUDE_H
#define INCLUDED_UNIVERSAL_INCLUDE_H

#define DARWINIA_VERSION "1.5.11"
#define DARWINIA_EXE_VERSION 1,5,11,0
#define STR_DARWINIA_EXE_VERSION "1, 5, 11, 0\0"

// === PICK ONE OF THESE TARGETS ===

#define TARGET_FULLGAME
//#define TARGET_DEMOGAME
//#define TARGET_PURITYCONTROL
//#define TARGET_DEMO2
//#define TARGET_DEBUG
//#define TARGET_VISTA
//#define TARGET_VISTA_DEMO2

// === PICK ONE OF THESE TARGETS ===

#define DEBUG_RENDER_ENABLED

//#define USE_CRASHREPORTING

#ifndef _OPENMP
#define PROFILER_ENABLED
#endif

#ifdef TARGET_VISTA
    #define DARWINIA_GAMETYPE "vista"
    #define LOCATION_EDITOR
    #define TARGET_OS_VISTA
    #define ATTRACTMODE_ENABLED
#endif

#ifdef TARGET_FULLGAME
    #define DARWINIA_GAMETYPE "full"
    #define LOCATION_EDITOR
#endif

#ifdef TARGET_FULLGAME_FRENCH
    #define DARWINIA_GAMETYPE "full"
    #define LOCATION_EDITOR
    #define FRENCH
#endif

#ifdef TARGET_DEMOGAME
    #define DARWINIA_GAMETYPE "demo"
    #define DEMOBUILD
#endif

#ifdef TARGET_DEMO2
    #define DARWINIA_GAMETYPE "demo2"
    #define DEMOBUILD
    #define DEMO2
#endif

#ifdef TARGET_VISTA_DEMO2
    #define DARWINIA_GAMETYPE "vista-demo2"
    #define DEMOBUILD
    #define DEMO2
    #define TARGET_OS_VISTA
#endif

#ifdef TARGET_PURITYCONTROL
    #define DARWINIA_GAMETYPE "full"
    #define PURITY_CONTROL
#endif

#ifdef TARGET_DEBUG
    #define DARWINIA_GAMETYPE "debug"
    #define LOCATION_EDITOR
    #define SOUND_EDITOR
    #define CHEATMENU_ENABLED
    #define GESTURE_EDITOR
    //#define TEST_HARNESS_ENABLED
    //#define SCRIPT_TEST_ENABLED
    #define AVI_GENERATOR
    //#define TRACK_MEMORY_LEAKS
	#define D3D_DEBUG_INFO
#endif

//#define PROMOTIONAL_BUILD                         // Their company logo is shown on screen

#if !defined(TARGET_DEBUG) &&       \
    !defined(TARGET_FULLGAME) &&    \
    !defined(TARGET_DEMOGAME) &&    \
    !defined(TARGET_DEMO2) &&       \
    !defined(TARGET_PURITYCONTROL) && \
    !defined(TARGET_VISTA) && \
    !defined(TARGET_VISTA_DEMO2 )
#error "Unknown target, cannot determine game type"
#endif

#ifndef PROFILER_ENABLED
#define DARWINIA_VERSION_PROFILER "-np"
#else
#define DARWINIA_VERSION_PROFILER ""
#endif

#define DARWINIA_VERSION_STRING DARWINIA_PLATFORM "-" DARWINIA_GAMETYPE "-" DARWINIA_VERSION DARWINIA_VERSION_PROFILER

#include <stdio.h>
#include <math.h>

#ifdef TARGET_MSVC
	#pragma warning( disable : 4244 4305 4800 4018 )
	#define for if (0) ; else for

	// Defines that will enable you to double click on a #pragma message
	// in the Visual Studio output window.
	#define MESSAGE_LINENUMBERTOSTRING(linenumber)	#linenumber
	#define MESSAGE_LINENUMBER(linenumber)			MESSAGE_LINENUMBERTOSTRING(linenumber)
	#define MESSAGE(x) message (__FILE__ "(" MESSAGE_LINENUMBER(__LINE__) "): "x)

	#include <crtdbg.h>
	#define snprintf _snprintf

	// Visual studio 2005 insists that we use the underscored versions
	#include <string.h>
	#define stricmp _stricmp
	#define strupr _strupr
    #define strnicmp _strnicmp
	#define strlwr _strlwr
	#define strdup _strdup

	#include <stdlib.h>
	#define itoa _itoa

    #define DARWINIA_PLATFORM "win32"

	#define WIN32_LEAN_AND_MEAN
	#define _WIN32_WINDOWS 0x0500	// for IsDebuggerPresent
	#include "windows.h"

	#define HAVE_DSOUND
#endif

#ifdef TARGET_MINGW
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h"
	#define HAVE_INET_NTOA
	#define HAVE_DSOUND
#endif

#ifdef TARGET_OS_LINUX
	#include <ctype.h>
	#include <string.h>

	#define stricmp strcasecmp
	#define strnicmp strncasecmp
	#define __stdcall
	template<class T> inline T min (T a, T b) { return (a < b) ? a : b; };
	template<class T> inline T max (T a, T b) { return (a > b) ? a : b; };
	inline char * strlwr(char *s) {
	  char *p = s;
	  for (char *p = s; *p; p++)
		*p = tolower(*p);
	  return s;
	}
	inline char * strupr(char *s) {
	  char *p = s;
	  for (char *p = s; *p; p++)
		*p = toupper(*p);
	  return s;
	}
	#include <unistd.h>
	#define Sleep sleep

    #define DARWINIA_PLATFORM "linux"
	#define _snprintf snprintf

	#undef AVI_GENERATOR
	#undef SOUND_EDITOR
#endif

#ifdef TARGET_OS_MACOSX
    #define DARWINIA_PLATFORM "macosx"

	#include <unistd.h>
	#define Sleep sleep

	#include <ctype.h>
	#define stricmp strcasecmp
	#define strnicmp strncasecmp

	template<class T> inline T min (T a, T b) { return (a < b) ? a : b; };
	template<class T> inline T max (T a, T b) { return (a > b) ? a : b; };

	inline char * strlwr(char *s) {
	  for (char *p = s; *p; p++)
		*p = tolower(*p);
	  return s;
	}
	inline char * strupr(char *s) {
	  for (char *p = s; *p; p++)
		*p = toupper(*p);
	  return s;
	}

	#define __stdcall
	#define HAVE_INET_NTOA
	#define _snprintf snprintf
	#undef SOUND_EDITOR

	/* acosf and asinf don't link for some reason */
	#define acosf acos
	#define asinf asin

	#undef AVI_GENERATOR
	#undef SOUND_EDITOR
#endif // TARGET_OS_MACOSX

#ifdef TARGET_OS_MACOSX
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#ifdef USE_DIRECT3D
	#include <d3d9.h>
	#include <d3dx9.h>
	#include "lib/opengl_directx.h"
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif // USE_DIRECTX
#endif // !TARGET_OS_MACOSX

#define SAFE_FREE(x) {free(x);x=NULL;}
#define SAFE_DELETE(x) {delete x;x=NULL;}
#define SAFE_DELETE_ARRAY(x) {delete[] x;x=NULL;}
#define SAFE_RELEASE(x) {if(x){(x)->Release();x=NULL;}}

#endif
