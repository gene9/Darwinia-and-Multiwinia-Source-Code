#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"
#include "lib/resource.h"
#include "lib/input/input.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/math/random_number.h"
#include "lib/preferences.h"
#include "lib/profiler.h"

#include "markersystem.h"
#include "multiwinia.h"
#include "app.h"
#include "renderer.h"
#include "globals.h"
#include "team.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "global_world.h"
#include "multiwiniahelp.h"
#include "global_world.h"
#include "achievement_tracker.h"
#include "unit.h"
#include "game_menu.h"
#include "rocket_status_panel.h"
#include "ui/MapData.h"

#include "network/clienttoserver.h"
#include "network/servertoclient.h"
#include "network/iframe.h"
#include "network/syncdiff.h"

#include "sound/soundsystem.h"

#include "worldobject/multiwiniazone.h"
#include "worldobject/rocket.h"
#include "worldobject/statue.h"
#include "worldobject/darwinian.h"
#include "worldobject/trunkport.h"
#include "worldobject/pulsebomb.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/restrictionzone.h"

#include "UI/GameMenuWindow.h"

DArray<MultiwiniaGameBlueprint *> Multiwinia::s_gameBlueprints;

static int s_numMultiwiniaZones = 0;

Multiwinia::Multiwinia()
:   m_timer(0.0),
    m_gameType(-1),
    m_winner(-1),
    m_statueSpawner(-1.0),
    m_reinforcementTimer(0.0),
    m_turretTimer(0),
    m_initialised(false),
    m_startTime(0.0),
    m_victoryTimer(-1.0),
    m_aiType(AITypeStandard),
    m_pulseTimer(-1.0),
    m_numRotations(0),
    m_pulseDetonated(false),
    m_resetLevel(false),
    m_oneMinuteWarning(false),
    m_positionTimer(0.0),
    m_coopMode(false),
    m_gracePeriodTimer(GRACE_TIME),
    m_assaultSingleRound(true),
    m_suddenDeathActive(false),
    m_unlimitedTime(false),
    m_basicCratesOnly(false),
    m_firstReinforcements(true),
    m_reinforcementCountdown(0),
    m_rocketStatusPanel(NULL)
{
	for( int i = 0; i < NUM_TEAMS; i++ )
	{
		m_teamsStatus[i] = 0;
	}

	LoadBlueprints();

	Reset();
}

Multiwinia::~Multiwinia()
{
	s_gameBlueprints.EmptyAndDelete();
}

static RGBAColour defaultColour[] = {
	RGBAColour( 100, 255, 100 ), 	// Normally Green AI
    RGBAColour( 255, 50, 50 ),	// Normally Virii
	RGBAColour( 255, 255, 50  ),	// Normally Player
	RGBAColour( 120, 180, 255),		// Blue
    RGBAColour( 255, 150, 0  ),	    // Orange
    RGBAColour( 250, 250, 250),		// Grey
	RGBAColour(  50, 255,  50),		// Green
};

static RGBAColour teamColours[] = {
    RGBAColour( 100, 255, 100 ),// Normally Green AI
    RGBAColour( 255, 50 , 50 ),	// Normally Virii
	RGBAColour( 255, 255, 50 ),	// Normally Player
	RGBAColour( 120, 180, 255),	// Blue
    RGBAColour( 255, 150, 0  ),	// Orange
    RGBAColour( 150, 0, 255 ), // Purple
    RGBAColour( 70, 200, 200 ), // cyan
    RGBAColour( 255, 150, 255 ), // pink
    RGBAColour( 10,10,10 ),
    RGBAColour( 200,200,200 )
};

static RGBAColour groupColours1[] = {
    RGBAColour(0,150,20),
    RGBAColour(150,0,20),
    RGBAColour(150,150,20),
	RGBAColour(20,0,150),
    RGBAColour(150,90,0),
    RGBAColour(180, 120, 255),
    RGBAColour(0,140,140),
    RGBAColour(255,90,255),
    RGBAColour( 10,10,10 ),
    RGBAColour( 200,200,200 )
};

static RGBAColour groupColours2[] = {
    RGBAColour(90,200,100),
    RGBAColour(255,80,80),
    RGBAColour(200,200,90),
	RGBAColour(90,90,200),
    RGBAColour(200,150,60),
    RGBAColour(100,60, 180),
    RGBAColour(110,210,210),
    RGBAColour(255, 190, 255), 
    RGBAColour( 10,10,10 ),
    RGBAColour( 200,200,20 )
};

void Multiwinia::Reset()
{
	for (int i = 0; i < NUM_TEAMS; i++) 
	{
		m_teams[i].m_teamType = TeamTypeUnused;
		m_teams[i].m_score = 0;
		m_teams[i].m_prevScore = 0;
		m_teams[i].m_colour = defaultColour[i];
        m_teams[i].m_rocketId = -1;
        m_teams[i].m_colourId = -1;
		m_teams[i].m_teamName = UnicodeString("");
        m_teams[i].m_ready = false;

		delete m_teams[i].m_iframe;
		m_teams[i].m_iframe = NULL;
		m_teams[i].m_syncDifferences.EmptyAndDelete();

        m_eliminated[i] = false;
        m_tankDeaths[i] = 0;
        m_timeRecords[i] = -1.0;

        m_currentPositions[i] = -1;

        m_eliminationTimer[i] = 30;
        m_eliminationBonusTimer[i] = 5;
        m_numEliminations[i] = 0;
        m_timeEliminated[i] = 0.0;
	}

    m_statueSpawner = -1.0;
    m_reinforcementTimer = 0.0;
    m_reinforcementCountdown = 0;
    m_startTime = 0.0;
    m_victoryTimer = -1.0;
    m_turretTimer = 0;
    m_pulseTimer = -1.0;
    m_numRotations = 0;
    m_pulseDetonated = false;
    m_resetLevel = false;
    m_winner = -1;
    m_coopMode = false;
    m_oneMinuteWarning = false;
    m_gracePeriodTimer = GRACE_TIME;
    m_gameType = -1;
    m_initialised = false;
	m_timer = 0.0;
    m_suddenDeathActive = false;
    m_unlimitedTime = false;
    m_basicCratesOnly = false;
    m_firstReinforcements = true;
    m_aiType = AITypeStandard;
    m_positionTimer = 0.0;

    for( int i = 0; i < NUM_TEAMS; i++ )
    {
        m_teamsStatus[i] = 0;
    }

    m_assaultObjectives.Empty();
    PulseBomb::s_defaultTime = 0.0;
    delete m_rocketStatusPanel;
    m_rocketStatusPanel = NULL;
}

void Multiwinia::ResetLocation()
{
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        m_teams[i].m_colourId = -1;
    }
    m_numRotations++;
    g_app->m_location->RotatePositions();
    int positions[NUM_TEAMS];
    memcpy( positions, g_app->m_location->m_teamPosition, sizeof(int) * NUM_TEAMS );
    positions[g_app->m_location->GetMonsterTeamId()] = -1;
    positions[g_app->m_location->GetFuturewinianTeamId()] = -1;
    int slice = g_app->m_location->GetLastProcessedSlice();

    m_teams[g_app->m_location->GetMonsterTeamId()].m_teamType = TeamTypeUnused;
    m_teams[g_app->m_location->GetFuturewinianTeamId()].m_teamType = TeamTypeUnused;

    delete g_app->m_location;
    g_app->m_location = new Location();
    memcpy( g_app->m_location->m_teamPosition, positions, sizeof(int) * NUM_TEAMS );
    g_app->m_location->Init( g_app->m_requestedMission, g_app->m_requestedMap );
    g_app->m_location->SetLastProcessedSlice( slice );

    g_app->m_renderer->StartFadeIn(1.0f);

    g_gameTimer.Reset();

    m_resetLevel = false;
    m_reinforcementTimer = 0.0f;
    m_reinforcementCountdown = 0;
    m_gracePeriodTimer = GRACE_TIME;
}

void Multiwinia::InitialiseTeam ( unsigned char _teamId, unsigned char _teamType, int _clientId )
{
	// One of these asserts fails for testbed games, but it seems harmless to ignore it
#ifndef TESTBED_ENABLED 
	AppDebugAssert( _teamId < NUM_TEAMS );
    AppDebugAssert( m_teams[_teamId].m_teamType == TeamTypeUnused );
#endif

    LobbyTeam *team = &m_teams[_teamId];

	AppDebugOut("Zeroing achievement mask\n");
	team->m_achievementMask = 0;

	team->m_clientId = _clientId;
	team->m_teamId = _teamId;
	team->m_teamType = _teamType;
    team->m_demoPlayer = false;

    if( !m_coopMode && m_gameType != GameTypeAssault )
    {
        team->m_colourId = _teamId;
    }

    AppDebugOut( "CLIENT : New team created, id %d, type %d\n", _teamId, _teamType );

    if( _teamType == TeamTypeLocalPlayer )
    {
        AppDebugOut( "CLIENT : Assigned team %d\n", _teamId );
        g_app->m_globalWorld->m_myTeamId = _teamId;
//		g_target->SetMousePos(g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH());
//		g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
    }

    if( _teamType != TeamTypeCPU )
    {
        team->m_disconnected = false;
    }

	// If the game is already in progress then initialise the team
    if (g_app->m_location && g_app->m_location->m_teamsInitialised) 
		g_app->m_location->InitialiseTeam( _teamId );

    /*if( g_app->m_atMainMenu )
    {
        GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow( "multiwinia_mainmenu_title" );
        if( gmw )
        {
            UnicodeString msg("PLAYERJOIN");
            gmw->NewChatMessage( msg, _clientId );
        }
    }*/

}


void Multiwinia::LoadBlueprints()
{
	//
    // Load game option blueprints
    
#ifdef TESTBED_ENABLED
	const char *filename = "multiwinia-testbed.txt";
#else
	const char *filename = "multiwinia.txt";
#endif

    TextReader *in = g_app->m_resource->GetTextReader( filename );
	AppReleaseAssert(in && in->IsOpen(), "Couldn't load multiwinia.txt");

    while( in->ReadLine() )
    {
        if( !in->TokenAvailable() ) continue;
        char *effect = in->GetNextToken();
        AppDebugAssert( stricmp( effect, "GAMETYPE" ) == 0 );

        MultiwiniaGameBlueprint *blueprint = new MultiwiniaGameBlueprint( in->GetNextToken() );
        s_gameBlueprints.PutData( blueprint );

        in->ReadLine();
        char *param = in->GetNextToken();
        while( stricmp( param, "END" ) != 0 )
        {
            MultiwiniaGameParameter *sb = new MultiwiniaGameParameter( param );
            blueprint->m_params.PutData( sb );

            sb->m_min       = iv_atof( in->GetNextToken() );
            sb->m_max       = iv_atof( in->GetNextToken() );
            sb->m_default   = iv_atof( in->GetNextToken() );
            sb->m_change    = atoi( in->GetNextToken() );
            
            in->ReadLine();
            param = in->GetNextToken();
        }
    }

	delete in;
}

bool Multiwinia::GameInLobby() 
{
	return g_app->m_atMainMenu;
}


bool Multiwinia::GameRunning()
{
    return( !GameInLobby() && m_gameType != -1 && g_app->m_location );
}


void Multiwinia::SetGameOptions( int _gameType, int *_params )
{
    m_gameType = _gameType;

    AppReleaseAssert( s_gameBlueprints.ValidIndex(m_gameType), 
                            "Invalid Gametype %d\n", m_gameType );

    int numParams = s_gameBlueprints[m_gameType]->m_params.Size();
    memcpy( m_params, _params, numParams * sizeof(int) );

	GameOptionsChanged();
}

bool Multiwinia::ValidGameOption( int _optionIndex )
{
	return 
		s_gameBlueprints.ValidIndex( m_gameType ) &&
		s_gameBlueprints[m_gameType]->m_params.ValidIndex( _optionIndex );
}

void Multiwinia::SetGameOption( int _optionIndex, int _value )
{
	AppReleaseAssert( ValidGameOption( _optionIndex ), "Invalid game option %d for game type: %d\n", _optionIndex, m_gameType );
	m_params[_optionIndex] = _value;

	GameOptionsChanged();
}


void Multiwinia::GameOptionsChanged()
{
    int timeLimit = GetGameOption( "TimeLimit" );    
    m_timeRemaining = -1;
    if( timeLimit > 0 ) m_timeRemaining = timeLimit * 60;
}


int Multiwinia::GetGameOption( char *_name )
{
    MultiwiniaGameBlueprint *blueprint = s_gameBlueprints[m_gameType];
    for( int i = 0; i < blueprint->m_params.Size(); ++i )
    {
        if( stricmp( blueprint->m_params[i]->m_name, _name ) == 0 )
        {
            return m_params[i];
        }
    }

    return -1;
}


void Multiwinia::SetGameResearch( int *_research )
{
    memcpy( g_app->m_globalWorld->m_research->m_researchLevel, _research, 
            GlobalResearch::NumResearchItems * sizeof(int) );
    
    memset( g_app->m_globalWorld->m_research->m_researchProgress, 0,
            GlobalResearch::NumResearchItems * sizeof(int) );
}


void Multiwinia::AwardPoints( int _teamId, int _points )
{
    if( !GameOver() )
    {
        m_teams[_teamId].m_score += _points;
    }
}


void Multiwinia::AdvanceSkirmish()
{
#ifndef MULTIWINIA_DEMOONLY
#ifdef INCLUDEGAMEMODE_DOMINATION

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0 )
    {        
        if( m_timeRemaining > 0 ) m_timeRemaining--;
        m_timer = 1.0;
		BackupScores();        
    
        int scoreMode = GetGameOption( "ScoreMode" );
        if( scoreMode == 0 ) ZeroScores();

        // ScoreMode 0 : Final Population
        // ScoreMode 1 : Population Per Second


        int numEliminated = 0;
        int totalTeams = 0;
        int notEliminated = -1;

        int highScore = 0;
        int winningTeam = -1;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
            bool specialTeam = ( g_app->m_location->m_teams[t]->m_monsterTeam || g_app->m_location->m_teams[t]->m_futurewinianTeam );
            //if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
            //if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;

            if( t == g_app->m_location->GetFuturewinianTeamId() ||
                t == g_app->m_location->GetMonsterTeamId() )
            {
                // this check has to be 1, not 0, because both special teams will always have an AI entity 
                if( g_app->m_location->m_teams[t]->NumEntities(255) <= 1 ) continue;
                if( SpawnPoint::s_numSpawnPoints[ t + 1 ] == 0 ) continue;
            }

            totalTeams++;
            if( m_eliminated[t] ) 
            {
                numEliminated++;
                continue;
            }

            notEliminated = t;

            m_teams[t].m_score += SpawnPoint::s_numSpawnPoints[t+1];
            if( GetNetworkTime() > m_startTime + 30.0 )
            {
                if( SpawnPoint::s_numSpawnPoints[t+1] == 0 && !specialTeam )
                {
                    if( m_eliminationTimer[t] == 30 )
                    {
                        if( t == g_app->m_globalWorld->m_myTeamId )
                        {
                            g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_skirmish_elimination_30") );
                        }
                        else
                        {
                            UnicodeString msg = LANGUAGEPHRASE("multiwinia_other_elimination");
                            msg.ReplaceStringFlag( L'T', g_app->m_location->m_teams[t]->GetTeamName() );
                            g_app->m_location->SetCurrentMessage( msg );
                        }
                    }
                    m_eliminationTimer[t] --;
                    if( m_eliminationTimer[t] <= 0 )
                    {
                        EliminateTeam(t);
                        Team *team = g_app->m_location->m_teams[t];
                        team->Eliminate();
                    }
                }
                else
                {
                    m_eliminationTimer[t] = 30;
                }
            }

            if( m_teams[t].m_score == highScore )
            {
                highScore = m_teams[t].m_score;
                winningTeam = MULTIWINIA_DRAW_TEAM;
            }
            else if( m_teams[t].m_score > highScore )
            {
                highScore = m_teams[t].m_score;
                winningTeam = t;
            }
        }

        if( m_suddenDeathActive )
        {
            if( winningTeam != MULTIWINIA_DRAW_TEAM )
            {
                DeclareWinner( winningTeam );
                return;
            }
        }
        else if( m_timeRemaining <= 0 && !m_unlimitedTime )
        {
            if( winningTeam == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
            {
                m_suddenDeathActive = true;
                g_app->m_location->SetCurrentMessage(LANGUAGEPHRASE("multiwinia_suddendeath"));

                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( m_teams[t].m_teamType == TeamTypeUnused ) continue;

                    if( m_teams[t].m_score != highScore )
                    {
                        g_app->m_location->m_teams[t]->Eliminate();
                    }
                }
            }
            else
            {
                DeclareWinner ( winningTeam );
            }
            return;
        }

        if( m_coopMode )
        {
            bool noWinner = false;
            for( int i = 0; i < totalTeams; ++i )
            {
                if( i == g_app->m_location->GetFuturewinianTeamId() ||
                    i == g_app->m_location->GetMonsterTeamId() )
                {
                    if( g_app->m_location->m_teams[i]->NumEntities(255) <= 1 ) continue;
                }

                if( !m_eliminated[i] && !g_app->m_location->IsFriend( i, notEliminated )  )
                {
                    noWinner = true;
                    break;
                }
            }

            if( !noWinner )
            {
                DeclareWinner( winningTeam );
            }
        }
        else if( totalTeams - 1 == numEliminated )
        {
            if( notEliminated != -1 )
            {
                DeclareWinner( notEliminated );
                if( notEliminated == g_app->m_globalWorld->m_myTeamId )
                {
                    g_app->GiveAchievement( App::DarwiniaAchievementDominator );
                }
            }
        }
    }

#endif
#endif
}


void Multiwinia::AdvanceKingOfTheHill()
{
#ifdef INCLUDEGAMEMODE_KOTH

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {        
        if( m_timeRemaining > 0 ) m_timeRemaining--;
        m_timer = 1.0f;
		BackupScores();        
    
        int scoreMode = GetGameOption( "ScoreMode" );
        if( scoreMode == 2 ) 
		{
			ZeroScores();
		}

        // ScoreMode 0 : Zones controlled per second
        // ScoreMode 1 : Zone Population per second
        // ScoreMode 2 : Final Zone population
        // ScoreMode 3 : Total Zone control

        s_numMultiwiniaZones = 0;
        int numZonesOwned[NUM_TEAMS];
        memset( numZonesOwned, 0, NUM_TEAMS * sizeof(int) );

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {   
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == Building::TypeMultiwiniaZone )
                {
                    MultiwiniaZone *zone = (MultiwiniaZone *) building;
                    if( scoreMode == 1 || scoreMode == 2 )
                    {
                        for( int t = 0; t < NUM_TEAMS; ++t )
                        {
                            //if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
                            //if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;
                            m_teams[t].m_score += zone->m_teamCount[t];
                        }
                    }

                    ++s_numMultiwiniaZones;
                    if( zone->m_id.GetTeamId() != 255 ) 
                    {
                        numZonesOwned[zone->m_id.GetTeamId()]++;
                    }
                }
            }
        }

		int teamId = g_app->m_globalWorld->m_myTeamId;
		if( teamId != 255 && numZonesOwned[teamId] > 0 )
		{
			g_app->m_multiwiniaHelp->NotifyCapturedKothZone( numZonesOwned[teamId] );
		}

        int highestScore = 0;
        int winningTeamId = -1;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
			if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
            //if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
            //if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;
            bool specialTeam = ( g_app->m_location->m_teams[t]->m_monsterTeam || g_app->m_location->m_teams[t]->m_futurewinianTeam );
            
            if( scoreMode == 0 )
            {
                m_teams[t].m_score += numZonesOwned[t];
            }
            if( scoreMode == 3 )
            {
                if( numZonesOwned[t] == s_numMultiwiniaZones )
                {
                    ++m_teams[t].m_score;
                }
            }

            //if( !specialTeam )
            {
                if( m_teams[t].m_score == highestScore )
                {
                    highestScore = m_teams[t].m_score;
                    winningTeamId = MULTIWINIA_DRAW_TEAM;
                }
                else if( m_teams[t].m_score > highestScore )
                {
                    highestScore = m_teams[t].m_score;
                    winningTeamId = t;
                }
            }
        }

        int myTeamId = g_app->m_globalWorld->m_myTeamId;
        if( myTeamId != 255 &&
            numZonesOwned[myTeamId] > 0 )
        {
            g_app->m_multiwiniaHelp->NotifyCapturedKothZone( numZonesOwned[myTeamId] );
        }

        if( m_timeRemaining <= 0 )
        {
            if( winningTeamId == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
            {
                m_timeRemaining = 60;
                m_suddenDeathActive = true;
                g_app->m_location->SetCurrentMessage(LANGUAGEPHRASE("multiwinia_suddendeath"));
                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
                    //if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
                   // if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;

                    if( m_teams[t].m_score != highestScore )
                    {
                        g_app->m_location->m_teams[t]->Eliminate();
                    }
                }
            }
            else
            {
                DeclareWinner( winningTeamId );

				AppDebugOut("Declared winner as %d\n", winningTeamId);
                if( winningTeamId == g_app->m_globalWorld->m_myTeamId )
                {
					AppDebugOut("Winning team id (%d) is my team id (%)\n", winningTeamId, g_app->m_globalWorld->m_myTeamId);
                    int secondHighest = 0;
                    for( int i = 0; i < NUM_TEAMS; ++i )
                    {
                        if( m_teams[i].m_teamType == TeamTypeUnused ) continue;
                       // if( i == g_app->m_location->GetFuturewinianTeamId() ) continue;
                       // if( i == g_app->m_location->GetMonsterTeamId() ) continue;

                        if( m_teams[i].m_score > secondHighest &&
                            m_teams[i].m_score < highestScore )
                        {
                            secondHighest = m_teams[i].m_score;
                        }                
                    }

					AppDebugOut("Highest score is %d, secondHighest is %d\n", highestScore, secondHighest);

                    if( highestScore > secondHighest * 1.5f )
                    {
						AppDebugOut("Giving achievement!\n");
                        g_app->GiveAchievement( App::DarwiniaAchievementHailToTheKing );
                    }
                }
            }
        }

        //
        // Check to see if there are enough Zones spawned
        // Spawn more if required

        //int numZonesRequired = GetGameOption( "NumberOfZones" );
        //if( numZonesRequired > s_numMultiwiniaZones )
        //{
        //    MultiwiniaZone zoneTemplate;
        //    zoneTemplate.m_size = GetGameOption("ZoneSize");
        //    for(int i = 0; i < 10; ++i) 
        //    {
        //        float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        //        float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        //        zoneTemplate.m_pos.Set( FRAND(worldSizeX), 0.0f, FRAND(worldSizeZ) );
        //        zoneTemplate.m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(zoneTemplate.m_pos.x, zoneTemplate.m_pos.z);
        //        if( zoneTemplate.m_pos.y > 20 )
        //        {
        //            float nearestZone = FLT_MAX;
        //            for( int b = 0; b < g_app->m_location->m_buildings.Size(); ++b )
        //            {
        //                if( g_app->m_location->m_buildings.ValidIndex(b) )
        //                {
        //                    Building *building = g_app->m_location->m_buildings[b];
        //                    if( building->m_type == Building::TypeMultiwiniaZone &&
        //                        (building->m_pos - zoneTemplate.m_pos).Mag() < nearestZone )
        //                    {
        //                        nearestZone = (building->m_pos - zoneTemplate.m_pos).Mag();
        //                    }
        //                }
        //            }
        //            if( nearestZone > zoneTemplate.m_size * 2 )
        //            {
        //                MultiwiniaZone *zone = (MultiwiniaZone *) Building::CreateBuilding( Building::TypeMultiwiniaZone );
        //                g_app->m_location->m_buildings.PutData( zone );
        //                zone->Initialise((Building *)&zoneTemplate);
        //                int id = g_app->m_globalWorld->GenerateBuildingId();
        //                zone->m_id.SetUnitId( UNIT_BUILDINGS );
        //                zone->m_id.SetUniqueId( id );   
        //                zone->m_life = GetGameOption( "ZoneMoveFrequency" );
        //                if( zone->m_life == 0.0f ) zone->m_life = -1.0f;
        //                break;
        //            }
        //        }
        //    }
        //}
    }

#endif
}


void Multiwinia::AdvanceChess()
{
    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {        
        if( m_timeRemaining > 0 ) m_timeRemaining--;
        m_timer = 1.0f;

		BackupScores();
    }
}

void Multiwinia::AdvanceShaman()
{
    if( !m_initialised )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            m_teams[i].m_score = 200;
            m_teams[i].m_prevScore = 200;
        }
        m_initialised = true;
    }

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {        
        m_timer = 1.0f;

		BackupScores();
    }

    /*m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {
        m_timer = 1.0f;
        if( m_timeRemaining > 0 ) m_timeRemaining--;
		BackupScores();

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            Building *b = g_app->m_location->GetBuilding(i);
            if( b && b->m_type == Building::TypePortal )
            {
                if( b->m_id.GetTeamId() != 255 )
                {
                    if( g_app->m_location->m_teams[b->m_id.GetTeamId()]->m_monsterTeam ) continue;
					++m_teams[b->m_id.GetTeamId()].m_score;
                }
            }
        }
    }*/
}


void Multiwinia::AdvanceRocketRiot()
{
#ifndef MULTIWINIA_DEMOONLY
#ifdef INCLUDEGAMEMODE_ROCKET

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {
        m_timer = 1.0f;
        if( m_timeRemaining > 0 ) m_timeRemaining--;
        BackupScores();


        //
        // Look for rockets if required

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            Team *team = g_app->m_location->m_teams[t];
            LobbyTeam *lobbyTeam = &m_teams[t];
            
            if( team->m_monsterTeam ) continue;            
            if( team->m_futurewinianTeam ) continue;

            if( lobbyTeam->m_rocketId == -1 )
            {            
                for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                {
                    if( g_app->m_location->m_buildings.ValidIndex(i) )
                    {
                        Building *building = g_app->m_location->m_buildings[i];

                        if( building &&
                            building->m_type == Building::TypeEscapeRocket &&
                            building->m_id.GetTeamId() == t )
                        {
                            lobbyTeam->m_rocketId = building->m_id.GetUniqueId();
                        }
                    }
                }
            }
        }


        //
        // Update scores

        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            Team *team = g_app->m_location->m_teams[t];
            LobbyTeam *lobbyTeam = &m_teams[t];

            if( team->m_monsterTeam ) continue;            
            if( team->m_futurewinianTeam ) continue;

            lobbyTeam->m_score = 0;

            EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

            if( rocket && rocket->m_type == Building::TypeEscapeRocket )
            {
                double groundHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( rocket->m_pos.x, rocket->m_pos.z );

                lobbyTeam->m_score = rocket->m_fuel;
                lobbyTeam->m_score += rocket->m_passengers;
                lobbyTeam->m_score += ( rocket->m_pos.y - groundHeight );

                if( rocket->m_state == EscapeRocket::StateFlight &&
                    rocket->m_pos.y > 600 )
                {
                    DeclareWinner( t );

                    if( t == g_app->m_globalWorld->m_myTeamId && rocket->m_damage <= 0.01 )
                    {
                        bool noWin = false;
                        for( int i = 0; i < NUM_TEAMS; ++i )
                        {
                            if( g_app->m_location->m_teams[i]->m_monsterTeam ) continue;            
                            if( g_app->m_location->m_teams[i]->m_futurewinianTeam ) continue;
                            if( m_teams[i].m_teamType == TeamTypeUnused ) continue;
							if( i == g_app->m_globalWorld->m_myTeamId ) continue;

                            EscapeRocket *r = (EscapeRocket *)g_app->m_location->GetBuilding( m_teams[i].m_rocketId );
                            if( r && r->m_type == Building::TypeEscapeRocket )
                            {
                                if( r->m_fuel > 50.0f ) 
                                {
                                    noWin = true;
                                    break;
                                }
                            }
                        }
                        if( !noWin )
                        {
                            g_app->GiveAchievement( App::DarwiniaAchievementUncladSkies );
                        }
                    }
                    break;
                }
            }
        }
    }


    AdvanceTrunkPorts();
#endif
#endif
}

void Multiwinia::AdvanceTrunkPorts()
{
#ifndef MULTIWINIA_DEMOONLY
    m_reinforcementTimer -= SERVER_ADVANCE_PERIOD;
    if( m_reinforcementTimer <= 0.0 )
    {
        m_reinforcementTimer = 1.0;
        m_reinforcementCountdown--;
    }
    if( m_reinforcementCountdown <= 0)
    {
        g_app->m_multiwiniaHelp->NotifyReinforcements();

        double timer = GetGameOption( "ReinforcementTimer" );
        if( timer < 0.0 ) timer = 60.0;
        m_reinforcementCountdown= timer;
        if( m_firstReinforcements ) m_reinforcementCountdown--;
        int maxArmour           = GetGameOption( "MaxArmour" );
        int reinforcementCount  = GetGameOption( "ReinforcementCount" );
        int turretTimer         = GetGameOption( "TurretFrequency" );
        int maxTurrets          = GetGameOption( "MaxTurrets" );
        if( maxTurrets == -1 )  maxTurrets = 2;

        int defenderCount = -1;
        int attackerCount = -1;
        int defenderReinforcementType = -1;
        int attackerReinforcementType = -1;

        if( reinforcementCount == -1 )
        {
            reinforcementCount = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionTrunkPortReinforcements];
            if( m_gameType == GameTypeAssault )
            {
                defenderCount = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionDefenderReinforcements];
                attackerCount = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionAttackerReinforcements];

                if( defenderCount == -1 ) defenderCount = reinforcementCount;
                if( attackerCount == -1 ) attackerCount = reinforcementCount;
            }
        }

        defenderReinforcementType = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionDefenderReinforcementType];
        attackerReinforcementType = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionAttackerReinforcementType];

        if( maxArmour == -1 )
        {
            maxArmour = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionTrunkPortArmour];
        }

        if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionNoGunTurrets] > 0)
        {
            turretTimer = 0;
        }

        bool giveTurrets = false;

        if( turretTimer > 0 )
        {
            --m_turretTimer;
            if( m_turretTimer <= 0 )
            {               
                giveTurrets = true;
                m_turretTimer = turretTimer;
            }
        }


        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == Building::TypeTrunkPort )
                {
                    int teamId = building->m_id.GetTeamId();

                    if( teamId != 255 )
                    {
                        if( m_gameType == GameTypeAssault )
                        {
                            if( g_app->m_location->m_levelFile->m_attacking[ g_app->m_location->GetTeamPosition(teamId) ] )
                            {
                                reinforcementCount = attackerCount;
                            }
                            else
                            {
                                reinforcementCount = defenderCount;
                            }
                        }

                        if( !((TrunkPort *)building)->PopulationLocked() )
                        {
                            for( int j = 0; j < reinforcementCount; ++j )
                            {
                                Vector3 spawnPos = building->m_pos + g_upVector * 50;
                                Vector3 rotation = g_upVector;
                                rotation.RotateAround( building->m_front * syncfrand( 2.0 * M_PI ) );
                                rotation.SetLength( syncfrand( 40 ) );
                                spawnPos += rotation;

                                Vector3 exitVel = building->m_front;
                                exitVel.y = syncfrand(1.0f);
                                exitVel *= ( 20 + syncfrand(15) );

                                WorldObjectId spawned = g_app->m_location->SpawnEntities( spawnPos, teamId, -1, Entity::TypeDarwinian, 1, exitVel, 0.0f );
                                Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( spawned, Entity::TypeDarwinian );
                                if( darwinian )
                                {
                                    darwinian->m_onGround = false;
                                }

                            }
                        }

                        Team *team = g_app->m_location->m_teams[teamId];
                        TaskManager *tm = team->m_taskManager;
                        if( !m_firstReinforcements ) tm->m_blockMostRecentPopup = true;

                        team->m_blockUnitChange = true;
                        
                        int currentTask = tm->m_currentTaskId;

                        // 2 of these should be -1
                        int currentEntity = g_app->m_location->m_teams[teamId]->m_currentEntityId;
                        int currentUnit = g_app->m_location->m_teams[teamId]->m_currentUnitId;
                        int currentBuilding = g_app->m_location->m_teams[teamId]->m_currentBuildingId;

                        if( tm->CountNumTasks( GlobalResearch::TypeArmour ) < maxArmour ||
                            maxArmour == 0 )
                        {
                            bool attacking = g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(teamId)];
                            if( (defenderReinforcementType == -1 && !attacking ) ||
                                (attackerReinforcementType == -1 && attacking ) )
                            {

                                //tm->RunTask( GlobalResearch::TypeArmour );                    

                                Task *task = new Task( teamId );
                                task->m_type = GlobalResearch::TypeArmour;                                
                                bool success = tm->RunTask( task );
								if( success )
								{
                                    tm->NotifyRecentTaskSource( 1 );

									Vector3 pos = building->m_pos + building->m_front * 75;
									pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
									task->Target( pos );

    								g_app->m_markerSystem->RegisterMarker_Task( task->m_objId, false );
								}
                            }
                        }

                        if( currentTask != -1 ) tm->SelectTask(currentTask);
                        //g_app->m_location->m_teams[teamId]->SelectUnit( currentUnit, currentEntity, currentBuilding );

                        int taskType = GlobalResearch::TypeGunTurret;
                        
                        if( m_gameType == Multiwinia::GameTypeBlitzkreig )
                        {
                            int numZones = MultiwiniaZone::GetNumZones(teamId);
                            if( numZones >= (s_numMultiwiniaZones + 1) / 2 ) taskType = GlobalResearch::TypeAirStrike;
                            int num = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionAirstrikeCaptureZones];
                            if( num != -1 && numZones > num ) taskType = GlobalResearch::TypeAirStrike;
                        }

                        if( giveTurrets && 
                            tm->CountNumTasks( taskType ) < maxTurrets )
                        {
                            tm->RunTask( taskType );    
                            tm->NotifyRecentTaskSource( 1 );
                        }
                        team->m_blockUnitChange = false;
                        tm->m_blockMostRecentPopup = false;
                    }
                }
            }
        }

        if( defenderReinforcementType != -1 )
        {
            for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
            {
                if( m_teams[i].m_teamType == TeamTypeUnused ) continue; 
                if(!g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(i)] )
                {
                    g_app->m_location->m_teams[i]->m_blockUnitChange = true;
                    TaskManager *tm = g_app->m_location->m_teams[i]->m_taskManager;
                    if( !m_firstReinforcements ) tm->m_blockMostRecentPopup = true;
                    Task *task = new Task( i );
                    task->m_type = defenderReinforcementType;
                    task->m_option = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionDefendReinforcementMode];
                    tm->RunTask(task);
                    g_app->m_location->m_teams[i]->m_blockUnitChange = false;
                    tm->m_blockMostRecentPopup = false;
                    tm->NotifyRecentTaskSource( 1 );
                }
            }
        }

        if( attackerReinforcementType != -1 )
        {
            for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
            {
                if( m_teams[i].m_teamType == TeamTypeUnused ) continue; 
                if(g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(i)] ||
                    m_gameType != Multiwinia::GameTypeAssault )
                {
                    g_app->m_location->m_teams[i]->m_blockUnitChange = true;
                    TaskManager *tm = g_app->m_location->m_teams[i]->m_taskManager;
                    if( !m_firstReinforcements ) tm->m_blockMostRecentPopup = true;
                    Task *task = new Task( i );
                    task->m_type = attackerReinforcementType;
                    task->m_option = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionAttackReinforcementMode];
                    tm->RunTask(task);
                    g_app->m_location->m_teams[i]->m_blockUnitChange = false;
                    tm->m_blockMostRecentPopup = false;
                    tm->NotifyRecentTaskSource( 1 );
                }
            }
        }
    }
    m_firstReinforcements = false;
#endif
}


void Multiwinia::AdvanceRetribution()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_teams[t].m_teamType == TeamTypeUnused ) continue;

        Team *team = g_app->m_location->m_teams[t];

        if( team->m_monsterTeam || 
            team->m_futurewinianTeam ) continue;
      
        if( IsEliminated(t) ) 
        {
            m_eliminationBonusTimer[t] -= SERVER_ADVANCE_PERIOD;
            if( m_eliminationBonusTimer[t] <= 0 )
            {
                m_eliminationBonusTimer[t] = 45;

                if( team->m_taskManager->CapacityUsed() < team->m_taskManager->Capacity() )
                {
                    LList<int> validPowerups;
                    validPowerups.PutData( Crate::CrateRewardAirstrike );  
                    if( !m_basicCratesOnly )
                    {
                        validPowerups.PutData( Crate::CrateRewardAirstrike ); 
                        validPowerups.PutData( Crate::CrateRewardMeteor );
                        validPowerups.PutData( Crate::CrateRewardForest );
                        validPowerups.PutData( Crate::CrateRewardDarkForest );
                        validPowerups.PutData( Crate::CrateRewardEggs );
                    }

                    if( validPowerups.Size() )
                    {
                        int chosenIndex = syncrand() % validPowerups.Size();
                        int chosenPowerup = validPowerups[ chosenIndex ];

                        Crate::CaptureCrate( chosenPowerup, t );
                        team->m_taskManager->NotifyRecentTaskSource( 2 );
                        //g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_retribution"), t );
                    }
                }
                else
                {
                    if( g_app->m_globalWorld->m_myTeamId == t )
                    {
                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_retribution_maxtasks"), t );
                    }
                }
            }
        }
        else
        {
            m_eliminationBonusTimer[t] = 45;
        }
    }
}

/*
    Code for retribution : random darwinwians spawn

    Vector3 pos;
    while( g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) < 20.0f ||
          RestrictionZone::IsRestricted( pos ) ||
          !g_app->m_location->ViablePosition( pos ) )
    {
        float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        pos.Set( FRAND(worldSizeX), 0.0f, FRAND(worldSizeZ) );
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
    }

    int num = 25 + syncrand() % 10;
    for( int i = 0; i < num; ++i )
    {
        Vector3 vel = g_upVector + Vector3(SFRAND(1), 0.0f, SFRAND(1) );
        vel.SetLength( 10.0f + syncfrand(20.0f) );
        WorldObjectId id = g_app->m_location->SpawnEntities( pos, t, -1, Entity::TypeDarwinian, 1, vel, 0.0f, 0.0f );
        Entity *entity = g_app->m_location->GetEntity( id );
        entity->m_front.y = 0.0f;
        entity->m_front.Normalise();
        entity->m_onGround = false;
    }
    g_app->m_markerSystem->RegisterMarker_Fixed( t, pos, "icons/icon_darwinian.bmp", false );
    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_retribution"), t );

    */


void Multiwinia::EnsureStartingStatue()
{
    //
    // Count statues and statue spawn zones

    LList<int> statues;
    LList<int> statueSpawns;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building )
            {
                if( building->m_type == Building::TypeStatue ) 
                {
                    statues.PutData( building->m_id.GetUniqueId() );
                }

                if( building->m_type == Building::TypeMultiwiniaZone &&
                    building->m_id.GetTeamId() == 255 )
                {
                    statueSpawns.PutData( building->m_id.GetUniqueId() );
                }
            }
        }
    }


    //
    // If there arent enough statues, consider spawning more
    // Spawn faster if there are less than we want

    if( statues.Size() == 0 )
    {
        int chosenIndex = syncrand() % statueSpawns.Size();
        int spawnerId = statueSpawns[chosenIndex];

        Building *spawner = g_app->m_location->GetBuilding( spawnerId );

        Statue carryTemplate;    
        carryTemplate.m_pos = spawner->m_pos;
        carryTemplate.m_front = Vector3(SFRAND(1.0),0.0,SFRAND(1.0) ).Normalise();

        Statue *building = (Statue *) Building::CreateBuilding( Building::TypeStatue );
        g_app->m_location->m_buildings.PutData( building );                                        
        building->Initialise((Building *)&carryTemplate);
        int id = g_app->m_globalWorld->GenerateBuildingId();                    
        building->m_id.SetUnitId( UNIT_BUILDINGS );
        building->m_id.SetUniqueId( id ); 
        building->m_id.SetTeamId(255);

        g_app->m_markerSystem->RegisterMarker_Statue( building->m_id );
    }
}


void Multiwinia::AdvanceCaptureStatue()
{
#ifdef INCLUDEGAMEMODE_CTS

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {
        m_timer = 1.0f;
        m_timeRemaining --;
        BackupScores();

        if( m_timeRemaining <= 0 )
        {
            int highScore = -1;
            int winningTeam = -1;

            int highestNotEliminated = -1;
            int notEliminatedWinner = -1;

            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
                if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;
                if( t == g_app->m_location->GetMonsterTeamId() ) continue;

                if( m_teams[t].m_score == highScore )
                {
                    highScore = m_teams[t].m_score;
                    winningTeam = MULTIWINIA_DRAW_TEAM;
                }
                else if( m_teams[t].m_score > highScore )
                {
                    highScore = m_teams[t].m_score;
                    winningTeam = t;
                }

                Team *team = g_app->m_location->m_teams[t];
                if( !IsEliminated(t) )
                {
                    if( m_teams[t].m_score > highestNotEliminated )
                    {
                        highestNotEliminated = m_teams[t].m_score;
                        notEliminatedWinner = t;
                    }
                    else if( m_teams[t].m_score == highestNotEliminated )
                    {
                        // tie between non-eliminated teams, which is an acceptable tie condition
                        highestNotEliminated = m_teams[t].m_score;
                        notEliminatedWinner = MULTIWINIA_DRAW_TEAM;
                    }
                }
            }

            if( winningTeam == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
            {
                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
                    if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;
                    if( t == g_app->m_location->GetMonsterTeamId() ) continue;

                    if( m_teams[t].m_score == highScore )
                    {
                        Team *team = g_app->m_location->m_teams[t];
                        if( IsEliminated(t)  )
                        {
                            // one of our draw teams is actually eliminated, so disregard them and use the highest team not eliminated
                            winningTeam = notEliminatedWinner;
                            break;
                        }
                    }
                }
            }

            if( m_coopMode )
            {
                int group1Total = 0, group2Total = 0;
                int group1Player = -1, group2Player = -1;
                for( int i = 0; i < NUM_TEAMS; ++i )
                {
                    if( g_app->m_location->m_coopGroups[i] == 1 )
                    {
                        group1Total += m_teams[i].m_score;
                        group1Player = i;
                    }
                    else if( g_app->m_location->m_coopGroups[i] == 2 ) 
                    {
                        group2Total += m_teams[i].m_score;
                        group2Player = i;
                    }
                }

                if( group1Total == group2Total ) 
                {
                    winningTeam = MULTIWINIA_DRAW_TEAM;
                }
                else if( group2Total > group1Total )
                {
                    winningTeam = group2Player;
                }
                else 
                {
                    winningTeam = group1Player;
                }                
            }

            if( winningTeam == -1 )
            {
                // there was no winner
                // odds are, this is because the futurewinians have arrived and dominated the map while there is a tie
                // in that case, just declare the futurewinians the winners
                Team *team = g_app->m_location->m_teams[ g_app->m_location->GetFuturewinianTeamId() ];
                if( team->NumEntities(255) > 50 )
                {
                    DeclareWinner( g_app->m_location->GetFuturewinianTeamId() );
                    return;
                }
                else
                {
                    // small chance that the Evil Darwinians have conquered the map
                    Team *evil = g_app->m_location->m_teams[ g_app->m_location->GetMonsterTeamId() ];
                    if( evil->NumEntities(255) > 50 )
                    {
                        DeclareWinner( g_app->m_location->GetMonsterTeamId() );
                        return;
                    }
                    else
                    {
                        // no winner, no tie, no futurewinians or evil darwinians
                        // this should theoretically be impossible, unless all teams spawn points are hit by nukes simultaneously 30 seconds before the end of the game
                        // if it happens, just say its a draw and either end the game, or enter sudden death depending on options
                        DeclareWinner( MULTIWINIA_DRAW_TEAM );
                        return;
                    }
                }
            }
            else if( m_suddenDeathActive )
            {
                if( winningTeam != MULTIWINIA_DRAW_TEAM )
                {
                    DeclareWinner( winningTeam );
                    return;
                }
            }
            else if( winningTeam == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
            {
                if( !m_coopMode )
                {
                    for( int t = 0; t < NUM_TEAMS; ++t )
                    {
                        if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
                        if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;
                        if( t == g_app->m_location->GetMonsterTeamId() ) continue;

                        if( m_teams[t].m_score != highScore )
                        {
                            g_app->m_location->m_teams[t]->Eliminate();
                        }
                    }
                }

                m_suddenDeathActive = true;
                g_app->m_location->SetCurrentMessage(LANGUAGEPHRASE("multiwinia_suddendeath"));

            }
            else
            {
                DeclareWinner( winningTeam );

                if( winningTeam == g_app->m_globalWorld->m_myTeamId )
                {
                    int secondHighest = 0;
                    for( int i = 0; i < NUM_TEAMS; ++i )
                    {
                        if( m_teams[i].m_teamType == TeamTypeUnused ) continue;
                        if( i == g_app->m_location->GetFuturewinianTeamId() ) continue;
                        if( i == g_app->m_location->GetMonsterTeamId() ) continue;

                        if( m_teams[i].m_score > secondHighest &&
                            m_teams[i].m_score < highScore )
                        {
                            secondHighest = m_teams[i].m_score;
                        }                
                    }

                    if( highScore > secondHighest * 2 &&
                        highScore > 3 )
                    {
                        g_app->GiveAchievement( App::DarwiniaAchievementHoarder );
                    }
                }

                return;
            }
        }

        //
        // Count statues and statue spawn zones

        LList<int> statues;
        LList<int> statueSpawns;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building )
                {
                    if( building->m_type == Building::TypeStatue ) 
                    {
                        statues.PutData( building->m_id.GetUniqueId() );
                    }

                    if( building->m_type == Building::TypeMultiwiniaZone &&
                        building->m_id.GetTeamId() == 255 )
                    {
                        statueSpawns.PutData( building->m_id.GetUniqueId() );
                    }
                }
            }
        }
        

        //
        // If there arent enough statues, consider spawning more
        // Spawn faster if there are less than we want

        if( statues.Size() < statueSpawns.Size() )
        {
            if( m_statueSpawner == -1.0 )
            {
                double maxSpawnTime = 60.0 * statues.Size() / (double) statueSpawns.Size();
                m_statueSpawner = maxSpawnTime/2.0 + syncfrand(maxSpawnTime/2.0);
                if( m_statueSpawner < 10.0 ) m_statueSpawner = 10.0;
                 
                AppDebugOut( "Statue Spawn timer = %d secs\n", (int)m_statueSpawner );
            }
            else if( m_statueSpawner > 0.0f )
            {
                m_statueSpawner -= 1.0f;
            }
            else if( m_statueSpawner <= 0.0f )
            {
                m_statueSpawner = -1.0f;

                // If there are no statues, chose a spawn point randomly
                // Otherwise chose the spawn point furthest from any statue

                int spawnerId = -1;

                if( statues.Size() == 0 )
                {
                    int chosenIndex = syncrand() % statueSpawns.Size();
                    spawnerId = statueSpawns[chosenIndex];
                }
                else
                {                
                    double largestSpawnerDistance = 0.0;

                    for( int i = 0; i < statueSpawns.Size(); ++i )
                    {
                        Building *spawner = g_app->m_location->GetBuilding( statueSpawns[i] );
                        double thisNearest = 99999.9;

                        for( int j = 0; j < statues.Size(); ++j )
                        {
                            Building *statue = g_app->m_location->GetBuilding( statues[j] );

                            double distance = ( statue->m_pos - spawner->m_pos ).Mag();
                            if( distance < thisNearest ) thisNearest = distance;
                        }

                        if( thisNearest > largestSpawnerDistance &&
                            thisNearest > 30 )
                        {
                            largestSpawnerDistance = thisNearest;
                            spawnerId = statueSpawns[i];
                        }
                    }
                }


                if( spawnerId != -1 )
                {
                    Building *spawner = g_app->m_location->GetBuilding( spawnerId );

                    Statue carryTemplate;    
                    carryTemplate.m_pos = spawner->m_pos;
                    carryTemplate.m_front = Vector3(SFRAND(1.0),0.0,SFRAND(1.0) ).Normalise();

                    Statue *building = (Statue *) Building::CreateBuilding( Building::TypeStatue );
                    g_app->m_location->m_buildings.PutData( building );                                        
                    building->Initialise((Building *)&carryTemplate);
                    int id = g_app->m_globalWorld->GenerateBuildingId();                    
                    building->m_id.SetUnitId( UNIT_BUILDINGS );
                    building->m_id.SetUniqueId( id ); 
                    building->m_id.SetTeamId(255);
                    
                    g_app->m_markerSystem->RegisterMarker_Statue( building->m_id );
                    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_cts_newstatue") );
                }
            }
        }
    }

#endif
}

void Multiwinia::AdvanceTankBattle()
{
#ifndef MULTIWINIA_DEMOONLY
#ifdef INCLUDEGAMEMODE_TANKBATTLE
    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {
        m_timer = 1.0f;
        int totalTeams = 0;
        int numEliminated = 0;
        int notEliminated = -1;
        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
            if( t == g_app->m_location->GetMonsterTeamId() ||
                t == g_app->m_location->GetFuturewinianTeamId() ) continue;

            totalTeams++;
            m_teams[t].m_score = GetGameOption( "NumTanks" ) - m_tankDeaths[t];
            BackupScores();
            if( !m_eliminated[t] )
            {
                notEliminated = t;
                Team *team = g_app->m_location->m_teams[t];
                
                bool tankFound = false;
                for( int i = 0; i < team->m_specials.Size(); ++i )
                {
                    Entity *e = g_app->m_location->GetEntity( *team->m_specials.GetPointer(i) );
                    if( e && e->m_type == Entity::TypeTank )
                    {
                        tankFound = true;
                        break;
                    }
                }

                if( !tankFound )
                {
                    m_tankDeaths[t]++;
                    UnicodeString msg;
                    if( m_tankDeaths[t] >= GetGameOption( "NumTanks" ) )
                    {
                        EliminateTeam(t);
                        //sprintf( msg, "%s is eliminated!", g_app->m_location->m_teams[t]->GetTeamName() );
                    }
                    else
                    {
                        msg = LANGUAGEPHRASE("multiwinia_tankloss");
                        msg.ReplaceStringFlag( L'T', g_app->m_location->m_teams[t]->GetTeamName() );
                        for( int i = 0; i < g_app->m_location->m_levelFile->m_instantUnits.Size(); ++i )
                        {
                            InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits[i];
                            if( iu->m_type == Entity::TypeTank && 
                                iu->m_teamId == g_app->m_location->GetTeamPosition(t) )
                            {
                                Vector3 pos( iu->m_posX, 0, iu->m_posZ );
                                pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
                                TaskManager *tm = g_app->m_location->m_teams[t]->m_taskManager;
                                tm->RunTask( GlobalResearch::TypeTank );
                                tm->GetTask( tm->m_mostRecentTaskId )->Target( pos );
                                break;
                            }
                        }
                    }
                    g_app->m_location->SetCurrentMessage(msg);
                }
            }
            else
            {
                numEliminated++;
            }
        }

        if( totalTeams - 1 == numEliminated )
        {
            DeclareWinner( notEliminated );
        }
    }
#endif
#endif
}

void Multiwinia::AdvanceAssault()
{
#ifndef MULTIWINIA_DEMOONLY
#ifdef INCLUDEGAMEMODE_ASSAULT

    if( !m_initialised )
    {
        m_initialised = true;
		m_defendedAll = true;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {   
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == Building::TypePulseBomb )
                {
                    PulseBomb *pb = (PulseBomb *)building;
                    for( int j = 0; j < pb->m_links.Size(); ++j )
                    {
                        m_assaultObjectives.PutDataAtEnd( pb->m_links[j] );
                    }

                    m_assaultObjectives.PutDataAtEnd( building->m_id.GetUniqueId() );
                }
            }
        }
    }

	// Assault achievement check throughout the game.
	// Only check after the first 10 seconds to make sure defenders have a chance to get all control stations.
	if( GetHighResTime() > m_startTime + 10 )
	{
		Building* assaultObjectiveDefendCheck = NULL;
		for( int i = 0; i < m_assaultObjectives.Size(); i++ )
		{
			if( !m_assaultObjectives.ValidIndex(i) ) continue;

			assaultObjectiveDefendCheck = g_app->m_location->GetBuilding(m_assaultObjectives.GetData(i));
			if( !assaultObjectiveDefendCheck ) continue;

			m_defendedAll &= g_app->m_location->IsDefending(assaultObjectiveDefendCheck->m_id.GetTeamId());
		}
	}

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {
        m_timer = 1.0f;
        if( m_timeRemaining > 0 && m_victoryTimer <= 0.0f ) m_timeRemaining--;

        if( m_pulseDetonated )
        {
            if( !m_assaultSingleRound )
            {
                if( m_victoryTimer <= 1.0f && g_app->m_renderer->IsFadeComplete() &&
                    m_numRotations < g_app->m_location->m_levelFile->m_numPlayers - 1 )
                {
                    g_app->m_renderer->StartFadeOut();
                }
            }

            if( m_victoryTimer <= 0.0f && !PulseWave::s_pulseWave )
            {
                if( m_numRotations == g_app->m_location->m_levelFile->m_numPlayers - 1 ||
                    m_assaultSingleRound )
                {
                    // we've rotated through all positions, so work out who won the game
                    int winner = -1;
                    int numFullDefenses = 0;
                    if( m_assaultSingleRound )
                    {
                        // only a single round, so we only care about whether the attackers or defenders win
                        // not about individual team victories (unless it's only 2 player, obviously
                        int pulseId = -1;
                        for( int i = 0; i < NUM_TEAMS; ++i )
                        {
                            if( PulseBomb::s_pulseBombs[i] != -1 )    
                            {
                                pulseId = PulseBomb::s_pulseBombs[i];
                                break;
                            }
                        }
                        Building *b = g_app->m_location->GetBuilding( pulseId );
                        if( b )
                        {
                            if( b->m_destroyed )
                            {
                                // pulse bomb is destroyed so attackers win
                                for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
                                {
                                    if( g_app->m_location->m_levelFile->m_attacking[ g_app->m_location->GetTeamPosition(i) ] )
                                    {
                                        winner = i;
                                        break;
                                    }
                                }
                            }
                            else
                            {
                                // pulse bomb isnt destroyed so defenders win
                                for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
                                {
                                    if( !g_app->m_location->m_levelFile->m_attacking[ g_app->m_location->GetTeamPosition(i) ] )
                                    {
                                        winner = i;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        float bestTime = 0.0f;

                        for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
                        {
                            if( m_timeRecords[i] >= PulseBomb::s_defaultTime ) numFullDefenses++;
                            if( m_timeRecords[i] > bestTime )
                            {
                                bestTime = m_timeRecords[i];
                                winner = i;
                            }
                        }
                    }

                    DeclareWinner( winner );
					Team *myTeam = g_app->m_location->GetMyTeam();
					if( myTeam && g_app->m_location->IsDefending(myTeam->m_teamId) )
					{
						if( m_defendedAll )
						{
							g_app->GiveAchievement( App::DarwiniaAchievementAggravatedAssault );
						}
					}
                }
                else
                {
                    m_victoryTimer = -1.0f;
                    m_pulseDetonated = false;
                    m_resetLevel = true;
                }
            }
        }
    }
#endif
#endif
}


//void Multiwinia::AdvanceBlitkrieg()
//{
//#ifndef MULTIWINIA_DEMOONLY
//#ifdef INCLUDEGAMEMODE_BLITZ
//
//    if( !m_initialised )
//    {
//        m_initialised = true;
//
//        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
//        {
//            if( g_app->m_location->m_buildings.ValidIndex(i) )
//            {   
//                Building *building = g_app->m_location->m_buildings[i];
//                if( building->m_type == Building::TypeMultiwiniaZone )
//                {
//                    MultiwiniaZone *zone = (MultiwiniaZone *) building;
//
//                    for( int j = 0; j < zone->m_blitzkriegLinks.Size(); ++j )
//                    {
//                        int nextLink = zone->m_blitzkriegLinks[j];
//                        MultiwiniaZone *linkZone = (MultiwiniaZone *)g_app->m_location->GetBuilding( nextLink );
//                        if( linkZone && linkZone->m_type == Building::TypeMultiwiniaZone )
//                        {
//                            linkZone->m_blitzkriegLinks.PutData( zone->m_id.GetUniqueId() );
//                        }
//                    }
//                }
//            }
//        }                          
//    }
//
//
//    m_timer -= SERVER_ADVANCE_PERIOD;
//    if( m_timer <= 0.0f )
//    {        
//        if( m_timeRemaining > 0 ) m_timeRemaining--;
//        m_timer = 1.0f;
//        BackupScores();        
//
//
//        s_numMultiwiniaZones = 0;
//        int numZonesOwned[NUM_TEAMS];
//        int numZonesLocked[NUM_TEAMS];
//        memset( numZonesOwned, 0, NUM_TEAMS * sizeof(int) );
//        memset( numZonesLocked, 0, NUM_TEAMS * sizeof(int) );
//
//        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
//        {
//            if( g_app->m_location->m_buildings.ValidIndex(i) )
//            {   
//                Building *building = g_app->m_location->m_buildings[i];
//                if( building->m_type == Building::TypeMultiwiniaZone )
//                {
//                    MultiwiniaZone *zone = (MultiwiniaZone *) building;
//                    ++s_numMultiwiniaZones;
//                    if( zone->m_id.GetTeamId() != 255 ) 
//                    {
//                        numZonesOwned[zone->m_id.GetTeamId()]++;
//
//                        if( zone->m_blitzkriegLocked ) 
//                        {
//                            numZonesLocked[zone->m_id.GetTeamId()]++;
//                        }
//
//                        for( int t = 0; t < NUM_TEAMS; ++t )
//                        {
//                            if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
//                            if( t == g_app->m_location->GetMonsterTeamId() ) continue;
//                            if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;
//                            if( m_eliminated[t] ) continue;
//
//                            if( zone->m_id.GetUniqueId() == MultiwiniaZone::s_blitzkriegBaseZone[t] )
//                            {
//                                if( zone->m_id.GetTeamId() != t )
//                                {
//                                    EliminateTeam(t);
//                                }
//                            }
//                        }
//                    }
//                }
//                else if( building->m_type == Building::TypeTrunkPort )
//                {
//                    if( building->m_id.GetTeamId() != 255 && m_eliminated[ building->m_id.GetTeamId() ] )
//                    {
//                        building->SetTeamId( 255 );
//                    }
//                }
//            }
//        }
//
//        int notEliminated = -1;
//        int numEliminated = 0;
//        int highscore = -1;
//        int winningTeam = -1;
//
//		bool retributionMode = GetGameOption( "RetributionMode" );
//
//        for( int i = 0; i < NUM_TEAMS; ++i )
//        {
//            if( m_teams[i].m_teamType == TeamTypeUnused ) continue;
//            if( i == g_app->m_location->GetMonsterTeamId() ) continue;
//            if( i == g_app->m_location->GetFuturewinianTeamId() ) continue;
//
//            if( m_eliminated[i] ) numEliminated++;
//            else notEliminated = i;
//
//            m_teams[i].m_score = numZonesOwned[i];
//
//            if( m_teams[i].m_score == highscore )
//            {
//                winningTeam = MULTIWINIA_DRAW_TEAM;
//                highscore = m_teams[i].m_score;
//            }
//            else if( m_teams[i].m_score > highscore )
//            {
//                winningTeam = i;
//                highscore = m_teams[i].m_score;
//            }
//
//            if( numZonesLocked[i] == s_numMultiwiniaZones )
//            {
//                DeclareWinner(i);
//            }
//
//            if( retributionMode && !m_suddenDeathActive )
//            {
//                if( m_eliminated[i] )
//                {
//                    Team *team = g_app->m_location->m_teams[i];
//                    if( team->m_taskManager->CountNumTasks( GlobalResearch::TypeSquad ) == 0 )
//                    {
//                        m_eliminationBonusTimer[i]--;
//                        if( m_eliminationBonusTimer[i] <= 0 )
//                        {
//                            m_eliminationBonusTimer[i] = 45;
//
//                            Vector3 pos;
//                            while(  g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) < 20.0f ||
//                                    g_app->m_location->m_landscape.m_normalMap->GetValue( pos.x, pos.z ).y < 0.9f ||
//                                    RestrictionZone::IsRestricted( pos ) )
//                            {
//                                float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
//                                float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
//                                pos.Set( FRAND(worldSizeX), 0.0f, FRAND(worldSizeZ) );
//                                pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
//                            }
//
//                            Team *team = g_app->m_location->m_teams[i];
//                            team->m_taskManager->RunTask( GlobalResearch::TypeSquad );
//                            team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->Target( pos );
//                            team->m_taskManager->GetTask( team->m_taskManager->m_mostRecentTaskId )->m_lifeTimer = -1.0f;
//                            g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("multiwinia_retribution"), i );
//                        }
//                    }
//                    else
//                    {
//                        m_eliminationBonusTimer[i] = 45;
//                    }
//                }
//            }
//        }
//
//        if( m_winner == -1 )
//        {
//            if( g_app->m_location->m_levelFile->m_numPlayers - numEliminated == 1 )
//            {
//                DeclareWinner( notEliminated );
//            }
//        }
//
//        if( m_winner != -1 )
//        {
//            if( GetNetworkTime() - m_startTime < 300.0f )
//            {
//                g_app->GiveAchievement( App::DarwiniaAchievementBlitzMaster );
//            }
//        }
//
//        if( m_timeRemaining <= 0 && !m_unlimitedTime )
//        {
//            if( m_suddenDeathActive )
//            {
//                if( winningTeam != MULTIWINIA_DRAW_TEAM )
//                {
//                    DeclareWinner( winningTeam );
//                }
//            }
//            else if( winningTeam == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
//            {
//                for( int t = 0; t < NUM_TEAMS; ++t )
//                {
//                    if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
//                    if( t == g_app->m_location->GetMonsterTeamId() ) continue;
//                    if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;
//
//                    if( m_teams[t].m_score != highscore )
//                    {
//                        g_app->m_location->m_teams[t]->Eliminate();
//                    }
//                }
//
//                m_suddenDeathActive = true;
//                g_app->m_location->SetCurrentMessage(LANGUAGEPHRASE("multiwinia_suddendeath"));
//            }
//            else
//            {
//                DeclareWinner( winningTeam );
//            }
//        }
//    }
//
//    AdvanceTrunkPorts();
//#endif
//#endif
//}


bool DoesBlitzkriegRouteExist( int fromZoneId, int attackingTeamId, LList<int> &zonesSoFar )
{
    //
    // If this zone has already been visited, abort

    for( int i = 0; i < zonesSoFar.Size(); ++i )
    {
        if( zonesSoFar[i] == fromZoneId )
        {
            return false;
        }
    }


    //
    // This zone must be fully owned by the attacking team

    MultiwiniaZone *currentZone = (MultiwiniaZone *)g_app->m_location->GetBuilding(fromZoneId);

    if( currentZone->m_id.GetTeamId() != attackingTeamId )
    {
        return false;
    }

    // If it is not fully owned, it must be under contention to break the link

    if( currentZone->m_blitzkriegOwnership < 100.0f &&
        currentZone->m_blitzkriegUpOrDown < 0 )
    {
        return false;
    }

    
    //
    // We can pass through this zone

    zonesSoFar.PutDataAtEnd(fromZoneId);


    //
    // Determine if we have reached the target

    int attackerZoneId = MultiwiniaZone::s_blitzkriegBaseZone[attackingTeamId];
    if( fromZoneId == attackerZoneId )
    {
        return true;
    }

    //
    // Otherwise go through our existing links

    for( int i = 0; i < currentZone->m_blitzkriegLinks.Size(); ++i )    
    {
        int linkId = currentZone->m_blitzkriegLinks[i];
        
        if( DoesBlitzkriegRouteExist( linkId, attackingTeamId, zonesSoFar ) )
        {
            return true;
        }
    }


    //
    // Nothing this way

    zonesSoFar.RemoveDataAtEnd();

    return false;
}


void Multiwinia::AdvanceBlitkrieg()
{
#ifndef MULTIWINIA_DEMOONLY
#ifdef INCLUDEGAMEMODE_BLITZ
    //
    // Initialise multiwinia zones if first time through

    if( !m_initialised )
    {
        m_initialised = true;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {   
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == Building::TypeMultiwiniaZone )
                {
                    MultiwiniaZone *zone = (MultiwiniaZone *) building;
                    s_numMultiwiniaZones++;

                    for( int j = 0; j < zone->m_blitzkriegLinks.Size(); ++j )
                    {
                        int nextLink = zone->m_blitzkriegLinks[j];
                        MultiwiniaZone *linkZone = (MultiwiniaZone *)g_app->m_location->GetBuilding( nextLink );
                        if( linkZone && linkZone->m_type == Building::TypeMultiwiniaZone )
                        {
                            linkZone->m_blitzkriegLinks.PutData( zone->m_id.GetUniqueId() );
                        }
                    }
                }
            }
        }                          
    }
    
    AdvanceTrunkPorts();    

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0f )
    {        
        if( m_timeRemaining > 0 ) m_timeRemaining--;
        m_timer = 1.0f;
        BackupScores();        
       

        //
        // Look to see if any teams have been eliminated
        // Method:  Find base zones, and if they are owned by an enemy team,
        // look for a route via flags back to that teams home flag
        // If found, the team is eliminated immediately

        bool retributionMode = GetGameOption( "RetributionMode" );
        for( int t = 0; t < NUM_TEAMS; ++t )
        {            
            if( !m_eliminated[t] &&
                 m_teams[t].m_teamType != TeamTypeUnused &&
                 t != g_app->m_location->GetMonsterTeamId() &&
                 t != g_app->m_location->GetFuturewinianTeamId() )
            {
                int baseZoneId = MultiwiniaZone::s_blitzkriegBaseZone[t];

                MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding(baseZoneId);
                
                if( zone->m_id.GetTeamId() != 255 &&
                    zone->m_id.GetTeamId() != t )
                {
                    // We are owned by an enemy team
                    // Look for route recursively
                    
                    LList<int> zonesSoFar;
                    
                    if( DoesBlitzkriegRouteExist( baseZoneId, zone->m_id.GetTeamId(), zonesSoFar ) )
                    {
                        EliminateTeam(t);
                        g_app->m_location->m_teams[t]->Eliminate();
                        m_teams[t].m_score = g_gameTimer.GetGameTime();
                        m_numEliminations[zone->m_id.GetTeamId()]++;
                    }
                }                
            }
        }


        //
        // Determine our scores

        for( int t = 0; t < NUM_TEAMS; ++t )
        {            
            if( !m_eliminated[t] &&
                m_teams[t].m_teamType != TeamTypeUnused &&
                t != g_app->m_location->GetMonsterTeamId() &&
                t != g_app->m_location->GetFuturewinianTeamId() )
            {
                int baseZoneId = MultiwiniaZone::s_blitzkriegBaseZone[t];
                MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding(baseZoneId);

                if     ( zone->m_id.GetTeamId() == 255 )        m_teams[t].m_score = BLITZKRIEGSCORE_UNOCCUPIED;     
                else if( zone->m_id.GetTeamId() != t )          m_teams[t].m_score = BLITZKRIEGSCORE_ENEMYOCCUPIED;     
                else if( !zone->m_blitzkriegLocked )            m_teams[t].m_score = BLITZKRIEGSCORE_UNDERTHREAT;     
                else                                            m_teams[t].m_score = BLITZKRIEGSCORE_SAFE;     

                // Explanation : Eliminated teams have a score equal to the number of seconds they survived
                // Currently active teams need a higher score than that
            }
        }    


        //
        // Has somebody won?

        int winningTeam = -1;

        for( int t = 0; t < NUM_TEAMS; ++t )
        {            
            if( !m_eliminated[t] &&
                m_teams[t].m_teamType != TeamTypeUnused &&
                t != g_app->m_location->GetMonsterTeamId() &&
                t != g_app->m_location->GetFuturewinianTeamId() )
            {
                if( winningTeam == -1 ) winningTeam = t;
                else winningTeam = -2;
            }
        }

        int highscore = 0;
        int realScores[NUM_TEAMS];
        memset( realScores, 0, sizeof(realScores));
        if( m_timeRemaining <= 0 && !m_unlimitedTime )
        {
            int numFlags[NUM_TEAMS];
            memset( numFlags, 0, sizeof(numFlags));
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *b = g_app->m_location->m_buildings[i];
                    if( b->m_type == Building::TypeMultiwiniaZone &&
                        b->m_id.GetTeamId() != 255 )
                    {
                        numFlags[b->m_id.GetTeamId()]++;
                    }
                }
            }
            
            // work out final scores
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( m_eliminated[i] ) continue;
                realScores[i] = numFlags[i] + (m_numEliminations[i] * 10);
                if( realScores[i] > highscore )
                {
                    highscore = realScores[i];
                    winningTeam = i;
                }
                else if( realScores[i] == highscore )
                {
                    winningTeam = MULTIWINIA_DRAW_TEAM;
                }
            }
        }

        if( winningTeam == MULTIWINIA_DRAW_TEAM && GetGameOption("SuddenDeath") == 1 )
        {
            if( !m_suddenDeathActive )
            {
                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( m_teams[t].m_teamType == TeamTypeUnused ) continue;
                    if( t == g_app->m_location->GetMonsterTeamId() ) continue;
                    if( t == g_app->m_location->GetFuturewinianTeamId() ) continue;

                    if( realScores[t] != highscore )
                    {
                        g_app->m_location->m_teams[t]->Eliminate();
                    }
                }

                m_suddenDeathActive = true;
                g_app->m_location->SetCurrentMessage(LANGUAGEPHRASE("multiwinia_suddendeath"));
            }
        }
        else if( winningTeam >= 0 )
        {
            DeclareWinner(winningTeam);
			Team *myTeam = g_app->m_location->GetMyTeam();

			if( myTeam && myTeam->m_teamId == m_teams[winningTeam].m_teamId &&
				m_startTime <= m_timeRemaining + 300 ) // 300 is 5 minutes!
			{
				g_app->GiveAchievement(App::DarwiniaAchievementBlitzMaster);
			}
        }
    }

#endif
#endif
}


void Multiwinia::AdvanceGracePeriod()
{
    // If everyone is ready decrease the game timer
    // until we are out of the grace period

    int numReady = 0;
    int numFound = 0;

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        Team *team = g_app->m_location->m_teams[i];
        if( team->m_teamType == TeamTypeLocalPlayer ||
            team->m_teamType == TeamTypeRemotePlayer ||
			team->m_teamType == TeamTypeSpectator)
        {
            ++numFound;
            if( team->m_ready ) ++numReady;
        }
    }

    if( numReady == numFound )
    {
        if( m_gracePeriodTimer == GRACE_TIME )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "CountdownTick", SoundSourceBlueprint::TypeMultiwiniaInterface );
        }

        m_gracePeriodTimer -= 0.1;  

		// The follow code is a check as the change from float to doubles has caused the check
		// for fabs((int)m_gracePeriodTimer - m_gracePeriodTimer) would cause a difference of
		// 0.99999999991. Which meant the sounds weren't getting triggered.
		static double difference;
		static bool nextTick = false;
		difference = iv_abs( ((int)m_gracePeriodTimer) - m_gracePeriodTimer );

		if( nextTick )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "CountdownTick", SoundSourceBlueprint::TypeMultiwiniaInterface );
			nextTick = false;
        }

		if( difference < 0.1 ) nextTick = true;
    }
    else
    {
        m_gracePeriodTimer = GRACE_TIME;
    }

    if( m_gracePeriodTimer <= 0.0 )
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "CountdownZeroTick", SoundSourceBlueprint::TypeMultiwiniaInterface );
        m_gracePeriodTimer = -1.0f;
        g_gameTimer.RequestSpeedChange( 1.0f, 1.0f, -1.0f );

        g_app->m_achievementTracker->AddVisitedLevel( g_app->m_location->m_levelFile->m_mapFilename );
    }


    //
    // Special case for Capture the Statue:
    // Ensure there is one statue somewhere

    if( m_gameType == GameTypeCaptureTheStatue )
    {
        EnsureStartingStatue();
    }
}


void Multiwinia::Advance()
{    
    if( GameOver() ) 
    {
        return;
    }
    

    if( GameInGracePeriod() )
    {
        AdvanceGracePeriod();
        return;
    }

    if( g_app->GamePaused() ) return;

    if( m_startTime == 0.0 ) 
    {
        m_startTime = GetNetworkTime();
        if( m_timeRemaining <= 0 ) m_unlimitedTime = true;
    }
    if( m_victoryTimer > 0.0 ) m_victoryTimer -= SERVER_ADVANCE_PERIOD;

    switch( m_gameType )
    {
        case GameTypeSkirmish :             AdvanceSkirmish();              break;
        case GameTypeKingOfTheHill :        AdvanceKingOfTheHill();         break;  
        //case GameTypeShaman :               AdvanceShaman();                break;
        case GameTypeRocketRiot :           AdvanceRocketRiot();            break;
        case GameTypeCaptureTheStatue :     AdvanceCaptureStatue();         break;
        case GameTypeTankBattle:            AdvanceTankBattle();            break;
        case GameTypeAssault:               AdvanceAssault();               break;
        case GameTypeBlitzkreig:            AdvanceBlitkrieg();             break;
    }

    if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionTrunkPortReinforcements] > 0 ||
        g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionTrunkPortArmour] > 0 )
    {
        AdvanceTrunkPorts();
    }

    bool retributionMode = GetGameOption( "RetributionMode" );
    if( retributionMode ) 
    {
        AdvanceRetribution();
    }


    if( m_timeRemaining > 0 && m_timeRemaining <= 60 && !m_oneMinuteWarning )
    {
        m_oneMinuteWarning = true;
        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_oneminute") );
    }

    if( m_gameType != GameTypeAssault &&
        m_gameType != GameTypeBlitzkreig )
    {
        m_positionTimer -= SERVER_ADVANCE_PERIOD;
        if( m_positionTimer <= 0.0 )
        {
            m_positionTimer = 1.0;
            LList<int> orderedList;
			int tied[NUM_TEAMS];
			memset( tied, -1, sizeof( tied ) );
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
                if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;
                Team *team = g_app->m_location->m_teams[t];
                if( team->m_teamType != TeamTypeUnused )
                {
                    bool added = false;
                    for( int i = 0; i < orderedList.Size(); ++i )
                    {
                        if( m_teams[t].m_score > m_teams[orderedList[i]].m_score )
                        {
                            orderedList.PutDataAtIndex( t, i );
                            added = true;
                            break;
                        }
                    }
                    if( !added )
                    {
                        orderedList.PutDataAtEnd(t);
                    }
                }
            }

			for( int t = 0; t < NUM_TEAMS; ++t )
			{
                int myPos = -1;
				for( int i = 0; i < orderedList.Size(); i++ )
                {
                    if( t == orderedList[i] )
                    {
                        myPos = i;
                        continue;
                    }
					if( m_teams[t].m_score == m_teams[orderedList[i]].m_score )
					{
                        if( tied[t] == -1 || i < tied[t] )
                        {
						    tied[t] = i;
                        }
					}
				}
                if( myPos != -1 && myPos < tied[t] ) tied[t] = myPos;
			}

            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                for( int i = 0; i < orderedList.Size(); i++ )
                {
                    //if( m_teams[t].m_score == 0 ) continue;
                    if( t != orderedList[i] ) continue;

                    if( m_currentPositions[t] != i )
                    {
                        // this teams position has changed
                        if( m_currentPositions[t] != -1 )
                        {
                            if( t == g_app->m_globalWorld->m_myTeamId )
                                //m_teams[t].m_score > 0)
                            {
								if( tied[t] != -1 )
								{
									char dialog[256];
									sprintf( dialog, "dialog_tied_%d", i + 1 );
									g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE(dialog), t );
								}
								else
								{
									char dialog[256];
									sprintf( dialog, "dialog_position_%d", i + 1 );
									g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE(dialog), t );
								}
                            }
                        }

                        m_currentPositions[t] = i;
                    }
                }
            }
        }
    }

    if( GameOver() )
    {
        int teamId = g_app->m_globalWorld->m_myTeamId;
        if( teamId >= 0 && teamId < NUM_TEAMS )
        {
            Team *team = g_app->m_location->m_teams[teamId];
            if( team->m_teamKills >= 2000 )
            {
                g_app->GiveAchievement( App::DarwiniaAchievementCarnage );
            }
        }
    }
}


double Multiwinia::GetPreciseTimeRemaining()
{
	return m_timeRemaining - (1.0 - m_timer);
}


bool Multiwinia::GameOver()
{
	if( !g_app->m_location ) 
		return false;

    return ( m_winner != -1 );

    if( m_gameType == Multiwinia::GameTypeAssault )
    {
        if( m_winner != -1 ) return true;
        else return false;
    }

    if( //m_gameType == Multiwinia::GameTypeShaman ||
        m_gameType == Multiwinia::GameTypeSkirmish ||
        m_gameType == Multiwinia::GameTypeTankBattle )
    {
        if( m_winner != -1 )
        {
            return ( m_victoryTimer <= 0.0f );
        }
        else
        {
            int totalTeams = 0;
            int numEliminated = 0;
            int notEliminated = -1;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( !g_app->m_location->m_teams[i] ) continue;
                if( g_app->m_location->m_teams[i]->m_teamType == TeamTypeUnused ) continue;
                if( g_app->m_location->GetMonsterTeamId() == i ) continue;
                if( g_app->m_location->GetFuturewinianTeamId() == i ) continue;
                totalTeams++;
                if( m_eliminated[i] ) numEliminated++;
                else notEliminated = i;
            }

            if( totalTeams - numEliminated == 1 )
            {
                DeclareWinner( notEliminated );
                m_victoryTimer = 5.0f;
            }
        }
    }

    if( m_gameType == Multiwinia::GameTypeRocketRiot )       // Rocket riot
    {
        if( m_rocketsFound )
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                Team *team = g_app->m_location->m_teams[t];
                LobbyTeam *lobbyTeam = &m_teams[t];

                if( team->m_monsterTeam ) continue;            
                if( team->m_futurewinianTeam ) continue;

                EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

                if( rocket && 
                    rocket->m_type == Building::TypeEscapeRocket &&
                    rocket->m_state == EscapeRocket::StateFlight &&
                    rocket->m_pos.y > 600 )
                {
                    DeclareWinner( t );
                    return true;
                }
            }
        }
    }

    if( m_timeRemaining == 0 ) return true;

    
    return false;
}

void Multiwinia::RemoveTeams( int _clientId/*, int _reason */ )
{
	for( int i = 0; i < NUM_TEAMS; ++i )
	{
		LobbyTeam &team = m_teams[i];

		if( team.m_teamType != TeamTypeUnused &&
			team.m_teamType != TeamTypeCPU &&
			team.m_clientId == _clientId )
		{
			if( GameInLobby() )
			{
				// We're in the lobby, so remove the team
				GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow( "multiwinia_mainmenu_title" );
				if( gmw )
				{
					UnicodeString msg("PLAYERLEAVE");
					gmw->NewChatMessage( msg, _clientId );
				}

				AppDebugOut( "Team %d removed\n", team.m_teamId );
				team.m_teamType = TeamTypeUnused;
                team.m_colourId = -1;
				team.m_teamName = UnicodeString("");
                team.m_disconnected = true;
			}
			else if( GameRunning() )
			{
				AppDebugOut( "Team %d replaced with CPU player\n", team.m_teamId );
				UnicodeString playerLeft(LANGUAGEPHRASE("multiwinia_player_left"));
				playerLeft.ReplaceStringFlag('T', team.m_teamName);

				if(g_app)
				{
					if(g_app->m_location)
					{
						g_app->m_location->m_messageNoOverwrite = false;
						g_app->m_location->SetCurrentMessage(playerLeft, team.m_teamId, true);

						team.m_teamType = TeamTypeCPU;
						// Add in AI for the player
						g_app->m_location->SpawnEntities( 
							Vector3(-100,0,-100), team.m_teamId, -1, Entity::TypeAI, 1, g_zeroVector, 0 );
						team.m_accurateCPUTakeOverTime = g_gameTimer.GetGameTime();



					}
				}
			}
		}
	}
}


void Multiwinia::EliminateTeam(int _teamId)
{
    m_eliminated[_teamId] = true;
    m_timeEliminated[_teamId] = GetNetworkTime();
    UnicodeString msg;
    if( g_app->m_globalWorld->m_myTeamId == _teamId )
    {
        msg = LANGUAGEPHRASE("multiwinia_player_eliminated");
    }
    else
    {
        msg = LANGUAGEPHRASE("multiwinia_other_eliminated");
        msg.ReplaceStringFlag( L'T', g_app->m_location->m_teams[_teamId]->GetTeamName() );
    }
    g_app->m_location->SetCurrentMessage( msg );  
}


int  Multiwinia::CountDarwinians ( int _teamId )
{
    Team *team = g_app->m_location->m_teams[_teamId];

    int count = 0;

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];

            if( entity &&
                entity->m_type == Entity::TypeDarwinian &&
                !entity->m_dead )
            {
                ++count;
            }
        }
    }

    return count;
}

int CountSpawnPoints()
{
	int totalSpawnPoints = 0;
    for( int i = 0; i < NUM_TEAMS + 1; ++i )
    {
        totalSpawnPoints += SpawnPoint::s_numSpawnPoints[i];
    }
	return totalSpawnPoints;
}

bool Multiwinia::IsEliminated( int _teamId )
{
    Team *team = g_app->m_location->m_teams[_teamId];

    switch( m_gameType )
    {    
        case GameTypeSkirmish:
        case GameTypeBlitzkreig:
            return m_eliminated[_teamId];
            
        case GameTypeKingOfTheHill:
        case GameTypeCaptureTheStatue:
            return( CountSpawnPoints() > 0 && // cant be eliminated in koth/cts on maps that dont have spawn points
					SpawnPoint::s_numSpawnPoints[_teamId+1] == 0 &&
                    CountDarwinians(_teamId) < 50 );
            
        case GameTypeAssault:
        case GameTypeRocketRiot:
            return false;
    }

    return false;
}

void Multiwinia::RecordDefenseTime( double _time, int _teamId )
{
    if( m_pulseTimer == -1.0f ) m_pulseTimer = _time;
    else if( _time < m_pulseTimer ) m_pulseTimer = _time;
    m_pulseDetonated = true;
    if( !m_assaultSingleRound )
    {
        m_victoryTimer = 15.0f;
    }
	else 
	{
		m_victoryTimer = 10.0f;
	}
    m_timeRecords[_teamId] = _time;
}

int Multiwinia::GetMaxPointsAvail()
{
    switch( m_gameType )
    {
        case 1:         // Kind of the hill
            return s_numMultiwiniaZones * m_timeRemaining;

    }


    return 0;
}


bool Multiwinia::GetScoreLine( int _teamId, wchar_t *_statusLine, size_t bufferBytes )
{
	size_t bufferWChars = bufferBytes / sizeof(wchar_t);
    int teamIndex = _teamId;
    Team *team = g_app->m_location->m_teams[teamIndex];
    LobbyTeam *lobbyTeam = &m_teams[teamIndex];
    char *teamType = Team::GetTeamType( team->m_teamType );
    double h = 14;

    int currentScore = m_teams[teamIndex].m_score;
    if( g_app->m_location->m_teams[teamIndex]->m_monsterTeam ||
        g_app->m_location->m_teams[teamIndex]->m_futurewinianTeam )
    {
        if( currentScore == 0 ) 
			return false;
    }
    int scoreChangeInt = currentScore - m_teams[teamIndex].m_prevScore;
    char scoreChange[256];
    if      ( scoreChangeInt > 0 ) sprintf( scoreChange, "(+%d)", scoreChangeInt );
    else if ( scoreChangeInt < 0 ) sprintf( scoreChange, "(%d)", scoreChangeInt );
    else                           sprintf( scoreChange, "" );
    
    if( m_gameType == Multiwinia::GameTypeCaptureTheStatue )
    {
        int carryCount = 0;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && 
                    building->m_type == Building::TypeStatue &&
                    building->m_id.GetTeamId() == _teamId )
                {
                    ++carryCount;
                }
            }
        }

        if( carryCount > 0 ) sprintf( scoreChange, "(+%d)", carryCount );
    }

    if( fmodf(g_gameTimer.GetGameTime(), 1.0f) > 0.5f &&
        m_gameType != Multiwinia::GameTypeCaptureTheStatue ) sprintf( scoreChange, "" );

    int estimatedScore = currentScore + scoreChangeInt * m_timeRemaining;
    int maxScore = currentScore + GetMaxPointsAvail();

    char inSyncMsg[32];
    inSyncMsg[0] = '\x0';
    
    if( !g_app->m_clientToServer->IsSynchronised(team->m_clientId) ) 
    {
	    int numDifferences = m_teams[teamIndex].m_syncDifferences.Size();
	    sprintf( inSyncMsg, "Sync[%d]", numDifferences );				
    }

    char estScoreMsg[64];
    estScoreMsg[0] = '\x0';
    
    if( ((m_gameType == Multiwinia::GameTypeKingOfTheHill && GetGameOption("ScoreMode") != 2 ) || 
        (m_gameType == Multiwinia::GameTypeSkirmish && GetGameOption("ScoreMode") == 1 ) )
        && m_timeRemaining > -1 )
    {
        sprintf( estScoreMsg, "(est %d)", estimatedScore );
    }

    if( GameInGracePeriod() )
    {
        swprintf(_statusLine, bufferWChars, L"%-15ls  %ls", 
            team->GetTeamName().m_unicodestring, 
                    team->m_teamType == TeamTypeCPU ? LANGUAGEPHRASE("multiwinia_ready").m_unicodestring :
                    team->m_ready ? LANGUAGEPHRASE("multiwinia_ready").m_unicodestring : L" " );				
    }
    else if( m_gameType == Multiwinia::GameTypeRocketRiot )
    {
        // Rocket riot
        double fuel = 0;
        int passengers =0;
        int damage = 0;
        int countdown = -1;
        double fuelPressure = 0;

        EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );
        if( rocket && rocket->m_type == Building::TypeEscapeRocket )
        {
            fuel = rocket->m_fuel;
            passengers = rocket->m_passengers;
            damage = rocket->m_damage;
            fuelPressure = rocket->m_refuelRate * 100;
            if( rocket->m_state == EscapeRocket::StateCountdown ) countdown = rocket->m_countdown;
        }

        wchar_t passengersLine[256];
        wchar_t damageLine[256];
        wchar_t fuelLine[256];
		UnicodeString countdownEdited;

        passengersLine[0] = L'\x0';
        damageLine[0] = L'\x0';
        fuelLine[0] = L'\x0';
        
        if( passengers > 0 ) swprintf( passengersLine, sizeof(passengersLine)/sizeof(wchar_t), L"%ls %3d/100", LANGUAGEPHRASE("multiwinia_rr_passengers").m_unicodestring, passengers );
        if( damage > 0 ) swprintf( damageLine, sizeof(damageLine)/sizeof(wchar_t), L"%ls %3d%%", LANGUAGEPHRASE("multiwinia_rr_damage").m_unicodestring, damage );
        
        if( int(fuelPressure) > 0 ) swprintf( fuelLine, sizeof(fuelLine)/sizeof(wchar_t), L"(+%dpsi)", (int)fuelPressure );

        swprintf(_statusLine, bufferWChars, L"%-15ls  %-10ls %ls %3d%% %-10ls %-20ls %hs",
                            team->GetTeamName().m_unicodestring, damageLine, LANGUAGEPHRASE("multiwinia_rr_fuel").m_unicodestring, (int)fuel, fuelLine, passengersLine, inSyncMsg );

        if( countdown != -1 && fmodf(GetHighResTime() * 2, 2.0f) < 1.0f)
        {
            char cd[16];
            sprintf( cd, "%d", countdown+1);
            countdownEdited = LANGUAGEPHRASE("multiwinia_rr_liftoff");
            countdownEdited.ReplaceStringFlag( L'S', cd);

            swprintf(_statusLine, bufferWChars, L"%-15ls            %ls", team->GetTeamName().m_unicodestring, countdownEdited.m_unicodestring );
        }

    }
    else if( m_gameType == Multiwinia::GameTypeAssault )
    {
        if( g_app->m_location->IsDefending( teamIndex ) )
        {
            swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs", 
                team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_assault_holdtheline").m_unicodestring, inSyncMsg);	
        }
        else 
        {
            if( m_timeRecords[teamIndex] != -1.0f )
            {
                int minutesRemaining = m_timeRecords[teamIndex] / 60;
                int secondsRemaining = (int)m_timeRecords[teamIndex] % 60;

                minutesRemaining = max( minutesRemaining, 0 );
                secondsRemaining = max( secondsRemaining, 0 );

                char time[64];
                sprintf( time, "%d:%02d",minutesRemaining, secondsRemaining);

                swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs", 
                    team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_defendedfor_score").m_unicodestring, inSyncMsg);	
				UnicodeString str(_statusLine);
				str.ReplaceStringFlag( L'S', time );
                wcscpy(_statusLine, str.m_unicodestring);
            }
            else
            {
                swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs", 
                    team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_assault_ticktock").m_unicodestring, inSyncMsg);
            }
        }
    }
    else if( m_gameType == GameTypeTankBattle )
    {
        swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs", 
            team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_numtanks").m_unicodestring, inSyncMsg);
        char score[32];
        sprintf( score, "%d", currentScore );
		UnicodeString temp(_statusLine);
		temp.ReplaceStringFlag( L'X', score );
		wcscpy( _statusLine, temp.m_unicodestring );
    }
    else if( m_gameType == GameTypeSkirmish )
    {
        if( m_eliminationTimer[teamIndex] < 30 &&
            m_eliminationTimer[teamIndex] > 0 )
        {
            char pointsStringId[256];
            if( GetGameOption("ScoreMode") == 1 )
            {
                sprintf( pointsStringId, "multiwinia_points");
            }
            else
            {
                sprintf( pointsStringId, "multiwinia_dom_spawnpoints" );
            }

            swprintf(_statusLine, bufferWChars, L"%-15ls  %ls, %ls %hs", 
                team->GetTeamName().m_unicodestring, LANGUAGEPHRASE(pointsStringId).m_unicodestring,
                LANGUAGEPHRASE("multiwinia_dom_eliminatedin").m_unicodestring, inSyncMsg);	

            char score[32];
            sprintf( score, "%d", currentScore );
			UnicodeString temp(_statusLine);
			temp.ReplaceStringFlag( L'X', score );
            

            char time[32];
            sprintf( time, "%d", m_eliminationTimer[teamIndex] );
            temp.ReplaceStringFlag( 'S', time );
			wcscpy( _statusLine, temp.m_unicodestring );
        }
        else if( m_eliminated[teamIndex] )
        {
            swprintf(_statusLine, bufferWChars,  L"%-20ls  %ls %hs", 
                team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_dom_eliminated").m_unicodestring, inSyncMsg);	
        }
        else if( GetGameOption("ScoreMode") == 1 )
        {
            swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %5hs      %hs %hs", 
				team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_points").m_unicodestring, scoreChange, estScoreMsg, inSyncMsg );				

            char score[32];
            sprintf( score, "%d", currentScore );
			UnicodeString temp(_statusLine);
            temp.ReplaceStringFlag( 'X', score );
			wcscpy( _statusLine, temp.m_unicodestring );
        }
        else
        {
            swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs", 
                team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_dom_spawnpoints").m_unicodestring, inSyncMsg);	

            char score[32];
            sprintf( score, "%d", currentScore );
			UnicodeString temp(_statusLine);
			temp.ReplaceStringFlag( 'X', score );
			wcscpy(_statusLine, temp.m_unicodestring);
        }
    }
    else if( m_gameType == Multiwinia::GameTypeBlitzkreig )
    {
        UnicodeString bkStatus;

        if( currentScore < BLITZKRIEGSCORE_ELIMINATED )
        {
            bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_eliminated"); 
        }
        else
        {
            switch( currentScore )
            {
                case BLITZKRIEGSCORE_ENEMYOCCUPIED:     bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_enemyoccupied");	break;
                case BLITZKRIEGSCORE_UNDERATTACK:       bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_underattack");		break;
                case BLITZKRIEGSCORE_UNOCCUPIED:        bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_unoccupied");		break;
                case BLITZKRIEGSCORE_UNDERTHREAT:       bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_underthreat");		break;
                case BLITZKRIEGSCORE_SAFE:              bkStatus = LANGUAGEPHRASE("multiwinia_blitzkrieg_safe");			break;
            }
        }

        swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %hs",
                                team->GetTeamName().m_unicodestring,
                                bkStatus.m_unicodestring,
                                inSyncMsg );

    }
    else 
    {
        swprintf(_statusLine, bufferWChars, L"%-15ls  %ls %4hs     %hs %hs", 
            team->GetTeamName().m_unicodestring, LANGUAGEPHRASE("multiwinia_points").m_unicodestring, scoreChange, estScoreMsg, inSyncMsg );				

        char score[32];
        sprintf( score, "%d", currentScore );
		UnicodeString temp(_statusLine);
        temp.ReplaceStringFlag( 'X', score );
		wcscpy( _statusLine, temp.m_unicodestring );
    }

	delete [] teamType;

	return true;
}


bool Multiwinia::GameInGracePeriod()
{
    return( m_gracePeriodTimer > 0.0f );
}


void Multiwinia::Render()
{
    if( g_app->m_hideInterface ) return;

    START_PROFILE("Multiawinia::Render");

    float scale = (float)g_app->m_renderer->ScreenH() / 800.0f;

    float x = 20 * scale;
    float y = 25 * scale;
    float h = 14 * scale;

    //
    // Sort team scores

    bool somebodyScored = false;
    LList<int> orderedList;
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        //if( g_app->m_location->m_teams[t]->m_monsterTeam ) continue;
        //if( g_app->m_location->m_teams[t]->m_futurewinianTeam ) continue;
        Team *team = g_app->m_location->m_teams[t];
        if( g_app->m_location->m_teams[t]->m_monsterTeam ||
            g_app->m_location->m_teams[t]->m_futurewinianTeam )
        {
            if( m_teams[t].m_score == 0 ) continue;
        }
                
        if( team->m_teamType != TeamTypeUnused )
        {
            bool added = false;
            if( m_teams[t].m_score > 0 ) somebodyScored = true;

            for( int i = 0; i < orderedList.Size(); ++i )
            {
                bool higher = m_teams[t].m_score > m_teams[orderedList[i]].m_score;
                if( m_gameType == GameTypeAssault )
                {
                    higher = m_timeRecords[t] > m_timeRecords[orderedList[i]];
                }

                if( higher )
                {
                    orderedList.PutDataAtIndex( t, i );
                    added = true;
                    break;
                }
            }
            if( !added )
            {
                orderedList.PutDataAtEnd(t);
            }
        }
    }

    int group1Total = 0, group2Total = 0;
	int evilTotal = m_teams[g_app->m_location->GetMonsterTeamId()].m_score;
	int futureTotal = m_teams[g_app->m_location->GetFuturewinianTeamId()].m_score;
    LList<int> coopOrderedList[2];
    if( m_coopMode )
    {
        orderedList.Empty();
        for( int i = 0; i < NUM_TEAMS; ++ i )
        {
            if( g_app->m_location->m_coopGroups[i] == 1 )
            {
                if( m_teams[i].m_score > group1Total ) coopOrderedList[0].PutDataAtStart(i);
                else coopOrderedList[0].PutDataAtEnd(i);
                group1Total += m_teams[i].m_score;
            }
            else if( g_app->m_location->m_coopGroups[i] == 2 )
            {
                if( m_teams[i].m_score > group2Total ) coopOrderedList[1].PutDataAtStart(i);
                else coopOrderedList[1].PutDataAtEnd(i);
                group2Total += m_teams[i].m_score;
            }
        }

        orderedList.PutData(1);
        if( group1Total > group2Total ) orderedList.PutDataAtEnd(2);
        else orderedList.PutDataAtStart(2);

        if( m_teams[g_app->m_location->GetMonsterTeamId()].m_score > 0 ) orderedList.PutDataAtEnd(g_app->m_location->GetMonsterTeamId());
        if( m_teams[g_app->m_location->GetFuturewinianTeamId()].m_score > 0 ) orderedList.PutDataAtEnd(g_app->m_location->GetFuturewinianTeamId());
            


    }


    //
    // Render team scores
    // Background box first

    float boxX = 10;
    float boxY = 10;
    float boxW = 440;
    float boxH = 50 + orderedList.Size() * 14.0f * 1.3f;

    if( !g_app->m_multiwiniaTutorial || somebodyScored )
    {
        glColor4f( 0.0f, 0.0f, 0.0f, 0.75f );

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable        ( GL_BLEND );

        if( m_coopMode && !GameOver() )
        {
            if( m_gameType == GameTypeAssault )
            {
                boxH *= 1.4f;
            }
            else
            {
                boxH *= 2.0f;
            }
        }

        if( m_gameType == GameTypeRocketRiot )
        {
            boxW = 530;
        }

        /*if( m_gameType == GameTypeAssault )
        {
            boxH += h * 4 * 1.3f;
        }*/

        if( GameOver() )
        {
            float screenCentre = g_app->m_renderer->ScreenW()/2;
            boxW *= scale;
            boxX = screenCentre - boxW / 2.0f;
            boxY = g_app->m_renderer->ScreenH() * 0.73f;
            boxH *= 1.3f * scale;
        }
        else
        {
            boxW *= scale;
            boxH *= scale;
            boxX *= scale;
            boxY *= scale;
        }

        if( !GameOver() ||
            ( m_gameType != GameTypeTankBattle &&
              m_gameType != GameTypeBlitzkreig &&
            ( m_gameType != GameTypeAssault || !m_assaultSingleRound) ))
        {
            glBegin( GL_QUADS );
                glVertex2i( boxX, boxY );
                glVertex2i( boxX + boxW, boxY );
                glVertex2i( boxX + boxW, boxY + boxH );
                glVertex2i( boxX, boxY + boxH );
            glEnd();

            glDisable       ( GL_TEXTURE_2D );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
           
            glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
            glBegin( GL_LINE_LOOP );
                glVertex2i( boxX, boxY );
                glVertex2i( boxX + boxW, boxY );
                glVertex2i( boxX + boxW, boxY + boxH );
                glVertex2i( boxX, boxY + boxH );
            glEnd();
        }

        if( !GameOver() )
		{
            UnicodeString scoresTitle = LANGUAGEPHRASE("multiwinia_teamscores");
            if( m_gameType == Multiwinia::GameTypeAssault ) scoresTitle = LANGUAGEPHRASE("multiwinia_teamscores_assault");

            glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2DSimple( x, y, h * 2, scoresTitle );

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2DSimple( x, y, h * 2, scoresTitle );

            y += 35 * scale;
        
            /*if( m_gameType == GameTypeAssault )
            {
                for( int i = 0; i < m_assaultObjectives.Size(); ++i )
				{
                    int buildingId = m_assaultObjectives[i];

                    Building *building = g_app->m_location->GetBuilding( buildingId );
                    if( building )
                    {
                        UnicodeString objType;
                        UnicodeString objStatus = LANGUAGEPHRASE("multiwinia_assault_unoccupied");

                        switch( building->m_type )
                        {
                        case Building::TypeControlStation:      
                            objType = LANGUAGEPHRASE("multiwinia_assault_objcntrlstation");     
                            break;

                        case Building::TypePulseBomb:           
                            objType = LANGUAGEPHRASE("multiwinia_assault_objpulsebomb");        
                            break;
                        }

                        RGBAColour teamCol(255,255,255,128);

                        if( building->m_id.GetTeamId() >= 0 &&
                            building->m_id.GetTeamId() < NUM_TEAMS )
                        {
                            teamCol = g_app->m_location->m_teams[building->m_id.GetTeamId()]->m_colour;

                            if( building->m_destroyed )
                            {
                                teamCol = GetColour(ASSAULT_ATTACKER_COLOURID);
                                objStatus = LANGUAGEPHRASE("multiwinia_assault_destroyed");
                            }
                            else if( g_app->m_location->IsDefending( building->m_id.GetTeamId() ) )
                            {
                                objStatus = LANGUAGEPHRASE("multiwinia_assault_defended");
                            }
                            else if( g_app->m_location->IsAttacking( building->m_id.GetTeamId() ) )
                            {
                                objStatus = LANGUAGEPHRASE("multiwinia_assault_lost");
                            }
                        }

                        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                        g_editorFont.SetRenderOutline(true);
                        g_editorFont.DrawText2DSimple( x, y, h, objType );

                        glColor4ubv( teamCol.GetData() );
                        g_editorFont.SetRenderOutline(false);
                        g_editorFont.DrawText2DSimple( x, y, h, objType );

                        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                        g_editorFont.SetRenderOutline(true);
                        g_editorFont.DrawText2DRight( boxX+boxW-50, y, h, objStatus );

                        glColor4ubv( teamCol.GetData() );
                        g_editorFont.SetRenderOutline(false);
                        g_editorFont.DrawText2DRight( boxX+boxW-50, y, h, objStatus );

                        y += h * 1.3f;
                    }
                }

                y+=h;
            }*/

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        
            for( int t = 0; t < orderedList.Size(); ++t )
            {
                bool special = false;
                if( orderedList[t] == g_app->m_location->GetMonsterTeamId() ||
                    orderedList[t] == g_app->m_location->GetFuturewinianTeamId() )
                {
                    special = true;
                }

                if( m_coopMode && !special )
                {
                    int groupId = orderedList[t];
                    int scoreTotal = 0;
                    if( groupId == 1 ) scoreTotal = group1Total;
                    else scoreTotal = group2Total;
                    //char teamLine[512];
                    UnicodeString teamName;
                    switch( g_app->m_location->m_groupColourId[ groupId - 1 ] )
                    {
                        case 0: teamName = LANGUAGEPHRASE("multiwinia_team_green");     break;
                        case 1: teamName = LANGUAGEPHRASE("multiwinia_team_red");       break;
                        case 2: teamName = LANGUAGEPHRASE("multiwinia_team_yellow");    break;
                        case 3: teamName = LANGUAGEPHRASE("multiwinia_team_blue");      break;
                        case 4: teamName = LANGUAGEPHRASE("multiwinia_team_orange");    break;
                        case 5: teamName = LANGUAGEPHRASE("multiwinia_team_purple");    break;
                        case 6: teamName = LANGUAGEPHRASE("multiwinia_team_cyan");      break;
                        case 7: teamName = LANGUAGEPHRASE("multiwinia_team_pink");      break;

                        default:    
							//teamName = UnicodeString("BAD TEAM NAME"); 
							teamName = orderedList[t] == g_app->m_location->GetMonsterTeamId() ?
								LANGUAGEPHRASE("multiwinia_team_virus") :
								LANGUAGEPHRASE("multiwinia_team_future");
							break;
                    }
                    //sprintf( teamLine, "Team %d     %d points", groupId, scoreTotal );
                    UnicodeString teamLine = LANGUAGEPHRASE("multiwinia_cooppoints");
                    char scoreString[16];
                    sprintf( scoreString, "%d", scoreTotal );
                    teamLine.ReplaceStringFlag( 'T', teamName );
                    teamLine.ReplaceStringFlag( 'S', scoreString );

                    if( m_gameType != Multiwinia::GameTypeAssault )
                    {
                        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                        g_editorFont.SetRenderOutline(true);
                        g_editorFont.DrawText2DSimple( x, y, h * 1.5f, teamLine );

                        glColor4ubv( GetColour( g_app->m_location->m_groupColourId[ groupId-1] ).GetData() );
                        g_editorFont.SetRenderOutline(false);
                        g_editorFont.DrawText2DSimple( x, y, h * 1.5f, teamLine );
                        
                        y += h * 1.5f * 1.3f;
                    }

                    for( int i = 0; i < coopOrderedList[groupId-1].Size(); ++i )
                    {
                        //if( g_app->m_location->m_coopGroups[i] == groupId )
                        {
                            int index = coopOrderedList[groupId-1][i];
                            Team *team = g_app->m_location->m_teams[index];
						    wchar_t statusLine[1024];

                            if( GetScoreLine( index, statusLine, sizeof(statusLine) ) )
                            {
                                UnicodeString status(statusLine);
                                if( index == g_app->m_globalWorld->m_myTeamId )
                                {
                                    float w = boxW - 20 * scale;
                                    glColor4f( 0.7f, 0.7f, 0.5f, 0.7f );
                                    glBegin( GL_QUADS );
                                        glVertex2f( x - 5 * scale, y - 10 * scale );
                                        glVertex2f( x + w, y - 10 * scale );
                                        glVertex2f( x + w, y + h - 4 * scale );
                                        glVertex2f( x - 5 * scale, y + h - 4 * scale );
                                    glEnd();
                                }

                                glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                                g_editorFont.SetRenderOutline(true);
		                        g_editorFont.DrawText2DSimple( x+ 50 * scale, y, h, status );

                                if( team->m_monsterTeam )
                                {
                                    glColor4f( 0.6f, 0.6f, 0.6f, 1.0f );
                                }
                                else
                                {
                                    glColor4ubv( team->m_colour.GetData() );
                                }
                                g_editorFont.SetRenderOutline(false);
                                g_editorFont.DrawText2DSimple( x + 50 * scale, y, h, status );
                                
                                y += h * 1.3f;
                            }
                        }
                    }
                }
                else
                {
                    int teamIndex = orderedList[t];
                    Team *team = g_app->m_location->m_teams[teamIndex];
                    LobbyTeam *lobbyTeam = &m_teams[teamIndex];

                    if( team->m_teamType != TeamTypeUnused )
                    {
                        wchar_t statusLine[1024];

                        if( GetScoreLine( teamIndex, statusLine, sizeof(statusLine) ) )
                        {
                            UnicodeString status( statusLine );
                            if( teamIndex == g_app->m_globalWorld->m_myTeamId )
                            {
                                float w = boxW - 20 * scale;
                                glColor4f( 0.7f, 0.7f, 0.5f, 0.7f );
                                glBegin( GL_QUADS );
                                    glVertex2f( x - 5 * scale, y - 10 );
                                    glVertex2f( x + w, y - 10 );
                                    glVertex2f( x + w, y + h - 4 );
                                    glVertex2f( x - 5 * scale, y + h - 4 );
                                glEnd();
                            }

                            glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                            g_editorFont.SetRenderOutline(true);
		                    g_editorFont.DrawText2DSimple( x, y, h, status );

                            if( team->m_monsterTeam )
                            {
                                glColor4f( 0.6f, 0.6f, 0.6f, 1.0f );
                            }
                            else
                            {
                                glColor4ubv( team->m_colour.GetData() );
                            }
                            g_editorFont.SetRenderOutline(false);
                            g_editorFont.DrawText2DSimple( x, y, h, status );
                            
                            y += h * 1.3f;
                        }
                    }
                }
            }
        }        
        else if( m_gameType != GameTypeTankBattle &&
            ( m_gameType != GameTypeAssault || !m_assaultSingleRound) &&
            m_gameType != GameTypeBlitzkreig )
		{
            // Scores at game over 
            float screenCentre = g_app->m_renderer->ScreenW()/2;
            int y = g_app->m_renderer->ScreenH() * 0.73f;
            y += 15 * scale;
            float fontSize = 30.0f * scale;

            for( int i = 0; i < orderedList.Size(); ++i )
            {
                int teamIndex = orderedList[i];
                RGBAColour col;

                wchar_t scoreString[512];

                if( m_coopMode )
				{
                    int score = 0;
                    if( teamIndex == 1 ) score = group1Total;
                    else if( teamIndex == 2 ) score = group2Total;
					else if( teamIndex == 4 ) score = evilTotal;
					else if( teamIndex == 5 ) score = futureTotal;
					else score = -1;
                    //sprintf( scoreString, "Team %d       %d points", teamIndex, score );

					if( teamIndex < 3 )
						col = GetColour( g_app->m_location->m_groupColourId[teamIndex-1] );
					else
						col = m_teams[teamIndex].m_colour;

                    UnicodeString teamName;
                    switch( g_app->m_location->m_groupColourId[ teamIndex - 1 ] )
                    {
                        case 0: teamName = LANGUAGEPHRASE("multiwinia_team_green");     break;
                        case 1: teamName = LANGUAGEPHRASE("multiwinia_team_red");       break;
                        case 2: teamName = LANGUAGEPHRASE("multiwinia_team_yellow");    break;
                        case 3: teamName = LANGUAGEPHRASE("multiwinia_team_blue");      break;
                        case 4: teamName = LANGUAGEPHRASE("multiwinia_team_orange");    break;
                        case 5: teamName = LANGUAGEPHRASE("multiwinia_team_purple");    break;
                        case 6: teamName = LANGUAGEPHRASE("multiwinia_team_cyan");      break;
                        case 7: teamName = LANGUAGEPHRASE("multiwinia_team_pink");      break;

                        default:    
							// Evilinians or Futurwinians
							teamName = g_app->m_location->m_teams[teamIndex]->m_monsterTeam ?
								LANGUAGEPHRASE("multiwinia_team_virus") :
								LANGUAGEPHRASE("multiwinia_team_future");
							break;
                    }
                    //sprintf( teamLine, "Team %d     %d points", groupId, scoreTotal );
                    UnicodeString teamLine = LANGUAGEPHRASE("multiwinia_cooppoints");
                    char temp[16];
                    sprintf( temp, "%d", score );
                    teamLine.ReplaceStringFlag( 'T', teamName );
                    teamLine.ReplaceStringFlag( 'S', temp );
                    wcscpy( scoreString, teamLine.m_unicodestring );

                }
                else
                {
                    Team *team = g_app->m_location->m_teams[teamIndex];
                    LobbyTeam *lobbyTeam = &m_teams[teamIndex];
                    col = team->m_colour;
                    switch( m_gameType )
                    {
                        case GameTypeSkirmish:
                        case GameTypeCaptureTheStatue:
                        case GameTypeKingOfTheHill:     
                            swprintf( scoreString, sizeof(scoreString)/sizeof(wchar_t),
									  L"%-15ls   %-4d", team->GetTeamName().m_unicodestring, lobbyTeam->m_score );     
                            break; 

                        case GameTypeBlitzkreig:
                            GetScoreLine( teamIndex, scoreString, sizeof(scoreString) );
                            break;

                        case GameTypeAssault:
                        {
                            int minutes = m_timeRecords[teamIndex] / 60;
                            int seconds = (int)m_timeRecords[teamIndex] % 60;

                            minutes = max( minutes, 0 );
                            seconds = max( seconds, 0 );
                            wchar_t teamNameString[256];
                            swprintf( teamNameString, sizeof(teamNameString)/sizeof(wchar_t),
									  L"%-15ls", team->GetTeamName().m_unicodestring );
                            char timeString[256];
                            sprintf( timeString, "%d:%02d",minutes, seconds );
                            //sprintf( scoreString, "%-15s defended for %d:%02d", team->GetTeamName().m_charstring, minutes, seconds );
                            UnicodeString msg = LANGUAGEPHRASE("multiwinia_defendedfor");
                            msg.ReplaceStringFlag( L'T', teamNameString );
                            msg.ReplaceStringFlag( L'S', timeString );
                            wcscpy( scoreString, msg.m_unicodestring );
                        }
                        break;

                        case GameTypeRocketRiot:
                        {
                            swprintf( scoreString, sizeof(scoreString)/sizeof(wchar_t),
									  L"%-15ls", team->GetTeamName().m_unicodestring );
                        }
                        break;

                    }
                }

                glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
                g_editorFont.SetRenderOutline(true);
                g_editorFont.DrawText2DCentre( screenCentre, y, fontSize, scoreString );

                glColor4ubv( col.GetData() );
                g_editorFont.SetRenderOutline(false);
                g_editorFont.DrawText2DCentre( screenCentre, y, fontSize, scoreString );

                y+= fontSize * 1.2f;
            }
        }
    }


    //
    // Render time limit if applicable

    bool tutorialBlock = false;
    if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 &&
        !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
    {
        tutorialBlock = true;
    }

    if( m_timeRemaining > 0 && !g_app->m_multiwinia->GameInGracePeriod() && !tutorialBlock )
    {        
        int minutesRemaining = m_timeRemaining / 60;
        int secondsRemaining = m_timeRemaining % 60;

        float x = g_app->m_renderer->ScreenW()/2.0f;
        if( g_app->m_renderer->ScreenW() <= 800 )
        {
            x = boxX + boxW + (100 * scale);
        }
        float y = 30 * scale;
		float boxSize = 80 * scale;
		float fontSize = 30 * scale;

        glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );

		if( secondsRemaining == 0 || m_timeRemaining <= 10 )
		{
			static int s_lastTimeRemaining = -1;
			if( s_lastTimeRemaining != m_timeRemaining )
			{
				s_lastTimeRemaining = m_timeRemaining;
				g_app->m_soundSystem->TriggerOtherEvent( NULL, "CountdownTick", SoundSourceBlueprint::TypeMultiwiniaInterface );

                if( m_timeRemaining == 0 )
                {
                    g_app->m_soundSystem->TriggerOtherEvent( NULL, "CountdownZeroTick", SoundSourceBlueprint::TypeMultiwiniaInterface );
                }
			}

			if( m_timeRemaining > 0 )
			{
				glColor4f( m_timer * 1.0f, m_timer * 1.0f, m_timer * 0.8f, 0.75f );
			}
		}

		if( m_timeRemaining <= 10 )
		{
			boxSize *= 1.2f;
			fontSize *= 1.2f;
		}

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glBegin( GL_QUADS );
            glVertex2i( x - boxSize, y - boxSize/4.0f );
            glVertex2i( x + boxSize, y - boxSize/4.0f );
            glVertex2i( x + boxSize, y + boxSize/4.0f + boxSize/8.0f );
            glVertex2i( x - boxSize, y + boxSize/4.0f + boxSize/8.0f );
        glEnd();

        glDisable       ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		if( secondsRemaining == 0 || m_timeRemaining <= 10)
		{
			if( m_timeRemaining > 0 )
			{
				glColor4f( 1.0f, 1.0f, 1.0f, 0.1f + m_timer * 0.9f );
			}
		}

        glBegin( GL_LINE_LOOP );
            glVertex2i( x - boxSize, y - boxSize/4.0f );
            glVertex2i( x + boxSize, y - boxSize/4.0f );
            glVertex2i( x + boxSize, y + boxSize/4.0f + boxSize/8.0f  );
            glVertex2i( x - boxSize, y + boxSize/4.0f + boxSize/8.0f  );
        glEnd();

        char caption[256];
        sprintf( caption, "%d%c%02d", 
                            minutesRemaining, 
                            fmodf( g_gameTimer.GetGameTime(), 1.0f ) < 0.5f ? ':' : ' ', 
                            secondsRemaining );

        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( x, y - 1, fontSize, caption );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( m_timeRemaining <= 10 )
        {
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        }

        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( x, y - 1, fontSize, caption );
    }

   
    bool eliminated = g_app->m_globalWorld->m_myTeamId >= 0 &&
                      g_app->m_globalWorld->m_myTeamId < NUM_TEAMS &&
                      IsEliminated(g_app->m_globalWorld->m_myTeamId);

    if( !eliminated && m_reinforcementCountdown != 0 )       // Rocket Riot
    {
        y += 20 * scale;

        UnicodeString msg = LANGUAGEPHRASE("multiwinia_reinforcements");
        char time[16];
        sprintf( time, "%d", m_reinforcementCountdown );
        msg.ReplaceStringFlag( L'S', time );

        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2D( x, y, 20 * scale, msg );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( m_reinforcementCountdown <= 5 )
        {
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        }
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2D( x, y, 20 * scale, msg );

    }

    //
    // Retribution counter

    if( eliminated && !GameInGracePeriod() && GetGameOption( "RetributionMode" ) )
    {
        int retributionCounter = m_eliminationBonusTimer[g_app->m_globalWorld->m_myTeamId];

        y += 20 * scale;

        UnicodeString msg = LANGUAGEPHRASE("multiwinia_retribution_timer");
        char time[16];
        sprintf( time, "%d", retributionCounter );
        msg.ReplaceStringFlag( L'S', time );

        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2D( x, y, 20 * scale, msg );

        glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2D( x, y, 20 * scale, msg );
    }


    y = 50 * scale;
    x = g_app->m_renderer->ScreenW() / 2.0f;
    if( g_app->m_location->m_spawnManiaTimer > 0.0f )
    {
        y += 20 * scale;

        char timer[16];
        sprintf( timer, "%1.0f", g_app->m_location->m_spawnManiaTimer );
        UnicodeString msg = LANGUAGEPHRASE("multiwinia_spawnmania_timer");
        msg.ReplaceStringFlag( L'T', timer );

        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( x, y, 50 * scale, msg );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( g_app->m_location->m_spawnManiaTimer < 2 )
        {
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        }

        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( x, y, 50 * scale, msg);

        y += 50 * scale;
    }

    if( g_app->m_location->m_bitzkreigTimer > 0.0f )
    {
        y += 20 * scale;

        char timer[16];
        sprintf( timer, "%1.0f", g_app->m_location->m_bitzkreigTimer );
        UnicodeString msg = LANGUAGEPHRASE("multiwinia_blitzkrieg_timer");
        msg.ReplaceStringFlag( L'T', timer );

        glColor4f( 0.75f, 0.75f, 0.75f, 0.0f );
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( x, y, 50 * scale, msg );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( g_app->m_location->m_bitzkreigTimer < 2 )
        {
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
        }

        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( x, y, 50 * scale, msg );

    }
    

    //
    // Render winner if game over

    if( m_gameType == GameTypeAssault )
    {
        // check to see if the pulse bomb has detonated, and if so render defense time and next round countdown
        if( m_pulseDetonated && m_numRotations < g_app->m_location->m_levelFile->m_numPlayers - 1 )
        {
            if( !m_assaultSingleRound )
            {
                //find defender
                int defender = -1;
                for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
                {
                    if( !g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(i)] )
                    {
                        defender = i;
                        break;
                    }
                }

                if( defender != -1 )
                {
                    int minutesRemaining = m_timeRecords[defender] / 60;
                    int secondsRemaining = (int)m_timeRecords[defender] % 60;

                    minutesRemaining = max( minutesRemaining, 0 );
                    secondsRemaining = max( secondsRemaining, 0 );

                    float screenCentre = g_app->m_renderer->ScreenW()/2;

                    char timeString[32];
                    sprintf( timeString, "%d:%02d", minutesRemaining, secondsRemaining );
                    UnicodeString defended = LANGUAGEPHRASE("multiwinia_defendedfor");
                    defended.ReplaceStringFlag( L'T', g_app->m_location->m_teams[defender]->GetTeamName() );
                    defended.ReplaceStringFlag( L'S', timeString );

                    glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                    g_gameFont.SetRenderOutline(true);
                    g_gameFont.DrawText2DCentre( screenCentre, 200, 50, defended );

                    RGBAColour col = m_teams[defender].m_colour;
                    glColor4ubv( col.GetData() );
                    g_gameFont.SetRenderOutline(false);
                    g_gameFont.DrawText2DCentre( screenCentre, 200, 50, defended );

                    if( m_victoryTimer < 11.0f )
                    {
                        char time[16];
                        sprintf( time, "%d", (int)m_victoryTimer );
                        UnicodeString msg = LANGUAGEPHRASE("multiwinia_nextroundin");
                        msg.ReplaceStringFlag( L'S', time );

                        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_gameFont.SetRenderOutline(true);
                        g_gameFont.DrawText2DCentre( screenCentre, 400, 100, msg );

                        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                        g_gameFont.SetRenderOutline(false);
                        g_gameFont.DrawText2DCentre( screenCentre, 400, 100, msg );
                    }
                }
            }
            else if( m_winner != -1 )
            {
                float screenCentre = g_app->m_renderer->ScreenW()/2;
                int y = 300 * scale;                
                float fontSize = 50.0f * scale;

                UnicodeString statusString;

                if( m_coopMode )
                {
                    if( g_app->m_location->m_levelFile->m_attacking[ g_app->m_location->GetTeamPosition( m_winner ) ] )
                    {
                        statusString = LANGUAGEPHRASE("multiwinia_attackers_win");
                    }
                    else
                    {
                        statusString = LANGUAGEPHRASE("multiwinia_defenders_win");
                    }
                }
                else
                {
                    statusString = LANGUAGEPHRASE("multiwinia_team_win");
                    statusString.ReplaceStringFlag( L'T', g_app->m_location->m_teams[m_winner]->GetTeamName() );
                }

                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_gameFont.SetRenderOutline(true);
                g_gameFont.DrawText2DCentre( screenCentre, y, fontSize, statusString );

                glColor4ubv( GetColour( m_teams[m_winner].m_colourId ).GetData() );
                g_gameFont.SetRenderOutline(false);
                g_gameFont.DrawText2DCentre( screenCentre, y, fontSize, statusString );
            }
        }
    }

    
    //
    // Grace period information

    if( GameInGracePeriod() )
    {
        RenderOverlay_GracePeriod();
    }



    //
    // Render winner if one has been declared

    if( GameOver() )
    {
        float screenCentre = g_app->m_renderer->ScreenW()/2;
        
        float yPos = 180 * scale;
        float fontSize = 70 * scale;

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, LANGUAGEPHRASE("multiwinia_gameover") );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, LANGUAGEPHRASE("multiwinia_gameover") );

        yPos = 250 * scale;
        fontSize = 35 * scale;

        if( m_gameType != GameTypeAssault ||
            !m_assaultSingleRound )
        {
            if( m_coopMode )
            {
                int winner = 1;
				int greatestScore = group1Total;
				if( group2Total > group1Total ) 
				{
					winner = 2;
					greatestScore = group2Total;
				}

				if( evilTotal > greatestScore )
				{
					winner = 3;
					greatestScore = evilTotal;
				}
				
				if( futureTotal > greatestScore )
				{
					winner = 4;
					greatestScore = futureTotal;
				}

                //char winnerString[256];
                //sprintf( winnerString, "%d", winner );
                UnicodeString winnerString;
                switch( g_app->m_location->m_groupColourId[ winner - 1 ] )
                {
                    case 0: winnerString = LANGUAGEPHRASE("multiwinia_team_green");     break;
                    case 1: winnerString = LANGUAGEPHRASE("multiwinia_team_red");       break;
                    case 2: winnerString = LANGUAGEPHRASE("multiwinia_team_yellow");    break;
                    case 3: winnerString = LANGUAGEPHRASE("multiwinia_team_blue");      break;
                    case 4: winnerString = LANGUAGEPHRASE("multiwinia_team_orange");    break;
                    case 5: winnerString = LANGUAGEPHRASE("multiwinia_team_purple");    break;
                    case 6: winnerString = LANGUAGEPHRASE("multiwinia_team_cyan");      break;
                    case 7: winnerString = LANGUAGEPHRASE("multiwinia_team_pink");      break;

                    default:    
						//winnerString = UnicodeString("BAD TEAM NAME");        
						winnerString = winner == 3 ?
							LANGUAGEPHRASE("multiwinia_team_virus") : 
							LANGUAGEPHRASE("multiwinia_team_future");
						break;
                }
                UnicodeString msg = LANGUAGEPHRASE("multiwinia_isthewinner");
                msg.ReplaceStringFlag( L'T', winnerString );

                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_titleFont.SetRenderOutline(true);
                g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, msg );

				if( winner < 3 )            glColor4ubv( GetColour( g_app->m_location->m_groupColourId[winner-1] ).GetData() );
				else if( winner == 3 )      glColor4ubv( m_teams[g_app->m_location->GetMonsterTeamId()].m_colour.GetData() );
				else                        glColor4ubv( m_teams[g_app->m_location->GetFuturewinianTeamId()].m_colour.GetData() );
                
                g_titleFont.SetRenderOutline(false);
                g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, msg );

            }
            else
            {
                if( m_winner != -1 )
                {
                    if( m_winner == MULTIWINIA_DRAW_TEAM )
                    {
                        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_titleFont.SetRenderOutline(true);
                        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, LANGUAGEPHRASE("multiwinia_tiegame") );

                        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                        g_titleFont.SetRenderOutline(false);
                        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, LANGUAGEPHRASE("multiwinia_tiegame") );
                    }
                    else
                    {
                        Team *team = g_app->m_location->m_teams[m_winner];

                        UnicodeString msg = LANGUAGEPHRASE("multiwinia_isthewinner");
                        msg.ReplaceStringFlag( L'T', team->GetTeamName() );
                        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                        g_titleFont.SetRenderOutline(true);
                        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, msg );

                        RGBAColour col = m_teams[m_winner].m_colour;
                        glColor4ubv( col.GetData() );
                        g_titleFont.SetRenderOutline(false);
                        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, msg );
                    }
                }
            }
        }

        UnicodeString exitString;
        if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
        {
            exitString = LANGUAGEPHRASE("multiwinia_atoexit");
        }
        else
        {
            exitString = LANGUAGEPHRASE("multiwinia_spacetoexit");
        }
        yPos = g_app->m_renderer->ScreenH() - fontSize * 1.2f;
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, exitString);

        float alpha = ( fmodf(GetHighResTime(),1) < 0.3f ? 1.0f : 0.2f );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2DCentre( screenCentre, yPos, fontSize, exitString);

    }


    if( m_gameType == GameTypeRocketRiot )
    {
        RenderRocketStatusPanel();
    }

    END_PROFILE("Multiawinia::Render");
}


void Multiwinia::RenderRocketStatusPanel()
{
    if( !m_rocketStatusPanel )
    {
        m_rocketStatusPanel = new RocketStatusPanel();
    }

    m_rocketStatusPanel->m_teamId = g_app->m_globalWorld->m_myTeamId;
    m_rocketStatusPanel->m_w = g_app->m_renderer->ScreenH() * 0.2f;
    m_rocketStatusPanel->m_x = g_app->m_renderer->ScreenW() - m_rocketStatusPanel->m_w - 10;
    m_rocketStatusPanel->m_y = 10;
    m_rocketStatusPanel->Render();
}


void Multiwinia::RenderOverlay_GracePeriod()
{
    //glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
    //g_gameFont.SetRenderOutline(true);
    //g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2, yPos, fontSize, "GAME PAUSED" );
    //
    //glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    //g_gameFont.SetRenderOutline(false);
    //g_gameFont.DrawText2DCentre( g_app->m_renderer->ScreenW()/2, yPos, fontSize, "GAME PAUSED" );

    float xPos = g_app->m_renderer->ScreenW()/2.0f;
    float yPos = 20;
    float fontSize = g_app->m_renderer->ScreenH() * 0.04f;

    int numReady = 0;
    int numFound = 0;
    bool usReady = false;

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        Team *team = g_app->m_location->m_teams[i];
        if( team->m_teamType == TeamTypeLocalPlayer ||
            team->m_teamType == TeamTypeRemotePlayer ||
			team->m_teamType == TeamTypeSpectator)
        {
            ++numFound;
            if( team->m_ready ) 
            {
                ++numReady;
                if( i == g_app->m_globalWorld->m_myTeamId )
                {
                    usReady = true;
                }
            }
        }
    }

    //yPos += fontSize * 0.1f;    

    UnicodeString statusMsg;

    if( m_gracePeriodTimer < GRACE_TIME )     
    {
        statusMsg = LANGUAGEPHRASE("multiwinia_gamestarttimer");
        char t[64];
        sprintf( t, "%d", (int)(m_gracePeriodTimer+1.0f) );
        UnicodeString time(t);
        statusMsg.ReplaceStringFlag(L'T', time );    
    }
    else if( usReady )
    {
        char players[128];
        sprintf( players, " (%d/%d)", numReady, numFound );
        statusMsg = LANGUAGEPHRASE("multiwinia_waitingforplayers") + UnicodeString(players);
    }
    else if( !usReady )
    {
        if( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD )     statusMsg = LANGUAGEPHRASE("multiwinia_spacetostart");
        else                                                            statusMsg = LANGUAGEPHRASE("multiwinia_atostart");
    }

    float alpha = ( fmodf(GetHighResTime(),1) < 0.3f ? 1.0f : 0.2f );
    if( m_gracePeriodTimer < GRACE_TIME ) 
    {
        alpha = 1.0f;
        fontSize *= 1.25f;
    }

    glColor4f( alpha, alpha, alpha, 0.0f );
    g_titleFont.SetRenderOutline(true);
    g_titleFont.DrawText2DCentre( xPos, yPos, fontSize, statusMsg );

    glColor4f( 1.0f, 0.0f, 0.0f, alpha );
    g_titleFont.SetRenderOutline(false);
    g_titleFont.DrawText2DCentre( xPos, yPos, fontSize, statusMsg );
}

int Multiwinia::GetNumHumanTeams() const
{
	int result = 0;
	for( int i = 0; i < NUM_TEAMS; i++ )
	{
		if( m_teams[i].m_teamType == TeamTypeLocalPlayer || 
			m_teams[i].m_teamType == TeamTypeRemotePlayer )
			result++;
	}
	return result;
}

int Multiwinia::GetNumCpuTeams() const
{
	int result = 0;
	for( int i = 0; i < NUM_TEAMS; i++ )
	{
		if( m_teams[i].m_teamType == TeamTypeCPU )
			result++;
	}
	return result;
}

std::string Multiwinia::GameTypeString( int _gameType ) 
{
	if( s_gameBlueprints.ValidIndex( _gameType ) )
		return s_gameBlueprints[_gameType]->GetName();
	else
		return "multiwinia_gametype_unknown";
}


void Multiwinia::DeclareWinner( int _teamId )
{
	Team* t = g_app->m_location->GetMyTeam();
	if( t ) g_app->m_achievementTracker->AddToTotalKills(t->m_teamKills);

	g_app->m_clientToServer->SendTeamScores( _teamId );

    if( m_winner != _teamId )
    {
        m_winner = _teamId;
		
        //
        // Stop time

        g_gameTimer.RequestSpeedChange( 0.0f, 0.2f, -1.0f );
        g_gameTimer.LockSpeedChange();


        //
        // Trigger sound events

        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GameOver", SoundSourceBlueprint::TypeMultiwiniaInterface );

        if( m_winner == g_app->m_globalWorld->m_myTeamId )
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "YouWon", SoundSourceBlueprint::TypeMultiwiniaInterface );
        }
        else
        {
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "YouLost", SoundSourceBlueprint::TypeMultiwiniaInterface );
        }

        if( _teamId == g_app->m_globalWorld->m_myTeamId )
        {
            g_app->m_achievementTracker->AddVictoryOnLevel( g_app->m_location->m_levelFile->m_mapFilename );

			CheckMasterAchievement();
        }
    }
}

void Multiwinia::CheckMasterAchievement()
{
	AppDebugOut("Checking master achievement\n");
    if( !g_app->EarnedAchievement( App::DarwiniaAchievementMasterPlayer ) )
    {
        AppDebugOut("Player has not yet earned MasterPlayer achievement\n");
		if( !m_coopMode && m_winner == g_app->m_location->GetMyTeam()->m_teamId )
		{
			for( int i = 0; i < NUM_TEAMS; i++ )
			{
				if( m_teams[i].m_teamType != TeamTypeCPU &&
					m_teams[i].m_teamType != TeamTypeUnused &&
					m_teams[i].m_teamId != g_app->m_location->GetMyTeam()->m_teamId &&		// This is not us
					(m_teams[i].m_achievementMask & VIRAL_MASTER_ACHIEVEMENT_MASK) )		// This is master player
				{
					g_app->GiveAchievement(App::DarwiniaAchievementMasterPlayer);
					g_app->m_achievementTracker->m_isMasterPlayer = true;
					break;
				}
			}
		}
    }
}

void Multiwinia::CheckMentorAchievement()
{
    if( !g_app->m_server ) return;
}

void Multiwinia::ZeroScores()
{
	for (int i = 0; i < NUM_TEAMS; i++)
	{
		m_teams[i].m_score = 0;
	}
}


void Multiwinia::BackupScores()
{
	for (int i = 0; i < NUM_TEAMS; i++)
	{
		m_teams[i].m_prevScore = m_teams[i].m_score;
	}
}

void Multiwinia::SetTeamColour( int _teamId, int _colourId )
{
    bool taken = false;
    if(_colourId == -1 )
    {
        m_teams[_teamId].m_colour = GetDefaultTeamColour( _teamId );
    }
    else
    {
        if( m_gameType == GameTypeAssault )
        {
            int numPlayers = g_app->m_gameMenu->GetNumPlayers();
            if( numPlayers != -1 )
            {
                int num = 0;
                for( int i = 0; i < NUM_TEAMS; ++i )
                {
                    if( m_teams[i].m_colourId == _colourId )
                    {
                        num++;
                    }
                }

                if( numPlayers == 3 )
                {
                    if( _colourId == 0 && num > 0 ) taken = true;
                    else if( _colourId == 1 && num == 2 ) taken = true;
                }
                else
                {
                    if( num >= numPlayers / 2 ) taken = true;
                }
            }
            else
            {
                return; // we need to abandon here and wait for GetNumPlayers to return a proper value
            }
        }
        else if( m_coopMode )
        {
            if( !IsValidCoopColour(_colourId) )
            {
                taken = true;
                return;
            }
        }
        else
        {
            taken = IsColourTaken( _colourId );
        }
        if( !taken )
        {
            m_teams[_teamId].m_colour = GetColour( _colourId );
        }
    }
    if( !taken )
    {
        m_teams[_teamId].m_colourId = _colourId;
    }
}

int Multiwinia::GetCPUColour()
{
    bool taken[MULTIWINIA_NUM_TEAMCOLOURS/2];
    memset( taken, false, sizeof(bool) * MULTIWINIA_NUM_TEAMCOLOURS/2 );
    for( int i = 0; i < NUM_TEAMS; ++i  )
    {
        if( g_app->m_multiwinia->m_teams[i].m_colourId != -1 &&
            g_app->m_multiwinia->m_teams[i].m_colourId < MULTIWINIA_NUM_TEAMCOLOURS/2)
        {
            taken[g_app->m_multiwinia->m_teams[i].m_colourId] = true;
        }
    }

    int id = syncrand() % (MULTIWINIA_NUM_TEAMCOLOURS / 2);
    while( taken[id] )
    {
        id = syncrand() % (MULTIWINIA_NUM_TEAMCOLOURS / 2);
    }

    return id;
}

int Multiwinia::GetNextAvailableColour()
{
    for( int i = 0; i < MULTIWINIA_NUM_TEAMCOLOURS; ++i )
    {
        if( !IsColourTaken(i) ) return i;
    }

    return -1;
}

int Multiwinia::IsColourTaken( int _colourId )
{
    int numTeams = 0;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_teams[i].m_teamType == TeamTypeUnused ) continue;
        if( m_teams[i].m_colourId == _colourId ) numTeams++;
    }

    return numTeams;
}

bool Multiwinia::IsValidCoopColour( int _colourId )
{
    int numColoursTaken = 0;
    int numOnThisColour = 0;
    for( int i = 0; i < MULTIWINIA_NUM_TEAMCOLOURS; ++i )
    {
        int taken = IsColourTaken(i);
        if( taken ) numColoursTaken++;
        if( i == _colourId ) numOnThisColour = taken;
    }

    if( numColoursTaken >= 2 && numOnThisColour == 0 ) return false;
    if( numOnThisColour >= 2 ) return false;

    return true;
}

RGBAColour Multiwinia::GetDefaultTeamColour( int _teamId )
{
    return defaultColour[_teamId];
}

RGBAColour Multiwinia::GetColour( int _colourId )
{
    return teamColours[_colourId];
}

RGBAColour Multiwinia::GetGroupColour( int _memberId, int _colourId )
{
    if( _colourId == -1 ) return RGBAColour(0.6f, 0.6f, 0.6f, 1.0f );
    if( _memberId == 0 ) return groupColours1[_colourId];
    else return groupColours2[_colourId];
}

void Multiwinia::SaveIFrame( int _clientId, IFrame *_iframe )
{
	for (int i = 0; i < NUM_TEAMS; i++)
	{
		if (m_teams[i].m_clientId == _clientId)
		{
			AppDebugOut( "Multiwinia: Captured IFrame from client %d\n", _clientId );
			m_teams[i].m_iframe = _iframe;
			break;
		}
	}

	// Calculate the differences between our client's iframe and the other clients
	for( int i = 0; i < NUM_TEAMS; i++ )
	{
		if( m_teams[i].m_clientId == g_app->m_clientToServer->m_clientId )
		{
			if( m_teams[i].m_iframe )
			{
				for( int j = 0; j < NUM_TEAMS; j++ )
				{
					if( i != j && 
						m_teams[j].m_iframe && 
						m_teams[j].m_syncDifferences.Size() == 0 &&
						m_teams[i].m_iframe->HashValue() != m_teams[j].m_iframe->HashValue() )
					{
						m_teams[i].m_iframe->CalculateDiff( 
							*m_teams[j].m_iframe, m_teams[j].m_colour, m_teams[j].m_syncDifferences );

						DumpSyncDiffs( m_teams[j].m_syncDifferences );
					}
				}
			}
		}			
	}	
}


MultiwiniaGameBlueprint::MultiwiniaGameBlueprint( const char *_name )
{
	strcpy( m_name, _name );
    sprintf( m_stringId, "multiwinia_gametype_%s", m_name );
    strlwr( m_stringId );
}

MultiwiniaGameBlueprint::~MultiwiniaGameBlueprint()
{
	m_params.EmptyAndDelete();
}


char *MultiwiniaGameBlueprint::GetName()
{        
    if( g_app->m_langTable->DoesPhraseExist( m_stringId ) )
    {
        return m_stringId;
    }
    else
    {
        return m_name;
    }
}

char *MultiwiniaGameBlueprint::GetDifficulty(int _type)
{
    switch( _type )
    {
        case Multiwinia::GameTypeSkirmish:
        case Multiwinia::GameTypeKingOfTheHill: return "multiwinia_complexity_basic";   

        case Multiwinia::GameTypeCaptureTheStatue:
        case Multiwinia::GameTypeTankBattle:
        case Multiwinia::GameTypeAssault:       return "multiwinia_complexity_intermediate";
            
        case Multiwinia::GameTypeRocketRiot:
        case Multiwinia::GameTypeBlitzkreig:     return "multiwinia_complexity_advanced";
    }
    return "multiwinia_complexity_basic";
}


MultiwiniaGameParameter::MultiwiniaGameParameter( const char *_name )
{
	strcpy( m_name, _name );
    sprintf( m_stringId, "multiwinia_param_%s", m_name );
    strlwr( m_stringId );
}

void MultiwiniaGameParameter::GetNameTranslated(UnicodeString& _dest)
{
    if( g_app->m_langTable->DoesPhraseExist( m_stringId ) )
    {
        _dest = LANGUAGEPHRASE( m_stringId );
    }
    else
    {
        _dest = m_name;
    }
}

char *MultiwiniaGameParameter::GetStringId()
{
    return m_stringId;
}
