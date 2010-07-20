// IsAsciiFile.cpp  Version 1.0
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

#include "stdafx.h"
#include "IsAsciiFile.h"
#include <crtdbg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


BOOL IsAsciiFile(HANDLE hFile, 
				 DWORD dwNumberCharsToCheck /*= 100*/, 
				 DWORD dwMaxIllegalCharacters /*= 5*/,
				 char cIllegalCharLow /*= ' '*/,
				 char cIllegalCharHigh /*= '~'*/)
{
	if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
	{
		TRACE(_T("ERROR - bad file handle\n"));
		_ASSERTE(FALSE);
		return FALSE;
	}

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
	{
		TRACE(_T("ERROR - GetFileSize failed or returned 0\n"));
		return FALSE;
	}

	if (dwNumberCharsToCheck > 10000)				// sanity check
		dwNumberCharsToCheck = 10000;

	if (dwFileSize < dwNumberCharsToCheck)
		dwNumberCharsToCheck = dwFileSize;

	char *pBuf = new char [dwNumberCharsToCheck+10];
	_ASSERTE(pBuf);
	if (!pBuf)
		return FALSE;

	DWORD dwBytesRead = 0;

	BOOL bRet = TRUE;

	BOOL bRetVal = ReadFile(hFile,					// handle to file
							pBuf,					// data buffer
							dwNumberCharsToCheck,	// number of bytes to read
							&dwBytesRead,			// number of bytes read
							NULL);					// overlapped buffer

	if (!bRetVal || dwBytesRead == 0)
	{
		bRet = FALSE;
	}
	else
	{
		bRet = IsAsciiFile(pBuf,
						   dwNumberCharsToCheck,
						   dwMaxIllegalCharacters,
						   cIllegalCharLow,
						   cIllegalCharHigh);
	}

	delete [] pBuf;

	TRACE(_T("IsAsciiFile returning %d\n"), bRet);

	return bRet;
}

BOOL IsAsciiFile(const char * lpBuf,
				 DWORD dwNumberCharsToCheck /*= 100*/, 
				 DWORD dwMaxIllegalCharacters /*= 5*/,
				 char cIllegalCharLow /*= ' '*/,
				 char cIllegalCharHigh /*= '~'*/)
{
	if (lpBuf == NULL)
	{
		TRACE(_T("ERROR - no buffer\n"));
		_ASSERTE(FALSE);
		return FALSE;
	}

	_ASSERTE(dwNumberCharsToCheck > 0);
	_ASSERTE(dwMaxIllegalCharacters > 0);
	_ASSERTE(cIllegalCharLow < cIllegalCharHigh);

	if ((dwNumberCharsToCheck == 0) ||
		(dwMaxIllegalCharacters == 0) ||
		(cIllegalCharLow >= cIllegalCharHigh))
	{
		return FALSE;
	}

	if (dwNumberCharsToCheck > 10000)				// sanity check
		dwNumberCharsToCheck = 10000;

	// loop to check if file is an Ascii file

	BOOL bRet = TRUE;

	for (DWORD dwIndex = 0; dwIndex < dwNumberCharsToCheck; dwIndex++)
	{
		char c = lpBuf[dwIndex];

		if (((c >= cIllegalCharLow) && (c <= cIllegalCharHigh)) ||
			 (c == '\f') ||		// formfeed
			 (c == '\n') ||		// newline
			 (c == '\r') ||		// carriage return
			 (c == '\t') ||		// horizontal tab
			 (c == '\v'))		// vertical tab
		{
			continue;				// char is legal
		}
		else
		{
			dwMaxIllegalCharacters--;
			if (dwMaxIllegalCharacters == 0)
			{
				bRet = FALSE;
				break;
			}
		}
	}

	TRACE(_T("IsAsciiFile returning %d\n"), bRet);

	return bRet;
}
