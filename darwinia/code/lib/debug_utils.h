#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <stdio.h>

void DebugOut(char *fmt, ...);


void DarwiniaReleaseAssert(bool _condition, char const *_fmt, ...);

#ifdef _DEBUG
  #ifdef TARGET_MSVC
	#define DarwiniaDebugAssert(x) \
		if(!(x)) { \
			/*GenerateBlackBox();*/ \
			::ShowCursor(true); \
			_ASSERT(x); \
		}
  #else
    #define DarwiniaDebugAssert(x) { assert(x); }
  #endif
#else
	#define DarwiniaDebugAssert(x)
#endif


void WriteSystemReport(FILE *_out);


#if 0
	#define OVERLOADED_NEW
	void SetNewReportingThreshold(int _size);

    #define OverloadedNew new( (char const *)__FILE__, __LINE__ )
	void* operator new (unsigned _size, char const *_filename, int _line);
	void* operator new[] (unsigned _size, char const *_filename, int _line);
	#define new OverloadedNew

//	#define OverloadedDelete delete( (char const *)__FILE__, __LINE__ )
	void operator delete (void *p, char const *_filename, int _line);
//	void operator delete[] (void *p, char const *_filename, int _line);
//	#define delete OverloadedDelete
#endif

#ifdef TRACK_MEMORY_LEAKS
    #define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

void PrintMemoryLeaks();
void GenerateBlackBox( char *_msg );

#endif

