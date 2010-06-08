// SendEmail.h  Version 1.0
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

#ifndef SENDEMAIL_H
#define SENDEMAIL_H

BOOL SendEmail(HWND    hWnd,			// parent window, must not be NULL
			   LPCTSTR lpszAppMagicPrefix, // may not be NULL
			   LPCTSTR lpszAppVersion,	// may not be NULL
			   LPCTSTR lpszCrashID,		// may be NULL
			   LPCTSTR lpszSubject,		// may be NULL
			   LPCTSTR lpszMessage,		// may be NULL
			   LPCTSTR lpszAttachment);

#endif //SENDEMAIL_H
