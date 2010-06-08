#include "lib/universal_include.h"
//#include "lib/input.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/language_table.h"
#include "lib/input/input.h"

#include "interface/prefs_screen_window.h"
#include "interface/drop_down_menu.h"
#include "interface/message_dialog.h"

#include "app.h"
#include "renderer.h"
#include "game_menu.h"

#ifdef TARGET_MSVC
#include "lib/input/win32_eventhandler.h"
#include "lib/window_manager_win32.h"
#endif

#ifdef TARGET_MSVC
#define HAVE_REFRESH_RATES
#endif

class ScreenResDropDownMenu : public GameMenuDropDown
{
#ifdef HAVE_REFRESH_RATES
    void SelectOption( int _option )
    {
        PrefsScreenWindow *parent = (PrefsScreenWindow *) m_parent;

        GameMenuDropDown::SelectOption( _option );

        // Update available refresh rates

        GameMenuDropDown *refresh = (GameMenuDropDown *) m_parent->GetButton( "dialog_refreshrate" );
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


class FullscreenRequiredMenu : public GameMenuDropDown
{
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {    
        GameMenuCheckBox *windowed = (GameMenuCheckBox *) m_parent->GetButton( "dialog_windowed" );
        bool available = !windowed->IsChecked();
        if( available )
        {
            m_inactive = false;
            GameMenuDropDown::Render( realX, realY, highlighted, clicked );
        }
        else
        {
            float texY = realY + m_h/2 - m_fontSize/4;
            glColor4f( 1.0, 1.0, 1.0, 0.0 );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2D( realX+m_w*0.05, texY, m_fontSize, LANGUAGEPHRASE(m_name) );
            glColor4f( 0.3, 0.3, 0.3, 1.0 );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2D( realX+m_w*0.05, texY, m_fontSize, LANGUAGEPHRASE(m_name) );


            glColor4f( 1.0, 1.0, 1.0, 0.5 );
			DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
            g_titleFont.DrawText2DRight( (realX+m_w) - m_w*0.05, texY, m_fontSize / 2.0, LANGUAGEPHRASE("dialog_windowedmode") );
            m_inactive = true;
        }
    }

    void MouseUp()
    {
        GameMenuCheckBox *windowed = (GameMenuCheckBox *) m_parent->GetButton( "dialog_windowed" );
        bool available = !windowed->IsChecked();
        if( available )
        {
            GameMenuDropDown::MouseUp();
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
		
	// if we're switching to windowed mode on OS X, our new backing buffer will have
	// garbage in it we need to clear.
	glClearColor(0,0,0,1);
	glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	
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


class SetScreenButton : public GameMenuButton
{
public:
    SetScreenButton()
    :   GameMenuButton("dialog_apply")
    {
    }

    void MouseUp()
    {
        if( m_inactive ) return;

        PrefsScreenWindow *parent = (PrefsScreenWindow *) m_parent;
        
        Resolution *resolution = g_windowManager->GetResolution( parent->m_resId );

        AppReleaseAssert( resolution != NULL, "Cannot get resolution id(%d)",
                          parent->m_resId );

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

        AppReleaseAssert( parent->m_resId != -1, "Cannot get resolution id for width(%d) and height(%d)",
                          g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                          g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME) );

        parent->m_windowed      = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME, 0 );
        parent->m_colourDepth   = g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME, 16 );
#ifdef HAVE_REFRESH_RATES
        parent->m_refreshRate   = g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME, 60 );
#endif		
        parent->m_zDepth        = g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME, 24 );

        LList<EclWindow *> *windows = EclGetWindows();
	    while (windows->Size() > 0) {
		    EclWindow *w = windows->GetData(0);
		    EclRemoveWindow(w->m_name);
        }

        //parent->Remove();
        //parent->Create();

        if( g_app->m_atMainMenu )
        {
            g_app->m_gameMenu->DestroyMenu();
            g_app->m_gameMenu->CreateMenu();
        }
        else
        {
            EclRegisterWindow( new LocationWindow() );
        }
        EclRegisterWindow( new OptionsMenuWindow() );
        EclRegisterWindow( new PrefsScreenWindow() );
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        PrefsScreenWindow *parent = (PrefsScreenWindow *) m_parent;
        
        Resolution *resolution = g_windowManager->GetResolution( parent->m_resId );

        AppReleaseAssert( resolution != NULL, "Cannot get resolution id(%d)",
                          parent->m_resId );

        if( resolution->m_width == g_prefsManager->GetInt( SCREEN_WIDTH_PREFS_NAME ) &&
            resolution->m_height == g_prefsManager->GetInt( SCREEN_HEIGHT_PREFS_NAME ) &&
#ifdef HAVE_REFRESH_RATES
        parent->m_refreshRate == g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME ) &&
#endif
        parent->m_windowed == g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME ) &&
        parent->m_colourDepth == g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME ) &&
        parent->m_zDepth == g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME) ) 
        {
            m_inactive = true;
        }
        else
        {
            m_inactive = false;
        }

        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }
};


PrefsScreenWindow::PrefsScreenWindow()
:   GameOptionsWindow( "dialog_screenoptions" )
{
	int height = 240;
	
    m_resId = g_windowManager->GetResolutionId( g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                                                g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME) );

    AppReleaseAssert( m_resId != -1, "Cannot get resolution id for width(%d) and height(%d)",
                      g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME),
                      g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME) );

    m_windowed      = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME, 0 );
    m_colourDepth   = g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME, 16 );
#ifdef HAVE_REFRESH_RATES
    m_refreshRate   = g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME, 60 );
	height += 30;
#endif
    m_zDepth        = g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME, 24 );
    m_overscan      = g_prefsManager->GetInt( SCREEN_OVERSCAN, 0 );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();
    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    /*SetMenuSize( 340, height );
    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );*/
}


void PrefsScreenWindow::Create()
{
    //DarwiniaWindow::Create();

    /*int x = GetMenuSize(150);
    int w = GetMenuSize(170);
    int y = 30;
    int h = GetMenuSize(30);
	int fontSize = GetMenuSize(13);*/

	/*int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int x = m_w/2;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w/2 - border * 2;
	int h = buttonH + border;
	int fontSize = GetMenuSize(13);*/
    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0;
    float y = leftY;
    float fontSize = fontSmall;

    /*InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y += border, m_w - 20, GetClientRectY2() - h * 2 );        
    RegisterButton( box );*/

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    buttonH *= 0.65f;

    ScreenResDropDownMenu *screenRes = new ScreenResDropDownMenu();
    screenRes->SetShortProperties( "dialog_resolution", x, y+=buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_resolution") );
    for( int i = 0; i < g_windowManager->m_resolutions.Size(); ++i )
    {
        Resolution *resolution = g_windowManager->m_resolutions[i];
        char caption[64];
        sprintf( caption, "%d x %d", resolution->m_width, resolution->m_height );
        screenRes->AddOption( caption, i );
    }
	screenRes->m_fontSize = fontSize;

    GameMenuCheckBox *windowed = new GameMenuCheckBox();
    windowed->SetShortProperties( "dialog_windowed", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_windowed") );
	windowed->m_fontSize = fontSize;
    windowed->RegisterInt( &m_windowed );


#ifdef HAVE_REFRESH_RATES
    FullscreenRequiredMenu *refresh = new FullscreenRequiredMenu();
    refresh->SetShortProperties( "dialog_refreshrate", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_refreshrate") );
    refresh->RegisterInt( &m_refreshRate );
	refresh->m_fontSize = fontSize;
    refresh->m_cycleMode = true;
    RegisterButton( refresh );
#endif

    RegisterButton( windowed );
    RegisterButton( screenRes );
    screenRes->RegisterInt( &m_resId );   

    FullscreenRequiredMenu *colourDepth = new FullscreenRequiredMenu();
    colourDepth->SetShortProperties( "dialog_colourdepth", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_colourdepth") );
    colourDepth->AddOption( "dialog_colourdepth_16", 16 );
    colourDepth->AddOption( "dialog_colourdepth_24", 24 );
    colourDepth->AddOption( "dialog_colourdepth_32", 32 );
    colourDepth->RegisterInt( &m_colourDepth );
	colourDepth->m_fontSize = fontSize;
    colourDepth->m_cycleMode = true;
    RegisterButton( colourDepth );

    m_buttonOrder.PutData( screenRes );
	m_buttonOrder.PutData( windowed );
    //if( !windowed->IsChecked() )
    {
#ifdef HAVE_REFRESH_RATES
        m_buttonOrder.PutData( refresh );
#endif
        m_buttonOrder.PutData( colourDepth );
    }
    
    GameMenuDropDown *zDepth = new GameMenuDropDown();
    zDepth->SetShortProperties( "dialog_zbufferdepth", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_zbufferdepth") );
    zDepth->AddOption( "dialog_colourdepth_16", 16 );
    zDepth->AddOption( "dialog_colourdepth_24", 24 );
    zDepth->RegisterInt( &m_zDepth );
	zDepth->m_fontSize = fontSize;
    zDepth->m_cycleMode = true;
    RegisterButton( zDepth );
	m_buttonOrder.PutData( zDepth );

    buttonH /= 0.65f;
    int buttonY = leftY + leftH - buttonH * 3;

    SetScreenButton *apply = new SetScreenButton();
    apply->SetShortProperties( "dialog_apply", x, buttonY, buttonW, buttonH, LANGUAGEPHRASE("dialog_apply") );
    apply->m_fontSize = fontMed;
   // apply->m_centered = true;
    RegisterButton( apply );
	m_buttonOrder.PutData( apply );

    MenuCloseButton *cancel = new MenuCloseButton("dialog_close");
    cancel->SetShortProperties( "dialog_close", x, buttonY += buttonH + gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    cancel->m_fontSize = fontMed;
    //cancel->m_centered = true;
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );
	m_backButton = cancel;
}


void PrefsScreenWindow::Render( bool _hasFocus )
{
    GameOptionsWindow::Render( _hasFocus );

	/*int border = GetClientRectX1() + 10;
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
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_zbufferdepth") );*/
}
