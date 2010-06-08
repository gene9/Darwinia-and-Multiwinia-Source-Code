#ifndef INCLUDED_STRING_UTILS_H
#define INCLUDED_STRING_UTILS_H

#include <string.h>

void StrToLower(char *_string);

inline char *NewStr(const char *src)
{
	return strcpy(new char[strlen(src)+1], src);
}

#endif
