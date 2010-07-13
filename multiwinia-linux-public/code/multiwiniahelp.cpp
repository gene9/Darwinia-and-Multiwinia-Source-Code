#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"
#include "lib/math/math_utils.h"
#include "lib/language_table.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/input/file_paths.h"

#include "worldobject/crate.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/multiwiniazone.h"
#include "worldobject/gunturret.h"
#include "worldobject/darwinian.h"
#include "worldobject/armour.h"
#include "worldobject/officer.h"
#include "worldobject/radardish.h"
#include "worldobject/armour.h"

#include "taskmanager_interface.h"
#include "multiwiniahelp.h"
#include "renderer.h"
#include "app.h"
#include "global_world.h"
#include "team.h"
#include "location.h"
#include "main.h"
#include "user_input.h"
#include "gamecursor.h"
#include "multiwinia.h"
#include "camera.h"
#include "entity_grid.h"
#include "markersystem.h"
#include "location_input.h"
#include "level_file.h"
#include "unit.h"
#include "animatedpanel_renderer.h"


std::vector<WorldObjectId> MultiwiniaHelp::s_neighbours;


MultiwiniaHelp::MultiwiniaHelp()
:	m_inputMode(INPUT_MODE_NONE),
    m_groupSelected(0),
    m_promoted(0),
    m_reinforcements(0),
    m_currentHelpTimer(0.0f),
    m_currentHelpHighlight(-1),
    m_currentTutorialHelp(-1),
    m_tutorialTimer(0.0f),
	m_showingScoreHelpTime(-1.0f),
	m_showingScoreCheck(false),
	m_setScoreHelpTime(false),
    m_animatedPanel(NULL),
    m_showSpaceDeselectHelp(0.0f),
    m_tutorialCrateId(-1),
    m_tutorial2RadarHelpDone(false),
    m_tutorial2AICanAct(false),
    m_tutorial2AICanSpawn(false),
    m_tutorial2FormationHelpDone(false),
    m_tutorial2ArmourHelpDone(false),
    m_spaceDeselectShowRMB(false)
{
}


void MultiwiniaHelp::LoadIcons()
{
    strcpy( m_tabs[HelpStartHere].m_iconFilename,           "mwhelp/starthere." TEXTURE_EXTENSION );
	strcpy( m_tabs[HelpKoth].m_iconFilename,                "mwhelp/koth." TEXTURE_EXTENSION );
	strcpy( m_tabs[HelpSpawnPoints].m_iconFilename,         "mwhelp/spawnpoint." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpSpawnPointsConquer].m_iconFilename,  "mwhelp/spawnpoint_assault." TEXTURE_EXTENSION );
	strcpy( m_tabs[HelpCrates].m_iconFilename,              "mwhelp/crates." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpLiftStatue].m_iconFilename,          "mwhelp/liftstatues." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpCaptureStatue].m_iconFilename,       "mwhelp/capturestatues." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpWallBuster].m_iconFilename,          "mwhelp/wallbuster." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpStationAttack].m_iconFilename,       "mwhelp/pulsebomb_attack." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpStationDefend].m_iconFilename,       "mwhelp/pulsebomb_defend." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpRocketRiot].m_iconFilename,          "mwhelp/rocketriot1." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpSolarPanels].m_iconFilename,         "mwhelp/rocketriot2." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpReinforcements].m_iconFilename,      "mwhelp/reinforcements." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpPulseBombAttack].m_iconFilename,     "mwhelp/pulsebomb." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpPulseBombDefend].m_iconFilename,     "mwhelp/pulsebomb." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpBlitzkrieg1].m_iconFilename,         "mwhelp/blitzkrieg1." TEXTURE_EXTENSION );
    strcpy( m_tabs[HelpBlitzkrieg2].m_iconFilename,         "mwhelp/blitzkrieg2." TEXTURE_EXTENSION );
	strcpy( m_tabs[HelpDarwinians].m_iconFilename,          "mwhelp/darwinian." TEXTURE_EXTENSION );
}


void MultiwiniaHelp::Reset()
{
    for( int i = 0; i < NumHelpItems; ++i )
    {
        m_tabs[i].m_accomplished = false;
        m_tabs[i].m_worldPos.Zero();
    }

    m_currentHelpHighlight = -1;
    m_currentHelpTimer = GetHighResTime();

    m_tutorialCrateId = -1;
    m_tutorial2RadarHelpDone = false;
    m_tutorial2FormationHelpDone = false;
    m_tutorial2ArmourHelpDone = false;
    m_tutorial2AICanAct = false;
    m_tutorial2AICanSpawn = false;

    delete m_animatedPanel;
    m_animatedPanel = NULL;
}


void MultiwiniaHelp::AttachTo3d( int helpId )
{
    MultiwiniaHelpItem *item = &m_tabs[helpId];

    switch( helpId )
    {
        case HelpStartHere:
        {
            Building *spawn = FindNearestBuilding( Building::TypeSpawnPoint, g_app->m_globalWorld->m_myTeamId, 0 );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpKoth:
        {
            Building *zone = FindNearestBuilding( Building::TypeMultiwiniaZone );
            if( zone ) item->m_worldPos = zone->m_centrePos;
            item->m_worldSize = 10.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpSpawnPoints:
        {
            Building *spawn = FindNearestBuilding( Building::TypeSpawnPoint, -1, -1 );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpSpawnPointsConquer:
        {            
            Building *spawn = FindNearestBuilding( Building::TypeSpawnPoint, -2, 1 );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 10.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpLiftStatue:
        {
            Building *spawn = FindNearestBuilding( Building::TypeStatue, 255 );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 10.0;
            item->m_worldOffset = 80;
            break;
        }

        case HelpCaptureStatue:
        {
            Building *spawn = FindNearestBuilding( Building::TypeMultiwiniaZone, g_app->m_globalWorld->m_myTeamId );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 10;
            break;
        }

        case HelpRocketRiot:
        {
            Building *spawn = FindNearestBuilding( Building::TypeEscapeRocket, g_app->m_globalWorld->m_myTeamId );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 12.0;
            item->m_worldOffset = 100;
            break;
        }

        case HelpSolarPanels:
        {
            Building *spawn = FindNearestBuilding( Building::TypeSolarPanel );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 9.0;
            item->m_worldOffset = 50;
            break;
        }

        case HelpReinforcements:
        {
            Building *spawn = FindNearestBuilding( Building::TypeTrunkPort, g_app->m_globalWorld->m_myTeamId );
            if( spawn ) item->m_worldPos = spawn->m_centrePos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpWallBuster:
        {
            Building *bomb = FindNearestBuilding( Building::TypeWallBuster );
            if( bomb ) item->m_worldPos = bomb->m_pos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpStationAttack:
        case HelpStationDefend:
        {
            Building *bomb = FindNearestBuilding( Building::TypeControlStation );
            if( bomb ) item->m_worldPos = bomb->m_pos;
            item->m_worldSize = 6.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpPulseBombAttack:
        case HelpPulseBombDefend:
        {
            Building *bomb = FindNearestBuilding( Building::TypePulseBomb );
            if( bomb ) item->m_worldPos = bomb->m_pos;
            item->m_worldSize = 12.0;
            item->m_worldOffset = 100;
            break;
        }

        case HelpBlitzkrieg1:
        {
            Building *zone = FindNearestBuilding( Building::TypeMultiwiniaZone );
            if( zone ) item->m_worldPos = zone->m_centrePos;
            item->m_worldSize = 10.0;
            item->m_worldOffset = 0;
            break;
        }

        case HelpBlitzkrieg2:
        {
            int randomTeam = rand() % g_app->m_location->m_levelFile->m_numPlayers;
            while( randomTeam == g_app->m_globalWorld->m_myTeamId )
            {
                randomTeam = rand() % g_app->m_location->m_levelFile->m_numPlayers;
            }
            Building *zone = g_app->m_location->GetBuilding( MultiwiniaZone::s_blitzkriegBaseZone[randomTeam] );
            if( zone ) item->m_worldPos = zone->m_centrePos;
            item->m_worldSize = 10.0;
            item->m_worldOffset = 0;
            break;
        }        
    }
}


void MultiwiniaHelp::RunTutorialHelp( int helpId, Vector3 const &pos, float offset )
{
    if( helpId != m_currentTutorialHelp )
    {
        if( GetHighResTime() < m_tutorialTimer + 0.5 )
        {
            // Too little time has passed since the previous help
            return;
        }

        if( m_currentTutorialHelp == HelpTutorialMoveToSpawnPoint ||
            m_currentTutorialHelp == HelpTutorialMoveToMainZone ||
            m_currentTutorialHelp == HelpTutorialSendToZone ||
            m_currentTutorialHelp == HelpTutorialCaptureZone )
        {
            if( GetHighResTime() < m_tutorialTimer + 10.0 )
            {
                // Give the player a break if he's just completed something
                return;
            }
        }

        if( m_currentTutorialHelp == HelpTutorialUseAirstrike )
        {
            if( GetHighResTime() < m_tutorialTimer + 20.0 )
            {
                // Give the player a break if he's just completed something
                return;
            }
        }
    }


    m_tabs[helpId].m_worldPos = pos;
    m_tabs[helpId].m_worldOffset = offset;
    m_tabs[helpId].m_appropriate = true;
    m_currentTutorialHelp = helpId;
    m_tutorialTimer = GetHighResTime();
}


void MultiwiniaHelp::AttachTutorialTo3d()
{
    if( !g_app->m_location->GetMyTeam() ) return;

    Team *team = g_app->m_location->GetMyTeam();

    //if( team->m_taskManager->m_mostRecentTaskId != -1 )
    //{
    //    RunTutorialHelp( HelpTutorialDismissInfoBox, Vector3(500, 500, 500), 0.0f );
    //    return;
    //}

    //
    // Lookup our important buildings

    Building *aiTargets[] = { g_app->m_location->GetBuilding( 30 ),
                              g_app->m_location->GetBuilding( 25 ),
                              g_app->m_location->GetBuilding( 40 ) };

    Building *zones[] = { g_app->m_location->GetBuilding( 9 ),
                          g_app->m_location->GetBuilding( 23 ) };

    Building *spawns[] = { g_app->m_location->GetBuilding( 3 ),
                           g_app->m_location->GetBuilding( 4 ),
                           g_app->m_location->GetBuilding( 8 ) };

	if( m_showingScoreHelpTime > 0.0f && m_showingScoreHelpTime + 10.0f >= GetHighResTime() )
	{
		m_showingScoreCheck = true;
	}

	if( m_showingScoreCheck )
	{
		if( m_showingScoreHelpTime + 40.0f > GetHighResTime() )
		{
			RunTutorialHelp( HelpTutorialScores, zones[0]->m_pos + g_upVector * 50, 20.0f );
		}
		else 
		{
			m_tabs[HelpTutorialScores].m_appropriate = false;
			m_showingScoreCheck = false;
			m_showingScoreHelpTime = -1.0f;
		}
		return;
	}


    //
    // Figure out how well our player is doing

    int latestSpawn = -1;
    int previousSpawn = -1;
    
    for( int i = 0; i < 3; ++i )
    {
		if( !spawns[i] ) return;
        if( spawns[i]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
        {
            latestSpawn = i;            
            if( i > 0 ) previousSpawn = i - 1;
        }
    }

    Entity *selectedEntity          = g_app->m_location->GetMyTeam()->GetMyEntity();
    int numSelectedDarwinians       = g_app->m_location->GetMyTeam()->m_numDarwiniansSelected;


    //
    // Set the help appropriately

    bool helpSet = false;


    // 
    // First look for darwinians near the first spawn point to capture the main zone
    // This will occur once the 1st and 2nd spawn points are ours

    if( !helpSet )
    {
        bool firstSpawnTaken = spawns[1]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId ||
                               CountDarwiniansGoingHere( spawns[1]->m_pos, g_app->m_globalWorld->m_myTeamId ) >= 10;

        if( firstSpawnTaken &&
            CountDarwiniansGoingHere( zones[0]->m_pos, g_app->m_globalWorld->m_myTeamId ) < 10 )
        {
            int numIdleDarwinians = CountIdleDarwinians( aiTargets[0]->m_pos, 100, g_app->m_globalWorld->m_myTeamId );

            if( numSelectedDarwinians == 0 && numIdleDarwinians > 15 )
            {
                // Group select some darwinians
                RunTutorialHelp( HelpTutorialGroupSelect, aiTargets[0]->m_pos, 40.0f );
                helpSet = true;
            }
            else if( numSelectedDarwinians > 0 )
            {
                // Move your darwinians to the zone
                RunTutorialHelp( HelpTutorialMoveToMainZone, zones[0]->m_pos, 40.0f );
                helpSet = true;
            }
        }
    }


    //
    // Next look for an occupied spawn point that can be used to capture another

	if( !helpSet && !selectedEntity )
    {
        for( int i = 0; i < 2; ++i )
        {
            if( spawns[i]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId &&
                spawns[i+1]->m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId )
            {
                int numIdleDarwinians           = CountIdleDarwinians( aiTargets[i]->m_pos, 100, g_app->m_globalWorld->m_myTeamId );
                int numDarwiniansAlreadyMoving  = CountDarwiniansGoingHere( spawns[i+1]->m_pos, g_app->m_globalWorld->m_myTeamId );

                if( numSelectedDarwinians == 0 && 
                    numIdleDarwinians >= 20 &&
                    numDarwiniansAlreadyMoving < 10 )
                {
                    // Group select some darwinians
                    RunTutorialHelp( HelpTutorialGroupSelect, aiTargets[i]->m_pos, 40.0f );
                    helpSet = true;
                }
                else if( numSelectedDarwinians > 0 )
                {
                    // Move your darwinians to the next spawn point
                    RunTutorialHelp( HelpTutorialMoveToSpawnPoint, spawns[i+1]->m_pos, 40.0f );
                    helpSet = true;
                }
            }
        }        
    }


  
    //
    // Next look for an occupied spawn point that needs an officer
    // Only do this once we have the central zone

    if( !helpSet )
    {
        if( zones[0]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId ||
            ((MultiwiniaZone *)zones[0])->m_teamCount[g_app->m_globalWorld->m_myTeamId] > 5 )
        {
            for( int i = 0; i < 2; ++i )
            {
                if( spawns[i]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
                {
                    bool nextZoneDealthWith = (spawns[i+1]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId || 
                                               CountDarwiniansGoingHere( spawns[i+1]->m_pos, g_app->m_globalWorld->m_myTeamId ) > 10 );
                    if( nextZoneDealthWith )
                    {
                        int numOfficersWithGoto = CountOfficersWithGotoHere( aiTargets[i]->m_pos, zones[0]->m_pos );

                        if( numOfficersWithGoto == 0 )
                        {
                            int numIdleDarwinians = CountIdleDarwinians( aiTargets[i]->m_pos, 100, g_app->m_globalWorld->m_myTeamId );

                            if( numSelectedDarwinians == 0 && 
                                numIdleDarwinians >= 10 &&
                                !selectedEntity )
                            {
                                // Promote an officer
                                if( i == 0 ) RunTutorialHelp( HelpTutorialPromote, aiTargets[i]->m_pos, 40.0f );
                                else         RunTutorialHelp( HelpTutorialPromoteAdvanced, aiTargets[i]->m_pos, 40.0f );
                                helpSet = true;
                            }         
                            else if( selectedEntity &&
                                    selectedEntity->m_type == Entity::TypeOfficer )
                            {
                                // Point officer to score zone
                                RunTutorialHelp( HelpTutorialSendToZone, zones[0]->m_pos, 20.0f );
                                helpSet = true;
                            }
                        }
                    }
                }
            }
        }
    }


    //
    // If all 3 spawn points are occupied
    // And both officers are set up
    // Get them to take the last koth zone

    if( !helpSet )
    {
        int numSpawnsCaptured = 0;
        int numOfficersWorking = 0;

        for( int i = 0; i < 3; ++i )
        {
            if( spawns[i]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                ++numSpawnsCaptured;
            }

            int numOfficersWithGoto = CountOfficersWithGotoHere( aiTargets[i]->m_pos, zones[0]->m_pos );
            if( numOfficersWithGoto > 0 )
            {
                ++numOfficersWorking;
            }
        }

                                       
        if( numSpawnsCaptured >= 3 && numOfficersWorking >= 2 )
        {
            bool finalZoneAlreadyTaken = ( zones[1]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId ||
                                           CountDarwiniansGoingHere( zones[1]->m_pos, g_app->m_globalWorld->m_myTeamId ) >= 10 );
            bool doneCrateHelp = (m_tutorialCrateId != -1 && 
                                  !g_app->m_location->GetBuilding( m_tutorialCrateId ) && 
                                  g_app->m_location->GetMyTaskManager()->CountNumTasks(GlobalResearch::TypeAirStrike) == 0 );

            int numIdleDarwinians = CountIdleDarwinians( aiTargets[2]->m_pos, 100, g_app->m_globalWorld->m_myTeamId );

            if( !finalZoneAlreadyTaken &&
                numIdleDarwinians > 10 )
            {
                // Send darwinians to final zone
                RunTutorialHelp( HelpTutorialCaptureZone, zones[1]->m_pos, 20.0f );
                helpSet = true;
            }
            else if( !doneCrateHelp )
            {
                if( m_tutorialCrateId == -1 && GetHighResTime() > m_tutorialTimer + 10.0f )
                {
                    Building *building = g_app->m_location->GetBuilding(43);
                    if( building )
                    {
                        Crate crateTemplate;
                        crateTemplate.m_pos = building->m_pos;
                        crateTemplate.m_pos.x -= 100.0f;
                        crateTemplate.m_pos.z -= 200.0f;
                        crateTemplate.m_pos.y += 800.0f;
                        crateTemplate.m_id.Set( 255, UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

                        Building *newBuilding = Building::CreateBuilding( Building::TypeCrate );
                        int id = g_app->m_location->m_buildings.PutData(newBuilding); 
                        newBuilding->Initialise( (Building*)&crateTemplate);
                        newBuilding->SetDetail( 1 );
                        ((Crate *)newBuilding)->m_rewardId = Crate::CrateRewardAirstrike;
                        m_tutorialCrateId = newBuilding->m_id.GetUniqueId();

                        g_app->m_markerSystem->RegisterMarker_Crate( newBuilding->m_id );

                        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_drop") );
                    }
                }
                

                Crate *crate = (Crate *)g_app->m_location->GetBuilding( m_tutorialCrateId );
                if( crate )
                {
                    if( crate->m_onGround )
                    {
                        RunTutorialHelp( HelpTutorialCrateCollect, crate->m_pos + crate->m_vel * SERVER_ADVANCE_PERIOD, 80.0f );
                        helpSet = true;
                    }
                    else
                    {
                        RunTutorialHelp( HelpTutorialCrateFalling, crate->m_pos, 80.0f );
                        helpSet = true;
                    }
                }
                else
                {
                    TaskManager *tm = g_app->m_location->GetMyTaskManager();
                    if( tm->m_mostRecentTaskId == -1 )
                    {
                        if( !tm->GetCurrentTask() )
                        {
                            RunTutorialHelp( HelpTutorialSelectTask, Vector3(500, 500, 500 ), 0.0f );
                            helpSet = true;
                        }
                        else
                        {
                            Building *spawn = g_app->m_location->GetBuilding(0);
                            if( spawn )
                            {
                                RunTutorialHelp( HelpTutorialUseAirstrike, spawn->m_pos, 20.0f );
                                helpSet = true;
                            }
                        }
                    }
                }
            }
			else if( finalZoneAlreadyTaken && !m_setScoreHelpTime )
			{
				m_showingScoreHelpTime = GetHighResTime();
				m_setScoreHelpTime = true;
			}
        }
    }

    //
    // If the player has done absolutely everything,
    // draw his attention to the scores
    // (Broken at different resolutions)

    /*if( !helpSet )
    {
        int numSpawnsCaptured = 0;
        int numZonesCaptured = 0;

        for( int i = 0; i < 3; ++i )
        {
            if( spawns[i]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
            {
                ++numSpawnsCaptured;
            }
        }

        if( zones[1]->m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
        {
            ++numZonesCaptured;
        }

        if( numSpawnsCaptured >= 3 && numZonesCaptured >= 1 )
        {
            RunTutorialHelp( HelpTutorialScores, g_zeroVector, 0.0f );
            helpSet = true;
        }
    }*/
}

void MultiwiniaHelp::AttachTutorial2To3d()
{
    if( m_tutorial2AICanSpawn && m_currentTutorialHelp == -1 ) return;
    bool helpSet = false;

    Team *team = g_app->m_location->GetMyTeam();
    int numDarwiniansSelected = team->m_numDarwiniansSelected;
    Entity *selectedEntity = team->GetMyEntity();

    Building *b = g_app->m_location->GetBuilding( 4 );
    if( !b ) return;

    Building *targetDish = g_app->m_location->GetBuilding( 9 );
    if( !targetDish ) return;

    if( !m_tutorial2AICanSpawn )
    {
        g_app->m_multiwinia->m_timeRemaining = 300;
    }

    //if( team->m_taskManager->m_mostRecentTaskId != -1 )
    //{
    //    RunTutorialHelp( HelpTutorialDismissInfoBox, Vector3(500, 500, 500), 0.0f );
    //    return;
    //}

    // Radar Dish Help
    if( b->m_type == Building::TypeRadarDish && !m_tutorial2RadarHelpDone )
    {
        RadarDish *radar = (RadarDish *)b;

        if( radar->CountValidLinks() > 1 )
        {
            radar->ClearLinks();
            radar->AddValidLink( 9 );
        }
        if( radar->m_id.GetTeamId() == 0 )
        {
            if( radar->GetConnectedDishId() != 9 )
            {
                if( g_app->m_location->GetMyTeam()->m_currentBuildingId != radar->m_id.GetUniqueId() )
                {
                    RunTutorialHelp( HelpTutorial2SelectRadar, radar->m_pos, 40.0f );
                    helpSet = true;
                }
                else
                {                
                    RunTutorialHelp( HelpTutorial2AimRadar, targetDish->m_pos, 60.0f );
                    helpSet = true;
                }
            }
            else
            {
                if( CountIdleDarwinians( targetDish->m_pos, 50.0f, 0 ) > 10 )
                {
                    m_tutorial2RadarHelpDone = true;
                }
                else if( radar->m_inTransit.Size() > 0 )
                {
                    RunTutorialHelp( HelpTutorial2RadarTransit, targetDish->m_pos, 60.0f );
                    helpSet = true;
                }
                else if( CountDarwiniansGoingHere( radar->m_pos, 0 ) > 0 )
                {
                    helpSet = true;
                }
                else if( g_app->m_location->GetMyTeam()->m_currentBuildingId != -1 )
                {
                    RunTutorialHelp( HelpTutorial2Deselect, radar->m_pos, 40.0f );
                    helpSet = true;
                }
                else if( numDarwiniansSelected == 0 )
                {
                    Building *spawn = g_app->m_location->GetBuilding( 3 );
                    RunTutorialHelp( HelpTutorialGroupSelect, spawn->m_pos + spawn->m_front * 100.0f, 20.0f );
                    helpSet = true;
                }
                else
                {
                    RunTutorialHelp( HelpTutorial2UseRadar, radar->m_pos, 40.0f );
                    helpSet = true;
                }
            }
        }
    }

    if( !m_tutorial2RadarHelpDone ) helpSet = true;

    // Officer Help
    if( !helpSet && !m_tutorial2FormationHelpDone )
    {
        Building *spawn = g_app->m_location->GetBuilding( 3 );
        int numOfficers = CountOfficersWithGotoHere( spawn->m_pos, b->m_pos );
        if( !selectedEntity && numOfficers == 0 && CountIdleDarwinians( spawn->m_pos, 100.0f, 0 ) > 10 )
        {
            RunTutorialHelp( HelpTutorialPromote, spawn->m_pos + spawn->m_front * 100.0f, 20.0f );
            helpSet = true;
        }
        else if( selectedEntity && numOfficers == 0 )
        {
            RunTutorialHelp( HelpTutorial2OrderInRadar, b->m_pos, 40.0f );
            helpSet = true;
        }
        else if( numOfficers > 0 )
        {
            Building *scoreZone = g_app->m_location->GetBuilding( 0 );
            Vector3 formationPos = GetFormationOfficerPosition();
            if( scoreZone->m_id.GetTeamId() ==  0 && formationPos != g_zeroVector )
            {
                m_tutorial2AICanAct = true; // failsafe check 
                m_tutorial2FormationHelpDone = true;
            }

            if( CountFormationOfficersGoingHere( scoreZone->m_pos ) > 0 )
            {
                RunTutorialHelp( HelpTutorial2FormationInfo, formationPos, 30.0f );
                helpSet = true;
                m_tutorial2AICanAct = true;
            }
            else if( selectedEntity && selectedEntity->m_type == Entity::TypeOfficer && !((Officer *)selectedEntity)->m_formation )
            {
                RunTutorialHelp( HelpTutorial2FormationSetup, selectedEntity->m_pos, 40.0f );
                helpSet = true;
            }
            else if( formationPos == g_zeroVector )
            {
                RunTutorialHelp( HelpTutorialPromote, targetDish->m_pos + targetDish->m_front * 30.0f, 20.0f );
                helpSet = true;
            }
            else if( selectedEntity && selectedEntity->m_type == Entity::TypeOfficer )
            {   
                Vector3 pos = scoreZone->m_pos;
                pos.z += 100.0f;
                RunTutorialHelp( HelpTutorial2FormationMove, pos, 20.0f );
                helpSet = true;
            }
        }
    }

    if( !m_tutorial2AICanAct ) helpSet = true;

    if( !helpSet && !m_tutorial2ArmourHelpDone )
    {
        Building *enemySpawn = g_app->m_location->GetBuilding( 83 );
        if( !enemySpawn ) return;
        Vector3 formationPos = GetFormationOfficerPosition();

        int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( enemySpawn->m_pos.x, enemySpawn->m_pos.z, 100.0f, 0 );
        if( (formationPos - enemySpawn->m_pos).Mag() < 200.0f && numEnemies < 10 && formationPos != g_zeroVector && enemySpawn->m_id.GetTeamId() != 0 ) 
        {
            RunTutorialHelp( HelpTutorial2BreakFormation, formationPos, 40.0f );
            helpSet = true;
        }
        else if( enemySpawn->m_id.GetTeamId() != 255 && enemySpawn->m_id.GetTeamId() != 0 && formationPos != g_zeroVector )
        {
            RunTutorialHelp( HelpTutorial2FormationAttack, enemySpawn->m_pos, 40.0f );
            helpSet = true;
        }
        /*else if( formationPos == g_zeroVector )
        {
            m_tutorial2FormationHelpDone = false;
            helpSet = true;
            // they've terminated their formation or something, so back we go
        }*/
        else if( enemySpawn->m_id.GetTeamId() == 0 )
        {
            if( m_tutorialCrateId == -1 && GetHighResTime() > m_tutorialTimer + 10.0f )
            {
                Building *building = g_app->m_location->GetBuilding(66);
                if( building )
                {
                    Crate crateTemplate;
                    crateTemplate.m_pos = building->m_pos;
                    crateTemplate.m_pos.x += 100.0f;
                    crateTemplate.m_pos.y += 300.0f;
                    crateTemplate.m_id.Set( 255, UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

                    Building *newBuilding = Building::CreateBuilding( Building::TypeCrate );
                    int id = g_app->m_location->m_buildings.PutData(newBuilding); 
                    newBuilding->Initialise( (Building*)&crateTemplate);
                    newBuilding->SetDetail( 1 );
                    ((Crate *)newBuilding)->m_rewardId = Crate::CrateRewardArmour;
                    m_tutorialCrateId = newBuilding->m_id.GetUniqueId();

                    g_app->m_markerSystem->RegisterMarker_Crate( newBuilding->m_id );

                    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("crate_drop") );
                }
            }

            Building *crate = g_app->m_location->GetBuilding( m_tutorialCrateId );
            if( crate )
            {
                if( crate->m_onGround )
                {
                    RunTutorialHelp( HelpTutorialCrateCollect, crate->m_pos, 80.0f );
                    helpSet = true;
                }
            }
            else
            {
                Building *freeSpawn = g_app->m_location->GetBuilding( 20 );
                TaskManager *tm = g_app->m_location->GetMyTaskManager();
                if( freeSpawn->m_id.GetTeamId() == 0 )
                {
                    m_tutorial2ArmourHelpDone = true;
                    helpSet = true;
                }
                else if( selectedEntity && selectedEntity->m_type == Entity::TypeArmour )
                {
                    Armour *a = (Armour *)selectedEntity;
                    if( a->GetNumPassengers() >= 20 &&
                        (a->GetSpeed() == 0.0 && ((a->m_pos - freeSpawn->m_pos).Mag() > 150.0f || (a->m_wayPoint - freeSpawn->m_pos).Mag() > 150.0 ) ))
                    {
                        RunTutorialHelp( HelpTutorial2MoveArmour, freeSpawn->m_pos, 40.0f );
                        helpSet = true;

                    } 
                    else if( (a->m_pos - freeSpawn->m_pos).Mag() < 200.0f )
                    {
                        RunTutorialHelp( HelpTutorial2UnloadArmour, a->m_wayPoint, 80.0f );
                        helpSet = true;
                    }
                    else if( a->m_state == Armour::StateIdle &&
                             a->GetSpeed() == 0 &&
                             a->GetNumPassengers() == 0 )
                    {
                        RunTutorialHelp( HelpTutorial2LoadArmour, a->m_pos, 80.0f);
                        helpSet = true;
                    }                       
                }
                
                if( !helpSet && 
                    tm->CountNumTasks( GlobalResearch::TypeArmour ) > 0 &&
                    tm->m_mostRecentTaskId == -1 )
                {
                    if( !tm->GetCurrentTask() )
                    {
                        RunTutorialHelp( HelpTutorial2SelectArmour, Vector3(500, 500, 500 ), 0.0f );
                        helpSet = true;
                    }
                    else if( CountActiveArmour() == 0 )
                    {
                        Building *spawn = g_app->m_location->GetBuilding(84);
                        if( spawn )
                        {
                            RunTutorialHelp( HelpTutorial2PlaceArmour, spawn->m_pos, 20.0f );
                            helpSet = true;
                        }
                    }
                }
            }
        }
    }

    if( !m_tutorial2ArmourHelpDone ) helpSet = true;

    if( !helpSet )
    {
        Building *controlStation = g_app->m_location->GetBuilding( 52 );
        RadarDish *radar = (RadarDish *)g_app->m_location->GetBuilding( 11 );
        if( !controlStation ) return;
        if( !radar || radar->m_type != Building::TypeRadarDish ) return;

        if( controlStation->m_id.GetTeamId() != 0 )
        {
            RunTutorialHelp( HelpTutorial2CaptureStation, controlStation->m_pos, 40.0f );
            helpSet = true;
        }
        else if( team->m_currentBuildingId != 11 && !radar->NotAligned() && radar->GetConnectedDishId() == -1 )
        {
            RunTutorialHelp( HelpTutorial2SelectRadar, radar->m_pos, 40.0f );
            helpSet = true;
        }
        else if( radar->GetConnectedDishId() == -1 )
        {
            Building *dish = g_app->m_location->GetBuilding(12);
            RunTutorialHelp( HelpTutorial2AimRadar, dish->m_pos, 40.0f );
            helpSet = true;
        }
        else
        {
            if( m_currentTutorialHelp != HelpTutorial2End )
            {
                Building *zone = g_app->m_location->GetBuilding(0);
                RunTutorialHelp( HelpTutorial2End, zone->m_pos, 60.0f );
                helpSet = true;
                g_app->m_multiwinia->m_basicCratesOnly = true;
                g_app->m_multiwinia->SetGameOption( 7, 0 );
                g_app->m_multiwinia->m_aiType = Multiwinia::AITypeStandard;
                m_tutorial2AICanSpawn = true;
            }

            if( GetHighResTime() > m_tutorialTimer + 20.0f )
            {
                m_currentTutorialHelp = -1;
            }
        }
    }
}

Vector3 MultiwiniaHelp::GetFormationOfficerPosition()
{
    Team *team = g_app->m_location->GetMyTeam();
    if( !team ) return Vector3();

    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Officer *officer = (Officer *)g_app->m_location->GetEntitySafe( id, Entity::TypeOfficer );

        if( officer && 
            officer->m_formation )
        {
            return officer->m_pos + officer->m_front * 30.0f;
        }
    }

    return Vector3();
}


int MultiwiniaHelp::CountOfficersWithGotoHere( Vector3 const &pos, Vector3 const &target )
{
    Team *team = g_app->m_location->GetMyTeam();
    if( !team ) return 0;

    float posRange = 100.0f;
    float targetRange = 100.0f;

    int result = 0;

    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Officer *officer = (Officer *)g_app->m_location->GetEntitySafe( id, Entity::TypeOfficer );

        if( officer && 
            officer->m_orders == Officer::OrderGoto )
        {
            float distToPos = ( officer->m_pos - pos ).Mag();
            float distToTarget = ( officer->m_orderPosition - target ).Mag();

            if( distToPos < posRange &&
                distToTarget < targetRange )
            {
                ++result;
            }
        }
    }

    return result;
}

int MultiwiniaHelp::CountActiveArmour()
{
    Team *team = g_app->m_location->GetMyTeam();
    if( !team ) return 0;

    int result = 0;

    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Entity *a = g_app->m_location->GetEntitySafe(id, Entity::TypeArmour );
        if( a )
        {
            result ++;
        }
    }

    return result;
}

int MultiwiniaHelp::CountFormationOfficersGoingHere( Vector3 const &pos )
{
    Team *team = g_app->m_location->GetMyTeam();
    if( !team ) return 0;

    float posRange = 150.0f;

    int result = 0;

    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Officer *officer = (Officer *)g_app->m_location->GetEntitySafe( id, Entity::TypeOfficer );

        if( officer && 
            officer->m_formation )
        {
            float distToPos = ( officer->m_wayPoint - pos ).Mag();

            if( distToPos < posRange )
            {
                ++result;
            }
        }
    }

    return result;
}


Building *MultiwiniaHelp::FindNearestBuilding( int type, int teamId, int occupied )
{
    Vector3 camPos = g_app->m_camera->GetPos();
    Building *result = NULL;
    float nearest = 99999.9f;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == type )
            {
                bool teamMatch = ( teamId == -1 );
                if( building->m_id.GetTeamId() == teamId ) teamMatch = true;
                if( teamId == -2 && building->m_id.GetTeamId() != g_app->m_globalWorld->m_myTeamId ) teamMatch = true;

                if( teamMatch )
                {
                    float distance = ( building->m_centrePos - camPos ).Mag();
                    if( distance < nearest )
                    {
                        bool occupiedPass = false;
                        if( occupied == 0 )
                        {
                            occupiedPass = true;
                        }
                        else
                        {
                            bool peoplePresent = g_app->m_location->m_entityGrid->AreNeighboursPresent( building->m_pos.x, building->m_pos.z, 100 );
                            if( occupied == 1 && peoplePresent ) occupiedPass = true;
                            if( occupied == -1 && !peoplePresent ) occupiedPass = true;
                        }
                        
                        if( occupiedPass )
                        {
                            result = building;
                            nearest = distance;
                        }
                    }
                }
            }
        }
    }

    return result;
}


void MultiwiniaHelp::Advance()
{
    if( !g_app->Multiplayer() ) return;

    //
	// Did we change the controller input?

	int newInputMode = g_inputManager->getInputMode();
	if( newInputMode != m_inputMode )
	{
		m_inputMode = newInputMode;
		LoadIcons();
	}
		

    //
    // Load our animated panel if required

    if( !m_animatedPanel &&
        g_app->m_multiwinia->GameInGracePeriod() )
    {
        InitialiseAnimatedPanel();
    }


    //
    // Figure out which tabs are visible and which arent

    Team *myTeam = g_app->m_location->GetMyTeam();
	if(!myTeam) return;

    Entity *entity = myTeam->GetMyEntity();

    WorldObjectId highlightedId;
    Vector3 pos;
    float radius;
    bool highlighted = false;
    
    if( !entity && myTeam->m_numDarwiniansSelected == 0 )
    {
        highlighted = g_app->m_gameCursor->GetHighlightedObject( highlightedId, pos, radius );
    }

    for( int i = 0; i < NumHelpItems; ++i )
    {        
        switch( i )
        {
            case HelpStartHere:             m_tabs[i].m_appropriate = (g_app->m_multiwinia->GameInGracePeriod() &&
                                                                       g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeRocketRiot &&
                                                                       g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault &&
                                                                       g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeBlitzkreig );      break;

			case HelpKoth:					m_tabs[i].m_appropriate = (g_app->m_multiwinia->GameInGracePeriod() &&
                                                                        g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeKingOfTheHill &&
                                                                       !g_app->m_multiwiniaTutorial );	break;

            case HelpSpawnPoints:
                {
                    int totalSpawnPoints = 0;
                    for( int i = 0; i < NUM_TEAMS + 1; ++i )
                    {
                        totalSpawnPoints += SpawnPoint::s_numSpawnPoints[i];                        
                    }
                    int availableForCapture = SpawnPointsAvailableForCapture();
                    
                    m_tabs[i].m_appropriate = ( totalSpawnPoints > 0 && 
                                                availableForCapture > 0 && 
                                                g_app->m_multiwinia->GameInGracePeriod() &&
                                                !g_app->m_multiwiniaTutorial );
                    m_tabs[i].m_accomplished = std::max( m_tabs[i].m_accomplished,
                                                    SpawnPointsOwned() - 1 );       
                }
                break;
            
            case HelpSpawnPointsConquer:    m_tabs[i].m_appropriate = (g_app->m_multiwinia->GameInGracePeriod() &&
                                                                       g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeSkirmish);	        break;
                
            case HelpCrates:                m_tabs[i].m_appropriate = ( g_app->m_multiwinia->GameInGracePeriod() &&
                                                                        CratesOnLevel() > 0 );              break;
            
            case HelpDarwinians:            m_tabs[i].m_appropriate = ( false );                            break;

            case HelpLiftStatue:            
            case HelpCaptureStatue:
                                            m_tabs[i].m_appropriate = (g_app->m_multiwinia->GameInGracePeriod() &&
                                                                        g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeCaptureTheStatue);	break;

            case HelpWallBuster:            m_tabs[i].m_appropriate = WallBusterOwned();                    break;

            case HelpStationDefend:
            case HelpPulseBombDefend:       m_tabs[i].m_appropriate = PulseBombToDefend();                  break;

            case HelpStationAttack:
            case HelpPulseBombAttack:       m_tabs[i].m_appropriate = PulseBombToAttack();                  break;
            
            case HelpRocketRiot:            m_tabs[i].m_appropriate = (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot);	break;
            case HelpSolarPanels:           m_tabs[i].m_appropriate = (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot);	break;
            case HelpReinforcements:        m_tabs[i].m_appropriate = (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot ||
                                                                       g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault ||
                                                                       g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig ); break;

            case HelpBlitzkrieg1:   
            case HelpBlitzkrieg2:           m_tabs[i].m_appropriate = g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig;   break;



                // ===============================================
                // Multiwinia Tutorial help tabs
                // ===============================================

            case HelpTutorialGroupSelect:
            case HelpTutorialMoveToSpawnPoint:
            case HelpTutorialPromote:
            case HelpTutorialSendToZone:
			case HelpTutorialCaptureZone:
            case HelpTutorialCrateFalling:
            case HelpTutorialCrateCollect:
            case HelpTutorialSelectTask:
            case HelpTutorialUseAirstrike:
            case HelpTutorial2SelectRadar:
            case HelpTutorial2AimRadar:
				m_tabs[i].m_appropriate = false;            break;

            case HelpTutorialScores:
				if( m_currentTutorialHelp != HelpTutorialScores )
				{
					m_tabs[i].m_appropriate = false;
				}
				break;

            case HelpTutorial2End:
                if( m_currentTutorialHelp != HelpTutorial2End )
				{
					m_tabs[i].m_appropriate = false;
				}
                break;

            default:
                m_tabs[i].m_appropriate = false;            break;
        }


        if( g_app->m_multiwinia->GameInGracePeriod() )
        {
            if( m_tabs[i].m_appropriate &&
                m_tabs[i].m_worldPos == g_zeroVector )
            {
                AttachTo3d( i );
            }
        }
    }

#ifdef INCLUDE_TUTORIAL
    if( g_app->m_multiwiniaTutorial )
    {
        if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 )
        {
            AttachTutorialTo3d();
        }
        else if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 )
        {
            AttachTutorial2To3d();
        }
    }
#endif


    //
    // Calculate target spot for each icon

    float size = g_app->m_renderer->ScreenH() * 0.2;
    if( size > 200 ) size = 200;
    float accomplishedSize = size * 0.66;

    float gap = 10;
    float x = g_app->m_renderer->ScreenW() - size - gap;
    float y = gap;
    float accomplishedX = g_app->m_renderer->ScreenW() - accomplishedSize - gap;

    for( int i = 0; i < NumHelpItems; ++i )
    {
        m_tabs[i].m_renderMe = ( m_tabs[i].m_appropriate );
        
        if( m_tabs[i].m_screenX == 0 &&
            m_tabs[i].m_screenY == 0 )
        {
            m_tabs[i].m_screenX = g_app->m_renderer->ScreenW();
            m_tabs[i].m_screenY = g_app->m_renderer->ScreenH();
        }

        if( m_tabs[i].m_renderMe )
        {
            if( g_app->m_multiwinia->GameInGracePeriod() )
            {
                m_tabs[i].m_screenX = x;
                m_tabs[i].m_screenY = y;
                m_tabs[i].m_size = size;
                m_tabs[i].m_alpha = 1.0;

                float camDist = ( g_app->m_camera->GetPos() - m_tabs[i].m_worldPos ).Mag();
                m_tabs[i].m_screenLinkSize = 10000 * m_tabs[i].m_worldSize * 1.0/camDist;

                float screenX, screenY;                
                Vector3 targetWorldPos = m_tabs[i].m_worldPos;
                targetWorldPos += g_upVector * m_tabs[i].m_worldOffset;
                g_app->m_camera->Get2DScreenPos( targetWorldPos, &screenX, &screenY );
                screenY = g_app->m_renderer->ScreenH() - screenY;                               
                //screenY -= m_tabs[i].m_size * 1.2;                 
                //screenX -= m_tabs[i].m_size * 0.5;                
                m_tabs[i].m_screenLinkX = screenX;
                m_tabs[i].m_screenLinkY = screenY;
                //m_tabs[i].m_alpha = 1.0;
                //m_tabs[i].m_targetSize = size;
            }
            else
            {
                m_tabs[i].m_targetX = accomplishedX;
                m_tabs[i].m_targetY = y;
                m_tabs[i].m_targetSize = accomplishedSize;
                m_tabs[i].m_alpha = 1.0;
                if( m_tabs[i].m_accomplished ) m_tabs[i].m_alpha *= 0.5;
            }
		    
            y += m_tabs[i].m_size;
		    y += gap;
        }
        else
        {
            m_tabs[i].m_alpha = 0.0;
        }
    }


    //
    // Move tabs towards their target spot

    if( !g_app->m_multiwinia->GameInGracePeriod() )
    {
        float advanceTime = g_advanceTime;
        if( advanceTime > 0.1 ) advanceTime = 0.1;
        float moveRate = advanceTime * 2.0;
        float sizeRate = advanceTime * 2.0;
        float alphaRate = advanceTime;
        float maxMove = g_app->m_renderer->ScreenH() / 5.0;

        float threshold = 0.1;
        float alphaThreshold = 0.001;

        for( int i = 0; i < NumHelpItems; ++i )
        {
            if( m_tabs[i].m_renderMe )
            {
                float xDiff = m_tabs[i].m_targetX - m_tabs[i].m_screenX;   
                if( xDiff > maxMove ) xDiff = maxMove;
                if( xDiff < -maxMove ) xDiff = -maxMove;

                if( iv_abs(xDiff) < threshold )   m_tabs[i].m_screenX = m_tabs[i].m_targetX;
                else                            m_tabs[i].m_screenX += xDiff * moveRate * 3.0;

                float yDiff = m_tabs[i].m_targetY - m_tabs[i].m_screenY;                
                if( yDiff > maxMove ) yDiff = maxMove;
                if( yDiff < -maxMove ) yDiff = -maxMove;

                if( iv_abs(yDiff) < threshold )   m_tabs[i].m_screenY = m_tabs[i].m_targetY;
                else                            m_tabs[i].m_screenY += yDiff * moveRate;

                float sDiff = m_tabs[i].m_targetSize - m_tabs[i].m_size;
                if( iv_abs(sDiff) < threshold )   m_tabs[i].m_size = m_tabs[i].m_targetSize;
                else                            m_tabs[i].m_size += sDiff * sizeRate;

                //float aDiff = m_tabs[i].m_targetAlpha - m_tabs[i].m_alpha;
                //if( iv_abs(aDiff) < threshold )   m_tabs[i].m_alpha = m_tabs[i].m_targetAlpha;
                //else                            m_tabs[i].m_alpha += aDiff * alphaRate;
            }
        }
    }

    //
    // Advance our current highlight

    if( g_app->m_multiwinia->GameInGracePeriod() )
    {
        if( GetHighResTime() > m_currentHelpTimer + 5.0 )
        {
            m_currentHelpTimer = GetHighResTime();

            int oldHelpHighlight = m_currentHelpHighlight;

            while( true )
            {
                ++m_currentHelpHighlight;
                if( m_currentHelpHighlight >= NumHelpItems ) m_currentHelpHighlight = 0;
                if( m_tabs[m_currentHelpHighlight].m_appropriate ) break;
                if( m_currentHelpHighlight == oldHelpHighlight ) break;
            }
        
        }
    }
}


int MultiwiniaHelp::CountIdleDarwinians( Vector3 const &pos, float range, int teamId )
{
    int numFound;
    g_app->m_location->m_entityGrid->GetFriends( s_neighbours, pos.x, pos.z, range, &numFound, teamId );

    int result = 0;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId thisId = s_neighbours[i];
        Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( thisId, Entity::TypeDarwinian );
        if( darwinian && 
            !darwinian->m_dead &&
            darwinian->m_onGround &&
            darwinian->m_state == Darwinian::StateIdle )
        {
            ++result;
        }
    }

    return result;
}


int MultiwiniaHelp::CountDarwiniansGoingHere( Vector3 const &pos, int teamId )
{
    Team *team = g_app->m_location->m_teams[teamId];

    int result = 0;
    float threshold = 80;

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];
            if( entity && entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *)entity;
                float destDistance = ( darwinian->m_orders - pos ).Mag();
                if( destDistance <= threshold )
                {
                    ++result;
                }
            }
        }
    }

    return result;
}



void MultiwiniaHelp::RenderTabLink( MultiwiniaHelpItem *item )
{
    //
    // Render box highlight

    glBegin( GL_LINE_LOOP );
        glVertex2f( item->m_screenX-1, item->m_screenY-1 );
        glVertex2f( item->m_screenX + item->m_size+1, item->m_screenY-1 );
        glVertex2f( item->m_screenX + item->m_size+1, item->m_screenY + item->m_size+1 );
        glVertex2f( item->m_screenX-1, item->m_screenY + item->m_size+1 );
    glEnd();


    //
    // Render circle around target
    
    glBegin( GL_LINE_LOOP );

    int numSteps = 50;
    for( int i = 0; i < numSteps; ++i )
    {
        float angle = 2.0f * M_PI * i / (float)numSteps;
        float xPos = item->m_screenLinkX + cosf( angle ) * item->m_screenLinkSize;
        float yPos = item->m_screenLinkY + sinf( angle ) * item->m_screenLinkSize;

        glVertex2f( xPos, yPos );
    }

    glEnd();


    //
    // Connect tab to circle

    float startY = item->m_screenY + item->m_size * 0.66;

    float finalLinkX = item->m_screenLinkX;
    float finalLinkY = item->m_screenLinkY;

    if( startY < item->m_screenLinkY - item->m_screenLinkSize ||
        startY > item->m_screenLinkY + item->m_screenLinkSize )
    {
        if( startY < item->m_screenLinkY - item->m_screenLinkSize )
        {
            finalLinkY = item->m_screenLinkY - item->m_screenLinkSize;
        }
        else if( startY > item->m_screenLinkY + item->m_screenLinkSize )
        {
            finalLinkY = item->m_screenLinkY + item->m_screenLinkSize;
        }
        glBegin( GL_LINES );
            glVertex2f( item->m_screenX, startY );
            glVertex2f( item->m_screenLinkX, startY );

            glVertex2f( item->m_screenLinkX, startY );
            glVertex2f( finalLinkX, finalLinkY );
        glEnd();
    }
    else
    {
        startY = item->m_screenLinkY;
        finalLinkX = item->m_screenLinkX + item->m_screenLinkSize;
        finalLinkY = item->m_screenLinkY;

        glBegin( GL_LINES );
            glVertex2f( item->m_screenX, startY );
            glVertex2f( finalLinkX, finalLinkY );
        glEnd();
    }


}


void MultiwiniaHelp::RenderTab2d( float screenX, float screenY, float size, char *icon, float alpha )
{
    //
    // Render the background tab

    glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.8f );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    if( g_app->m_multiwinia->GameInGracePeriod() )
    {
        RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
        glColor4ub( col.r, col.g, col.b, alpha * 128 );
        glLineWidth( 2.0f );
    }
    else
    {
        glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.2f );
    }

    glBegin( GL_LINE_LOOP );
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + size, screenY );
        glVertex2f( screenX + size, screenY + size );
        glVertex2f( screenX, screenY + size );
    glEnd();
   

    glLineWidth( 1.0f );


    //
    // Render the icon

    glColor4f( 1.0f, 1.0f, 1.0f, alpha );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( icon ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex2f( screenX, screenY );
        glTexCoord2i(1,1);      glVertex2f( screenX + size, screenY );
        glTexCoord2i(1,0);      glVertex2f( screenX + size, screenY + size );
        glTexCoord2i(0,0);      glVertex2f( screenX, screenY + size );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
}


void MultiwiniaHelp::RenderTab3d( Vector3 const &pos, float width, UnicodeString &title, UnicodeString &message )
{
    bool scoresHelp = ( pos == g_zeroVector );

    //
    // Convert our position to 2d screen co-ords

    float screenX, screenY;
    g_app->m_renderer->SetupMatricesFor3D();
    bool onScreen = g_app->m_markerSystem->CalculateScreenPosition( pos, screenX, screenY );
    g_app->m_renderer->SetupMatricesFor2D();

    if( !onScreen )
    {
        screenY = g_app->m_renderer->ScreenH() - screenY;
    }

    // special case for the select task help
    if( m_currentTutorialHelp == HelpTutorialSelectTask ||
        m_currentTutorialHelp == HelpTutorial2SelectArmour )
    {
        screenX = g_app->m_renderer->ScreenW() / 2.0f;
        screenY = g_app->m_renderer->ScreenH() * 0.85f;
    }

    if( m_currentTutorialHelp == HelpTutorialDismissInfoBox )
    {
        screenX = g_app->m_renderer->ScreenW() / 2.0f;
        screenY = g_app->m_renderer->ScreenH() * 0.65f;
    }


    //
    // Render the background tab

    glDisable( GL_DEPTH_TEST );

    double timeFactor = sinf(GetHighResTime()*8.0f);
    if( scoresHelp )
    {
        timeFactor = 1.0f;
    }
    //width *= ( g_app->m_renderer->ScreenH() / 27.0f );
    width *= 35.0f;
    width *= ( 1.0f + timeFactor * 0.02f );

    float alpha = 0.9f;
    float size = width;
    float titleSize = width * 0.12f;
    float helpSize = width * 0.06f;

 
    LList<UnicodeString *> *wrappedTitle = WordWrapText( title, size, titleSize*0.6f, true);
    LList<UnicodeString *> *wrapped = WordWrapText( message, size, helpSize*0.55f, true);

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    float scaleFactor = 1.0f;
    
    size *= scaleFactor;
    titleSize *= scaleFactor;
    helpSize *= scaleFactor;

    float height = titleSize * wrappedTitle->Size() +                   
                   helpSize * wrapped->Size() + 
                   25.0 * scaleFactor;

    float tabH = height * 0.5f;
    float tabW = size * 0.5f;
    float arrowSize = tabH * 0.5f;
    float topTitleBarH = titleSize * wrappedTitle->Size() + 10.0f * scaleFactor;

    int offscreenSide = -1;

    if( screenX <= tabW + arrowSize )
    {
        // left
        screenX = tabW + arrowSize;
        offscreenSide = 1;
    }
    
    if( screenX >= g_app->m_renderer->ScreenW() - tabW - arrowSize )
    {
        // right
        screenX = g_app->m_renderer->ScreenW() - tabW - arrowSize;
        offscreenSide = 3;
    }

    if( screenY <= tabH * 2.0f )
    {
        // top
        screenY = tabH * 2.0f;
        offscreenSide = 2;
        tabH = topTitleBarH * 0.5f;
        //screenY -= topTitleBarH;
    }
    
    if( screenY >= g_app->m_renderer->ScreenH() )
    {
        // bottom
        screenY = g_app->m_renderer->ScreenH();
        offscreenSide = 4;
    }
    
    //
    // Override the position in the special case of scores help

    if( scoresHelp )
    {
        int numTeams = 2;
        float boxX = 10;
        float boxY = 10;
        float boxW = 440;
        float boxH = 50 + numTeams * 14.0f * 1.3f;

        tabW = boxW*0.5f;
        tabH *= 1.2f;
        screenX = boxX + tabW;
        screenY = boxY + tabH * 2.5f;

        helpSize = boxW * 0.04f;
        titleSize = boxW * 0.075f;
        topTitleBarH *= 0.9f;

        wrapped->EmptyAndDelete();
        delete wrapped;
        
        wrapped = WordWrapText( message, boxW, helpSize*0.6f, true);
        offscreenSide = -1;
    }
    
    screenY -= tabH * 1.5f;

    bool flashing = ( timeFactor > 0.0f );
    if( scoresHelp ) flashing = ( fmodf( g_gameTimer.GetGameTime(), 1.0f ) < 0.5f );

    glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.8f );
       
    glBegin( GL_QUADS );
        glVertex2f( screenX - tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY + tabH );
        glVertex2f( screenX - tabW, screenY + tabH );
    glEnd();

    RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
    glColor4ub( col.r, col.g, col.b, alpha * 128 );   
    if( flashing ) glColor4ub( col.r, col.g, col.b, alpha * 255 );   
    glLineWidth( 2.0f );

    glBegin( GL_LINE_LOOP );
        glVertex2f( screenX - tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY + tabH );
        glVertex2f( screenX - tabW, screenY + tabH );
    glEnd();

    //
    // Render the arrow

    if( !scoresHelp )
    {
        glBegin( GL_TRIANGLES );
        switch( offscreenSide )
        {
            case -1:        // on screen
            case 4:         // bottom
                glVertex2f( screenX - tabW, screenY + tabH );
                glVertex2f( screenX + tabW, screenY + tabH );
                glVertex2f( screenX, screenY + tabH * 1.5f );
                break;

            case 1:     // left
                glVertex2f( screenX - tabW, screenY - tabH );
                glVertex2f( screenX - tabW, screenY + tabH );
                glVertex2f( screenX - tabW - arrowSize, screenY );
                break;

            case 2:     // top
                glVertex2f( screenX - tabW, screenY - tabH );
                glVertex2f( screenX + tabW, screenY - tabH );
                glVertex2f( screenX, screenY - tabH * 2.5f );
                break;

            case 3:     // right
                glVertex2f( screenX + tabW, screenY - tabH );
                glVertex2f( screenX + tabW, screenY + tabH );
                glVertex2f( screenX + tabW + arrowSize, screenY );
                break;

        }        
        glEnd();
    }


    glColor4ub( col.r, col.g, col.b, alpha * 64 );
    if( flashing ) glColor4ub( col.r, col.g, col.b, alpha * 256 );

    glBegin( GL_QUADS );
        glVertex2f( screenX - tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY - tabH );
        glVertex2f( screenX + tabW, screenY - tabH + topTitleBarH );
        glVertex2f( screenX - tabW, screenY - tabH + topTitleBarH );
    glEnd();

    glLineWidth( 1.0f );


    //
    // Render the title

    float textPosX = screenX - tabW;    
    float textPosY = screenY - tabH;
    textPosX += 13.0f * scaleFactor;
    textPosY += 13.0f * scaleFactor;
    
    if( !scoresHelp )
    {
        for( int j = 0; j < 2; ++j )
        {
            if( j == 0 )
            {
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_titleFont.SetRenderOutline(true);
            }
            else
            {
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_titleFont.SetRenderOutline(false);
            }

            for( int i = 0; i < wrappedTitle->Size(); ++i )
            {
                UnicodeString *string = wrappedTitle->GetData(i);
                float thisTextPosY = textPosY + i * titleSize;
                g_titleFont.DrawText2D( textPosX, thisTextPosY, titleSize, *string );
            }
        }
    }

    textPosY += titleSize * wrappedTitle->Size();
        
    wrappedTitle->EmptyAndDelete();
    delete wrappedTitle;


    //
    // Render the text

    if( scoresHelp )
    {
        textPosY += titleSize * 1.2f;
    }

    if( offscreenSide != 2 )
    {
        textPosY += 10.0f * scaleFactor;

        for( int j = 0; j < 2; ++j )
        {
            if( j == 0 )
            {
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
                g_editorFont.SetRenderOutline(true);
            }
            else
            {
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
                g_editorFont.SetRenderOutline(false);
            }

            for( int i = 0; i < wrapped->Size(); ++i )
            {
                UnicodeString *string = wrapped->GetData(i);
                float thisTextPosY = textPosY + i * helpSize;
                g_editorFont.DrawText2D( textPosX, thisTextPosY, helpSize, *string );
            }
        }
    }

    wrapped->EmptyAndDelete();
    delete wrapped;

}

/*
void MultiwiniaHelp::RenderTab3d( Vector3 pos, float width, UnicodeString &title, UnicodeString &message )
{

    //
    // Render the background tab

    glDisable( GL_DEPTH_TEST );

    double timeFactor = sinf(GetHighResTime()*8.0f);
    width *= ( 1.0f + timeFactor * 0.02f );

    float alpha = 0.9f;
    float size = width;

    float titleSize = width * 0.12f;
    float helpSize = width * 0.07f;

    LList<UnicodeString *> *wrappedTitle = WordWrapText( title, size, titleSize*0.6f, true);
    LList<UnicodeString *> *wrapped = WordWrapText( message, size, helpSize*0.6f, true);

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    Vector3 camPos = g_app->m_camera->GetPos();

    float camDist = ( camPos - pos ).Mag();
    float scaleFactor = sqrtf(camDist*3.0f) * 0.6f;

    size *= scaleFactor;
    titleSize *= scaleFactor;
    helpSize *= scaleFactor;

    float height = titleSize * wrappedTitle->Size() +                   
        helpSize * wrapped->Size() + 1.0 * scaleFactor;

    Vector3 camUp = g_app->m_camera->GetUp() * height * 0.5f;
    Vector3 camRight = g_app->m_camera->GetRight() * size * 0.5f;

    Vector3 tabPos = pos + camUp * 2;

    bool flashing = ( timeFactor > 0.0f );

    glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.8f );
    if( flashing ) glColor4f( 0.2f, 0.4f, 0.2f, alpha );

    glBegin( GL_QUADS );
    glVertex3dv( (tabPos - camRight - camUp).GetData() );
    glVertex3dv( (tabPos + camRight - camUp).GetData() );
    glVertex3dv( (tabPos + camRight + camUp).GetData() );
    glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glEnd();

    RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
    glColor4ub( col.r, col.g, col.b, alpha * 128 );   
    if( flashing ) glColor4ub( col.r, col.g, col.b, alpha * 255 );   
    glLineWidth( 2.0f );

    glBegin( GL_LINE_LOOP );
    glVertex3dv( (tabPos - camRight - camUp).GetData() );
    glVertex3dv( (tabPos + camRight - camUp).GetData() );
    glVertex3dv( (tabPos + camRight + camUp).GetData() );
    glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glEnd();

    glBegin( GL_TRIANGLES );
    glVertex3dv( (tabPos - camRight - camUp).GetData() );
    glVertex3dv( (tabPos + camRight - camUp).GetData() );
    glVertex3dv( (tabPos - camUp * 1.5f).GetData() );
    glEnd();

    glColor4ub( col.r, col.g, col.b, alpha * 64 );
    Vector3 topTitleBarUp = g_app->m_camera->GetUp() * (titleSize * wrappedTitle->Size()) + g_app->m_camera->GetUp() * 4;

    glBegin( GL_QUADS );
    glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glVertex3dv( (tabPos + camRight + camUp).GetData() );
    glVertex3dv( (tabPos + camRight + camUp - topTitleBarUp).GetData() );
    glVertex3dv( (tabPos - camRight + camUp - topTitleBarUp).GetData() );
    glEnd();

    glLineWidth( 1.0f );


    //
    // Render the title

    Vector3 textPos = tabPos + camRight + camUp;
    camUp = g_app->m_camera->GetUp();
    camRight = g_app->m_camera->GetRight();
    textPos -= camRight * 0.4 * scaleFactor;
    textPos -= camUp * 0.1f * scaleFactor;
    textPos -= camUp * titleSize * 0.5f;

    for( int j = 0; j < 2; ++j )
    {
        if( j == 0 )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_titleFont.SetRenderOutline(true);
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            g_titleFont.SetRenderOutline(false);
        }

        for( int i = 0; i < wrappedTitle->Size(); ++i )
        {
            UnicodeString *string = wrappedTitle->GetData(i);
            Vector3 thisTextPos = textPos - i * camUp * titleSize;
            g_titleFont.DrawText3D( thisTextPos, titleSize, *string );
        }
    }

    textPos -= camUp * titleSize * wrappedTitle->Size();

    wrappedTitle->EmptyAndDelete();
    delete wrappedTitle;


    //
    // Render the text

    textPos -= camUp * 2;

    for( int j = 0; j < 2; ++j )
    {
        if( j == 0 )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_editorFont.SetRenderOutline(true);
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            g_editorFont.SetRenderOutline(false);
        }

        for( int i = 0; i < wrapped->Size(); ++i )
        {
            UnicodeString *string = wrapped->GetData(i);
            Vector3 thisTextPos = textPos - i * camUp * helpSize;
            g_editorFont.DrawText3D( thisTextPos, helpSize, *string );
        }
    }

    wrapped->EmptyAndDelete();
    delete wrapped;

}
*/

void MultiwiniaHelp::RenderTab3d( Vector3 const &pos, float size, char *icon, float alpha )
{
    //
    // Render the background tab

    glDisable( GL_DEPTH_TEST );

    glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.8f );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    Vector3 camPos = g_app->m_camera->GetPos();
    
    float camDist = ( camPos - pos ).Mag();
    size *= sqrtf(camDist) * 0.75f;

    Vector3 camUp = g_app->m_camera->GetUp() * size * 0.5f;
    Vector3 camRight = g_app->m_camera->GetRight() * size * 0.5f;

    Vector3 tabPos = pos + camUp * 2;

    glBegin( GL_QUADS );
        glVertex3dv( (tabPos - camRight - camUp).GetData() );
        glVertex3dv( (tabPos + camRight - camUp).GetData() );
        glVertex3dv( (tabPos + camRight + camUp).GetData() );
        glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glEnd();

    RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
    glColor4ub( col.r, col.g, col.b, alpha * 128 );
    glLineWidth( 2.0 );

    glBegin( GL_LINE_LOOP );
        glVertex3dv( (tabPos - camRight - camUp).GetData() );
        glVertex3dv( (tabPos + camRight - camUp).GetData() );
        glVertex3dv( (tabPos + camRight + camUp).GetData() );
        glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glEnd();

    glBegin( GL_TRIANGLES );
        glVertex3dv( (tabPos - camRight - camUp).GetData() );
        glVertex3dv( (tabPos + camRight - camUp).GetData() );
        glVertex3dv( (tabPos - camUp * 1.5).GetData() );
    glEnd();

    glLineWidth( 1.0 );


    //
    // Render the icon

    glColor4f( 1.0, 1.0, 1.0, alpha );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( icon ) );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    glBegin( GL_QUADS );
        glTexCoord2i(1,0);      glVertex3dv( (tabPos - camRight - camUp).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (tabPos + camRight - camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (tabPos + camRight + camUp).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (tabPos - camRight + camUp).GetData() );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_DEPTH_TEST );
}


int MultiwiniaHelp::CratesOnLevel()
{
    return Crate::s_crates;
}


int MultiwiniaHelp::SpawnPointsOwned()
{
    int teamId = g_app->m_globalWorld->m_myTeamId;
    if( teamId == 255 ) return 0;

    return SpawnPoint::s_numSpawnPoints[ teamId + 1 ];
}


int MultiwiniaHelp::SpawnPointsAvailableForCapture()
{
    int count = 0;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeSpawnPoint &&
                b->m_id.GetTeamId() == 255 )
            {
                count++;
            }
        }
    }

    return count;
}


bool MultiwiniaHelp::WallBusterOwned()
{
    if( g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(g_app->m_globalWorld->m_myTeamId)] )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeWallBuster )
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool MultiwiniaHelp::PulseBombToAttack()
{
    return (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault &&
           g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(g_app->m_globalWorld->m_myTeamId)]);
}

bool MultiwiniaHelp::PulseBombToDefend()
{
    return (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault &&
           !g_app->m_location->m_levelFile->m_attacking[g_app->m_location->GetTeamPosition(g_app->m_globalWorld->m_myTeamId)]);
}


float CalculateHighlightAlpha( float timeIndex )
{
    if( timeIndex < 1.0f ) return 0.0f;
    if( timeIndex > 2.0f ) return 1.0f;

    timeIndex -= 1.0f;
    if( fmodf( timeIndex, 0.4f ) < 0.2f ) return 1.0f;

    return 0.3f;
}

UnicodeString MultiwiniaHelp::GetHelpString( int _helpType )
{
	bool gamepad = INPUT_MODE_GAMEPAD == g_inputManager->getInputMode();
    switch( _helpType )
    {
        case HelpStartHere:             return LANGUAGEPHRASE("multiwinia_help_starthere");
        case HelpReinforcements:        return LANGUAGEPHRASE("multiwinia_help_trunkport");
        case HelpSpawnPoints:           return LANGUAGEPHRASE("multiwinia_help_spawnpoint");
        case HelpKoth:                  return LANGUAGEPHRASE("multiwinia_help_kothzone");
        case HelpLiftStatue:            return LANGUAGEPHRASE("multiwinia_help_liftstatues");
        case HelpCaptureStatue:         return LANGUAGEPHRASE("multiwinia_help_capturestatues");
        case HelpSolarPanels:           return LANGUAGEPHRASE("multiwinia_help_rocketriot2");
        case HelpRocketRiot:            return LANGUAGEPHRASE("multiwinia_help_rocketriot1");
        case HelpSpawnPointsConquer:    return LANGUAGEPHRASE("multiwinia_help_spawnpoint_dom");
        case HelpCrates:                return LANGUAGEPHRASE("multiwinia_help_crates");
        case HelpWallBuster:            return LANGUAGEPHRASE("multiwinia_help_wallbuster");
        case HelpStationAttack:         return LANGUAGEPHRASE("multiwinia_help_station_attack");
        case HelpStationDefend:         return LANGUAGEPHRASE("multiwinia_help_station_defend");
        case HelpPulseBombAttack:       return LANGUAGEPHRASE("multiwinia_help_pulsebomb_attack");
        case HelpPulseBombDefend:       return LANGUAGEPHRASE("multiwinia_help_pulsebomb_defend");
        case HelpBlitzkrieg1:           return LANGUAGEPHRASE("multiwinia_help_blitzkrieg1");
        case HelpBlitzkrieg2:           return LANGUAGEPHRASE("multiwinia_help_blitzkrieg2");
        case HelpTutorialGroupSelect:	
			return LANGUAGEPHRASE( gamepad ? "multiwinia_help_groupselect_xbox" : "multiwinia_help_groupselect" );
        case HelpTutorialMoveToSpawnPoint:
#ifdef TARGET_OS_MACOSX
			return LANGUAGEPHRASE("multiwinia_help_movetospawnpoint_mac");
#else
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_movetospawnpoint_xbox" : "multiwinia_help_movetospawnpoint" );
#endif
        case HelpTutorialMoveToMainZone:
            return LANGUAGEPHRASE(gamepad ? "multiwinia_help_movetomainzone_xbox" : "multiwinia_help_movetomainzone" );
        case HelpTutorialPromote:
            return LANGUAGEPHRASE(gamepad ? "multiwinia_help_promote_xbox" : "multiwinia_help_promote" );
        case HelpTutorialPromoteAdvanced:
            return LANGUAGEPHRASE(gamepad ? "multiwinia_help_promote_advanced_xbox" : "multiwinia_help_promote_advanced" );
        case HelpTutorialSendToZone:    
            return LANGUAGEPHRASE(gamepad ? "multiwinia_help_sendtozone_xbox" : "multiwinia_help_sendtozone" );
        case HelpTutorialCaptureZone:           return LANGUAGEPHRASE( "multiwinia_help_capturezone" );
        case HelpTutorialScores:                return LANGUAGEPHRASE( "multiwinia_help_scores" );
        case HelpTutorialCrateFalling:          return LANGUAGEPHRASE("multiwinia_help_cratefalling" );
        case HelpTutorialCrateCollect:          return LANGUAGEPHRASE("multiwinia_help_cratecollect" );
        case HelpTutorialSelectTask:            
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_selecttask_xbox" : "multiwinia_help_selecttask");
        case HelpTutorialUseAirstrike:          
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_useairstrike_xbox" : "multiwinia_help_useairstrike" );

        case HelpTutorial2SelectRadar:          
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_radardish_xbox" : "multiwinia_help_radardish");
        case HelpTutorial2AimRadar:             
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_aimdish_xbox" : "multiwinia_help_aimdish");
        case HelpTutorial2Deselect:             
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_deselect_xbox" : "multiwinia_help_deselect" );
        case HelpTutorial2UseRadar:             
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_useradar_xbox" : "multiwinia_help_useradar") ;
        case HelpTutorial2RadarTransit:         return LANGUAGEPHRASE("multiwinia_help_radartransit");
        case HelpTutorial2OrderInRadar:         
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_orderradar_xbox" : "multiwinia_help_orderradar");
        case HelpTutorial2FormationSetup:       
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_makeformation_xbox" : "multiwinia_help_makeformation");
        case HelpTutorial2FormationMove:        
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_moveformation_xbox" : "multiwinia_help_moveformation" );
        case HelpTutorial2FormationInfo:        return LANGUAGEPHRASE("multiwinia_help_formationsinfo");
        case HelpTutorial2FormationAttack:      return LANGUAGEPHRASE("multiwinia_help_fattack" );
        case HelpTutorial2SelectArmour:         
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_selectarmour_xbox" : "multiwinia_help_selectarmour");
        case HelpTutorial2PlaceArmour:          
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_placearmour_xbox" : "multiwinia_help_placearmour");
        case HelpTutorial2LoadArmour:           
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_loadarmour_xbox" : "multiwinia_help_loadarmour" );
        case HelpTutorial2MoveArmour:           
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_movearmour_xbox" : "multiwinia_help_movearmour");
        case HelpTutorial2UnloadArmour:         
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_unloadarmour_xbox" : "multiwinia_help_unloadarmour") ;
        case HelpTutorial2CaptureStation:       return LANGUAGEPHRASE("multiwinia_help_station" );
        case HelpTutorial2End:                  return LANGUAGEPHRASE("multiwinia_help_win");
        case HelpTutorial2BreakFormation:       
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_breakformation_xbox" : "multiwinia_help_breakformation" );
        case HelpTutorialDismissInfoBox:        
			return LANGUAGEPHRASE(gamepad ? "multiwinia_help_clearinfo_xbox" : "multiwinia_help_clearinfo");

        default:    return UnicodeString();
    }
}

UnicodeString MultiwiniaHelp::GetTitleString(int _helpType)
{
     switch( _helpType )
    {
        case HelpStartHere:                 return LANGUAGEPHRASE("multiwinia_help_title_starthere");
        case HelpReinforcements:            return LANGUAGEPHRASE("multiwinia_help_title_trunkport");
        case HelpSpawnPoints:               return LANGUAGEPHRASE("multiwinia_help_title_spawnpoint");
        case HelpKoth:                      return LANGUAGEPHRASE("multiwinia_help_title_kothzone");
        case HelpLiftStatue:                return LANGUAGEPHRASE("multiwinia_help_title_liftstatues");
        case HelpCaptureStatue:             return LANGUAGEPHRASE("multiwinia_help_title_capturestatues");
        case HelpSolarPanels:               return LANGUAGEPHRASE("multiwinia_help_title_rocketriot2");
        case HelpRocketRiot:                return LANGUAGEPHRASE("multiwinia_help_title_rocketriot1");
        case HelpSpawnPointsConquer:        return LANGUAGEPHRASE("multiwinia_help_title_spawnpoint_dom");
        case HelpCrates:                    return LANGUAGEPHRASE("multiwinia_help_title_crates");
        case HelpWallBuster:                return LANGUAGEPHRASE("multiwinia_help_title_wallbuster");
        case HelpStationAttack:             return LANGUAGEPHRASE("multiwinia_help_title_station_attack");
        case HelpStationDefend:             return LANGUAGEPHRASE("multiwinia_help_title_station_defend");
        case HelpPulseBombAttack:           return LANGUAGEPHRASE("multiwinia_help_title_pulsebomb_attack");
        case HelpPulseBombDefend:           return LANGUAGEPHRASE("multiwinia_help_title_pulsebomb_defend");
        case HelpBlitzkrieg1:               return LANGUAGEPHRASE("multiwinia_help_title_blitzkrieg1");
        case HelpBlitzkrieg2:               return LANGUAGEPHRASE("multiwinia_help_title_blitzkrieg2");
        case HelpTutorialGroupSelect:       return LANGUAGEPHRASE("multiwinia_help_title_groupselect");
        case HelpTutorialMoveToSpawnPoint:  return LANGUAGEPHRASE("multiwinia_help_title_movetospawnpoint" );
        case HelpTutorialMoveToMainZone:    return LANGUAGEPHRASE("multiwinia_help_title_movetomainzone" );
        case HelpTutorialPromote:           return LANGUAGEPHRASE("multiwinia_help_title_promote" );
        case HelpTutorialPromoteAdvanced:   return LANGUAGEPHRASE("multiwinia_help_title_promote" );
        case HelpTutorialSendToZone:        return LANGUAGEPHRASE("multiwinia_help_title_sendtozone" );        
        case HelpTutorialCaptureZone:       return LANGUAGEPHRASE("multiwinia_help_title_capturezone" );
        case HelpTutorialScores:            return LANGUAGEPHRASE("multiwinia_help_title_scores" );
        case HelpTutorialCrateCollect:      return LANGUAGEPHRASE("multiwinia_help_title_cratecapture");
        case HelpTutorialCrateFalling:      return LANGUAGEPHRASE("multiwinia_help_title_crates" );
        case HelpTutorialSelectTask:        return LANGUAGEPHRASE("multiwinia_help_title_taskselect");
        case HelpTutorialUseAirstrike:      return LANGUAGEPHRASE("multiwinia_help_title_airstrike");

        case HelpTutorial2SelectRadar:      return LANGUAGEPHRASE("multiwinia_help_title_radardish");
        case HelpTutorial2AimRadar:         return LANGUAGEPHRASE("multiwinia_help_title_aimdish");
        case HelpTutorial2Deselect:         return LANGUAGEPHRASE("multiwinia_help_title_deselect");
        case HelpTutorial2UseRadar:         return LANGUAGEPHRASE("multiwinia_help_title_useradar" );
        case HelpTutorial2RadarTransit:     return LANGUAGEPHRASE("multiwinia_help_title_radartransit" );
        case HelpTutorial2OrderInRadar:     return LANGUAGEPHRASE("multiwinia_help_title_useradar");
        case HelpTutorial2FormationSetup:   return LANGUAGEPHRASE("multiwinia_help_title_makeformation");
        case HelpTutorial2FormationMove:    return LANGUAGEPHRASE("multiwinia_help_title_moveformation");
        case HelpTutorial2FormationInfo:    return LANGUAGEPHRASE("multiwinia_help_title_formations");
        case HelpTutorial2FormationAttack:  return LANGUAGEPHRASE("multiwinia_help_title_fattack");
        case HelpTutorial2SelectArmour:     return LANGUAGEPHRASE("multiwinia_help_title_selectarmour");
        case HelpTutorial2PlaceArmour:      return LANGUAGEPHRASE("multiwinia_help_title_placearmour") ;
        case HelpTutorial2LoadArmour:       return LANGUAGEPHRASE("multiwinia_help_title_loadarmour" );
        case HelpTutorial2MoveArmour:       return LANGUAGEPHRASE("multiwinia_help_title_movearmour") ;
        case HelpTutorial2UnloadArmour:     return LANGUAGEPHRASE("multiwinia_help_title_unloadarmour");
        case HelpTutorial2CaptureStation:   return LANGUAGEPHRASE("multiwinia_help_title_station");
        case HelpTutorial2End:              return LANGUAGEPHRASE("multiwinia_help_title_win");
        case HelpTutorial2BreakFormation:   return LANGUAGEPHRASE("multiwinia_help_title_breakformation" );

        case HelpTutorialDismissInfoBox:    return LANGUAGEPHRASE("multiwinia_help_title_clearinfo");

        default:    return UnicodeString();
    }
}

void MultiwiniaHelp::Render()
{
    if( !g_app->m_location ) return;
    if( !g_app->Multiplayer() ) return;
    if( g_app->m_hideInterface ) return;
    if( g_app->m_multiwinia->GameOver() ) return;
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault && g_app->m_multiwinia->m_victoryTimer > 0.0f ) return;
    if( !g_app->m_location->GetMyTeam() ) return;

    START_PROFILE("Render MultiwiniaHelp");

    glDisable   ( GL_CULL_FACE );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable    ( GL_BLEND );

    g_app->m_renderer->SetupMatricesFor2D();

#ifdef INCLUDE_TUTORIAL
    if( g_app->m_multiwiniaTutorial )
    {
        if( !g_app->m_multiwinia->GameInGracePeriod() )
        {
            //g_app->m_renderer->SetupMatricesFor3D();

            RenderTutorialHelp();
        }
    }
    else
#endif
    {
#ifdef THIS_IS_FUCKING_SHIT_AND_IT_ALWAYS_HAS_BEEN
        for( int i = 0; i < NumHelpItems; ++i )
        {
            if( m_tabs[i].m_appropriate )
            {
                float alpha = m_tabs[i].m_alpha * 0.8f;
                
                if( g_app->m_multiwinia->GameInGracePeriod() && i == m_currentHelpHighlight )
                {
                    float highlightAlpha = CalculateHighlightAlpha( GetHighResTime() - m_currentHelpTimer );
                    glColor4f( 0.0f, 0.0f, 0.0f, 0.3f * highlightAlpha );
                    glLineWidth( 8.0f );
                    RenderTabLink( &m_tabs[i] );

                    RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
                    glColor4ub( col.r, col.g, col.b, highlightAlpha* 255 );
                    glLineWidth( 2.0f );
                    RenderTabLink( &m_tabs[i] );

                    glBegin( GL_QUADS );
                        glVertex2f( m_tabs[i].m_screenX,  m_tabs[i].m_screenY );
                        glVertex2f( m_tabs[i].m_screenX + m_tabs[i].m_size,  m_tabs[i].m_screenY );
                        glVertex2f( m_tabs[i].m_screenX + m_tabs[i].m_size,  m_tabs[i].m_screenY + m_tabs[i].m_size );
                        glVertex2f( m_tabs[i].m_screenX,  m_tabs[i].m_screenY + m_tabs[i].m_size );
                    glEnd();
                }
                    
                RenderTab2d( m_tabs[i].m_screenX, 
                             m_tabs[i].m_screenY, 
                             m_tabs[i].m_size, 
                             m_tabs[i].m_iconFilename, 
                             alpha );

                UnicodeString titleText = GetTitleString(i);
                if( titleText.Length() > 0 )
                {
                    float fontSize = m_tabs[i].m_size / 5.0f;
                    float width = g_titleFont.GetTextWidthReal( titleText.Length(), fontSize );
                    if( width > m_tabs[i].m_size )
                    {
                        fontSize *= m_tabs[i].m_size / (width * 1.05f);
                    }
                    int x = m_tabs[i].m_screenX + m_tabs[i].m_size * 0.5f;
                    int y = m_tabs[i].m_screenY + m_tabs[i].m_size * 0.05f;

                    g_titleFont.DrawText2DCentre( x, y, fontSize, titleText );
                }

                UnicodeString helpText = GetHelpString( i );
                if( helpText.Length() > 0 )
                {
                    //helpText.StrUpr();
                    float fontSize = m_tabs[i].m_size / 12.0f;
                    LList<UnicodeString *> *wrapped = WordWrapText( helpText, m_tabs[i].m_size * 1.5f, fontSize, true);

                    int x = m_tabs[i].m_screenX + m_tabs[i].m_size * 0.5f;
                    int y = m_tabs[i].m_screenY + m_tabs[i].m_size * 0.95f;

                    for( int i = 0; i < wrapped->Size(); ++i )
                    {
                        g_editorFont.DrawText2DCentre( x, y, fontSize, *wrapped->GetData(i) );
                        y+=fontSize;
                    }
				    wrapped->EmptyAndDelete();
                    delete wrapped;
                }
            }
        }
#endif
    }

    g_app->m_renderer->SetupMatricesFor2D();

    if( g_app->m_multiwinia->GameInGracePeriod() )
    {       
        if( g_app->m_multiwiniaTutorial )
        {
            RenderGracePeriodHelp();
        }
        else if( g_app->m_multiwinia->m_gracePeriodTimer >= GRACE_TIME )
        {
            RenderAnimatedPanel();
        }
    }
    else
    {
        if( g_prefsManager->GetInt( "HelpEnabled", 1 ) )
        {
            bool gamepad = INPUT_MODE_GAMEPAD == g_inputManager->getInputMode();

            if( m_showSpaceDeselectHelp > 0.0f )
            {
                float alpha = 1.0f;
                alpha = std::min( alpha, m_showSpaceDeselectHelp );
                           
			    if( m_spaceDeselectShowRMB )
				{
				#ifdef TARGET_OS_MACOSX
					RenderHelpMessage( alpha, LANGUAGEPHRASE("multiwinia_rmbhelp_mac"), false, true );
				#else
					RenderHelpMessage( alpha, LANGUAGEPHRASE(gamepad ? "multiwinia_rmbhelp_xbox" : "multiwinia_rmbhelp"), false );
				#endif
                }
				else
					RenderHelpMessage( alpha, LANGUAGEPHRASE(gamepad ? "multiwinia_deselect_xbox" : "multiwinia_deselect"), false );

                m_showSpaceDeselectHelp -= g_advanceTime;
            }
            else if( m_showSpaceDeselectHelp < 0.0f )
            {
                m_showSpaceDeselectHelp += g_advanceTime * 0.33f;
                m_showSpaceDeselectHelp = std::min( m_showSpaceDeselectHelp, 0.0f );
            }

            if( m_showShiftToSpeedUpHelp > 0.0f && !gamepad )
            {
                float alpha = 1.0f;
                alpha = std::min( alpha, m_showShiftToSpeedUpHelp );

                RenderHelpMessage( alpha, LANGUAGEPHRASE("multiwinia_shiftspeedhelp") );
                
                m_showShiftToSpeedUpHelp -= g_advanceTime;
            }


            Team *team = g_app->m_location->GetMyTeam();
            if( team )
            {
                //
                // Help for deselecting current building

                if( team->m_currentBuildingId != -1 )
                {
                    Building *building = g_app->m_location->GetBuilding( team->m_currentBuildingId );
                    if( building )
                    {
                        if( building->m_type == Building::TypeGunTurret ||
                            building->m_type == Building::TypeRadarDish )
                        {
					        RenderHelpMessage( 1.0f, LANGUAGEPHRASE(gamepad?"multiwinia_deselect_xbox":"multiwinia_deselect"), true );
                        }
                    }
                }

                //
                // Help for terminating an officer

                WorldObjectId idUnderMouse = g_app->m_gameCursor->m_objUnderMouse;
                Entity *entity = g_app->m_location->GetEntity( idUnderMouse );
                if( entity && entity->m_type == Entity::TypeOfficer )
                {
                    Officer *officer = (Officer *)entity;
                    if( officer->m_orders == Officer::OrderNone )
                    {
                        RenderHelpMessage( 1.0f, LANGUAGEPHRASE(gamepad?"multiwinia_officerterminate_xbox":"multiwinia_officerterminate") );
                    }
                }

                //
                // Help for toggling armour orders

                if( g_app->m_markerSystem->m_isOrderMarkerUnderMouse )
                {
                    Armour *armour = (Armour *) g_app->m_location->GetEntitySafe( g_app->m_markerSystem->m_markerUnderMouse, Entity::TypeArmour );
                    if( armour && armour->m_conversionOrder == Armour::StateIdle )
                    {
                        RenderHelpMessage( 1.0f, LANGUAGEPHRASE(gamepad?"multiwinia_armourtoggle_xbox":"multiwinia_armourtoggle" ) ); 
                    }
                }
            }
        }
    }

    g_app->m_renderer->SetupMatricesFor3D();    

    END_PROFILE("Render MultiwiniaHelp");
}


void MultiwiniaHelp::NotifyCapturedCrate()
{
    m_tabs[HelpCrates].m_accomplished++;
}


void MultiwiniaHelp::NotifyCapturedKothZone( int numZones )
{
    m_tabs[HelpKoth].m_accomplished = std::max( m_tabs[HelpKoth].m_accomplished, numZones );
}


void MultiwiniaHelp::NotifyLiftedStatue()
{
    m_tabs[HelpLiftStatue].m_accomplished = true;
}


void MultiwiniaHelp::NotifyCapturedStatue()
{
    m_tabs[HelpCaptureStatue].m_accomplished = true;
}


void MultiwiniaHelp::NotifyGroupSelectedDarwinians()
{
    ++m_groupSelected;

    if( m_groupSelected >= 1 && m_promoted >= 1 )
    {
        m_tabs[HelpDarwinians].m_accomplished = true;
    }
}


void MultiwiniaHelp::NotifyMadeOfficer()
{
    ++m_promoted;

    if( m_groupSelected >= 1 && m_promoted >= 1 )
    {
        m_tabs[HelpDarwinians].m_accomplished = true;
    }
}


void MultiwiniaHelp::NotifyFueledUp()
{
    m_tabs[HelpRocketRiot].m_accomplished = true;
    m_tabs[HelpSolarPanels].m_accomplished = true;
}


void MultiwiniaHelp::NotifyReinforcements()
{
    ++m_reinforcements;

    if( m_reinforcements >= 3 )
    {
        m_tabs[HelpReinforcements].m_accomplished = true;
    }
}




bool MultiwiniaHelp::HighlightingDarwinian()
{
    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
    WorldObjectId darwinianId = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );

    Team *team = g_app->m_location->GetMyTeam();

    if( darwinianId.IsValid() &&
        team->m_numDarwiniansSelected == 0 &&
        team->m_selectionCircleCentre == g_zeroVector &&
        !team->GetMyEntity() &&
        !team->GetMyUnit() )
    {
        return true;
    }

    return false;
}


void MultiwiniaHelp::RenderCurrentUnitHelp( float iconCentreX, float iconCentreY, float iconSize, int entityType )
{
	char icon[512];
	float width = 120;
	float height = width * (120/150.0f);

	switch( entityType )
	{
		case Entity::TypeOfficer:		sprintf(icon, "mwhelp/officercontrol");			break;
		case Entity::TypeDarwinian:		sprintf(icon, "mwhelp/darwiniancontrol");		break;
	}

	if( m_inputMode == INPUT_MODE_GAMEPAD )
	{
		strcat( icon, "_xbox" );
	}

	strcat( icon, "." TEXTURE_EXTENSION );

	const BitmapRGBA *bitmap = g_app->m_resource->GetBitmap( icon );
	if( bitmap )
	{		
		height = width * bitmap->m_height / (float)bitmap->m_width;
	}

	float screenX = iconCentreX - width/2;
	float screenY = iconCentreY + iconSize/2 + 5;
	float alpha = 0.9f;

	//
	// Render the background tab

	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f       ( 0.0f, 0.0f, 0.0f, alpha * 0.8f );

	glBegin( GL_QUADS );
		glVertex2f( screenX, screenY );
		glVertex2f( screenX + width, screenY );
		glVertex2f( screenX + width, screenY + height );
		glVertex2f( screenX, screenY + height );
	glEnd();

    glColor4f       ( 1.0f, 1.0f, 1.0f, alpha * 0.1f );

    glBegin( GL_LINE_LOOP);
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + width, screenY );
        glVertex2f( screenX + width, screenY + height );
        glVertex2f( screenX, screenY + height );
    glEnd();


	//
	// Render the icon

	glColor4f( 1.0f, 1.0f, 1.0f, alpha );
    glEnable        ( GL_TEXTURE_2D );

	glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( icon ) );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

	glBegin( GL_QUADS );
	glTexCoord2i(0,1);      glVertex2f( screenX, screenY );
	glTexCoord2i(1,1);      glVertex2f( screenX + width, screenY );
	glTexCoord2i(1,0);      glVertex2f( screenX + width, screenY + height );
	glTexCoord2i(0,0);      glVertex2f( screenX, screenY + height );
	glEnd();

	glDisable       ( GL_TEXTURE_2D );
}

void MultiwiniaHelp::RenderCurrentTaskHelp( float _iconCentreX, float _iconCentreY, float _iconSize, int _taskId )
{
	float width = 100;
	float height = 100;

	int helpType = GetHelpType( _taskId );
    if( helpType != -1 )
    {
	    float screenX = _iconCentreX - width/2;
	    float screenY = _iconCentreY + _iconSize/2 + 5;
	    float alpha = 0.9f;

	    //
	    // Render the background tab

        glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.8f );
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	    glBegin( GL_QUADS );
		    glVertex2f( screenX, screenY );
		    glVertex2f( screenX + width, screenY );
		    glVertex2f( screenX + width, screenY + height );
		    glVertex2f( screenX, screenY + height );
	    glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 0.1f * alpha );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + width, screenY );
            glVertex2f( screenX + width, screenY + height );
            glVertex2f( screenX, screenY + height );
        glEnd();

	    //
	    // Render the icon

	    glColor4f( 1.0f, 1.0f, 1.0f, alpha );

        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( m_tabs[helpType].m_iconFilename ) );
	    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_TEXTURE_2D );

	    glBegin( GL_QUADS );
	    glTexCoord2i(0,1);      glVertex2f( screenX, screenY );
	    glTexCoord2i(1,1);      glVertex2f( screenX + width, screenY );
	    glTexCoord2i(1,0);      glVertex2f( screenX + width, screenY + height );
	    glTexCoord2i(0,0);      glVertex2f( screenX, screenY + height );
	    glEnd();

	    glDisable       ( GL_TEXTURE_2D );
    }
}


void MultiwiniaHelp::RenderGracePeriodHelp()
{
    if( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
    {
        return;
    }

    float w = g_app->m_renderer->ScreenH() * 0.2f;
    if( g_app->m_multiwiniaTutorial ) w *= 1.5f;
    float h = w * 128.0f / 304.0f;
    
    float screenX = g_app->m_renderer->ScreenW()/2.0f - w/2.0f;
    float screenY = g_app->m_renderer->ScreenH() * 0.70f;

    //
    // Render a fill box

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glColor4f( 0.0f, 0.0f, 0.0f, 0.6f );

    if( g_app->m_multiwiniaTutorial &&
        fmodf(GetHighResTime(),1) < 0.3f )
    {
        glColor4f( 0.4f, 0.4f, 0.4f, 0.8f );
    }

    glBegin( GL_QUADS );
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + w, screenY );
        glVertex2f( screenX + w, screenY + h );
        glVertex2f( screenX, screenY + h );
    glEnd();

    //
    // Render an outer line
    
    glColor4f( 1.0f, 1.0f, 1.0f, 0.2f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( screenX, screenY );
        glVertex2f( screenX + w, screenY );
        glVertex2f( screenX + w, screenY + h );
        glVertex2f( screenX, screenY + h );
    glEnd();

    //
    // Render the icon

	const char *filename;

	if( InputPrefs::IsKeyboardFrenchLocale() )
		filename = "mwhelp/control_camera_french." TEXTURE_EXTENSION;
	else
		filename = "mwhelp/control_camera." TEXTURE_EXTENSION;

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ) );
    glEnable( GL_TEXTURE_2D );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    
    float texW = ( 304.0f / 512.0f );
    float texX = ( 1.0f - texW ) / 2.0f;

    glBegin( GL_QUADS );
        glTexCoord2f(texX,1);           glVertex2f( screenX, screenY );
        glTexCoord2f(texX+texW,1);      glVertex2f( screenX + w, screenY );
        glTexCoord2f(texX+texW,0);      glVertex2f( screenX + w, screenY + h );
        glTexCoord2f(texX,0);           glVertex2f( screenX, screenY + h );
    glEnd();

    glDisable( GL_TEXTURE_2D );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void MultiwiniaHelp::RenderControlHelp()
{
    //
    // Look up all objects we are controlling, highlighting etc

    Team *myTeam = g_app->m_location->GetMyTeam();
    if( !myTeam ) return;

    Task *task = myTeam->m_taskManager->GetCurrentTask();
    bool taskStarted = task && task->m_state == Task::StateStarted;


    WorldObjectId idUnderMouse;
    g_app->m_renderer->SetupMatricesFor3D();
    bool objectUnderMouse = g_app->m_locationInput->GetObjectUnderMouse( idUnderMouse, g_app->m_globalWorld->m_myTeamId );
    if( idUnderMouse.GetUnitId() == UNIT_BUILDINGS )
    {
        objectUnderMouse = false;
        Building *building = g_app->m_location->GetBuilding( idUnderMouse.GetUniqueId() );
        if( building ) 
        {
            if( building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeGunTurret )
            {
                objectUnderMouse = true;
            }
        }
    }
    g_app->m_renderer->SetupMatricesFor2D();

    Entity *entityUnderMouse = g_app->m_location->GetEntity( idUnderMouse );
    bool officerUndermouse = ( entityUnderMouse && entityUnderMouse->m_type == Entity::TypeOfficer );

    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
    WorldObjectId darwinianUnderMouseId = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );
    Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( darwinianUnderMouseId, Entity::TypeDarwinian );


    bool objectSelected =   myTeam->m_currentUnitId != -1 ||
                            myTeam->m_currentEntityId != -1 ||
                            myTeam->m_currentBuildingId != -1 ||
                            myTeam->m_numDarwiniansSelected > 0;
    

    //
    // Render our help based on our selections and highlights

    if( myTeam->m_taskManager->m_mostRecentTaskId != -1 )
    {
        // Looking at a newly run task
        RenderControlHelp( 1, "multiwinia_control_close", "mouse_lmb", "button_a", 1.0f, 0.0f );
        RenderControlHelp( 2 );
    }
    else if( myTeam->m_selectionCircleCentre != g_zeroVector )
    {
        // Currently expanding/holding a selection circle
        RenderControlHelp( 1, "multiwinia_control_select", "mouse_lmb_hold", "button_a_hold", 1.0f, 1.0f );
        RenderControlHelp( 2 );

        if( myTeam->m_numDarwiniansSelected > 0 ) RenderDeselect();
    }
    else if( objectSelected )
    {
        // Something is selected and under our control
        float action1 = g_inputManager->controlEvent(ControlIssueDarwinianOrders) ? 1.0f : 0.0f;

        bool tutorial1Highlight = ( m_currentTutorialHelp == HelpTutorialMoveToSpawnPoint ) &&
                                    sinf(GetHighResTime()*8.0f) > 0.0f;

        Entity *selected = myTeam->GetMyEntity();
        Unit *selectedUnit = myTeam->GetMyUnit();
        Building *building = g_app->m_location->GetBuilding( myTeam->m_currentBuildingId );

        if( selectedUnit && selectedUnit->m_troopType == Entity::TypeInsertionSquadie &&
            (g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) ) )
        {
            action1 = (g_inputManager->controlEvent( ControlUnitSecondaryFireTarget ) || g_inputManager->controlEvent( ControlUnitSecondaryFireDirected ) ) ? 1.0f : 0.0f;
            RenderControlHelp( 1, "multiwinia_control_grenade", "mouse_lmb", "button_rt", 1.0f, action1 );
        }
        else if( !building )
        {
            RenderControlHelp( 1, "multiwinia_control_move", "mouse_lmb", "button_a", 1.0f, action1, tutorial1Highlight );
        }

        if( myTeam->m_numDarwiniansSelected > 0 )
        {
            // Darwinians selected

            RenderControlHelp( 2 );
        }
        else if( selected )
        {
            if( selected->m_type == Entity::TypeOfficer )
            {
                // Officer selected
                Officer *officer = (Officer *)selected;
                Vector3 mousePos = myTeam->m_currentMousePos;
                bool formationToggle = officer->IsFormationToggle( mousePos );

                if( formationToggle )                
                {
                    float action4 = g_inputManager->controlEvent(ControlIssueSpecialOrders) ? 1.0f : 0.0f;
                    RenderControlHelp( 2, "multiwinia_control_formation", "mouse_rmb", "button_x", 1.0f, action4 );
                }
                else
                {
                    float action2 = g_inputManager->controlEvent(ControlIssueSpecialOrders) ? 1.0f : 0.0f;

                    bool tutorial2Highlight = ( m_currentTutorialHelp == HelpTutorialSendToZone ) &&
                                                sinf(GetHighResTime()*8.0f) > 0.0f;

                    RenderControlHelp( 2, "multiwinia_control_order", "mouse_rmb", "button_x", 1.0f, action2, tutorial2Highlight );
                }
                
            }
            else if( selected->m_type == Entity::TypeArmour )
            {
                // Armour selected
                Armour *armour = (Armour *)selected;

                if( armour->IsLoading() || armour->GetNumPassengers() > 0 )
                {
                    RenderControlHelp( 2, "multiwinia_control_unload", "mouse_rmb", "button_x", 1.0f );
                }
                else
                {                   
                    RenderControlHelp( 2, "multiwinia_control_load", "mouse_rmb", "button_x", 1.0f );
                }
            }
            else if( selected->m_type == Entity::TypeTank )
            {
                float highlight = 0.0f;
                if( g_inputManager->controlEvent( ControlUnitPrimaryFireTarget ) ||
                    g_inputManager->controlEvent( ControlIssueSpecialOrders ) ) 
                {
                    highlight = 1.0f;
                }
                RenderControlHelp(2, "control_help_secondaryfire", "mouse_rmb", "button_x", 1.0f, highlight );
            }
        }
        else if( selectedUnit )
        {
            if( selectedUnit->m_troopType == Entity::TypeInsertionSquadie )
            {
                RenderControlHelp( 2, "multiwinia_control_laser", "mouse_rmb", "button_ra", 1.0f );
            }
        }

        if( selected || selectedUnit )
        {
            float action3 = g_inputManager->controlEvent(ControlIconsTaskManagerEndTask) ? 1.0f : 0.0f;
            RenderControlHelp( 3, "multiwinia_control_destroy", "key_c", "button_y", 0.5f, action3 );
        }

        RenderDeselect();
    }
    else if( task )
    {
        // Placing a task
        
        float action1 = g_inputManager->controlEvent(ControlUnitCreate) ? 1.0f : 0.0f;
        float action3 = g_inputManager->controlEvent(ControlIconsTaskManagerEndTask) ? 1.0f : 0.0f;

        RenderControlHelp( 1, "multiwinia_control_place", "mouse_lmb", "button_a", 1.0f, action1 );
        RenderControlHelp( 3, "multiwinia_control_destroy", "key_c", "button_y", 0.5f, action3 );

        RenderDeselect();
    }
    else if( darwinian && !objectUnderMouse )
    {
        // Highlighting a Darwinian

        float alpha = 0.5f;

        if( darwinian && darwinian->IsSelectable() ) alpha = 1.0f;

        float action1 = g_inputManager->controlEvent(ControlEnlargeSelectionCircle) ? 1.0f : 0.0f;
        float action2 = g_inputManager->controlEvent(ControlDarwinianPromote) ? 1.0f : 0.0f;

        bool tutorial1Highlight = ( m_currentTutorialHelp == HelpTutorialGroupSelect ) &&
                                    sinf(GetHighResTime()*8.0f) > 0.0f;

        bool tutorial2Highlight = ( m_currentTutorialHelp == HelpTutorialPromote ) &&
                                    sinf(GetHighResTime()*8.0f) > 0.0f;

        RenderControlHelp( 1, "multiwinia_control_select", "mouse_lmb_hold", "button_a_hold", alpha, action1, tutorial1Highlight );
        RenderControlHelp( 2, "multiwinia_control_promote", "mouse_rmb", "button_x", alpha, action2, tutorial2Highlight );
    }
    else if( objectUnderMouse )
    {
        // Highlighting an object under the mouse
        // but definately not a darwinian

        float action1 = g_inputManager->controlEvent(ControlUnitSelect) ? 1.0f : 0.0f;
        RenderControlHelp( 1, "multiwinia_control_select", "mouse_lmb", "button_a", 1.0f, action1 );
    }
    else
    {
        // Not doing anything

        float alpha = 0.5f;

        float action1 = g_inputManager->controlEvent(ControlEnlargeSelectionCircle) ? 1.0f : 0.0f;
        float action2 = g_inputManager->controlEvent(ControlDarwinianPromote) ? 1.0f : 0.0f;

        if( myTeam->m_numDarwiniansSelected == 0 ) action1 = 0.0f;

        RenderControlHelp( 1, "multiwinia_control_select", "mouse_lmb_hold", "button_a_hold", alpha, action1 );
        RenderControlHelp( 2, "multiwinia_control_promote", "mouse_rmb", "button_x", alpha, action2 );
    }

}


void MultiwiniaHelp::RenderDeselect()
{    
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;    
    float fontSize = g_app->m_renderer->ScreenH() * 0.015f;
    float screenY = g_app->m_renderer->ScreenH() - fontSize;    

    UnicodeString phrase = LANGUAGEPHRASE("multiwinia_deselect");
    if( INPUT_MODE_GAMEPAD == g_inputManager->getInputMode() )
    {
        phrase = LANGUAGEPHRASE("multiwinia_deselect_xbox");
    }
	phrase.StrUpr();

    glColor4f( 0.7f, 0.7f, 0.7f, 0.0f );
    g_gameFont.SetRenderOutline(true);
	g_gameFont.DrawText2DCentre( screenX, screenY, fontSize, phrase );

    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    g_gameFont.SetRenderOutline(false);
    g_gameFont.DrawText2DCentre( screenX, screenY, fontSize, phrase );
}


void MultiwiniaHelp::RenderControlHelp( int position, char *caption, char *iconKb, char *iconXbox, float alpha, float highlight, bool tutorialHighlight )
{
    float overallScale = 1.0f;

    bool keyboardInput = ( g_inputManager->getInputMode() == INPUT_MODE_KEYBOARD );
    char *icon = ( keyboardInput ? iconKb : iconXbox );


    // Position arrangement :       1       2
    //                              3       4

    if( alpha <= 0.0f ) return;
    if( alpha > 1.0f ) alpha = 1.0f;
    if( !caption && !iconKb && !iconXbox ) alpha = 0.3f;

    if( position < 1 || position > 4 ) return;

    // Note : first element is not used

    static float s_timers[]     = { -1.0f, -1.0f, -1.0f, -1.0f, -1.0f };
    static char *s_captions[]   = { NULL, NULL, NULL, NULL, NULL };
    static char *s_icons[]      = { NULL, NULL, NULL, NULL, NULL };

    static double s_lastHighlight = 0.0f;

    //
    // Handle new requests for highlights
        
    if( highlight > 0.0f )
    {
        //if( s_timers[position] <= 0.0f )
        {
            s_captions[position] = caption;
            s_icons[position] = icon;
        }
        s_timers[position] = std::max( s_timers[position], highlight );
    }


    //
    // Advance previous highlights

    bool highlightExists = false;

    for( int i = 1; i <= 4; ++i )
    {
        if( s_timers[i] > 0.0f ) 
        {
            highlightExists = true;
            s_lastHighlight = GetHighResTime();
            if( position == i )
            {
                caption = s_captions[position];
                icon = s_icons[position];
            }
            s_timers[i] -= g_advanceTime * 0.5f;
        }
    }


    float currentTimer = ( s_timers[position] );
    if( currentTimer > 0.0f ) currentTimer = 1.0f;
    if( alpha < currentTimer ) alpha = currentTimer;
    

    float colouredIn = ( ( icon || caption ) && currentTimer > 0.0f ) ? 1.0f : 0.0f;


    //
    // Where will our box be on screen?

    int leftOrRight = ( position == 1 || position == 3 ? -1 : +1 );
    int upOrDown = ( position == 3 || position == 4 ? +1 : 0 );

    float w = g_app->m_renderer->ScreenH() * 0.15f * overallScale;
    float h = w * 0.3f;

	// Make the boxes a little bigger for the other languages
	if( stricmp( g_prefsManager->GetString( "TextLanguage", "english" ), "english" ) != 0 )
		w *= 1.2; 

    float screenY = g_app->m_renderer->ScreenH() * 0.75f;    
    float screenX = g_app->m_renderer->ScreenW() * 0.5f;    
    screenX += leftOrRight * w * 0.6f;
    screenX -= w/2.0f;
    screenY += upOrDown * h * 1.2f;
    
    /*
        Strange bit of code by Chris
        That causes the left/right mouse button help to "lag" behind the cursor
        which looks quite nice sometimes

        float mouseX = g_target->X();
        float mouseY = g_target->Y();

        static float screenDX = 0;
        static float screenDY = 0;

        float newDX = ( mouseX - g_app->m_renderer->ScreenW()/2.0f ) * 0.5f;
        float newDY = ( mouseY - g_app->m_renderer->ScreenH()/2.0f ) * 0.5f;

        float timeFactor = g_advanceTime;
        screenDX = screenDX * (1-timeFactor) + newDX * timeFactor;
        screenDY = screenDY * (1-timeFactor) + newDY * timeFactor; 
        
        screenY += screenDY;
        screenX += screenDX;*/


    //
    // Pulsate

    if( alpha > 0.5f && !g_app->m_multiwiniaTutorial &&
        !highlightExists && GetHighResTime() > s_lastHighlight + 1.0f )
    {
        if( position == 1 || position == 2 )
        {
            float timeIndex = GetHighResTime() * 6 + (position-1) * 3.0f;

            if( sinf( timeIndex ) > 0.1f ) colouredIn = std::max( colouredIn, 0.7f );
        }
    }


    // 
    // Background box + outline
 
    if( icon || caption )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glColor4f( 0.0f, 0.0f, 0.0f, alpha * 0.75f );
        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, alpha * 0.2f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
    }


    //
    // Highlight

    if( colouredIn > 0.0f )
    {
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;

        if( colouredIn >= 1.0f && 
            strcmp( caption, "multiwinia_control_select" ) != 0  )
        {
            float timeFactor = sinf( GetHighResTime() * 50 );
            if( timeFactor > 0.5f ) colouredIn = 0.1f;
        }

        glColor4ub( col.r * 0.7f, col.g * 0.7f, col.b * 0.7f, alpha * colouredIn * 255 );   

        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        float b = 2;

        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX-b, screenY-b );
            glVertex2f( screenX + w+b, screenY-b );
            glVertex2f( screenX + w+b, screenY + h+b );
            glVertex2f( screenX-b, screenY + h+b );
        glEnd();

    }
   

    //
    // Tutorial Highlight

    if( tutorialHighlight )
    {
        RGBAColour col = g_app->m_location->GetMyTeam()->m_colour;
        glColor4ub( col.r, col.g, col.b, alpha * 255 );   

        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        glBegin( GL_QUADS );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
        
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBegin( GL_LINE_LOOP );
            glVertex2f( screenX, screenY );
            glVertex2f( screenX + w, screenY );
            glVertex2f( screenX + w, screenY + h );
            glVertex2f( screenX, screenY + h );
        glEnd();
    }


    //
    // Icon

    if( icon )
    {
        char fullFilename[512];
        sprintf( fullFilename, "icons/%s.bmp", icon );
        int iconTexId = g_app->m_resource->GetTexture( fullFilename );
        const BitmapRGBA *bitmap = g_app->m_resource->GetBitmap( fullFilename );

        float iconSize = w * 0.25f;
        float iconW = bitmap->m_width/64.0f * iconSize;
        float iconH = bitmap->m_height/64.0f * iconSize;
        float iconCentreX = screenX + w * 0.5f;
        float iconCentreY = screenY + iconSize * 1.1f - iconH * 0.5f;
                
        iconCentreX += leftOrRight * w * -0.5f;
        iconCentreX += leftOrRight * iconW * 0.5f;
        iconCentreX += leftOrRight * w * 0.05f;

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        
        glBindTexture   ( GL_TEXTURE_2D, iconTexId );
        glEnable        ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(1,1);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY-iconH/2.0f );
            glTexCoord2i(1,0);      glVertex2f( iconCentreX+iconW/2.0f, iconCentreY+iconH/2.0f );
            glTexCoord2i(0,0);      glVertex2f( iconCentreX-iconW/2.0f, iconCentreY+iconH/2.0f  );
        glEnd();

        glDisable( GL_TEXTURE_2D );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    
    }


    //
    // Caption

    if( caption )
    {
        UnicodeString translatedCaption = LANGUAGEPHRASE( caption );
		translatedCaption.StrUpr();

        float fontX = screenX + w / 2.0f;
        float fontY = screenY + h * 0.5f;        
        float fontSize = h * 0.4f;
        
        if( upOrDown > 0 && leftOrRight > 0 ) fontX += leftOrRight * w * 0.1f;
        fontX += leftOrRight * w * 0.1f;

        glColor4f( alpha, alpha, alpha, 0.0f);
        g_gameFont.SetRenderOutline(true);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_gameFont.SetRenderOutline(false);
        g_gameFont.DrawText2DCentre( fontX, fontY, fontSize, translatedCaption );
    }
}


void MultiwiniaHelp::RenderTutorialHelp()
{
    for( int i = 0; i < NumHelpItems; ++i )
    {
        if( m_tabs[i].m_appropriate )
        {
            float alpha = m_tabs[i].m_alpha * 0.8f;
            float size = 8;

            Vector3 worldPos = m_tabs[i].m_worldPos + g_upVector * m_tabs[i].m_worldOffset;
			UnicodeString titleString = GetTitleString(i);
			UnicodeString helpString = GetHelpString(i);
            RenderTab3d( worldPos, size, titleString, helpString);

            //UnicodeString titleText = GetTitleString(i);
        
            //UnicodeString helpText = GetHelpString( i );
            //if( helpText.Length() > 0 )
            //{
            //    //helpText.StrUpr();
            //    float fontSize = m_tabs[i].m_size / 15.0f;
            //    LList<UnicodeString *> *wrapped = WordWrapText( helpText, m_tabs[i].m_size * 1.5f, fontSize, true);

            //    int x = m_tabs[i].m_screenX + m_tabs[i].m_size * 0.5f;
            //    int y = m_tabs[i].m_screenY + m_tabs[i].m_size * 0.95f;

            //    for( int i = 0; i < wrapped->Size(); ++i )
            //    {
            //        g_titleFont.DrawText2DCentre( x, y, fontSize, *wrapped->GetData(i) );
            //        y+=fontSize;
            //    }
            //    wrapped->EmptyAndDelete();
            //    delete wrapped;
            //}
        }
    }
}


void MultiwiniaHelp::RenderBasicControlHelp()
{
    Team *team = g_app->m_location->GetMyTeam();
    bool somethingSelected = false;
    static double s_leftTimer = -1.0f;
    static double s_rightTimer = -1.0f;    
    
    if( team->m_currentUnitId != -1 ||
        team->m_currentEntityId != -1 ||
        team->m_currentBuildingId != -1 ||        
        team->m_numDarwiniansSelected > 0 ||        
        team->m_taskManager->m_tasks.Size() > 0 )
    {
        somethingSelected = true;        
    }

    if( s_leftTimer > 0.0f )    s_leftTimer -= g_advanceTime;
    if( s_rightTimer > 0.0f )   s_rightTimer -= g_advanceTime;


    for( int i = 0; i < 2; ++i )
    {
        float w = g_app->m_renderer->ScreenH() * 0.15f;
        float h = w * 50.0f/ 150.0f;
        float screenY = g_app->m_renderer->ScreenH() * 0.8f;

        float alpha = ( somethingSelected ? 0.0f : 0.8f );

        if( i == 0 && s_leftTimer > 0.0f )
        {
            alpha = std::max( alpha, 0.8f * (float)s_leftTimer );
        }
        if( i == 1 && s_rightTimer > 0.0f )
        {
            alpha = std::max( alpha, 0.8f * (float)s_rightTimer );
        }

        float screenX = g_app->m_renderer->ScreenW()/2.0f;
        if( i == 0 ) screenX -= w * 0.6f;
        if( i == 1 ) screenX += w * 0.6f;
        screenX -= w/2.0f;

        if( alpha > 0.0f )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.2f * alpha );
            glBegin( GL_LINE_LOOP );
                glVertex2f( screenX, screenY );
                glVertex2f( screenX + w, screenY );
                glVertex2f( screenX + w, screenY + h );
                glVertex2f( screenX, screenY + h );
            glEnd();
            
            glColor4f( 1.0f, 1.0f, 1.0f, alpha );

            char *filename = NULL;

            if( m_inputMode == INPUT_MODE_KEYBOARD )
            {
                if( i == 0 )    filename = "mwhelp/basic_select." TEXTURE_EXTENSION;    
                else            filename = "mwhelp/basic_promote." TEXTURE_EXTENSION;    
            }                
            else
            {
                if( i == 0 )    filename = "mwhelp/basic_select_xbox." TEXTURE_EXTENSION;    
                else            filename = "mwhelp/basic_promote_xbox." TEXTURE_EXTENSION;    
            }

            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ) );
            glEnable( GL_TEXTURE_2D );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glBegin( GL_QUADS );
                glTexCoord2i(0,1);      glVertex2f( screenX, screenY );
                glTexCoord2i(1,1);      glVertex2f( screenX + w, screenY );
                glTexCoord2i(1,0);      glVertex2f( screenX + w, screenY + h );
                glTexCoord2i(0,0);      glVertex2f( screenX, screenY + h );
            glEnd();

            glDisable( GL_TEXTURE_2D );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            float highlight = 0.0f;
            if( i == 0 && g_inputManager->controlEvent(ControlEnlargeSelectionCircle) ) 
            {
                s_leftTimer = 1.0f;                             
            }
            if( i == 1 && g_inputManager->controlEvent(ControlDarwinianPromote) ) 
            {
                s_rightTimer = 1.0f;                
            }

            if( i == 1 )
            {
                Team *team = g_app->m_location->GetMyTeam();
                if( team->m_numDarwiniansSelected == 0 &&
                    team->m_currentUnitId == -1 &&
                    team->m_currentEntityId == -1 &&
                    team->m_currentBuildingId == -1 )
                {
                    Vector3 mousePos = g_app->m_userInput->GetMousePos3d();
                    WorldObjectId id = Task::FindDarwinian( mousePos, g_app->m_globalWorld->m_myTeamId );
                    Darwinian *darwinian = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
                    if( darwinian && darwinian->IsSelectable() )
                    {
                        highlight = 0.25f;
                    }
                }
            }

            if( i == 0 && s_leftTimer > 0.0 ) highlight = s_leftTimer;
            if( i == 1 && s_rightTimer > 0.0f ) highlight = s_rightTimer;

            if( highlight > 0.0f )
            {
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                glColor4f( 0.5f, 0.5f, 1.0f, highlight * 0.75f * alpha );
                glBegin( GL_QUADS );
                    glVertex2f( screenX, screenY );
                    glVertex2f( screenX + w, screenY );
                    glVertex2f( screenX + w, screenY + h );
                    glVertex2f( screenX, screenY + h );
                glEnd();

                glColor4f( 1.0f, 1.0f, 1.0f, highlight * alpha );
                glBegin( GL_LINE_LOOP );
                    glVertex2f( screenX, screenY );
                    glVertex2f( screenX + w, screenY );
                    glVertex2f( screenX + w, screenY + h );
                    glVertex2f( screenX, screenY + h );
                glEnd();
            }
        }
    }
}


void MultiwiniaHelp::InitialiseAnimatedPanel()
{
    TextReader *reader = NULL;

    //
    // Choose an animation 

    switch( g_app->m_multiwinia->m_gameType )
    {
        case Multiwinia::GameTypeSkirmish:          reader = g_app->m_resource->GetTextReader( "animations/domination.anim" );          break;
        case Multiwinia::GameTypeKingOfTheHill:     reader = g_app->m_resource->GetTextReader( "animations/koth.anim" );                break;
        case Multiwinia::GameTypeCaptureTheStatue:  reader = g_app->m_resource->GetTextReader( "animations/cts.anim" );                 break;
        case Multiwinia::GameTypeAssault:           reader = g_app->m_resource->GetTextReader( "animations/assault_defend.anim" );      break;
        case Multiwinia::GameTypeRocketRiot:        reader = g_app->m_resource->GetTextReader( "animations/rocket.anim" );              break;
        case Multiwinia::GameTypeBlitzkreig:        reader = g_app->m_resource->GetTextReader( "animations/blitz.anim" );               break;
    }

    if( !reader ) return;

    m_animatedPanel = new AnimatedPanelRenderer();
    m_animatedPanel->Read( reader );
    delete reader;

    float scale = g_app->m_renderer->ScreenH()/1300.0f;    

    m_animatedPanel->m_screenX = g_app->m_renderer->ScreenW()/2.0f - scale * m_animatedPanel->m_width/2.0f;
    m_animatedPanel->m_screenY = g_app->m_renderer->ScreenH() -  m_animatedPanel->m_height * 1.15f * scale;
    m_animatedPanel->m_screenW = m_animatedPanel->m_width * scale;
    m_animatedPanel->m_screenH = m_animatedPanel->m_height * scale;

    m_animatedPanel->m_borderSize = 5;
    m_animatedPanel->m_titleH = 25;
    m_animatedPanel->m_captionH = 15;
}




void MultiwiniaHelp::RenderAnimatedPanel()
{    
    if( m_animatedPanel )
    {        
        m_animatedPanel->RenderBackground( 0.9f );
        m_animatedPanel->RenderObjects();

        //
        // Title

        MultiwiniaGameBlueprint *blueprint = g_app->m_multiwinia->s_gameBlueprints[ g_app->m_multiwinia->m_gameType ];
        UnicodeString gameName = LANGUAGEPHRASE( blueprint->GetName() );
        gameName.StrUpr();
        m_animatedPanel->RenderTitle( gameName );

        //
        // Caption

        char stringId[256];
        sprintf( stringId, "multiwinia_animdesc_%s", blueprint->m_name );

        UnicodeString caption = LANGUAGEPHRASE( stringId );
        m_animatedPanel->RenderCaption( caption );
    }
}


void MultiwiniaHelp::ShowSpaceDeselectHelp( bool immediate, bool showRMB )
{
    if( m_showSpaceDeselectHelp > 0.0f )
    {
        m_showSpaceDeselectHelp += 1.0f;
        m_showSpaceDeselectHelp = std::min( m_showSpaceDeselectHelp, 3.0f );
    }
    else if( m_showSpaceDeselectHelp <= 0.0f )
    {
        m_showSpaceDeselectHelp -= 0.75f;
        if( m_showSpaceDeselectHelp < -1.0f || immediate ) 
        {
            m_spaceDeselectShowRMB = showRMB;
            m_showSpaceDeselectHelp = 3.0f;
        }
    }    
}


void MultiwiniaHelp::ShowShiftSpeedupHelp()
{
    m_showShiftToSpeedUpHelp = 3.0f;
}


void MultiwiniaHelp::RenderHelpMessage( float alpha, UnicodeString caption, bool buildingPosition, bool oversize )
{
    // Building Position means render the text to the left of the screen, rather than centre
    // So it doesnt clash with the building in focus

	float w;
    if (oversize)
		w = g_app->m_renderer->ScreenH() * 0.55f;
	else
		w = g_app->m_renderer->ScreenH() * 0.35f;
    float h = g_app->m_renderer->ScreenH() * 0.03f;
    float x = g_app->m_renderer->ScreenW()/2.0f - w/2.0f;
    float y = g_app->m_renderer->ScreenH() * 0.78f;
    float fontH = g_app->m_renderer->ScreenH() * 0.02f;
    
    LList<UnicodeString *> *wrapped = WordWrapText( caption, w, fontH, false );
    h = (wrapped->Size()+1) * fontH;

    if( buildingPosition ) 
    {
        x = g_app->m_renderer->ScreenW() * 0.1f;
        y = g_app->m_renderer->ScreenH() - h * 1.5f;
    }

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    //
    // Background box

    glColor4f( 0.0f, 0.0f, 0.0f, alpha*0.75f );
    glBegin( GL_QUADS );
        glVertex2f( x, y );
        glVertex2f( x+w, y );
        glVertex2f( x+w, y+h );
        glVertex2f( x, y+h );
    glEnd();


    //
    // Fill effect if the player is pressing the direction keys

    if( buildingPosition )
    {
        if( g_inputManager->controlEvent( ControlCameraLeft ) ||
            g_inputManager->controlEvent( ControlCameraRight ) ||
            g_inputManager->controlEvent( ControlCameraForwards ) ||
            g_inputManager->controlEvent( ControlCameraBackwards ) )
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.6f );
            glBegin( GL_QUADS );
                glVertex2f( x, y );
                glVertex2f( x+w, y );
                glVertex2f( x+w, y+h );
                glVertex2f( x, y+h );
            glEnd();
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }
    }


    //
    // Line border 

    glColor4f( 1.0f, 1.0f, 1.0f, alpha*0.5f );
    glBegin( GL_LINE_LOOP );
        glVertex2f( x, y );
        glVertex2f( x+w, y );
        glVertex2f( x+w, y+h );
        glVertex2f( x, y+h );
    glEnd();


    //
    // Caption

    for( int i = 0; i < wrapped->Size(); ++i )
    {
        UnicodeString *thisLine = wrapped->GetData(i);

        glColor4f( alpha, alpha, alpha, 0.0f );
        g_editorFont.SetRenderOutline(true);
        g_editorFont.DrawText2DCentre( x + w/2.0f, y + fontH/2.0f + 7, fontH, *thisLine);

        glColor4f( 1.0f, 1.0f, 1.0f, alpha );
        g_editorFont.SetRenderOutline(false);
        g_editorFont.DrawText2DCentre( x + w/2.0f, y + fontH/2.0f + 7, fontH, *thisLine);

        y += fontH;
    }

    wrapped->EmptyAndDelete();
    delete wrapped;
}


int MultiwiniaHelp::GetHelpType( int _taskId )
{
    return -1;
}

int MultiwiniaHelp::GetResearchType(int _helpType)
{
    return -1;
}

MultiwiniaHelpItem::MultiwiniaHelpItem()
:   m_appropriate(true),
    m_accomplished(0),
    m_screenX(0.0f),
    m_screenY(0.0f),
    m_size(0.0f),
    m_alpha(0.0f),
    m_renderMe(false),
    m_worldSize(0.0f),
    m_worldOffset(0.0f),
    m_screenLinkX(0.0f),
    m_screenLinkY(0.0f),
    m_screenLinkSize(0.0f),
    m_targetX(0.0f),
    m_targetY(0.0f),
    m_targetSize(0.0f)
{
}
