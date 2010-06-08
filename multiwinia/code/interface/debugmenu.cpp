#include "lib/universal_include.h"
#include "lib/debug_utils.h"
//#include "lib/input.h"
#include "lib/text_renderer.h"
#include "lib/preferences.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"

#include "interface/debugmenu.h"
#include "interface/gesture_window.h"
#include "interface/grabber_window.h"
#include "interface/network_window.h"
#include "interface/prefs_screen_window.h"
#include "interface/prefs_graphics_window.h"
#include "interface/prefs_sound_window.h"
#include "interface/profilewindow.h"
#include "interface/soundeditor_window.h"
#include "interface/sound_profile_window.h"
#include "interface/soundstats_window.h"
#include "interface/cheat_window.h"
#include "interface/userprofile_window.h"
#include "interface/reallyquit_window.h"

#include "app.h"
#include "camera.h"
#include "renderer.h"
#include "user_input.h"

// ****************************************************************************
// Menu Buttons
// ****************************************************************************

class ProfileButton : public DarwiniaButton
{
public:
	void MouseUp()
    {
		DebugKeyBindings::ProfileButton();
    }
};


class NetworkButton: public DarwiniaButton
{
public:
    void MouseUp()
    {
		DebugKeyBindings::NetworkButton();
    }
};


#ifdef LOCATION_EDITOR
class EditorButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
		DebugKeyBindings::EditorButton();
    }
};
#endif // LOCATION_EDITOR


class DebugCameraButton: public DarwiniaButton
{
public:
	void MouseUp()
	{
		DebugKeyBindings::DebugCameraButton();
	}
};


class FPSButton: public DarwiniaButton
{
public:
	void MouseUp()
	{
		DebugKeyBindings::FPSButton();
	}
};


class PrefsScreenButton: public DarwiniaButton
{
public:
	void MouseUp()
	{
		if (!EclGetWindow("dialog_screenoptions"))
		{
			EclRegisterWindow(new PrefsScreenWindow());
		}
	}
};


class PrefsGfxDetailButton: public DarwiniaButton
{
public:
	void MouseUp()
	{
		if (!EclGetWindow("dialog_graphicsoptions"))
		{
			EclRegisterWindow(new PrefsGraphicsWindow());
		}
    }
};


class PrefsSoundButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        if(!EclGetWindow("dialog_soundoptions"))
        {
            EclRegisterWindow(new PrefsSoundWindow());
        }
    };
};


#ifdef SOUND_EDITOR
class PokeyEditorButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::PokeyButton();
    }
};
#endif // SOUND_EDITOR


#if defined(GESTURE_EDITOR) && defined(USE_SEPULVEDA_HELP_TUTORIAL)
class GestureEditorButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::GestureButton();
    }
};
#endif // GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL


#ifdef CHEATMENU_ENABLED
class CheatButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::CheatButton();
    }
};
#endif


#ifdef SOUND_EDITOR
class SoundStatsButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::SoundStatsButton();
    }
};
#endif // SOUND_EDITOR


#ifdef SOUND_EDITOR
class SoundProfileButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::SoundProfileButton();
    }
};
#endif // SOUND_EDITOR


#ifdef SOUND_EDITOR
class SoundEditorButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        DebugKeyBindings::SoundEditorButton();
    }
};
#endif // SOUND_EDITOR


// ****************************************************************************
// Class DebugMenu
// ****************************************************************************

DebugMenu::DebugMenu( char *name )
:   DarwiniaWindow( name )
{
	m_x = 10;
	m_y = 20;
	m_w = 170;
	m_h = 75;
}


void DebugMenu::Advance()
{
}


void DebugMenu::Create()
{
	DarwiniaWindow::Create();

    int pitch = 18;
	int y = 5;

	DarwiniaButton *button;

	button = new ProfileButton();
    button->SetShortProperties( "Profile (F6)", 10, y += pitch, m_w - 20, -1, UnicodeString("Profile (F6)") );
    RegisterButton( button );
	m_buttonOrder.PutData( button );
    
    button = new NetworkButton();
    button->SetShortProperties( "Network Stats", 10, y += pitch, m_w - 20, -1, UnicodeString("Network Stats") );
    RegisterButton( button );
	m_buttonOrder.PutData( button );

	button = new FPSButton();
	button->SetShortProperties("Display FPS (F5)", 10, y += pitch, m_w - 20, -1, UnicodeString("Display FPS (F5)") );
	RegisterButton( button );
	m_buttonOrder.PutData( button );

	y += pitch / 2.0;
    
	y += pitch / 2.0;

	button = new DebugCameraButton();
	button->SetShortProperties("Dbg Cam (F2)", 10, y += pitch, m_w - 20, -1, UnicodeString("Dbg Cam (F2)") );
	RegisterButton( button );
	m_buttonOrder.PutData( button );

	y += pitch / 2.0;

#ifdef USE_DARWINIA_MOD_SYSTEM
    bool modsEnabled = g_prefsManager->GetInt( "ModSystemEnabled", 0 ) != 0;
#else
    bool modsEnabled = false;
#endif
	
#ifdef LOCATION_EDITOR
    if( modsEnabled )
    {
        button = new EditorButton();
	    button->SetShortProperties("Toggle Editor (F3)", 10, y += pitch, m_w - 20, -1, UnicodeString("Toggle Editor (F3)"));
	    RegisterButton(button);
		m_buttonOrder.PutData( button );
    }
#endif // LOCATION_EDITOR

#ifdef CHEATMENU_ENABLED
    button = new CheatButton();
    button->SetShortProperties("Cheat Menu (F4)", 10, y += pitch, m_w - 20, -1, UnicodeString("Cheat Menu (F4)") );
    RegisterButton( button );
#endif

#if defined(GESTURE_EDITOR) && defined(USE_SEPULVEDA_HELP_TUTORIAL)
    if( modsEnabled )
    {
        button = new GestureEditorButton();
        button->SetShortProperties("Gesture Editor", 10, y += pitch, m_w - 20, -1, UnicodeString("Gesture Editor") );
        RegisterButton( button );
		m_buttonOrder.PutData( button );
    }
#endif // GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL

	y += pitch / 2.0;

#ifdef SOUND_EDITOR
    if( modsEnabled )
    {
        button = new PokeyEditorButton();
        button->SetShortProperties("Pokey Playground", 10, y += pitch, m_w - 20, -1, UnicodeString("Pokey Playground") );
        RegisterButton( button );
		m_buttonOrder.PutData( button );
    }
#endif // SOUND_EDITOR

#ifdef SOUND_EDITOR
    if( modsEnabled )
    {
        button = new SoundStatsButton();
        button->SetShortProperties("Sound Stats (F7)", 10, y += pitch, m_w - 20, -1, UnicodeString("Sound Stats(F7)") );
        RegisterButton( button );
		m_buttonOrder.PutData( button );
    }
#endif // SOUND_EDITOR

#ifdef SOUND_EDITOR
    if( modsEnabled )
    {
        button = new SoundEditorButton();
        button->SetShortProperties("Sound Editor (F8)", 10, y += pitch, m_w - 20, -1, UnicodeString("Sound Editor (F8)") );
        RegisterButton( button );
		m_buttonOrder.PutData( button );
    }
#endif // SOUND_EDITOR

#ifdef SOUND_EDITOR
    if( modsEnabled )
    {
        button = new SoundProfileButton();
        button->SetShortProperties("Sound Profile (F9)", 10, y += pitch, m_w - 20, -1, UnicodeString("Sound Profile (F8)") );
        RegisterButton( button );
		m_buttonOrder.PutData( button );
    }
#endif // SOUND_EDITOR
}


void DebugMenu::Render(bool hasFocus)
{
	Advance();

	DarwiniaWindow::Render(hasFocus);

	EclButton *camDbgButton = GetButton("Dbg Cam (F2)");
	AppDebugAssert(camDbgButton);
	int y = m_y + camDbgButton->m_y + 11;

	switch (g_app->m_camera->GetDebugMode())
	{
		case Camera::DebugModeAlways:	
			g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Always"); 
			break;
		case Camera::DebugModeAuto:		
			g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Auto"); 
			break;
		case Camera::DebugModeNever:	
			g_editorFont.DrawText2D(m_x + m_w - 47, y, 10, "Never"); 
			break;
	}
}


// ****************************************************************************
// Class DebugKeyBindings
// ****************************************************************************

void DebugKeyBindings::DebugMenu()
{
	char *debugMenuWindowName = "dialog_toolsmenu";
	if (EclGetWindow(debugMenuWindowName))
		EclRemoveWindow(debugMenuWindowName);
	else
		EclRegisterWindow(new ::DebugMenu(debugMenuWindowName));
}

void DebugKeyBindings::ProfileButton()
{
    if( EclGetWindow("Profiler") )
	{
		EclRemoveWindow("Profiler");
	}
	else
    {
        ProfileWindow *pw = new ProfileWindow("Profiler");
        pw->m_w = 570;
        pw->m_h = 600;
        pw->m_x = g_app->m_renderer->ScreenW() - pw->m_w - 20;
        pw->m_y = 30;
        EclRegisterWindow(pw);
    }
}


void DebugKeyBindings::NetworkButton()
{
    if (!EclGetWindow("Network Stats") )
    {
        NetworkWindow *nw = new NetworkWindow("Network Stats");
        EclRegisterWindow(nw);
    }
}


#ifdef LOCATION_EDITOR
void DebugKeyBindings::EditorButton()
{
	g_app->m_requestToggleEditing = true;
}
#endif // LOCATION_EDITOR


#ifdef AVI_GENERATOR
void DebugKeyBindings::GrabberButton()
{
    if( !EclGetWindow("Grabber") )
    {
        GrabberWindow *gw = new GrabberWindow("Grabber");
        gw->m_w = 200;
        gw->m_h = 50;
        gw->m_x = 10;
        gw->m_y = g_app->m_renderer->ScreenH() - gw->m_h;
        EclRegisterWindow( gw );
    }
}
#endif // AVI_GENERATOR

void DebugKeyBindings::DebugCameraButton()
{
	g_app->m_camera->SetNextDebugMode();
}

void DebugKeyBindings::FPSButton()
{
	g_app->m_renderer->m_displayFPS = !g_app->m_renderer->m_displayFPS;
}


#ifdef SOUND_EDITOR
void DebugKeyBindings::PokeyButton()
{
    if (!EclGetWindow("Pokey Playground") )
    {
	    PokeyWindow *pokeyWin = new PokeyWindow("Pokey Playground");
	    pokeyWin->m_w = 400;
	    pokeyWin->m_h = 480;
	    pokeyWin->m_x = 10;
	    pokeyWin->m_y = 40;
	    EclRegisterWindow(pokeyWin);                
    }
}
#endif // SOUND_EDITOR


#if defined(GESTURE_EDITOR) && defined(USE_SEPULVEDA_HELP_TUTORIAL)
void DebugKeyBindings::GestureButton()
{
    if (!EclGetWindow("Gesture Editor") )
    {
        GestureWindow *gesture = new GestureWindow("Gesture Editor");
        gesture->m_w = 660;
        gesture->m_h = 660;
        gesture->m_x = 30;
        gesture->m_y = 30;
        EclRegisterWindow(gesture);
    }
}
#endif // GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL


#ifdef CHEATMENU_ENABLED
void DebugKeyBindings::CheatButton()
{
    if( !EclGetWindow("Cheat Window") )
    {
        CheatWindow *window = new CheatWindow("Cheat Window" );
        window->m_w = 200;
        window->m_h = 200;
        window->m_x = 250;
        window->m_y = 50;
        EclRegisterWindow( window );
    }
}
#endif

#ifdef SOUND_EDITOR
void DebugKeyBindings::SoundStatsButton()
{
    if( EclGetWindow(SOUND_STATS_WINDOW_NAME) )
	{
		EclRemoveWindow(SOUND_STATS_WINDOW_NAME);
	}
	else
    {
        SoundStatsWindow *window = new SoundStatsWindow(SOUND_STATS_WINDOW_NAME);
        EclRegisterWindow(window);
    }
}
#endif // SOUND_EDITOR


#ifdef SOUND_EDITOR
void DebugKeyBindings::SoundProfileButton()
{
    if( EclGetWindow(SOUND_PROFILE_WINDOW_NAME ) )
	{
		EclRemoveWindow(SOUND_PROFILE_WINDOW_NAME);
	}
	else
    {
        SoundProfileWindow *window = new SoundProfileWindow(SOUND_PROFILE_WINDOW_NAME);
        EclRegisterWindow(window);
    }
}
#endif // SOUND_EDITOR


#ifdef SOUND_EDITOR
void DebugKeyBindings::SoundEditorButton()
{
    if(EclGetWindow(SOUND_EDITOR_WINDOW_NAME))
	{
		EclRemoveWindow(SOUND_EDITOR_WINDOW_NAME);
	}
	else
    {
        SoundEditorWindow *sound = new SoundEditorWindow(SOUND_EDITOR_WINDOW_NAME);
        sound->m_w = 500;
        sound->m_h = 550;
        sound->m_x = 50;
        sound->m_y = 25;
        EclRegisterWindow( sound );
    }
}
#endif // SOUND_EDITOR

void DebugKeyBindings::ReallyQuitButton()
{
	// Bring up a really quit window
	if (!EclGetWindow(REALLYQUIT_WINDOWNAME)) 
		EclRegisterWindow( new ReallyQuitWindow() );
}

void DebugKeyBindings::ToggleFullscreenButton()
{
	bool switchingToWindowed;
	SetWindowed(!g_windowManager->Windowed(), true, switchingToWindowed);
}
