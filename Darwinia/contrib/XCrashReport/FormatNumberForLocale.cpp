// FormatNumberForLocale.cpp  Version 1.0
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
#include "FormatNumberForLocale.h"

BOOL      CFormatNumberForLocale::m_bInitialized = FALSE;
NUMBERFMT CFormatNumberForLocale::m_nf = { 0 };
TCHAR     CFormatNumberForLocale::m_szThousandSep[20] = { 0 };
TCHAR     CFormatNumberForLocale::m_szDecimalSep[20] = { 0 };

