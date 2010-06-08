#include "lib/universal_include.h"

#ifdef USE_SEPULVEDA_HELP_TUTORIAL

#define JAL_DEBUG 0
#if JAL_DEBUG
	#include <iostream>
	#include "lib/debug.h"
	#define TRACE_RET_B(x)  TRACE_LINE( "-> " << B(x) )
	using namespace std;
#else
	#define TRACE_FUNC(x,y)
	#define TRACE_LINE(x)
	#define TRACE_RET_B(x)
#endif

#include "lib/input/input.h"
#include "lib/language_table.h"
#include "app.h"
#include "sepulveda.h"

#include "lib/hi_res_time.h"

#ifdef TARGET_MSVC
	#define snprintf _snprintf
#endif

#ifndef SEPULVEDA_MAX_PHRASE_LENGTH
	#define SEPULVEDA_MAX_PHRASE_LENGTH	1024
#endif
#define MAX_READ_KEY_LENGTH 64
#define MAX_FINAL_KEY_LENGTH 96


#if JAL_DEBUG
char head_str[50];

const char * head( char const * _str ) {
	int maxLen = 25;
	head_str[0] = '"';
	strncpy( head_str + 1, _str, maxLen - 2 );
	int len = 1;
	while ( len < maxLen && head_str[len] != '\0' ) ++len;
	if ( len >= maxLen )
		strcpy( head_str + (maxLen - 4), "...\"" );
	else
		strcpy( head_str + len, "\"" );
	return head_str;
}

ostream & operator<<( ostream &stream, CaptionParserMode &mode ) {
	stream << "CPM{ w = " << mode.writing << ", d = " << mode.ifdepth
	       << ", in = " << mode.inOffset << ", out = " << mode.outOffset
		   << ", mood = " << mode.mood << " }";
	return stream;
}
#endif

#endif // USE_SEPULVEDA_HELP_TUTORIAL
