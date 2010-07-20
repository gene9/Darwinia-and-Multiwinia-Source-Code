// IsAsciiFile.h  Version 1.0
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

#ifndef ISASCIIFILE_H
#define ISASCIIFILE_H

BOOL IsAsciiFile(HANDLE hFile, 
				 DWORD dwNumberCharsToCheck = 100, 
				 DWORD dwMaxIllegalCharacters = 5,
				 char cIllegalCharLow = ' ',
				 char cIllegalCharHigh = '~');

BOOL IsAsciiFile(const char * lpBuf, 
				 DWORD dwNumberCharsToCheck = 100, 
				 DWORD dwMaxIllegalCharacters = 5,
				 char cIllegalCharLow = ' ',
				 char cIllegalCharHigh = '~');

#endif //ISASCIIFILE_H
