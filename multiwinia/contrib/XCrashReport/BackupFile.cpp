// BackupFile.h  Version 1.0
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
#include "BackupFile.h"
#include <io.h>

BOOL BackupFile(LPCTSTR lpszFile)
{
	ASSERT(lpszFile);

	// if file doesn't exist, nothing to do
	if (_taccess(lpszFile, 00) == -1)
		return TRUE;

	// file exists, so make a backup copy

	CString strFileRoot = lpszFile;

	CString strPath = _T("");
	CString strExt = _T("");
	BOOL bSuccess = FALSE;
	for (int i = 1; i <= 9999; i++)
	{
		strExt.Format(_T(".%04d"), i);
		strPath = strFileRoot;
		strPath += strExt;
		if (_taccess(strPath, 00) == -1)
		{
			// file doesn't exist, so we can use it
			bSuccess = CopyFile(lpszFile, strPath, TRUE);
			break;
		}
	}

	return bSuccess;
}
