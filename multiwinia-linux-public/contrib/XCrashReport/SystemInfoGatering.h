#pragma once


// SystemInfoGatering dialog

class SystemInfoGatering : public CDialog
{

public:
	SystemInfoGatering(CWnd* pParent = NULL);   // standard constructor
	virtual ~SystemInfoGatering();

// Dialog Data
	//{{AFX_DATA(SystemInfoGatering)
	enum { IDD = IDD_SYSTEMINFO };
	CProgressCtrl			m_Progress1;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SystemInfoGatering)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON		m_hIcon;
	TCHAR		m_szModulePath[MAX_PATH*2];

	void InitializeDisplay();

	// Generated message map functions
	//{{AFX_MSG(SystemInfoGatering)
	virtual BOOL OnInitDialog();
	afx_msg void OnTimer(UINT nIDEvent);
	//}}AFX_MSG

	SHELLEXECUTEINFO ShExecInfo;

	static const int MAX_WAIT = 240;
	int waitClock;

	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

