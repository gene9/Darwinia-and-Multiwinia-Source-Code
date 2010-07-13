// GetFilePart.cpp  Version 1.0
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like, except
// that you may not sell this source code.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#include "windows.h"
#include "tchar.h"
#include "GetFilePart.h"

TCHAR * GetFilePart(LPCTSTR lpszFile)
{
	TCHAR *result = const_cast<TCHAR *>(_tcsrchr(lpszFile, _T('\\')));
	if (result)
		result++;
	else
		result = (TCHAR *) lpszFile;
	return result;
}
