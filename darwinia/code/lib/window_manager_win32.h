#ifndef INCLUDED_WINDOW_MANAGER_WIN32_H
#define INCLUDED_WINDOW_MANAGER_WIN32_H

class WindowManagerWin32
{
public:
	HWND		m_hWnd;
	HDC			m_hDC;
	HGLRC		m_hRC;

	WindowManagerWin32()
	:	m_hWnd(NULL),
		m_hDC(NULL),
		m_hRC(NULL)
	{
	}
};

#endif
