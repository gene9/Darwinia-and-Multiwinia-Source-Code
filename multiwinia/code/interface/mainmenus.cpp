#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/language_table.h"
#include "lib/targetcursor.h"
#ifdef TARGET_MSVC
#include "lib/input/win32_eventhandler.h"
#endif

#include "interface/mainmenus.h"
#include "interface/prefs_screen_window.h"
#include "interface/prefs_graphics_window.h"
#include "interface/prefs_sound_window.h"
#include "interface/prefs_keybindings_window.h"
#include "interface/prefs_other_window.h"
#include "interface/userprofile_window.h"
#include "interface/mods_window.h"
#include "interface/demoend_window.h"
#include "interface/multiwinia_window.h"
#include "interface/helpandoptions_windows.h"
#include "interface/leaderboards_window.h"
#include "interface/userprofile_window.h"

#include "app.h"
#include "renderer.h"
#include "global_world.h"
#include "game_menu.h"
#include "main.h"

#include "ui/GameMenuWindow.h"

#include "lib/input/input.h"

class WebsiteButton;


GameOptionsWindow::GameOptionsWindow( char *_name)
:   DarwiniaWindow(_name),
    m_renderRightBox(false),
    m_renderWholeScreen(false),
    m_showingErrorDialogue(false),
    m_dialogueSuccessMessage(false)
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    //SetMenuSize( 200, 230 );
//    SetPosition( screenW/2.0f - m_w/2.0f,
//                 screenH/2.0f - m_h/2.0f );

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
    m_resizable = false;
}

void GameOptionsWindow::Create()
{
}

void GameOptionsWindow::Update()
{
    if( m_buttonOrder.Size() > 0 && !m_showingErrorDialogue )
	{
		DarwiniaWindow::Update();
	}
    else if( m_showingErrorDialogue )
    {
        if( g_inputManager->controlEvent( ControlCloseTaskHelp ) )
        {
            m_showingErrorDialogue = false;
            m_errorMessage = UnicodeString();
            m_lockMenu = false;
        }
    }
}

void GameOptionsWindow::Render( bool _hasFocus )
{
    if( !_hasFocus ) 
    {
        return;
    }

    GameMenuWindow::RenderBigDarwinian();


    //
    // Render the title

    float titleX, titleY, titleW, titleH;
    float fontLarge, fontMed, fontSmall;

    GetPosition_TitleBar( titleX, titleY, titleW, titleH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    DrawFillBox( titleX, titleY, titleW, titleH );

    float fontX = titleX + titleW/2;
    float fontY = titleY + titleH * 0.2f;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
	g_titleFont.DrawText2DCentre( fontX, fontY, fontLarge, LANGUAGEPHRASE("multiwinia_mainmenu_title") );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_titleFont.SetRenderOutline(false);
	g_titleFont.DrawText2DCentre( fontX, fontY, fontLarge, LANGUAGEPHRASE("multiwinia_mainmenu_title") );

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2DCentre( fontX, fontY+fontLarge, fontMed, LANGUAGEPHRASE("multiwinia_mainmenu_subtitle") );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2DCentre( fontX, fontY+fontLarge, fontMed, LANGUAGEPHRASE("multiwinia_mainmenu_subtitle") );

    

    //
    // Darwinian logo

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture("icons/mwlogo.bmp" ) ); 
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    float screenW = g_app->m_renderer->ScreenW();
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

    float logoRatio = 277.0f/194.0f;
    float logoX = titleX + titleW * 0.05f;
    float logoY = titleY + titleH * 0.1f;
    float logoH = titleH * 0.8f;
    float logoW = logoH * logoRatio;

    glBegin( GL_QUADS);   
        glTexCoord2f(0,1);  glVertex2f( logoX, logoY );
        glTexCoord2f(1,1);  glVertex2f( logoX+logoW, logoY );
        glTexCoord2f(1,0);  glVertex2f( logoX+logoW, logoY+logoH );
        glTexCoord2f(0,0);  glVertex2f( logoX, logoY+logoH );    
    glEnd();    

    logoX = titleX + titleW - titleW * 0.05f - logoW;

    glBegin( GL_QUADS);   
        glTexCoord2f(1,1);  glVertex2f( logoX, logoY );
        glTexCoord2f(0,1);  glVertex2f( logoX+logoW, logoY );
        glTexCoord2f(0,0);  glVertex2f( logoX+logoW, logoY+logoH );
        glTexCoord2f(1,0);  glVertex2f( logoX, logoY+logoH );    
    glEnd();    

    glDisable( GL_TEXTURE_2D );
    

    //
    // Test render boxes

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;

    GetPosition_LeftBox( leftX, leftY, leftW, leftH );
    GetPosition_RightBox( rightX, rightY, rightW, rightH );
    

    //
    // Render options
    // Build the help string as we go

    if( m_renderWholeScreen )
    {
        DrawFillBox( leftX, leftY, titleW, leftH );
    }
    else
    {
        DrawFillBox( leftX, leftY, leftW, leftH );
        if( m_renderRightBox )
        {
            DrawFillBox( rightX, rightY, rightW, rightH );
        }
    }

    EclWindow::Render( _hasFocus );

    
    if( m_showingErrorDialogue )
    {
        RenderErrorDialogue();
    }
}

void GameOptionsWindow::CreateErrorDialogue( UnicodeString _error, bool _success )
{
    m_errorMessage = _error;
    m_showingErrorDialogue = true;
    m_lockMenu = true;
    m_dialogueSuccessMessage = _success;
}

void GameOptionsWindow::RenderErrorDialogue()
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    int boxW = screenW / 3.0f;
    int boxH = screenH / 3.0f;
    int boxX = (screenW / 2.0f) - (boxW / 2.0f);
    int boxY = (screenH / 2.0f) - (boxH / 2.0f);

    DrawFillBox( boxX, boxY, boxW, boxH );

    float fontLarge, fontMed, fontSmall;
    GetFontSizes( fontLarge, fontMed, fontSmall );

    int yPos = boxY + fontSmall;
    int xPos = screenW / 2.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    UnicodeString header;
    if( m_dialogueSuccessMessage )
    {
        header = LANGUAGEPHRASE("multiwinia_editor_success");
    }
    else
    {
        header = LANGUAGEPHRASE("error");
    }
    g_titleFont.DrawText2DCentre( xPos, yPos, fontMed, header );

    yPos += fontMed *1.5f;

    LList<UnicodeString *> *wrapped = WordWrapText(m_errorMessage, boxW * 1.5f, fontSmall );

    for( int i = 0; i < wrapped->Size(); ++i )
    {
        UnicodeString thisLine(*(wrapped->GetData(i)));
        g_titleFont.DrawText2DCentre( xPos, yPos+=fontSmall*1.1f, fontSmall, thisLine );
    }
    wrapped->EmptyAndDelete();
    delete wrapped;
}


GameMenuInputField *GameOptionsWindow::CreateMenuControl( 
	char const *name, int dataType, NetworkValue *value, int y, float change, 
	DarwiniaButton *callback, int x, int w, float fontSize )
{
    if( x == -1 ) x = 10;
    if( w == -1 ) w = m_w - x * 2;

    GameMenuInputField *input = new GameMenuInputField();
    input->SetShortProperties( (char *)name, x, y, w, 30, LANGUAGEPHRASE(name) );
    input->SetCallback( callback );
    input->m_fontSize = fontSize;
	input->RegisterNetworkValue( dataType, value );
    RegisterButton( input );

    if( dataType != InputField::TypeString )
    {
        float texY = y + fontSize / 2;
        char nameLeft[64];
        sprintf( nameLeft, "%s left", name );
        GameMenuInputScroller *left = new GameMenuInputScroller();
        left->SetProperties( nameLeft, input->m_x + input->m_w - fontSize * 4.0f, texY, fontSize, fontSize, UnicodeString("<"), UnicodeString("Value left") );
        left->m_inputField = input;
        left->m_change = -change;
        left->m_fontSize = fontSize;
        RegisterButton( left );

        char nameRight[64];
        sprintf( nameRight, "%s right", name );
        GameMenuInputScroller *right = new GameMenuInputScroller();
        right->SetProperties( nameRight, input->m_x + input->m_w - fontSize, texY, fontSize, fontSize, UnicodeString(">"), UnicodeString("Value right") );
        right->m_inputField = input;
        right->m_change = change;
        right->m_fontSize = fontSize;
        RegisterButton( right );
    }

	return input;
}


class AboutDarwiniaButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( !EclGetWindow("about_darwinia" ) )
        {
            EclRegisterWindow( new AboutDarwiniaWindow(), m_parent );
        }
    }
};

class MainMenuUserProfileButton : public GameMenuButton
{
public:
    MainMenuUserProfileButton()
    :   GameMenuButton(LANGUAGEPHRASE("dialog_profile"))
    {
    }

    void MouseUp()
    {   
		if (!EclGetWindow("dialog_profile"))
		{
			EclRegisterWindow( new UserProfileWindow(), m_parent );
		}
    }
};


class ModsButton : public GameMenuButton
{
public:
    ModsButton()
    :   GameMenuButton(LANGUAGEPHRASE("dialog_mods"))
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_mods"))
		{
#ifdef USE_DARWINIA_MOD_SYSTEM
			EclRegisterWindow( new ModsWindow(), m_parent );
#endif
		}
    }
};





class MultiwiniaButton : public GameMenuButton
{
public:
    MultiwiniaButton()
    :   GameMenuButton(LANGUAGEPHRASE("dialog_multiwinia"))
    {
    }

    void MouseUp()
    {
        if( !EclGetWindow("dialog_multiwinia"))
        {
            EclRegisterWindow( new MultiwiniaWindow(), m_parent );
        }
    }
};


class ScreenOptionsButton : public GameMenuButton
{
public:
    ScreenOptionsButton()
    : GameMenuButton("dialog_screenoptions") 
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_screenoptions"))
		{
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
			EclRegisterWindow( new PrefsScreenWindow(), m_parent );
            g_app->m_doMenuTransition = true;
		}
    }
};


class GraphicsOptionsButton : public GameMenuButton
{
public:
    GraphicsOptionsButton()
    : GameMenuButton("dialog_graphicsoptions") 
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_graphicsoptions"))
		{
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
			EclRegisterWindow( new PrefsGraphicsWindow(), m_parent );
            g_app->m_doMenuTransition = true;
		}
    }
};


class SoundOptionsButton : public GameMenuButton
{
public:
    SoundOptionsButton()
    : GameMenuButton("dialog_soundoptions") 
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_soundoptions"))
		{
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
	        EclRegisterWindow( new PrefsSoundWindow(), m_parent );
            g_app->m_doMenuTransition = true;
		}
    }
};

class OtherOptionsButton : public GameMenuButton
{
public:
    OtherOptionsButton()
    : GameMenuButton("dialog_otheroptions") 
    {
    }

    void MouseUp()
    {
		if (!EclGetWindow("dialog_otheroptions"))
		{
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
            g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
			EclRegisterWindow( new PrefsOtherWindow(), m_parent );
            g_app->m_doMenuTransition = true;
		}
    }
};


class KeybindingsOptionsButton : public GameMenuButton
{
public:
    KeybindingsOptionsButton()
    : GameMenuButton("dialog_inputoptions") 
    {
    }

	void MouseUp()
	{
		if (!EclGetWindow("dialog_inputoptions"))
		{
			EclRegisterWindow( new PrefsKeybindingsWindow, m_parent );
		}
	}
};


// ****************************************************************************
// Class MainMenuWindow
// ****************************************************************************

MainMenuWindow::MainMenuWindow()
:   GameOptionsWindow("dialog_mainmenu")
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();
    
	SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable(false);
}

void MainMenuWindow::Render( bool _hasFocus )
{
	GameOptionsWindow::Render(_hasFocus);
}


// ***************************************************************************
// Class OptionsMenuWindow
// ***************************************************************************

class MainCloseButton   :   public MenuCloseButton
{
public:
    MainCloseButton( char *_name )
    :   MenuCloseButton( _name )
    {
    }

    void MouseUp()
    {
        if( g_app->m_location )
        {
            int x = g_app->m_renderer->ScreenW() / 2;
            int y = g_app->m_renderer->ScreenH() / 2;
            g_target->SetMousePos( x, y );
        }
        EclRemoveWindow( m_parent->m_name );
        g_app->m_hideInterface = false;
    }
};

OptionsMenuWindow::OptionsMenuWindow()
:   GameOptionsWindow( "dialog_options" )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    //SetMenuSize( 200, 230 );
//    SetPosition( screenW/2.0f - m_w/2.0f,
//                 screenH/2.0f - m_h/2.0f );

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
    m_resizable = false;
}


void OptionsMenuWindow::Create()
{
    GameOptionsWindow::Create();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_settings" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    ScreenOptionsButton *screen = new ScreenOptionsButton();
    screen->SetShortProperties( "dialog_screenoptions", x, y, buttonW, buttonH , LANGUAGEPHRASE("dialog_screenoptions"));
    screen->m_fontSize = fontSize;
    //screen->m_centered = true;
    RegisterButton( screen );
	m_buttonOrder.PutData( screen );

    GraphicsOptionsButton *graphics = new GraphicsOptionsButton();
    graphics->SetShortProperties( "dialog_graphicsoptions", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_graphicsoptions") );
    graphics->m_fontSize = fontSize;
    //graphics->m_centered = true;
    RegisterButton( graphics );
	m_buttonOrder.PutData( graphics );

    SoundOptionsButton *sound = new SoundOptionsButton();
    sound->SetShortProperties( "dialog_soundoptions", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_soundoptions") );
    sound->m_fontSize = fontSize;
    //sound->m_centered = true;
    RegisterButton( sound );
	m_buttonOrder.PutData( sound );

	/*KeybindingsOptionsButton *keys = new KeybindingsOptionsButton();
	keys->SetShortProperties( "dialog_inputoptions", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_inputoptions"));
	keys->m_fontSize = fontSize;
	keys->m_centered = true;
	RegisterButton( keys );
	m_buttonOrder.PutData( keys );*/

    OtherOptionsButton *other = new OtherOptionsButton();
    other->SetShortProperties( "dialog_otheroptions", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_otheroptions") );
    other->m_fontSize = fontSize;
	//other->m_centered = true;
    RegisterButton( other );
	m_buttonOrder.PutData( other );

    float yPos = leftY + leftH - buttonH * 2;

    MenuCloseButton *close = new MenuCloseButton("dialog_back");
    close->SetShortProperties( "dialog_close", x, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    close->m_fontSize = fontSize;
    //close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
	m_backButton = close;

}


// ============================================================================

class ResetLevelButton : public GameMenuButton
{
public:
    ResetLevelButton()
    :   GameMenuButton(UnicodeString())
    {
    }
    void MouseUp()
    {
        EclRegisterWindow( new ResetLocationWindow(), m_parent );
    }
};


class ExitLevelButton : public GameMenuButton
{
public:
    ExitLevelButton()
    :   GameMenuButton("dialog_leavelocation")
    {
    }

    void MouseUp()
    {
		RemoveAllWindows();//EclRemoveWindow( m_parent->m_name );

        
#ifdef DEMO2
        EclRegisterWindow( new DemoEndWindow(1.0f, true) );
#else
        g_app->m_requestedLocationId = -1;    
#endif

#ifdef	TARGET_OS_VISTA
		g_app->m_saveThumbnail = true;
#endif
        if( g_app->m_gameMode != App::GameModeCampaign )
        {
            g_app->m_atMainMenu = true;
        }
    }
};


class WebsiteButton : public GameMenuButton
{
public:
    char m_website[256];

    WebsiteButton()
    :   GameMenuButton(LANGUAGEPHRASE("dialog_visitwebsite"))
    {
    }

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
#ifdef WIN32
			getW32EventHandler()->ResetWindowHandle();
#endif
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

ConfirmExitWindow::ConfirmExitWindow()
:   GameOptionsWindow("multiwinia_confirmexit")
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
}

void ConfirmExitWindow::Create()
{
    GameOptionsWindow::Create();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

	int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY;
    float fontSize = fontMed;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    GameMenuButton *button = new ExitLevelButton();
    button->SetShortProperties( "exit_level", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_yes") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    button = new MenuCloseButton("menu_close");
    button->SetShortProperties( "menu_close", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_no") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
    m_backButton = button;
}

class ConfirmExitButton : public GameMenuButton
{
public:
    ConfirmExitButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
        g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
        EclRegisterWindow( new ConfirmExitWindow(), m_parent );
        g_app->m_doMenuTransition = true;
    }
};


ConfirmResetWindow::ConfirmResetWindow()
:   GameOptionsWindow("multiwinia_confirmreset")
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
}

void ConfirmResetWindow::Create()
{
    GameOptionsWindow::Create();

	int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY;
    float fontSize = fontMed;

    GameMenuButton *button = new MenuCloseButton("menu_close");
    button->SetShortProperties( "menu_close", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_no") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    button = new ResetProfileButton();
    button->SetShortProperties( "reset_profile", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_yes") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
}


ConfirmResetButton::ConfirmResetButton()
:   GameMenuButton(UnicodeString())
{
}

void ConfirmResetButton::MouseUp()
{
    EclRegisterWindow( new ConfirmResetWindow(), m_parent );
}


LocationWindow::LocationWindow()
:   GameOptionsWindow("dialog_locationmenu")
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    /*SetMenuSize( 200, 220 );
    SetPosition( screenW/2.0f - m_w/2.0f,
                 screenH/2.0f - m_h/2.0f );
                 */

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
    m_resizable = false;
}

LocationWindow::~LocationWindow()
{
}

void LocationWindow::Create()
{
    GameOptionsWindow::Create();

    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

	int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY;
    float fontSize = fontMed;

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;
   
    GlobalLocation *loc = g_app->m_globalWorld->GetLocation( g_app->m_locationId );

    //y -= buttonH+gap;

    HelpAndOptionsButton *options = new HelpAndOptionsButton();
    options->SetShortProperties( "dialog_options", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_helpandoptions") );
    options->m_fontSize = fontSize;
    //options->m_centered = true;
    RegisterButton( options );
    m_buttonOrder.PutData( options );

    if( g_app->m_multiwinia->GameOver() )
    {
        ExitLevelButton *exitLevel = new ExitLevelButton();
        exitLevel->SetShortProperties( "multiwinia_menu_quit", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_leavegame") );
        exitLevel->m_fontSize = fontSize;
       // exitLevel->m_centered = true;
        RegisterButton( exitLevel );
        m_buttonOrder.PutData( exitLevel );
    }
    else
    {
        ConfirmExitButton *confirm = new ConfirmExitButton();
        confirm->SetShortProperties( "multiwinia_menu_quit", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_leavegame") );
        confirm->m_fontSize = fontSize;
        RegisterButton( confirm );
        m_buttonOrder.PutData( confirm );

    }

    y = leftY + leftH - buttonH * 2;

    MainCloseButton *close = new MainCloseButton("multiwinia_menu_back");
    close->SetShortProperties( "multiwinia_menu_returntogame", x, y, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    close->m_fontSize = fontSize;
    //close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
    m_backButton = close;
}


// ============================================================================


class ResetLocationButton : public GameMenuButton
{
public:
    ResetLocationButton()
    :   GameMenuButton(UnicodeString())
    {
    }

    void MouseUp()
    {
        EclRemoveWindow( m_parent->m_name );
        EclRemoveWindow( "dialog_locationmenu" );

#ifdef DEMO2
        g_app->ResetLevel(true);
#else
        g_app->ResetLevel(false);
#endif
    }
};


ResetLocationWindow::ResetLocationWindow()
:   GameOptionsWindow( "dialog_resetlocation" )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

    /*SetMenuSize( 200, 220 );
    SetPosition( screenW/2.0f - m_w/2.0f,
                 screenH/2.0f - m_h/2.0f );
                 */

    SetMenuSize( screenW, screenH );
    SetPosition( 0, 0 );
    SetMovable( false );
}


void ResetLocationWindow::Create()
{
    DarwiniaWindow::Create();

	int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    y+= buttonH * 4.0f;

	//int y = m_h - 30;

    /*InvertedBox *box = new InvertedBox();
    box->SetProperties( "invert", border, y+border, m_w - 20, m_h - 2*h, UnicodeString("invert") );
    RegisterButton( box );*/


    ResetLocationButton *reset = new ResetLocationButton();
    reset->SetShortProperties( "dialog_yes", x, y, buttonW, buttonH, LANGUAGEPHRASE("dialog_yes") );
    reset->m_fontSize = fontSize;
    //reset->m_centered = true;
    RegisterButton( reset );
	m_buttonOrder.PutData( reset );

    MenuCloseButton *close = new MenuCloseButton("dialog_no");
    close->SetShortProperties( "dialog_no", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_no") );
    close->m_fontSize = fontSize;
    //close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );
}


void ResetLocationWindow::Render( bool _hasFocus )
{
    GameOptionsWindow::Render( _hasFocus );

    int w = g_app->m_renderer->ScreenW();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontSmall;
    float h = fontSize * 1.2f;

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_titleFont.DrawText2D( x, y+=h, fontSmall, LANGUAGEPHRASE("dialog_reset1") );
    g_titleFont.DrawText2D( x, y+=h, fontSmall, LANGUAGEPHRASE("dialog_reset2") );

    y+=h;

    g_titleFont.DrawText2D( x, y+=h, fontSmall, LANGUAGEPHRASE("dialog_reset3") );
    g_titleFont.DrawText2D( x, y+=h, fontSmall, LANGUAGEPHRASE("dialog_reset4") );
    g_titleFont.DrawText2D( x, y+=h, fontSmall, LANGUAGEPHRASE("dialog_reset5") );
    
}



void MainMenuWindow::Create()
{
    GameOptionsWindow::Create();

#ifdef USE_DARWINIA_MOD_SYSTEM
    bool modsEnabled = g_prefsManager->GetInt( "ModSystemEnabled", 0 ) != 0;
#else
    bool modsEnabled = false;
#endif

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    y -= gap+buttonH;

    MainCloseButton *close = new MainCloseButton("multiwinia_menu_returntogame");
    close->SetShortProperties( "multiwinia_menu_returntogame", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_returntogame") );
    close->m_fontSize = fontSize;
    //close->m_centered = true;
    RegisterButton( close );
	m_buttonOrder.PutData( close );

    MainMenuUserProfileButton *profile = new MainMenuUserProfileButton();
    profile->SetShortProperties( "dialog_profile", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_profile") );
    profile->m_fontSize = fontSize;
   // profile->m_centered = true;
    RegisterButton( profile );
	m_buttonOrder.PutData(profile);

    if( modsEnabled )
    {
        ModsButton *mods = new ModsButton();
        mods->SetShortProperties( "dialog_mods", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_mods") );
        mods->m_fontSize = fontSize;
      //  mods->m_centered = true;
        RegisterButton( mods );
		m_buttonOrder.PutData(mods);
    }

    HelpAndOptionsButton *options = new HelpAndOptionsButton();
    options->SetShortProperties( "dialog_options", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_helpandoptions") );
    options->m_fontSize = fontSize;
    //options->m_centered = true;
    RegisterButton( options );
	m_buttonOrder.PutData( options );

    LeaderBoardButton *button = new LeaderBoardButton("multiwinia_menu_leaderboards");
    button->SetShortProperties( "multiwinia_menu_leaderboards", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_leaderboards") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    button->m_inactive = true;
    m_buttonOrder.PutData( button );

    AchievementsButton *achievements = new AchievementsButton();
    achievements->SetShortProperties( "achievements", x, y+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_achievements") );
    achievements->m_fontSize = fontSize;
    RegisterButton( achievements );
    m_buttonOrder.PutData( achievements );

#ifdef OBERONBUILD
    AboutDarwiniaButton *adb = new AboutDarwiniaButton();
    adb->SetShortProperties( "about_darwinia", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("about_darwinia") );
    adb->m_fontSize = fontSize;
    //adb->m_centered = true;
    RegisterButton( adb );
	m_buttonOrder.PutData(adb);
#else
	WebsiteButton *website = new WebsiteButton();
    website->SetShortProperties( "dialog_visitwebsite", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_visitwebsite") );
    website->m_fontSize = fontSize;
   // website->m_centered = true;
#ifdef EGAMESBUILD
    strcpy( website->m_website, "http://www.savedarwinia.com" );
#else
    strcpy( website->m_website, "http://www.multiwinia.co.uk/" );
#endif
    RegisterButton( website );
	m_buttonOrder.PutData(website);

    MultiwiniaButton *multiwinia = new MultiwiniaButton();
    multiwinia->SetShortProperties( "dialog_multiwinia", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_multiwinia") );
    multiwinia->m_fontSize = fontSize;
    //multiwinia->m_centered = true;
    RegisterButton( multiwinia );
    m_buttonOrder.PutData( multiwinia );
#endif

   // int buttonY = (leftY + leftH) - (buttonH * 2.0f) - buttonH * 0.3f;

    MenuGameExitButton *exit = new MenuGameExitButton();
    exit->SetShortProperties( "dialog_leavedarwinia", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("dialog_leavedarwinia") );
    exit->m_fontSize = fontSize;
    //exit->m_centered = true;
    RegisterButton( exit );
	m_buttonOrder.PutData(exit);
}

AboutDarwiniaWindow::AboutDarwiniaWindow()
:   DarwiniaWindow( "about_darwinia" )
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
    close->SetShortProperties( "dialog_close", border, m_h - h, buttonW, buttonH, LANGUAGEPHRASE("dialog_close") );
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

	UnicodeString tempUString = LANGUAGEPHRASE("bootloader_credits_4") + LANGUAGEPHRASE("bootloader_credits_5");

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, APP_NAME " " DARWINIA_VERSION );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=2*h, fontSize, tempUString );
    g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, fontSize, "http://www.introversion.co.uk" );

    /*glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );
    glBegin( GL_QUADS );
        glTexCoord2f( 0.0f, 1.0f );     glVertex2f( m_x + 10, m_y + 30 );
        glTexCoord2f( 1.0f, 1.0f );     glVertex2f( m_x + GetMenuSize(60), m_y + 30 );
        glTexCoord2f( 1.0f, 0.0f );     glVertex2f( m_x + GetMenuSize(60), m_y + GetMenuSize(80) );
        glTexCoord2f( 0.0f, 0.0f );     glVertex2f( m_x + 10, m_y + GetMenuSize(80) );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );

    float y = m_y+20;
    float h = GetMenuSize(18);

	float fontSize = GetMenuSize(13);

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.DrawText2D( m_x+GetMenuSize(80), y+=h, fontSize, APP_NAME " " DARWINIA_VERSION );
    g_gameFont.DrawText2D( m_x+GetMenuSize(80), y+=h, fontSize, LANGUAGEPHRASE("about_developer") );
    //g_gameFont.DrawText2D( m_x+GetMenuSize(80), y+=h, fontSize, "http://www.multiwinia.co.uk/" );
    g_gameFont.DrawText2D( m_x+GetMenuSize(80), y+=h, fontSize, "http://www.introversion.co.uk" );*/

}

HelpAndOptionsButton::HelpAndOptionsButton()
:   GameMenuButton( UnicodeString() )
{
}

void HelpAndOptionsButton::MouseUp()
{
	GameMenuButton::MouseUp();
    EclRegisterWindow( new HelpAndOptionsWindow(), m_parent );
    g_app->m_doMenuTransition = true;
}

LeaderBoardButton::LeaderBoardButton(char *_iconName)
:	GameMenuButton(_iconName)
{
}

void LeaderBoardButton::MouseUp()
{
    if( m_inactive ) return;

    GameMenuButton::MouseUp();

	EclRegisterWindow( new LeaderboardSelectionWindow(), m_parent );
    g_app->m_doMenuTransition = true;
}