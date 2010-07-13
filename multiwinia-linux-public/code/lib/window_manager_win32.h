#ifndef INCLUDED_WINDOW_MANAGER_WIN32_H
#define INCLUDED_WINDOW_MANAGER_WIN32_H

#include "lib/window_manager.h"

class WindowManagerWin32 : public WindowManager
{
protected:
	bool		m_waitVRT;

	int			m_borderWidth;
	int			m_titleHeight;
	
	void UseSecondaryContext();
	void ReleaseSecondaryContext();
	void DestroySecondaryContext();
	
protected:
	HWND		m_hWnd;
	HDC			m_hDC;
	HGLRC		m_hRC;
	HGLRC		m_hRC2;

public:
	WindowManagerWin32();
	~WindowManagerWin32();
	void SaveDesktop();
	void RestoreDesktop();
	bool EnableOpenGL(int _colourDepth, int _zDepth);
	void DisableOpenGL();
	void ListAllDisplayModes();
	bool CreateWin(int _width, int _height, bool _windowed,
			   int _colourDepth, int _refreshRate, int _zDepth, bool _waitVRT,
			   bool _antiAlias, const wchar_t *_title);
	void DestroyWin();
	PlatformWindow *Window();
	void Flip();
	void NastyPollForMessages();
	void NastySetMousePos(int x, int y);
	void NastyMoveMouse(int x, int y);
	bool Captured();
	void CaptureMouse();
	void UncaptureMouse();
	void HideMousePointer();
	void UnhideMousePointer();
	void WindowMoved();
	void OpenWebsite( const char *_url );
};

void AppMain();

#endif
