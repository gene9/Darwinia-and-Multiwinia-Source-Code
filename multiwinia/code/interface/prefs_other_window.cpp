#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/resource.h"
#include "network/network_defines.h"
#include "network/clienttoserver.h"
#include "interface/prefs_other_window.h"
#include "interface/drop_down_menu.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "level_file.h"
#include "water.h"
#include "main.h"

class ApplyOtherButton : public GameMenuButton
{
public:
    ApplyOtherButton()
    :   GameMenuButton("dialog_apply")
    {
    }

    void MouseUp()
    {
        if( m_inactive ) return;

        PrefsOtherWindow *parent = (PrefsOtherWindow *) m_parent;
        
        g_prefsManager->SetInt( OTHER_HELPENABLED, parent->m_helpEnabled );

		g_prefsManager->SetInt( OTHER_CONTROLHELPENABLED, parent->m_controlHelpEnabled );
		
		if (g_app->m_locationId == -1)
		{
			// Only set the difficulty from the top level
			// Preferences value is 1-based, m_difficultyLevel is 0-based.			
			g_prefsManager->SetInt( OTHER_DIFFICULTY, parent->m_difficulty + 1 );
			g_app->m_difficultyLevel = parent->m_difficulty;
		}
		
        if( parent->m_bootLoader == 0 ) 
        {
            g_prefsManager->SetString( OTHER_BOOTLOADER, "none" );
        }
        else if( parent->m_bootLoader == 1 )
        {
            g_prefsManager->SetString( OTHER_BOOTLOADER, "random" );
        }
        else if( parent->m_bootLoader == 2 )
        {
            g_prefsManager->SetString( OTHER_BOOTLOADER, "firsttime" );
        }

        if( g_app->m_location )
        {
            LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
		    g_app->m_location->m_landscape.Init(def);

            delete g_app->m_location->m_water;
            g_app->m_location->m_water = new Water();
        }

		// Network settings

		if( g_prefsManager->GetInt( OTHER_USEPORTFORWARDING ) != parent->m_usePortForwarding ||
			g_prefsManager->GetInt( OTHER_SERVERPORT ) != parent->m_serverPort ||
			g_prefsManager->GetInt( OTHER_CLIENTPORT ) != parent->m_clientPort )
		{
			g_prefsManager->SetInt( OTHER_USEPORTFORWARDING, parent->m_usePortForwarding );
			g_prefsManager->SetInt( OTHER_SERVERPORT, parent->m_serverPort );
			g_prefsManager->SetInt( OTHER_CLIENTPORT, parent->m_clientPort ); 

			if( g_app->m_clientToServer )
				g_app->m_clientToServer->OpenConnections();
		}

		bool removeWindows = false;
        char *desiredLanguage = parent->m_languages[ parent->m_language ];
        if( stricmp( desiredLanguage, g_prefsManager->GetString( OTHER_LANGUAGE ) ) != 0 )
        {
            g_prefsManager->SetString( OTHER_LANGUAGE, desiredLanguage );
            g_app->SetLanguage( desiredLanguage, false );
            
	        removeWindows = true;
        }

		g_prefsManager->SetInt( OTHER_AUTOMATICCAM, parent->m_automaticCamera );		

		bool oldMode = g_app->m_largeMenus;
		g_prefsManager->SetInt( OTHER_LARGEMENUS, parent->m_largeMenus );
		if( parent->m_largeMenus == 2 ) // (todo) or is running in media center and tenFootMode == -1
		{
			g_app->m_largeMenus = true;

		}
#ifdef TARGET_OS_VISTA
        else if( parent->m_largeMenus == 0 &&
            g_mediaCenter == true )
        {
            g_app->m_largeMenus = true;
        }
#endif
		else
		{
			g_app->m_largeMenus = false;
		}

		if( oldMode != g_app->m_largeMenus ) // tenFootMode option has changed, close all windows
		{
			removeWindows = true;
		}
		
		if( removeWindows )
		{
			RemoveAllWindows();
            if( g_app->m_atMainMenu )
            {
                g_app->m_gameMenu->CreateMenu();
            }
            else
            {
                EclRegisterWindow( new LocationWindow() );
            }
		}
        
        g_prefsManager->Save();
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        PrefsOtherWindow *parent = (PrefsOtherWindow *) m_parent;

        bool langMatch = false;
        for( int i = 0; i < parent->m_languages.Size(); ++i )
        {
            if( stricmp( parent->m_languages[i], g_prefsManager->GetString( OTHER_LANGUAGE ) ) == 0 )
            {
                if(parent->m_language == i) langMatch = true;
            }
        }

        if( parent->m_helpEnabled == g_prefsManager->GetInt( OTHER_HELPENABLED ) &&
            langMatch &&
			g_prefsManager->GetInt( OTHER_USEPORTFORWARDING ) == parent->m_usePortForwarding &&
			g_prefsManager->GetInt( OTHER_SERVERPORT ) == parent->m_serverPort &&
			g_prefsManager->GetInt( OTHER_CLIENTPORT ) == parent->m_clientPort )
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


PrefsOtherWindow::PrefsOtherWindow()
:   GameOptionsWindow( "dialog_otheroptions" )
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();
    SetMenuSize( w, h );
    SetPosition( 0, 0 );
    SetMovable(false);
	m_resizable = false;

    m_helpEnabled = g_prefsManager->GetInt( OTHER_HELPENABLED, 1 );
	m_controlHelpEnabled = g_prefsManager->GetInt( OTHER_CONTROLHELPENABLED, 1 );
	m_usePortForwarding = g_prefsManager->GetInt( OTHER_USEPORTFORWARDING, 0 );
	m_serverPort = g_prefsManager->GetInt( OTHER_SERVERPORT, 4000 );
	m_clientPort = g_prefsManager->GetInt( OTHER_CLIENTPORT, 4001 );

    const char *bootloader = g_prefsManager->GetString( OTHER_BOOTLOADER, "random" );
    if      ( stricmp( bootloader, "none" ) == 0 )      m_bootLoader = 0;
    else if ( stricmp( bootloader, "random" ) == 0 )    m_bootLoader = 1;
    else                                                m_bootLoader = 2;

    m_christmas = g_prefsManager->GetInt( OTHER_CHRISTMASENABLED, 1 );
	if( g_app->m_locationId == -1 ) {
		m_difficulty = g_prefsManager->GetInt( OTHER_DIFFICULTY, 1 ) - 1;
		if( m_difficulty < 0 ) m_difficulty = 0;
	}
	else
	{	
		m_difficulty = g_app->m_difficultyLevel;
	}

	m_largeMenus = g_prefsManager->GetInt( OTHER_LARGEMENUS, 0 );
    m_automaticCamera = g_prefsManager->GetInt( OTHER_AUTOMATICCAM, 0 );
	
    ListAvailableLanguages();
    m_language = -1;
    for( int i = 0; i < m_languages.Size(); ++i )
    {
        if( stricmp( m_languages[i], g_prefsManager->GetString( OTHER_LANGUAGE ) ) == 0 )
        {
            m_language = i;
        }
    }
}


void PrefsOtherWindow::ListAvailableLanguages()
{
    m_languages.EmptyAndDelete();

    LList<char *> *fileList = g_app->m_resource->ListResources( "language/", "*.*", false );
    for( int i = 0; i < fileList->Size(); ++i )
    {
        char *lang = fileList->GetData(i);
        char *dot = strrchr( lang, '.' );
        if( dot ) *dot = '\x0';
        m_languages.PutData( strdup( lang ) );
    }
    fileList->EmptyAndDeleteArray();
	delete fileList;
}


void PrefsOtherWindow::Create()
{
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

    y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

    buttonH *= 0.65f;
    float border = gap + buttonH;

	// Keep all the coordinates on integer offsets so that we don't get
	// rounding error creeping in

	y = int(y);
	border = int(border);
	buttonH = int(buttonH);

    GameMenuCheckBox *helpEnabled = new GameMenuCheckBox();
    helpEnabled->SetShortProperties( "dialog_helpsystem", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_helpsystem") );
    helpEnabled->RegisterInt( &m_helpEnabled );
	helpEnabled->m_fontSize = fontSize;
    RegisterButton( helpEnabled );
	m_buttonOrder.PutData( helpEnabled );

	bool networkingOptionDisabled = g_app->m_server != NULL;
	
    GameMenuCheckBox *usePortforwarding = new GameMenuCheckBox();
    usePortforwarding->SetShortProperties( "dialog_useportforwarding", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_useportforwarding") );
    usePortforwarding->RegisterInt( &m_usePortForwarding );
	usePortforwarding->m_fontSize = fontSize;
	usePortforwarding->m_disabled = networkingOptionDisabled;
    RegisterButton( usePortforwarding );
	m_buttonOrder.PutData( usePortforwarding );	

	GameMenuInputField *serverPort = new GameMenuInputField();
	serverPort->SetShortProperties( "dialog_serverport", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_serverport") );
	NetworkInt *serverPortNI = new LocalInt( m_serverPort );
	serverPortNI->SetBounds( 0.0, 65535.1 );
	serverPort->RegisterNetworkValue( InputField::TypeString, new NetworkIntAsString( serverPortNI ) );
	serverPort->m_fontSize = fontSize;
	serverPort->m_disabled = networkingOptionDisabled;
	RegisterButton( serverPort );
	m_buttonOrder.PutData( serverPort );

	GameMenuInputField *clientPort = new GameMenuInputField();
	clientPort->SetShortProperties( "dialog_clientport", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_clientport") );
	NetworkInt *clientPortNI = new LocalInt( m_clientPort );
	clientPortNI->SetBounds( 0.0, 65535.1 );
	clientPort->RegisterNetworkValue( InputField::TypeString, new NetworkIntAsString( clientPortNI ) );
	clientPort->m_fontSize = fontSize;
	clientPort->m_disabled = networkingOptionDisabled;
	RegisterButton( clientPort );
	m_buttonOrder.PutData( clientPort );

// On OS X we get the language setting directly from the OS
#ifndef TARGET_OS_MACOSX
    GameMenuDropDown *language = new GameMenuDropDown();
    language->SetShortProperties( "dialog_language", x, y+=border, buttonW, buttonH, LANGUAGEPHRASE("dialog_language") );
    for( int i = 0; i < m_languages.Size(); ++i )
    {
        char languageString[256];
        sprintf( languageString, "language_%s", m_languages[i] );
		
        if( ISLANGUAGEPHRASE(languageString) )
        {
            language->AddOption( languageString );
        }
        else
        {
            language->AddOption( m_languages[i] );
        }
    }
    language->RegisterInt( &m_language );
	language->m_fontSize = fontSize;
    RegisterButton( language );
	m_buttonOrder.PutData( language );
#endif

    buttonH /= 0.65f;
    int buttonY = leftY + leftH - buttonH * 3;    

	ApplyOtherButton *apply = new ApplyOtherButton();
    apply->SetShortProperties( "dialog_apply", x, buttonY, buttonW, buttonH, LANGUAGEPHRASE("dialog_apply") );
    apply->m_fontSize = fontMed;
    //apply->m_centered = true;
    RegisterButton( apply );    
	m_buttonOrder.PutData( apply );

	MenuCloseButton *cancel = new MenuCloseButton("dialog_close");
    cancel->SetShortProperties( "dialog_close", x, buttonY + buttonH + 5, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    cancel->m_fontSize = fontMed;
    //cancel->m_centered = true;    
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );
	m_backButton = cancel;
}


void PrefsOtherWindow::Render( bool _hasFocus )
{
    GameOptionsWindow::Render( _hasFocus );
    return;

    DarwiniaWindow::Render( _hasFocus );
        
	int border = GetClientRectX1() + 10;
	int size = GetMenuSize(13);
    int x = m_x + 20;
    int y = m_y + GetClientRectY1() + border*2;
    int h = GetMenuSize(20) + border;

    g_editorFont.DrawText2D( x, y+=border, size, LANGUAGEPHRASE("dialog_helpsystem") );
	g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_controlhelpsystem") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_bootloaders") );
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_language") );
	
#ifndef DEMOBUILD	
	if (g_app->m_locationId != -1)
		glColor4f( 0.5, 0.5, 0.5, 1.0 );		
		
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_difficulty") );	
#endif // DEMOBUILD
	
	glColor4f( 1.0, 1.0, 1.0, 1.0 );    
//    if( Location::ChristmasModEnabled() )
    {
     //   g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_christmas") );
    }

	g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_largemenus") );	
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_autocam") );	

    g_editorFont.DrawText2DCentre( m_x+m_w/2.0, m_y+m_h - GetMenuSize(50), GetMenuSize(15), DARWINIA_VERSION_STRING );
}





