#include "lib/universal_include.h"

#include <ctype.h>

#include "lib/string_utils.h"


void StrToLower(char *_string)
{
	while (*_string != '\0')
	{
		*_string = tolower(*_string);
		_string++;
	}
}

