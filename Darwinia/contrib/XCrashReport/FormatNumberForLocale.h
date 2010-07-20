// FormatNumberForLocale.h  Version 1.0
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

#ifndef FORMATNUMBERFORLOCALE_H
#define FORMATNUMBERFORLOCALE_H

class CFormatNumberForLocale
{
public:
	CFormatNumberForLocale()
	{

	}

	static CString FormatNumber(LPCTSTR lpszNumber)
	{
		CString str = lpszNumber;

		if (!m_bInitialized)
			InitNumberFormatForLocale();

		TCHAR buf[200];
		if (GetNumberFormat(LOCALE_SYSTEM_DEFAULT, 
							0, 
							lpszNumber, 
							&m_nf, 
							buf, 
							_countof(buf)-2))
		{
			str = buf;
		}

		return str;
	}

protected:

	static void InitNumberFormatForLocale()
	{
		// initialize number format for GetNumberFormat()

		m_nf.NumDigits     = 0;
		m_nf.LeadingZero   = 0;
		m_nf.Grouping      = 3;
		m_nf.lpDecimalSep  = _T(".");
		m_nf.lpThousandSep = _T(",");
		m_nf.NegativeOrder = 1;

		lstrcpy(m_szDecimalSep,  _T("."));
		lstrcpy(m_szThousandSep, _T(","));

		if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_SDECIMAL, 
			m_szDecimalSep, _countof(m_szDecimalSep)))
		{
			m_nf.lpDecimalSep = m_szDecimalSep;
		}
		if (GetLocaleInfo(LOCALE_SYSTEM_DEFAULT, LOCALE_STHOUSAND, 
			m_szThousandSep, _countof(m_szThousandSep)))
		{
			m_nf.lpThousandSep = m_szThousandSep;
		}
		m_bInitialized = TRUE;
	}

	static BOOL      m_bInitialized;
	static NUMBERFMT m_nf;
	static TCHAR     m_szThousandSep[20];
	static TCHAR     m_szDecimalSep[20];
};

#endif //FORMATNUMBERFORLOCALE_H
