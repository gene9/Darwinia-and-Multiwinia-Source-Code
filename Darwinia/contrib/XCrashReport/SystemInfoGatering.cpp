// SystemInfoGatering.cpp : implementation file
//

#include "stdafx.h"
#include "XCrashReport.h"
#include "SystemInfoGatering.h"
#include "MiniVersion.h"

// SystemInfoGatering dialog

//IMPLEMENT_DYNAMIC(SystemInfoGatering, CDialog)

/////////////////////////////////////////////////////////////////////////////
// SystemInfoGatering dialog

BEGIN_MESSAGE_MAP(SystemInfoGatering, CDialog)
	//{{AFX_MSG_MAP(SystemInfoGatering)
	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

SystemInfoGatering::SystemInfoGatering(CWnd* pParent /*=NULL*/)
	: CDialog(SystemInfoGatering::IDD, pParent)
{
	//{{AFX_DATA_INIT(SystemInfoGatering)
	//}}AFX_DATA_INIT

	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	memset ( &ShExecInfo, 0, sizeof(SHELLEXECUTEINFO) );

	waitClock = 0;

}

SystemInfoGatering::~SystemInfoGatering()
{
}

void SystemInfoGatering::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlg)
	DDX_Control(pDX, IDC_PROGRESS1, m_Progress1);
	//}}AFX_DATA_MAP
}

// SystemInfoGatering message handlers

///////////////////////////////////////////////////////////////////////////////
// OnInitDialog
BOOL SystemInfoGatering::OnInitDialog()
{

	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	ZeroMemory(m_szModulePath, sizeof(m_szModulePath));
	::GetModuleFileName(NULL, m_szModulePath, _countof(m_szModulePath)-2);
	TCHAR *cp = _tcsrchr(m_szModulePath, _T('\\'));
	if (*cp)
	{
		*(cp+1) = _T('\0');		// remove file name
	}

	InitializeDisplay();

	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpVerb = NULL;

	ShExecInfo.lpFile = "dxdiag.exe";
	CString parameters("/t ");
	parameters += m_szModulePath;
	parameters += "dxdiag_info.txt";
	ShExecInfo.lpParameters = parameters;

	//ShExecInfo.lpFile = "msinfo32.exe";
	//CString parameters("/nfo ");
	//parameters += m_szModulePath;
	//parameters += "msinfo32_info.nfo";
	//parameters += " /categories +Components-ComponentsMultimedia-ComponentsInfrared-ComponentsModem-ComponentsPorts-ComponentsPrinting-ComponentsProblemDevices-ComponentsUSB+SWEnvDrivers+SWEnvSignedDrivers";
	//ShExecInfo.lpParameters = parameters;
	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_HIDE;
	ShExecInfo.hInstApp = NULL;

	if ( !ShellExecuteEx(&ShExecInfo) ) {
		ShExecInfo.hProcess = 0;
	}

	SetTimer(1, 500, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control

}

///////////////////////////////////////////////////////////////////////////////
// InitializeDisplay
void SystemInfoGatering::InitializeDisplay()
{
	// get path to exe that crashed - we assume it is in same directory
	CString strExe = m_szModulePath;
	strExe += theApp.m_pszModule;

	// get version resource
	CMiniVersion ver(strExe);
	TCHAR szBuf[1000];
	szBuf[0] = _T('\0');

	// try to get description
	ver.GetFileDescription(szBuf, _countof(szBuf)-2);

	// if that failed try to get product name
	if (szBuf[0] == _T('\0'))
		ver.GetProductName(szBuf, _countof(szBuf)-2);

	ver.Release();

	// if that failed just use exe name
	if (szBuf[0] == _T('\0'))
		_tcscpy(szBuf, theApp.m_pszModule);

	SetWindowText(szBuf);

	m_Progress1.SetRange(0, MAX_WAIT);
	m_Progress1.SetStep(1);
	m_Progress1.SetPos(0);

}

///////////////////////////////////////////////////////////////////////////////
// OnTimer
void SystemInfoGatering::OnTimer(UINT /*nIDEvent*/) 
{
	
	bool exitSystemInfo = false;

	waitClock++;
	if ( ShExecInfo.hProcess == 0 || waitClock >= MAX_WAIT ) {
		exitSystemInfo = true;
	}
	else {
		DWORD ret;
		ret = WaitForSingleObject ( ShExecInfo.hProcess, 0 );
		if ( ret == WAIT_OBJECT_0 || ret == WAIT_ABANDONED )
			exitSystemInfo = true;
	}

	if ( exitSystemInfo ) {
		CDialog::OnOK();
	}
	else {
		m_Progress1.SetPos(waitClock);
	}

}
