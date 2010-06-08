#include "lib/universal_include.h"
#include "lib/language_table.h"
#include "lib/render/renderer.h"
#include "lib/preferences.h"
#include "lib/gucci/window_manager.h"

#include "interface/components/drop_down_menu.h"
#include "screenoptions_window.h"


#define HAVE_REFRESH_RATES


class ScreenResDropDownMenu : public DropDownMenu
{
#ifdef HAVE_REFRESH_RATES
    void SelectOption( int _option )
    {
        ScreenOptionsWindow *parent = (ScreenOptionsWindow *) m_parent;

        DropDownMenu::SelectOption( _option );

        // Update available refresh rates

        DropDownMenu *refresh = (DropDownMenu *) m_parent->GetButton( "Refresh Rate" );
        refresh->Empty();

        WindowResolution *resolution = g_windowManager->GetResolution( _option );
        if( resolution )
        {
            for( int i = 0; i < resolution->m_refreshRates.Size(); ++i )
            {
                int thisRate = resolution->m_refreshRates[i];

                char caption[128];
                sprintf( caption, LANGUAGEPHRASE("dialog_refreshrate_X") );
                LPREPLACEINTEGERFLAG( 'R', thisRate, caption );

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
        DropDownMenu *windowed = (DropDownMenu *) m_parent->GetButton( "Windowed" );
        bool available = (windowed->GetSelectionValue() == 0);
        if( available )
        {
            DropDownMenu::Render( realX, realY, highlighted, clicked );
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
            g_renderer->Text( realX+10, realY+9, White, 13, LANGUAGEPHRASE("dialog_windowedmode") );
        }
    }

    void MouseUp()
    {
        DropDownMenu *windowed = (DropDownMenu *) m_parent->GetButton( "Windowed" );
        bool available = (windowed->GetSelectionValue() == 0);
        if( available )
        {
            DropDownMenu::MouseUp();
        }
    }    
};

static void AdjustWindowPositions(int _newWidth, int _newHeight, int _oldWidth, int _oldHeight)
{
	if (_newWidth != _oldWidth || _newHeight != _oldHeight)
    {
		// Resolution has changed, adjust the window positions accordingly
		EclInitialise( _newWidth, _newHeight );

		LList<EclWindow *> *windows = EclGetWindows();
		for (int i = 0; i < windows->Size(); i++) 
        {
			EclWindow *w = windows->GetData(i);
			
            int oldCentreX = _oldWidth/2.0f - w->m_w/2;
            int oldCentreY = _oldHeight/2.0f - w->m_h/2;

            if( fabs(float(w->m_x - oldCentreX)) < 30 &&
                fabs(float(w->m_y - oldCentreY)) < 30 )
            {
                // This window was centralised
                w->m_x = _newWidth/2.0f - w->m_w/2.0f;
                w->m_y = _newHeight/2.0f - w->m_h/2.0f;
            }
            else
            {
                //
                // Find the two nearest edges, and keep those distances the same
                // How far are we from each edge?

                float distances[4];
                distances[0] = w->m_x;                              // left
                distances[1] = _oldWidth - (w->m_x + w->m_w);       // right
                distances[2] = w->m_y;                              // top
                distances[3] = _oldHeight - (w->m_y + w->m_h);      // bottom

                //
                // Sort those distances, closest first

                LList<int> sortedDistances;
                for( int j = 0; j < 4; ++j )
                {
                    float thisDistance = distances[j];
                    bool added = false;
                    for( int k = 0; k < sortedDistances.Size(); ++k )
                    {
                        float sortedDistance = distances[sortedDistances[k]];
                        if( thisDistance < sortedDistance )
                        {
                            sortedDistances.PutDataAtIndex( j, k );
                            added = true;
                            break;
                        }
                    }
                    if( !added )
                    {
                        sortedDistances.PutDataAtEnd( j );
                    }
                }
            
                //
                // Preserve the two nearest edge distances;

                for( int j = 0; j < 2; ++j )
                {
                    int thisNearest = sortedDistances[j];
                    
                    if( thisNearest == 0 )      w->m_x = distances[thisNearest];
                    if( thisNearest == 1 )      w->m_x = _newWidth - w->m_w - distances[thisNearest];
                    if( thisNearest == 2 )      w->m_y = distances[thisNearest];
                    if( thisNearest == 3 )      w->m_y = _newHeight - w->m_h - distances[thisNearest];
                }
            }
		}
	}
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
	bool oldIsWindowedPref = g_preferences->GetInt( PREFS_SCREEN_WINDOWED );
	bool oldIsWindowed = g_windowManager->Windowed();
	g_preferences->SetInt( PREFS_SCREEN_WINDOWED, _isWindowed );

	if (oldIsWindowed != _isWindowed) {
	
		_isSwitchingToWindowed = _isWindowed; 
		
		//RestartWindowManagerAndRenderer();
	}
	else
		_isSwitchingToWindowed = false;

	if (!_isPermanent) 
		g_preferences->SetInt( PREFS_SCREEN_WINDOWED, oldIsWindowedPref );
	else
		g_preferences->Save(); 
}


class SetScreenButton : public InterfaceButton
{
    void MouseUp()
    {
        ScreenOptionsWindow *parent = (ScreenOptionsWindow *) m_parent;
        
        int oldWidth = g_preferences->GetInt( PREFS_SCREEN_WIDTH );
        int oldHeight = g_preferences->GetInt( PREFS_SCREEN_HEIGHT );

        WindowResolution *resolution = g_windowManager->GetResolution( parent->m_resId );
        if( resolution )
        {
            g_preferences->SetInt( PREFS_SCREEN_WIDTH, resolution->m_width );
            g_preferences->SetInt( PREFS_SCREEN_HEIGHT, resolution->m_height );
        }

#ifdef HAVE_REFRESH_RATES
        g_preferences->SetInt( PREFS_SCREEN_REFRESH, parent->m_refreshRate );
#endif
        g_preferences->SetInt( PREFS_SCREEN_WINDOWED, parent->m_windowed );
        g_preferences->SetInt( PREFS_SCREEN_COLOUR_DEPTH, parent->m_colourDepth );
        g_preferences->SetInt( PREFS_SCREEN_Z_DEPTH, parent->m_zDepth );
        g_preferences->SetInt( PREFS_SCREEN_ANTIALIAS, parent->m_antiAlias );
		
#ifndef TARGET_OS_LINUX
	    AdjustWindowPositions(  resolution->m_width, resolution->m_height, 
                                oldWidth, oldHeight);
#endif

        parent->RestartWindowManager();
        
        g_preferences->Save();        

        parent->m_resId = g_windowManager->GetResolutionId( g_preferences->GetInt(PREFS_SCREEN_WIDTH),
                                                            g_preferences->GetInt(PREFS_SCREEN_HEIGHT) );
        
        parent->m_windowed      = g_preferences->GetInt( PREFS_SCREEN_WINDOWED, 0 );
        parent->m_colourDepth   = g_preferences->GetInt( PREFS_SCREEN_COLOUR_DEPTH, 16 );

#ifdef HAVE_REFRESH_RATES
        parent->m_refreshRate   = g_preferences->GetInt( PREFS_SCREEN_REFRESH, 60 );
#endif		
        parent->m_zDepth        = g_preferences->GetInt( PREFS_SCREEN_Z_DEPTH, 24 );
		parent->m_antiAlias		= g_preferences->GetInt( PREFS_SCREEN_ANTIALIAS, 0 );

        parent->Remove();
        parent->Create();
    }
};


void ScreenOptionsWindow::RestartWindowManager()
{
};


ScreenOptionsWindow::ScreenOptionsWindow()
:   InterfaceWindow( "ScreenOptions", "dialog_screenoptions", true )
{
	int height = 300;
	
    m_resId = g_windowManager->GetResolutionId( g_preferences->GetInt(PREFS_SCREEN_WIDTH),
                                                g_preferences->GetInt(PREFS_SCREEN_HEIGHT) );

    m_windowed      = g_preferences->GetInt( PREFS_SCREEN_WINDOWED, 0 );
    m_colourDepth   = g_preferences->GetInt( PREFS_SCREEN_COLOUR_DEPTH, 16 );
#ifdef HAVE_REFRESH_RATES
    m_refreshRate   = g_preferences->GetInt( PREFS_SCREEN_REFRESH, 60 );
	height += 30;
#endif
    m_zDepth        = g_preferences->GetInt( PREFS_SCREEN_Z_DEPTH, 24 );
	m_antiAlias		= g_preferences->GetInt( PREFS_SCREEN_ANTIALIAS, 0 );
	
    SetSize( 340, height );
}


void ScreenOptionsWindow::Create()
{
    InterfaceWindow::Create();

    int x = 150;
    int w = 170;
    int y = 30;
    int h = 30;

    InvertedBox *box = new InvertedBox();
    box->SetProperties( "invert", 10, 50, m_w - 20, m_h - 140, " ", " ", false, false );        
    RegisterButton( box );

    ScreenResDropDownMenu *screenRes = new ScreenResDropDownMenu();
    screenRes->SetProperties( "Resolution", x, y+=h, w, 20, "dialog_resolution", " ", true, false );
    for( int i = 0; i < g_windowManager->m_resolutions.Size(); ++i )
    {
        WindowResolution *resolution = g_windowManager->m_resolutions[i];

        char caption[128];
		sprintf( caption, LANGUAGEPHRASE("dialog_resolution_W_H") );
		LPREPLACEINTEGERFLAG( 'W', resolution->m_width, caption );
		LPREPLACEINTEGERFLAG( 'H', resolution->m_height, caption );

        screenRes->AddOption( caption, i );
    }

    DropDownMenu *windowed = new DropDownMenu();
    windowed->SetProperties( "Windowed", x, y+=h, w, 20, "dialog_windowed", " ", true, false );
    windowed->AddOption( LANGUAGEPHRASE("dialog_yes"), 1 );
    windowed->AddOption( LANGUAGEPHRASE("dialog_no"), 0 );
    windowed->RegisterInt( &m_windowed );

#ifdef HAVE_REFRESH_RATES
    FullscreenRequiredMenu *refresh = new FullscreenRequiredMenu();
    refresh->SetProperties( "Refresh Rate", x, y+=h, w, 20, "dialog_refreshrate", " ", true, false );
    refresh->RegisterInt( &m_refreshRate );
    RegisterButton( refresh );
#endif

    RegisterButton( windowed );
    RegisterButton( screenRes );
    screenRes->RegisterInt( &m_resId );   


    FullscreenRequiredMenu *colourDepth = new FullscreenRequiredMenu();
    colourDepth->SetProperties( "Colour Depth", x, y+=h, w, 20, "dialog_colourdepth", " ", true, false );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_16"), 16 );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_24"), 24 );
    colourDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_32"), 32 );
    colourDepth->RegisterInt( &m_colourDepth );
    RegisterButton( colourDepth );
    
    DropDownMenu *zDepth = new DropDownMenu();
    zDepth->SetProperties( "Z Buffer Depth", x, y+=h, w, 20, "dialog_zbufferdepth", " ", true, false );
    zDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_16"), 16 );
    zDepth->AddOption( LANGUAGEPHRASE("dialog_colourdepth_24"), 24 );
    zDepth->RegisterInt( &m_zDepth );
    RegisterButton( zDepth );

    DropDownMenu *antiAlias = new DropDownMenu();
    antiAlias->SetProperties( LANGUAGEPHRASE("dialog_antialias"), x, y+=h, w, 20 );
    antiAlias->AddOption( LANGUAGEPHRASE("dialog_yes"), 1 );
    antiAlias->AddOption( LANGUAGEPHRASE("dialog_no"), 0 );
    antiAlias->RegisterInt( &m_antiAlias );
    RegisterButton( antiAlias );
	
	CloseButton *cancel = new CloseButton();
    cancel->SetProperties( "Close", 10, m_h - 30, m_w / 2 - 15, 20, "dialog_close", " ", true, false );
    RegisterButton( cancel );

    SetScreenButton *apply = new SetScreenButton();
    apply->SetProperties( "Apply", cancel->m_x+cancel->m_w+10, m_h - 30, m_w / 2 - 15, 20, "dialog_apply", " ", true, false );
    RegisterButton( apply );     
}


void ScreenOptionsWindow::Render( bool _hasFocus )
{
    InterfaceWindow::Render( _hasFocus );

    int x = m_x + 20;
    int y = m_y + 35;
    int h = 30;
    int size = 13;

    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_resolution") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_windowed") );
#ifdef HAVE_REFRESH_RATES	
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_refreshrate") );
#endif
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_colourdepth") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_zbufferdepth") );
    g_renderer->Text( x, y+=h, White, size, LANGUAGEPHRASE("dialog_antialias") );
}
