#ifndef _included_universalinclude_h
#define _included_universalinclude_h



#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma warning( disable : 4244 4305 4800 4018 )
#define for if (0) ; else for


// Defines that will enable you to double click on a #pragma message
// in the Visual Studio output window.
#define MESSAGE_LINENUMBERTOSTRING(linenumber)	#linenumber
#define MESSAGE_LINENUMBER(linenumber)			MESSAGE_LINENUMBERTOSTRING(linenumber)
#define MESSAGE(x) message (__FILE__ "(" MESSAGE_LINENUMBER(__LINE__) "): "x) 

#include <crtdbg.h>
#define snprintf _snprintf

#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include <GL/GL.H>
#include <GL/GLU.H>

#endif