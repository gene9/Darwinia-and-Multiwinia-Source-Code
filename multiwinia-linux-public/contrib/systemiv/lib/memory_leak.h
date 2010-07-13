#ifndef _included_memoryleak_h
#define _included_memoryleak_h

/*
 *	USAGE
 *
 *  Include this header file in every source file you wish to track leaks from
 *  ie #include "memory_leak.h" in some sort of universal_include.h
 *  that is included from everywhere
 *
 *  At the very end of your application close down procedure
 *  you should then call AppPrintMemoryLeaks.
 *
 */

#include <strstream>
#include <ostream>
#include <xiosbase>
#include <xlocale>
#include <xdebug>
#include <map>

#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)


void AppPrintMemoryLeaks( char *_filename );




#endif
