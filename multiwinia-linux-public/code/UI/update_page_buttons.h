#ifndef _included_update_page_buttons_h
#define _included_update_page_buttons_h

class UpdatePageButton : public GameMenuButton
{
public:
	UpdatePageButton()
	:	GameMenuButton( UnicodeString() )
	{
	}

	void MouseUp()
	{
		GameMenuWindow *window = (GameMenuWindow *)m_parent;
		window->m_newPage = GameMenuWindow::PageUpdateAvailable;

		GameMenuButton::MouseUp();
	}
};

class RunAutoPatcherButton : public GameMenuButton
{
public:
	RunAutoPatcherButton()
	:	GameMenuButton(UnicodeString())
	{
	}

#if defined(TARGET_MSVC) 
	void MouseUp()
	{
		STARTUPINFO si;
		ZeroMemory(&si,sizeof(si));
		si.cb = sizeof(si);
		PROCESS_INFORMATION pi;
		ZeroMemory(&pi,sizeof(pi));
		CreateProcess("autopatch.exe",NULL,NULL,NULL,false,0,NULL,NULL,&si,&pi);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		exit(0);
	}
#endif
};

#ifdef TARGET_OS_MACOSX
extern void PerformSparkleCheck();

class RunSparkleButton : public GameMenuButton
{
public:
	RunSparkleButton()
	:	GameMenuButton(UnicodeString())
	{
	}

	void MouseUp()
	{
		PerformSparkleCheck();
	}
};
#endif

class VisitWebsiteButton : public GameMenuButton
{
public:
	char *m_url;
	VisitWebsiteButton()
	:	GameMenuButton( UnicodeString() ),
		m_url(NULL)
	{
	}

	void MouseUp()
	{
		if( m_url )
		{
			g_windowManager->OpenWebsite( m_url );
		}
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
		if( !m_url )
		{
			Directory *updateUrlDir = MetaServer_RequestData( NET_METASERVER_DATA_UPDATEURL );
			if( updateUrlDir )
			{
				m_url = strdup(updateUrlDir->GetDataString( NET_METASERVER_DATA_UPDATEURL ));
				delete updateUrlDir;
			}
		}
		GameMenuButton::Render( realX, realY, highlighted, clicked );
	}
};

#endif