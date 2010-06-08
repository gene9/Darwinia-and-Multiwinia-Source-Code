// Details.h  Version 1.0
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

#ifndef DETAILS_H
#define DETAILS_H

#include "XListCtrl.h"
#include "ROEdit.h"
#include "XColorStatic.h"

struct FILEDETAILS
{
	FILEDETAILS()
	{
		strFilePath    = _T("");
		strDescription = _T("");
		strFileSize    = _T("");
		strType        = _T("");
		bSend          = TRUE;
	}

	CString strFilePath;
	CString strDescription;
	CString strFileSize;
	CString strType;
	BOOL    bSend;
};

/////////////////////////////////////////////////////////////////////////////
// CDetails dialog

class CDetails : public CDialog
{
// Construction
public:
	CDetails(CDWordArray * paFileDetails, CWnd* pParent = NULL);
	virtual ~CDetails();

// Dialog Data
	//{{AFX_DATA(CDetails)
	enum { IDD = IDD_DETAILS };
	CXColorStatic	m_Banner;
	CStatic			m_Name;
	CStatic			m_Desc;
	CROEdit			m_FileContents;
	CXListCtrl		m_FileList;
	//}}AFX_DATA

	CDWordArray *	m_paFileDetails;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDetails)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CFont m_FileContentsFont;

	void DisplayFile(FILEDETAILS *fd);
	void DisplayHex(const char * lpBuf, DWORD dwSize);
	void FillList(CXListCtrl& list);
	void InitListCtrl(CXListCtrl& list);

	// Generated message map functions
	//{{AFX_MSG(CDetails)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	afx_msg void OnDblClick(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif //DETAILS_H
