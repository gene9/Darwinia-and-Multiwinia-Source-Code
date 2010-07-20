// Details.cpp  Version 1.0
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
#include "Details.h"
#include <io.h>
#include "IsAsciiFile.h"
#include "GetFilePart.h"
#include "strcvt.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_FILE_SIZE_TO_DISPLAY	(64 * 1024)

/////////////////////////////////////////////////////////////////////////////
// CDetails dialog

BEGIN_MESSAGE_MAP(CDetails, CDialog)
	//{{AFX_MSG_MAP(CDetails)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblClick)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CDetails::CDetails(CDWordArray * paFileDetails, CWnd* pParent /*=NULL*/)
	: CDialog(CDetails::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDetails)
	//}}AFX_DATA_INIT
	m_paFileDetails = paFileDetails;
}

CDetails::~CDetails()
{
	if (m_FileContentsFont.GetSafeHandle())
		m_FileContentsFont.DeleteObject();
}

void CDetails::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDetails)
	DDX_Control(pDX, IDC_STATIC_DETAILS_BANNER, m_Banner);
	DDX_Control(pDX, IDC_STATIC_NAME, m_Name);
	DDX_Control(pDX, IDC_STATIC_DESC, m_Desc);
	DDX_Control(pDX, IDC_CONTENTS, m_FileContents);
	DDX_Control(pDX, IDC_LIST, m_FileList);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CDetails message handlers

BOOL CDetails::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	ASSERT(m_paFileDetails);

	LOGFONT lf;
	CFont *pFont = NULL;

	m_Banner.SetTextColor(RGB(0,0,255), FALSE);
	m_Banner.SetBackgroundColor(RGB(255,255,255), FALSE);
	m_Banner.SetMargins(0, 4);
	pFont = m_Banner.GetFont();
	pFont->GetLogFont(&lf);
	lf.lfHeight = (lf.lfHeight > 0) ? lf.lfHeight+2 : lf.lfHeight-2;
	m_Banner.SetFont(&lf, FALSE);
	m_Banner.SetWindowText(_T("    Please uncheck any files you do not wish to send."));

	pFont = m_FileContents.GetFont();
	pFont->GetLogFont(&lf);
	_tcscpy(lf.lfFaceName, _T("Courier New"));
	m_FileContentsFont.CreateFontIndirect(&lf);
	m_FileContents.SetFont(&m_FileContentsFont);

	m_FileContents.SetLimitText(0xFFFFFFF);

	InitListCtrl(m_FileList);

	FillList(m_FileList);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDetails::InitListCtrl(CXListCtrl& list)
{
	TRACE(_T("in CPageFileMasks::InitListCtrl\n"));

	list.SetExtendedStyle(LVS_EX_FULLROWSELECT); // | LVS_EX_GRIDLINES);

	// call EnableToolTips to enable tooltip display
	list.EnableToolTips(TRUE);

	// set column width according to window rect
	CRect rect;
	list.GetWindowRect(&rect);

	int w = rect.Width() - 2;
	int colwidths[5] = { 9, 18, 17, 12, 8 };	// sixty-fourths

	TCHAR *	lpszHeaders[] = { _T("Send"),
							  _T("Name"),
							  _T("Description"),
							  _T("Type"),
							  _T("Size"),
							  NULL };
	int i;
	int total_cx = 0;
	LV_COLUMN lvcolumn;
	memset(&lvcolumn, 0, sizeof(lvcolumn));

	// add columns
	for (i = 0; ; i++)
	{
		if (lpszHeaders[i] == NULL)
			break;

		lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		lvcolumn.fmt = (i == 4) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		lvcolumn.pszText = lpszHeaders[i];
		lvcolumn.iSubItem = i;
		lvcolumn.cx = (lpszHeaders[i+1] == NULL) ? w - total_cx - 2 : (w * colwidths[i]) / 64;
		total_cx += lvcolumn.cx;
		list.InsertColumn(i, &lvcolumn);
	}

	// create the image list from bitmap resource
	VERIFY(list.m_cImageList.Create(IDB_CHECKBOXES, 16, 3, RGB(255, 0, 255)));
	list.m_HeaderCtrl.SetImageList(&list.m_cImageList);

	// iterate through header items and attach the image list
	HDITEM hditem;

	for (i = 0; i < list.m_HeaderCtrl.GetItemCount(); i++)
	{
		hditem.mask = HDI_IMAGE | HDI_FORMAT;
		list.m_HeaderCtrl.GetItem(i, &hditem);
		hditem.fmt |=  HDF_IMAGE;
		if (i == 0)
			hditem.iImage = XHEADERCTRL_UNCHECKED_IMAGE;
		else
			hditem.iImage = XHEADERCTRL_NO_IMAGE;

		list.m_HeaderCtrl.SetItem(i, &hditem);
	}

	memset(&lvcolumn, 0, sizeof(lvcolumn));

	// set the format again - must do this twice or first column does not get set
	for (i = 0; ; i++)
	{
		if (lpszHeaders[i] == NULL)
			break;

		lvcolumn.mask = LVCF_FMT | LVCF_SUBITEM;
		lvcolumn.fmt = (i == 4) ? LVCFMT_RIGHT : LVCFMT_LEFT;
		lvcolumn.iSubItem = i;
		list.SetColumn(i, &lvcolumn);
	}
}

void CDetails::FillList(CXListCtrl& list)
{
	list.DeleteAllItems();

	for (int nItem = 0; nItem < m_paFileDetails->GetSize(); nItem++)
	{
		FILEDETAILS *fd = (FILEDETAILS *) m_paFileDetails->GetAt(nItem);

		// send column
		list.InsertItem(nItem, _T(""));
		list.SetCheckbox(nItem, 0, fd->bSend);

		// name column
		list.SetItemText(nItem, 1, GetFilePart(fd->strFilePath));

		list.SetItemText(nItem, 2, fd->strDescription);
		list.SetItemText(nItem, 3, fd->strType);
		list.SetItemText(nItem, 4, fd->strFileSize);
	}
}

void CDetails::OnOK() 
{
	// update status of bSend data member

	for (int nItem = 0; nItem < m_paFileDetails->GetSize(); nItem++)
	{
		FILEDETAILS *fd = (FILEDETAILS *) m_paFileDetails->GetAt(nItem);

		fd->bSend = m_FileList.GetCheckbox(nItem, 0);
	}
	
	CDialog::OnOK();
}

void CDetails::DisplayFile(FILEDETAILS *fd)
{
	if (_taccess(fd->strFilePath, 00) == -1)
		return;

	// open file to get its size
	HANDLE hFile = CreateFile(
		fd->strFilePath,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		TRACE(_T("ERROR - failed to open %s\n"), fd->strFilePath);
		return;
	}

	DWORD dwFileSize = GetFileSize(hFile, NULL);

	char *pBuf = NULL;

	if (dwFileSize == INVALID_FILE_SIZE || dwFileSize == 0)
	{
		TRACE(_T("ERROR - GetFileSize failed or returned 0\n"));
	}
	else
	{
		CWaitCursor wait;

		if (dwFileSize > MAX_FILE_SIZE_TO_DISPLAY)
			dwFileSize = MAX_FILE_SIZE_TO_DISPLAY;

		pBuf = new char [dwFileSize + 32];
		ZeroMemory(pBuf, dwFileSize + 32);
		
		DWORD dwBytesRead = 0;

		BOOL bRetVal = ReadFile(hFile,			// handle to file
								pBuf,			// data buffer
								dwFileSize,		// number of bytes to read
								&dwBytesRead,	// number of bytes read
								NULL);			// overlapped buffer

		if (bRetVal && dwBytesRead != 0)
		{
			m_Name.SetWindowText(GetFilePart(fd->strFilePath));
			m_Desc.SetWindowText(fd->strDescription);

			if (IsAsciiFile(pBuf))
			{
#ifdef _UNICODE
				WCHAR *pBufW = new WCHAR[dwFileSize + 32];
				ASSERT(pBufW);
				memset(pBufW, 0, sizeof((dwFileSize + 32)*sizeof(WCHAR)));
				xmbstowcsz(pBufW, pBuf, dwFileSize+2);
				m_FileContents.SetWindowText(pBufW);
				delete [] pBufW;
#else
				m_FileContents.SetWindowText(pBuf);
#endif
			}
			else
			{
				DisplayHex(pBuf, dwBytesRead);
			}
		}
	}

	CloseHandle(hFile);

	if (pBuf)
		delete [] pBuf;
}

void CDetails::DisplayHex(const char * lpBuf, DWORD dwSize)
{
	DWORD *pdwBuf = (DWORD *) lpBuf;

	// round up to 4 dwords (one line) - this works because the buffer
	// is larger than the file
	DWORD dwDwords = (dwSize + (16 - (dwSize % 16))) / 4;
	int nDwordsPrinted = 0;
	DWORD dwStartIndex = 0;
	CString strLine = _T("");
	CString strDword = _T("");

	{
		m_FileContents.LockWindowUpdate();

		for (DWORD dwIndex = 0; dwIndex < dwDwords; )
		{
			if (nDwordsPrinted == 0)
			{
				strLine.Format(_T("0x%08x: "), 4*dwIndex);
				dwStartIndex = dwIndex;
			}

			strDword.Format(_T("%08x "), pdwBuf[dwIndex++]);
			strLine += strDword;
			nDwordsPrinted++;
			if (nDwordsPrinted == 4)
			{
				// now display ascii

				for (int i = 0; i < 4; i++)
				{
					DWORD dwVal = pdwBuf[dwStartIndex++];
					for (int j = 0; j < 4; j++)
					{
						char c = (char)(dwVal & 0xFF);
						if (c < 0x20 || c > 0x7E)
							c = '.';
#ifdef _UNICODE
						WCHAR w = (WCHAR)c;
						strLine += w;
#else
						strLine += c;
#endif
						dwVal = dwVal >> 8;
					}
				}

				strLine += _T("\r\n");
				m_FileContents.SetSel(-1, -1, TRUE);
				m_FileContents.ReplaceSel(strLine);
				nDwordsPrinted = 0;
			}
		} // for (DWORD dwIndex = 0; dwIndex < dwSize; )

		strLine = _T("\r\n");
		m_FileContents.SetSel(-1, -1);
		m_FileContents.ReplaceSel(strLine);

		m_FileContents.UnlockWindowUpdate();
	}

	m_FileContents.SetSel(0, 0);
}

///////////////////////////////////////////////////////////////////////////////
// OnDblClick
void CDetails::OnDblClick(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LPNMITEMACTIVATE pNMIA = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nItem = -1;
	int nSubItem = -1;
	if (pNMIA)
	{
		nItem = pNMIA->iItem;
		nSubItem = pNMIA->iSubItem;
	}
	TRACE(_T("in CDetails::OnDblClick:  %d, %d\n"), nItem, nSubItem);

	m_FileContents.SetWindowText(_T(""));

	if ( 0 <= nItem && nItem < m_paFileDetails->GetSize () ) {

		FILEDETAILS *fd = (FILEDETAILS *) m_paFileDetails->GetAt(nItem);

		if (!fd)
		{
			TRACE(_T("ERROR - no file details\n"));
			ASSERT(FALSE);
			return;
		}

		DisplayFile(fd);

	}

}
