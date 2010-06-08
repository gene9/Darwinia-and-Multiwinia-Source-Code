#include "lib/universal_include.h"

#include "network/network_defines.h"
#include "app.h"
#include "renderer.h"
#include "lib/metaserver/authentication.h"
#include "lib/preferences.h"
#include "lib/resource.h"
#include "network/clienttoserver.h"
#include "network/servertoclient.h"
#include "network/server.h"
#include "sound/soundsystem.h"
#include "lib/metaserver/metaserver_defines.h"
#include "lib/unicode/unicode_text_stream_reader.h"
#include "multiwiniahelp.h"

#include "MapData.h"

#include "../game_menu.h"
#include "location.h"

#include "interface/helpandoptions_windows.h"
#include "interface/chatinput_window.h"

#include "mapcrc.h"

#include "GameMenuWindow.h"
#include "GameServerButton.h"
#include "DarwiniaModeButton.h"
#include "NewOrJoinButton.h"
#include "TutorialButton.h"
#include "BuyNowModeButton.h"
#include "AuthPageButton.h"
#include "MultiwiniaEditorButton.h"
#include "QuitButton.h"
#include "PrologueButton.h"
#include "CampaignButton.h"
#include "WaitingButton.h"
#include "ServerTitleInfoButton.h"
#include "FiltersPageButton.h"

#include "QuickMatchButton.h"
#include "NewServerButton.h"
#include "JoinGameButton.h"
#include "GameTypeInfoButton.h"
#include "GameTypeImageButton.h"
#include "GameTypeButton.h"
#include "FinishMultiwiniaButton.h"
#include "MapDetailsButton.h"
#include "MapImageButton.h"
#include "MapSelectModeButton.h"
#include "MapInfoButton.h"
#include "PlayGameButton.h"
#include "AdvancedOptionsModeButton.h"
#include "GamerTagButton.h"
#include "MapListTitlesButton.h"
#include "AnyLevelSelectButton.h"
#include "LevelSelectButton.h"
#include "HelpSelectButton.h"
#include "HelpImageButton.h"
#include "PrevHelpButton.h"
#include "NextHelpButton.h"
#include "OptionDetailsButton.h"
#include "MultiwiniaVersusCpuButton.h"
#include "ImageButton.h"
#include "BuyNowInfoButton.h"
#include "UnlockFullGameButton.h"
#include "AuthInputField.h"
#include "PasteKeyButton.h"
#include "EnterAuthKeyButton.h"
#include "FancyColourButton.h"
#include "ApplyNameButton.h"
#include "DarwiniaPlusSPButton.h"
#include "DarwiniaPlusMPButton.h"
#include "GameTypeAnyButton.h"
#include "QuitAuthKeyButton.h"
#include "credits_button.h"
#include "PlayDemoAuthkeyButton.h"
#include "AuthStatusButton.h"
#include "TestBedButtons.h"
#include "AchievementButton.h"
#include "KickPlayerButton.h"
#include "ConnectButton.h"

#ifdef TESTBED_ENABLED
#include <time.h>
#endif


#include "CSpectatorButton.h"

#include "motdButton.h"
#include "update_page_buttons.h"


#include "interface/mainmenus.h"
#ifdef TARGET_MSVC
#include "lib/input/inputdriver_xinput.h"
#endif

#include "lib/metaserver/metaserver_defines.h"
#include "lib/metaserver/metaserver.h"
#include <set>

#ifdef TARGET_OS_MACOSX
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

//#define	 FAKE_SERVERS
#define NUM_FAKE_SERVER 100

//extern void RemoveDelistedServers( LList<Directory *> *_serverList );
//extern const char *GameTypeToShortCaption( int _gameType );

extern const char *s_pageNames[GameMenuWindow::NumPages];
extern int g_lastProcessedSequenceId;
#ifdef TARGET_MSVC
extern XInputDriver *g_xInputDriver;
#endif

const char* GetColourNameChar(int colourId)
{
	static char *names[8] = {
		"multiwinia_colour_green",
		"multiwinia_colour_red",
		"multiwinia_colour_yellow",
		"multiwinia_colour_blue",
		"multiwinia_colour_orange",
		"multiwinia_colour_cyan",
		"multiwinia_colour_purple",
		"multiwinia_colour_pink" };

	return names[colourId];
}
UnicodeString GetColourName(int colourId)
{
	switch( colourId )
	{
		case 0:	return LANGUAGEPHRASE("multiwinia_colour_green");
		case 1:	return LANGUAGEPHRASE("multiwinia_colour_red");
		case 2:	return LANGUAGEPHRASE("multiwinia_colour_yellow");
		case 3:	return LANGUAGEPHRASE("multiwinia_colour_blue");
		case 4:	return LANGUAGEPHRASE("multiwinia_colour_orange");
		case 6:	return LANGUAGEPHRASE("multiwinia_colour_cyan");
		case 5:	return LANGUAGEPHRASE("multiwinia_colour_purple");
		case 7:	return LANGUAGEPHRASE("multiwinia_colour_pink");
	}

	return UnicodeString();
}


// *************************
// GameMenuWindow Class
// *************************


class GameMenuChatButton: public DarwiniaButton
{
public:
    GameMenuChatButton()
    :   DarwiniaButton()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

        static float boxHeight = 0.0f;
        float fontSize = m_fontSize;

        float x = realX;
        float y = realY + m_h;

        //g_editorFont.BeginText2D();

        /*if( !ChatInputWindow::IsChatVisible() )
        {
            bool renderMessage = true;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeRemotePlayer )
                {
                    renderMessage = true;
                }
            }

            if( renderMessage )
            {
                UnicodeString pressEnter = LANGUAGEPHRASE("multiwinia_lobbypresstotalk");
                glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
                g_editorFont.SetRenderOutline( true );
                g_editorFont.DrawText2D( x + fontSize, y - fontSize * 0.8f, fontSize, pressEnter );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_editorFont.SetRenderOutline( false );
                g_editorFont.DrawText2D( x + fontSize, y - fontSize * 0.8f, fontSize, pressEnter );
            }
        }*/

        glColor4f( 0.0f, 0.0f, 0.0f, 1.0f * (200.0f/255.0f) );
        glEnable    ( GL_BLEND );
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        int h = m_h - fontSize * 1.5f;

        glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + h );
            glVertex2f( realX, realY + h );
        glEnd();
        
        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );

        glBegin( GL_LINE_LOOP );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + h );
            glVertex2f( realX, realY + h );
        glEnd();

        if( parent->m_chatMessages.Size() == 0 ) 
        {
            //g_editorFont.EndText2D();
            return;
        }

        y -=  (fontSize * 2.0f);

        int w = m_w * 0.95f;

        x+=fontSize;

        boxHeight = fontSize;
        bool stopRendering = false;
        y -= (fontSize * 1.1f);
        for( int i = 0; i < parent->m_chatMessages.Size(); ++i )
        {
            float alphaTime = GetHighResTime() - parent->m_chatMessages[i]->m_recievedTime;
            bool maxed = false;
            if( !stopRendering )
            {
                float alpha = 255.0f;

                int teamId = -1;
                for( int j = 0; j < NUM_TEAMS; ++j )
                {
                    if( g_app->m_multiwinia->m_teams[j].m_clientId == parent->m_chatMessages[i]->m_clientId && 
                        !g_app->m_multiwinia->m_teams[j].m_disconnected )
                    {
                        teamId = j;
                        break;
                    }
                }

                UnicodeString msg;
                UnicodeString teamName;
                if( teamId == -1 )
                {
                    teamName = parent->m_chatMessages[i]->m_senderName;
                }
                else
                {
                    teamName = g_app->m_multiwinia->m_teams[ teamId ].m_teamName;
					parent->m_chatMessages[i]->m_colour = g_app->m_multiwinia->m_teams[ teamId] .m_colour;
                    parent->m_chatMessages[i]->m_senderName = teamName;
                }
				if( teamName.Length() == 0 )
				{
					teamName = UnicodeString("Player");
					parent->m_chatMessages[i]->m_colour = RGBAColour( 150, 150, 150, 255 );
				}
                UnicodeString thisMsg = parent->m_chatMessages[i]->m_msg;
                if( thisMsg == UnicodeString("PLAYERJOIN") )
                {
                    msg = LANGUAGEPHRASE("multiwinia_player_join");
                    msg.ReplaceStringFlag( L'P', teamName );
                }
				else if( thisMsg == UnicodeString("PLAYERLEAVE") )
				{
					msg = LANGUAGEPHRASE("multiwinia_player_leave");
					msg.ReplaceStringFlag( L'P', teamName );
				}
                else
                {
                    msg = teamName + UnicodeString(": ") + thisMsg;
                }

                LList<UnicodeString *> *wrapped = WordWrapText( msg, m_w*1.75f, fontSize, true, true );

                if( y < m_y + m_h * 0.05f )
                {
                    maxed = true;
                }

                if( !maxed )
                {
                    float a = alpha / 255.0f;
                    for( int j = wrapped->Size() -1; j >= 0; --j )
                    {
                        if( a == 1.0f )
                        {
                            g_editorFont.SetRenderOutline(true);
                            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
                            g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                        }
                        RGBAColour col;
                        if( teamId == -1 )
                        {
                            col = parent->m_chatMessages[i]->m_colour;
                        }
                        else
                        {
                            col = g_app->m_multiwinia->m_teams[teamId].m_colour;
                            parent->m_chatMessages[i]->m_colour = col;
                        }
                        col.a = alpha;
                        glColor4ubv( col.GetData() );
                        g_editorFont.SetRenderOutline(false);
        //                g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                        g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                        y -= fontSize * 1.1f;

                        if( y < m_y + m_h * 0.05f )
                        {
                            maxed = true;
                            break;
                        }
                    }
                    //y -= (fontSize * 1.1f) * wrapped->Size();
                    boxHeight += (fontSize * 1.2f) * wrapped->Size();
                }
                wrapped->EmptyAndDelete();
                delete wrapped;
            }

            /*if( alphaTime > 60.0f )
            {
                ChatMessage *m = parent->m_chatMessages[i];
                parent->m_chatMessages.RemoveData(i);
                delete m;
                --i;
            }*/

            if( maxed )
            {
                // stop if we get too far up teh screen
                // we could break here, but then none of the older messages would ever get deleted
                stopRendering = true;
            }
        }
    }
};


GameMenuWindow::GameMenuWindow()
:   DarwiniaWindow("multiwinia_mainmenu_title"),
    m_currentPage(-1),
    m_newPage(PageMain),
	m_serverList( NULL ),
    m_highlightedLevel(-1),
    m_helpPage(-1),
    m_currentHelpImage(-1),
    m_currentRequestedColour(-1),
	m_xblaSessionType(-1),
	m_xblaSelectSessionTypeNextPage(-1),
	m_xblaOfferSplitscreen(false),
	m_gameType( GAMEOPTION_GAME_TYPE, g_app->m_multiwinia->m_gameType ),
	m_aiMode( GAMEOPTION_AIMODE, g_app->m_multiwinia->m_aiType ),
	m_coopMode( GAMEOPTION_COOPMODE, g_app->m_multiwinia->m_coopMode ),
    m_basicCrates( GAMEOPTION_BASICCRATES, g_app->m_multiwinia->m_basicCratesOnly ),
    m_singleAssaultRound( GAMEOPTION_SINGLEASSAULT, g_app->m_multiwinia->m_assaultSingleRound ),
	m_params( MULTIWINIA_MAXPARAMS, NetworkGameParamFactory() ),
	m_researchLevel( GlobalResearch::NumResearchItems, NetworkGameResearchLevelFactory() ),
    m_quickStart(false),
    m_highlightedGameType(0),
    m_singlePlayer(false),
    m_demoUpgradeCheck(false),
	m_clickedCampaign(false),
	m_clickedPrologue(false),
	m_showingDeviceSelector(false),
    m_uiGameType(-1),
    //m_clickGameType(-1),
	//m_leaderboardScrollOffset(0),
    m_settingUpCustomGame(false),
    m_customGameType(-1),
    m_customMapId(-1),
	m_waitingForTeamID(false),
	m_waitingGameType(-1),
    m_localSelectedLevel(-1),
    m_setupNewPage(false),
	m_reloadPage(false),
    m_startingTutorial(false),
	m_scrollBar(NULL),
    m_gameTypeFilter(-1),
    m_showDemosFilter(true),
    m_showingErrorDialogue(false),
    m_showIncompatibleGamesFilter(true),
	m_showFullGamesFilter(true),
	m_darwiniaPlusAnimOffset(0.0f),
	m_darwiniaPlusMPOnly(false),
	m_darwiniaPlusAnimating(false),
	m_darwiniaPlusSelectionX(0.0f), 
	m_darwiniaPlusSelectionY(0.0f), 
	m_darwiniaPlusSelectionW(0.0f), 
	m_darwiniaPlusSelectionH(0.0f),
	m_darwiniaPlusSelectedLeft(false),
	m_darwiniaPlusSelectAnimTime(0.0f),
	m_initedDarwiniaSelection(false),
	m_motdFound(false),
    m_soundsStarted(false),
    m_joinServer(NULL),
    m_errorBackPage(-1),
	m_setupAchievementsPage(false),
    m_chatToggledThisUpdate(false),
    m_serverIndex(-1),
    m_disconnectFromGame(false),
    m_showCustomMapsFilter(true)
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();
    SetPosition( 0, 0 );
    SetSize( w , h  );
    m_resizable = false;
    SetMovable( false );
    m_closeable = false;

#ifndef MULTIWINIA_DEMOONLY
    Authentication_GetKey(m_authKey);
    int authResult = Authentication_SimpleKeyCheck( m_authKey, METASERVER_GAMETYPE_MULTIWINIA );
    if( authResult != AuthenticationAccepted )
    {
        if( strcmp( m_authKey, "authkey not found" ) == 0 )
        {
            strcpy( m_authKey, "" );
        }

        m_newPage = GameMenuWindow::PageAuthKeyEntry;
	}
#endif // MULTIWINIA_DEMOONLY

    UnicodeString name;
	UnicodeString prefsName = g_prefsManager->GetUnicodeString( "PlayerName", "multiwina_playername_default");
	bool defaultName = prefsName == UnicodeString( "multiwina_playername_default" );
	if( defaultName )	name = LANGUAGEPHRASE("multiwina_playername_default");
	else				name = UnicodeString(prefsName);

    for( int i = 0; i < wcslen(name.m_unicodestring); ++i )
    {
        if( name.m_unicodestring[i] == L'%' ) name.m_unicodestring[i] = L' ';
    }
	m_teamName.Set(name);
    //m_currentRequestedColour = g_prefsManager->GetInt( "PlayerPreferedColour" );

	AppReleaseAssert( sizeof(s_pageNames)/sizeof(char *) == NumPages, "Mismatch between Page enum and static page names array, %d != %d", sizeof(s_pageNames)/sizeof(char *), NumPages );

    m_showDemosFilter               = (bool)g_prefsManager->GetInt( "FilterShowDemos", 1 );
    m_showIncompatibleGamesFilter   = (bool)g_prefsManager->GetInt( "FilterShowIncompatible", 1 );
	m_showFullGamesFilter           = (bool)g_prefsManager->GetInt( "FilterShowFullGames", 1 );
    m_showCustomMapsFilter          = (bool)g_prefsManager->GetInt( "FilterShowCustomMaps", 1 );
    m_showPasswordedFilter          = (bool)g_prefsManager->GetInt( "FilterShowPassworded", 1 );
}


GameMenuWindow::~GameMenuWindow()
{
    m_chatMessages.EmptyAndDelete();
}

void GameMenuWindow::PreloadTextures()
{
	static const char *textureNames[] = 
	{
		"icons/button_b.bmp",
		"icons/button_lb_shadow.bmp",
		"icons/button_rb_shadow.bmp",
		"icons/button_lb.bmp",
		"icons/button_rb.bmp",
		"icons/button_a.bmp",
		"textures/interface_blur.bmp",
		"sprites/darwinian.bmp",
		"mwhelp/gear." TEXTURE_EXTENSION,
		"textures/laser.bmp",
		"textures/clouds.bmp",
		NULL,
	};

	for( const char **textureName = textureNames; *textureName; textureName++ )	
	{
		g_app->m_resource->GetTexture( *textureName );
	}
}

void GameMenuWindow::Create()
{
}

void GameMenuWindow::Update()
{
    if( !m_soundsStarted )
    {
        if( !SoundSystem::NotReadyForGameSoundsYet() )
        {
            m_soundsStarted = true;
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "LobbyAmbience", SoundSourceBlueprint::TypeMultiwiniaInterface );
        }
    }

    if( m_buttonOrder.Size() > 0 && !m_showingErrorDialogue && !ChatInputWindow::IsChatVisible() && !m_chatToggledThisUpdate )
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

    if( m_currentPage != m_newPage )
    {
        //if( m_clickGameType == m_gameType ||
        //    g_app->m_clientToServer->m_connectionState == ClientToServer::StateDisconnected )
        //{
		    ShutdownPage( m_currentPage );
            SetupNewPage( m_newPage );

            int w = g_app->m_renderer->ScreenW();
            int h = g_app->m_renderer->ScreenH();
            SetPosition( 0, 0 );
            SetSize( w, h );
        //}
    }
	else if( m_reloadPage )
	{
		SetupNewPage(m_currentPage);
		m_reloadPage = false;
	}

    if( m_demoUpgradeCheck != IS_DEMO )
    {
        m_demoUpgradeCheck = IS_DEMO;
        if( m_currentPage == PageDemoSinglePlayer )
        {
            m_newPage = PageDarwinia;
        }
    }

    //if( !GetButton( g_target->X(), g_target->Y() ) && g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )m_highlightedLevel = -1;

#ifdef TESTBED_ENABLED
	UpdateTestBed();
#endif

	switch( m_currentPage )
	{
		case PageMain:
            if( m_quickStart ) UpdateGameOptions(false);
			UpdateMainPage();
			break;

		case PageJoinGame: 
			UpdateJoinGamePage();
			break;

		case PageAchievements:
			UpdateAchievementsPage();
			break;

		case PageQuickMatchGame:
			UpdateQuickMatchGamePage();
			break;

		/*case PageLeaderBoard:
			UpdateLeaderboardPage();
			break;*/

		case PageGameConnecting:
			UpdateGameOptions( false );
			UpdateGameConnectingPage();
			break;

		case PageGameSetup:
			UpdateMultiplayerPage();
        case PageMapSelect:
        case PageGameSelect:
		case PagePlayerOptions:
			UpdateGameOptions( true );
			break;

        case PageDemoSinglePlayer:
			UpdatePageSinglePlayer();
            break;

        case PageAdvancedOptions:
            UpdateAdvancedOptionsPage();
            break;

        case PageTutorials:
            UpdateGameOptions(false);
            UpdateMainPage();
            break;

		case PageWaitForGameToStart:
			break;
	}

    if( m_currentPage != m_newPage ) 
    {
        m_setupNewPage = true;
    }
}

#ifdef TESTBED_ENABLED

typedef struct _tagGameEntry
{
	int			m_iGameNumber;
	int			m_iMapNumber;
	int			m_iMapCRC;

} GAME_ENTRY, *PGAME_ENTRY;


std::vector<GAME_ENTRY>		g_vGameEntries;
bool bMapSelected = false;

typedef enum TESTBED_PROGRESSION
{
	TESTBED_LINEAR,
	TESTBED_RANDOM,
	TESTBED_SEQUENCE,
};

TESTBED_PROGRESSION g_eTestBedProgression;


typedef struct _tagGameSequence
{
	int		m_iIndex;
	int		m_iRandSeed;

} GAME_SEQUENCE, *PGAME_SEQUENCE;

std::vector<GAME_SEQUENCE>		g_vGameSequence;
int								g_iGameSequenceIndex;
char							m_cTestBedName[256];
bool							m_bFirstTime = true;
GAME_SEQUENCE					g_CurrentGameSequence;
int								g_iSelectedRandSeed;


static void BuildGameEntries()
{
	g_vGameEntries.clear();
	for(int i = 0; i < g_app->m_gameMenu->m_maps->Size(); i++)
	{
		// The != 6 is all about coping with the fact that the game finds 7 game modes instead of just 6
		// Since the buttons for the 7th game do not exist the testbed will fail so don't let them on this list
		if(g_app->m_gameMenu->m_maps->ValidIndex(i) && i != 6)
		{

			DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[i];

			for(int x = 0; x < maps.Size(); x++)
			{
				if(maps.ValidIndex(x))
				{
					GAME_ENTRY ge;
					ge.m_iGameNumber = i;
					ge.m_iMapNumber = x;
					ge.m_iMapCRC = maps.GetData(i)->m_mapId;
					g_vGameEntries.push_back(ge);
				}
			}
		}
	}
}

int FindGameEntry( int _gameMode, int _mapNumber )
{
	int numEntries = g_vGameEntries.size();

	for( int i = 0; i < numEntries; i++ )
	{
		GAME_ENTRY &ge = g_vGameEntries[i];

		if( ge.m_iGameNumber == _gameMode &&
			ge.m_iMapNumber == _mapNumber )
		{
			return i;
		}
	}
	return -1;
}

void GameMenuWindow::UpdateTestBed()
{
	if(!g_app)
	{
		return;
	}

	if(g_app->m_server)
	{
		if(g_iSelectedRandSeed != 0)
		{
			g_app->m_server->m_iRandSeed = g_iSelectedRandSeed;
			g_app->m_server->m_bRandSeedOverride = true;
		}
		else
		{
			g_app->m_server->m_bRandSeedOverride = false;
		}
	}


	static int s_testBedSeqId = 0;
	if(g_app->GetTestBedMode() == TESTBED_ON)
	{
		if(g_app->m_clientToServer)
		{
			g_app->m_clientToServer->ReliableSetTeamName( g_app->GetTestBedServerName() );
		}

		if(g_app->GetTestBedType() == TESTBED_SERVER)
		{
			switch(g_app->GetTestBedState())
			{
			case TESTBED_BUILD_GAME_ENTRIES:
				// I am going to build a massive table of games and maps so that different testbed modes
				// can be supported.

				 srand ( time(NULL) );

				// Reset the main testbed variables
				g_iSelectedRandSeed = 0;				
				g_eTestBedProgression = TESTBED_LINEAR;
				g_app->SetTestBedGameIndex(0);
				m_bFirstTime = true;
				g_app->m_iTerminationSequenceID = 0;

				if(g_app->m_server)
				{
					g_app->m_server->m_bRandSeedOverride = false;
				}

				BuildGameEntries();

				// Load in the testbed config
				ReadTestBedConfig();

				AdvanceTestBedGame();

				g_app->SetTestBedState(TESTBED_MAINMENU);
				break;

			case TESTBED_SEQUENCEID_DELAY:
			{
				if(g_lastProcessedSequenceId > g_app->m_iDesiredSequenceID)
				{
					g_app->SetTestBedState(g_app->m_eNextState);
				}

			}
			break;

			case TESTBED_GAME_OVER_DELAY:
				{
					if(g_app->m_fTestBedLastTime <= 0.0f)
					{
						g_app->m_fTestBedLastTime = GetHighResTime();

					}
					else
					{

						g_app->m_fTestBedDelay += GetHighResTime() - g_app->m_fTestBedLastTime;
						g_app->m_fTestBedLastTime = GetHighResTime();

						if(g_app->m_fTestBedDelay > 10.0f)
						{
							AdvanceTestBedGame();
							g_app->m_multiwinia->RemoveTeams(g_app->m_clientToServer->m_clientId);
							g_app->m_spectator = false;

							//g_app->ShutdownCurrentGame();
							g_app->SetTestBedState(TESTBED_MAINMENU);
						}

					}
	
				}
				break;

			case TESTBED_MAINMENU:
				{
					// Reset the delay
					g_app->m_fTestBedDelay = 0.0f;
					g_app->m_fTestBedLastTime = 0.0f;
					g_app->m_iClientCount = 0;

					if(m_currentPage == PageMain)
					{
						NewOrJoinButton* pButton = (NewOrJoinButton*)GetButton("multiwinia_menu_multiplayergame");

						if(pButton)
						{
							pButton->MouseUp();
							g_app->SetTestBedState(TESTBED_HOST_GAME);
						}
					}
				}
				break;

			case TESTBED_HOST_GAME:
				{								
					if(m_currentPage == PageNewOrJoin)
					{
						((NewServerButton*)GetButton("multiwinia_menu_hostgame"))->MouseUp();
						g_app->SetTestBedState(TESTBED_WAIT_FOR_CLIENT_ID);
					}
				}

				break;

			case TESTBED_WAIT_FOR_CLIENT_ID:
				{
					if(g_app->m_globalWorld->m_myTeamId != 255)
					{
						g_app->SetTestBedState(TESTBED_SELECT_GAME);

						g_app->m_clientToServer->ReliableSetTeamName( g_app->GetTestBedServerName() );
					}
				}

				break;

			case TESTBED_SELECT_GAME:
				{
					if(m_currentPage == PageGameSelect)
					{
						GameTypeButton* pButton = 0;

						GAME_ENTRY ge = g_vGameEntries[g_app->GetTestBedGameIndex()];

						int iGameMode = ge.m_iGameNumber;
						int iMap = ge.m_iMapNumber;

						g_app->m_iLastGame = iGameMode;
						g_app->m_iLastMap = iMap;

						switch(iGameMode)
						{
						case 0:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_domination");
								strcpy(g_app->m_cGameMode,"Domination");
								break;
						case 1:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_kingofthehill");
								strcpy(g_app->m_cGameMode,"King Of The Hill");
								break;
						case 2:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_capturethestatue");
								strcpy(g_app->m_cGameMode,"Capture the Statue");
								break;
						case 3:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_assault");
								strcpy(g_app->m_cGameMode,"Assault");
								break;
						case 4:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_rocketriot");
								strcpy(g_app->m_cGameMode,"Rocket Riot");
								break;
						case 5:
								pButton = (GameTypeButton*)GetButton("multiwinia_gametype_blitzkrieg");
								strcpy(g_app->m_cGameMode,"Blitzkrieg");
								break;
						}

						if(pButton)
						{
							pButton->MouseUp();
							g_app->SetTestBedState(TESTBED_SELECT_MAP);
							bMapSelected = false;

							g_app->m_clientToServer->ReliableSetTeamName( g_app->GetTestBedServerName() );
						}

					}
					
				}
				break;

			case TESTBED_SELECT_MAP:
				{
					if(m_currentPage == PageMapSelect && !bMapSelected)
					{
						GAME_ENTRY ge = g_vGameEntries[g_app->GetTestBedGameIndex()];

						int iGameMode = ge.m_iGameNumber;
						int iMap = ge.m_iMapNumber;

						DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[iGameMode];

						if(maps.Size() != 0)
						{						
							char *buttonName = maps[iMap]->m_levelName;
							if( !buttonName )
								buttonName = maps[iMap]->m_fileName;

							LevelSelectButton* pButton = (LevelSelectButton*) GetButton( buttonName );
								
							if(pButton)
							{
								strcpy( g_app->m_cGameMap, LANGUAGEPHRASE( GetMapNameId( maps[iMap]->m_fileName ) ) );

								pButton->MouseUp();
								g_app->SetTestBedState(TESTBED_WAIT_FOR_MAP);
								bMapSelected = true;
							}
						}
					}
				}
				break;

			case TESTBED_WAIT_FOR_MAP:
				{
					if(m_localSelectedLevel == m_requestedMapId)
					{
						if( 0 <= m_gameType && m_gameType < MAX_GAME_TYPES &&
							g_app->m_gameMenu->m_maps[m_gameType].ValidIndex( m_requestedMapId ) )
						{
							g_app->SetTestBedState(TESTBED_SETGAMEOPTIONSDELAY);
							s_testBedSeqId = g_lastProcessedSequenceId;
						}
					}
				}
				break;
	
			case TESTBED_SETGAMEOPTIONSDELAY:
				if( g_lastProcessedSequenceId > s_testBedSeqId+5 )
				{
					g_app->SetTestBedState(TESTBED_SETGAMEOPTIONS);
				}
				break;

			case TESTBED_SETGAMEOPTIONS:
				{
					//int timeLimitOption = g_app->m_multiwinia->GetGameOption( "TimeLimit" );
					//if( timeLimitOption != -1 )
					//{
					//	m_params[timeLimitOption] = 1;
					//}

					// Can add more like Sudden Death here

					// Become a spectator
					if(g_app->m_globalWorld->m_myTeamId != 255)
					{
						// JAK TEST
						g_app->m_clientToServer->RequestToRegisterAsSpectator(g_app->m_globalWorld->m_myTeamId);
					}

					g_app->SetSequenceIDDelay(g_lastProcessedSequenceId + 10,TESTBED_WAIT_FOR_CLIENTS);
				}
				break;

			case TESTBED_WAIT_FOR_CLIENTS:
				{
					if(m_currentPage == PageGameSetup)
					{

						if( g_app->m_server->m_clients.NumUsed() > 1 )
						{
							g_app->SetSequenceIDDelay(g_lastProcessedSequenceId + 10, TESTBED_PLAY);

						}
					}

				}
				break;

			case TESTBED_PLAY:
				{
					// See GameMenuWindow::UpdateMultiplayer() for testbed ready code
					if( m_currentPage != PageGameSetup )
					{
						g_app->IncrementGameCount();
						g_app->SetTestBedState(TESTBED_IDLE);
					}
				}
				break;

			case TESTBED_IDLE:

				break;
			}
		}
		else
		{
			// We are in client mode
			switch(g_app->GetTestBedState())
			{

			case TESTBED_BUILD_GAME_ENTRIES:

				g_iSelectedRandSeed = 0;
				g_eTestBedProgression = TESTBED_LINEAR;
				g_app->SetTestBedGameIndex(0);
				g_app->m_iTerminationSequenceID = 0;

				if(g_app->m_server)
				{
					g_app->m_server->m_bRandSeedOverride = false;
				}

				BuildGameEntries();

				// Load in the testbed config
				ReadTestBedConfig();

				g_app->SetTestBedState(TESTBED_MAINMENU);
				break;

			case TESTBED_SEQUENCEID_DELAY:
			{
				if(g_lastProcessedSequenceId > g_app->m_iDesiredSequenceID)
				{
					g_app->SetTestBedState(g_app->m_eNextState);
				}

			}
			break;

			case TESTBED_GAME_OVER_DELAY:
				{
					if(g_app->m_fTestBedLastTime <= 0.0f)
					{
						g_app->m_fTestBedLastTime = GetHighResTime();

					}
					else
					{

						g_app->m_fTestBedDelay += GetHighResTime() - g_app->m_fTestBedLastTime;
						g_app->m_fTestBedLastTime = GetHighResTime();

						if(g_app->m_fTestBedDelay > 40.0f)
						{
							g_app->m_multiwinia->RemoveTeams(g_app->m_clientToServer->m_clientId);
							g_app->m_spectator = false;

							//g_app->ShutdownCurrentGame();
							g_app->SetTestBedState(TESTBED_MAINMENU);

						}

					}
	
				}
				break;

			case TESTBED_MAINMENU:
				{

					g_app->m_fTestBedDelay = 0.0f;
					g_app->m_fTestBedLastTime = 0.0f;

					if(m_currentPage == PageMain)
					{
						NewOrJoinButton* pButton = (NewOrJoinButton*)GetButton("multiwinia_menu_multiplayergame");

						if(pButton)
						{
							pButton->MouseUp();
							g_app->SetTestBedState(TESTBED_JOIN_GAME);
						}
					}
				}
				break;


			case TESTBED_JOIN_GAME:
				{
					if(m_currentPage == PageNewOrJoin)
					{
						((NewServerButton*)GetButton("multiwinia_menu_joingame"))->MouseUp();
						g_app->SetTestBedState(TESTBED_PICK_SERVER);
					}
				}
				break;

			case TESTBED_PICK_SERVER:
				{

					if(m_currentPage == PageJoinGame)
					{

						g_app->m_fTestBedDelay += GetHighResTime() - g_app->m_fTestBedLastTime;
						g_app->m_fTestBedLastTime = GetHighResTime();

						if(g_app->m_fTestBedDelay > 3.0f)
						{

							g_app->m_fTestBedDelay = 0.0f;
							g_app->m_fTestBedLastTime = 0.0f;
							g_app->m_fTestBedLastTime = GetHighResTime();
							
							if(m_serverButtons.Size() != 0)
							{
								// Pick a server
								for(int i = 0; i < m_serverButtons.Size(); i++)
								{
									if(m_serverButtons.ValidIndex(i))
									{
										GameServerButton *pButton = m_serverButtons[i];

										if( pButton )
										{
											if( pButton->m_hostName == g_app->GetTestBedServerName() )
											{
												if(strcmp(pButton->m_mapName.GetCharArray(),"Unknown Map") != 0)
												{
													strcpy(g_app->m_cGameMode,pButton->m_gameType.GetCharArray());
													strcpy(g_app->m_cGameMap,pButton->m_mapName.GetCharArray());

													AppDebugOut("Client attaching to %s game mode and %s map\n",pButton->m_gameType.GetCharArray(),pButton->m_mapName.GetCharArray());
													
													pButton->MouseUp();
													g_app->IncrementGameCount();
													//CDebug::Dump();


													//g_app->m_clientToServer->RequestToRegisterAsSpectator(g_app->m_globalWorld->m_myTeamId);
													g_app->SetTestBedState(TESTBED_MAKE_CLIENT_SPECTATOR);
													break;
												}
											}
										}
									}

								}
							}
							else
							{
								g_app->m_fTestBedDelay = 0.0f;
								g_app->m_fTestBedLastTime = 0.0f;
								g_app->m_fTestBedLastTime = GetHighResTime();


							}

						}
					}
				}
				break;

			case TESTBED_MAKE_CLIENT_SPECTATOR:
				if( m_gameType != -1 && 
					m_requestedMapId != -1 )
				{
					int gameIndex = FindGameEntry( m_gameType, m_requestedMapId );
					if( gameIndex != -1 )
					{
						g_app->m_iGameIndex = gameIndex;
						g_app->SetTestBedState(TESTBED_IDLE);
					}
				}
				break;

			case TESTBED_IDLE:
				{

					int a = 0;
					a++;
				}

				break;

			}

		}

	}


}

void GameMenuWindow::AdvanceTestBedGame()
{

	switch(g_eTestBedProgression)
	{
	case TESTBED_LINEAR:
		if(g_app->GetTestBedGameIndex() < static_cast<int>(g_vGameEntries.size()))
		{
			int iIndex = g_app->GetTestBedGameIndex();
			iIndex++;
			g_app->SetTestBedGameIndex(iIndex);
		}
		break;

	case TESTBED_RANDOM:
		g_app->SetTestBedGameIndex( rand()% (int)g_vGameEntries.size() );
		break;

	case TESTBED_SEQUENCE:
		g_CurrentGameSequence.m_iIndex = 0;
		g_CurrentGameSequence.m_iRandSeed = 0;

		if(m_bFirstTime)
		{
			g_iGameSequenceIndex = 0;

			g_CurrentGameSequence = g_vGameSequence[g_iGameSequenceIndex];
			g_iGameSequenceIndex++;

			if(g_iGameSequenceIndex == static_cast<int>(g_vGameSequence.size()))
			{
				g_iGameSequenceIndex = 0;
			}

			m_bFirstTime = false;
		}
		else
		{

			if(g_iGameSequenceIndex < static_cast<int>(g_vGameSequence.size()))
			{
				g_CurrentGameSequence = g_vGameSequence[g_iGameSequenceIndex];
				g_iGameSequenceIndex++;

				if(g_iGameSequenceIndex == static_cast<int>(g_vGameSequence.size()))
				{
					g_iGameSequenceIndex = 0;
				}
			}
			else
			{
				g_iGameSequenceIndex = 0;

				g_CurrentGameSequence = g_vGameSequence[g_iGameSequenceIndex];
				g_iGameSequenceIndex++;

				if(g_iGameSequenceIndex == static_cast<int>(g_vGameSequence.size()))
				{
					g_iGameSequenceIndex = 0;
				}
			}
		}


		g_iSelectedRandSeed = g_CurrentGameSequence.m_iRandSeed;
		g_app->SetTestBedGameIndex(g_CurrentGameSequence.m_iIndex);
		break;
	}


}

void GameMenuWindow::ReadTestBedConfig()
{
	TextFileReader reader( "testbedconfig.txt" );

	if( !reader.IsOpen() )
		return;

	g_vGameSequence.clear();

	while( reader.ReadLine() )
	{
		const char *command = reader.GetNextToken();
		if( !command ) 
			continue;

		if( strcmp( command, "SetProgression" ) == 0 )
		{
			const char *parameter = reader.GetNextToken();
			if( !parameter )
				continue;

			if( strcmp( parameter, "LINEAR" ) == 0 )
				g_eTestBedProgression = TESTBED_LINEAR;
			else if( strcmp( parameter, "RANDOM" ) == 0 )
				g_eTestBedProgression = TESTBED_RANDOM;
			else if( strcmp( parameter, "SEQUENCE" ) == 0 )
				g_eTestBedProgression = TESTBED_SEQUENCE;
		}
		else if( strcmp( command, "SetRandSeed" ) == 0 )
		{
			const char *parameter = reader.GetNextToken();
			if( !parameter )
				continue;

			sscanf( parameter, "%d", &g_iSelectedRandSeed );
		}
		else if( strcmp( command, "SetTestBedName" ) == 0 )
		{
			const char *parameter = reader.GetNextToken();
			if( !parameter )
				continue;

			sscanf( parameter, "%s", &m_cTestBedName );
			g_app->m_testbedServerName = m_cTestBedName;
		}
		else if( strcmp( command,"AddSequenceItem" ) == 0 )
		{
			GAME_SEQUENCE gs;

			const char *parameter = reader.GetNextToken();
			if( !parameter )
				continue;
			sscanf( parameter, "%d", &gs.m_iIndex );

			parameter = reader.GetNextToken();
			if( !parameter )
				continue;
			sscanf( parameter, "%d", &gs.m_iRandSeed );

			g_vGameSequence.push_back(gs);
		}
		else if( strcmp( command, "SequenceIDToTerminateOn") == 0 )
		{
			const char *parameter = reader.GetNextToken();
			if( !parameter )
				continue;
			sscanf( parameter, "%d", &g_app->m_iTerminationSequenceID );
		}
	}
}

#endif

void GameMenuWindow::CreateErrorDialogue( UnicodeString _error, int _backPage )
{
    m_errorMessage = _error;
    //m_showingErrorDialogue = true;
    //m_lockMenu = true;
    m_newPage = PageError;
    m_errorBackPage = _backPage;
    g_app->m_atLobby = false;

    m_chatMessages.EmptyAndDelete();
}

void GameMenuWindow::NewChatMessage( UnicodeString _msg, int _fromTeamId )
{
//    if( _fromTeamId < 0 || _fromTeamId > 3 ) return;

    ChatMessage *message = new ChatMessage;
    message->m_msg = _msg;
    message->m_clientId = _fromTeamId;
    message->m_recievedTime = GetHighResTime();
    m_chatMessages.PutDataAtStart( message );

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( g_app->m_multiwinia->m_teams[i].m_clientId == _fromTeamId )
        {
            message->m_senderName = g_app->m_multiwinia->m_teams[i].m_teamName;
            message->m_colour = g_app->m_multiwinia->m_teams[i].m_colour;
            break;
        }
    }

    if( _msg != UnicodeString("PLAYERJOIN") )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "ChatMessage", SoundSourceBlueprint::TypeMultiwiniaInterface );
    }
}

void GameMenuWindow::UpdatePageSinglePlayer()
{
	// This is the single player menu
    if( m_quickStart ) 
		UpdateGameOptions(false);

	if( g_app->m_multiwiniaTutorial )
		UpdateMainPage();
}

void GameMenuWindow::UpdateMultiplayerPage()
{
    if( g_inputManager->controlEvent( ControlLobbyChat ) && !EclIsTextEditing() && !m_chatToggledThisUpdate )
    {
        BeginTextEdit("chat_input");
        ChatInputField *input = (ChatInputField *)GetButton( "chat_input" );
        input->m_buf = UnicodeString();
    }

    m_chatToggledThisUpdate = false;

    if( m_disconnectFromGame )
    {
        // we've clicked the exit game button but our team was set to ready
        // wait until our team is no longer ready before continuing
        bool go = false;
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( g_app->m_multiwinia->m_teams[i].m_teamType != TeamTypeUnused && 
                !g_app->m_multiwinia->m_teams[i].m_ready )
            {
                go = true;
                break;
            }
        }
        int teamId = g_app->m_globalWorld->m_myTeamId;
        if( teamId != 255 && go )
        {
            if( !g_app->m_multiwinia->m_teams[teamId].m_ready )
            {
                g_app->ShutdownCurrentGame();
		        g_app->m_atLobby = false;
                m_requestedMapId = -1;
		        m_newPage = PageMain;

                m_chatMessages.EmptyAndDelete();
                m_serverPassword.Set( UnicodeString() );
                
                m_disconnectFromGame = false;
            }
        }
    }

    if( g_app->m_server )
    {
        bool notReady = false;
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            LobbyTeam *team = &g_app->m_multiwinia->m_teams[i];
            if( team->m_teamType == TeamTypeUnused || team->m_teamType == TeamTypeCPU ) continue;
            if( !team->m_ready )
            {
                notReady = true;
				break;
            }
        }
#ifdef TESTBED_ENABLED
		if( g_app->GetTestBedMode() == TESTBED_ON )
			notReady = !(g_app->GetTestBedState() == TESTBED_PLAY);
#endif

        if( IS_DEMO && DemoRestrictedMap() )
        {
            notReady = true;
        }

        if( !notReady )
        {
            if( 0 <= m_gameType && m_gameType < MAX_GAME_TYPES &&
                g_app->m_gameMenu->m_maps[m_gameType].ValidIndex( m_requestedMapId ) )
            {
                MapData *mapData = g_app->m_gameMenu->m_maps[m_gameType][m_requestedMapId];
                mapData->CalculeMapId();
                if( g_app->IsMapPermitted( m_gameType, mapData->m_mapId ) )
                {
                    g_app->m_soundSystem->WaitIsInitialized();

                    // fill up any empty player slots with AI teams before starting the game
                    for( int i = 0; i < NUM_TEAMS; ++i )
                    {
                        if( i < g_app->m_gameMenu->m_maps[m_gameType][m_requestedMapId]->m_numPlayers &&
                            g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused )
                        {
                            g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
                        }
                    }

			        UpdateGameOptions( true );
			        g_app->m_clientToServer->RequestStartGame();

				    // Transition to a new page while we wait for the game to start
				    // instead of going through this again and again 
					m_newPage = PageWaitForGameToStart;
                }
            }
		    else 
            {
			    AppDebugOut("Game Menu: Failed to start game due to invalid game type or map id (Game Type = %d, Map ID = %d)\n",
				    (int) m_gameType, (int) m_requestedMapId );
		    }
        }
    }

	if( !m_waitingForTeamID ) return;

	if( g_app->m_globalWorld->m_myTeamId != 255 ||
        g_app->m_spectator )
	{
		AppDebugOut("Setting up MP page\n");
		SetupMultiplayerPage(m_waitingGameType);
		m_waitingGameType = -1;
		m_waitingForTeamID = false;
	}
}

void GameMenuWindow::UpdateGameConnectingPage()
{
	if( g_app->m_clientToServer->m_connectionState == ClientToServer::StateConnected && 
		m_gameType != -1 )
	{		
		//m_clickGameType = m_gameType;
		m_newPage = PageGameSetup;

		AppDebugOut("------------------> Been disconnected\n");
	}
	else
	{
#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() == TESTBED_ON)
		{
			g_app->m_fTestBedDelay += GetHighResTime() - g_app->m_fTestBedLastTime;
			g_app->m_fTestBedLastTime = GetHighResTime();

			if(g_app->m_fTestBedDelay > 10.0f)
			{
				g_app->m_spectator = false;

				FinishMultiwiniaButton* pButton = (FinishMultiwiniaButton*)GetButton("multiwinia_menu_back");

				if(pButton)
				{
					pButton->MouseUp();
				}

				//g_app->ShutdownCurrentGame();
				g_app->SetTestBedState(TESTBED_PICK_SERVER);
			}
		}
#endif		


	}

}

bool GameMenuWindow::DemoRestrictedMap() const
{
	return DemoRestrictedMap( m_gameType, m_requestedMapId );
}

bool GameMenuWindow::DemoRestrictedMap( int _mapId ) const
{	
	return DemoRestrictedMap( m_gameType, _mapId );
}

bool GameMenuWindow::DemoRestrictedMap( int _gameType, int _mapId ) const
{
	if( _gameType < 0 || _gameType >= MAX_GAME_TYPES || _mapId < 0 )
		return true;

	DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[ _gameType ];
	
	if( !maps.ValidIndex( _mapId ) )
		return true;

	return !g_app->IsMapAvailableInDemo( _gameType, maps[ _mapId ]->m_mapId );
}

void GameMenuWindow::SetTeamNameAndColour()
{
	ClientToServer *cToS = g_app->m_clientToServer;

	if( cToS )
	{
        int myTeam = g_app->m_globalWorld->m_myTeamId;
        if( myTeam != -1 && myTeam != 255 &&
            m_currentRequestedColour != -1 &&
            g_app->m_multiwinia->m_teams[myTeam].m_colourId != m_currentRequestedColour &&
            g_app->m_multiwinia->IsColourTaken( m_currentRequestedColour ) &&
            !m_coopMode &&
            m_gameType != Multiwinia::GameTypeAssault )
        {
            int preferedColour = g_prefsManager->GetInt( "PlayerPreferedColour", -1 );
            if( preferedColour != -1 && 
                !g_app->m_multiwinia->IsColourTaken(preferedColour) )
            {
                m_currentRequestedColour = preferedColour;
            }
            else
            {
                m_currentRequestedColour = g_app->m_multiwinia->GetNextAvailableColour();
            }
        }

        if( m_currentPage == PageGameSetup ||
            m_currentPage == PagePlayerOptions ||
            m_startingTutorial)
        {
            if( m_currentRequestedColour != -1 || m_coopMode || m_gameType == Multiwinia::GameTypeAssault )
			    cToS->ReliableSetTeamColour( m_currentRequestedColour );
        }

        if( !cToS->m_requestedTeamDetails.m_name && m_teamName.Get().WcsLen() > 0 )
            cToS->ReliableSetTeamName( m_teamName.Get() );
	}
}

void GameMenuWindow::UpdateGameOptions( bool _checkUserInterface )
{

	// Check to see if the user interface has updated any options
	ClientToServer *cToS = g_app->m_clientToServer;
	Multiwinia *mw = g_app->m_multiwinia;
	int teamId = g_app->m_globalWorld->m_myTeamId;

	//g_app->m_multiwinia->RemoveTeams(teamId);
	//g_app->m_clientToServer->RequestTeam(TeamTypeUnused,teamId);
	//g_app->m_spectator = true;

	if( _checkUserInterface && cToS )
	{
		SetTeamNameAndColour();
	}

    if( m_currentPage == PageMapSelect ) UpdateMapSelectPage();

	bool gameTypeChanged = false;
	gameTypeChanged =
		m_uiGameType != mw->m_gameType &&
		mw->m_gameType != -1 && m_uiGameType != -1;

	if( gameTypeChanged && 
        (m_currentPage == PageGameSetup ||
         m_currentPage == PageMapSelect ) )
	{
		ShutdownPage( m_currentPage );
		SetupNewPage( m_currentPage );
	}

    if( m_quickStart )
    {
        if( g_app->m_server && g_app->m_multiwiniaTutorial && !m_startingTutorial )
        {
            if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 )
            {
                SetupTutorial();
            }
            else if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 )
            {
                SetupTutorial2();
            }
        }

        if( teamId != 255 || g_app->m_spectator )
        {
            g_app->m_soundSystem->WaitIsInitialized();

            //cToS->RequestTeam( TeamTypeLocalPlayer, 0 );
            SetTeamNameAndColour();

            if( m_gameType >= 0 && m_gameType < MAX_GAME_TYPES && g_app->m_gameMenu->m_maps[m_gameType].ValidIndex( m_requestedMapId ) )
            {
                for( int i = g_app->m_gameMenu->m_maps[m_gameType].GetData( m_requestedMapId )->m_numPlayers; i > 1; i-- )
                {
                    cToS->RequestTeam( TeamTypeCPU, -1 );
                }
            }
            
            cToS->RequestStartGame();
            m_quickStart = false;
        }
    }
}

void GameMenuWindow::ApplyServerFilters()
{
    if( !m_serverList ) return;

    ApplyFilterInvalidGameTypes();
    if( m_gameTypeFilter != -1 )		ApplyFilterGameType();

    if( !m_showFullGamesFilter)			ApplyFilterFullGames();
    if( !m_showIncompatibleGamesFilter) ApplyFilterProtocolMatch();
    if( !m_showDemosFilter )			ApplyFilterShowDemos();
    if( !m_showIncompatibleGamesFilter)	ApplyFilterMissingMaps();
    if( !m_showPasswordedFilter )       ApplyFilterPasswords();
}


void GameMenuWindow::ApplyBasicServerListSort()
{
    if( !m_serverList ) return;


    LList<Directory *> *newList = new LList<Directory *>();

    int numGoodServers = 0;

    for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *server = m_serverList->GetData(i);

        if( !IsProtocolMatch(server) )
        {
            newList->PutDataAtEnd(server);
        }
        else if( IsServerFull(server) ||
                 server->HasData( NET_METASERVER_DEMORESTRICTED ) )
        {
            newList->PutDataAtIndex(server, numGoodServers);
        }
        else
        {
            newList->PutDataAtStart(server);
            ++numGoodServers;
        }
    }


    //
    // Replace the un-ordered list with the ordered list

    delete m_serverList;
    m_serverList = newList;
}


void GameMenuWindow::ApplyBetterSlowerServerListSort()
{
    if( !m_serverList ) return;


    //
    // Score each server depending on the sort order

    for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *server = m_serverList->GetData(i);

        int score = 0;

        if( IsServerFull(server) ) score = 1;
        if( !IsProtocolMatch(server) ) score = 10;
        
        server->CreateData( "OrderRank", score );
    }


    //
    // Build an ordered list based on the score

    LList<Directory *> *newList = new LList<Directory *>();

    for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *server = m_serverList->GetData(i);
        int score = server->GetDataInt( "OrderRank" );

        bool added = false;
        for( int j = 0; j < newList->Size(); ++j )
        {
            Directory *thisServer = newList->GetData(j);
            int thisScore = thisServer->GetDataInt( "OrderRank" );

            if( score <= thisScore )
            {
                newList->PutDataAtIndex( server, j );
                added = true;
                break;
            }
        }

        if( !added )
        {
            newList->PutDataAtEnd( server );
        }
    }


    //
    // Replace the un-ordered list with the ordered list

    delete m_serverList;
    m_serverList = newList;

}

void GameMenuWindow::ApplyFilterPasswords()
{
    for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);
		if( d->HasData( NET_DARWINIA_SERVER_PASSWORD ) )
		{
			delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
		}

    }
}

void GameMenuWindow::ApplyFilterGameType()
{
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);
        
        if( d->GetDataChar(NET_METASERVER_GAMEMODE ) != (unsigned char)m_gameTypeFilter )
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
        }
	}
}

void GameMenuWindow::ApplyFilterFullGames()
{
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);

		int max_teams = d->GetDataChar( NET_METASERVER_MAXTEAMS );
		int num_teams = d->GetDataChar( NET_METASERVER_NUMTEAMS );
        
        if( num_teams >= max_teams)
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
        }
	}
}


void GameMenuWindow::ApplyFilterInvalidGameTypes()
{
#if defined(HIDE_INVALID_GAMETYPES)
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);

        if( !g_app->IsGameModePermitted( d->GetDataChar( NET_METASERVER_GAMEMODE ) ) )
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
        }
	}
#endif
}

void GameMenuWindow::ApplyFilterMissingMaps()
{
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);
        
        int gameMode = d->GetDataChar( NET_METASERVER_GAMEMODE );
        int mapCRC = d->GetDataInt( NET_METASERVER_MAPCRC );
        int mapIndex = g_app->m_gameMenu->GetMapIndex( gameMode, mapCRC );
        if( mapIndex == -1 )
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
        }
	}
}

bool IsProtocolMatch( Directory *d )
{
    bool protocolMatch = false;
    if( d->HasData( NET_METASERVER_PROTOCOLMATCH, DIRECTORY_TYPE_INT ) )
    {
        protocolMatch = d->GetDataInt( NET_METASERVER_PROTOCOLMATCH ) != 0;
    }

    if( !protocolMatch )
    {
        if( d->HasData( NET_METASERVER_GAMEVERSION, DIRECTORY_TYPE_STRING ) )
        {
            protocolMatch = (strcmp( d->GetDataString( NET_METASERVER_GAMEVERSION ), APP_VERSION ) == 0 );
        }
    }

	return protocolMatch;
}

bool GameMenuWindow::IsServerFull( Directory *server  )
{
    int maxPlayers = server->GetDataChar(NET_METASERVER_MAXTEAMS);
    int numPlayers = server->GetDataChar(NET_METASERVER_NUMTEAMS);
    
    return numPlayers == maxPlayers;
}

void GameMenuWindow::ApplyFilterProtocolMatch()
{
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);

        if( !IsProtocolMatch( d ) )
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
        }
	}
}

void GameMenuWindow::ApplyFilterShowDemos()
{
	for( int i = 0; i < m_serverList->Size(); ++i )
    {
        Directory *d = m_serverList->GetData(i);

        if( d->GetDataBool(NET_METASERVER_DEMOSERVER) )
        {
            delete d;
            m_serverList->RemoveData(i);
            i--;
            continue;
        }
	}
}

struct ServerKey
{
	ServerKey( Directory *_server );

	unsigned long long m_serverStartTime;
	int m_serverRandomNumber;

	bool operator < ( const ServerKey &_other ) const;
};

ServerKey::ServerKey( Directory *_server )
:	m_serverStartTime( _server->GetDataULLong( NET_METASERVER_STARTTIME ) ),
	m_serverRandomNumber( _server->GetDataInt( NET_METASERVER_RANDOM ) )
{
}

inline bool ServerKey::operator < ( const ServerKey &_other ) const
{
	return m_serverStartTime < _other.m_serverStartTime ||
		( m_serverStartTime == _other.m_serverStartTime && m_serverRandomNumber < _other.m_serverRandomNumber );
}

void RemoveDuplicateWANServers( LList<Directory *> *_lanServerList, LList<Directory *> *_wanServerList )
{
	typedef std::set< ServerKey > ServerKeySet;
	ServerKeySet lanServer;

	int serverListSize = _lanServerList->Size();
	for( int i = 0; i < serverListSize; i++ )
	{
		Directory *server = _lanServerList->GetData( i );
		lanServer.insert( server );
	}

	serverListSize = _wanServerList->Size();
	for( int i = 0; i < serverListSize; i++ )
	{
		Directory *server = _wanServerList->GetData( i );

		if( lanServer.find( server ) != lanServer.end() )
		{
			_wanServerList->RemoveData( i );
			delete server;
			i--;
			serverListSize--;
		}
	}
} 

void GameMenuWindow::UpdateServerList()
{
	if( m_serverList )
	{
		m_serverList->EmptyAndDelete();
		delete m_serverList;	
		m_serverList = NULL;
	}

	// Get the LAN servers first, and remove any duplicate WAN entries 
	m_serverList = MetaServer_GetServerList( false, false, true ); 
	LList<Directory *> *wanServers = MetaServer_GetServerList( false, true, false ); 
	RemoveDuplicateWANServers( m_serverList, wanServers );
	Append( *m_serverList, *wanServers );
	delete wanServers;

	RemoveDelistedServers( m_serverList );  

    ApplyServerFilters();
    ApplyBasicServerListSort();    
}

void GameMenuWindow::JoinServer( int _serverNumber )
{	
	// Signal that we are not joining as a spectator
	m_bSpectator = false;

	Directory *server = m_serverList->GetData( _serverNumber );
    
    if( m_joinServer ) delete m_joinServer;
    m_joinServer = new Directory( *server );

	char localIp[256];
	int localPort;
	bool haveIdentity = g_app->m_clientToServer->GetIdentity( localIp, &localPort );
	char *serverIp = server->GetDataString( NET_METASERVER_IP );
	int serverPort = 
		server->HasData( NET_METASERVER_PORT ) 
			?	server->GetDataInt( NET_METASERVER_PORT ) 
			:	-1;

	if( haveIdentity && stricmp( serverIp, localIp ) == 0 )
	{
		// Their public IP matches ours, so it's probably a LAN game.
		serverIp = server->GetDataString( NET_METASERVER_LOCALIP );
		serverPort = server->GetDataInt( NET_METASERVER_LOCALPORT );
	}

	if( serverPort == -1 )
	{
		return;
	}

    if( m_serverPassword.Get().Length() > 0 )
    {
        g_app->m_clientToServer->m_password = m_serverPassword.Get();
    }
	
	g_app->StartNetwork( false, serverIp, serverPort ); 
	
#ifdef TESTBED_ENABLED
	// We need a timeout on the game connecting screen if we are in testbed mode
	if(g_app->GetTestBedMode() == TESTBED_ON)
	{
		g_app->m_fTestBedDelay = 0.0f;
		g_app->m_fTestBedLastTime = GetHighResTime();

	}
#endif
	m_newPage = GameMenuWindow::PageGameConnecting;
}

void GameMenuWindow::JoinServerAsSpectator( int _serverNumber )
{	
	// Signal that we are joining as a spectator
	m_bSpectator = true;
	Directory *server = m_serverList->GetData( _serverNumber );
    
    if( m_joinServer ) delete m_joinServer;
    m_joinServer = new Directory(*server);

	char localIp[256];
	int localPort;
	bool haveIdentity = g_app->m_clientToServer->GetIdentity( localIp, &localPort );
	char *serverIp = server->GetDataString( NET_METASERVER_IP );
	int serverPort = server->GetDataInt( NET_METASERVER_PORT );

	if( haveIdentity && stricmp( serverIp, localIp ) == 0 )
	{
		// Their public IP matches ours, so it's probably a LAN game.
		serverIp = server->GetDataString( NET_METASERVER_LOCALIP );
		serverPort = server->GetDataInt( NET_METASERVER_LOCALPORT );
	}

	if( haveIdentity && stricmp( serverIp, localIp ) == 0 )
	{
		// Their public IP matches ours, so it's probably a LAN game.
		serverIp = server->GetDataString( NET_METASERVER_LOCALIP );
		serverPort = server->GetDataInt( NET_METASERVER_LOCALPORT );
	}

	
	g_app->StartNetwork( false, serverIp, serverPort ); 
	m_newPage = GameMenuWindow::PageGameConnecting;
}


void GameMenuWindow::UpdateQuickMatchGamePage()
{
	double now = GetHighResTime();
	static double lastTimeUpdatedList = now;

	if (now > lastTimeUpdatedList + 1.0) 
	{
		lastTimeUpdatedList = now;
		/*
		*
		* Check the metaserver for some games
		*
		*/

		UpdateServerList();
	}

	if( m_serverList && m_serverList->Size() > 0 )
	{
		// Found a game, let's try to join it
		for( int i = 0; i < m_serverList->Size(); i++ )
		{
			if( m_serverList->ValidIndex(i) )
			{
				Directory* d = m_serverList->GetData(i);

				if( !IsProtocolMatch(d) || IsServerFull(d) )
				{
					continue;
				}

				JoinServer(i);
				break;
			}
		}
	}

    double updateInterval = 10.0;
	static double lastTimeRequestedWAN = now - updateInterval - 1;
    if (now > lastTimeRequestedWAN + updateInterval) 
	{		
		lastTimeRequestedWAN = now;
		MetaServer_RequestServerListWAN( METASERVER_GAMETYPE_MULTIWINIA );
	}	

}


void GameMenuWindow::UpdateMapSelectPage()
{
    if( m_clickGameType != -1 )
    {
        if( m_clickGameType == m_gameType )
        {
            SetupMapSelectPage();
        }
    }
}


void GameMenuWindow::UpdateAdvancedOptionsPage()
{
    bool inactive = false;
    if( g_app->m_multiwinia->GetGameOption( "TimeLimit" ) == 0 ) inactive = true;

    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[m_gameType];
    for( int i = 0; i < blueprint->m_params.Size(); ++i )
    {
        MultiwiniaGameParameter *param = blueprint->m_params[i];
        if( strcmp( param->m_name, "SuddenDeath" ) == 0 )
        {
            EclButton *button = GetButton(param->GetStringId());
            if( button )
            {
                if( button->m_inactive != inactive )
                {
                    if( param->m_change == -1 )
                    {
                        GameMenuCheckBox *buttonCheckBox = (GameMenuCheckBox *)button;
                        buttonCheckBox->Set(inactive ? 0 : 1);
                    }
                    else if( param->m_change == 0 )
                    {
                        GameMenuDropDown *buttonDropDown = (GameMenuDropDown *)button;
                        buttonDropDown->SelectOption(inactive ? 0 : 1);
                    }
                }
                button->m_inactive = inactive;                
            }
        }
    }
}

static bool s_gotListFromMetaserver = false;

void GameMenuWindow::UpdateAchievementsPage()
{
	m_y = 0;
	if( g_app->m_achievementTracker->HasLoaded() && !m_setupAchievementsPage )
	{
		m_setupAchievementsPage = true;
	}
}

void GameMenuWindow::UpdateJoinGamePage()
{
	static double lastTimeUpdatedList = 0.0f;
	static int oldScrollValue = 0;

    double now = GetHighResTime();
    bool updateNow = ( now > lastTimeUpdatedList + 2.0f );
	bool receivedList = true;

	receivedList = MetaServer_HasReceivedListWAN();

	if( m_serverList && 
        m_serverList->Size() == 0 && 
        now > lastTimeUpdatedList + 1.0f &&
		receivedList ) updateNow = true;

	if (updateNow) 
	{
		lastTimeUpdatedList = now;
		/*
		* Check the metaserver for some games
		*/

		UpdateServerList();
        s_gotListFromMetaserver = true;

		// Add in any manual server addresses
		for( int i = 0; i < 30; i++ )
		{
			char prefsName[256];
			sprintf( prefsName, "Server%d", i + 1 );

			if( !g_prefsManager->DoesKeyExist( prefsName ) )
				continue;

			const char *serverLine = g_prefsManager->GetString( prefsName, NULL );
			if( !serverLine )
				continue;

			char serverHost[256] = "0.0.0.0";
			int serverPort = g_prefsManager->GetInt( PREFS_NETWORKSERVERPORT, 4000 );
			char serverName[256];// = "????";
			strcpy( serverName, serverLine );

			int numItems = sscanf( serverLine, "%s %d %s", serverHost, &serverPort, serverName );

			/*if( numItems > 0 && numItems < 3 )
				sprintf( serverName, "%s:%d", serverHost, serverPort );*/

            int gameMode = i % (Multiwinia::NumGameTypes - 1);
			Directory *d = new Directory;
			d->CreateData( NET_METASERVER_SERVERNAME, serverName );
			d->CreateData( NET_METASERVER_NUMTEAMS, (unsigned char) 0 );
			d->CreateData( NET_METASERVER_MAXTEAMS, (unsigned char) 4 );
			d->CreateData( NET_METASERVER_GAMEINPROGRESS, (unsigned char) 0 );
            d->CreateData( NET_METASERVER_GAMEMODE, (unsigned char) gameMode );

			d->CreateData( NET_METASERVER_IP, serverHost );
			d->CreateData( NET_METASERVER_PORT, serverPort );
			d->CreateData( NET_METASERVER_LOCALIP, serverHost );
			d->CreateData( NET_METASERVER_LOCALPORT, serverPort );

			m_serverList->PutDataAtStart( d );
		}
		
		int numGames = min( m_serverList->Size(), SERVERS_ON_SCREEN);	// List only the first six games

		int a = 0;
		a++;

		// Make enough buttons
		while( m_serverButtons.Size() < numGames )
		{
			int buttonNum = m_serverButtons.Size();
			GameServerButton *button = new GameServerButton( buttonNum );

			char cName[128];
			static int buttonNumber = 0;
			buttonNumber++;
			sprintf( cName, "%s%d", button->m_name, buttonNumber );

			button->SetShortProperties( 
				cName, m_serverX, m_serverY+m_serverGap*buttonNum, m_serverW, m_serverH, UnicodeString() );
			button->m_fontSize = m_serverFontSize;

			RegisterButton( button );
			m_buttonOrder.PutDataAtIndex( button, m_buttonOrder.Size() - 1 );
			m_serverButtons.PutData( button );
		}

		// Remove extra buttons
		while( m_serverButtons.Size() > numGames )
		{
			int buttonNum = m_serverButtons.Size() - 1;
			GameServerButton *button = m_serverButtons.GetData( buttonNum );
			RemoveButton( button->m_name );
			m_serverButtons.RemoveDataAtEnd();
			m_buttonOrder.RemoveData( m_serverButtonOrderStartIndex + buttonNum );
		}

		// Set button properties 
		for( int i = 0; i < numGames; i++ )
		{
			Directory *server = m_serverList->GetData( i );
			GameServerButton *button = m_serverButtons.GetData( i );

			// AppDebugOut( "Processing server result %d\n", i );
			
            int gameMode = server->GetDataChar( NET_METASERVER_GAMEMODE );
            int mapCrc = server->GetDataInt( NET_METASERVER_MAPCRC );
            const char *mapName = g_app->m_gameMenu->GetMapName( gameMode, mapCrc );
			
			if( !mapName ) mapName = "Unknown Map";

			char gameModeName[512];
			GameTypeToShortCaption( gameMode, gameModeName );

			button->m_hostName = UnicodeString(server->GetDataString( NET_METASERVER_SERVERNAME ));
			button->m_gameType = LANGUAGEPHRASE(gameModeName);
			button->m_mapName = LANGUAGEPHRASE(mapName);
			button->m_maxPlayers = server->GetDataChar(NET_METASERVER_MAXTEAMS);
			button->m_numPlayers = server->GetDataChar(NET_METASERVER_NUMTEAMS);
		}

		if( m_scrollBar )
		{
			m_scrollBar->SetNumRows( m_serverList->Size() );
		}


		// Set the position of the back button
		//EclButton *back = GetButton( "Back" );
		//if( back ) 
		//	back->m_y = m_serverY + numGames * m_serverGap;		
	}


    //
    // Request the server listing every updateInterval (10.0) seconds

    double updateInterval = 10.0;
    static double lastTimeRequestedWAN = 0.0f;

    if (now > lastTimeRequestedWAN + updateInterval) 
    {		
        lastTimeRequestedWAN = now;
        MetaServer_RequestServerListWAN( METASERVER_GAMETYPE_MULTIWINIA );
        s_gotListFromMetaserver = false;
    }	

}

void GameMenuWindow::RenderNetworkStatus()
{
	//
	// Render the connected parties if we are a server

	Server *server = g_app->m_server;
#ifdef TARGET_DEBUG
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();

	int fontSize = 15;
	int h = fontSize + 3;
	int x = 10;
    int y = 10;

	if (server) 
	{
		int numClients = server->m_clients.Size();

		glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
		g_gameFont.DrawText2D( x, y+=h, fontSize, "IP:Port [Sent/Known/CaughtUp]" );

		for (int i = 0; i < numClients; i++) 
		{
			if (server->m_clients.ValidIndex( i )) 
			{
				ServerToClient *sToC = server->m_clients[i];

				char fromStr[64] = "";
				sprintf( fromStr, "%s:%d", sToC->m_ip, sToC->m_port );
				g_gameFont.DrawText2D( x, y+=h, fontSize, "%s [%d/%d/%d]", 
					fromStr, sToC->m_lastSentSequenceId, sToC->m_lastKnownSequenceId, sToC->m_caughtUp);
			}
		}

		y += 2 * h;
	}

	if (g_app->m_clientToServer) 
	{
		// Render connection status
		glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );

		switch (g_app->m_clientToServer->m_connectionState) 
		{
		case ClientToServer::StateDisconnected:
			g_gameFont.DrawText2D( x, y+=h, fontSize, "Disconnected" );
			break;

		case ClientToServer::StateConnecting:
			g_gameFont.DrawText2D( x, y+=h, fontSize, "Connecting (%d)", g_app->m_clientToServer->m_connectionAttempts );
			break;

		case ClientToServer::StateHandshaking:
			g_gameFont.DrawText2D( x, y+=h, fontSize, "Handshaking (%d/%d)", g_lastProcessedSequenceId, g_app->m_clientToServer->m_lastValidSequenceIdFromServer );
			break;

		case ClientToServer::StateConnected:
			g_gameFont.DrawText2D( x, y+=h, fontSize, "Connected" );
			break;
		}

		y += 2 * h;
	}
#endif
}


void GameMenuWindow::RenderBigDarwinian()
{
    //
    // Add colour to background

    glShadeModel( GL_SMOOTH );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    float overscan = 0.0f;

    float x = 0;
    float y = 0;
    float w = g_app->m_renderer->ScreenW();
    float h = g_app->m_renderer->ScreenH();

    glBegin( GL_QUADS);
    glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
    glVertex2f( x - overscan, y - overscan );
    glVertex2f( x + w + overscan, y - overscan);
    glVertex2f( x + w + overscan, y + h + overscan);
    glColor4f( 1.0f, 0.5f, 0.1f, 1.0f );
    glVertex2f( x - overscan, y + h + overscan );
    glEnd();

    //
    // Switch to lower quality rendering if our fps is bad

    static double s_smoothFps = 60.0f;
    static bool s_renderHighDetail = true;
    
    if( g_app->m_multiwinia->GameInLobby() )
    {
        // Only make the decision if we're in the lobby
        // since in-game, other factors may destroy our frame rate
        double smoothFactor = g_advanceTime * 0.5f;
        s_smoothFps = ( s_smoothFps * (1-smoothFactor) ) + float( g_app->m_renderer->m_fps * smoothFactor);
        if( s_smoothFps < 20 ) s_renderHighDetail = false;
        if( s_smoothFps >= 60 ) s_renderHighDetail = true;
    }


    //
    // Draw a big fucking darwinian

    Vector3 centrePos( x + w * 0.75f, 0, y + h * 0.7f );
    Vector3 up( 0, 0, 1 );
    Vector3 right( 1, 0, 0 );
    float size = w * 0.3f;

    //centrePos += Vector3( sinf(GetHighResTime()) * 5, 0, cosf(GetHighResTime()) + sinf(GetHighResTime()) * 4);
    size += sinf(GetHighResTime()) * 5;

    up.RotateAroundY( 0.2f );
    right.RotateAroundY( 0.2f );

    Vector3 pos1 = centrePos - up * size - right * size;
    Vector3 pos2 = centrePos - up * size + right * size;
    Vector3 pos3 = centrePos + up * size + right * size;
    Vector3 pos4 = centrePos + up * size - right * size;

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

    glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
    glBegin( GL_QUADS );
    glTexCoord2i(0,1);  glVertex2f( pos1.x, pos1.z );
    glTexCoord2i(1,1);  glVertex2f( pos2.x, pos2.z );
    glTexCoord2i(1,0);  glVertex2f( pos3.x, pos3.z );
    glTexCoord2i(0,0);  glVertex2f( pos4.x, pos4.z );
    glEnd();

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    int numIterations = ( s_renderHighDetail ? 10 : 0 );

    for( int i = 0; i < numIterations; ++i )
    {
        size *= 1.02f;
        centrePos -= Vector3(6,0,6);
        pos1 = centrePos - up * size - right * size;
        pos2 = centrePos - up * size + right * size;
        pos3 = centrePos + up * size + right * size;
        pos4 = centrePos + up * size - right * size;

        float alpha = 0.2f * (1 - i/(float)numIterations);
        glColor4f( 1.0f, 0.2f, 0.2f, alpha );
        glBegin( GL_QUADS );
        glTexCoord2i(0,1);  glVertex2f( pos1.x, pos1.z );
        glTexCoord2i(1,1);  glVertex2f( pos2.x, pos2.z );
        glTexCoord2i(1,0);  glVertex2f( pos3.x, pos3.z );
        glTexCoord2i(0,0);  glVertex2f( pos4.x, pos4.z );
        glEnd();
    }

    glDisable( GL_TEXTURE_2D );

    //
    // Gear effect

    size = w * 0.6f;

    int texId = g_app->m_resource->GetTexture( "mwhelp/gear." TEXTURE_EXTENSION );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, texId );

    Vector3 v1( - size, - size, 0 );
    Vector3 v2( + size, - size, 0 );
    Vector3 v3( + size, + size, 0 );
    Vector3 v4( - size, + size, 0 );

    float timeFactor = GetHighResTime() * 0.2f;
    v1.RotateAroundZ( timeFactor );
    v2.RotateAroundZ( timeFactor );
    v3.RotateAroundZ( timeFactor );
    v4.RotateAroundZ( timeFactor );

    Vector3 offset( w * 0.5f, h * 0.5f, 0 );

    v1 += offset;
    v2 += offset;
    v3 += offset;
    v4 += offset;

    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glColor4f( 1.0f, 1.0f, 1.0f, 0.06f );

    for( int i = 0; i < numIterations; ++i )
    {
        v1.RotateAroundZ( 0.02f );
        v2.RotateAroundZ( 0.02f );
        v3.RotateAroundZ( 0.02f );
        v4.RotateAroundZ( 0.02f );

        glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( v1.GetData() );
        glTexCoord2i(1,1);      glVertex3dv( v2.GetData() );
        glTexCoord2i(1,0);      glVertex3dv( v3.GetData() );
        glTexCoord2i(0,0);      glVertex3dv( v4.GetData() );
        glEnd();
    }

    glDisable( GL_TEXTURE_2D );    
}

void RenderLoadingAchievementsEffect()
{
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;
    float screenY = g_app->m_renderer->ScreenH() * 0.6f;
        
    float large, med, small;
    GetFontSizes( large, med, small );

    UnicodeString caption = LANGUAGEPHRASE("multiwinia_error_loadingachievement");

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_editorFont.SetRenderOutline(true);
    g_editorFont.DrawText2DCentre( screenX, screenY, small, caption );

    if( fmodf( GetHighResTime(), 1.0f ) < 0.5f )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
    }

    g_editorFont.SetRenderOutline(false);
    g_editorFont.DrawText2DCentre( screenX, screenY, small, caption );
}

void RenderRequestingEffect()
{
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;
    float screenY = g_app->m_renderer->ScreenH() * 0.6f;
        
    float large, med, small;
    GetFontSizes( large, med, small );

    UnicodeString caption = LANGUAGEPHRASE("multiwinia_error_requestinglist");
	bool receivedList = true;
	receivedList = MetaServer_HasReceivedListWAN();
    if( receivedList && s_gotListFromMetaserver )
    {
        caption = LANGUAGEPHRASE("multiwinia_error_noserversfound");
    }

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_editorFont.SetRenderOutline(true);
    g_editorFont.DrawText2DCentre( screenX, screenY, small, caption );

    if( fmodf( GetHighResTime(), 1.0f ) < 0.5f )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 0.5f );
    }

    g_editorFont.SetRenderOutline(false);
    g_editorFont.DrawText2DCentre( screenX, screenY, small, caption );
}


void GameMenuWindow::Render( bool _hasFocus )
{
    if( !_hasFocus && !EclGetWindow("location_chat")) return;

    RenderBigDarwinian();

    //
    // Render the title

    float titleX, titleY, titleW, titleH;
    float fontLarge, fontMed, fontSmall;

    GetPosition_TitleBar( titleX, titleY, titleW, titleH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    DrawFillBox( titleX, titleY, titleW, titleH );

    float fontX = titleX + titleW/2;
    float fontY = titleY + titleH * 0.2f;

    UnicodeString titleCaption = LANGUAGEPHRASE("multiwinia_mainmenu_title");

    if( titleCaption.StrCmp( "MULTIWINIA" ) == 0 )
    {
        // Use the bitmap to render the title at higher quality

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/mwtitle.bmp", false ) );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        float titleImageW = g_titleFont.GetTextWidth(titleCaption.Length(), fontLarge);
        titleImageW *= 1.1f;
        float titleImageH = titleImageW / 5.4f;            
        float titleImageX = titleX + titleW/2.0f - titleImageW/2.0f;
        float titleImageY = titleY + titleH * 0.1f;

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( titleImageX, titleImageY );
            glTexCoord2i(1,1);      glVertex2f( titleImageX + titleImageW, titleImageY );
            glTexCoord2i(1,0);      glVertex2f( titleImageX + titleImageW, titleImageY + titleImageH );
            glTexCoord2i(0,0);      glVertex2f( titleImageX, titleImageY + titleImageH );
        glEnd();

        glDisable( GL_TEXTURE_2D );
    }
    else
    {               
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        for( int i = 0; i < 3; ++i )
        {
            g_titleFont.DrawText2DCentre( fontX, fontY, fontLarge, titleCaption );
        }

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );    
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DCentre( fontX, fontY, fontLarge, titleCaption );
    }

    //
    // Render the subtitle

    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2DCentre( fontX+5, fontY+fontLarge, fontMed, LANGUAGEPHRASE("multiwinia_mainmenu_subtitle") );
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2DCentre( fontX+5, fontY+fontLarge, fontMed, LANGUAGEPHRASE("multiwinia_mainmenu_subtitle") );



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

	bool loadedAchievementImaged = true;
    switch( m_currentPage )
    {
        case PageGameSetup:
            DrawFillBox( leftX, leftY, leftW, leftH );
            DrawFillBox( rightX, rightY, rightW, rightH );
#ifdef TARGET_DEBUG
            RenderNetworkStatus();
#endif
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            break;

        case PageJoinGame:
            DrawFillBox( leftX, leftY, titleW, leftH );
            //DrawFillBox( rightX, rightY, rightW, rightH );
    
            if( !m_serverList || m_serverList->Size() == 0  )
            {
                RenderRequestingEffect();
            }
            break;

		case PageAchievements:
			DrawFillBox( leftX, leftY, titleW, leftH );
			for( int i = 0; i < m_achievementButtons.Size(); i++ )
			{
				loadedAchievementImaged &= m_achievementButtons.GetData(i)->HasLoadedImage();
			}
			if( !g_app->m_achievementTracker->HasLoaded() || !loadedAchievementImaged )
			{
				RenderLoadingAchievementsEffect();
			}
			break;

        case PageGameConnecting:
#ifdef TARGET_DEBUG
            RenderNetworkStatus();
#endif
            DrawFillBox( leftX, leftY, leftW, leftH );
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );			
            break;

        case PageGameSelect:
        case PageMapSelect:
	    case PageLeaderBoardSelect:
        case PageAdvancedOptions:
        case PageDemoSinglePlayer:
        case PageBuyMeNow:
            DrawFillBox( leftX, leftY, leftW, leftH );
            DrawFillBox( rightX, rightY, rightW, rightH );
            break;

		case PageMain:
        {
			DrawFillBox( leftX, leftY, leftW, leftH );

			bool showAuth = ShowAuthBox();

            float motdH = rightH;
            if( showAuth  ) motdH *= 0.75f;

            if( m_motdFound ) DrawFillBox( rightX, rightY, rightW, motdH );
            if( showAuth  )
            {
                DrawFillBox( rightX, rightY + rightH * 0.8f, rightW, rightH * 0.2f );
            }
            break;
        }

        default:
            DrawFillBox( leftX, leftY, leftW, leftH );
            break;
    }



    //DarwiniaWindow::Render( _hasFocus );
    // render nothing but the buttons
    if( !GameMenuDropDownWindow::s_window )
    {
        EclWindow::Render( _hasFocus );
    }
    else
    {
        // If there is a DropDown menu open, only render that button.
        EclButton *button = GetButton( GameMenuDropDownWindow::s_window->m_name );
        if( button )
        {
            bool highlighted = EclMouseInButton( this, button ) || strcmp(m_currentTextEdit, button->m_name) == 0;
            bool clicked = ( _hasFocus && strcmp ( EclGetCurrentClickedButton(), button->m_name ) == 0 );
            button->Render( m_x + button->m_x, m_y + button->m_y, highlighted, clicked );
        }
    }

    if( m_showingErrorDialogue )
    {
        RenderErrorDialogue();
    }
}

bool GameMenuWindow::ShowAuthBox()
{
#ifdef MULTIWINIA_DEMOONLY
    return false;
#endif

	//if( !g_app->IsFullVersion() ) return false;
	// Called from both GameMenuWindow::Render() for the border
	// and AuthStatusButton::Render() to determine whether the 
	// auth status should be rendered or not.

    char authKey[256];
    Authentication_GetKey( authKey );
	bool isDemoKey = Authentication_IsDemoKey( authKey );

    int authResult = Authentication_GetStatus( authKey );
   

    //
    // Also show the auth box if our auth status has changed
    // in the last couple of seconds

    static int s_lastAuth = -1;
    static bool s_lastDemo = false;
    static double s_lastTime = 0.0;

    if( authResult != s_lastAuth || isDemoKey != s_lastDemo )
    {
        s_lastAuth = authResult;
        s_lastDemo = isDemoKey;
        s_lastTime = GetHighResTime();
    }

    if( GetHighResTime() < s_lastTime + 2.0f ) 
    {
        return true;
    }


    //
    // Always show demo status
    // And bad status

    if( authResult != AuthenticationAccepted || isDemoKey )
    {
        return true;
    }

	return false;
}

void GameMenuWindow::RenderErrorDialogue()
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
    g_titleFont.DrawText2DCentre( xPos, yPos, fontMed, LANGUAGEPHRASE("error") );

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

void GameMenuWindow::SetupNewPage( int _page )
{
	AppDebugOut("Setting up page: %s (%d) \n", s_pageNames[_page], _page );

    Remove();
	if( m_scrollBar )
	{
		delete m_scrollBar;
		m_scrollBar = NULL;
	}

    switch( _page )
    {
        case PageMain:          SetupMainPage();                    break;
        case PageDarwinia:      SetupDarwiniaPage();                break;
		case PageNewOrJoin:     SetupNewOrJoinPage();				break;
		case PageJoinGame:	    SetupJoinGamePage();				break;
		case PageAchievements:	SetupAchievementsPage();			break;
        case PageGameSelect:    SetupGameSelectPage();              break;
		case PageGameConnecting:SetupGameConnectingPage();			break;
        case PageGameSetup:     SetupMultiplayerPage( m_gameType ); break;
        case PageResearch:      SetupResearchPage();                break;
        case PageMapSelect:     SetupMapSelectPage();               break;
        case PageHelpMenu:      SetupHelpMenuPage();                break;
        case PageHelp:          SetupHelpPage();                    break;
        case PageAdvancedOptions:   SetupAdvancedOptionsPage();     break;
        case PageDemoSinglePlayer:  SetupSPDemoPage();              break;
        case PageBuyMeNow:      SetupBuyMePage();                   break;
		case PageLeaderBoardSelect:	SetupLeaderBoardSelectPage();	break;
		case PageLeaderBoard:	SetupLeaderboardPage();				break;
		case PageQuickMatchGame: SetupQuickMatchGamePage();			break;
        case PageAuthKeyEntry:  SetupAuthKeyPage();                 break;
		case PagePlayerOptions:	SetupPlayerOptionsPage();			break;
        case PageSearchFilters: SetupFiltersPage();                 break;
		case PageUpdateAvailable:	SetupUpdatePage();				break;
        case PageCredits:       SetupCreditsPage();                 break;
        case PageTutorials:     SetupTutorialPage();                break;
        case PageError:         SetupErrorPage();                   break;
        case PageEnterPassword: SetupPageEnterPassword();           break;
		case PageWaitForGameToStart: break;
		default:
			AppReleaseAssert( false, "Game menu invalid page (%d)", _page );
    }

	if( g_app->m_gameMenu->m_menuCreated && !m_reloadPage )
    {
        g_app->m_renderer->StartMenuTransition();
    }

    m_currentPage = _page;
    m_setupNewPage = false;
}

void GameMenuWindow::SetupMainPage()
{
    SetTitle( LANGUAGEPHRASE( "multiwinia_mainmenu_title" ) );

#if defined(LAN_PLAY_ENABLED) || defined(WAN_PLAY_ENABLED)
	m_singlePlayer = false;
#else
	//m_singlePlayer = true;
#endif
    m_settingUpCustomGame = false;
    m_localSelectedLevel = -1;

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

	float rightX, rightY, rightW, rightH;
    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
	GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;
   
    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_mainmenutitle" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;


    // Single player game
    bool tutButton = true;
#if defined(INCLUDE_TUTORIAL)
    if( !g_app->IsFullVersion() )
    {
        TutorialPageButton *tpb = new TutorialPageButton();
        tpb->SetShortProperties( "multiwinia_menu_tutorial_short", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_tutorial_short") );
        tpb->m_fontSize = fontSize;
        RegisterButton( tpb );
        m_buttonOrder.PutData( tpb );

        tutButton = false;
    }
#endif

    GameMenuButton *button = new DarwiniaModeButton("multiwinia_menu_singleplayergame");
    button->SetShortProperties( "multiwinia_menu_singleplayergame", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_singleplayergame") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    // Multi player game


    button = new NewOrJoinButton("multiwinia_menu_multiplayergame");
    button->SetShortProperties( "multiwinia_menu_multiplayergame", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_multiplayergame" ) );
    button->m_fontSize = fontSize;
	if( !g_app->MultiplayerPermitted() )
		button->m_inactive = true;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

#ifdef INCLUDE_TUTORIAL
    if( tutButton )
    {
        TutorialPageButton *tpb = new TutorialPageButton();
        tpb->SetShortProperties( "multiwinia_menu_tutorial_short", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_tutorial_short") );
        tpb->m_fontSize = fontSize;
        RegisterButton( tpb );
        m_buttonOrder.PutData( tpb );
    }
#endif

    // Help and options

    button = new HelpAndOptionsButton();
    button->SetShortProperties( "helpandoptions", buttonX, yPos +=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_helpandoptions") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );


#ifdef TESTBED_ENABLED
	StartTestbedButton *startTestServer = new StartTestbedButton( TESTBED_SERVER );
    startTestServer->SetShortProperties("start_testbed_server", buttonX, yPos += buttonH + gap, buttonW, buttonH, LANGUAGEPHRASE("start_testbed_server") );
    startTestServer->m_fontSize = fontSize;
    RegisterButton( startTestServer );
    m_buttonOrder.PutData( startTestServer );

	StartTestbedButton *startTestClient = new StartTestbedButton( TESTBED_CLIENT );
    startTestClient->SetShortProperties("start_testbed_server", buttonX, yPos += buttonH + gap, buttonW, buttonH, LANGUAGEPHRASE("start_testbed_client") );
    startTestClient->m_fontSize = fontSize;
    RegisterButton( startTestClient );
    m_buttonOrder.PutData( startTestClient );	
#endif

#ifdef LOCATION_EDITOR
	MultiwiniaEditorButton *editor = new MultiwiniaEditorButton();
	editor->SetShortProperties( "editor", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("taskmanager_mapeditor") );
	editor->m_fontSize = fontSize;
    if( !g_app->IsFullVersion() )
    {
        editor->m_inactive = true;
    }
	RegisterButton( editor );
	m_buttonOrder.PutData( editor );
#endif

	Directory *latestVersion = MetaServer_RequestData( NET_METASERVER_DATA_LATESTVERSION );
    if( latestVersion )
    {
		if( latestVersion->HasData( NET_METASERVER_DATA_LATESTVERSION ) )
		{
			char *latestVersionString = latestVersion->GetDataString( NET_METASERVER_DATA_LATESTVERSION );
			if( strcmp( latestVersionString, APP_VERSION ) != 0 )
			{
				UpdatePageButton *updates = new UpdatePageButton();
				updates->SetShortProperties("multiwinia_update", buttonX, yPos += buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_update_available") );
				updates->m_fontSize = fontSize;
				RegisterButton( updates );
				m_buttonOrder.PutData( updates );
			}
		}
		delete latestVersion;
	}

    CreditsButton *credits = new CreditsButton();
    credits->SetShortProperties("credits_button", buttonX, yPos += buttonH + gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_credits") );
    credits->m_fontSize = fontSize;
    RegisterButton( credits );
    m_buttonOrder.PutData( credits );

    // Buy now 

    if( IS_DEMO )
    {
        button = new BuyNowModeButton();
        button->SetShortProperties( "multiwinia_menu_buyfullgame", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_buyfullgame") );
        button->m_fontSize = fontSize;
        RegisterButton( button );
        m_buttonOrder.PutData( button );
    }

    yPos = leftY + leftH - buttonH * 2;


    // Quit

    button = new QuitButton();
    button->SetShortProperties( "multiwinia_menu_quit", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_quit") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	MOTDButton *motd = new MOTDButton();
	motd->SetShortProperties( "multiwinia_motd", rightX + rightW * 0.05f, rightY + rightH * 0.05f, rightW * 0.9f, ( rightH * 0.75f ) * 0.9f, UnicodeString() );
	motd->m_fontSize = fontSmall;
	RegisterButton( motd );

	AuthStatusButton *authStatus = new AuthStatusButton();
	authStatus->SetShortProperties( "multiwinia_authStatus", rightX + rightW * 0.05f, rightY + rightH * 0.8f + rightH * 0.05f, rightW * 0.9f, ( rightH * 0.2f ) * 0.9f, UnicodeString() );
	authStatus->m_fontSize = fontSize;
	RegisterButton( authStatus );

	//g_app->m_gameMode = App::GameModeNone;
}


void GameMenuWindow::SetupDarwiniaPage()
{
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

//    PrologueButton *pb = new PrologueButton( "icons/menu_prologue.bmp" );
    PrologueButton *pb = new PrologueButton( "Prologue" );
    pb->SetShortProperties("prologue", x, y, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_prologue") );
    pb->m_fontSize = fontSize;
    RegisterButton( pb );
    m_buttonOrder.PutData( pb );

//    CampaignButton *cb = new CampaignButton( "icons/menu_campaign.bmp" );
    CampaignButton *cb = new CampaignButton( "Campaign" );
    cb->SetShortProperties( "campaign", x, y+=gap+buttonH, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_campaign") );
    RegisterButton( cb );
    cb->m_fontSize = fontSize;
    m_buttonOrder.PutData( cb);

    y = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new BackPageButton( PageMain );
    back->SetShortProperties( "multiwinia_menu_back", x, y, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontSize;
    m_buttonOrder.PutData( back );
}

void GameMenuWindow::SetupQuickMatchGamePage()
{
	g_app->m_clientToServer->StartIdentifying();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY;
    float fontSize = fontMed;

    float buttonX = x;

	y += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_setup_quick_match") );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    y += buttonH*1.3f;

	WaitingButton *text = new WaitingButton("multiwinia_menu_searching");
	text->SetShortProperties( "text", buttonX, y+=buttonH+gap, buttonW, buttonH, UnicodeString() );
	text->m_fontSize = fontSize;
	RegisterButton( text );

	m_serverX = buttonX;
	m_serverY = y;
	m_serverW = buttonW;
    m_serverH = buttonH * 0.6f;
	m_serverFontSize = fontSmall;
    m_serverGap = m_serverH;
	m_serverButtonOrderStartIndex = m_buttonOrder.Size();


    float yPos = leftY + leftH - buttonH * 2;

	//SetTitle( LANGUAGEPHRASE( "multiwinia_setup_quick_match" ) );

	GameMenuButton *button = new BackPageButton( PageNewOrJoin );
    button->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
}

void GameMenuWindow::SetupAchievementsPage()
{
	float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float titleX, titleY, titleW, titleH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH );
    GetPosition_TitleBar(titleX, titleY, titleW, titleH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    float buttonX = x;

    buttonW = (titleW / 2.0f) * 0.9f;

	m_serverX = buttonX;
	m_serverY = y;
	m_serverW = titleW * 0.9f;
	m_serverH = (g_app->m_renderer->ScreenH()/16) * 1.6f; // 64 pixels is height of achievement images
	m_serverFontSize = fontSmall;
    m_serverGap = m_serverH;
	m_serverButtonOrderStartIndex = m_buttonOrder.Size();

    float yPos = leftY;

	yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, titleW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_achievements") );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.5f;

	m_serverY = yPos;

	int scrollbarW = buttonH / 2.0f;
	int scrollbarX = leftX + titleW - (scrollbarW + 10);// 10 + buttonW;
	int scrollbarY = m_serverY;//leftY+10+buttonH*1.6f;
	int scrollbarH = (leftY + leftH - buttonH * 2)-scrollbarY;

	m_scrollBar = new ScrollBar(this);
	m_scrollBar->Create("scrollBar", scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0, 10 );

	yPos = leftY + leftH - buttonH * 2;

    GameMenuButton *button = new BackPageButton( PageMain );
    button->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

	m_setupAchievementsPage = false;
}

void GameMenuWindow::SetupJoinGamePage()
{
	g_app->m_clientToServer->StartIdentifying();
    MetaServer_RequestServerListWAN( METASERVER_GAMETYPE_MULTIWINIA );

    m_highlightedGameType = -1;
    m_highlightedLevel = -1;
    
	//SetTitle( LANGUAGEPHRASE( "multiwinia_join_game_title" ) );			

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float titleX, titleY, titleW, titleH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH );
    GetPosition_TitleBar(titleX, titleY, titleW, titleH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float x = leftX + (leftW-buttonW)/2.0f;
    float y = leftY + buttonH;
    float fontSize = fontMed;

    float buttonX = x;

    buttonW = (titleW / 2.0f) * 0.9f;

	m_serverX = buttonX;
	m_serverY = y;
	m_serverW = titleW * 0.9f;
    m_serverH = buttonH * 0.6f;
	m_serverFontSize = fontSmall;
    m_serverGap = m_serverH;
	m_serverButtonOrderStartIndex = m_buttonOrder.Size();

    float yPos = leftY;

	yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, titleW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_join_game_title") );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

	ServerTitleInfoButton *titles = new ServerTitleInfoButton();
	titles->SetShortProperties( "Titles", buttonX, leftY+10+buttonH*1.6f, m_serverW, buttonH*0.6f, UnicodeString("Titles") );
	titles->m_fontSize = fontSmall;
	RegisterButton(titles);
	yPos += buttonH * 0.8f;

	m_serverY = yPos;

	int scrollbarW = buttonH / 2.0f;
	int scrollbarX = leftX + titleW - (scrollbarW + 10);// 10 + buttonW;
	int scrollbarY = m_serverY;//leftY+10+buttonH*1.6f;
	int scrollbarH = m_serverGap * SERVERS_ON_SCREEN;

	m_scrollBar = new ScrollBar(this);
	m_scrollBar->Create("scrollBar", scrollbarX, scrollbarY, scrollbarW, scrollbarH, 0, 10 );

	yPos = leftY + leftH - buttonH * 2;

    GameMenuButton *button = new BackPageButton( PageNewOrJoin );
    button->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    button = new FiltersPageButton();
    button->SetShortProperties( "filters_page", buttonX + buttonW * 1.1f, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_filters") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );

    //
    // Map preview on right side

    float thumbnailW = rightW * 0.8f;
    float thumbnailH = thumbnailW * 0.75f;
    float thumbnailX = rightX + rightW * 0.1f;

    yPos = rightY + buttonH;

    /*MapImageButton *mib = new MapImageButton();
    mib->SetShortProperties( "MapThumbnail", thumbnailX, yPos, thumbnailW, thumbnailH, UnicodeString("MapThumbnail") );
    RegisterButton( mib );

    MapInfoButton *info = new MapInfoButton();
    info->SetShortProperties( "MapInfo", thumbnailX, yPos + thumbnailH + fontSmall, thumbnailW, thumbnailH, UnicodeString("MapInfo") );
    info->m_fontSize = fontMed;
    RegisterButton( info );*/

#ifdef FAKE_SERVERS
	MetaServer_ClearServerList();
	for( int i = 0; i < NUM_FAKE_SERVER; i ++ )
	{
		Directory* server = new Directory();
		char authkey[256];
		char name[256];
		char ip[16];
		char version[256];
		int port = rand() % 65000;
		int gameType = rand() % Multiwinia::NumGameTypes - 1;	// No tank battle, please!
		int maxTeams = 1;
        int mapCRC = 0;
		int usedTeams;
		double time = GetHighResTime();

        MapData *mapData = NULL;

		// Determine how many players can join this game
		if( rand() % 5 == 1 ) gameType = -1;

		if( 0 <= gameType && gameType < MAX_GAME_TYPES )
		{
			DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[gameType];

			if( i > 0 && maps.ValidIndex(i%(((int)i/6)+1)) )
			{
				MapData *m = maps[i%(((int)i/6)+1)];
                mapData = m;
				maxTeams = m->m_numPlayers;
                mapCRC = m->m_mapId;
			}
			else
			{
				for( int j = 0; j < maps.Size(); j++)
				{
					if( maps.ValidIndex( j ) )
					{
						MapData *m = maps[j];
						if( rand() % 500 <= 1 )
						{
							mapData = m;
							maxTeams = m->m_numPlayers;
							mapCRC = m->m_mapId;
							break;
						}
					}
				}
			}
			// Didn't find a fake one, so use the first one we can
			if( !mapData )
			{
				for( int j = 0; j < maps.Size(); j++)
				{
					if( maps.ValidIndex( j ) )
					{
						MapData *m = maps[j];
						mapData = m;
						maxTeams = m->m_numPlayers;
						mapCRC = m->m_mapId;
						break;
					}
				}
			}
		}	

		usedTeams = (rand() % maxTeams)+1;
		sprintf(name, "FakeServer-%d", i);
		sprintf(ip, "%d.%d.%d.%d", rand() % 255, rand() % 255, rand() % 255, rand() % 255);
		if( rand() % 2 == 1 )
			sprintf(version, "1.9.%d", i % 16);
		else
			sprintf(version, APP_VERSION);

		Authentication_GenerateKey(authkey, 4, true);
		
		server->CreateData( NET_METASERVER_SERVERNAME,		name );
        server->CreateData( NET_METASERVER_GAMENAME,		APP_NAME );
        server->CreateData( NET_METASERVER_GAMEVERSION,		version );
		server->CreateData( NET_METASERVER_STARTTIME,		*(unsigned long long *) &time );
		server->CreateData( NET_METASERVER_RANDOM,			i );
		server->CreateData( NET_METASERVER_LOCALIP,			ip );
        server->CreateData( NET_METASERVER_LOCALPORT,		port );
		server->CreateData( NET_METASERVER_NUMTEAMS,		(unsigned char) usedTeams );
        server->CreateData( NET_METASERVER_NUMHUMANTEAMS,   (unsigned char) (rand() % (usedTeams+1)) );
        server->CreateData( NET_METASERVER_MAXTEAMS,		(unsigned char) maxTeams );
		server->CreateData( NET_METASERVER_NUMSPECTATORS,	(unsigned char) 0 );
		server->CreateData( NET_METASERVER_MAXSPECTATORS,	(unsigned char) 0 );
		server->CreateData( NET_METASERVER_GAMEINPROGRESS,	(unsigned char) ((bool)(rand() % 2)) );
		server->CreateData( NET_METASERVER_GAMEMODE,		(unsigned char) gameType );
        server->CreateData( NET_METASERVER_MAPCRC,			mapCRC );
		server->CreateData(	NET_METASERVER_AUTHKEY,			authkey );
		server->CreateData( NET_METASERVER_PORT,			port );

		MetaServer_UpdateServerList(ip, port, true, server);
	}
#endif
}

void GameMenuWindow::ShutdownPage( int _page )
{
	switch( _page )
	{
	case PageJoinGame:
		ShutdownJoinGamePage();
		break;

	case PageGameConnecting:
		ShutdownGameConnectingPage();
		break;

	case PageAchievements:
		ShutdownAchievementsPage();
		break;
	}
}

void GameMenuWindow::ShutdownAchievementsPage()
{
	m_achievementButtons.Empty();
}

void GameMenuWindow::ShutdownJoinGamePage()
{
	if( m_newPage != PageGameConnecting )
		g_app->m_clientToServer->StopIdentifying();

	m_serverButtons.Empty();
}

void GameMenuWindow::ShutdownGameConnectingPage()
{
	g_app->m_clientToServer->StopIdentifying();
}


void GameMenuWindow::SetupTutorial()
{
#ifdef INCLUDE_TUTORIAL
    AppDebugOut("Setting up Tutorial\n");
    m_gameType = Multiwinia::GameTypeKingOfTheHill;
    g_app->m_multiwinia->m_gameType = Multiwinia::GameTypeKingOfTheHill;
    m_aiMode = Multiwinia::AITypeEasy;
    m_requestedMapId = g_app->GetMapID( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_2P_1 );			
    m_currentRequestedColour = 0;

    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[Multiwinia::GameTypeKingOfTheHill];
    for( int i = 0; i < blueprint->m_params.Size(); ++i )
    {
        MultiwiniaGameParameter *param = blueprint->m_params[i];   
        m_params[i] = param->m_default;
    }
    m_startingTutorial = true;
    g_app->m_renderer->StartFadeOut();
#endif
}

void GameMenuWindow::SetupTutorial2()
{
#ifdef INCLUDE_TUTORIAL
    AppDebugOut("Setting up Tutorial 2\n");
    m_gameType = Multiwinia::GameTypeKingOfTheHill;
    g_app->m_multiwinia->m_gameType = Multiwinia::GameTypeKingOfTheHill;
    m_aiMode = Multiwinia::AITypeEasy;
    m_requestedMapId = g_app->GetMapID( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_3P_1 );			
    m_currentRequestedColour = 0;

    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[Multiwinia::GameTypeKingOfTheHill];
    for( int i = 0; i < blueprint->m_params.Size(); ++i )
    {
        MultiwiniaGameParameter *param = blueprint->m_params[i];   
        m_params[i] = param->m_default;
    }

    for( int i = 0; i < 3; ++i )
    {
        g_app->m_multiwinia->m_teams[i].m_colourId = i;
    }
    m_startingTutorial = true;
    g_app->m_renderer->StartFadeOut();
#endif
}

void GameMenuWindow::UpdateMainPage()
{
	// Player selected single player mode from the menu
	if( m_singlePlayer )
	{
		if( !g_app->m_server )
		{
			g_app->StartMultiPlayerServer();
		}
	}

	static bool updateFound = false;
	if( !updateFound )
	{
		Directory *latestVersion = MetaServer_RequestData( NET_METASERVER_DATA_LATESTVERSION );
		if( latestVersion )
		{
            updateFound = true;
            if( latestVersion->HasData(NET_METASERVER_DATA_LATESTVERSION) )
            {
                char *latestVersionString = latestVersion->GetDataString( NET_METASERVER_DATA_LATESTVERSION );
			    if( strcmp( latestVersionString, APP_VERSION ) != 0 )
                {
			        SetupNewPage(PageMain);
                }
            }
            delete latestVersion;
		}
	}
}

void GameMenuWindow::SetupNewOrJoinPage()
{
	GameMenuButton *button = NULL;
	int w = g_app->m_renderer->ScreenW();
	int h = g_app->m_renderer->ScreenH();

	float leftX, leftY, leftW, leftH;
	float fontLarge, fontMed, fontSmall;
	float buttonW, buttonH, gap;
	GetPosition_LeftBox(leftX, leftY, leftW, leftH );
	GetFontSizes( fontLarge, fontMed, fontSmall );
	GetButtonSizes( buttonW, buttonH, gap );

	float buttonX = leftX + (leftW-buttonW)/2;
	float yPos = leftY;
	float fontSize = fontMed;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_neworjoin_title" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH * 1.3f;


	m_xblaSessionType = -1;
#if defined(LAN_PLAY_ENABLED) || defined(WAN_PLAY_ENABLED)
	m_singlePlayer = false;
#else
	m_singlePlayer = true;
#endif
   
	m_settingUpCustomGame = false;

	button = new NewServerButton();
	button->SetShortProperties( "multiwinia_menu_hostgame", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_hostgame") );
	button->m_fontSize = fontSize;
	if( !g_app->HostGamePermitted() )
		button->m_disabled = true;
	RegisterButton( button );
	m_buttonOrder.PutData( button );	

	button = new JoinGameButton();
	button->SetShortProperties( "multiwinia_menu_joingame", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_joingame") );
	button->m_fontSize = fontSize;
	RegisterButton( button );
	m_buttonOrder.PutData( button );

	// Quick match for PC
    // Removed by Chris.  It doesn't work.

	//QuickMatchButton *qm = new QuickMatchButton();
	//qm->SetShortProperties( "quickmatch", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_quickmatch") );
	//qm->m_fontSize = fontSize;
	//RegisterButton( qm );
	//m_buttonOrder.PutData( qm );
    
	yPos = leftY + leftH - buttonH * 2;

    button = new BackPageButton( PageMain );
    button->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;
    RegisterButton( button );
    m_buttonOrder.PutData( button );
}


void GameMenuWindow::SetupGameSelectPage()
{
	if( !g_app->m_server )
	{
		HRESULT hr;
        if( m_singlePlayer )
        {
            hr = (HRESULT)g_app->StartSinglePlayerServer();
        }
        else
        {
            hr = (HRESULT)g_app->StartMultiPlayerServer();
        }
	}	

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    
    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float xPos = buttonX;
    float yPos = leftY;
    float fontSize = fontMed;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_gameselect_title" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH * 1.3f;

    //
    // Subtitle

    /*MapListTitlesButton *titles = new MapListTitlesButton();
    titles->SetShortProperties( "Titles", leftX+10, leftY+10+buttonH*1.6f, leftW-20, buttonH*0.5f, UnicodeString("Titles") );
    titles->m_fontSize = fontSmall * 0.75;
    titles->m_screenId = GameMenuWindow::PageGameSelect;
    RegisterButton( titles );*/
    

    //
    // One button for each game mode

    //if( m_singlePlayer )
    //{
    //    QuickStartButton *qsb = new QuickStartButton();
    //    qsb->SetShortProperties( "multiwinia_menu_quickstart", xPos, yPos+=buttonH, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_quickstart") );
    //    qsb->m_fontSize = fontSize;
    //    RegisterButton( qsb );
    //    m_buttonOrder.PutData( qsb );
    //    yPos+=buttonH;
    //    //buttonH *= 0.7f;
    //    //fontSize *= 0.7f;
    //}

    

    for( int i = 0; i < Multiwinia::s_gameBlueprints.Size(); ++i )
    {
#if defined(HIDE_INVALID_GAMETYPES)
        if( !g_app->IsGameModePermitted(i) ) continue;
#endif


        GameTypeButton *button = new GameTypeButton( Multiwinia::s_gameBlueprints[i]->GetName(), i );
        button->SetShortProperties( Multiwinia::s_gameBlueprints[i]->GetName(), xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE(Multiwinia::s_gameBlueprints[i]->GetName()) );
        button->m_fontSize = fontSize;
	    if( strcmp(button->m_name, "TankBattle") == 0 )
	    {
		    button->m_inactive = true;
	    }

	    if( !button->m_inactive )
	    {
		    // LEANDER : This hides tank battle from the menu until it is unlocked. 
		    // This would be the place to show some graphic about the tank and which PDLCs you have.
		    m_buttonOrder.PutData( button );
		    RegisterButton( button );
	    }

        if( !g_app->IsGameModePermitted(i) )
        {
            button->m_inactive = true;
        }

        /*if( IS_DEMO )
        {
            if( g_app->m_gameMenu->m_numDemoMaps[i] == 0 )
            {
                button->m_inactive = true;
            }
            else
            {
                m_buttonOrder.PutData( button );
            }
        }
        else
        {
            m_buttonOrder.PutData( button );
        }
        RegisterButton( button );*/        
    }

    //if( m_singlePlayer )
    //{
    //    yPos += buttonH;

    //    PrologueButton *pb = new PrologueButton( "multiwinia_menu_prologue" );
    //    pb->SetShortProperties("prologue", xPos, yPos+=(buttonH+gap), buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_prologue") );
    //    pb->m_fontSize = fontSize;
    //    pb->m_useEditorFont = true;
    //    RegisterButton( pb );
    //    m_buttonOrder.PutData( pb );

    ////    CampaignButton *cb = new CampaignButton( "icons/menu_campaign.bmp" );
    //    CampaignButton *cb = new CampaignButton( "multiwinia_menu_campaign" );
    //    cb->SetShortProperties( "campaign", xPos, yPos+=(buttonH+gap), buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_campaign") );
    //    cb->m_fontSize = fontSize;
    //    cb->m_useEditorFont = true;
    //    cb->m_inactive = true;
    //    RegisterButton( cb );
    //    //m_buttonOrder.PutData( cb);
    //}

    yPos+=buttonH+gap;

    int page = PageNewOrJoin;
    if( m_singlePlayer )
    {
        page = PageMain;
    }
    m_localSelectedLevel = -1;

    yPos = leftY + leftH - buttonH * 2;
    GameMenuButton *button = new FinishMultiwiniaButton(page);    
    button->SetShortProperties( "multiwinia_menu_back", xPos, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;    
    RegisterButton( button );
    m_buttonOrder.PutData( button );


    //
    // Status buttons on the right

    //m_clickGameType = -1;

    xPos = rightX + rightW * 0.1f;
    yPos = rightY + rightW * 0.1f;
    int thumbnailW = rightW * 0.8f;
    int thumbnailH = thumbnailW * 2/3.0f;
    
    //
    // Title

    /*yPos += buttonH * 0.5f;
    GameMenuTitleButton *titleR = new GameMenuTitleButton();
    titleR->SetShortProperties( "title", xPos, yPos, thumbnailW, buttonH*1.3f, "GAME" );
    titleR->m_fontSize = fontMed;
    RegisterButton( titleR );
    yPos += buttonH * 2;*/

    GameTypeImageButton *imageButton = new GameTypeImageButton();
    imageButton->SetShortProperties( "gametype", xPos, yPos, thumbnailW, thumbnailH, UnicodeString("gametype") );
    RegisterButton( imageButton );

    GameTypeInfoButton *info = new GameTypeInfoButton();
    info->SetShortProperties("gametypeinfo", xPos, yPos + thumbnailH + fontSmall, thumbnailW, (rightH * 0.85f) - thumbnailH, UnicodeString() );
    info->m_fontSize = fontSmall;
    RegisterButton( info );


    //
    // Preload big images to avoid laggy interface

    for( int i = 0; i < Multiwinia::s_gameBlueprints.Size(); ++i )
    {
        char fullFilename[256];
        sprintf( fullFilename, "mwhelp/mode_%s." TEXTURE_EXTENSION, Multiwinia::s_gameBlueprints[i]->m_name );
        strlwr( fullFilename );
        if( g_app->m_resource->DoesTextureExist(fullFilename) )
        {
            //AppDebugOut( "Preloading %s\n", fullFilename );
            g_app->m_resource->GetTexture( fullFilename, false, false );
        }
    }
}

void GameMenuWindow::SetupGameConnectingPage()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    
    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float xPos = buttonX;
    float yPos = leftY;
    float fontSize = fontMed;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_joining" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH * 1.3f;

    WaitingButton *waiting = new WaitingButton( "multiwinia_menu_connecting" );
    waiting->SetShortProperties( "connecting", xPos, yPos+=buttonH+gap, buttonW, buttonH, UnicodeString() );
    waiting->m_fontSize = fontSize;
    RegisterButton( waiting );

    int page = -1;
    if( m_currentPage == PageJoinGame ) page = PageJoinGame;
    else page = PageNewOrJoin;

    yPos = leftY + leftH - buttonH * 2;
    GameMenuButton *button = new FinishMultiwiniaButton(page);    
    button->SetShortProperties( "multiwinia_menu_back", xPos, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    button->m_fontSize = fontSize;    
    RegisterButton( button );
    m_buttonOrder.PutData( button );
}

void GameMenuWindow::SetupMultiplayerPage( int _gameType )
{
    if( _gameType == -1 ) return;
	int mapId = m_localSelectedLevel;
	if( mapId == -1 ) mapId = m_requestedMapId;
    if( mapId == -1 || (g_app->m_globalWorld->m_myTeamId == 255 &&
        !g_app->m_spectator ))
	{
		m_waitingForTeamID = true;
		m_waitingGameType = _gameType;
		return;
	}

    if( m_currentPage == PageMapSelect )
    {
        if( m_uiGameType != _gameType )
        {
            SetupNewPage( PageMapSelect );
            return;
        }
    }
    else if( m_currentPage != PageResearch &&
        m_currentPage != PageAdvancedOptions &&
		m_currentPage != PagePlayerOptions) 
    {
		m_uiGameType = _gameType;

		if( g_app->m_server )
			m_requestedMapId = 0;
    }

    g_app->m_atLobby = true;

    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[_gameType];

    if( m_currentPage == PagePlayerOptions &&
        m_teamName.Get().WcsLen() == 0 )
    {
        m_teamName.Set( LANGUAGEPHRASE("multiwina_playername_default") );
    }

        
    if( m_currentPage != PagePlayerOptions &&
        m_currentPage != PageAdvancedOptions )
    {
        if( _gameType == Multiwinia::GameTypeAssault || m_coopMode ) 
        {
            m_previousRequestedColour = m_currentRequestedColour;
            m_currentRequestedColour = -1;
        }
        else
        {
            int preferedColour = g_prefsManager->GetInt( "PlayerPreferedColour", -1 );
            if( preferedColour != -1 && 
                !g_app->m_multiwinia->IsColourTaken(preferedColour) )
            {
                m_currentRequestedColour = preferedColour;
            }
            else
            {
                m_currentRequestedColour = g_app->m_multiwinia->GetNextAvailableColour();
            }
        }
    }

    m_highlightedLevel = -1;
 
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float xPos = buttonX;
    float yPos = leftY + buttonH;
    float fontSize = fontSmall;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(blueprint->GetName()) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;


    //
    // Map thumbnail + info on right side

    float thumbnailW = rightW * 0.8f;
    float thumbnailH = thumbnailW * 0.75f;
    float thumbnailX = rightX + rightW * 0.1f;
    float thumbnailY = rightY + rightW * 0.1f;

    MapImageButton *mib = new MapImageButton();
    mib->SetShortProperties( "MapThumbnail", thumbnailX, thumbnailY, thumbnailW, thumbnailH, UnicodeString("MapThumbnail") );
    RegisterButton( mib );
    thumbnailY += thumbnailH + fontSmall * 0.5f;
    
    MapSelectModeButton *msmb = new MapSelectModeButton();
    msmb->SetShortProperties("Map", thumbnailX, thumbnailY, thumbnailW, buttonH*0.66f, " " );
    msmb->m_fontSize = fontSmall;
    RegisterButton( msmb );

    thumbnailY += buttonH * 1.0f + gap;

    float infoH = rightH - thumbnailH - (fontSmall * 0.5f) - 2.0f * (buttonH + gap) - gap;

    DarwiniaButton *info;
    float font = fontSmall;
    if( m_singlePlayer )
    {
        info = new MapInfoButton();
    }
    else
    {
        font *= 0.8f;
        info = new GameMenuChatButton();
    }
    info->SetShortProperties( "chat_button", thumbnailX, thumbnailY, thumbnailW, infoH, UnicodeString("Chat") );
    info->m_fontSize = font;
    RegisterButton( info );

    if( !m_singlePlayer )
    {
        ChatInputField *chatwindow = new ChatInputField();
        chatwindow->SetShortProperties( "chat_input", rightX + rightW * 0.1f, rightY + (rightH * 0.95f ), rightW * 0.8f, fontSmall * 1.2f, UnicodeString() );
        chatwindow->RegisterUnicodeString( m_currentChatMessage );
        RegisterButton( chatwindow );
    }



	if( g_app->m_server ) 
	{
		if( m_currentPage != PageResearch && 
			//m_currentPage != PageMapSelect &&
			m_currentPage != PageAdvancedOptions &&
			m_currentPage != PagePlayerOptions ) 
		{
			for( int i = 0; i < blueprint->m_params.Size(); ++i )
			{
				MultiwiniaGameParameter *param = blueprint->m_params[i];   
				m_params[i] = param->m_default;
			}
		}
	}

    
    //yPos += buttonH + gap;

    UnicodeString caption = LANGUAGEPHRASE("multiwinia_menu_ready");
    if( m_singlePlayer )
    {
        caption = LANGUAGEPHRASE("multiwinia_menu_play");
    }

    PlayGameButton *play = new PlayGameButton();
    play->SetShortProperties( "Play", xPos, yPos, buttonW, buttonH, caption );
    RegisterButton( play );
    play->m_fontSize = fontMed;
    m_buttonOrder.PutData( play );
    m_currentButton = m_buttonOrder.Size() - 1;

    yPos += buttonH + gap;

    if( g_app->m_server )
    {

#if !defined(SPECTATOR_ONLY)
        AdvancedOptionsModeButton *aomb = new AdvancedOptionsModeButton();
        aomb->SetShortProperties( "Advanced Options", xPos, yPos, buttonW, buttonH*0.66f, LANGUAGEPHRASE("multiwinia_menu_advanceoptions") );
        aomb->m_fontSize = fontSmall;
        RegisterButton( aomb );
        m_buttonOrder.PutData( aomb );
        yPos += buttonH * 0.66f + gap;

        GameMenuDropDown *menu = new GameMenuDropDown();
        menu->SetShortProperties( "multiwinia_menu_aimode", xPos, yPos, buttonW, buttonH*0.66f, LANGUAGEPHRASE("multiwinia_menu_aimode") );
        menu->AddOption( "dialog_easy_difficulty", Multiwinia::AITypeEasy );
        menu->AddOption( "multiwinia_menu_normal", Multiwinia::AITypeStandard );
        menu->AddOption( "multiwinia_menu_hard", Multiwinia::AITypeHard );
        RegisterButton( menu );
        menu->RegisterNetworkInt( &(m_aiMode) );
        menu->m_fontSize = fontSize;
        menu->m_noBackground = true;
        menu->m_cycleMode = true;
        m_buttonOrder.PutData( menu );        

        char authKey[256];
        Authentication_GetKey( authKey );
        bool demoKey = Authentication_IsDemoKey( authKey );

        if( demoKey )
        {
            aomb->m_disabled = true;
            aomb->m_inactive = true;
            menu->m_disabled = true;
            menu->m_inactive = true;
        }
#endif
    }
    else
    {
        yPos += buttonH + gap;
        yPos += buttonH * 0.66f + gap;
    }


    //
    // Game type image button

    float gameTypeW = buttonW * 0.45f;
    float gameTypeH = gameTypeW * 2/3.0f;
    float gameTypeX = leftX + leftW - gameTypeW - (leftW-buttonW)/2.0f;
    float gameTypeY = leftY + buttonH * 6;
    
    GameTypeImageButton *imageButton = new GameTypeImageButton();
    imageButton->SetShortProperties( "gametype", gameTypeX, gameTypeY, gameTypeW, gameTypeH, UnicodeString("gametype") );
    RegisterButton( imageButton );

    /*MapDetailsButton *mdb = new MapDetailsButton();
    mdb->SetShortProperties( "MapDetails", gameTypeX, gameTypeY, buttonW*0.5f, buttonH*0.5f, UnicodeString("MapDetails") );
    mdb->m_fontSize = fontSmall * 0.75f;
    RegisterButton( mdb );*/


    //
    // Back button

    BackPageButton *back = NULL;
    if( m_singlePlayer )
    {
        back = new FinishSinglePlayerButton( PageMapSelect );
    }
    else
    {
        if( g_app->m_server )
            back = new FinishMultiwiniaButton( PageGameSelect );
        else
			back = new FinishMultiwiniaButton( PageNewOrJoin );
    }
    yPos = leftY + leftH - buttonH * 2;
    back->SetShortProperties( "multiwinia_menu_back", xPos, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;  

	buttonX = leftX + (leftW-buttonW)/2.0f;
    yPos = leftY + buttonH * 6;
    fontSize = fontSmall;
    buttonH *= 0.75f;        
    buttonW *= 0.45f;

	int gameType = _gameType;

	AppReleaseAssert( 0 <= gameType && gameType < MAX_GAME_TYPES &&
	                  g_app->m_gameMenu->m_maps[gameType].ValidIndex( mapId ), 
	                  "Failed to get level due to invalid game type or map id (Game Type = %d, Map ID = %d), map name (may be inaccurate) = '%s'",
	                  gameType, mapId, g_app->m_requestedMap );
	MapData *md = g_app->m_gameMenu->m_maps[gameType][mapId];

	for( int i = 0; i < md->m_numPlayers; ++i )
	{
		GamertagButton *gtb = new GamertagButton();
        char tagname[64];
        sprintf( tagname, "tag%d", i );
		gtb->SetShortProperties( tagname, buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("teamtype_2") ); // TeamTypeCPU
		gtb->m_fontSize = fontSize;
		gtb->m_teamId = i;
		RegisterButton(gtb);
		m_buttonOrder.PutData( gtb );

        if( !m_singlePlayer && i > 0 && g_app->m_server )
        {
            char kickname[64];
            sprintf( kickname, "kick%d", i );
            KickPlayerButton *kick = new KickPlayerButton();
            kick->SetShortProperties(kickname, buttonX + buttonW * 1.05f, yPos, buttonH, buttonH, LANGUAGEPHRASE("teamtype_2") ); // TeamTypeCPU
            kick->m_teamId = i;
            RegisterButton( kick );
            m_buttonOrder.PutData( kick );
        }

		yPos+=buttonH*1.5f;
	}


#ifdef TESTBED
	// TODO: The following code tests out spectator mode - uncomment it to get that mode back

    GetButtonSizes( buttonW, buttonH, gap );
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );


	// Add another row of buttons
	buttonX = leftX + (leftW-buttonW)/2.0f;
	buttonX += 280.0f;
    yPos = leftY + buttonH * 6;
    fontSize = fontSmall;
    buttonH *= 0.75f;        
    buttonW *= 0.45f;

	gameType = _gameType;
	mapId = m_localSelectedLevel;
	if( mapId == -1 ) mapId = m_requestedMapId;

	AppReleaseAssert( 0 <= gameType && gameType < MAX_GAME_TYPES &&
	                  g_app->m_gameMenu->m_maps[gameType].ValidIndex( mapId ), 
	                  "Failed to get level due to invalid game type or map id (Game Type = %d, Map ID = %d), map name (may be inaccurate) = '%s'",
	                  gameType, mapId, g_app->m_requestedMap );
	md = g_app->m_gameMenu->m_maps[gameType][mapId];

	for( int i = 0; i < md->m_numPlayers; ++i )
	{
		CSpectatorButton *pSpectator = new CSpectatorButton();
		pSpectator->SetShortProperties( "tag", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("teamtype_2") ); // TeamTypeCPU
		pSpectator->m_fontSize = fontSize;
		pSpectator->m_teamId = i;
		RegisterButton(pSpectator);
		m_buttonOrder.PutData( pSpectator );

		yPos+=buttonH*1.5f;
	}



	//m_buttonOrder.PutData( msmb );

#endif

    m_buttonOrder.PutData( back );

}

void GameMenuWindow::SetupResearchPage()
{
    int x, y, gap;
    GetDefaultPositions( &x, &y, &gap );
    int w = 600 * float( x / 200.0f );
    int h = (w / 12.0f );
    float fontSize = 0.9f * h;
    if( h - fontSize > gap )
    {
        h = fontSize;
    }

    y-=gap;

    for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
    {
        char *name = GlobalResearch::GetTypeName( i );//Translated( i );
		m_researchLevel[i].SetBounds( 0, 4 );
        CreateMenuControl( name, InputField::TypeInt, &(m_researchLevel[i]), y+=gap, 1, NULL, x, w, fontSize );
        GetButton(name)->SetCaption( name );
        GetButton(name)->m_h = h;
        m_buttonOrder.PutData( GetButton( name ) );
    }

    BackPageButton *back = new BackPageButton( PageGameSetup );
    back->SetShortProperties( "multiwinia_menu_back", x, y+=gap * 1.5f, w, h, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontSize * 1.2f;
    m_buttonOrder.PutData( back );
}

void GameMenuWindow::PreloadThumbnails()
{
	AppDebugOut("GameMenuWindow - Starting to preload thumbnails.\n");
    int gameType = 
		m_settingUpCustomGame ? m_customGameType : m_gameType;

    if( gameType < 0 || gameType >= MAX_GAME_TYPES ) 
		return;

    DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[gameType];

    for( int i = 0; i < maps.Size(); ++i )
    {
        if( !maps.ValidIndex( i ) ) continue;

        char thumbfile[256];
        char fullFilename[256];
        strcpy( thumbfile, maps[i]->m_fileName );
        thumbfile[ strlen( thumbfile ) - 4 ] = '\0';
        strcat( thumbfile, "." TEXTURE_EXTENSION );
        sprintf( fullFilename, "thumbnails/%s", thumbfile );
        strlwr( fullFilename );

		if( g_app->m_resource->DoesTextureExist( fullFilename ) )
		{
			g_app->m_resource->GetTexture( fullFilename, false, false );
		}
    }
}

void GameMenuWindow::SetupMapSelectPage()
{
    AppDebugOut("Begin Caching Thumbnails - %f\n", GetHighResTime() );
	g_loadingScreen->m_workQueue->Add( &GameMenuWindow::PreloadThumbnails, this );
	AppDebugOut("Starting rendering the loading screen - %f\n", GetHighResTime() );
    g_loadingScreen->Render();
    AppDebugOut("End Caching Thumbnails - %f\n", GetHighResTime() );
    //g_app->m_renderer->StartMenuTransition();

    if( m_clickGameType != -1 && m_gameType != m_clickGameType && !m_settingUpCustomGame ) return;
    int gameType = m_gameType;
    if( m_settingUpCustomGame ) gameType = m_customGameType;
    if( gameType < 0 || gameType >= MAX_GAME_TYPES ) return;
    m_uiGameType = gameType;
    m_localSelectedLevel = -1;
    m_clickGameType = -1;

    MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[gameType];

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    
    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float xPos = buttonX;
    float yPos = leftY;
    float fontSize = fontSmall;

    float thumbnailW = rightW * 0.8f;
    float thumbnailH = thumbnailW * 0.75f;
    float thumbnailX = rightX + rightW * 0.1f;
    float thumbnailY = rightY + rightW * 0.1f;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE(blueprint->GetName()) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH * 1.3f;

    //
    // Subtitle

    MapListTitlesButton *titles = new MapListTitlesButton();
    titles->SetShortProperties( "Titles", xPos, leftY+10+buttonH*1.8f, buttonW, buttonH*0.7f, UnicodeString("Titles") );
    titles->m_fontSize = fontSmall;
    titles->m_screenId = GameMenuWindow::PageMapSelect;
    RegisterButton( titles );
    yPos += buttonH * 0.35f;


	MapImageButton *mib = new MapImageButton();
	mib->SetShortProperties( "MapThumbnail", thumbnailX, thumbnailY, thumbnailW, thumbnailH, UnicodeString("MapThumbnail") );
	RegisterButton( mib );

	MapInfoButton *info = new MapInfoButton();
	info->SetShortProperties( "MapInfo", thumbnailX, thumbnailY + thumbnailH + fontSmall, thumbnailW, (rightH * 0.85f)-thumbnailH, UnicodeString("MapInfo") );
	info->m_fontSize = fontSmall * 1.1f;
	RegisterButton( info );	

	DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[gameType];

	if( m_settingUpCustomGame )
	{
		AnyLevelSelectButton *lsb = new AnyLevelSelectButton;
		const char *name = "multiwinia_level_any";

		lsb->SetProperties( name, xPos, yPos+=buttonH * 0.6f, buttonW, buttonH * 0.6f, LANGUAGEPHRASE(name) );
		lsb->m_fontSize = fontSize;

		RegisterButton( lsb );
		m_buttonOrder.PutData( lsb );
	}

	float newButtonX = leftX + ((buttonX-leftX)/2);
	float newButtonW = buttonW;     // + ( (leftW - buttonW)/2 );

	for( int i = 0; i < maps.Size(); ++i )
	{
#ifdef PERMITTED_MAPS
		if( !g_app->IsMapPermitted( gameType, maps[i]->m_mapId ) ) continue;
#endif

		LevelSelectButton *lsb = new LevelSelectButton();

		char name[512];
		if( maps[i]->m_levelName )
		{
			strcpy( name, maps[i]->m_levelName );
		}
		else
		{
			strcpy( name, maps[i]->m_fileName );
		}

        float levelSButtonH = buttonH * 0.7f;
        float levelSButtonG = (buttonH + gap) * 0.7f;
		lsb->SetProperties( name, xPos, yPos+=levelSButtonG, newButtonW, levelSButtonH, LANGUAGEPHRASE(name) );
		lsb->m_levelIndex = i;
		lsb->m_fontSize = fontSmall;
		lsb->m_inactive = !g_app->IsMapPermitted( gameType, maps[i]->m_mapId );

		RegisterButton( lsb );
		m_buttonOrder.PutData( lsb );
	}

    yPos = leftY + leftH - buttonH * 2;

    int page = PageGameSelect;
    if( m_singlePlayer )
    {
        page = PageGameSelect;
    } 
	else if( m_requestedMapId == -1 ) 
    {
        page = PageGameSelect;
    }
    BackPageButton *back = new BackPageButton( page );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );

}

void GameMenuWindow::SetupHelpMenuPage()
{
    //SetTitle( LANGUAGEPHRASE( "multiwinia_menu_help" ) );

    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    //
    // Status buttons on the right

    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

    float xPos = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY + buttonH;
    float fontSize = fontMed;

   /* for( int i = 1; i < NumHelpPages; ++i )
    {
        char name[512];
        sprintf( name, "multiwinia_helppage_%d", i );
        HelpSelectButton *hsb = new HelpSelectButton();
        hsb->SetShortProperties( name, xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE(name) );
        hsb->m_helpType = i;
        hsb->m_fontSize = fontSize;
        RegisterButton( hsb );
        m_buttonOrder.PutData( hsb );
    }*/

    BackPageButton *back = new BackPageButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontSize;
    m_buttonOrder.PutData( back );
    
}

void GameMenuWindow::SetupHelpPage()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    int imageW = 600;
    int imageH = 400;

    int xPos = (w / 2.0f) - ( imageW / 2.0f );
    int yPos = (h / 2.0f) - ( imageH / 2.0f );

    float buttonH = h * 0.05f;
    float buttonW = w * 0.1f;
    float fontSize = buttonH * 0.8f;
    float gap = buttonH * 0.5f;

    HelpImageButton *hib = new HelpImageButton();
    hib->SetShortProperties( "helpimage", xPos, yPos, imageW, imageH, UnicodeString("helpimage") );
    RegisterButton( hib );

    PrevHelpButton *phb = new PrevHelpButton();
    phb->SetShortProperties( "multiwinia_menu_back", xPos, yPos + imageH + fontSize, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    phb->m_fontSize = fontSize;
    RegisterButton( phb );
    m_buttonOrder.PutData( phb );

    NextHelpButton *nhb = new NextHelpButton();
    nhb->SetShortProperties( "dialog_next", xPos + imageW - buttonW, yPos += imageH + fontSize, buttonW, buttonH, LANGUAGEPHRASE("dialog_next") );
    nhb->m_fontSize = fontSize;
    RegisterButton( nhb );
    m_buttonOrder.PutData( nhb );
}

void GameMenuWindow::SetupAdvancedOptionsPage()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );

   

    float xPos = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontSmall;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_advanceoptions" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;


    buttonH *= 0.65f;

	if( m_gameType != -1 )
	{
		MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[m_gameType];
		for( int i = 0; i < blueprint->m_params.Size(); ++i )
		{
			MultiwiniaGameParameter *param = blueprint->m_params[i];
			char name[256];
			sprintf( name, "Param%d", i );
			if( param->m_change == -1 )
			{
				m_params[i].Unbind();
				GameMenuCheckBox *checkBox = new GameMenuCheckBox();
				checkBox->SetShortProperties( param->GetStringId(), xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE(param->GetStringId()) );
				checkBox->RegisterNetworkInt( &m_params[i] );
				checkBox->m_fontSize = fontSize;
				RegisterButton( checkBox );
				m_buttonOrder.PutData( checkBox );
			}
			else if( param->m_change == 0 )
			{
				m_params[i].Unbind();
				GameMenuDropDown *menu = new GameMenuDropDown();
				menu->SetShortProperties( param->GetStringId(), xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE(param->m_name) );
				for( int p = param->m_min; p <= param->m_max; ++p )
				{
					char thisOption[256];
					sprintf( thisOption, "multiwinia_option_%d_%d_%d", (int) m_gameType, i, p );
					menu->AddOption( thisOption, p );
				}
				RegisterButton( menu );
				menu->RegisterNetworkInt( &(m_params[i]) );
				menu->m_fontSize = fontSize;
				menu->m_cycleMode = true;
				m_buttonOrder.PutData( menu );
			}
			else
			{
				m_params[i].SetBounds( param->m_min, param->m_max );
				CreateMenuControl( param->GetStringId(), InputField::TypeInt, &(m_params[i]), yPos+=buttonH+gap,
								   param->m_change, NULL, xPos, buttonW, fontSize );
				UnicodeString temp;
				param->GetNameTranslated(temp);
				EclButton *b = GetButton(param->GetStringId());
				b->SetCaption( temp );
				b->m_h = buttonH;
				m_buttonOrder.PutData( b );
			}
		}

		//yPos+=buttonH + gap;

		AppReleaseAssert( 0 <= m_gameType && m_gameType < MAX_GAME_TYPES &&
						  g_app->m_gameMenu->m_maps[m_gameType].ValidIndex( m_requestedMapId ), 
						  "Failed to get level due to invalid game type or map id (Game Type = %d, Map ID = %d), map name (may be inaccurate) = '%s'",
						  (int) m_gameType, (int) m_requestedMapId, g_app->m_requestedMap );

#ifdef INCLUDE_CRATES_ADVANCED
        GameMenuCheckBox *crates = new GameMenuCheckBox();
        crates->SetShortProperties( "multiwinia_param_basiccrates", xPos, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_param_basiccrates") );
        crates->m_fontSize = fontSize;
        RegisterButton(crates);
        crates->RegisterNetworkInt( &(m_basicCrates) );
        m_buttonOrder.PutData( crates );
#endif

		if( g_app->m_gameMenu->m_maps[m_gameType][m_requestedMapId]->m_coop && !IS_DEMO)
		{
			GameMenuCheckBox *menu = new GameMenuCheckBox();
			menu->SetShortProperties( "multiwinia_menu_coop", xPos, yPos+=buttonH+gap, buttonW, buttonH ,LANGUAGEPHRASE("multiwinia_menu_coop"));
			RegisterButton( menu );
			menu->RegisterNetworkInt( &(m_coopMode) );
			menu->m_fontSize = fontSize;
			m_buttonOrder.PutData( menu );
		}
	}


    yPos += buttonH + gap;

    if( g_app->m_server && !m_singlePlayer )
    {
        CreateMenuControl( "multiwinia_menu_serverpassword", InputField::TypeUnicodeString, &m_serverPassword, yPos+=buttonH+gap, 0.0f, NULL, xPos, buttonW, fontSize );
        GameMenuInputField *pass = (GameMenuInputField *)GetButton("multiwinia_menu_serverpassword");
	    pass->m_h = buttonH;
	    m_buttonOrder.PutData(pass);
    }

    buttonH /= 0.65f;

    yPos = leftY + leftH - buttonH * 2;

    BackPageButton *back = new BackPageButton( PageGameSetup );
    back->SetShortProperties( "multiwinia_menu_back", xPos, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );

    xPos = rightX + (rightW * 0.05f);
    yPos = rightY + (rightH * 0.05f);
    buttonW = rightW * 0.9f;
    buttonH = rightH * 0.9f;

    OptionDetailsButton *details = new OptionDetailsButton();
    details->SetShortProperties( "option_details", xPos, yPos, buttonW, buttonH, UnicodeString() );
    details->m_fontSize = fontSmall;
    RegisterButton( details );
}

void GameMenuWindow::SetupLeaderBoardSelectPage()
{
	return;
}


void GameMenuWindow::SetupLeaderboardPage()
{
    return;
}

void GameMenuWindow::SetupSPDemoPage()
{
    float leftX, leftY, leftW, leftH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    float rightX, rightY, rightW, rightH;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );
    GetPosition_RightBox(rightX, rightY, rightW, rightH);

    float xPos = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY;
    float fontSize = fontMed;

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_singleplayer_title" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH * 1.3f;

    MultiwiniaVersusCpuButton *vcpu = new MultiwiniaVersusCpuButton();
    vcpu->SetShortProperties( "versus", xPos, yPos+=(buttonH+gap), buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_versuscpu") );
    vcpu->m_fontSize = fontSize;
    RegisterButton( vcpu );
    m_buttonOrder.PutData( vcpu );
    yPos += buttonH + gap;

#ifdef INCLUDEGAMEMODE_PROLOGUE
    PrologueButton *pb = new PrologueButton( "Prologue" );
    pb->SetShortProperties("prologue", xPos, yPos+=(buttonH+gap), buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_prologue")  );
    pb->m_fontSize = fontSize;
    //pb->m_centered = true;
    RegisterButton( pb );
    m_buttonOrder.PutData( pb );
#endif


#ifdef INCLUDEGAMEMODE_CAMPAIGN
//    CampaignButton *cb = new CampaignButton( "icons/menu_campaign.bmp" );
    CampaignButton *cb = new CampaignButton( "Campaign" );
    cb->SetShortProperties( "campaign", xPos, yPos+=(buttonH+gap), buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_campaign") );
    cb->m_fontSize = fontSize;
    //cb->m_centered = true;
    cb->m_inactive = true;
    RegisterButton( cb );
    if( !IS_DEMO )
    {
        m_buttonOrder.PutData( cb );
    }
#endif

    yPos = leftY + leftH - buttonH * 2;

    FinishMultiwiniaButton *back = new FinishMultiwiniaButton( PageMain );
    back->SetShortProperties( "multiwinia_menu_back", xPos, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    back->m_fontSize = fontSize;
    //back->m_centered = true;
    RegisterButton( back );
    m_buttonOrder.PutData( back );

    //m_clickGameType = -1;

    xPos = rightX + rightW * 0.1f;
    yPos = rightY + rightW * 0.1f;
    int thumbnailW = rightW * 0.8f;
    int thumbnailH = thumbnailW * 2/3.0f;
    
    GameTypeImageButton *imageButton = new GameTypeImageButton();
    imageButton->SetShortProperties( "gametype", xPos, yPos, thumbnailW, thumbnailH, UnicodeString("gametype") );
    RegisterButton( imageButton );

    GameTypeInfoButton *info = new GameTypeInfoButton();
    info->SetShortProperties("gametypeinfo", xPos, yPos + thumbnailH + fontSmall, thumbnailW, (rightH * 0.85f) - thumbnailH, UnicodeString() );
    info->m_fontSize = fontSmall * 1.2f;
    RegisterButton( info );


    //
    // Preload big images to avoid laggy interface

    for( int i = 0; i < Multiwinia::s_gameBlueprints.Size(); ++i )
    {
        char fullFilename[256];
        sprintf( fullFilename, "mwhelp/mode_%s." TEXTURE_EXTENSION, Multiwinia::s_gameBlueprints[i]->m_name );
        strlwr( fullFilename );
        if( g_app->m_resource->DoesTextureExist(fullFilename) )
        {
            //AppDebugOut( "Preloading %s\n", fullFilename );
            g_app->m_resource->GetTexture( fullFilename, false, false );
        }
    }
}

void GameMenuWindow::SetupBuyMePage()
{
    int w = g_app->m_renderer->ScreenW();
    int h = g_app->m_renderer->ScreenH();

    float leftX, leftY, leftW, leftH;
    float rightX, rightY, rightW, rightH;
    float fontLarge, fontMed, fontSmall;
    float buttonW, buttonH, gap;
    GetPosition_LeftBox(leftX, leftY, leftW, leftH );
    GetPosition_RightBox(rightX, rightY, rightW, rightH );
    GetFontSizes( fontLarge, fontMed, fontSmall );
    GetButtonSizes( buttonW, buttonH, gap );    

    float buttonX = leftX + (leftW-buttonW)/2.0f;
    float yPos = leftY + buttonH;
    float fontSize = fontMed;

    //SetTitle( LANGUAGEPHRASE("multiwinia_menu_buyfullgame") );

    //
    // Title

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("launchpad_buyme_1" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*0.5f;

    yPos += buttonH * 1;

    TextButton *pricebutton = new TextButton("multiwinia_buy_prices");
    pricebutton->SetShortProperties( "info", buttonX, yPos, buttonW, buttonH, UnicodeString() );
    pricebutton->m_fontSize = fontSmall * 1.3f;
    pricebutton->m_shadow = true;    
    pricebutton->m_centered = true;
    RegisterButton( pricebutton );

    yPos += buttonH * 2;

    float imageH = (40.0f / 300.0f) * buttonW * 0.6f;

    ImageButton *price = new ImageButton( "textures/prices.bmp" );
    price->SetShortProperties( "image", buttonX+buttonW*0.2f, yPos+buttonH, buttonW*0.6f, imageH, UnicodeString() );
    price->m_border = false;
    RegisterButton( price );

    yPos += buttonH * 2.5f;
    UnlockFullGameButton *unlock = new UnlockFullGameButton();
    unlock->SetShortProperties( "launchpad_button_buynow", buttonX+buttonW*0.2f, yPos, buttonW*0.6f, buttonH*2, LANGUAGEPHRASE("launchpad_button_buynow") );
    unlock->m_centered = true;
    strcpy( unlock->m_url, g_app->GetBuyNowURL() );

    RegisterButton( unlock );
    unlock->m_fontSize = fontMed * 1.2f;
    m_buttonOrder.PutData( unlock );

    yPos = leftY + leftH - buttonH * 3;

    BackPageButton *back = new BackPageButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos+=buttonH, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontSize;
    m_buttonOrder.PutData( back );
    
    buttonX = rightX + rightW * 0.1f;
    yPos = rightY + rightH * 0.1f;

    imageH = (149.0f / 394.0f) * rightW * 0.8f;

    ImageButton *button = new ImageButton( "textures/multiwinia.bmp" );
    button->SetShortProperties( "image", buttonX, yPos, rightW*0.8f, imageH, UnicodeString() );
    RegisterButton( button );

    TextButton *infobutton = new TextButton("multiwinia_buynow");
    infobutton->SetShortProperties( "info", buttonX, yPos+=imageH+fontSmall, rightW, rightH, UnicodeString() );
    infobutton->m_fontSize = fontSmall;
    infobutton->m_shadow = true;
    RegisterButton( infobutton );


    yPos -=  buttonH * 3;
}


bool GameMenuWindow::CheckNewAuthKeyStatus()
{
	// This function only does a basic check, and requests the metaserver
	// to check the key (so that we get the its true status back at a later stage)

    char authKey[256];
    Authentication_GetKey( authKey );
    int authStatus = Authentication_SimpleKeyCheck( authKey, METASERVER_GAMETYPE_MULTIWINIA );

    if( authStatus == AuthenticationAccepted )
    {
		if( !Authentication_IsDemoKey( authKey ) )
		{
			// Trigger a check of the auth key
			Authentication_RequestStatus( authKey, METASERVER_GAMETYPE_MULTIWINIA );
		}

		return true;
    }
	
	return false;
}

void GameMenuWindow::PasteFromClipboard()
{
    bool gotClipboardText = false;
    char textClipboard[AUTHENTICATION_KEYLEN + 1];
    *textClipboard = '\0';

    // Read key into clipboard
#ifdef TARGET_MSVC	
    bool opened = OpenClipboard(NULL);
    if( opened )
    {
        bool textAvailable = IsClipboardFormatAvailable(CF_TEXT);
        if( textAvailable )
        {
            HANDLE clipTextHandle = GetClipboardData(CF_TEXT);
            if( clipTextHandle )
            {
                char *text = (char *) GlobalLock(clipTextHandle); 
                if(clipTextHandle) 
                { 
                    strncpy( textClipboard, text, AUTHENTICATION_KEYLEN-1 );
                    textClipboard[AUTHENTICATION_KEYLEN - 1] = '\0';
                    gotClipboardText = true;
                    
                    GlobalUnlock(text); 
                }
            }
        }
        CloseClipboard();
    }
#elif TARGET_OS_MACOSX
    PasteboardRef clipboard = NULL;
    ItemCount numItems = 0;
    CFDataRef clipboardData = NULL;
    CFStringRef clipboardString;
    
    PasteboardCreate(kPasteboardClipboard, &clipboard);
    if ( clipboard )
    {
        PasteboardGetItemCount(clipboard, &numItems);
        
        // Use the first item, if it exists. Multiple items are only for drag-and-drop, AFAIK
        if ( numItems > 0 )
        {
            PasteboardItemID firstItem;
            PasteboardGetItemIdentifier( clipboard, 1, &firstItem );
            PasteboardCopyItemFlavorData( clipboard, firstItem,
                                          CFSTR("public.utf16-plain-text"), &clipboardData);
            if ( clipboardData )
            {
                clipboardString = CFStringCreateWithBytes(NULL, CFDataGetBytePtr(clipboardData),
                                                          CFDataGetLength(clipboardData),
                                                          kCFStringEncodingUnicode, false);
                
                // Convert to Latin 1 encoding, and copy as much as will fit
                memset(textClipboard, 0, sizeof(textClipboard));
                CFStringGetBytes(clipboardString, CFRangeMake(0, CFStringGetLength(clipboardString)),
                                 kCFStringEncodingWindowsLatin1, 0, false,
                                 (UInt8 *)textClipboard, AUTHENTICATION_KEYLEN-1, NULL);
                gotClipboardText = true;
                
                CFRelease(clipboardString);
                CFRelease(clipboardData);
            }
        }
    }
    
    CFRelease( clipboard );
#endif // platform specific

    // Cross-platform code, once we've gotten the clipboard contents into textClipboard
    //
    if ( gotClipboardText && strlen( textClipboard ) > 0 )
    {
        strncpy( m_authKey, textClipboard, AUTHENTICATION_KEYLEN );
        m_authKey[ sizeof(m_authKey) - 1 ] = '\0';

        strupr( m_authKey );

		char path[512];
		strcpy( path, g_app->GetProfileDirectory() );
		char fullFileName[512];
		sprintf( fullFileName, "%sauthkey", path );

        Authentication_SetKey( m_authKey );
        Authentication_SaveKey( m_authKey, fullFileName );

		CheckNewAuthKeyStatus();
    }
}


void GameMenuWindow::SetupAuthKeyPage()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_enterauthkey" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

	AuthInputField *auth = new AuthInputField();
    auth->SetShortProperties( "auth_input", buttonX, yPos+=buttonH+gap, buttonW, buttonH/2.0f, UnicodeString() );
    auth->RegisterString( m_authKey, AUTHENTICATION_KEYLEN+1 );
    auth->m_fontSize = buttonW / (AUTHENTICATION_KEYLEN-1);

    RegisterButton( auth );

    BeginTextEdit("auth_input");

#if defined(TARGET_PC_HARDWARECOMPAT) || defined(TARGET_BETATEST_GROUP) || defined(TARGET_ASSAULT_STRESSTEST)
    yPos = leftY + leftH - buttonH * 4;
#else
    yPos = leftY + leftH - buttonH * 5;
#endif

    // Only show the paste button on platforms with a paste implementation
#if defined(TARGET_MSVC) || defined(TARGET_OS_MACOSX)
    PasteKeyButton *paste = new PasteKeyButton();
    paste->SetProperties( "paste", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_paste"), LANGUAGEPHRASE("multiwinia_menu_paste_tooltip") );
    RegisterButton( paste );
    paste->m_fontSize = fontMed;
    m_buttonOrder.PutData( paste );
#endif

#if defined(TARGET_PC_HARDWARECOMPAT) || defined(TARGET_BETATEST_GROUP) || defined(TARGET_ASSAULT_STRESSTEST)
    float buttonWBottom = (buttonW-gap)/2.0f;
#else
    float buttonWBottom = buttonW;

	yPos = leftY + leftH - buttonH * 3;

    PlayDemoAuthkeyButton *playDemo = new PlayDemoAuthkeyButton();
    playDemo->SetShortProperties( "multiwinia_menu_play_demo", buttonX, yPos, buttonWBottom, buttonH, LANGUAGEPHRASE("multiwinia_menu_play_demo") );
    RegisterButton( playDemo );
    playDemo->m_fontSize = fontMed;
    m_buttonOrder.PutData( playDemo );
#endif

    yPos = leftY + leftH - buttonH * 2;

    EnterAuthkeyButton *done = new EnterAuthkeyButton();

    done->SetShortProperties( "multiwinia_menu_done", buttonX, yPos, buttonWBottom, buttonH, LANGUAGEPHRASE("multiwinia_menu_done") );
    RegisterButton( done );
    done->m_fontSize = fontMed;
    m_buttonOrder.PutData( done );

#if defined(TARGET_PC_HARDWARECOMPAT) || defined(TARGET_BETATEST_GROUP) || defined(TARGET_ASSAULT_STRESSTEST)
    QuitAuthkeyButton *quit = new QuitAuthkeyButton();
    quit->SetShortProperties( "multiwinia_menu_quit_auth_key", buttonX + buttonWBottom + gap, yPos, buttonWBottom, buttonH, LANGUAGEPHRASE("multiwinia_menu_quit") );
    RegisterButton( quit );
    quit->m_fontSize = fontMed;
    m_buttonOrder.PutData( quit );
#endif

    /*BackPageButton *back = new BackPageButton( GameMenuWindow::PageMain );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );*/
}


void GameMenuWindow::SetupPlayerOptionsPage()
{
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
    float fontSize = fontSmall;

	buttonH *= 0.65f;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_playeroptions" ) );
    title->m_fontSize = fontSize;
    RegisterButton( title );
    yPos += buttonH*1.3f;

	CreateMenuControl( "multiwinia_player_name", InputField::TypeUnicodeString, &m_teamName, yPos += buttonH + gap, -1, NULL, buttonX, buttonW, fontSize );
	GameMenuInputField *name = (GameMenuInputField *)GetButton("multiwinia_player_name");
	name->m_h = buttonH;
	m_buttonOrder.PutData(name);

	yPos += buttonH;

	FancyColourButton *button = NULL;
    if( m_gameType == Multiwinia::GameTypeAssault )
    {
        button = new FancyColourButton(-1);
        button->SetShortProperties("multiwinia_random", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_option_0_7_0"));
        button->m_fontSize = fontSize;
        RegisterButton(button);
        m_buttonOrder.PutData(button);

        button = new FancyColourButton(ASSAULT_ATTACKER_COLOURID);
        button->SetShortProperties("multiwinia_attackers", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_attackers"));
        button->m_fontSize = fontSize;
        RegisterButton(button);
        m_buttonOrder.PutData(button);

        button = new FancyColourButton(ASSAULT_DEFENDER_COLOURID);
        button->SetShortProperties("multiwinia_defenders", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_defenders"));
        button->m_fontSize = fontSize;
        RegisterButton(button);
        m_buttonOrder.PutData(button);
    }
    else
    {
	    for( int i = 0; i < MULTIWINIA_NUM_TEAMCOLOURS; ++i )
	    {
		    button = new FancyColourButton(i);
		    button->SetShortProperties( GetColourNameChar(i), buttonX, yPos+=buttonH+gap, buttonW, buttonH, GetColourName(i) );
		    button->m_fontSize = fontSize;
		    RegisterButton(button);
		    m_buttonOrder.PutData(button);
	    }
    }

	buttonH /= 0.65f;
	yPos = leftY + leftH - buttonH * 2;

    ApplyNameButton *back = new ApplyNameButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    back->m_fontSize = fontMed;
    RegisterButton( back );
    m_buttonOrder.PutData( back );


}

void GameMenuWindow::SetupFiltersPage()
{
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
    float fontSize = fontSmall;

    yPos += buttonH * 0.5f;
    GameMenuTitleButton *title = new GameMenuTitleButton();
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_filters" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    buttonH *= 0.65f;

    GameMenuDropDown *dropdown = new GameMenuDropDown(false);
    dropdown->SetShortProperties( "multiwinia_gametype", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_gametype") );
    dropdown->AddOption( "multiwinia_gametype_any", -1 );
#if defined(HIDE_INVALID_GAMETYPES)
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeSkirmish ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_domination", Multiwinia::GameTypeSkirmish );
#if defined(HIDE_INVALID_GAMETYPES)
    }
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeKingOfTheHill ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_kingofthehill", Multiwinia::GameTypeKingOfTheHill );
#if defined(HIDE_INVALID_GAMETYPES)
    }
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeCaptureTheStatue ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_capturethestatue", Multiwinia::GameTypeCaptureTheStatue );
#if defined(HIDE_INVALID_GAMETYPES)
    }
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeAssault ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_assault", Multiwinia::GameTypeAssault );
#if defined(HIDE_INVALID_GAMETYPES)
    }
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeRocketRiot ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_rocketriot", Multiwinia::GameTypeRocketRiot );
#if defined(HIDE_INVALID_GAMETYPES)
    }
    if( g_app->IsGameModePermitted( Multiwinia::GameTypeBlitzkreig ) )
    {
#endif
    dropdown->AddOption( "multiwinia_gametype_blitzkrieg", Multiwinia::GameTypeBlitzkreig );
#if defined(HIDE_INVALID_GAMETYPES)
    }
#endif
    dropdown->RegisterInt( &m_gameTypeFilter );
    dropdown->m_fontSize = fontSize;
    RegisterButton( dropdown );
    m_buttonOrder.PutData( dropdown );

    GameMenuCheckBox *checkbox = new GameMenuCheckBox();
    checkbox->SetShortProperties( "check_box", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_showdemogames") );
    checkbox->RegisterBool( &m_showDemosFilter );
    checkbox->m_fontSize = fontSize;
    RegisterButton( checkbox );
    m_buttonOrder.PutData( checkbox );

    checkbox = new GameMenuCheckBox();
    checkbox->SetShortProperties( "check_box2", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_showincompatible") );
    checkbox->RegisterBool( &m_showIncompatibleGamesFilter);
    checkbox->m_fontSize = fontSize;
    RegisterButton( checkbox );
    m_buttonOrder.PutData( checkbox );

	checkbox = new GameMenuCheckBox();
    checkbox->SetShortProperties( "check_box3", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_showfull") );
    checkbox->RegisterBool( &m_showFullGamesFilter);
    checkbox->m_fontSize = fontSize;
    RegisterButton( checkbox );
    m_buttonOrder.PutData( checkbox );

    checkbox = new GameMenuCheckBox();
    checkbox->SetShortProperties( "check_box5", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_showpasswords") );
    checkbox->RegisterBool( &m_showPasswordedFilter );
    checkbox->m_fontSize = fontSize;
    RegisterButton( checkbox );
    m_buttonOrder.PutData( checkbox );

    buttonH /= 0.65f;
    yPos = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new SaveFiltersButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );
}

void GameMenuWindow::SetupUpdatePage()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_update_available" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

#ifdef TARGET_OS_MACOSX
	RunSparkleButton *updater = new RunSparkleButton();
	updater->SetShortProperties( "multiwinia_menu_autopatch", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_autopatch") );
#else
	RunAutoPatcherButton *updater = new RunAutoPatcherButton();
	// FIXME: localize
	updater->SetShortProperties( "multiwinia_menu_autopatch", buttonX, yPos+=buttonH+gap, buttonW, buttonH, "Update" );
#endif
	updater->m_fontSize = fontSize;
	RegisterButton( updater );
	m_buttonOrder.PutData( updater );

	VisitWebsiteButton *website = new VisitWebsiteButton();
	website->SetShortProperties( "dialog_visitwebsite", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("dialog_visitwebsite") );
	website->m_fontSize = fontSize;
	RegisterButton( website );
	m_buttonOrder.PutData( website );

	yPos = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new BackPageButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );
}

static void ReadMultiwiniaBetaTesters( UnicodeString &_betaTesters )
{
	UnicodeString newLine = UnicodeString(" \n");
    TextReader *reader = g_app->m_resource->GetTextReader( "multiwinia_betatesters.txt" );

    while( reader && reader->ReadLine() )
    {
        UnicodeString line = reader->GetRestOfUnicodeLine();
        _betaTesters = _betaTesters + line + newLine;
    }
    delete reader;
}

static void ReadCredits( UnicodeString &_credits )
{
	UnicodeString betaTestersMacro = "*BETATESTERS";
	UnicodeString betaTesters;
	ReadMultiwiniaBetaTesters( betaTesters );
	
	UnicodeString newLine = UnicodeString(" \n");
	
	for( int counter = 1; ; counter++ ) 
	{
		char key[64];
		sprintf(key, "multiwinia_credits_%d", counter);

		if( !ISLANGUAGEPHRASE( key ) )
			break;

		const UnicodeString &line = LANGUAGEPHRASE( key );
		
		if( line.StrCmp( betaTestersMacro ) == 0 )
			_credits = _credits + betaTesters + newLine;
		else
			_credits = _credits + line + newLine;
	}
}


void GameMenuWindow::SetupCreditsPage()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("dialog_credits" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

    float textH = leftH - buttonH * 1.5f;
    textH -= buttonH * 1.3f;
    textH -= buttonH + gap * 2.0f;

    ScrollingTextButton *text = new ScrollingTextButton();
    text->SetShortProperties( "credits_box", buttonX, yPos, buttonW, textH, UnicodeString() );
    text->m_fontSize = fontSmall;
    RegisterButton( text );

	UnicodeString credits;
	ReadCredits( credits );

    text->m_string = credits;

    yPos = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new BackPageButton( m_currentPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );
}

void GameMenuWindow::SetupTutorialPage()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_menu_tutorial_short" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*1.3f;

#ifdef INCLUDE_TUTORIAL
    TutorialButton *qsb = new TutorialButton();
    qsb->SetShortProperties( "multiwinia_menu_tutorial1", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_tutorial1") );
    qsb->m_fontSize = fontSize;
    qsb->m_tutorialType = App::MultiwiniaTutorial1;
#if defined(TARGET_PC_HARDWARECOMPAT) 
    qsb->m_inactive = true;
#endif
    RegisterButton( qsb );
    m_buttonOrder.PutData( qsb );

    if( g_app->GetMapID( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_2P_1 ) == -1 || !g_app->IsMapPermitted( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_2P_1 ) )
    {
        qsb->m_inactive = true;
    }

    qsb = new TutorialButton();
    qsb->SetShortProperties( "multiwinia_menu_tutorial2", buttonX, yPos+=buttonH+gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_tutorial2") );
    qsb->m_fontSize = fontSize;
    qsb->m_tutorialType = App::MultiwiniaTutorial2;
#if defined(TARGET_PC_HARDWARECOMPAT) 
    qsb->m_inactive = true;
#endif
    RegisterButton( qsb );
    m_buttonOrder.PutData( qsb );

    if( g_app->GetMapID( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_3P_1 ) == -1 || !g_app->IsMapPermitted( Multiwinia::GameTypeKingOfTheHill, MAPID_MP_KOTH_3P_1 ) )
    {
        qsb->m_inactive = true;
    }
#endif

    yPos = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new BackPageButton( PageMain );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );

	if( g_app->m_multiwiniaHelp ) g_app->m_multiwiniaHelp->m_currentTutorialHelp = -1;
}

void GameMenuWindow::SetupErrorPage()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("error" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*2;

    TextButton *text = new TextButton( m_errorMessage );
    text->SetShortProperties( "error_text", buttonX, yPos, buttonW, buttonH, UnicodeString() );
    text->m_fontSize = fontSmall;
    text->m_centered = true;
    RegisterButton( text );


    yPos = leftY + leftH - (buttonH * 2);

    BackPageButton *back = new BackPageButton( m_errorBackPage );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );

    m_errorBackPage = -1;
}

void GameMenuWindow::SetupPageEnterPassword()
{
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
    title->SetShortProperties( "title", leftX+10, leftY+10, leftW-20, buttonH*1.5f, LANGUAGEPHRASE("multiwinia_enterpassword" ) );
    title->m_fontSize = fontMed;
    RegisterButton( title );
    yPos += buttonH*2;

    TextButton *tb = new TextButton("multiwinia_passwordrequired");
    tb->SetShortProperties( "passrequired_text", buttonX, yPos, buttonW, buttonH, UnicodeString() );
    tb->m_fontSize = fontSmall;
    tb->m_centered = true;
    tb->m_shadow = true;
    RegisterButton( tb );

    buttonH *= 0.65f;
    yPos += buttonH;

    CreateMenuControl( "multiwinia_enterpassword", InputField::TypeUnicodeString, &m_serverPassword, yPos+=buttonH+gap, 0.0, NULL, buttonX, buttonW, fontSmall );
    GameMenuInputField *pass = (GameMenuInputField *)GetButton("multiwinia_enterpassword");
    pass->m_h = buttonH;
    pass->m_renderStyle2 = true;
    m_buttonOrder.PutData(pass);
    BeginTextEdit("multiwinia_enterpassword");

    buttonH /= 0.65f;

    yPos = leftY + leftH - (buttonH * 3);

    ServerConnectButton *connect = new ServerConnectButton();
    connect->SetShortProperties( "multiwinia_menu_connect", buttonX, yPos, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_connect") );
    RegisterButton( connect );
    connect->m_fontSize = fontMed;
    m_buttonOrder.PutData( connect );

    BackPageButton *back = new BackPageButton( PageJoinGame );
    back->SetShortProperties( "multiwinia_menu_back", buttonX, yPos+=buttonH+ gap, buttonW, buttonH, LANGUAGEPHRASE("multiwinia_menu_back") );
    RegisterButton( back );
    back->m_fontSize = fontMed;
    m_buttonOrder.PutData( back );
}

GameMenuInputField *GameMenuWindow::CreateMenuControl( 
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

    if( dataType != InputField::TypeString && 
		dataType != InputField::TypeUnicodeString )
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


void GameMenuWindow::GetDefaultPositions(int *_x, int *_y, int *_gap)
{
    float w = g_app->m_renderer->ScreenW();
    float h = g_app->m_renderer->ScreenH();

    *_x = float((w / 1152) * 200.0f);
    switch( m_newPage )
    {
        case PageMain:     
        case PageDarwinia:      *_y = float((h / 864.0f ) * 200.0f); *_gap = *_y;            break;
		case PageNewOrJoin:
		case PageJoinGame:
		case PageGameConnecting:							  
        case PageGameSelect:    *_y = float((h / 864.0f ) * 175.0f); *_gap = *_y / 1.5f;     break;			
        case PageGameSetup:
        case PageResearch:      *_y = float((h / 864.0f ) * 70.0f); *_gap = (h / 864 ) * 60; break;
    }

    *_x = min( *_x, 200 );
}