#include "lib/universal_include.h"
//#include "lib/input.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"

#include "interface/prefs_screen_window.h"
#include "interface/drop_down_menu.h"
#include "interface/message_dialog.h"

#include "app.h"
#include "renderer.h"

#include "lib/input/win32_eventhandler.h"
#include "lib/window_manager_win32.h"

#ifdef TARGET_MSVC
#define HAVE_REFRESH_RATES
#endif

class ScreenResDropDownMenu : public DropDownMenu
{
#ifdef HAVE_REFRESH_RATES
    void SelectOption( int _option )
    {
        PrefsScreenWindow *parent = (PrefsScreenWindow *) m_parent;

        DropDownMenu::SelectOption( _option );

        // Update available refresh rates

        DropDownMenu *refresh = (DropDownMenu *) m_parent->GetButton( LANGUAGEPHRASE("dialog_refreshrate") );
        refresh->Empty();

        Resolution *resolution = g_windowManager->GetResolution( _option );
        if( resolution )
        {
            for( int i = 0; i < resolution->m_refreshRates.Size(); ++i )
            {
                int thisRate = resolution->m_refreshRates[i];
                char caption[64];
                sprintf( caption, "%d Hz", thisRate );
                refresh->AddOption( caption, thisRate );
            }
            refresh->SelectOption( parent->m_refreshRate );
        }
    }
#endif
};


class FullscreenRequiredMenu : public DropDownMenu
{
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {    
        DropDownMenu *windowed = (DropDownMenu *) m_parent->GetButton( LANGUAGEPHRASE("dialog_windowed") );
        bool available = (windowed->GetSelectionValue() == 0);
        if( available )
        {
            DropDownMenu::Render( realX, realY, highlighted, clicked );
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
			DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
            g_editorFont.DrawText2D( realX+10, realY+9, parent->GetMenuSize(13), LANGUAGEPHRASE("dialog_windowedmode") );
        }
    }

    void MouseUp()
    {
        DropDownMenu *windowed = (DropDownMenu *) m_parent->GetButton( LANGUAGEPHRASE("dialog_windowed") );
        bool available = (windowed->GetSelectionValue() == 0);
        if( available )
        {
            DropDownMenu::MouseUp();
        }
    }    
};

static void AdjustWindowPositions(int _newWidth, int _newHeight, int _oldWidth, int _oldHeight)
{
	if (_newWidth != _oldWidth || _newHeight != _oldHeight) {
		// Resolution has changed, adjust the window positions accordingly
		EclInitialise( _newWidth, _newHeight );

		LList<EclWindow *> *windows = EclGetWindows();
		for (int i = 0; i < windows->Size(); i++) {
			EclWindow *w = windows->GetData(i);
			
			// We attempt to keep the centre of the window in the same place
			
			double halfWidth = w->m_w / 2.0;
			double halfHeight = w->m_h / 2.0;

			double oldCentreX = w->m_x + halfWidth; 
			double oldCentreY = w->m_y + halfHeight; 
			
			double newCentreX = oldCentreX * _newWidth / _oldWidth; 
			double newCentreY = oldCentreY * _newHeight / _oldHeight;

			w->SetPosition( int( newCentreX - halfWidth ), int( newCentreY - halfHeight ) );
		}
	}
}


void RestartWindowManagerAndRenderer()
{
#ifdef TARGET_OS_LINUX
	// Resolution/Fullscreen switching is broken on linux, pop up a dialog box
	// asking the user to restart to get the effect.

	EclRegisterWindow(
		new MessageDialog("Restart Required", 
			"Please restart Darwinia for the screen\n"
			"settings to take affect")
		);
#else	
	int oldWidth = g_app->m_renderer->ScreenW();
	int oldHeight = g_app->m_renderer->ScreenH(); 
	
	bool hack = !g_windowManager->Windowed() && !g_prefsManager->GetInt( "ScreenWindowed" );

	// shutdown old window
	//getW32EventHandler()->UnbindAltTab(); // was unbind done by someone else? it will be too late when window is destroyed
	g_windowManager->DestroyWin();
	delete g_app->m_renderer;

	// necessary for resolution change in fullscreen
	// create and destroy temporary window
	// todo: reduce this slow code, find out minimal sufficient code
	if(hack)
	{
		g_prefsManager->SetInt( "ScreenWindowed", 1 );
		int oldW = g_prefsManager->GetInt( "ScreenWidth" );
		int oldH = g_prefsManager->GetInt( "ScreenHeight" );
		g_prefsManager->SetInt( "ScreenWidth", 640 );
		g_prefsManager->SetInt( "ScreenHeight", 480 );
		g_app->m_renderer = new Renderer();
		g_app->m_renderer->Initialise();
		g_windowManager->DestroyWin();
		delete g_app->m_renderer;
		g_prefsManager->SetInt( "ScreenWidth", oldW );
		g_prefsManager->SetInt( "ScreenHeight", oldH );
		g_prefsManager->SetInt( "ScreenWindowed", 0 );
	}

	// start new window
	g_app->m_renderer = new Renderer();
	g_app->m_renderer->Initialise();
	g_app->m_resource->FlushOpenGlState();
	g_app->m_resource->RegenerateOpenGlState();
	
	int newWidth = g_app->m_renderer->ScreenW();
	int newHeight = g_app->m_renderer->ScreenH(); 
	
	AdjustWindowPositions(newWidth, newHeight, oldWidth, oldHeight);
#endif
}

// SetWindowed - switch to windowed or fullscreen mode.
// Used by lib/input_sdl.cpp 
//         debugmenu.cpp
//
// sets _isSwitchingToWindowed true if we actually leave fullscreen mode and
// switch to windowed mode.
//
// This is useful for Mac OS X automatic switch back, when leaving fullscreen mode
// Command-Tab

void SetWindowed(bool _isWindowed, bool _isPermanent, bool &_isSwitchingToWindowed)
{
	bool oldIsWindowedPref = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME );
	bool oldIsWindowed = g_windowManager->Windowed();
	g_prefsManager->SetInt( SCREEN_WINDOWED_PREFS_NAME, _isWindowed );

	if (oldIsWindowed != _isWindowed) {
	
		_isSwitchingToWindowed = _isWindowed; 
		
		RestartWindowManagerAndRenderer();
	}
	else
		_isSwitchingToWindowed = false;

	if (!_isPermanent) 
		g_prefsManager->SetInt( SCREEN_WINDOWED_PREFS_NAME, oldIsWindowedPref );
	else
		g_prefsManager->Save(); 
}


class SetScreenButton : public DarwiniaButton
{
    void MouseUp()
    {
        PrefsScreenWindow *parent = (PrefsScreenWindow *) m_parent;
        
        Resolution *resolution = g_windowManager->GetResolution( parent->m_resId );
        g_prefsManager->SetInt( SCREEN_WIDTH_PREFS_NAME, resolution->m_width );
        g_prefsManager->SetInt( SCREEN_HEIGHT_PREFS_NAME, resolution->m_height );
#ifdef HAVE_REFRESH_RATES
        g_prefsManager->SetInt( SCREEN_REFRESH_PREFS_NAME, parent->m_refreshRate );
#endif
        g_prefsManager->SetInt( SCREEN_WINDOWED_PREFS_NAME, parent->m_windowed );
        g_prefsManager->SetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME, parent->m_colourDepth );
        g_prefsManager->SetInt( SCREEN_Z_DEPTH_PREFS_NAME, parent->m_zDepth );

		RestartWindowManagerAndRenderer();
		
        g_prefsManager->Save();        

        parent->m_resId = g_windowManager->GetResolutionId( g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                                                            g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME) );
        
        parent->m_windowed      = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME, 0 );
        parent->m_colourDepth   = g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME, 16 );
#ifdef HAVE_REFRESH_RATES
        parent->m_refreshRate   = g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME, 60 );
#endif		
        parent->m_zDepth        = g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME, 24 );

        parent->Remove();
        parent->Create();
    }
};


PrefsScreenWindow::PrefsScreenWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_screenoptions") )
{
	int height = 240;
	
    m_resId = g_windowManager->GetResolutionId( g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                                                g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME) );

    m_windowed      = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME, 0 );
    m_colourDepth   = g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME, 16 );
#ifdef HAVE_REFRESH_RATES
    m_refreshRate   = g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME, 60 );
	height += 30;
#endif
    m_zDepth        = g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME, 24 );

    SetMenuSize( 410, height );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );
}


void PrefsScreenWindow::Create()
{
    DarwiniaWindow::Create();

    /*int x = GetMenuSize(150);
    int w = GetMenuSize(170);
    int y = 30;
    int h = GetMenuSize(30);
	int fontSize = GetMenuSize(13);*/

	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int x = m_w * 0.5;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w - x - border * 2;
	int buttonW2 = m_w/2 - border * 2;
	int h = buttonH + border;
	int fontSize = GetMenuSize(13);

    InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y += border, m_w - 20, GetClientRectY2() - h * 2 );        
    RegisterButton( box );

    ScreenResDropDownMenu *screenRes = new ScreenResDropDownMenu();
    screenRes->SetShortProperties( LANGUAGEPHRASE("dialog_resolution"), x, y+=border, buttonW, buttonH );
    for( int i = 0; i < g_windowManager->m_resolutions.Size(); ++i )
    {
        Resolution *resolution = g_windowManager->m_resolutions[i];
        char caption[64];
        sprintf( caption, "%d x %d", resolution->m_width, resolution->m_height );
        screenRes->AddOption( caption, i );
    }
	screenRes->m_fontSize = fontSize;

    DropDownMenu *windowed = new DropDownMenu();
    windowed->SetShortProperties( LANGUAGEPHRASE("dialog_windowed"), x, y+=h, buttonW, buttonH );
    windowed->AddOption( LANGUAGEPHRASE("dialog_yes"), 1 );
    windowed->AddOption( LANGUAGEPHRASE("dialog_no"), 0 );
	windowed->m_fontSize = fontSize;
    windowed->RegisterInt( &m_windowed );


#ifdef HAVE_REFRESH_RATES
    FullscreenRequiredMenu *refresh = new FullscreenRequiredMenu();
    refresh->SetShortProperties( LANGUAGEPHRASE("dialog_refreshrate"), x, y+=h, buttonW, buttonH );
    refresh->RegisterInt( &m_refreshRate );
	refresh->m_fontSize = fontSize;
    RegisterButton( refresh );
#endif

    RegisterButton( windowed );
    RegisterButton( screenRes );
    screenRes->RegisterInt( &m_resId );   

    FullscreenRequiredMenu *colourDepth = new FullscreenRequiredMenu();
    colourDepth->SetShortProperties( LANGUAGEPHRASE("dialog_colourdepth"), x, y+=h, buttonW, buttonH );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_16"), 16 );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_24"), 24 );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_32"), 32 );
    colourDepth->RegisterInt( &m_colourDepth );
	colourDepth->m_fontSize = fontSize;
    RegisterButton( colourDepth );

    m_buttonOrder.PutData( screenRes );
	m_buttonOrder.PutData( windowed );
    if( windowed->GetSelectionValue() == 0 )
    {
        m_buttonOrder.PutData( refresh );
        m_buttonOrder.PutData( colourDepth );
    }
    
    DropDownMenu *zDepth = new DropDownMenu();
    zDepth->SetShortProperties( LANGUAGEPHRASE("dialog_zbufferdepth"), x, y+=h, buttonW, buttonH );
    zDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_16"), 16 );
    zDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_24"), 24 );
    zDepth->RegisterInt( &m_zDepth );
	zDepth->m_fontSize = fontSize;
    RegisterButton( zDepth );
	m_buttonOrder.PutData( zDepth );

	y = m_h - h;

    CloseButton *cancel = new CloseButton();
    cancel->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, y, buttonW2, buttonH );
    cancel->m_fontSize = fontSize;
    cancel->m_centered = true;
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );

    SetScreenButton *apply = new SetScreenButton();
    apply->SetShortProperties( LANGUAGEPHRASE("dialog_apply"), m_w - buttonW2 - border, y, buttonW2, buttonH );
    apply->m_fontSize = fontSize;
    apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );
}


void PrefsScreenWindow::Render( bool _hasFocus )
{
    DarwiniaWindow::Render( _hasFocus );

	int border = GetClientRectX1() + 10;
    int x = m_x + 20;
    int y = m_y + GetClientRectY1() + border*2;
    int h = GetMenuSize(20) + border;
    int size = GetMenuSize(13);

    g_editorFont.DrawText2D( x, y+=border, size, LANGUAGEPHRASE("dialog_resolution") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_windowed") );
#ifdef HAVE_REFRESH_RATES	
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_refreshrate") );
#endif
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_colourdepth") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_zbufferdepth") );
}
