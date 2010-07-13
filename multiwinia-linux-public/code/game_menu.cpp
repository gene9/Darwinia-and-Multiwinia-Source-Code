#include "lib/universal_include.h"

#include <fstream>

#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/input/keydefs.h"
#include "lib/language_table.h"
#include "lib/resource.h"
#include "lib/targetcursor.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "lib/metaserver/metaserver_defines.h"
#include "lib/math/filecrc.h"
#include "lib/math/math_utils.h"
#include "sound/soundsystem.h"

#include "lib/window_manager.h"
#ifdef TARGET_MSVC
	#include "lib/input/win32_eventhandler.h"
#endif

#include "lib/metaserver/authentication.h"

#include "interface/drop_down_menu.h"
#include "interface/input_field.h"
#include "interface/mainmenus.h"
#include "interface/helpandoptions_windows.h"
#include "interface/scrollbar.h"
#include "interface/checkbox.h"
#include "interface/multiwinia_window.h"

#include "network/network_defines.h"
#include "network/clienttoserver.h"
#include "network/servertoclient.h"
#include "network/server.h"

#include "game_menu.h"

#include "main.h"
#include "camera.h"
#include "app.h"
#include "global_world.h"
#include "global_internet.h"
#include "multiwinia.h"
#include "renderer.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "tutorial.h"
#endif
#include "startsequence.h"
#include "achievement_tracker.h"
#include "clouds.h"
#include "loading_screen.h"
#include "level_file.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "demoendsequence.h"
#endif
#include "mapfile.h"

#include "UI/ImageButton.h"
#include "UI/MapData.h"
#include "UI/GameMenuWindow.h"

#include "lib/metaserver/metaserver_defines.h"
#include "lib/metaserver/metaserver.h"

const char *s_pageNames[GameMenuWindow::NumPages] = 
{
	"Main",
    "Darwinia",
    "GameSelect",
	"NewOrJoin",
	"JoinGame",
	"QuickMatchGame",
    "GameSetup",
	"GameConnecting",
    "Research",
    "MapSelect",
    "HelpMenu",
    "Help",
    "AdvancedOptions",
	"XBLASelectSessionType",
	"XBLASignIn",
    "DemoSinglePlayer",
    "BuyMeNow",
	"LeaderBoardSelect",
	"Leaderboards",
    "XBLASelectGameType",
    "XBLASelectCustomMatchtype",
	"Downloaded PDLC",
	"PDLC Upsell",
    "AuthKeyEntry",
	"PlayerOptions",
    "SearcHFilters",
	"DarwiniaPlus SP Page",
	"DarwiniaPlus MP Page",
	"DarwiniaPlus Darwinia Page",
	"DarwiniaPlus Darwinia SP Page",
	"DarwiniaPlus Darwinia MP Page",
	"Updates Available Page",
    "Credits Page",
    "Tutorial Page",
    "Error Page",
	"Achievements Page",
    "Password Entry",
	"Wait for Game to Start",
};

char *GetMapNameId( char *_filename )
{
    static char nameString[256];
    strcpy( nameString, _filename );
    nameString[ strlen( nameString ) - 4 ] = '\0';
    sprintf( nameString, "%s_name", nameString );

    return nameString;
}

char *GetMapDescriptionId( char *_filename )
{
    static char descriptionString[256];
    strcpy( descriptionString, _filename );
    descriptionString[ strlen( descriptionString ) - 4 ] = '\0';
    sprintf( descriptionString, "%s_description", descriptionString );

    return descriptionString;
}

void PlayerNameString::Set( const UnicodeString &_x )
{
	m_name = _x;
	m_name.Truncate( m_maxLength );

    UnicodeString name;
    if( _x.Length() == 0 )
    {
        name = LANGUAGEPHRASE("multiwina_playername_default");
    }
    else
    {
        name = m_name;
    }

    g_prefsManager->SetString("PlayerName", name );
    g_prefsManager->Save();

    ClientToServer *cToS = g_app->m_clientToServer;
    if( cToS )
    {
	    cToS->ReliableSetTeamName( name );
    }		
    AppDebugOut("PlayerNameString::Set called at %2.4f\n", GetHighResTime() );
}


void GameTypeToCaption( int _gameType, char* _dest )
 {
	if( Multiwinia::s_gameBlueprints.ValidIndex( _gameType ) )
		sprintf(_dest, Multiwinia::s_gameBlueprints[_gameType]->GetName());
	else
		sprintf(_dest, "");
}


void GameTypeToShortCaption( int _gameType, char* _dest )
{
    if( Multiwinia::s_gameBlueprints.ValidIndex( _gameType ) )
    {
        sprintf( _dest, "multiwinia_gametypeshort_%s", Multiwinia::s_gameBlueprints[_gameType]->m_name );
    }
    else
    {
        GameTypeToCaption( _gameType, _dest );
    }
}


void RemoveDelistedServers( LList<Directory *> *_serverList )
{
	int serverListSize = _serverList->Size();

	for( int i = 0; i < serverListSize; i++ )
	{
		Directory *server = _serverList->GetData( i );

		if( server->HasData( NET_METASERVER_DELISTED ) )
		{
			_serverList->RemoveData( i );
			delete server;
			i--;
			serverListSize--;
		}
	}
}

// *************************
// Button Classes
// *************************

EclButton *GameMenuButton::s_previousHighlightButton = NULL;

GameMenuButton::GameMenuButton( const UnicodeString &caption )
:   m_useEditorFont(false),
	m_ir(0.5f), m_ig(0.5f), m_ib(0.5f), m_ia(1.0f),
    m_transitionDirection(1)
{
    m_fontSize = 20;
    m_caption = caption;
    m_birthTime = GetHighResTime();
}


void GameMenuButton::MouseUp()
{

    if( m_transitionDirection == 1 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuCancel", SoundSourceBlueprint::TypeMultiwiniaInterface );
    }
	g_app->m_renderer->InitialiseMenuTransition( 1.0f, m_transitionDirection );
}

void GameMenuButton::LockController()
{
}

void GameMenuButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
    
    float realXf = (float)realX;
    double timeNow = GetHighResTime();
    int numButtons = parent->m_buttonOrder.Size();
    double showTime = m_birthTime + 0.6f * (float)realY / (float)g_app->m_renderer->ScreenH();
    double flashShowTime = m_birthTime + (5.0f/numButtons) * (float)realY / (float)g_app->m_renderer->ScreenH();

    if( m_birthTime > 0.0f && timeNow < flashShowTime )
    {
        realXf -= ( showTime - timeNow ) * g_app->m_renderer->ScreenW() * 0.1f;            

        float boxAlpha = (flashShowTime-timeNow);
        Clamp( boxAlpha, 0.0f, 0.9f );

        glColor4f( 1.0f, 1.0f, 1.0f, boxAlpha );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glBegin( GL_QUADS);
            glVertex2f( realXf, realY );
            glVertex2f( realXf + m_w, realY );
            glVertex2f( realXf + m_w, realY + m_h );
            glVertex2f( realXf, realY + m_h );
        glEnd();
    }


    if( !m_mouseHighlightMode )
    {
        highlighted = false;
    }

    if( parent->m_buttonOrder[parent->m_currentButton] == this )
    {
        highlighted = true;
    }

    if( highlighted )
    {
        //RenderHighlightBeam( realX, realY+m_h/2.0f, m_w, m_h );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
        glBegin( GL_QUADS);
            glVertex2f( realXf, realY );
            glVertex2f( realXf + m_w, realY );
            glVertex2f( realXf + m_w, realY + m_h );
            glVertex2f( realXf, realY + m_h );
        glEnd();
    }

    UpdateButtonHighlight();

    
    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);

    float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;
    TextRenderer *font = &g_titleFont;
    //if( m_useEditorFont ) font = &g_editorFont;

    if( m_centered )
    {
        font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
    }
    else
    {
        font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );
    }

    glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );

    if( m_useEditorFont ) glColor4f( 1.0f, 1.0f, 0.3f, 1.0f );


    if( highlighted  )
    {
        glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
    }

    if( m_inactive ) glColor4f( m_ir, m_ig, m_ib, m_ia );
    
    g_titleFont.SetRenderOutline(false);

    if( m_centered )
    {
        font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
    }
    else
    {
        font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );
    }

    if( m_birthTime > 0.0f )
    {
        if( timeNow >= showTime )
        {
            m_birthTime = -timeNow;
        }
    }
    else if( m_birthTime > -999 )
    {
        float fxLen = 0.1f;
        float timePassed = timeNow - (m_birthTime * -1);

        glColor4f( 1.0f, 1.0f, 1.0f, std::max(0.0f, 1.0f-timePassed/fxLen) );
        if( m_centered )
        {
            font->DrawText2DCentre( realXf + m_w/2, texY, m_fontSize, m_caption );
        }
        else
        {
            font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize, m_caption );

            glColor4f( 1.0f, 1.0f, 1.0f, std::max(0.0f, 0.5f-timePassed/fxLen) );
            font->DrawText2D( realXf + m_w*0.05f, texY, m_fontSize*1.3f, m_caption );
        }    
        
        if( timePassed >= fxLen ) m_birthTime = -1000;
    }


    if( highlighted && s_previousHighlightButton != this )
    {
        if( s_previousHighlightButton != NULL )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
        }
        s_previousHighlightButton = this;
    }
}


BackPageButton::BackPageButton( int _page )
:   GameMenuButton( "multiwinia_menu_back" )
{		
    m_pageId = _page;
}

void BackPageButton::MouseUp()
{
	GameMenuButton::MouseUp();
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuCancel", SoundSourceBlueprint::TypeMultiwiniaInterface );
	g_app->m_renderer->InitialiseMenuTransition( 1.0f, -1 );


    ((GameMenuWindow *) m_parent)->m_newPage = m_pageId;
}

void BackPageButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    GameMenuButton::Render( realX, realY, highlighted, clicked );

    if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
    {
        float iconSize = m_fontSize;
        float iconGap = 10.0f;
        float iconAlpha = 1.0f;
		int xPos = realX + g_gameFont.GetTextWidth( m_caption.Length(), m_fontSize ) + 60;
        int yPos = realY + (m_h / 2.0f);

        if( m_centered )
        {
            xPos += m_w / 2.0f;
        }

        Vector2 iconCentre = Vector2(xPos, yPos);            

        // Render the icon

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/button_b.bmp" ) );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        glColor4f   ( 1.0f, 1.0f, 1.0f, iconAlpha );

        float x = iconSize/2;

        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );           glVertex2f( iconCentre.x - x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 1 );           glVertex2f( iconCentre.x + x, iconCentre.y - iconSize/2 );
            glTexCoord2i( 1, 0 );           glVertex2f( iconCentre.x + x, iconCentre.y + iconSize/2 );
            glTexCoord2i( 0, 0 );           glVertex2f( iconCentre.x - x, iconCentre.y + iconSize/2 );
        glEnd();

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_TEXTURE_2D );


        if( g_inputManager->controlEvent( ControlMenuClose ) )
        {
            MouseUp();
        }
    }
}



void GameMenuTitleButton::Render( int realX, int realY, bool highlighted, bool clicked )
{   
    //
    // Fill  box

    //glColor4f( 0.7f, 1.0f, 0.5f, 0.5f );

    RenderHighlightBeam( realX, realY + m_h/2, m_w, m_h );

    //
    // Caption

    UnicodeString caption( m_caption.m_unicodestring );
	caption.StrUpr();

    float fontSize = m_fontSize * 1.5f;
    float width = g_titleFont.GetTextWidthReal(caption, fontSize);
	if( width > m_w )
	{
		fontSize *= m_w / ( width * 1.05f );
	}


    float texY = realY + m_h/2.0f - fontSize/2.0f + 7.0f;

    g_titleFont.SetRenderOutline(true);
    glColor4f( 0.75f, 1.0f, 0.45f, 1.0f );
    g_titleFont.DrawText2DCentre( realX + m_w*0.5f, texY, fontSize, caption );

    g_titleFont.SetRenderOutline(false);
    g_titleFont.SetRenderShadow(true);
    glColor4f( 1.0f, 0.9f, 0.9f, 0.0f );
    g_titleFont.DrawText2DCentre( realX + m_w*0.5f, texY, fontSize, caption );

    g_titleFont.SetRenderShadow(false);
}



int GetTimeOption()
{
    GameMenuWindow *main = (GameMenuWindow *)EclGetWindow( /*"GameMenu"*/"multiwinia_mainmenu_title" );
    int gameType = main->m_gameType;
    if( main->m_settingUpCustomGame ) gameType = main->m_customGameType;
    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[gameType];
    for( int i = 0; i < blueprint->m_params.Size(); ++i )
    {
        if( stricmp( blueprint->m_params[i]->m_name, "TimeLimit") == 0 )
        {
            return i;
        }
    }

    return -1;
}

// *************************
// Game Menu Class
// *************************

GameMenu::GameMenu()
:   m_menuCreated(false)
{
    memset( m_numDemoMaps, 0, sizeof(int) * MAX_GAME_TYPES );
    CreateMapList();
    m_clouds = new Clouds();
	m_clouds->Advance();
}

GameMenu::~GameMenu()
{
	m_allMaps.EmptyAndDelete();
	delete m_clouds; m_clouds = NULL;
}

void GameMenu::Render()
{
#ifdef _DEBUG
    static float existingFactor = 0.91f;
   /* if( g_inputManager->controlEvent( ControlCameraLeft ) ) 
    {
        existingFactor -= 0.001f;
        g_editorFont.SetHorizSpacingFactor( existingFactor );
    }
    if( g_inputManager->controlEvent( ControlCameraRight ) ) 
    {
        existingFactor += 0.001f;
        g_editorFont.SetHorizSpacingFactor( existingFactor );
    }
    if( g_inputManager->controlEvent( ControlCameraUp ) ) 
    {
        existingFactor = 0.91;
        g_editorFont.SetHorizSpacingFactor( existingFactor );
    }*/
#endif
	g_app->m_camera->Advance();

    g_app->m_camera->m_pos = g_zeroVector;
    g_app->m_camera->SetFOV( 90 );
	
    Vector3 pos(0,600,0);
    Vector3 front(0,0,1);
    Vector3 up(0,1,0);

    // -1.899f;
    // -1.079f;

    //
    // Render clouds
	
    static float s_amount = -1.899;
    static float s_amount2 = -1.079;
    /*if( g_inputManager->controlEvent( ControlCameraLeft ) ) s_amount += 0.01f;
    if( g_inputManager->controlEvent( ControlCameraRight ) ) s_amount -= 0.01f;
    if( g_inputManager->controlEvent( ControlCameraForwards ) ) s_amount2 += 0.01f;
    if( g_inputManager->controlEvent( ControlCameraBackwards ) ) s_amount2 -= 0.01f;*/
	
    front.RotateAroundY( s_amount );
    up.RotateAroundY( s_amount );
    front.RotateAroundZ( s_amount2 );
    up.RotateAroundZ( s_amount2 );
    
    Vector3 focusPos = pos + front * 100;

    gluLookAt( pos.x, pos.y, pos.z, focusPos.x, focusPos.y, focusPos.z, up.x, up.y, up.z );

	
    m_clouds->Render(GetHighResTime() * -1.0f);
}

void GameMenu::CreateMenu()
{
    g_app->m_renderer->StartFadeIn(0.25f);
    // close all currently open windows
    RemoveAllWindows();
    
    // create the actual menu window
    g_app->m_multiwinia->Reset();
    EclRegisterWindow( new GameMenuWindow() );

    // set the camera to a position with a good view of the internet
    g_app->m_camera->RequestMode(Camera::ModeSphereWorldFocus);
    g_app->m_camera->SetTarget(Vector3(-900, 3000, 3970), Vector3(0,0.5f,-1));
    g_app->m_camera->CutToTarget();
    g_app->m_camera->SetDebugMode(0);

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    if( g_app->m_tutorial )
    {
        // its possible that the player has loaded the prologue, then returned to the main menu
        // if so, delete the tutorial
        delete g_app->m_tutorial;
        g_app->m_tutorial = NULL;
    }

    if( g_app->m_demoEndSequence )
    {
        delete g_app->m_demoEndSequence;
        g_app->m_demoEndSequence = NULL;
    }
#endif

    g_app->m_gameMode = App::GameModeNone;
    if( g_app->m_multiwiniaTutorial )
    {
        GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow( "multiwinia_mainmenu_title" );
        if( gmw )
        {
            gmw->m_newPage = GameMenuWindow::PageTutorials;
        }
    }
    g_app->m_multiwiniaTutorial = false;

    m_menuCreated = true;
}

void GameMenu::DestroyMenu()
{
    m_menuCreated = false;
    EclRemoveWindow(/*"GameMenu"*/"multiwinia_mainmenu_title");
	if( !g_app->m_editing )
	{
		g_app->m_renderer->StartFadeOut();
	}
}

bool GameMenu::AddMap( TextReader *_in, char *_filename )
{
    if( _in && _in->IsOpen() )
    {
		//AppDebugOut("Got file %s when making map list\n", file->GetFilename());
        MapData *md = new MapData();
        md->m_fileName = newStr(_filename);
		bool storedMd = false;

        while( _in->ReadLine() )
		{
            if( !_in->TokenAvailable() ) continue;

            char *token = _in->GetNextToken();
            if( strcmp( token, "Name" ) == 0 )
            {
                md->m_levelName = newStr(_in->GetRestOfLine());
            }

            if( strcmp( token, "GameTypes" ) == 0 )
            {
                while( _in->TokenAvailable() )
                {
                    char *type = _in->GetNextToken();
                    for( int j = 0; j < Multiwinia::s_gameBlueprints.Size(); ++j )
                    {
                        if( strcmp( type, Multiwinia::s_gameBlueprints[j]->m_name ) == 0 )
                        {
                            m_maps[j].PutData( md );
							md->m_gameTypes[j] = true;
							storedMd = true;
                        }
                    }
                }
            }

            if( strcmp( token, "NumPlayers" ) == 0  )
            {
                md->m_numPlayers = atoi( _in->GetNextToken() );                    
            }

            if( strcmp( token, "Author" ) == 0 )
            {
                md->m_author = newStr( _in->GetRestOfLine() );
            }

            if( strcmp( token, "Description" ) == 0 ) 
            {
                md->m_description = newStr( _in->GetRestOfLine() );
            }

            if( strcmp( token, "PlayTime" ) == 0 )
            {
                md->m_playTime = atoi( _in->GetNextToken() );
            }

            if( strcmp( token, "Difficulty" ) == 0 )
            {
                md->m_difficulty = newStr( _in->GetNextToken() );
            }

            if( strcmp( token, "CoopMode" ) == 0 )
            {
                md->m_coop = true;
            }

            if( strcmp( token, "ForceCoop" ) == 0 )
            {
                md->m_forceCoop = true;
            }

            if( strcmp( token, "OfficialLevel" ) == 0 )
            {
				md->m_officialMap = true;
            }

            if( strcmp( token, "MultiwiniaOptions_EndDefinition" ) == 0 )
            {
                break;
            }
        }

        // Determine CRC identifier of map
        md->CalculeMapId();
        if( md->m_mapId == -1 ) md->m_customMap = true;

		if( IS_DEMO )
		{
			for( int j = 0; j < MAX_GAME_TYPES; j++ )
			{
				if( md->m_gameTypes[j] && g_app->IsMapAvailableInDemo( j, md->m_mapId ) )
                {
					m_numDemoMaps[j]++;
                }
			}
		}

		if( storedMd )
		{
			m_allMaps.PutData( md );
            return md->m_officialMap;
		}
		else
		{
			AppDebugOut("Warning: Discarded map %s because it didn't contain a game type\n", _filename );
			delete md;
		}
    }	

    return false;
}

void GameMenu::CreateMapList()
{
    LList<char *> *levels = g_app->m_resource->ListResources( "levels/", "mp_*", false );

    int numOfficialLevels = 0;
    for( int i = 0; i < levels->Size(); ++i )
    {
        char filename[512];
        snprintf( filename, sizeof(filename), "levels/%s", levels->GetData(i) );
        filename[ sizeof(filename) - 1 ] = '\0';

        TextReader *file = NULL;

        if( strcmp( GetExtensionPart(filename), "txt" ) == 0 )
        {
            file = g_app->m_resource->GetTextReader( filename );
        }

        if( AddMap( file, levels->GetData(i) ) ) numOfficialLevels++;
        delete file;
    }

	levels->EmptyAndDelete();
    delete levels;
    levels = NULL;

    g_app->m_achievementTracker->SetNumLevels( numOfficialLevels );

    //
    // bubble sort maps into order
    // Num players first, then difficulty

    for( int gameType = 0; gameType < Multiwinia::s_gameBlueprints.Size(); ++gameType )
    {
        for( int n = 0; n < m_maps[gameType].Size(); ++n )
        {
            for( int i = 1; i < m_maps[gameType].Size(); ++i )
            {
                MapData *prevMap = m_maps[gameType].GetData(i-1);
                MapData *thisMap = m_maps[gameType].GetData(i);
                
                
                bool swapEm = false;

                if( thisMap->m_numPlayers < prevMap->m_numPlayers ) 
                {
                    swapEm = true;
                }
                else if( thisMap->m_numPlayers == prevMap->m_numPlayers )
                {
                    int prevDif = 0;
                    int thisDif = 0;

                    if( prevMap->m_difficulty )
                    {
                        if( strstr( prevMap->m_difficulty, "basic" ) ) prevDif = 1;
                        if( strstr( prevMap->m_difficulty, "inter" ) ) prevDif = 2;
                        if( strstr( prevMap->m_difficulty, "advan" ) ) prevDif = 3;
                    }

                    if( thisMap->m_difficulty )
                    {
                        if( strstr( thisMap->m_difficulty, "basic" ) ) thisDif = 1;
                        if( strstr( thisMap->m_difficulty, "inter" ) ) thisDif = 2;
                        if( strstr( thisMap->m_difficulty, "advan" ) ) thisDif = 3;
                    }

                    if( thisDif < prevDif ) swapEm = true;
                }

                if( swapEm )
                {
                    m_maps[gameType].PutData( thisMap, i-1 );
                    m_maps[gameType].PutData( prevMap, i );
                }
            }
        }
    }

#ifdef LOCATION_EDITOR
    // load player-made custom maps from .mwm files
    char dir[512];
    sprintf( dir, "%s", g_app->GetMapDirectory() );
    levels = ListDirectory( dir, "*.mwm", false );
    for( int i = 0; i < levels->Size(); ++i )
    {
        char filename[512];
        sprintf( filename, "%s%s", dir, levels->GetData(i) );
        MapFile *mapFile = new MapFile( filename );
        TextReader *in = mapFile->GetTextReader();
        AddMap( in, levels->GetData(i) );
        delete in;
    }
    levels->EmptyAndDelete();
    delete levels;
    levels = NULL;
#endif
}

int GameMenu::GetNumPlayers()
{
    // this isn't nessecerily the best place for this, but its convinient
    GameMenuWindow *window = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
    if( window )
    {
        if( 0 <= window->m_gameType && window->m_gameType < MAX_GAME_TYPES && 
            m_maps[window->m_gameType].ValidIndex( window->m_requestedMapId ) )
        {
            return m_maps[window->m_gameType][window->m_requestedMapId]->m_numPlayers;
        }
    }
    return -1;
}

MapData *GameMenu::GetMap( int _gameMode, const char *_fileName )
{
	int index = GetMapIndex( _gameMode, _fileName );
	if( index < 0 || !m_maps[_gameMode].ValidIndex( index ) )
		return NULL;
	
	return m_maps[_gameMode].GetData( index );
}

MapData *GameMenu::GetMap( int _gameMode, int _mapCrc )
{
	int index = GetMapIndex( _gameMode, _mapCrc );
	if( index < 0 || !m_allMaps.ValidIndex( index ) )
		return NULL;
	
	return m_allMaps.GetData( index );
}



int GameMenu::GetMapIndex( int gameMode, int mapCrc )
{
    if( gameMode < 0 || gameMode >= MAX_GAME_TYPES )
        return -1;

    for( int i = 0; i < m_maps[gameMode].Size(); ++i )
    {
        if( m_maps[gameMode].ValidIndex(i) )
        {
            MapData *data = m_maps[gameMode].GetData(i);
			
            if( data->m_mapId == mapCrc )
            {
                return i;
            }
        }
    }

    return -1;
}

int GameMenu::GetMapIndex( int _gameMode, const char *_fileName )
{
    if( _gameMode < 0 || _gameMode >= MAX_GAME_TYPES || _fileName  == NULL )
        return -1;

    for( int i = 0; i < m_maps[_gameMode].Size(); ++i )
    {
        if( m_maps[_gameMode].ValidIndex(i) )
        {
            MapData *data = m_maps[_gameMode].GetData(i);
			// AppDebugOut("Comparing %s with data->m_fileName = %s\n", _fileName, data->m_fileName );
			if( stricmp( data->m_fileName, _fileName ) == 0 )
			{
				return i;
			}
        }
    }

    return -1;
}

const char *GameMenu::GetMapName( int gameMode, int mapCrc )
{
    int mapIndex = GetMapIndex( gameMode, mapCrc );
    if( 0 <= gameMode && gameMode < MAX_GAME_TYPES && m_maps[gameMode].ValidIndex(mapIndex) )
    {
        MapData *data = m_maps[gameMode].GetData(mapIndex);
        return  GetMapNameId(data->m_fileName);
    }

    return NULL;
}

const char *GameMenu::GetMapName( int _gameMode, const char *_fileName )
{
    int mapIndex = GetMapIndex( _gameMode, _fileName );
    if( 0 <= _gameMode && _gameMode < MAX_GAME_TYPES && m_maps[_gameMode].ValidIndex(mapIndex) )
    {
        MapData *data = m_maps[_gameMode].GetData(mapIndex);
        return GetMapNameId(data->m_fileName);
    }

    return NULL;
}






void GetPositions( float &left, float &right, float &top, float &bottom, 
                   float &horizSplit, float &vertSplit, float &gap )
{
    float screenW = g_app->m_renderer->ScreenW();
    float screenH = g_app->m_renderer->ScreenH();

    horizSplit = screenH * 0.26f;
    vertSplit = screenW * 0.6f;    
    gap = vertSplit * 0.05f;
    
    left = gap;
    top = gap;
    right = screenW - gap;
    bottom = screenH - gap;

    // Widescreen - move edges in

    float screenRatio = screenW/screenH;
    if( screenRatio > 1.4f )
    {
        left += screenW * 0.08f;
        right -= screenW * 0.08f;
    }
}


void GetPosition_TitleBar( float &x, float &y, float &w, float &h )
{
    float left, right, top, bottom, horizSplit, vertSplit, gap;
    GetPositions( left, right, top, bottom, horizSplit, vertSplit, gap );

    x = left;
    y = top;
    w = right - x;
    h = horizSplit - gap * 0.5f - top;
}


void GetPosition_LeftBox( float &x, float &y, float &w, float &h )
{
    float left, right, top, bottom, horizSplit, vertSplit, gap;
    GetPositions( left, right, top, bottom, horizSplit, vertSplit, gap );

    x = left;
    y = horizSplit + gap * 0.5f;
    w = vertSplit - gap * 0.5f - x;
    h = bottom - y;
}


void GetPosition_RightBox( float &x, float &y, float &w, float &h )
{
    float left, right, top, bottom, horizSplit, vertSplit, gap;
    GetPositions( left, right, top, bottom, horizSplit, vertSplit, gap );

    x = vertSplit + gap * 0.5f;
    y = horizSplit + gap * 0.5f;
    w = right - x;
    h = bottom - y;
}



void GetFontSizes( float &large, float &med, float &_small )
{
    float screenW = g_app->m_renderer->ScreenW();
    float screenH = g_app->m_renderer->ScreenH();

    large = screenH * 0.095f;
    med = screenH * 0.1f * 0.4f;
    _small = screenH * 0.1f * 0.22f;
}


void GetButtonSizes( float &width, float &height, float &gap )
{
    float fontLarge, fontMed, fontSmall;
    GetFontSizes( fontLarge, fontMed, fontSmall );

    float leftX, leftY, leftW, leftH;
    GetPosition_LeftBox( leftX, leftY, leftW, leftH );

    width = leftW * 0.9f;
    height = fontMed * 1.3f;
    gap = fontMed * 0.1f;
}

void DrawFillBox( float x, float y, float w, float h )
{
    //
    // Shadows around the edges

    float shadowSize = g_app->m_renderer->ScreenW() * 0.01f;
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glEnable( GL_BLEND );

    RGBAColour colourOn ( 255,255,255,0 );
    RGBAColour colourOff( 0,0,0,0 );

    glBegin( GL_QUADS );
        glColor4ubv( colourOn.GetData() );      glVertex2f( x, y );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x - shadowSize, y - shadowSize );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x + w + shadowSize, y - shadowSize );
        glColor4ubv( colourOn.GetData() );      glVertex2f( x + w, y );

        glColor4ubv( colourOn.GetData() );      glVertex2f( x + w, y );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x + w + shadowSize, y - shadowSize );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x + w + shadowSize, y + h + shadowSize );
        glColor4ubv( colourOn.GetData() );      glVertex2f( x + w, y + h );

        glColor4ubv( colourOn.GetData() );      glVertex2f( x + w, y + h );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x + w + shadowSize, y + h + shadowSize );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x - shadowSize, y + h + shadowSize );
        glColor4ubv( colourOn.GetData() );      glVertex2f( x, y + h );

        glColor4ubv( colourOn.GetData() );      glVertex2f( x, y + h );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x - shadowSize, y + h + shadowSize );
        glColor4ubv( colourOff.GetData() );     glVertex2f( x - shadowSize, y - shadowSize );    
        glColor4ubv( colourOn.GetData() );      glVertex2f( x, y );
    glEnd();


    //
    // Main box bg texture

    glColor4f( 0.8f, 0.8f, 1.0f, 0.9f );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/interface_blur.bmp" ) ); 
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    
    glBegin( GL_QUADS);
        glTexCoord2f(0,0);  glVertex2f( x, y );
        glTexCoord2f(w/64.0f,0);  glVertex2f( x + w, y );
        glTexCoord2f(w/64.0f,1);  glVertex2f( x + w, y + h );
        glTexCoord2f(0,1);  glVertex2f( x, y + h );
    glEnd();

    //
    // Main box cloudy glow effect

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/cloudyglow.bmp" ) ); 
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    
    float texX = 0;
    float texY = 0;
    float texW = w/(float)g_app->m_renderer->ScreenW();
    float texH = h/(float)g_app->m_renderer->ScreenH();

    texX -= GetHighResTime() * 0.01f;
    texY -= GetHighResTime() * 0.02f;
    texW += sinf(GetHighResTime()) * 0.01f;
    texH += sinf(GetHighResTime()) * 0.02f;

    glColor4f( 0.3f, 0.6f, 0.9f, 0.4f );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE ); 

    glBegin( GL_QUADS);
        glTexCoord2f(texX,texY);            glVertex2f( x, y );
        glTexCoord2f(texX+texW,texY);       glVertex2f( x + w, y );
        glTexCoord2f(texX+texW,texY+texH);  glVertex2f( x + w, y + h );
        glTexCoord2f(texX,texY+texH);       glVertex2f( x, y + h );
    glEnd();

    glDisable( GL_TEXTURE_2D );

    //
    // Line loops 

    glColor4f( 0.8f, 0.7f, 1.0f, 0.8f );    
    glBlendFunc( GL_SRC_ALPHA, GL_ONE ); 
    glLineWidth( 2.0f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();

    glColor4f( 0.8f, 0.7f, 1.0f, 0.4f );

    float inset = 3;
    glLineWidth( 1.0f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( x+inset, y+inset );
        glVertex2f( x + w-inset, y+inset );
        glVertex2f( x + w-inset, y + h-inset );
        glVertex2f( x+inset, y + h-inset );
    glEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );   
}



// *************************
// GameMenu Input classes
// *************************

#define PIXELS_PER_CHAR	25

// *************************
// GM InputField
// *************************

GameMenuInputField::GameMenuInputField()
:   InputField(),
    m_noBackground(false),
    m_renderStyle2(false)
{
}

void GameMenuInputField::Render( int realX, int realY, bool highlighted, bool clicked )
{
    if( !&m_caption ) return;
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

    if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		highlighted = true;

        if( m_type != InputField::TypeString &&
			m_type != InputField::TypeUnicodeString 
			)
        {
            char name[64];
            if( g_inputManager->controlEvent( ControlMenuLeft ) )
            {
                sprintf( name, "%s left", m_name );
				GameMenuInputScroller *scroller = (GameMenuInputScroller *)m_parent->GetButton(name);
				if( scroller) 
				{
					int change = scroller->m_change;

					NetworkInt *v = (NetworkInt *) m_value;
					v->Set( v->Get() + change );
				}
            }

            if( g_inputManager->controlEvent( ControlMenuRight ) )
            {
                sprintf( name, "%s right", m_name );
				GameMenuInputScroller *scroller = (GameMenuInputScroller *)m_parent->GetButton(name);
				if( scroller) 
				{				
					int change = scroller->m_change;

					NetworkInt *v = (NetworkInt *) m_value;
					v->Set( v->Get() + change );
				}
            }
        }
	}

    float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;
    if( m_renderStyle2 )
    {

        if( m_type == InputField::TypeString ||
		    m_type == InputField::TypeUnicodeString )
	    {
            glColor4ub( 10,10,10,255 );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glBegin( GL_QUADS);
                glVertex2f( realX, realY );
                glVertex2f( realX + m_w, realY );
                glVertex2f( realX + m_w, realY + m_h );
                glVertex2f( realX, realY + m_h );
            glEnd();

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            glBegin( GL_LINE_LOOP);
                glVertex2f( realX, realY );
                glVertex2f( realX + m_w, realY );
                glVertex2f( realX + m_w, realY + m_h );
                glVertex2f( realX, realY + m_h );
            glEnd();

            if (m_parent->m_currentTextEdit && (strcmp(m_parent->m_currentTextEdit, m_name) == 0) )
	        {
		        if (fmod(GetHighResTime(), 1.0) < 0.5)
		        {
                    int cursorOffset = g_titleFont.GetTextWidthReal( m_buf, m_fontSize ) / 2.0f;
			        glLineWidth(3.0f);
			        glBegin( GL_LINES );
				        glVertex2f( (realX + m_w/2.0f) + cursorOffset, realY + 2 );
				        glVertex2f( (realX + m_w/2.0f) + cursorOffset, realY + m_h - 2 );
			        glEnd();
		        }
	        }

		    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		    g_titleFont.SetRenderOutline(true);
		    g_titleFont.DrawText2DCentre( realX + m_w / 2.0f, texY, m_fontSize, m_buf);
		    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		    g_titleFont.SetRenderOutline(false);
		    g_titleFont.DrawText2DCentre( realX + m_w / 2.0f, texY, m_fontSize, m_buf);
	    }
    }
    else
    {
	    if( highlighted )
        {
            glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
        }

        if( highlighted || !m_noBackground )
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glBegin( GL_QUADS);
                glVertex2f( realX, realY );
                glVertex2f( realX + m_w, realY );
                glVertex2f( realX + m_w, realY + m_h );
                glVertex2f( realX, realY + m_h );
            glEnd();
        }

        if( highlighted  )
        {
            glColor4f( 1.0, 0.3f, 0.3, 1.0f );

        }

        //float texY = realY + m_h/2 - m_fontSize/4;

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );
        g_titleFont.SetRenderOutline(true);

        glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
        if( highlighted  )
        {
            glColor4f( 1.0, 0.3f, 0.3, 1.0f );
        }
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );

        int fieldX = realX + m_w;

        if (m_parent->m_currentTextEdit && (strcmp(m_parent->m_currentTextEdit, m_name) == 0) )
	    {
		    if (fmod(GetHighResTime(), 1.0) < 0.5)
		    {
			    int cursorOffset = m_fontSize * 0.95f;//strlen(m_buf) * m_fontSize+2
			    glLineWidth(5.0f);
			    glBegin( GL_LINES );
				    glVertex2f( fieldX - cursorOffset, realY + 2 );
				    glVertex2f( fieldX - cursorOffset, realY + m_h - 2 );
			    glEnd();
		    }
	    }

        if ( strcmp(m_parent->m_currentTextEdit, m_name ) != 0 &&
             fmod(GetHighResTime(), 1.0) < 0.5)
	    {
		    if( m_value )
			    m_value->ToUnicodeString( m_buf );

		    m_inputBoxWidth = m_buf.Length() * PIXELS_PER_CHAR + 7;
        }

	    if( m_type == InputField::TypeString ||
		    m_type == InputField::TypeUnicodeString )
	    {
		    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		    g_titleFont.SetRenderOutline(true);
		    g_titleFont.DrawText2DRight( realX + m_w - m_fontSize, texY, m_fontSize, m_buf);
		    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		    g_titleFont.SetRenderOutline(false);
		    g_titleFont.DrawText2DRight( realX + m_w - m_fontSize, texY, m_fontSize, m_buf);
	    }
	    else
	    {
		    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
		    g_titleFont.SetRenderOutline(true);
		    g_titleFont.DrawText2DRight( realX + m_w - m_fontSize * 2, texY, m_fontSize * 0.8f, m_buf);
		    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		    g_titleFont.SetRenderOutline(false);
		    g_titleFont.DrawText2DRight( realX + m_w - m_fontSize * 2, texY, m_fontSize * 0.8f, m_buf);
	    }
    }

    if( highlighted && GameMenuButton::s_previousHighlightButton != this && strcmp( m_parent->m_currentTextEdit, "none" ) == 0 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
        GameMenuButton::s_previousHighlightButton = this;
    }
}

void GameMenuInputField::MouseUp()
{
    if( m_type != InputField::TypeString &&
		m_type != InputField::TypeUnicodeString ) return;
    InputField::MouseUp();

}

void GameMenuInputField::Keypress( int keyCode, bool shift, bool ctrl, bool alt, unsigned char ascii )
{
    if( keyCode == KEY_ENTER )
    {
        ((DarwiniaWindow *)m_parent)->m_skipUpdate = true;
    }

    InputField::Keypress( keyCode, shift, ctrl, alt, ascii );
}

// *************************
// GM InputScroller
// *************************

GameMenuInputScroller::GameMenuInputScroller()
:   InputScroller()
{
}

#define INTEGER_INCREMENT_PERIOD 0.1f

void GameMenuInputScroller::Render(int realX, int realY, bool highlighted, bool clicked)
{
    if( !&m_caption ) return;
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2D( realX, realY, m_fontSize, m_caption );

    //glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );

    if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		highlighted = true;
	}

    glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );

    if( highlighted  )
    {
        glColor4f( 1.0, 0.3f, 0.3, 1.0f );
    }
    
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2D( realX, realY, m_fontSize, m_caption );

	CheckInput( realX, realY );
}

// *************************
// GM DropDownWindow
// *************************

GameMenuDropDownWindow::GameMenuDropDownWindow( char *_name, char *_parentName )
:   DropDownWindow( _name, _parentName )
{
    SetMovable(false);
    m_resizable = false;
}

GameMenuDropDownWindow::~GameMenuDropDownWindow()
{
    if( s_window )
    {
        DropDownWindow *removeMe = s_window;
        s_window = NULL;
        EclRemoveWindow( removeMe->m_name );        
    }
}

void GameMenuDropDownWindow::Create()
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

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(m_name) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

}

DropDownWindow *GameMenuDropDownWindow::CreateGMDropDownWindow( char *_name, char *_parentName )
{
    if( s_window )
    {
        DropDownWindow *removeMe = s_window;
        s_window = NULL;
        EclRemoveWindow( removeMe->m_name );        
    }

    s_window = new GameMenuDropDownWindow( _name, _parentName );
    return s_window;
}

void GameMenuDropDownWindow::Render( bool hasFocus )
{
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

    GetPosition_LeftBox( leftX, leftY, leftW, leftH );
    

    //
    // Render options
    // Build the help string as we go

    DrawFillBox( leftX, leftY, leftW, leftH );


    EclWindow::Render( hasFocus );
}

// *************************
// GM DropDownMenu
// *************************

GameMenuDropDown::GameMenuDropDown( bool _sortItems )
:   DropDownMenu( _sortItems ),
	m_previousValue( -1 ),
    m_noBackground(false),
    m_cycleMode(false)
{
}

void GameMenuDropDown::MouseUp()
{
    if( m_inactive ) return;

    if( m_cycleMode )
    {
        int index = m_currentOption + 1;
        if( index >= m_options.Size() ) index = 0;
        SelectOption( m_options[index]->m_value );
    }
    else
    {
	    g_app->m_renderer->InitialiseMenuTransition(1.0f, 1);
	    g_app->m_doMenuTransition = true;
	    DropDownMenu::MouseUp();
    }
}

void GameMenuDropDown::CreateMenu()
{
    GameMenuDropDownWindow *window = (GameMenuDropDownWindow *)GameMenuDropDownWindow::CreateGMDropDownWindow(m_name, m_parent->m_name);
    window->SetPosition( 0,0 );//m_parent->m_x + m_x + m_w, m_parent->m_y + m_y );
    EclRegisterWindow( window );

    float leftX, leftY, leftW, leftH;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetButtonSizes( buttonW, buttonH, gap );
	leftY+= buttonH * 1.8f;

    int screenH = g_app->m_renderer->ScreenH();
    int screenW = g_app->m_renderer->ScreenW();

    float availableHeight = leftH - buttonH * 1.5f;
    availableHeight -= buttonH * 1.8f;

    int numColumnsRequired = 1 + (( buttonH * m_options.Size() ) / availableHeight);
    int numPerColumn =  ceil( (float) m_options.Size() / (float) numColumnsRequired );

    int index = 0;

    for( int col = 0; col < numColumnsRequired; ++col )
    {
        for( int i = 0; i < numPerColumn; ++i )
        {
            if( index >= m_options.Size() ) break;

            char *thisOption = m_options[index]->m_word;            
            char thisName[64];
            sprintf( thisName, "%s %d", m_name, index );

            int w = buttonW / (numColumnsRequired);
            int x = leftX + ( col * w ) + ((leftW-(w*numColumnsRequired)) / 2.0f);
			w *= 0.95f;

            GameMenuDropDownMenuOption *menuOption = new GameMenuDropDownMenuOption();
            menuOption->SetProperties( thisName, x, leftY + (i+1)*m_h*1.05f, w, m_h, LANGUAGEPHRASE(thisOption) );
            menuOption->SetParentMenu( m_parent, this, m_options[index]->m_value );
            menuOption->m_fontSize = m_fontSize;
            if( index == m_currentOption ) menuOption->m_currentOption = true;
            window->RegisterButton( menuOption );
			window->m_buttonOrder.PutData( menuOption );
			if( GetSelectionValue() == m_options[index]->m_value )
			{
				window->m_currentButton = index;
			}

            ++index;
        }
    }

    window->SetSize( screenW, screenH );//m_w*numColumnsRequired, (numPerColumn+1) * m_h );    
    //window->SetPosition( m_x+m_w/2 + 3, m_y - 45 );
}

void GameMenuDropDown::Render(int realX, int realY, bool highlighted, bool clicked)
{
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;
/*
	if( m_previousValue != *m_int )
	{
		SelectOption( *m_int );
	}
*/
    glColor4f( 0.3f, 1.0f, 0.3f, 1.0f );

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

    if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		highlighted = true;
	}
    
    if( highlighted )
    {
        glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
    }

    if( highlighted || !m_noBackground )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glBegin( GL_QUADS);
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
    }

    float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;//m_h/2 - m_fontSize/4;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, LANGUAGEPHRASE(m_name) );

    
    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);

    glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );

    if( highlighted  )
    {
        glColor4f( 1.0, 0.3f, 0.3, 1.0f );
    }

    if( m_inactive ) glColor4f( 0.5f, 0.5f, 0.5f, 1.0f);

    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, LANGUAGEPHRASE(m_name) );

    if( !EclGetWindow(m_name) )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2DRight( (realX+m_w) - +m_fontSize, texY, m_fontSize, m_caption );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( m_inactive ) glColor4f( 0.5f, 0.5f, 0.5f, 1.0f);
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DRight( (realX+m_w) - +m_fontSize, texY, m_fontSize, m_caption );
    }

	if( highlighted && GameMenuButton::s_previousHighlightButton != this )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
        GameMenuButton::s_previousHighlightButton = this;
    }
}

// *************************
// GM DropDownMenuOption
// *************************

GameMenuDropDownMenuOption::GameMenuDropDownMenuOption()
:   DropDownMenuOption(),
    m_currentOption(false)
{
}

void GameMenuDropDownMenuOption::MouseUp()
{
	g_app->m_renderer->InitialiseMenuTransition(1.0f, -1);
	g_app->m_doMenuTransition = true;
	DropDownMenuOption::MouseUp();
}

void GameMenuDropDownMenuOption::Render(int realX, int realY, bool highlighted, bool clicked)
{
    GameMenuDropDownWindow *parent = (GameMenuDropDownWindow *)m_parent;

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

    if( parent->m_buttonOrder[parent->m_currentButton] == this )
	{
		highlighted = true;
	}

    if( m_currentOption ) clicked = true;

	if( highlighted || clicked )
    {
        glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
    }

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glBegin( GL_QUADS);
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
        glVertex2f( realX, realY + m_h );
    glEnd();    

	float textY = realY + m_h/2;// - m_fontSize/4;
	float textX = realX + realX * 0.05f;

	glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2D( textX, textY, m_fontSize, m_caption );

	if( highlighted )
    {
        glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
    }
	else
	{
		glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	}
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2D( textX, textY, m_fontSize, m_caption );
}

void RenderHighlightBeam( float x, float centreY, float w, float h )
{

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/menuhighlight.bmp" ) ); 
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    float screenW = g_app->m_renderer->ScreenW();
    //float h = screenW * 0.05f;

    glBegin( GL_QUADS);
    {
        glColor4f( 0.6f, 1.0f, 0.4f, 0.8f );
        glTexCoord2f(0,0);              glVertex2f( x,     centreY-h/2.0f );
        glTexCoord2f(1,0);              glVertex2f( x + w, centreY-h/2.0f );

        glColor4f( 0.3f, 0.5f, 0.2f, 0.7f );
        glTexCoord2f(1,1);              glVertex2f( x + w, centreY+h/2.0f );
        glTexCoord2f(0,1);              glVertex2f( x,     centreY+h/2.0f );
    }
    glEnd();    

    //
    // Render thin black line

    glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );

    glBegin( GL_LINE_LOOP );
    {
        glVertex2f( x,     centreY-h/2.0f );
        glVertex2f( x + w, centreY-h/2.0f );
        glVertex2f( x + w, centreY+h/2.0f );
        glVertex2f( x,     centreY+h/2.0f );
    }
    glEnd();  
}

void GameMenuCheckBox::Render(int realX, int realY, bool highlighted, bool clicked)
{
    DarwiniaWindow *parent = (DarwiniaWindow *)m_parent;

    UpdateButtonHighlight();

    if( !m_mouseHighlightMode )
	{
		highlighted = false;
	}

    if( parent->m_buttonOrder[ parent->m_currentButton ] == this ) highlighted = true;

	if( highlighted )
    {
        glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
    }

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glBegin( GL_QUADS);
        glVertex2f( realX, realY );
        glVertex2f( realX + m_w, realY );
        glVertex2f( realX + m_w, realY + m_h );
        glVertex2f( realX, realY + m_h );
    glEnd();

    if( highlighted  )
    {
        glColor4f( 1.0, 0.3f, 0.3, 1.0f );

    }

    //float texY = realY + m_h/2 - m_fontSize/4;
    float texY = realY + m_h/2 - m_fontSize/2 + 7.0f;

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );
    g_titleFont.SetRenderOutline(true);

    glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
    if( highlighted  )
    {
        glColor4f( 1.0, 0.3f, 0.3, 1.0f );
    }
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );

    
    glColor4f( 0.3f, 0.3f, 0.3f, 1.0f );
    
    float w = m_h * 0.75f;
    float h = m_h * 0.75f;
    float x = (realX + m_w) - h * 2.0f;
    float y = realY + ((m_h - h) / 2.0f);
    
    glBegin( GL_QUADS );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();

    if( IsChecked() )
    {
        x+= m_h * 0.05f;
        y+= m_h * 0.05f;
        w = h = m_h * 0.65f;   

        //glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/tick.bmp" ) );

        glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
        glBegin( GL_QUADS );
            glTexCoord2i(0,1); glVertex2f( x, y );
            glTexCoord2i(1,1); glVertex2f( x + w, y );
            glTexCoord2i(1,0); glVertex2f( x + w, y + h );
            glTexCoord2i(0,0); glVertex2f( x, y + h );
        glEnd();

        glDisable( GL_TEXTURE_2D );
    }

    w = h = m_h * 0.75f;
    x = (realX + m_w) - h * 2.0f;
    y = realY + ((m_h - h) / 2.0f);

    glLineWidth(1.0f);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glBegin( GL_LINE_LOOP );
        glVertex2i( x, y );
        glVertex2i( x+w, y );
        glVertex2i( x+w, y+h );
        glVertex2i( x, y+h );
    glEnd();

    if( highlighted && GameMenuButton::s_previousHighlightButton != this )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
        GameMenuButton::s_previousHighlightButton = this;
    }
}
