// XCrashReport.cpp  Version 1.0
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
#include "XCrashReport.h"
#include "Dlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CApp

BEGIN_MESSAGE_MAP(CApp, CWinApp)
	//{{AFX_MSG_MAP(CApp)
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CApp construction

CApp::CApp()
{
	m_pszModule = NULL;
	m_pszCrashID = NULL;
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CApp object

CApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CApp initialization

char g_appMagicPrefix[32], g_appVersion[32];

BOOL CApp::InitInstance()
{
#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

#ifdef _DEBUG
	for (int i = 0; i < __argc; i++)
		TRACE(_T("argv[%d]=<%s>\n"), i, __targv[i]);
#endif

	if (__argc < 2)
	{
#ifdef _DEBUG
		m_pszModule = _T("darwinia.exe");
#else
		return FALSE;
#endif
	}
	else
	{
		m_pszModule = __targv[1];
	}

	if (__argc < 3)
	{
#ifdef _DEBUG
		m_pszCrashID = _T("");
#else
		return FALSE;
#endif
	}
	else
	{
		m_pszCrashID = __targv[2];
	}

	// TODO:
	// We need to set up 	TCHAR *m_pszMagicPrefix;
	// TCHAR *m_pszAppVersion;

	if (__argc < 5)
		return FALSE;

	m_pszMagicPrefix = __targv[3];
	m_pszAppVersion = __targv[4];

	CDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	return FALSE;
}
