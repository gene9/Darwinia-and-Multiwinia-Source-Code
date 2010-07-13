/*
 *  math_stubs.cpp
 *  Multiwinia
 *
 *  Created by Mike Blaguszewski on 10/25/07.
 *  Copyright 2007 Ambrosia Software, Inc.. All rights reserved.
 *
 */

#include "lib/universal_include.h"
#include "lib/math/math_stubs.h"

#include <stdlib.h>
#include <wchar.h>

extern "C"
{
	double iv_strtod(const char *s00, char **se);
};

#ifndef _XBOX

double iv_trunc_low_nibble(double x)
{
	volatile float f = (float)x;
	return f;
}

#endif

double iv_atof(const char *str)
{
	return iv_trunc_low_nibble(atof(str));
}

int iv_wtoi( const wchar_t *str )
{
	int result;
	if( swscanf( str, L"%d", &result ) == 1 )
		return result;
	else
		return 0;
}

double iv_wtof( const wchar_t *str )
{
	double result;

	if( swscanf( str, L"%lf", &result ) == 1 )
		return iv_trunc_low_nibble(result);
	else
		return 0;
}
