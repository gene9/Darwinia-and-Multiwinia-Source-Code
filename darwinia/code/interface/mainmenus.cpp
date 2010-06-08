#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/language_table.h"
#include "lib/input/win32_eventhandler.h"

#include "interface/mainmenus.h"
#include "interface/prefs_screen_window.h"
#include "interface/prefs_graphics_window.h"
#include "interface/prefs_sound_window.h"
#include "interface/prefs_keybindings_window.h"
#include "interface/prefs_other_window.h"
#include "interface/userprofile_window.h"
#include "interface/mods_window.h"
#include "interface/demoend_window.h"

#include "app.h"
#include "renderer.h"
#include "global_world.h"
#include "script.h"

#include "lib/input/input.h"

class WebsiteButton;

class SkipPrologueWindowButton : public DarwiniaButton
{
	void MouseUp()
	{
		if( !EclGetWindow(LANGUAGEPHRASE("dialog_skipprologue") ) )
		{
			EclRegisterWindow( new SkipPrologueWindow(), m_parent );
		}
	}
};

class SkipPrologueButton : public DarwiniaButton
{
	void MouseUp()
	{
		LList<EclWindow *> *windows = EclGetWindows();
		while (windows->Size() > 0) {
			EclWindow *w = windows->GetData(0);
			EclRemoveWindow(w->m_name);
		}

        g_app->m_script->Skip();
        g_app->LoadCampaign();
	}
};

class PlayPrologueButton : public DarwiniaButton
{
	void MouseUp()
	{
		LList<EclWindow *> *windows = EclGetWindows();
		while (windows->Size() > 0) {
			EclWindow *w = windows->GetData(0);
			EclRemoveWindow(w->m_name);
		}

        g_app->m_script->Skip();
		g_app->LoadPrologue();		
	}
};

class PlayPrologueWindowButton : public DarwiniaButton
{
	void MouseUp()
	{
		if( !EclGetWindow( LANGUAGEPHRASE("dialogue_playprologue") ) )
		{
			EclRegisterWindow( new PlayPrologueWindow(), m_parent );
		}
	}
};

class AboutDarwiniaButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( !EclGetWindow(LANGUAGEPHRASE("about_darwinia") ) )
        {
            EclRegisterWindow( new AboutDarwiniaWindow(), m_parent );
        }
    }
};

class MainMenuUserProfileButton : public DarwiniaButton
{
    void MouseUp()
    {   
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_profile")))
		{
			EclRegisterWindow( new UserProfileWindow(), m_parent );
		}
    }
};


class ModsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_mods")))
		{
			EclRegisterWindow( new ModsWindow(), m_parent );
		}
    }
};


class OptionsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_options")))
		{
			EclRegisterWindow( new OptionsMenuWindow(), m_parent );
		}
    }
};


class ScreenOptionsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_screenoptions")))
		{
			EclRegisterWindow( new PrefsScreenWindow(), m_parent );
		}
    }
};


class GraphicsOptionsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_graphicsoptions")))
		{
			EclRegisterWindow( new PrefsGraphicsWindow(), m_parent );
		}
    }
};


class SoundOptionsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_soundoptions")))
		{
	        EclRegisterWindow( new PrefsSoundWindow(), m_parent );
		}
    }
};


class OtherOptionsButton : public DarwiniaButton
{
    void MouseUp()
    {
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_otheroptions")))
		{
			EclRegisterWindow( new PrefsOtherWindow(), m_parent );
		}
    }
};


class KeybindingsOptionsButton : public DarwiniaButton
{
	void MouseUp()
	{
		if (!EclGetWindow(LANGUAGEPHRASE("dialog_inputoptions")))
		{
			EclRegisterWindow( new PrefsKeybindingsWindow, m_parent );
		}
	}
};


// ****************************************************************************
// Class MainMenuWindow
// ****************************************************************************

MainMenuWindow::MainMenuWindow()
:   DarwiniaWindow(LANGUAGEPHRASE("dialog_mainmenu"))
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();
    
	SetMenuSize( 220, 260 );
    SetPosition( screenW/2.0f - m_w/2.0f,
                 screenH/2.0f - m_h/2.0f );
}

void MainMenuWindow::Render( bool _hasFocus )
{
	DarwiniaWindow::Render(_hasFocus);
}



// ***************************************************************************
// Class OptionsMenuWindow
// ***************************************************************************

OptionsMenuWindow::OptionsMenuWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_options") )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    SetMenuSize( 240, 230 );
//    SetPosition( screenW/2.0f - m_w/2.0f,
//                 screenH/2.0f - m_h/2.0f );
}


void OptionsMenuWindow::Create()
{
    DarwiniaWindow::Create();

    int fontSize = GetMenuSize(13);
	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - border * 2;
	int h = buttonH + border;	

    ScreenOptionsButton *screen = new ScreenOptionsButton();
    screen->SetShortProperties( LANGUAGEPHRASE("dialog_screenoptions"), border, y+=border, buttonW, buttonH );
    screen->m_fontSize = fontSize;
    screen->m_centered = true;
    RegisterButton( screen );
	m_buttonOrder.PutData( screen );

    GraphicsOptionsButton *graphics = new GraphicsOptionsButton();
    graphics->SetShortProperties( LANGUAGEPHRASE("dialog_graphicsoptions"), border, y+=h, buttonW, buttonH );
    graphics->m_fontSize = fontSize;
    graphics->m_centered = true;
    RegisterButton( graphics );
	m_buttonOrder.PutData( graphics );

    SoundOptionsButton *sound = new SoundOptionsButton();
    sound->SetShortProperties( LANGUAGEPHRASE("dialog_soundoptions"), border, y+=h, buttonW, buttonH );
    sound->m_fontSize = fontSize;
    sound->m_centered = true;
    RegisterButton( sound );
	m_buttonOrder.PutData( sound );

	KeybindingsOptionsButton *keys = new KeybindingsOptionsButton();
	keys->SetShortProperties( LANGUAGEPHRASE("dialog_inputoptions"), border, y+=h, buttonW, buttonH);
	keys->m_fontSize = fontSize;
	keys->m_centered = true;
	RegisterButton( keys );
	m_buttonOrder.PutData( keys );

    OtherOptionsButton *other = new OtherOptionsButton();
    other->SetShortProperties( LANGUAGEPHRASE("dialog_otheroptions"), border, y+=h, buttonW, buttonH );
    other->m_fontSize = fontSize;
	other->m_centered = true;
    RegisterButton( other );
	m_buttonOrder.PutData( other );

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
    close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );

}


// ============================================================================

class ResetLevelButton : public DarwiniaButton
{
    void MouseUp()
    {
        EclRegisterWindow( new ResetLocationWindow(), m_parent );
    }
};


class ExitLevelButton : public DarwiniaButton
{
    void MouseUp()
    {
        EclRemoveWindow( m_parent->m_name );
        
		if (!g_app->HasBoughtGame()) 
		{
			EclRegisterWindow( new DemoEndWindow(1.0f, true) );
		}
		else
		{
			g_app->m_requestedLocationId = -1;
		}

#ifdef	TARGET_OS_VISTA
		g_app->m_saveThumbnail = true;
#endif
        if( g_app->m_gameMode != App::GameModeCampaign )
        {
            //g_app->m_atMainMenu = true;
        }
    }
};


class WebsiteButton : public DarwiniaButton
{
public:
    char m_website[256];

    void MouseUp()
    {
        int windowed = g_prefsManager->GetInt( "ScreenWindowed", 1 );
        if( !windowed )
        {
            // Switch to windowed mode if required
            g_prefsManager->SetInt( "ScreenWindowed", 1 );
            g_prefsManager->SetInt( "ScreenWidth", 800 );
            g_prefsManager->SetInt( "ScreenHeight", 600 );
            
		    g_windowManager->DestroyWin();
            delete g_app->m_renderer;
            g_app->m_renderer = new Renderer();
            g_app->m_renderer->Initialise();
			getW32EventHandler()->ResetWindowHandle();
		    g_app->m_resource->FlushOpenGlState();
		    g_app->m_resource->RegenerateOpenGlState();
		    
            g_prefsManager->Save();        

            EclInitialise( 800, 600 );

            m_parent->SetPosition( g_app->m_renderer->ScreenW()/2 - m_parent->m_w/2, 
                                   g_app->m_renderer->ScreenH()/2 - m_parent->m_h/2 );

        }
        g_windowManager->OpenWebsite( m_website );
    }
};



LocationWindow::LocationWindow()
:   DarwiniaWindow(LANGUAGEPHRASE("dialog_locationmenu"))
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    SetMenuSize( 200, 220 );
    SetPosition( screenW/2.0f - m_w/2.0f,
                 screenH/2.0f - m_h/2.0f );

}


void LocationWindow::Create()
{
    DarwiniaWindow::Create();

	int fontSize = GetMenuSize(13);
	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - border * 2;
	int h = buttonH + border;	
	
	int gap = border;

    GlobalLocation *loc = g_app->m_globalWorld->GetLocation( g_app->m_locationId );

	if (g_app->HasBoughtGame()) 
	{
		// Full game menu

		if( loc && !loc->m_missionCompleted )
		{
			ResetLevelButton *reset = new ResetLevelButton();
			reset->SetShortProperties( LANGUAGEPHRASE("dialog_resetlocation"), border, y+=gap, buttonW, buttonH);
			reset->m_fontSize = fontSize;
			reset->m_centered = true;
			RegisterButton( reset );
			m_buttonOrder.PutData( reset );
			gap = h;
		}

		if( g_app->m_gameMode == App::GameModePrologue )
		{
			GameExitButton *exit = new GameExitButton();
			exit->SetShortProperties( LANGUAGEPHRASE("dialog_leavedarwinia"), border, y+=h, buttonW, buttonH );
			exit->m_fontSize = fontSize;
			exit->m_centered = true;
			RegisterButton( exit );
			m_buttonOrder.PutData(exit);
		}
		else
		{
			ExitLevelButton *exitLevel = new ExitLevelButton();
			exitLevel->SetShortProperties( LANGUAGEPHRASE("dialog_leavelocation"), border, y+=gap, buttonW, buttonH );
			exitLevel->m_fontSize = fontSize;
			exitLevel->m_centered = true;
			RegisterButton( exitLevel );
			m_buttonOrder.PutData( exitLevel );
		}
	}
	else {
		// Demo mode
		
		ResetLevelButton *reset = new ResetLevelButton();
		reset->SetShortProperties( LANGUAGEPHRASE("dialog_resetlocation"), border, y+=gap, buttonW, buttonH );
		reset->m_fontSize = fontSize;
		reset->m_centered = true;
		RegisterButton( reset );
		m_buttonOrder.PutData( reset );

		WebsiteButton *buy = new WebsiteButton();
		buy->SetShortProperties( LANGUAGEPHRASE("dialog_buyonline"), border, y+=h, buttonW, buttonH );
		buy->m_fontSize = fontSize;
		buy->m_centered = true;

		strcpy( buy->m_website, "http://store.introversion.co.uk" );
	    
		RegisterButton( buy );
		m_buttonOrder.PutData( buy );

		ExitLevelButton *exitLevel = new ExitLevelButton();
		exitLevel->SetShortProperties( LANGUAGEPHRASE("dialog_leavedarwinia"), border, y+=h, buttonW, buttonH );
		exitLevel->m_fontSize = fontSize;
		exitLevel->m_centered = true;
		RegisterButton( exitLevel );
		m_buttonOrder.PutData( exitLevel );
	}

    OptionsButton *options = new OptionsButton();
    options->SetShortProperties( LANGUAGEPHRASE("dialog_options"), border, y+=h, buttonW, buttonH );
    options->m_fontSize = fontSize;
    options->m_centered = true;
    RegisterButton( options );
	m_buttonOrder.PutData( options );

	if( g_app->HasBoughtGame() && g_app->m_gameMode == App::GameModePrologue )
	{
		SkipPrologueWindowButton *skip = new SkipPrologueWindowButton();
		skip->SetShortProperties( LANGUAGEPHRASE("dialog_skipprologue"), border, y+=h, buttonW, buttonH );
		skip->m_fontSize = fontSize;
		skip->m_centered = true;
		RegisterButton( skip );
		m_buttonOrder.PutData( skip );
	}
    
    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, m_h-h, buttonW, buttonH );
    close->m_fontSize = fontSize;
    close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
}


// ============================================================================


class ResetLocationButton : public DarwiniaButton
{
    void MouseUp()
    {
        EclRemoveWindow( m_parent->m_name );
        EclRemoveWindow( LANGUAGEPHRASE("dialog_locationmenu") );

#ifdef DEMO2
        g_app->ResetLevel(true);
#else
        g_app->ResetLevel(g_app->m_gameMode == App::GameModePrologue);
#endif
    }
};


ResetLocationWindow::ResetLocationWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_resetlocation") )
{
    SetMenuSize( 300, 200 );
}


void ResetLocationWindow::Create()
{
    DarwiniaWindow::Create();

	int fontSize = GetMenuSize(13);
	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w/2 - border * 2;
	int h = buttonH + border;	

	//int y = m_h - 30;

    InvertedBox *box = new InvertedBox();
    box->SetProperties( "invert", border, y+border, m_w - 20, m_h - 2*h);
    RegisterButton( box );

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_no"), border, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
    close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );

    ResetLocationButton *reset = new ResetLocationButton();
    reset->SetShortProperties( LANGUAGEPHRASE("dialog_yes"), m_w - buttonW - border, m_h - h, buttonW, buttonH );
    reset->m_fontSize = fontSize;
    reset->m_centered = true;
    RegisterButton( reset );
	m_buttonOrder.PutData( reset );
}


void ResetLocationWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

    float y = m_y+25;
    float h = GetMenuSize(18);

	float fontSize = GetMenuSize(13);

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, LANGUAGEPHRASE("dialog_reset1") );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, LANGUAGEPHRASE("dialog_reset2") );

    y+=h;

    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, LANGUAGEPHRASE("dialog_reset3") );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, LANGUAGEPHRASE("dialog_reset4") );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, LANGUAGEPHRASE("dialog_reset5") );
    
}



void MainMenuWindow::Create()
{
    DarwiniaWindow::Create();

    bool modsEnabled = g_prefsManager->GetInt( "ModSystemEnabled", 0 ) != 0;

    int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - border * 2;
	int h = buttonH + border;	
    
	int fontSize = GetMenuSize(13);

    MainMenuUserProfileButton *profile = new MainMenuUserProfileButton();
    profile->SetShortProperties( LANGUAGEPHRASE("dialog_profile"), border, y+=border, buttonW, buttonH );
    profile->m_fontSize = fontSize;
    profile->m_centered = true;
    RegisterButton( profile );
	m_buttonOrder.PutData(profile);

    if( modsEnabled )
    {
        ModsButton *mods = new ModsButton();
        mods->SetShortProperties( LANGUAGEPHRASE("dialog_mods"), border, y+=h, buttonW, buttonH );
        mods->m_fontSize = fontSize;
        mods->m_centered = true;
        RegisterButton( mods );
		m_buttonOrder.PutData(mods);
    }
    
    OptionsButton *options = new OptionsButton();
    options->SetShortProperties( LANGUAGEPHRASE("dialog_options"), border, y+=h, buttonW, buttonH );
    options->m_fontSize = fontSize;
    options->m_centered = true;
    RegisterButton( options );
	m_buttonOrder.PutData(options);

	WebsiteButton *website = new WebsiteButton();
    website->SetShortProperties( LANGUAGEPHRASE("dialog_visitwebsite"), border, y+=h, buttonW, buttonH );
    website->m_fontSize = fontSize;
    website->m_centered = true;
    strcpy( website->m_website, "http://www.darwinia.co.uk" );
    RegisterButton( website );
	m_buttonOrder.PutData(website);

	PlayPrologueWindowButton *play = new PlayPrologueWindowButton();
	play->SetShortProperties( LANGUAGEPHRASE("dialog_playprologue"), border, y+=h, buttonW, buttonH );
	play->m_fontSize = fontSize;
	play->m_centered = true;
	RegisterButton( play );
	m_buttonOrder.PutData( play );

    GameExitButton *exit = new GameExitButton();
    exit->SetShortProperties( LANGUAGEPHRASE("dialog_leavedarwinia"), border, y+=h, buttonW, buttonH );
    exit->m_fontSize = fontSize;
    exit->m_centered = true;
    RegisterButton( exit );
	m_buttonOrder.PutData(exit);

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
    close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData(close);
}

AboutDarwiniaWindow::AboutDarwiniaWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("about_darwinia") )
{
    SetMenuSize( 350, 250 );
}

void AboutDarwiniaWindow::Create()
{
    int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - border * 2;
	int h = buttonH + border;	
    int fontSize = GetMenuSize(13);

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
    close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData(close);
}

void AboutDarwiniaWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    float texH = 1.0f;
    float texW = texH * 512.0f / 64.0f;

    glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 1.0f );     glVertex2f( m_x + m_w/2 - 25, m_y + 30 );
        glTexCoord2f( 1.0f, 1.0f );     glVertex2f( m_x + m_w/2 + 25, m_y + 30 );
        glTexCoord2f( 1.0f, 0.0f );     glVertex2f( m_x + m_w/2 + 25, m_y + GetMenuSize(80) );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x + m_w/2 - 25, m_y + GetMenuSize(80) );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    float y = m_y+100;
    float h = GetMenuSize(18);

	float fontSize = GetMenuSize(13);

    char about[512];
    sprintf( about, "%s %s", LANGUAGEPHRASE("bootloader_credits_4"), LANGUAGEPHRASE("bootloader_credits_5") );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, "Darwinia v1.5.4" );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=2*h, fontSize, about );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, "http://www.introversion.co.uk" );
}

SkipPrologueWindow::SkipPrologueWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_skipprologue") )
{
    SetMenuSize( 360, 350 );
}

void SkipPrologueWindow::Create()
{
    int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w / 2 - border * 2;
	int h = buttonH + border;	
    int fontSize = GetMenuSize(13);

	SkipPrologueButton *skip = new SkipPrologueButton();
	skip->SetShortProperties( LANGUAGEPHRASE("dialog_skipprologue"), border, m_h - h, buttonW, buttonH );
	skip->m_fontSize = fontSize;
	skip->m_centered = true;
	RegisterButton( skip );
	m_buttonOrder.PutData( skip );

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border * 2 + buttonW, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
	close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData(close);
}

void SkipPrologueWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/campaign.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    float texH = 1.0f;
    float texW = texH * 512.0f / 64.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 1.0f );     glVertex2f( m_x + 25, m_y + 30 );
        glTexCoord2f( 1.0f, 1.0f );     glVertex2f( m_x + m_w - 25, m_y + 30 );
        glTexCoord2f( 1.0f, 0.0f );     glVertex2f( m_x + m_w - 25, m_y + GetMenuSize(200) );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x + 25, m_y + GetMenuSize(200) );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    float y = m_y+m_h-GetMenuSize(150);
    float h = GetMenuSize(18);

	float fontSize = GetMenuSize(13);

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	LList<char *> *wrapped = WordWrapText( LANGUAGEPHRASE("dialog_skip1"), m_w*1.70f, fontSize, true);
    for( int i = 0; i < wrapped->Size(); ++i )
    {
		g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, wrapped->GetData(i) );
    }
    delete wrapped->GetData(0);
    delete wrapped;
};

PlayPrologueWindow::PlayPrologueWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_playprologue") )
{
    SetMenuSize( 350, 350 );
}

void PlayPrologueWindow::Create()
{
    int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w / 2 - border * 2;
	int h = buttonH + border;	
    int fontSize = GetMenuSize(13);

	PlayPrologueButton *play = new PlayPrologueButton();
	play->SetShortProperties( LANGUAGEPHRASE("dialog_playprologue"), border, m_h - h, buttonW, buttonH );
	play->m_fontSize = fontSize;
	play->m_centered = true;
	RegisterButton( play );
	m_buttonOrder.PutData( play );

    CloseButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border * 2 + buttonW, m_h - h, buttonW, buttonH );
    close->m_fontSize = fontSize;
	close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData(close);
}

void PlayPrologueWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/prologue.bmp" ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

    float texH = 1.0f;
    float texW = texH * 512.0f / 64.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 1.0f );     glVertex2f( m_x + 25, m_y + 30 );
        glTexCoord2f( 1.0f, 1.0f );     glVertex2f( m_x + m_w - 25, m_y + 30 );
        glTexCoord2f( 1.0f, 0.0f );     glVertex2f( m_x + m_w - 25, m_y + GetMenuSize(200) );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x + 25, m_y + GetMenuSize(200) );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    float y = m_y+m_h-GetMenuSize(150);
    float h = GetMenuSize(18);

	float fontSize = GetMenuSize(13);

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	LList<char *> *wrapped = WordWrapText( LANGUAGEPHRASE("dialog_prologue1"), m_w*1.70f, fontSize, true);
    for( int i = 0; i < wrapped->Size(); ++i )
    {
		g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, wrapped->GetData(i) );
    }
    delete wrapped->GetData(0);
    delete wrapped;
};