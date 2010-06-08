#include "lib/universal_include.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"
#include "lib/filesys_utils.h"
#include "lib/resource.h"

#include "interface/prefs_other_window.h"
#include "interface/drop_down_menu.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "level_file.h"
#include "water.h"
#include "main.h"

class ApplyOtherButton : public DarwiniaButton
{
    void MouseUp()
    {
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

        if( Location::ChristmasModEnabled() )
        {
            g_prefsManager->SetInt( OTHER_CHRISTMASENABLED, parent->m_christmas );
        }

        if( g_app->m_location )
        {
            LandscapeDef *def = &g_app->m_location->m_levelFile->m_landscape;
		    g_app->m_location->m_landscape.Init(def);

            delete g_app->m_location->m_water;
            g_app->m_location->m_water = new Water();
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
			LList<EclWindow *> *windows = EclGetWindows();
	        while (windows->Size() > 0) {
		        EclWindow *w = windows->GetData(0);
                EclRemoveWindow(w->m_name);
	        }
		}
        
        g_prefsManager->Save();
    }
};


PrefsOtherWindow::PrefsOtherWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("dialog_otheroptions") )
{
    SetMenuSize( 468, 350 );

    SetPosition( g_app->m_renderer->ScreenW()/2 - m_w/2, 
                 g_app->m_renderer->ScreenH()/2 - m_h/2 );

    m_helpEnabled = g_prefsManager->GetInt( OTHER_HELPENABLED, 1 );
	m_controlHelpEnabled = g_prefsManager->GetInt( OTHER_CONTROLHELPENABLED, 1 );

    char *bootloader = g_prefsManager->GetString( OTHER_BOOTLOADER, "random" );
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
    fileList->EmptyAndDelete();
}


void PrefsOtherWindow::Create()
{
    DarwiniaWindow::Create();

    /*int x = GetMenuSize(150);
    int w = GetMenuSize(170);
    int y = 30;
    int h = GetMenuSize(30);*/

	int y = GetClientRectY1();
	int border = GetClientRectX1() + 10;
	int x = m_w/2;
	int buttonH = GetMenuSize(20);
	int buttonW = m_w/2 - border * 2;
	int h = buttonH + border;
	int fontSize = GetMenuSize(13);
    
    InvertedBox *box = new InvertedBox();
    box->SetShortProperties( "invert", 10, y += border, m_w - 20, h * 7 + border * 2 );        
    RegisterButton( box );

    DropDownMenu *helpEnabled = new DropDownMenu();
    helpEnabled->SetShortProperties( LANGUAGEPHRASE("dialog_helpsystem"), x, y+=border, buttonW, buttonH );
    helpEnabled->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    helpEnabled->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    helpEnabled->RegisterInt( &m_helpEnabled );
	helpEnabled->m_fontSize = fontSize;
    RegisterButton( helpEnabled );
	m_buttonOrder.PutData( helpEnabled );

    DropDownMenu *controlHelpEnabled = new DropDownMenu();
    controlHelpEnabled->SetShortProperties( LANGUAGEPHRASE("dialog_controlhelpsystem"), x, y+=h, buttonW, buttonH );
    controlHelpEnabled->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    controlHelpEnabled->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    controlHelpEnabled->RegisterInt( &m_controlHelpEnabled );
	controlHelpEnabled->m_fontSize = fontSize;
    RegisterButton( controlHelpEnabled );
	m_buttonOrder.PutData( controlHelpEnabled );

    DropDownMenu *bootLoader = new DropDownMenu();
    bootLoader->SetShortProperties( LANGUAGEPHRASE("dialog_bootloaders"), x, y+=h, buttonW, buttonH );
    bootLoader->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
    bootLoader->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
    bootLoader->AddOption( LANGUAGEPHRASE("intro_bootloader"), 2 );
    bootLoader->RegisterInt( &m_bootLoader );
	bootLoader->m_fontSize = fontSize;
    RegisterButton( bootLoader );
	m_buttonOrder.PutData( bootLoader );

    DropDownMenu *language = new DropDownMenu();
    language->SetShortProperties( LANGUAGEPHRASE("dialog_language"), x, y+=h, buttonW, buttonH );
    for( int i = 0; i < m_languages.Size(); ++i )
    {
        char languageString[256];
        sprintf( languageString, "language_%s", m_languages[i] );
        if( ISLANGUAGEPHRASE(languageString) )
        {
            language->AddOption( LANGUAGEPHRASE(languageString) );
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

#ifndef DEMOBUILD	
	DropDownMenu *difficulty = new DropDownMenu();
	difficulty->SetShortProperties( LANGUAGEPHRASE("dialog_difficulty"), x, y+=h, buttonW, buttonH );
    
	for (int i = 0; i < 10; i++) {	
		char option[32];
		switch (i) {
			case 0:
				sprintf(option, "%d (%s)", i + 1, LANGUAGEPHRASE("dialog_standard_difficulty"));
				break;
				
			case 9:
				sprintf(option, "%d (%s)", i + 1, LANGUAGEPHRASE("dialog_hard_difficulty"));
				break;
				
			default: 
				sprintf(option, "%d", i + 1);
				break;
		}
		difficulty->AddOption(option, i);
	}
	difficulty->RegisterInt( &m_difficulty );
	difficulty->SetDisabled( g_app->m_locationId != -1 );
	difficulty->m_fontSize = fontSize;
	RegisterButton(difficulty);
	m_buttonOrder.PutData( difficulty );
#endif // DEMOBUILD

    if( Location::ChristmasModEnabled() )
    {
        DropDownMenu *christmas = new DropDownMenu();
        christmas->SetShortProperties( LANGUAGEPHRASE("dialog_christmas"), x, y+=h, buttonW, buttonH );
        christmas->AddOption( LANGUAGEPHRASE("dialog_enabled"), 1 );
        christmas->AddOption( LANGUAGEPHRASE("dialog_disabled"), 0 );
        christmas->RegisterInt( &m_christmas );
		christmas->m_fontSize = fontSize;
        RegisterButton( christmas );
		m_buttonOrder.PutData( christmas );

        box->m_h = h * 8 + border * 2;
        SetMenuSize( 390, 380 );
    }

	DropDownMenu *tenfoot = new DropDownMenu();
	tenfoot->SetShortProperties( LANGUAGEPHRASE("dialog_largemenus"), x, y+=h, buttonW, buttonH );
	tenfoot->AddOption( LANGUAGEPHRASE("dialog_auto"), 0 );
	tenfoot->AddOption( LANGUAGEPHRASE("dialog_disabled"), 1 );
	tenfoot->AddOption( LANGUAGEPHRASE("dialog_enabled"), 2 );
	tenfoot->RegisterInt( &m_largeMenus );
	tenfoot->m_fontSize = fontSize;
	RegisterButton( tenfoot );
	m_buttonOrder.PutData( tenfoot );

    DropDownMenu *autoCam = new DropDownMenu();
	autoCam->SetShortProperties( LANGUAGEPHRASE("dialog_autocam"), x, y+=h, buttonW, buttonH );
	autoCam->AddOption( LANGUAGEPHRASE("dialog_auto"), 0 );
	autoCam->AddOption( LANGUAGEPHRASE("dialog_disabled"), 1 );
	autoCam->AddOption( LANGUAGEPHRASE("dialog_enabled"), 2 );
	autoCam->RegisterInt( &m_automaticCamera );
	autoCam->m_fontSize = fontSize;
	RegisterButton( autoCam );
	m_buttonOrder.PutData( autoCam );

	y = m_h - h;

	CloseButton *cancel = new CloseButton();
    cancel->SetShortProperties( LANGUAGEPHRASE("dialog_close"), border, y, buttonW, buttonH );
    cancel->m_fontSize = fontSize;
    cancel->m_centered = true;    
    RegisterButton( cancel );
	m_buttonOrder.PutData( cancel );

    ApplyOtherButton *apply = new ApplyOtherButton();
    apply->SetShortProperties( LANGUAGEPHRASE("dialog_apply"), m_w - buttonW - border, y, buttonW, buttonH );
    apply->m_fontSize = fontSize;
    apply->m_centered = true;
    RegisterButton( apply );    
	m_buttonOrder.PutData( apply );
}


void PrefsOtherWindow::Render( bool _hasFocus )
{
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
		glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );		
		
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_difficulty") );	
#endif // DEMOBUILD
	
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );    
    if( Location::ChristmasModEnabled() )
    {
        g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_christmas") );
    }

	g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_largemenus") );	
    g_editorFont.DrawText2D( x, y+=h, size, LANGUAGEPHRASE("dialog_autocam") );	

    g_editorFont.DrawText2DCentre( m_x+m_w/2.0f, m_y+m_h - GetMenuSize(50), GetMenuSize(15), DARWINIA_VERSION_STRING );
}

