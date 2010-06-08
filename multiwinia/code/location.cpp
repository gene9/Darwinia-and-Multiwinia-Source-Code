#include "lib/universal_include.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <float.h>

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#include "lib/FixedPipeline.h"
#endif
#include "lib/tosser/bounded_array.h"
#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/input/input.h"
#include "lib/profiler.h"
#include "lib/language_table.h"
#include "lib/math_utils.h"
#include "lib/matrix33.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"
#include "lib/filesys/filesys_utils.h"

#include "interface/prefs_other_window.h"
#include "interface/chatinput_window.h"

#include "network/servertoclientletter.h"
#include "network/clienttoserver.h"

#include "app.h"
#include "camera.h"
#include "clouds.h"
#include "deform.h"
#include "entity_grid.h"
#include "entity_grid_cache.h"
#include "global_world.h"
#include "landscape.h"
#include "level_file.h"
#include "location.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "main.h"
#include "obstruction_grid.h"
#include "particle_system.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "water.h"
#include "gamecursor.h"
#include "script.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "multiwinia.h"
#include "markersystem.h"
#include "multiwiniahelp.h"

#include "worldobject/weapons.h"
#include "worldobject/factory.h"
#include "worldobject/worldobject.h"
#include "worldobject/cave.h"
#include "worldobject/teleport.h"
#include "worldobject/engineer.h"
#include "worldobject/lasertrooper.h"
#include "worldobject/officer.h"
#include "worldobject/armour.h"
#include "worldobject/darwinian.h"
#include "worldobject/snow.h"
#include "worldobject/tree.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/ai.h"
#include "worldobject/shaman.h"
#include "worldobject/spiritreceiver.h"
#include "worldobject/harvester.h"
#include "worldobject/crate.h"
#include "worldobject/gunturret.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/portal.h"
#include "worldobject/dustball.h"
#include "worldobject/lightning.h"
#include "worldobject/pulsebomb.h"
#include "worldobject/tank.h"
#include "worldobject/restrictionzone.h"
#include "worldobject/aiobjective.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/generator.h"
#include "worldobject/laserfence.h"
#include "worldobject/multiwiniazone.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/controlstation.h"

#include "sound/soundsystem.h"

std::vector<WorldObjectId> Location::s_neighbours;
bool Location::s_neighboursInUse = false;


// ****************************************************************************
//  Class Location
// ****************************************************************************

// *** Constructor
Location::Location()
:   m_missionComplete(false),
	m_entityGrid(NULL),
    m_obstructionGrid(NULL),
	m_levelFile(NULL),
    m_clouds(NULL),
    m_water(NULL),
    m_lastSliceProcessed(0),
    m_christmasTimer(-99.9),
    m_spawnSync(0.0),
    m_crateTimer(0.0),
    m_allowMonsterTeam(true),
    m_messageTimer(0.0),
    m_message(UnicodeString()),
    m_spawnManiaTimer(0.0),
    m_bitzkreigTimer(0.0),
    m_messageTeam(-1),
    m_showCrateDropMessage(true),
    m_entityGridCache(NULL),
    m_sortedBuildingsAlphas(NULL),
    m_sizeSortedBuildingsAlphas(0),
    m_teamsInitialised(false),
    m_messageNoOverwrite(false),
    m_christmasMode(false)
{
	for (int i = 0; i < NUM_TEAMS; i++)
    {
		m_teams[i] = NULL;
        m_teamPosition[i] = -1;
        m_coopGroups[i] = 0;
    }

    memset( m_groupColourId, -1, sizeof( m_groupColourId ) );
    memset( m_coopGroupColourTaken, false, sizeof(m_coopGroupColourTaken) );

	m_spirits.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_lasers.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_effects.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_buildings.SetTotalNumSlices(NUM_SLICES_PER_FRAME);

    m_spirits.SetSize( 100 );

    m_lights.SetStepSize(1);
	m_buildings.SetStepSize(10);
    m_spirits.SetStepSize( 100 );
	m_lasers.SetStepSize(100);
	m_effects.SetStepSize(100);
}


// *** Destructor
Location::~Location()
{
	Empty();
    Entity::s_entityPopulation = 0;

	for (int i = 0; i < NUM_TEAMS; i++)
		delete m_teams[i];

	delete m_entityGridCache;

	if( m_sortedBuildingsAlphas )
		delete [] m_sortedBuildingsAlphas;

    m_chatMessages.EmptyAndDelete();
}


void Location::Init( char const *_missionFilename, char const *_mapFilename )
{
	double t = GetHighResTime(); 
	
    LoadLevel(_missionFilename, _mapFilename);
    AppSeedRandom( 1 );

    GlobalWorld::s_nextUniqueBuildingId = 0;
    Crate::s_crates = 0;
    Crate::s_spaceShipUsed = false;
    AIObjective::s_objectivesInitialised = false;
    Armour::s_armourObjectives.Empty();
    Engineer::s_buildingIds.EmptyAndDelete();
    Engineer::s_researchIds.EmptyAndDelete();
    SpawnPopulationLock::Reset();
    SoulDestroyer::s_numSoulDestroyers = 0;
    DustBall::s_vortexTimer = 0.0f;
    DustBall::s_vortexPos = g_zeroVector;
    if( Unit::s_offsets )
    {
        SAFE_DELETE_ARRAY( Unit::s_offsets );
    }
    for( int i = 0; i < NUM_TEAMS; ++i ) 
    {
        AI::s_ai[i] = NULL;
        SpawnPoint::s_numSpawnPoints[i] = 0;
//        Portal::s_masterPortals[i] = NULL;
        GetTeamPosition(i);
        PulseBomb::s_pulseBombs[i] = -1;
        MultiwiniaZone::s_blitzkriegBaseZone[i] = -1;
    }

    if( g_app->m_clientToServer->m_christmasMode )
    {
        g_app->m_clientToServer->m_christmasMode = false;
        m_christmasMode = true;
    }

	AppDebugOut("Location::Init Step 1 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
    InitLights();
	AppDebugOut("Location::Init Step 2 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
    InitLandscape();
	AppDebugOut("Location::Init Step 3 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();

    m_water = new Water();	
	AppDebugOut("Location::Init Step 4 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();

    //memset( m_teamPosition, -1, sizeof(int) * NUM_TEAMS);

    // if we're in coop mode, set up our team positions, colours, allies etc
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
    {
        SetupAssault();
    }
    else if( g_app->m_multiwinia->m_coopMode )
    {
        for( int i = 0; i < m_levelFile->m_numPlayers; ++i ) 
        {
            if( i != GetMonsterTeamId() &&
                i != GetFuturewinianTeamId() &&
                m_coopGroups[i] == 0 )
            {        
                SetupCoop( i );
            }
        }
    }
    else
    {
        // set up the cpu team colours now
        if( !g_app->m_multiwinia->m_coopMode && g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault )
        {
            for( int i = 0; i < m_levelFile->m_numPlayers; ++i ) 
            {
                if( i != GetMonsterTeamId() &&
                    i != GetFuturewinianTeamId() &&
                    g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeCPU )
                {
                    g_app->m_multiwinia->m_teams[i].m_colourId = -1;
                    int colId = g_app->m_multiwinia->GetCPUColour();
                    if( g_app->m_multiwiniaTutorial )
                    {
                        // We want green and red in tutorial
                        colId = i;
                    }
                    g_app->m_multiwinia->m_teams[i].m_colourId = colId;
                    g_app->m_multiwinia->m_teams[i].m_colour = g_app->m_multiwinia->GetColour( colId );
                }
            }
        }
    }
	AppDebugOut("Location::Init Step 5 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();

    if( !g_app->m_editing )
    {
        InitBuildings();
        RecalculateAITargets();

		AppDebugOut("Location::Init Step 5.1 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();

        double obstrGridSize = 64.f;
        if( g_app->Multiplayer() ) obstrGridSize = 32.0;

        m_entityGrid        = new EntityGrid( 32.0, 32.0 );
        m_entityGridCache   = new EntityGridCache();
        m_obstructionGrid   = new ObstructionGrid( obstrGridSize, obstrGridSize );

		AppDebugOut("Location::Init Step 5.2 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();

        m_clouds            = new Clouds();
        
		AppDebugOut("Location::Init Step 5.3 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
        m_routingSystem.Initialise();
		m_seaRoutingSystem.Initialise();

		AppDebugOut("Location::Init Step 5.4 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
    }
    else
    {
	    for (int i = 0; i < m_levelFile->m_buildings.Size(); i++)
	    {
		    Building *building = m_levelFile->m_buildings.GetData(i);
            building->m_pos.y = m_landscape.m_heightMap->GetValue( building->m_pos.x, building->m_pos.z );
        }
    }

	AppDebugOut("Location::Init Step 6 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
    
    InitTeams();  

    LList<int> usedColours;
    if( m_levelFile->m_numPlayers < 4 )
    {
        // move unused team colours to unused teams so that things which use random team colours
        // (magical forest, randomiser etc) have the full colour range
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( i == GetMonsterTeamId() || 
                i == GetFuturewinianTeamId() ) continue;

            if( i < m_levelFile->m_numPlayers )
            {
                usedColours.PutData(m_teams[i]->m_lobbyTeam->m_colourId);
            }
            else
            {
                int id = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS / 2;
                while(true)
                {
                    bool used = false;
                    for( int j = 0; j < usedColours.Size(); ++j )
                    {
                        if( id == usedColours[j] ) used = true;
                    }

                    if( !used ) 
                    {
                        m_teams[i]->m_colour = g_app->m_multiwinia->GetColour(id);
                        break;
                    }
                    id = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS / 2;
                }
            }
        }
    }

	AppDebugOut("Location::Init Step 7 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
	
	if( m_levelFile->m_levelDifficulty == -1 )
	{
		// Remember the difficulty factor that this with which this level was created.
		m_levelFile->m_levelDifficulty = g_app->m_difficultyLevel;
	}
	else 
	{
		// Set difficulty level to the created level difficulty
		g_app->m_difficultyLevel = m_levelFile->m_levelDifficulty;		
	}   

    m_allowMonsterTeam = true;

    if( g_app->Multiplayer() )
    {
		AppDebugOut("Location::Init Step 8 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
        InitMonsterTeam( GetMonsterTeamId() );
		AppDebugOut("Location::Init Step 9 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
        InitFuturewinianTeam( GetFuturewinianTeamId() );
		AppDebugOut("Location::Init Step 10 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
        for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
        {
            g_app->m_globalWorld->m_research->m_researchLevel[i] = 3;
        }   

        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeSquad] = 1;
        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeLaser] = 2;
        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeGrenade] = 2;
        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeRocket] = 2;
        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeAirStrike] = 2;
        g_app->m_globalWorld->m_research->m_researchLevel[GlobalResearch::TypeDarwinian] = 4;
    }
	AppDebugOut("Location::Init Step 11 took %f seconds\n", GetHighResTime() - t ); t = GetHighResTime();
}

void Location::Empty()
{
    if( m_water ) {delete m_water;				m_water = NULL; }

	m_landscape.Empty();

	m_lights.EmptyAndDelete();			  // LList <Light *>
	m_buildings.EmptyAndDelete(); // LList <Building *>
	m_spirits.Empty();			  // FastDArray <Spirit>
    m_lasers.Empty();
    m_effects.EmptyAndDelete();

    if( Unit::s_offsets )
    {
        SAFE_DELETE_ARRAY( Unit::s_offsets );
    }

	for (int i = 0; i < NUM_TEAMS; i++) 
	{
		delete m_teams[i];
		m_teams[i] = NULL;
	}

    if( m_levelFile ) {         delete m_levelFile;			m_levelFile = NULL; }
    if( m_entityGrid ) {        delete m_entityGrid;		m_entityGrid = NULL; }
    if( m_entityGridCache ) {   delete m_entityGridCache;   m_entityGridCache = NULL;   }
    if( m_obstructionGrid ) {   delete m_obstructionGrid;	m_obstructionGrid = NULL; }
    if( m_clouds ) {            delete m_clouds;			m_clouds = NULL;    }

    AIObjective::s_objectivesInitialised = false;
    Armour::s_armourObjectives.Empty();

    Engineer::s_buildingIds.EmptyAndDelete();
    Engineer::s_researchIds.EmptyAndDelete();
}


void Location::LoadLevel(char const *missionFilename, char const *mapFilename)
{    
	m_levelFile = new LevelFile(missionFilename, mapFilename, (strcmp( GetExtensionPart(mapFilename), "txt" ) == 0 ));


    //
    // Are the objectives already completed on entry?
    // If so, suppress congratulations message

    if( MissionComplete() )
    {
        m_missionComplete = true;

        GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(g_app->m_requestedLocationId);
        gloc->m_missionCompleted = true;
    }
	
}


void Location::InitLandscape()
{
	m_landscape.Init(&m_levelFile->m_landscape);
}


void Location::InitLights()
{	
	for (int i = 0; i < m_levelFile->m_lights.Size(); i++)
	{
		Light *levelLight = m_levelFile->m_lights.GetData(i);
		Light *light = new Light();
		light->SetColour(levelLight->m_colour);
		light->SetFront(levelLight->m_front);
		m_lights.PutData(light);
	}
}


void Location::InitBuildings()
{
    int highestId = -1;
	LList<int> stations;
	WorldObjectId pulseBomb;

	for (int i = 0; i < m_levelFile->m_buildings.Size(); i++)
	{
		Building *building = m_levelFile->m_buildings.GetData(i);
        
        Building *existing = g_app->m_location->GetBuilding(building->m_id.GetUniqueId());
        if( existing )
        {
            AppReleaseAssert( false,
                           "Error loading level file...duplicate building found\n"
                           "Map filename = %s, Mission filename = %s\n"
                           "Existing building type = %s, new building type = %s",
                           m_levelFile->m_mapFilename,
                           m_levelFile->m_missionFilename,
                           Building::GetTypeName(existing->m_type),
                           Building::GetTypeName(building->m_type) );
        }
        Building *newBuilding = Building::CreateBuilding( building->m_type );
        m_buildings.PutData(newBuilding);
        //newBuilding->m_id = building->m_id;
        if( building->m_id.GetTeamId() != 255 &&
            building->m_id.GetTeamId() != -1 )
        {
            building->SetTeamId( GetTeamFromPosition( building->m_id.GetTeamId() ) );                
        }

        newBuilding->Initialise( building );        
        newBuilding->SetDetail( 1 );  
        if( building->m_id.GetUniqueId() > highestId ) highestId = building->m_id.GetUniqueId();

		if( building->m_type == Building::TypeControlStation ) stations.PutData(building->m_id.GetUniqueId());
		else if( building->m_type == Building::TypePulseBomb ) pulseBomb = building->m_id;
	}

	// Set up control station dishes for assault
	if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault && pulseBomb.GetUniqueId() != -1 )
	{
		Building* pb = GetBuilding(pulseBomb.GetUniqueId());
		if( pb && pb->m_type == Building::TypePulseBomb )
		{
			PulseBomb *pulse = (PulseBomb*) pb;
			for( int i = 0; i < stations.Size(); i++ )
			{
				if( stations.ValidIndex(i) )
				{
					Building* b = GetBuilding(stations.GetData(i));
					if( b && 
						b->m_type == Building::TypeControlStation )
					{
						for( int j = 0; j < pulse->m_links.Size(); j++ )
						{
							if( pulse->m_links.ValidIndex(j) )
							{
								if( b->m_id.GetUniqueId() == pulse->m_links.GetData(j) )
								{
									ControlStation* cs = (ControlStation*)b;
									cs->LinkToPulseBomb(pulseBomb.GetUniqueId());
								}
							}
						}
						
					}
				}
			}
		}
	}

    GlobalWorld::s_nextUniqueBuildingId = highestId + 1;
}


void Location::InitTeams()
{	
    for (int i = 0; i < NUM_TEAMS; i++) 
    {
	    LobbyTeam &lobbyTeam = g_app->m_multiwinia->m_teams[i];
        if( lobbyTeam.m_colourId == -1 )
        {
            // this is a failsafe condition to prevent teams without valid colour ids
            lobbyTeam.m_colourId = g_app->m_multiwinia->GetNextAvailableColour();
            lobbyTeam.m_colour = g_app->m_multiwinia->GetColour( lobbyTeam.m_colourId );
        }

	    m_teams[i] = new Team( lobbyTeam );

	    if (lobbyTeam.m_teamType != TeamTypeUnused)
	    {
		    InitialiseTeam( i );
	    }
    }
    m_teamsInitialised = true;
}


// *** SpawnEntities
// Returns id of last entity spawned
// Only useful if we've only spawned one entity, eg an engineer
WorldObjectId Location::SpawnEntities( Vector3 const &_pos, unsigned char _teamId, int _unitId,
                                       unsigned char _type, int _numEntities, Vector3 const &_vel, 
                                       double _spread, double _range, int _routeId, int _routeWaypointId )
{
    if( _teamId == GetMonsterTeamId() &&
        m_teams[_teamId]->m_teamType == TeamTypeUnused )
    {
        // no monster team yet, set it up now
        InitMonsterTeam( _teamId );
    }

    if( _teamId == GetFuturewinianTeamId() &&
        m_teams[_teamId]->m_teamType == TeamTypeUnused )
    {
        InitFuturewinianTeam(_teamId);
    }

    AppDebugAssert( _teamId < NUM_TEAMS &&
                    _teamId >= 0 &&
                    m_teams[_teamId]->m_teamType > TeamTypeUnused );
       
    Team *team = m_teams[_teamId];
    WorldObjectId entityId;

    /*if( Entity::s_entityPopulation >= m_levelFile->m_populationCap &&
        m_levelFile->m_populationCap != -1 ) return WorldObjectId();*/

    for (int i = 0; i < _numEntities; i++)
    {
        int unitIndex;
        Entity *s = team->NewEntity( _type, _unitId, &unitIndex );
        AppDebugAssert( s );

        s->SetType( _type );        
        s->m_pos         = FindValidSpawnPosition( _pos, _spread );
        s->m_onGround    = false;
        s->m_vel         = _vel;
        if (s->m_vel.MagSquared() > 0.0)
		{
			s->m_front       = _vel;
			s->m_front.Normalise();
		}
		else
		{
			s->m_front.Set(1,0,0);

            if( _type == Entity::TypeDarwinian )
            {
                s->m_front.Set( SFRAND(1), 0, SFRAND(1) );
                s->m_front.Normalise();
            }
		}
        s->m_id.SetTeamId(_teamId);
        s->m_id.SetUnitId(_unitId);
        s->m_id.SetIndex(unitIndex);
        s->m_spawnPoint = _pos;
        s->m_roamRange = _range;
		s->m_routeId = _routeId;
        s->m_routeWayPointId = _routeWaypointId;
        if( _range == -1.0 ) s->m_roamRange = _spread;
        s->Begin();

        m_entityGrid->AddObject( s->m_id, s->m_pos.x, s->m_pos.z, s->m_radius );

        entityId = s->m_id;
    }

    if( _unitId != -1 )
    {
        Unit *unit = team->m_units[ _unitId ];
        unit->RecalculateOffsets();
    }

    return entityId;
}

void Location::InitMonsterTeam( int _teamId )
{
    if( m_teams[_teamId]->m_teamType == TeamTypeUnused )
    {
        m_teams[_teamId]->m_monsterTeam = true;
        g_app->m_multiwinia->InitialiseTeam( _teamId, TeamTypeCPU, -1 );
        m_teams[_teamId]->m_colour.Set( 10,10,10,255 );
        m_teams[_teamId]->m_lobbyTeam->m_colourId = MULTIWINIA_NUM_TEAMCOLOURS;
    }
}

void Location::InitFuturewinianTeam( int _teamId )
{
    if( m_teams[_teamId]->m_teamType == TeamTypeUnused )
    {
        m_teams[_teamId]->m_futurewinianTeam = true;
        g_app->m_multiwinia->InitialiseTeam( _teamId, TeamTypeCPU, -1 );
        m_teams[_teamId]->m_colour.Set( 200,200,200,255);
        m_teams[_teamId]->m_lobbyTeam->m_colourId = MULTIWINIA_NUM_TEAMCOLOURS+1;
    }
}

Vector3 Location::FindValidSpawnPosition( Vector3 const &_pos, double _spread )
{
    int tries = 10;

    while( tries > 0 )
    {
        Vector3 randomPos = _pos;

        double radius = syncfrand(_spread);
	    double theta = syncfrand(M_PI * 2);
    	randomPos.x += radius * iv_sin(theta);
	    randomPos.z += radius * iv_cos(theta);
        
        bool onMap = randomPos.x >= 0 && randomPos.z >= 0 &&
                     randomPos.x < m_landscape.GetWorldSizeX() &&
                     randomPos.z < m_landscape.GetWorldSizeZ();

        if( onMap )
        {
            double landHeight = m_landscape.m_heightMap->GetValue( randomPos.x, randomPos.z );

            if( landHeight > 0.0 )
            {
                return randomPos;
            }
        }
        
        tries--;
    }

    //
    // Failed to find a valid pos

    Vector3 pos = _pos;
    pos.x = max( pos.x, 20.0 );
    pos.z = max( pos.z, 20.0 );
    pos.x = min( pos.x, m_landscape.GetWorldSizeX()-20 );
    pos.z = min( pos.z, m_landscape.GetWorldSizeZ()-20 );

    return pos;
}


int Location::SpawnSpirit( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, WorldObjectId _id )
{
    if( _teamId == GetMonsterTeamId() )
    {
        return -1;
    }

    AppDebugAssert( _teamId < NUM_TEAMS );
    
    int index = m_spirits.GetNextFree();
    Spirit *s = m_spirits.GetPointer( index );
    s->m_pos = _pos + g_upVector;
    s->m_vel = _vel;
    s->m_teamId = _teamId;
    s->m_worldObjectId = _id;
    s->m_id.Set( _teamId, UNIT_SPIRITS, index, -1 );
    s->Begin();

    return index;
}


//  *** GetSpirit
int Location::GetSpirit( WorldObjectId _id )
{
    if( !_id.IsValid() )
    {
        return -1;
    }

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *spirit = m_spirits.GetPointer(i);
            if( spirit->m_worldObjectId == _id )
            {
                return i;
            }
        }
    }

    return -1;
}


WorldObject *Location::GetWorldObject( WorldObjectId _id )
{
    switch( _id.GetUnitId() )
    {
        case UNIT_BUILDINGS:            return GetBuilding( _id.GetUniqueId() );
        case UNIT_EFFECTS:              return GetEffect( _id );
        case UNIT_SPIRITS:              return GetSpirit( _id.GetIndex() );
        default:                        return GetEntity( _id );
    }    
}


Spirit *Location::GetSpirit( int _index )
{
    if( m_spirits.ValidIndex( _index ) )
    {
        return &m_spirits[_index];
    }

    return NULL;
}


WorldObject *Location::GetEffect( WorldObjectId _id )
{
    if( m_effects.ValidIndex(_id.GetIndex()) )
    {
        WorldObject *wobj = m_effects[_id.GetIndex()];
        if( wobj->m_id.GetUniqueId() == _id.GetUniqueId() )
        {
            return wobj;
        }
    }

    return NULL;
}


Entity *Location::GetEntity( WorldObjectId _id )
{         
    unsigned char const teamId = _id.GetTeamId();
    int const unitId = _id.GetUnitId();
    int const index = _id.GetIndex();
    int const uniqueId = _id.GetUniqueId();

    if( teamId >= NUM_TEAMS ||
        m_teams[teamId]->m_teamType == TeamTypeUnused ) 
    {
        return NULL;
    }

    if( m_teams[teamId]->m_units.ValidIndex(unitId) )
    {
        Unit *unit = m_teams[teamId]->m_units[unitId];
        if( unit->m_entities.ValidIndex( index ) )
        {
            Entity *entity = unit->m_entities[index];
            if( entity->m_id.GetUniqueId() == uniqueId )
            {
                return entity;
            }
        }
    }

    if( unitId == -1 )
    {
        if( m_teams[teamId]->m_others.ValidIndex(index) )
        {
            Entity *entity = m_teams[teamId]->m_others[index];
            if( entity->m_id.GetUniqueId() == uniqueId )
            {
                return entity;
            }
        }
    }

    return NULL;
}


Entity *Location::GetEntity(Vector3 const &_rayStart, Vector3 const &_rayDir)
{
	for (unsigned int i = 0; i < NUM_TEAMS; ++i)
	{
		Entity *result = m_teams[i]->RayHitEntity(_rayStart, _rayDir);
		if (result)
		{
			return result;
		}
	}

	return NULL;
}


Entity *Location::GetEntitySafe( WorldObjectId _id, unsigned char _type )
{
    WorldObject *wobj = GetEntity( _id );
    Entity *ent = (Entity *) wobj;
    
    if( ent && ent->m_type == _type )
    {
        return ent;
    }

    return NULL;
}


Unit *Location::GetUnit( WorldObjectId _id )
{
    unsigned char teamId = _id.GetTeamId();
    int unitId = _id.GetUnitId();

    if( teamId >= NUM_TEAMS ||
        m_teams[teamId]->m_teamType == TeamTypeUnused ) 
    {
        return NULL;
    }

    if( m_teams[teamId]->m_units.ValidIndex(unitId) )
    {
        Unit *unit = m_teams[teamId]->m_units[unitId];
        return unit;
    }

    return NULL;
}


Building *Location::GetBuilding( int _id )
{
    if( _id == -1 ) return NULL;
    
    if( g_app->m_editing )
    {
        return m_levelFile->GetBuilding( _id );
    }
    else
    {
        for( int i = 0; i < m_buildings.Size(); ++i )
        {
            if( m_buildings.ValidIndex(i) )
            {
                Building *building = m_buildings.GetData(i);
                if( building->m_id.GetUniqueId() == _id )
                {
                    return building;
                }
            }
        }
    }

    return NULL;
}


Building *Location::GetBuilding(Vector3 const &_rayStart, Vector3 const &_rayDir )
{
	for (unsigned int i = 0; i < m_buildings.Size(); ++i)
	{
		if (m_buildings.ValidIndex(i))
		{
			Building *b = m_buildings[i];
			if (b->DoesRayHit(_rayStart, _rayDir))
			{
				return b;
			}
		}
	}

	return false;
}

int Location::GetMonsterTeamId()
{
    if( !m_allowMonsterTeam ) return -1;
    return 4;
/*
    int numTeams = 0;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_teams[i]->m_monsterTeam )
        {                        
            return i;
        }
        
        if( m_teams[i]->m_teamType != TeamTypeUnused )
        {
            numTeams++;
        }
    }

    return numTeams;
    */
}

int Location::GetFuturewinianTeamId()
{
    if( !m_allowMonsterTeam ) return -1;
    return 5;
}


bool Location::IsVisible( Vector3 const &_from, Vector3 const &_to )
{    
    Vector3 rayDir = (_to - _from).Normalise();
    double tolerance = 20.0;
    Vector3 startPos = _from + rayDir * tolerance;
    
    Vector3 hitPos;
    bool landHit = g_app->m_location->m_landscape.RayHit( startPos, rayDir, &hitPos );

    if( !landHit ) return true;

    double distanceToTarget = ( _to - _from ).Mag();
    double distanceToHit = ( hitPos - _from ).Mag();
    if( distanceToHit > distanceToTarget + tolerance ) return true;

    return false;
}   


double Location::GetWalkingDistance ( Vector3 const &from, Vector3 const &to, bool checkObstructions )
{
    double totalDistance = 0.0;
    double waterLevel = 0.0;
    double stepSize = 40.0;   
    double straightLineDistance = ( from - to ).Mag();
    int numSteps = straightLineDistance / stepSize;
    double distanceUnderWater = 0.0;

    Vector3 oldPosition = from;
    oldPosition.y = m_landscape.m_heightMap->GetValue(oldPosition.x, oldPosition.z);

    for( int i = 1; i <= numSteps; ++i )
    {
        Vector3 position = from + ( to - from ) * ( i / (double)numSteps );
        position.y = m_landscape.m_heightMap->GetValue(position.x, position.z);

        if( position.y <= waterLevel )
        {
            distanceUnderWater += stepSize;
        }

        if( distanceUnderWater >= 40.0 )
        {
            return -1;
        }

        //
        // Check for collisions with blocking objects, eg walls

        if( m_obstructionGrid && checkObstructions )
        {
            LList<int> *buildingIds = m_obstructionGrid->GetBuildings( position.x, position.z );

            for( int j = 0; j < buildingIds->Size(); ++j )
            {
                int buildingId = buildingIds->GetData(j);
                Building *building = GetBuilding( buildingId );

                if( building )
                {
                    if( building->m_type == Building::TypeWall &&            
                        building->DoesSphereHit( position, stepSize/2.0 ) )
                    {
                        return -1;
                    }

                    if( building->m_type == Building::TypeLaserFence &&
                        ((LaserFence *)building)->IsEnabled() &&
                        building->DoesSphereHit( position, stepSize/2.0) )
                    {
                        return -1;
                    }
                }
            }
        }


        double thisDistance = ( position - oldPosition ).Mag();
        double horizDist = (Vector2(position) - Vector2(oldPosition)).Mag();
        double gradient = ( position.y - oldPosition.y ) / horizDist;        
        double appliedGradient = 1.0 + max( 0.0, gradient * 1.5 );
        
        totalDistance += thisDistance * appliedGradient;
        oldPosition = position;
    }

    return totalDistance;
}


bool Location::IsWalkable( Vector3 const &_from, Vector3 const &_to, bool _evaluateCliffs, bool permitSteepDownhill, int *_laserFenceId )
{
    double waterLevel = 0.0;

    if( _from.y <= waterLevel || _to.y <= waterLevel ) 
    {
        return false;
    }
  
    double stepSize = 20.0;   
    double totalDistance = ( _from - _to ).Mag();
    int numSteps = totalDistance / stepSize;
    Vector3 diff = ( _to - _from ) / (double)numSteps;
    double distanceUnderWater = 0.0;

    Vector3 position = _from;
    Vector3 oldPosition = _from;
    for( int i = 0; i < numSteps; ++i )
    {
        position.y = m_landscape.m_heightMap->GetValue(position.x, position.z);
        if( position.y <= waterLevel )
        {
            distanceUnderWater += stepSize;
        }

        if( distanceUnderWater >= 40.0 )
        {
            return false;
        }
        
        if( _evaluateCliffs )
        {
            double gradient = ( position.y - oldPosition.y ) / stepSize;        
            if( gradient > 2.4 )
            {
                return false;
            }

            if( !permitSteepDownhill || gradient > 0.0 )
            {
                // Only check the surface normal if the gradient is sloping upwards
                // ie going downhill doesnt matter how steep it is

                Vector3 surfaceNormal = m_landscape.m_normalMap->GetValue( position.x, position.z );
                if( surfaceNormal.y < 0.8 )
                {
                    return false;               
                }
            }
        }

        //
        // Check for collisions with blocking objects, eg walls

        if( m_obstructionGrid )
        {
            LList<int> *buildingIds = m_obstructionGrid->GetBuildings( position.x, position.z );
        
            for( int j = 0; j < buildingIds->Size(); ++j )
            {
                int buildingId = buildingIds->GetData(j);
                Building *building = GetBuilding( buildingId );

                if( building )
                {
                    if( building->m_type == Building::TypeWall &&            
                        building->DoesSphereHit( position, stepSize * 0.66 ) )
                    {
                        return false;
                    }

                    if( building->m_type == Building::TypeLaserFence &&
                        ((LaserFence *)building)->IsEnabled() &&
                        building->DoesSphereHit( position, stepSize/2.0) )
                    {
                        if( _laserFenceId )
                        {
                            *_laserFenceId = building->m_id.GetUniqueId();
                        }
                        //return false;
                    }
                }
            }
        }

        position += diff;
    }

    return true;
}

void Location::AdvanceWeapons( int _slice )
{
    START_PROFILE( "Advance Lasers");
    int startIndex, endIndex;
    m_lasers.GetNextSliceBounds(_slice, &startIndex, &endIndex);   
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_lasers.ValidIndex(i) )
        {
            Laser *l = m_lasers.GetPointer(i);
            bool remove = l->Advance();
            if( remove )
            {
                m_lasers.RemoveData(i);
            }
        }
    }
    END_PROFILE( "Advance Lasers");


    START_PROFILE( "Advance Effects");
    m_effects.GetNextSliceBounds(_slice, &startIndex, &endIndex);   
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_effects.ValidIndex(i) )
        {
            WorldObject *e = m_effects[i];            

#ifdef TRACK_SYNC_RAND
			if( e->m_type != WorldObject::EffectSnow )
				SyncRandLogText( e->LogState( "EFFECT BEFORE: " ) );
#endif

            bool remove = e->Advance();

#ifdef TRACK_SYNC_RAND
			if( e->m_type != WorldObject::EffectSnow )
	            SyncRandLogText( e->LogState( "EFFECT AFTER: " ) );

			bool WatchEntities();
			WatchEntities();				
#endif

			if( remove )
            {
                m_effects.RemoveData(i);
                delete e;
            }
        }
    }
    END_PROFILE( "Advance Effects");
}


// *** AdvanceBuildings
void Location::AdvanceBuildings( int _slice )
{        
    START_PROFILE( "Advance Buildings");
    bool obstructionGridChanged = false;

    int startIndex, endIndex;
    m_buildings.GetNextSliceBounds( _slice, &startIndex, &endIndex );
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_buildings.ValidIndex(i) )
        {
            Building *building = m_buildings.GetData(i);

            START_PROFILE( Building::GetTypeName( building->m_type ) );
            SyncRandLogText( building->LogState( "Building before : " ) );
            bool removeBuilding = building->Advance();
            SyncRandLogText( building->LogState( "Building after : " ) );
#ifdef TRACK_SYNC_RAND
			bool WatchEntities();
			WatchEntities();
#endif
            END_PROFILE(  Building::GetTypeName( building->m_type ) );
            
            if( removeBuilding )
            {
                m_buildings.RemoveData(i);
                obstructionGridChanged = true;
                bool regenGrid = false;
                if( building->m_type == Building::TypeAITarget ||
                    building->m_type == Building::TypeWall ) regenGrid = true; 
                // the destruction of walls will create new holes for ai links, so recalculate

                delete building;
                building = NULL;

                if( regenGrid ) RecalculateAITargets();
            }
        }
    }

    if( obstructionGridChanged )
    {
        // TODO: This is WAY too slow, should only recalculate the areas affected
        g_app->m_location->m_obstructionGrid->CalculateAll();
    }
    
    END_PROFILE( "Advance Buildings");    
}



// *** AdvanceTeams
void Location::AdvanceTeams( int _slice )
{    
    for (int i = 0; i < NUM_TEAMS; i++)
    {
		if (m_teams[i]->m_teamType != TeamTypeUnused)
		{
			m_teams[i]->Advance(_slice);
		}
    }
}


// *** AdvanceSpirits
void Location::AdvanceSpirits( int _slice )
{
    START_PROFILE( "Advance Spirits");

    int startIndex, endIndex;
    m_spirits.GetNextSliceBounds(_slice, &startIndex, &endIndex);
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *s = m_spirits.GetPointer(i);
            bool removeSpirit = s->Advance();
            if( removeSpirit )
            {
                m_spirits.RemoveData(i);
            }
        }
    }

    END_PROFILE( "Advance Spirits");
}


// *** AdvanceClouds
void Location::AdvanceClouds( int _slice )
{
    if( _slice == 3 )
    {
        START_PROFILE( "Advance Clouds");
        m_clouds->Advance();
        END_PROFILE( "Advance Clouds");
    }
}

// *** DoMissionCompleteActions
// Does whatever needs to be done when a mission is completed
void Location::DoMissionCompleteActions()
{
	// 
	// Update the mission file for this location
	
	GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(g_app->m_locationId);
    gloc->m_missionCompleted = true;
    
    //gloc->m_missionAvailable = false;
	//strcpy(gloc->m_missionFilename, "null");

    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageObjectivesComplete, -1, 5.0 );


//    if( !g_app->m_camera->IsInteractive() ||
//        g_app->m_script->IsRunningScript() )
//    {
//        return;
//    }

    //
	// Get Sepulveda to tell you that the level is complete

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_sepulveda->Say("PrimaryObjectivesComplete");
#endif
}


// *** MissionComplete
bool Location::MissionComplete()
{
	if (m_missionComplete) return true;

	LList <GlobalEventCondition *> *objectivesList = &m_levelFile->m_primaryObjectives;

	for (int i = 0; i < objectivesList->Size(); ++i)
	{
		GlobalEventCondition *gec = objectivesList->GetData(i);
		if (!gec->Evaluate())
		{
			return false;
		}
	}

	return true;
}


// *** Advance
void Location::Advance( int _slice )
{
    m_lastSliceProcessed = _slice;


	#undef for
	#pragma omp parallel for schedule(dynamic)
	for(int step=0;step<=4;step++)
	{
		switch(step)
		{
			case 0: AdvanceTeams        ( _slice ); break;
			case 1: AdvanceWeapons      ( _slice ); break;
			case 2: AdvanceBuildings    ( _slice ); break;
			case 3: AdvanceSpirits      ( _slice ); break;
			case 4: AdvanceClouds       ( _slice ); break;
		}
	}

    if (!m_missionComplete && MissionComplete())
	{
		m_missionComplete = true;
		DoMissionCompleteActions();
	}

    if( _slice == 3 )
    {
       /* for( int i = 0; i < m_levelFile->m_effects.Size(); ++i )
        {
            if( stricmp( m_levelFile->m_effects[i], "snow" ) == 0 )
            {
                AdvanceChristmas();
            }

            if( stricmp( m_levelFile->m_effects[i], "soulrain" ) == 0 )
            {
                AdvanceSoulRain();
            }

            if( stricmp( m_levelFile->m_effects[i], "dustwind" ) == 0 )
            {
                AdvanceDustWind();
            }

            if( stricmp( m_levelFile->m_effects[i], "lightning" ) == 0 )
            {
                AdvanceLightning();
            }
        }*/

        if( m_levelFile->m_effects[LevelFile::LevelEffectSnow] ) AdvanceChristmas();
        if( m_levelFile->m_effects[LevelFile::LevelEffectSoulRain] ) AdvanceSoulRain();
        if( m_levelFile->m_effects[LevelFile::LevelEffectDustStorm] ) AdvanceDustWind();
        if( m_levelFile->m_effects[LevelFile::LevelEffectLightning] ) AdvanceLightning();
        

        if( ChristmasModEnabled() == 1 )
        {
            AdvanceChristmas();
        }
    }


    if( _slice == 7 ) m_entityGridCache->Advance();

	if( g_app->Multiplayer() && _slice == 8 )
    {
        AdvanceCrates();
        if( m_spawnManiaTimer > 0.0 ) m_spawnManiaTimer -= SERVER_ADVANCE_PERIOD;
        if( m_bitzkreigTimer > 0.0 ) m_bitzkreigTimer -= SERVER_ADVANCE_PERIOD;
    }       
}

void Location::AdvanceSoulRain()
{
    /*for( int i = 0; i < m_doubleingSpirits.Size(); ++i )
    {
        UnprocessedSpirit *spirit = m_doubleingSpirits[i];
        bool finished = spirit->Advance();
        if( finished )
        {
            m_doubleingSpirits.RemoveData(i);
            delete spirit;
            --i;
        }
    }*/

    
    //
    // Spawn more unprocessed spirits

    m_spawnSync -= SERVER_ADVANCE_PERIOD;
    if( m_spawnSync <= 0.0 )
    {
        m_spawnSync = 1.0;

        double sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
        double sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
        double posY = 700.0 + syncfrand(300.0);
        Vector3 spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;
        UnprocessedSpirit *spirit = new UnprocessedSpirit();
        spirit->m_pos = spawnPos;
        m_effects.PutData( spirit );
    }
}

void Location::AdvanceChristmas()
{    
    if( m_christmasTimer < -90.0 )
    {
        for( int i = 0; i < 150; ++i )
        {
            double sizeX = m_landscape.GetWorldSizeX();
            double sizeZ = m_landscape.GetWorldSizeZ();
            double posY = syncfrand(1000.0);
            Vector3 spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;
            Snow *snow = new Snow();
            snow->m_pos = spawnPos;
            int index = m_effects.PutData( snow );
            snow->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
            snow->m_id.GenerateUniqueId();
        }
    }
    
    m_christmasTimer -= SERVER_ADVANCE_PERIOD;
    if( m_christmasTimer <= 0.0 )
    {
        m_christmasTimer = 0.1;
        double sizeX = m_landscape.GetWorldSizeX();
        double sizeZ = m_landscape.GetWorldSizeZ();
        double posY = 700.0 + syncfrand(300.0);
        Vector3 spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;
        Snow *snow = new Snow();
        snow->m_pos = spawnPos;
        int index = m_effects.PutData( snow );
        snow->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
        snow->m_id.GenerateUniqueId();
    }
}

void Location::AdvanceDustWind()
{
    double worldSizeX = m_landscape.GetWorldSizeX();
    double worldSizeZ = m_landscape.GetWorldSizeZ();
    DustBall::s_vortexTimer -= SERVER_ADVANCE_PERIOD;
    if( DustBall::s_vortexTimer <= 0.0 )
    {
        DustBall::s_vortexTimer = 15.0 + syncfrand(15.0);
        if( syncrand() % 10 == 0 && DustBall::s_vortexPos == g_zeroVector )
        {
            Vector3 spawnPos = Vector3( FRAND(worldSizeX), FRAND(500.0), FRAND(worldSizeZ) ) ;
            while( g_app->m_location->m_landscape.m_heightMap->GetValue( spawnPos.x, spawnPos.z ) <= 1.0 )
            {
                spawnPos = Vector3( FRAND(worldSizeX), FRAND(500.0), FRAND(worldSizeZ) ) ;
            }
            DustBall::s_vortexPos = spawnPos;
        }
        else
        {
            DustBall::s_vortexPos = g_zeroVector;

            Vector3 to, from;
            to.Set( FRAND( worldSizeX ), 0.0, FRAND(worldSizeZ) );
            from.Set( FRAND( worldSizeX ), 0.0, FRAND(worldSizeZ) );
            m_windDirection = ( to - from );
            m_windDirection.Normalise();            
        }
    }

    if( m_christmasTimer < -90.0 )
    {
        for( int i = 0; i < 300; ++i )
        {
            double posY = syncfrand(1000.0);
            Vector3 spawnPos = Vector3( FRAND(worldSizeX), posY, FRAND(worldSizeZ) ) ;
            DustBall *dustball = new DustBall();
            dustball->m_pos = spawnPos;
            int index = m_effects.PutData( dustball );
            dustball->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
            dustball->m_id.GenerateUniqueId();
            dustball->m_size += syncfrand( 20.0 );
        }
    }
    
    m_christmasTimer -= SERVER_ADVANCE_PERIOD;
    if( m_christmasTimer <= 0.0 )
    {
        for( int i = 0; i < 2; ++i )
        {
            m_christmasTimer = 0.1;
            double sizeX = m_landscape.GetWorldSizeX();
            double sizeZ = m_landscape.GetWorldSizeZ();
            double posY = syncfrand(1000.0);
            Vector3 spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;

            //spawnPos -= m_windDirection * spawnPos.Mag();

            DustBall *dustball = new DustBall();
            dustball->m_pos = spawnPos;
            int index = m_effects.PutData( dustball );
            dustball->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
            dustball->m_id.GenerateUniqueId();
            dustball->m_size += syncfrand( 20.0 );
        }
    }

    if( DustBall::s_vortexPos != g_zeroVector )
    {
        Vector3 pos = DustBall::s_vortexPos;
        pos.y = m_landscape.m_heightMap->GetValue( pos.x, pos.z );

        int numFound;
        m_entityGrid->GetNeighbours( s_neighbours, pos.x, pos.z, 50.0f, &numFound );

        for( int i = 0; i < numFound; ++i )
        {
            Darwinian *d = (Darwinian *)GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
            if( d )
            {
                if( d->m_onGround )
                {
                    d->m_state = Darwinian::StateInVortex;
                    d->m_onGround = false;
                }
            }
        }
    }
}


void Location::AdvanceLightning()
{    
    m_christmasTimer -= SERVER_ADVANCE_PERIOD;
    if( m_christmasTimer <= 0.0 )
    { 
        m_christmasTimer = syncfrand( 15.0 );
        double sizeX = m_landscape.GetWorldSizeX();
        double sizeZ = m_landscape.GetWorldSizeZ();
        double posY = 1200.0;

        Vector3 spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;
        while( g_app->m_location->m_landscape.m_heightMap->GetValue( spawnPos.x, spawnPos.z ) <= 1.0 )
        {
            spawnPos = Vector3( FRAND(sizeX), posY, FRAND(sizeZ) ) ;
        }

        Lightning *lightning = new Lightning();
        lightning->m_pos = spawnPos;
        int index = m_effects.PutData( lightning );
        lightning->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
        lightning->m_id.GenerateUniqueId();
    }
}

char *HashDouble( double value, char *buffer );

void Location::CreateRandomCrate()
{
#ifdef INCLUDE_CRATES_BASIC
    Crate crateTemplate;

    int dropMode = g_app->m_multiwinia->GetGameOption( "CrateDropMode" );
    bool random = ( dropMode == 0 || dropMode == -1);
    if( dropMode == 1 )
    {
        bool success = false;
        double totalScore = 0;
        double highScore = 0;
        double lowScore = FLT_MAX;
        int teamScores[NUM_TEAMS];
        memset( teamScores, -1, sizeof(teamScores));
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( i == GetMonsterTeamId() || 
                i == GetFuturewinianTeamId() || 
                g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused) continue;

            int score = g_app->m_multiwinia->m_teams[i].m_score;
            totalScore += score;
            if( score > highScore ) highScore = score;
            if( score < lowScore ) lowScore = score;
            teamScores[i] = score;
        }

        double weightedDropChance = highScore / totalScore;
        double lowDropChance = 1.0 - (lowScore / totalScore );
        if( lowDropChance > weightedDropChance ) weightedDropChance = lowDropChance;
        if( syncfrand(1.0) > weightedDropChance )
        {
            random = true;
        }
        else
        {
            AppDebugOut("Begining Weighted Crate Drop\n");
            int dropTeam = -1;
            double totalDropChance = 0.0;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( teamScores[i] == -1 ) continue;

                double dropRatio = totalScore / (double)teamScores[i];
                if( teamScores[i] == 0 ) dropRatio = totalScore;
                totalDropChance += dropRatio;
            }

            double dropZone = syncfrand(1.0);
            double chanceTotal = 0.0;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( teamScores[i] == -1 ) continue;

                double dropRatio = totalScore / (double)teamScores[i];
                if( teamScores[i] == 0 ) dropRatio = totalScore;
                double finalDropOdds = dropRatio / totalDropChance;

                chanceTotal += finalDropOdds;
                if( dropZone <= chanceTotal )
                {
                    dropTeam = i;
                    break;
                }
            }

            if( dropTeam != -1 )
            {
                AppDebugOut("Weighted Crate Drop team determined - %d\n", dropTeam );
                LList<int> buildingIndexList;
                for( int i = 0; i < m_buildings.Size(); ++i )
                {
                    if( m_buildings.ValidIndex(i) )
                    {
                        Building *b = m_buildings[i];
                        if( b->m_id.GetTeamId() == dropTeam )
                        {
                            buildingIndexList.PutData(i);
                        }
                    }
                }

                if( buildingIndexList.Size() > 0 )
                {
                    AppDebugOut("Weighted Crate Drop - Building list generated\n");
                    int sanity = 0;
                    while( sanity++ < 1000 )
                    {
                        int index = buildingIndexList[ syncrand() % buildingIndexList.Size() ];
                        Building *b = m_buildings[index];
                       // if( !RestrictionZone::IsRestricted( b->m_pos ) )
                        {
                            Vector3 pos = b->m_pos;
                            pos.x += syncsfrand(200.0);
                            pos.z += syncsfrand(200.0);
                            if( m_landscape.m_heightMap->GetValue( pos.x, pos.z > 20.0 ) )
                            {
                                AppDebugOut("Weighted Crate Drop - Valid Position found\n");
                                crateTemplate.m_pos.Set( pos.x, CRATE_START_HEIGHT, pos.z );
                                crateTemplate.m_pos.y += syncfrand(200);
                                success = true;
                                break;
                            }
                        }
                    }
                }
            }
        }
        
        if( !success ) 
        {
            AppDebugOut("Weighted Crate Drop - no valid pos found, falling back to random\n");
            random = true;
        }
    }

    bool nocrate = false;
    if( random )
    {
        int sanity = 0;
        while( g_app->m_location->m_landscape.m_heightMap->GetValue( crateTemplate.m_pos.x, crateTemplate.m_pos.z ) < 20.0 ||
               RestrictionZone::IsRestricted( crateTemplate.m_pos ) )
        {
            if( sanity > 100 ) 
            {
                nocrate = true;
                break;
            }
            sanity++;
            double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
            double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
            crateTemplate.m_pos.Set( FRAND(worldSizeX), CRATE_START_HEIGHT, FRAND(worldSizeZ) );
            crateTemplate.m_pos.y += syncfrand(200);
            
            if( Crate::s_crates > 0 )
            {
                for( int i = 0; i < m_buildings.Size(); ++i )
                {
                    if( m_buildings.ValidIndex( i ) )
                    {
                        Building *b = m_buildings[i];
                        if( b->m_type == Building::TypeCrate )
                        {
                            Vector3 dist = (b->m_pos - crateTemplate.m_pos);
                            dist.y = 0.0;

                            if( dist.Mag() < CRATE_SCAN_RANGE * 10.0 )
                            {
                                crateTemplate.m_pos.Set(0,0,0); // too close to another crate, find another spot
                            }
                        }
                    }
                }
            }
        }
    }

    if( !nocrate )
    {
        crateTemplate.m_id.Set( 255, UNIT_BUILDINGS, -1, g_app->m_globalWorld->GenerateBuildingId() );

        Building *newBuilding = Building::CreateBuilding( Building::TypeCrate );
        int id = m_buildings.PutData(newBuilding); 
        newBuilding->Initialise( (Building*)&crateTemplate);
        newBuilding->SetDetail( 1 );

        g_app->m_markerSystem->RegisterMarker_Crate( newBuilding->m_id );

        if( m_showCrateDropMessage )
        {
            SetCurrentMessage( LANGUAGEPHRASE("crate_drop") );
            m_showCrateDropMessage = false;
        }
    }
#endif
}


void Location::AdvanceCrates()
{
#ifdef INCLUDE_CRATES_BASIC

    if( g_app->m_multiwiniaTutorial ) 
    {
        if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ||
            !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
        {
            // No crates in the tutorial game please
            return;
        }
    }

    int maxCrates = int( m_landscape.GetWorldSizeX() / 500.0 );
    if( Crate::s_crates >= maxCrates ) return;

    double crateTime = g_app->m_multiwinia->GetGameOption( "CrateDropRate" );
    if( crateTime == 0.0 || crateTime == -1 ) return;

    if( m_crateTimer == 0.0 )
    {
        m_crateTimer = crateTime * ( 1.5 - syncfrand(1.0) );
    }

    m_crateTimer -= SERVER_ADVANCE_PERIOD;
    if( m_crateTimer <= 0.0 )
    {
        m_crateTimer = crateTime * ( 1.5 - syncfrand(1.0) );
        // generate a new crate

        CreateRandomCrate();
    }    

#endif
}

// *** Render Landscape
void Location::RenderLandscape()
{
	m_landscape.Render();
}


// *** Render Particles
void Location::RenderParticles()
{    
}


// *** Render Teams
void Location::RenderTeams()
{
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        m_teams[i]->Render();
    }
}


// *** Render Spirits
void Location::RenderSpirits()
{
    START_PROFILE( "Render Spirits");

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_BLEND );
    glDepthMask     ( false );

    double timeSinceAdvance = g_predictionTime;

	int monsterTeamId = g_app->m_location->GetMonsterTeamId();
	int numSpirits = m_spirits.Size();

	// Render monster team first

    glBlendFunc		( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glBegin         ( GL_QUADS );

	for( int i = 0; i < numSpirits; ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *r = m_spirits.GetPointer(i);

			if( r->m_teamId == monsterTeamId ) 
			{	                        
				if( i > m_spirits.GetLastUpdated() )
				{
					r->RenderWithoutGlBegin( timeSinceAdvance + g_gameTimer.GetServerAdvancePeriod() );
				}
				else
				{
					r->RenderWithoutGlBegin( timeSinceAdvance );
				}
			}
        }
    }   
    glEnd           ();

	// Render others

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBegin         ( GL_QUADS );

    for( int i = 0; i < numSpirits; ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *r = m_spirits.GetPointer(i);
                   
			if( r->m_teamId != monsterTeamId ) 
			{
				if( i > m_spirits.GetLastUpdated() )
				{
					r->RenderWithoutGlBegin( timeSinceAdvance + g_gameTimer.GetServerAdvancePeriod() );
				}
				else
				{
					r->RenderWithoutGlBegin( timeSinceAdvance );
				}
			}
        }
    }    

    glEnd           ();
    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_CULL_FACE );

    END_PROFILE( "Render Spirits");
}


// *** RenderWater
void Location::RenderWater()
{
    if( m_water ) m_water->Render();
}



// *** Render
void Location::Render(bool renderWaterAndClouds)
{   
#ifdef TESTBED_ENABLED
	if( g_app->GetTestBedMode() == TESTBED_ON )
	{
		if( !g_app->GetTestBedRendering() )
			return;
	}
#endif

    //
    // Render all solid objects

	if( renderWaterAndClouds ) RenderClouds();
	CHECK_OPENGL_STATE();

	RenderLandscape();   

	CHECK_OPENGL_STATE();
    if( renderWaterAndClouds ) RenderWater();
#ifdef USE_DIRECT3D
		else glDisable(GL_CLIP_PLANE2);
#endif
	CHECK_OPENGL_STATE();


	
	RenderBuildings();	
	CHECK_OPENGL_STATE();

	// good up to here
	if (!g_app->m_editing && m_teams)
	{
		RenderTeams();	// must be u
		CHECK_OPENGL_STATE();
	}
	
	// good after here
	
    //if( m_entityGrid ) m_entityGrid->Render();
    //if( m_entityGridCache ) m_entityGridCache->Render();
	//if( m_obstructionGrid ) m_obstructionGrid->Render();

	//
	// Render all alpha'd objects

	RenderBuildingAlphas();
	CHECK_OPENGL_STATE();

	if(!g_app->m_editing)
	{
		CHECK_OPENGL_STATE();
		RenderParticles();
		CHECK_OPENGL_STATE();
		RenderWeapons();
		CHECK_OPENGL_STATE();
		RenderSpirits();
		CHECK_OPENGL_STATE();
	}
         
    START_PROFILE("Render Routing System");
    m_routingSystem.Render();
	//m_seaRoutingSystem.Render(); // only ai use this so it shouldn't be rendered under normal circumstances
    END_PROFILE("Render Routing System");

    START_PROFILE("RenderCurrentMessage");
	RenderCurrentMessage();
    //RenderChatMessages();
    END_PROFILE("RenderCurrentMessage");
	CHECK_OPENGL_STATE();
}

// *** Render Buildings
void Location::RenderBuildings()
{
    START_PROFILE( "Render Buildings");
    double timeSinceAdvance = g_predictionTime;

    SetupFog        ();
    glEnable        (GL_FOG);
	g_app->m_renderer->SetObjectLighting();
#ifdef USE_DIRECT3D
	g_fixedPipeline->m_state.m_specular = true;
#endif


    //
    // Special lighting mode used for Demo2
    
    if( g_prefsManager->GetInt( "RenderSpecialLighting" ) == 1 ||
        ( g_app->Multiplayer() && g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot ) ||
        m_levelFile->m_effects[LevelFile::LevelEffectSpecialLighting] ||
		g_app->m_gameMode == App::GameModePrologue )
    {
        SetupSpecialLights();
    }

#ifdef CHEATMENU_ENABLED
    if( g_inputManager->controlEvent( ControlRTLoaderPixelWaveIncrease ) ) g_prefsManager->SetInt( "RenderSpecialLighting", 1 );
    if( g_inputManager->controlEvent( ControlRTLoaderPixelWaveDecrease ) ) g_prefsManager->SetInt( "RenderSpecialLighting", 0 );
#endif

    for( int i = 0; i < m_buildings.Size(); ++i )
    {
	    if( m_buildings.ValidIndex(i) )
        {
            Building *building = m_buildings.GetData(i);
            if( building->IsInView() )
            {
                START_PROFILE( Building::GetTypeName( building->m_type ) );
                if( i > m_buildings.GetLastUpdated() )
                {         
                    building->Render( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
                }
                else
                {
                    building->Render( timeSinceAdvance );
                }
                END_PROFILE(  Building::GetTypeName( building->m_type ) );
            }
        }
    }

#ifdef USE_DIRECT3D
	g_fixedPipeline->m_state.m_specular = false;
#endif
    glDisable       (GL_FOG);
	g_app->m_renderer->SetObjectLighting();
	g_app->m_renderer->UnsetObjectLighting();

    END_PROFILE( "Render Buildings");    

	CHECK_OPENGL_STATE();
}


int DepthSortedBuildingCompare(const void * elem1, const void * elem2 )
{
    Location::DepthSortedBuilding *b1 = (Location::DepthSortedBuilding *) elem1;
    Location::DepthSortedBuilding *b2 = (Location::DepthSortedBuilding *) elem2;

    if      ( b1->m_distance < b2->m_distance )     return -1;
    else if ( b1->m_distance > b2->m_distance )     return +1;
    else                                            return 0;
}


// *** Render Building Alphas
void Location::RenderBuildingAlphas()
{
    START_PROFILE( "Render Building Alphas");
    double timeSinceAdvance = g_predictionTime;

    //
    // Prepare data for depth sorting of our buildings

    #define StepDepthSortedBuildings 128
    int nextSortedBuilding = 0;


    //
    // Render all non-depth sorted Alphas
    // Also record all depth sorted Alphas

    SetupFog        ();
    glEnable        (GL_FOG);
    
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
	    if( m_buildings.ValidIndex(i) )
        {
            Building *building = m_buildings.GetData(i);
            if( building->IsInView() )
            {
                Vector3 centrePos;
                if( building->PerformDepthSort( centrePos ) )
                {
                    if( nextSortedBuilding >= m_sizeSortedBuildingsAlphas )
                    {
                        int oldSize = m_sizeSortedBuildingsAlphas;
                        m_sizeSortedBuildingsAlphas += StepDepthSortedBuildings;

                        DepthSortedBuilding *oldSortedBuildings = m_sortedBuildingsAlphas;
                        m_sortedBuildingsAlphas = new DepthSortedBuilding[m_sizeSortedBuildingsAlphas];
                        if( oldSortedBuildings )
                        {
                            memcpy( m_sortedBuildingsAlphas, oldSortedBuildings, sizeof(DepthSortedBuilding) * oldSize );
                            delete [] oldSortedBuildings;
                        }
                    }

                    double distance = ( centrePos - g_app->m_camera->GetPos() ).MagSquared();
                    m_sortedBuildingsAlphas[nextSortedBuilding].m_buildingIndex = i;
                    m_sortedBuildingsAlphas[nextSortedBuilding].m_distance = distance;
                    nextSortedBuilding++;
                }
                else
                {
                    START_PROFILE( Building::GetTypeName( building->m_type ) );
                    
                    if( i > m_buildings.GetLastUpdated() )     
                    {
                        building->RenderAlphas( timeSinceAdvance + SERVER_ADVANCE_PERIOD );                
                    }
                    else                                
                    {
                        building->RenderAlphas( timeSinceAdvance );
                    }
                    
                    END_PROFILE(  Building::GetTypeName( building->m_type ) );
                }                
            }
        }
    }


    //
    // Sort the buildings that require sorting

    START_PROFILE( "Depth Sort" );
    qsort( m_sortedBuildingsAlphas, nextSortedBuilding, sizeof(DepthSortedBuilding), DepthSortedBuildingCompare );
    END_PROFILE( "Depth Sort" );


    //
    // Render those sorted buildings in reverse depth order

    for( int i = nextSortedBuilding-1; i >= 0; i-- )
    {
        int buildingIndex = m_sortedBuildingsAlphas[i].m_buildingIndex;
        Building *building = m_buildings.GetData(buildingIndex);

        START_PROFILE( Building::GetTypeName( building->m_type ) );

        if( buildingIndex > m_buildings.GetLastUpdated() )
        {
            building->RenderAlphas( timeSinceAdvance + SERVER_ADVANCE_PERIOD );                
        }
        else                                
        {
            building->RenderAlphas( timeSinceAdvance );
        }

        END_PROFILE(  Building::GetTypeName( building->m_type ) );
    }


    glDisable       (GL_FOG);

    END_PROFILE( "Render Building Alphas");    
}


// *** Render Clouds
void Location::RenderClouds()
{
    START_PROFILE( "Render Clouds");

    if( m_clouds )
    {
        double timeSinceAdvance = g_predictionTime;
        if( m_lastSliceProcessed >= 3 )
        {
            m_clouds->Render( timeSinceAdvance );
        }
        else
        {
            m_clouds->Render( timeSinceAdvance + g_gameTimer.GetServerAdvancePeriod() );
        }
    }

    END_PROFILE( "Render Clouds");
}


// *** Render Effects
void Location::RenderWeapons()
{
    START_PROFILE(  "Render Weapons" );

    double timeSinceAdvance = g_predictionTime;
    

    //
    // Render effects
	
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	glEnable        ( GL_BLEND );
	glDisable       ( GL_CULL_FACE );
	glDepthMask     ( false );

    for( int i = 0; i < m_effects.Size(); ++i )
	{
		if( m_effects.ValidIndex(i) )
		{
			WorldObject *w = m_effects[i];
            if( g_app->m_camera->PosInViewFrustum( w->m_pos ) || 
                w->m_type == WorldObject::EffectPulseWave ||
                w->m_type == WorldObject::EffectSpamInfection ||
                w->m_type == WorldObject::EffectLightning )
            {                    
			    if( i > m_effects.GetLastUpdated() )
			    {
				    w->Render( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
			    }
			    else
			    {
				    w->Render( timeSinceAdvance );
			    }
            }
		}
	}
	glEnable        ( GL_CULL_FACE );



    //
    // Render lasers

    glEnable        (GL_LINE_SMOOTH );
	glDisable		(GL_CULL_FACE);
	glEnable		(GL_TEXTURE_2D);
	glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp", false));

	double nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.2,
									 	   g_app->m_renderer->GetFarPlane());
	
	glBegin         (GL_QUADS);
	for( int i = 0; i < m_lasers.Size(); ++i )
	{
		if( m_lasers.ValidIndex(i) )
		{
			Laser *l = m_lasers.GetPointer(i);
                    
			if( i > m_lasers.GetLastUpdated() )
			{
				l->Render( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
			}
			else
			{
				l->Render( timeSinceAdvance );
			}
		}
	}

    glEnd           ();
	glDisable		(GL_TEXTURE_2D);
	glEnable		(GL_CULL_FACE);
	glDepthMask     ( true );
	glDisable       ( GL_BLEND );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable       ( GL_LINE_SMOOTH );

	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart, 
										   g_app->m_renderer->GetFarPlane());

    END_PROFILE(  "Render Weapons" );
}

void Location::RenderCurrentMessage()
{
    if( g_app->m_hideInterface ) return;
	if( m_message.Length() > 0 )
    {
        double timeRemaining = m_messageTimer - GetHighResTime();
        
        if( timeRemaining <= 0.0 )
        {
            //delete m_message;
            m_message = UnicodeString();
            m_messageNoOverwrite = false;
            return;
        }

        double alpha = timeRemaining * 0.5;
        alpha = min( alpha, 1.0 );
        alpha = max( alpha, 0.0 );        
        double size = 40.0;

		double textWidth = g_gameFont.GetTextWidth( m_message.Length(), size );
        int screenW = g_app->m_renderer->ScreenW();

        if( textWidth > screenW )
        {
            double factor = screenW / textWidth;
            size *= factor * 0.95;
        }

        if( timeRemaining < 2.0 ) 
        {
            //size = iv_sqrt( 2.7 - timeRemaining ) * 30.0;        
            size += iv_sqrt( 2.0 - timeRemaining ) * 15.0;
        }

        double outlineAlpha = alpha * alpha * alpha;

        double scale = (double)g_app->m_renderer->ScreenW() / 1280.0;

        g_gameFont.BeginText2D();

        g_gameFont.SetRenderOutline(true);
        glColor4f( outlineAlpha, outlineAlpha, outlineAlpha, 0.0 );
        g_gameFont.DrawText2DCentre( screenW/2.0, 370.0 * scale, size, m_message );
        
        g_gameFont.SetRenderOutline(false);
        if( m_messageTeam != 255 )
        {
            if( m_teams[m_messageTeam]->m_monsterTeam )
            {
                glColor4f( 0.6, 0.6, 0.6, 1.0 );
            }
            else
            {
                RGBAColour col = m_teams[m_messageTeam]->m_colour;
                col.a = alpha * 255;
                glColor4ubv( col.GetData() );
            }
        }
        else
        {
            glColor4f( 1.0, 1.0, 1.0, alpha );
        }
        g_gameFont.DrawText2DCentre( screenW/2.0, 370.0 * scale, size, m_message );

        g_gameFont.EndText2D();
    }
}

void Location::RenderChatMessages()
{
    if( g_app->m_hideInterface ) return;
    if( g_app->m_editing ) return;

    static float boxHeight = 0.0f;

    float fontSize = g_app->m_renderer->ScreenH() * 0.015f;

    float scale = (float)g_app->m_renderer->ScreenW() / 1280.0f;
    if( g_app->m_renderer->ScreenW() <= 800 )
    {
        scale = (float)g_app->m_renderer->ScreenW() / 1024.0f;
        if( g_app->m_renderer->ScreenW() <= 640 )
        {
            scale = 0.75;
        }
    }

    fontSize = 14 * scale;

    float x = 10 * scale;
    float y = g_app->m_renderer->ScreenH() - (10 * scale);

    g_editorFont.BeginText2D();

    if( !ChatInputWindow::IsChatVisible() )
    {
        bool renderMessage = false;
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( m_teams[i]->m_teamType == TeamTypeRemotePlayer )
            {
                renderMessage = true;
            }
        }

        if( renderMessage )
        {
            UnicodeString pressEnter = LANGUAGEPHRASE("multiwinia_presstotalk");
            glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
            g_editorFont.SetRenderOutline( true );
            g_editorFont.DrawText2D( x + fontSize, y - fontSize, fontSize, pressEnter );
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            g_editorFont.SetRenderOutline( false );
            g_editorFont.DrawText2D( x + fontSize, y - fontSize, fontSize, pressEnter );
        }
    }

    if( m_chatMessages.Size() == 0 ) 
    {
        g_editorFont.EndText2D();
        return;
    }

    y -=  (fontSize * 2.0f);

    int w = g_app->m_renderer->ScreenW() * 0.25f;

    x+=fontSize;

    boxHeight = fontSize;
    bool stopRendering = false;
    for( int i = 0; i < m_chatMessages.Size(); ++i )
    {
        float alphaTime = GetHighResTime() - m_chatMessages[i]->m_recievedTime;
        bool maxed = false;
        if( !stopRendering )
        {
            float alpha = 255.0f;
            if( alphaTime > 55.0f )
            {
                float mod = ((60.0f - alphaTime) / 5.0f);
                mod = min( 1.0f, mod );
                mod = max( 0.0f, mod );
                alpha *= mod;
            }

            int teamId = -1;
            for( int j = 0; j < NUM_TEAMS; ++j )
            {
                if( m_teams[j]->m_clientId == m_chatMessages[i]->m_clientId )
                {
                    teamId = j;
                    break;
                }
            }
            
            UnicodeString msg;
            if( teamId == -1 )
            {
                // the team that sent this message has left, so use the cached values from teh sender
                msg = m_chatMessages[i]->m_senderName + UnicodeString(": ") + m_chatMessages[i]->m_msg;
            }
            else
            {
                msg = m_teams[ teamId ]->GetTeamName() + UnicodeString(": ") + m_chatMessages[i]->m_msg;
            }

            LList<UnicodeString *> *wrapped = WordWrapText( msg, g_app->m_renderer->ScreenW() * 0.4f, fontSize, true, true );
            y -= (fontSize * 1.1f) * wrapped->Size();

            if(  y < g_app->m_renderer->ScreenH() / 2.0f )
            {
                maxed = true;
            }

            float a = alpha / 255.0f;
            for( int j = 0; j < wrapped->Size(); ++j )
            {
                glColor4f( 0.0f, 0.0f, 0.0f, 0.5f * (alpha/255.0f) );
                glEnable    ( GL_BLEND );
                glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                float scale = 1.1f;
                float mod = 0.0f;
                float bottomMod = 0.0f;
                if( i == 0 && j == wrapped->Size() -1 ) mod = fontSize * 0.25f;
                if( (maxed || i == m_chatMessages.Size() - 1) && j == 0 ) bottomMod = fontSize * 0.25f;
                glBegin( GL_QUADS );
                    glVertex2i( x-fontSize, y - bottomMod - (scale * fontSize) / 2.0f );
                    glVertex2i( x-fontSize + w, y - bottomMod - (scale * fontSize) / 2.0f );
                    glVertex2i( x-fontSize + w, y + mod + (scale * fontSize) / 2.0f );
                    glVertex2i( x-fontSize, y + mod + (scale * fontSize) / 2.0f);
                glEnd();

                if( a == 1.0f )
                {
                    g_editorFont.SetRenderOutline(true);
                    glColor4f(1.0f, 1.0f, 1.0f, 0.0f);
                    g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                }
                RGBAColour col;
                if( teamId == -1 )
                {
                    col = m_chatMessages[i]->m_colour;
                }
                else
                {
                    col = m_teams[ teamId ]->m_colour;
                }
                col.a = alpha;
                glColor4ubv( col.GetData() );
                g_editorFont.SetRenderOutline(false);
//                g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                g_editorFont.DrawText2D( x, y, fontSize, *wrapped->GetData(j) );
                y += fontSize * 1.1f;
            }
            y -= (fontSize * 1.1f) * wrapped->Size();
            boxHeight += (fontSize * 1.2f) * wrapped->Size();
            wrapped->EmptyAndDelete();
            delete wrapped;
        }

        if( alphaTime > 60.0f )
        {
            ChatMessage *m = m_chatMessages[i];
            m_chatMessages.RemoveData(i);
            delete m;
            --i;
        }

        if( maxed )
        {
            // stop if we get too far up teh screen
            // we could break here, but then none of the older messages would ever get deleted
            stopRendering = true;
        }
    }
    g_editorFont.EndText2D();
}

int Location::GetTeamPosition( unsigned char _teamId )
{
    if( !g_app->Multiplayer() ) return _teamId;
    
    
    if( g_app->m_multiwiniaTutorial )
    {
        // Enforce standard positions for tutorial
        m_teamPosition[_teamId] = _teamId;
        return _teamId;
    }


    if( _teamId == GetMonsterTeamId() ||
        _teamId == GetFuturewinianTeamId() ) m_teamPosition[_teamId] = _teamId;

    if( m_teamPosition[_teamId] == -1 )
    {
        LList<int>  unused;
        bool used[NUM_TEAMS];
        memset( used, 0, sizeof(bool) * NUM_TEAMS);
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            for( int j = 0; j < g_app->m_location->m_levelFile->m_numPlayers; ++j )
            {
                if( m_teamPosition[i] == j ) used[j] = true;
            }
        }

        for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
        {
            if( !used[i] )  unused.PutData(i);
        }

        if( unused.Size() == 0 ) return -1; // no spaces left, more teams than available slots

        m_teamPosition[_teamId] = unused[ syncrand() % unused.Size() ];
    }
    
    return m_teamPosition[_teamId];
}

int Location::GetTeamFromPosition( int _positionId )
{
	if (g_app->IsSinglePlayer())
	{
		return _positionId;
	}

    for( int i = 0; i < m_levelFile->m_numPlayers; ++i )
    {
        if( m_teamPosition[i] == -1 ) GetTeamPosition(i);
        if( m_teamPosition[i] == _positionId ) return i;
    }

    return 255;
}

void Location::RotatePositions()
{
    int newPositions[NUM_TEAMS];
    memset( newPositions, -1, sizeof(int) * NUM_TEAMS );
    int index = 0;
    for( int i = 0; i < m_levelFile->m_numPlayers; ++i )
    {
        index++;
        if( index >= m_levelFile->m_numPlayers ) index = 0;
        newPositions[i] = m_teamPosition[index];
    }
    memcpy( m_teamPosition, newPositions, sizeof(int) * NUM_TEAMS );
}

RGBAColour Location::GetGroupColour( int _groupId )
{
    if( m_groupColourId[_groupId-1] != -1 )
    {
        if( m_coopGroupColourTaken[_groupId-1][0] )
        {
            m_coopGroupColourTaken[_groupId-1][1] = true;
            return g_app->m_multiwinia->GetGroupColour( 1, m_groupColourId[_groupId-1] );
        }
        else
        {
            m_coopGroupColourTaken[_groupId-1][0] = true;
            return g_app->m_multiwinia->GetGroupColour( 0, m_groupColourId[_groupId-1] );
        }
    }
    return RGBAColour(255,255,255);
}

void Location::SetGroupColourId( int _groupId, int _colourId )
{
    int id = _groupId -1;
    m_groupColourId[id] = _colourId;
}

bool Location::IsAttacking( int _teamId )
{
    if( _teamId == 255 ) return false;

    return ( m_levelFile->m_attacking[ GetTeamPosition( _teamId ) ] );
}

bool Location::IsDefending(int _teamId)
{
    if( _teamId == 255 ) return false;
    return !IsAttacking( _teamId );
}

void Location::SetSoloTeam( int _teamId, int _groupId )
{
   // Team *team = m_teams[_teamId];
    int colourId = g_app->m_multiwinia->m_teams[_teamId].m_colourId;
    if( colourId == -1 )
    {
        LList<int> used;
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused ) continue;
            if( g_app->m_multiwinia->m_teams[i].m_colourId != -1 ) used.PutData( g_app->m_multiwinia->m_teams[i].m_colourId );
        }

        while( colourId == -1 )
        {
            int id = syncrand() % 4;
            bool bad = false;

            for( int i = 0; i < used.Size(); ++i )
            {
                if( id == used[i] )
                {
                    bad = true;
                    break;
                }
            }

            if( !bad )
            {
                colourId = id;
            }
        }

    }

    m_coopGroups[_teamId] = _groupId;
    SetGroupColourId( _groupId, colourId );
    g_app->m_multiwinia->m_teams[_teamId].m_colourId = colourId;
    g_app->m_multiwinia->m_teams[_teamId].m_colour = g_app->m_multiwinia->GetColour( colourId );
    //GetGroupColour( setGroup );

    if( _groupId == 1 ) m_teamPosition[_teamId] = m_levelFile->m_coopGroup1Positions[0];
    else if( _groupId == 2 ) m_teamPosition[_teamId] = m_levelFile->m_coopGroup2Positions[0];

    g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 0;
}

void Location::SetCoopTeam( int _teamId )
{
    //Team *team = m_teams[_teamId];
    int totalTeam1Players = 0;
    int totalTeam2Players = 0;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_coopGroups[i] == 1 ) totalTeam1Players++;
        if( m_coopGroups[i] == 2 ) totalTeam2Players++;
    }

    bool findUnassignedTeam = true;
    if( g_app->m_multiwinia->m_teams[_teamId].m_colourId != -1 )
    {
        if( m_coopGroups[_teamId] != 0 )
        {
            // we've already been put into our group by our other team mate
            // we dont need ot do anything here, so just do nothing
            findUnassignedTeam = false;
        }
        else
        {
            // this team has selected a colour, so check to see if another team has joined them
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( i == _teamId ) continue;
                if( i == GetMonsterTeamId() || i == GetFuturewinianTeamId() ) continue;
                if( g_app->m_multiwinia->m_teams[i].m_colourId == g_app->m_multiwinia->m_teams[_teamId].m_colourId )
                {
                    // found a match
                    if( m_coopGroups[i] != 0 )
                    {
                        // our partner has already been assigned a group, so drop us in the same group
                        m_coopGroups[_teamId] = m_coopGroups[i];
                    }
                    else if( totalTeam1Players == 0 )
                    {
                        // team 1 is empty, so put us both in there
                        m_coopGroups[_teamId] = 1;
                        m_coopGroups[i] = 1;
                    }
                    else if( totalTeam2Players == 0 )
                    {
                        m_coopGroups[_teamId] = 2;
                        m_coopGroups[i] = 2;
                    }

                    SetGroupColourId( m_coopGroups[_teamId], g_app->m_multiwinia->m_teams[_teamId].m_colourId );
                    g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour( m_coopGroups[_teamId] );
                    g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;
                    g_app->m_multiwinia->m_teams[i].m_colour = GetGroupColour( m_coopGroups[_teamId] );
                    g_app->m_multiwinia->m_teams[i].m_coopGroupPosition = 2;

                    findUnassignedTeam = false;
                    break;
                }
            }
        }
    }
    
    if( findUnassignedTeam )
    {
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( i == _teamId ) continue;
            if( i == GetMonsterTeamId() || i == GetFuturewinianTeamId() ) continue;
            if( g_app->m_multiwinia->m_teams[i].m_colourId == -1 )
            {
                // this team didnt pick a colour either so just team up with them
                if( totalTeam1Players == 0 )
                {
                    m_coopGroups[_teamId] = 1;
                    m_coopGroups[i] = 1;
                }
                else if( totalTeam2Players == 0 )
                {
                    m_coopGroups[_teamId] = 2;
                    m_coopGroups[i] = 2;
                }
                else
                {
                    // assignment has already started on the other 2 teams, so now we need to find one with a space to join
                    continue;
                }

                bool taken[MULTIWINIA_NUM_TEAMCOLOURS];
                memset( taken, false, sizeof(bool) * MULTIWINIA_NUM_TEAMCOLOURS );
                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( g_app->m_multiwinia->m_teams[t].m_colourId != -1 )
                    {
                        taken[g_app->m_multiwinia->m_teams[t].m_colourId] = true;
                    }
                }

                int colourId = 0;
                if( g_app->m_multiwinia->m_teams[_teamId].m_colourId != -1 ) colourId = g_app->m_multiwinia->m_teams[_teamId].m_colourId;
                else
                {
                    while( taken[colourId] )
                    {
                        colourId = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS/2;
                    }
                }

                SetGroupColourId( m_coopGroups[_teamId], colourId );
                g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                g_app->m_multiwinia->m_teams[_teamId].m_colourId = colourId;
                g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;
                g_app->m_multiwinia->m_teams[i].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                g_app->m_multiwinia->m_teams[i].m_colourId= colourId;
                g_app->m_multiwinia->m_teams[i].m_coopGroupPosition = 2;
                break;
            }
            else if( g_app->m_multiwinia->m_teams[_teamId].m_colourId == -1 )
            {
                bool partnerFound = false;
                // this team has a colour, so check to see if another team is paired with them
                for( int t = 0; t < NUM_TEAMS; ++t )
                {
                    if( t == i ) continue;
                    if( i == GetMonsterTeamId() || i == GetFuturewinianTeamId() ) continue;
                    if( g_app->m_multiwinia->m_teams[t].m_colourId == g_app->m_multiwinia->m_teams[i].m_colourId )
                    {
                        partnerFound = true;
                        break;
                    }
                }

                if( !partnerFound )
                {
                    // this team is alone, so join them
                    if( m_coopGroups[i] == 0 )
                    {
                        // not assigned to a group yet, so set us both up now
                        if( totalTeam1Players == 0 )
                        {
                            m_coopGroups[_teamId] = 1;
                            m_coopGroups[i] = 1;
                        }
                        else
                        {
                            m_coopGroups[_teamId] = 2;
                            m_coopGroups[i] = 2;
                        }

                        SetGroupColourId( m_coopGroups[_teamId], g_app->m_multiwinia->m_teams[i].m_colourId );
                        g_app->m_multiwinia->m_teams[i].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                        g_app->m_multiwinia->m_teams[i].m_coopGroupPosition = 1;

                        g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                        g_app->m_multiwinia->m_teams[_teamId].m_colourId = g_app->m_multiwinia->m_teams[i].m_colourId;
                        g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 2;
                    }
                    else
                    {
                        m_coopGroups[_teamId] = m_coopGroups[i];
                        g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(m_coopGroups[i]);
                    }
                }
            }
        }

        if( m_coopGroups[_teamId] == 0 )
        {
            // we scanned through all the teams, and we still don't have a teammate
            // this means that at least 2 teams picked colours and werent paired with another player
            // in this case, just pick them at random and give them a random colour
            // if they cant play nice, we'll force them

            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( t == _teamId ) continue;
                if( t == GetMonsterTeamId() || t == GetFuturewinianTeamId() ) continue;
                bool paired = false;
                for( int i = 0; i < NUM_TEAMS; ++i )
                {
                    if( i == _teamId ) continue;
                    if( i == GetMonsterTeamId() || i == GetFuturewinianTeamId() ) continue; 
                    if( g_app->m_multiwinia->m_teams[t].m_colourId == g_app->m_multiwinia->m_teams[i].m_colourId )
                    {
                        paired = true;
                        break;
                    }
                }

                if( !paired )
                {
                    // ok, this team doesnt have a team mate, so they'll do 
                    if( totalTeam1Players == 0 )
                    {
                        m_coopGroups[_teamId] = 1;
                        m_coopGroups[t] = 1;
                    }
                    else
                    {
                        m_coopGroups[_teamId] = 2;
                        m_coopGroups[t] = 2;
                    }

                    // find us a random colour
                    // this will probably be a colour which neither team has chosen, since their ids are currently set
                    // but thats fine, it will teach them not to be able to pick teams nicely!
                    bool taken[MULTIWINIA_NUM_TEAMCOLOURS];
                    memset( taken, false, sizeof(bool) * MULTIWINIA_NUM_TEAMCOLOURS );
                    for( int i = 0; i < NUM_TEAMS; ++i )
                    {
                        if( g_app->m_multiwinia->m_teams[i].m_colourId != -1 )
                        {
                            taken[g_app->m_multiwinia->m_teams[i].m_colourId] = true;
                        }
                    }

                    int colourId = 0;
                    while( taken[colourId] )
                    {
                        colourId = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS/2;
                    }

                    SetGroupColourId( m_coopGroups[_teamId], colourId );
                    g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                    g_app->m_multiwinia->m_teams[_teamId].m_colourId = colourId;
                    g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;
                    g_app->m_multiwinia->m_teams[t].m_colour = GetGroupColour(m_coopGroups[_teamId]);
                    g_app->m_multiwinia->m_teams[t].m_colourId = colourId;
                    g_app->m_multiwinia->m_teams[t].m_coopGroupPosition = 2;

                    break;
                }
            }
        }
    }
}

void Location::SetCoopTeam( int _teamId, int _groupId )
{
    int setGroup = _groupId;
    //Team *team = m_teams[_teamId];
    // we need to set up fixed-position teams here
    m_coopGroups[_teamId] = setGroup;
    int position = GetTeamPosition( _teamId );
    int position2 = -1;
    if( setGroup == 1 )
    {
        if( m_levelFile->m_coopGroup1Positions[0] == position ) position2 = m_levelFile->m_coopGroup1Positions[1];
        else position2 = m_levelFile->m_coopGroup1Positions[0];
    }
    else
    {
        if( m_levelFile->m_coopGroup2Positions[0] == position ) position2 = m_levelFile->m_coopGroup2Positions[1];
        else position2 = m_levelFile->m_coopGroup2Positions[0];
    }

    if( g_app->m_multiwinia->m_teams[_teamId].m_colourId != -1 )
    {
        SetGroupColourId( setGroup, g_app->m_multiwinia->m_teams[_teamId].m_colourId );
        bool matchFound = false;
        int unassignedTeam = -1;
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            if( i == _teamId ) continue;
            if( g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused ) continue;

            if( g_app->m_multiwinia->m_teams[i].m_colourId == g_app->m_multiwinia->m_teams[_teamId].m_colourId )
            {
                g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(setGroup);
                g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;

                g_app->m_multiwinia->m_teams[i].m_coopGroupPosition = 2;
                g_app->m_multiwinia->m_teams[i].m_colour = GetGroupColour(setGroup);

                m_coopGroups[i] = setGroup;

                m_teamPosition[i] = position2;
                matchFound = true;
                break;
            }
            else if( g_app->m_multiwinia->m_teams[i].m_colourId == -1 )
            {
                unassignedTeam = i;
            }
        }
        if( !matchFound && unassignedTeam != -1 )
        {
            // we picked a colour but no one else joined us

            g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour(setGroup);
            g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;

            g_app->m_multiwinia->m_teams[unassignedTeam].m_coopGroupPosition = 2;
            g_app->m_multiwinia->m_teams[unassignedTeam].m_colour = GetGroupColour(setGroup);
            g_app->m_multiwinia->m_teams[unassignedTeam].m_colourId = g_app->m_multiwinia->m_teams[_teamId].m_colourId;

            m_coopGroups[unassignedTeam] = setGroup;

            m_teamPosition[unassignedTeam] = position2;

        }
    }
    else
    {
        // we didnt pick a colour, so find us one randomly
        for( int t = 0; t < NUM_TEAMS; ++t )
        {
            if( t == _teamId ) continue;
            if( g_app->m_multiwinia->m_teams[t].m_teamType == TeamTypeUnused ) continue;

            if( g_app->m_multiwinia->m_teams[t].m_colourId == -1 )
            {
                // find us a random colour for our team
                int colourId = -1;
                bool taken[MULTIWINIA_NUM_TEAMCOLOURS];
                memset( taken, false, sizeof(bool) * MULTIWINIA_NUM_TEAMCOLOURS );

                for( int i = 0; i < NUM_TEAMS; ++i )
                {
                    if( g_app->m_multiwinia->m_teams[i].m_colourId != -1 )
                    {
                        taken[g_app->m_multiwinia->m_teams[i].m_colourId] = true;
                    }
                }

                colourId = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS/2;
                while( taken[colourId] )
                {
                    colourId = syncrand() % MULTIWINIA_NUM_TEAMCOLOURS/2;
                }

                m_teamPosition[t] = position2;
                g_app->m_multiwinia->m_teams[_teamId].m_colourId = colourId;
                g_app->m_multiwinia->m_teams[t].m_colourId = colourId;
                m_coopGroups[t] = setGroup;
                SetGroupColourId( setGroup, colourId );

                g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour( setGroup );
                g_app->m_multiwinia->m_teams[t].m_colour = GetGroupColour( setGroup );

                g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 1;
                g_app->m_multiwinia->m_teams[t].m_coopGroupPosition = 2;
                break;
            }
            else
            {
                bool matchFound = false;
                for( int i = 0; i < NUM_TEAMS; ++i )
                {
                    bool notLoner = true;
                    if( m_coopGroups[i] == 1 && m_levelFile->m_coopGroup1Positions.Size() == 1 ) notLoner = false;
                    else if( m_coopGroups[i] == 2 && m_levelFile->m_coopGroup2Positions.Size() == 1 ) notLoner = false;

                    if( g_app->m_multiwinia->m_teams[t].m_colourId == g_app->m_multiwinia->m_teams[i].m_colourId || !notLoner )
                    {
                        matchFound = true;
                        break;
                    }                            
                }

                if( !matchFound )
                {
                    int colourId = g_app->m_multiwinia->m_teams[t].m_colourId;

                    m_teamPosition[t] = position2;
                    g_app->m_multiwinia->m_teams[_teamId].m_colourId = colourId;
                    m_coopGroups[t] = setGroup;
                    SetGroupColourId( setGroup, colourId );

                    g_app->m_multiwinia->m_teams[t].m_colour = GetGroupColour( setGroup );
                    g_app->m_multiwinia->m_teams[_teamId].m_colour = GetGroupColour( setGroup );

                    g_app->m_multiwinia->m_teams[t].m_coopGroupPosition = 1;
                    g_app->m_multiwinia->m_teams[_teamId].m_coopGroupPosition = 2;
                    break;
                }
            }                        
        }
    }
}

int Location::GetNumAttackers()
{
    int attacking = 0;
    for( int i = 0; i < m_levelFile->m_numPlayers; ++i )
    {
        if( m_levelFile->m_attacking[i] ) attacking++;
    }

    return attacking;
}

void Location::GetCurrentTeamSpread( int &_attackers, int &_defenders )
{
    for( int i = 0; i < m_levelFile->m_numPlayers; ++i )
    {
        if( m_teamPosition[i] != -1 && m_levelFile->m_attacking[ m_teamPosition[i] ] ) _attackers++;
        else if( g_app->m_multiwinia->m_teams[i].m_colourId == ASSAULT_ATTACKER_COLOURID ) _attackers++;

        if( m_teamPosition[i] != -1 && !m_levelFile->m_attacking[ m_teamPosition[i] ] ) _defenders++;
        else if( g_app->m_multiwinia->m_teams[i].m_colourId == ASSAULT_DEFENDER_COLOURID ) _defenders++;
    }
}

void Location::SetupAssault()
{
    memset( m_teamPosition, -1, sizeof(m_teamPosition));

    int totalAttackers = GetNumAttackers();
    int totalDefenders = m_levelFile->m_numPlayers - totalAttackers;

    for( int i = 0; i < m_levelFile->m_numPlayers; ++i )
    {
        int numAttackers = 0, numDefenders = 0;
        GetCurrentTeamSpread( numAttackers, numDefenders );
        LobbyTeam *team = &g_app->m_multiwinia->m_teams[i];
        if( team->m_colourId == -1 )
        {
            if( numAttackers == totalAttackers ) team->m_colourId = ASSAULT_DEFENDER_COLOURID;
            else if( numDefenders == totalDefenders ) team->m_colourId = ASSAULT_ATTACKER_COLOURID;
            else team->m_colourId = syncrand() % 2;
        }

        if( team->m_colourId == ASSAULT_DEFENDER_COLOURID ) m_teamPosition[i] = GetFreeDefenderPosition();
        else m_teamPosition[i] = GetFreeAttackerPosition();

        int colourId = team->m_colourId;
        team->m_colour = g_app->m_multiwinia->GetColour( colourId );

        if( g_app->m_multiwinia->m_coopMode )
        {
            int group = -1;
            if( m_levelFile->m_coopGroup1Positions.FindData( m_teamPosition[i] ) != -1 ) group = 1;
            else group = 2;
            m_coopGroups[i] = group;
            SetGroupColourId( group, colourId );

            int position = 1;
            if( colourId == ASSAULT_DEFENDER_COLOURID && numDefenders > 0 ) position = 2;
            else if( colourId == ASSAULT_ATTACKER_COLOURID && numAttackers > 0 ) position = 2;

            team->m_colour = GetGroupColour( group );
            team->m_coopGroupPosition = position;
        }
    }    
}

int Location::GetFreeAttackerPosition()
{
    DArray<int> taken;
    DArray<int> positions;
    for( int i = 0; i < m_levelFile->m_numPlayers;++i )
    {
        if( m_teamPosition[i] != -1 && m_levelFile->m_attacking[ m_teamPosition[i] ] ) taken.PutData(m_teamPosition[i]);
        if( m_levelFile->m_attacking[i] ) positions.PutData(i);
    }

    DArray<int> valid;
    for( int i = 0; i < positions.Size(); ++i )
    {
        bool invalid = false;
        for( int j = 0; j < taken.Size(); ++j )
        {
            if( positions[i] == taken[j] )
            {
                invalid = true;
            }
        }

        if( !invalid ) valid.PutData( positions[i] );
    }

    if( valid.Size() > 0 ) return valid[ syncrand() % valid.Size() ];
    else return -1;
}

int Location::GetFreeDefenderPosition()
{
    DArray<int> taken;
    DArray<int> positions;
    for( int i = 0; i < m_levelFile->m_numPlayers;++i )
    {
        if( m_teamPosition[i] != -1 && !m_levelFile->m_attacking[ m_teamPosition[i] ] ) taken.PutData(m_teamPosition[i]);
        if( !m_levelFile->m_attacking[i] ) positions.PutData(i);
    }

    DArray<int> valid;
    for( int i = 0; i < positions.Size(); ++i )
    {
        bool invalid = false;
        for( int j = 0; j < taken.Size(); ++j )
        {
            if( positions[i] == taken[j] )
            {
                invalid = true;
            }
        }

        if( !invalid ) valid.PutData( positions[i] );
    }

    if( valid.Size() > 0 ) return valid[ syncrand() % valid.Size() ];
    else return -1;
}

void Location::SetupCoop( int _teamId )
{
    int position = GetTeamPosition( _teamId );
    int totalTeam1Players = 0;
    int totalTeam2Players = 0;
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_coopGroups[i] == 1 ) totalTeam1Players++;
        if( m_coopGroups[i] == 2 ) totalTeam2Players++;
    }

    int setGroup = 0;
    bool loner = false;

    bool group1Solo = false, group2Solo = false;
    if( m_levelFile->m_coopGroup1Positions.Size() == 1 ) group1Solo = true;
    if( m_levelFile->m_coopGroup2Positions.Size() == 1 ) group2Solo = true;

    for( int i = 0; i < m_levelFile->m_coopGroup1Positions.Size(); ++i )
    {
        if( m_levelFile->m_coopGroup1Positions[i] == position )
        {
            setGroup = 1;
            if( totalTeam1Players == 2 || 
                (totalTeam1Players == 1 && group1Solo) )
            {
                setGroup = 2;
                if( GetTeamFromPosition(m_levelFile->m_coopGroup2Positions[0]) == 255 || group2Solo) 
                {
                    position = m_levelFile->m_coopGroup2Positions[0];
                }
                else
                {
                    position = m_levelFile->m_coopGroup2Positions[1];
                }
                m_teamPosition[_teamId] = position;
            }
            break;
        }
    }

    if( setGroup == 0 )
    {
        for( int i = 0; i < m_levelFile->m_coopGroup2Positions.Size(); ++i )
        {
            if( m_levelFile->m_coopGroup2Positions[i] == position )
            {
                setGroup = 2;
                if( totalTeam2Players == 2 || 
                   (totalTeam2Players == 1 && group2Solo) ) 
                {
                    setGroup = 1;
                    if( GetTeamFromPosition(m_levelFile->m_coopGroup1Positions[0]) == 255 || group1Solo ) 
                    {
                        position = m_levelFile->m_coopGroup1Positions[0];
                        
                    }
                    else
                    {
                        position = m_levelFile->m_coopGroup1Positions[1];
                    }
                    m_teamPosition[_teamId] = position;
                }
                break;
            }
        }
    }

    for( int i = 0; i < m_levelFile->m_numPlayers; ++i  )
    {
        if( i == _teamId ) continue;
        if( m_teamPosition[i] == m_teamPosition[_teamId] ) m_teamPosition[i] = -1;
        GetTeamPosition(i);
    }

    if( setGroup == 1 && m_levelFile->m_coopGroup1Positions.Size() == 1 ) loner = true;
    else if( setGroup == 2 && m_levelFile->m_coopGroup2Positions.Size() == 1 ) loner = true;

    if( loner )
    {
        // this team doesnt have a team mate
        SetSoloTeam( _teamId, setGroup );
    }
    else if( setGroup == 0 ) 
    {
        // no predefined alliance groups, so pick some randomly
        SetCoopTeam( _teamId );
    }
    else
    {
        // there is a fixed group for this team so set that up
        SetCoopTeam( _teamId, setGroup );
    }

    if( g_app->m_globalWorld->m_myTeamId == _teamId )
    {
        g_app->m_clientToServer->ReliableSetTeamColour( g_app->m_multiwinia->m_teams[_teamId].m_colourId );
    }
}

void Location::InitialiseTeam( unsigned char _teamId )
{    
    if( _teamId < 0 || _teamId >= NUM_TEAMS ) return;

	Team *team = m_teams[_teamId];
	int _teamType = team->m_teamType;

	team->Initialise();  

	if( g_app->m_gameMode == App::GameModeMultiwinia )
	{
		if( _teamType == TeamTypeCPU )
		{
			SpawnEntities( Vector3(-100,0,-100), _teamId, -1, Entity::TypeAI, 1, g_zeroVector, 0 );
		}

        /*if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeShaman )
        {
            if( Portal::s_masterPortals[_teamId] )
            {
                Vector3 pos = Portal::s_masterPortals[_teamId]->m_pos;
                team->m_taskManager->RunTask( GlobalResearch::TypeShaman );
                for( int i = 0; i < team->m_taskManager->m_tasks.Size(); ++i )
                {
                    if( team->m_taskManager->m_tasks[i]->m_type == GlobalResearch::TypeShaman )
                    {
                        team->m_taskManager->TargetTask( team->m_taskManager->m_tasks[i]->m_id, pos );
                    }
                }
            }
        }*/

        int numPowerups =  g_app->m_multiwinia->GetGameOption( "StartingPowerups" );
        if( _teamId == GetFuturewinianTeamId() || _teamId == GetMonsterTeamId() ) numPowerups = 0;
        if( numPowerups > 0 )
        {
            for( int i = 0; i < numPowerups; ++i )
            {
                int powerupId = -1;
                while(true)
                {
                    powerupId = syncrand() % Crate::NumCrateRewards;
                    if( powerupId != Crate::CrateRewardRandomiser &&
                        powerupId != Crate::CrateRewardCrateMania &&
                        powerupId != Crate::CrateRewardDarwinians &&
                        powerupId != Crate::CrateRewardBitzkreig &&
                        powerupId != Crate::CrateRewardSpawnMania &&
//                        powerupId != Crate::CrateRewardFormationIncrease &&
                        powerupId != Crate::CrateRewardPlague &&
                        powerupId != Crate::CrateRewardRocketDarwinians &&
                        powerupId != Crate::CrateRewardSpaceShip &&
                        powerupId != Crate::CrateRewardSpam &&
                        powerupId != Crate::CrateRewardTriffids &&
                        powerupId != Crate::CrateRewardSpeedUpTime &&
                        powerupId != Crate::CrateRewardSlowDownTime &&
                        powerupId != Crate::CrateRewardForest &&
#ifdef INCLUDE_THEBEAST
						powerupId != Crate::CrateRewardTheBeast &&
#endif
                        Crate::IsValid( powerupId ) )
                    {
                        break;
                    }
                }
                Crate::CaptureCrate( powerupId, _teamId );
            }
            for( int i = 0; i < team->m_taskManager->m_tasks.Size(); ++i )
            {
                if( team->m_taskManager->m_tasks[i]->m_type != GlobalResearch::TypeShaman )
                {
                    team->m_taskManager->m_tasks[i]->m_startTimer = 30.0;
                }
            
            
            }           
        }

        // look for any spawn points which are on our team and spawn some darwinians on it
        for( int i = 0; i < m_buildings.Size(); ++i )
        {
            if( m_buildings.ValidIndex(i) )
            {
                Building *b = m_buildings[i];
                if( (b->m_type == Building::TypeSpawnPoint ||
                     b->m_type == Building::TypeSolarPanel ) &&
                    b->m_id.GetTeamId() == _teamId )
                {
                    for( int j = 0; j < b->m_ports.Size(); ++j )
                    {
                        Vector3 portPos, portFront;
                        b->GetPortPosition( j, portPos, portFront );
                        WorldObjectId id = SpawnEntities( portPos, _teamId, -1, Entity::TypeDarwinian, 1, g_zeroVector, 0.0 );
                        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
                        if( d )
                        {
                            d->m_state = Darwinian::StateOperatingPort;
                            d->m_front = portFront;
                            d->m_portId = j;
                            d->m_buildingId = b->m_id.GetUniqueId();
                        }
                    }
                    if( b->m_type == Building::TypeSpawnPoint )
                    {
                        SpawnPoint *sp = (SpawnPoint *)b;
                        sp->EvaluatePorts();
                        sp->RecalculateOwnership();
                        sp->m_activationTimer = 0.0;
                    }
                    else if( b->m_type == Building::TypeSolarPanel )
                    {
                        SolarPanel *sp = (SolarPanel *)b;
                        sp->EvaluatePorts();
                        sp->RecalculateOwnership();
                        //sp->m_activationTimer = 0.0f;
                    }
                }
            }
        }
	}    

    int position = GetTeamPosition( _teamId );

    //
	// Create instant units that belong to this team
	for (int i = 0; i < m_levelFile->m_instantUnits.Size(); i++)
	{
		InstantUnit *iu = m_levelFile->m_instantUnits.GetData(i);
		if (position != iu->m_teamId) continue;
		
		Team *team = g_app->m_location->m_teams[_teamId];
		Vector3 pos(iu->m_posX, 0, iu->m_posZ);
		pos.y = m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        if( iu->m_runAsTask )
        {
            int taskType = -1;
            switch( iu->m_type )
            {
                case Entity::TypeArmour:    taskType = GlobalResearch::TypeArmour;          break;
                case Entity::TypeInsertionSquadie:  taskType = GlobalResearch::TypeSquad;   break;
                case Entity::TypeHarvester: taskType = GlobalResearch::TypeHarvester;       break;
                case Entity::TypeShaman:    taskType = GlobalResearch::TypeShaman;          break;  
                case Entity::TypeEngineer:  taskType = GlobalResearch::TypeEngineer;        break;
                case Entity::TypeTank:      taskType = GlobalResearch::TypeTank;            break;
            }

            if( taskType == -1 ) continue;

            team->m_taskManager->RunTask( taskType );
            int taskId = team->m_taskManager->m_mostRecentTaskId;
            Vector3 pos( iu->m_posX, 0, iu->m_posZ );
            pos.y = m_landscape.m_heightMap->GetValue( pos.x, pos.z );
            team->m_taskManager->m_verifyTargetting = false;
            team->m_taskManager->TargetTask( taskId, pos );
            team->m_taskManager->m_verifyTargetting = true;
            team->m_taskManager->GetTask(taskId)->m_lifeTimer = -1.0;
        }
        else
        {
		    int unitId = -1;
            Unit *newUnit = NULL;
            if( iu->m_inAUnit ) 
            {
                newUnit = team->NewUnit(iu->m_type, iu->m_number, &unitId, pos);
		        newUnit->SetWayPoint(pos);
			    newUnit->m_routeId = iu->m_routeId;
			    iu->m_routeId = -1;
            }

            WorldObjectId spawnedId = SpawnEntities(pos, _teamId, unitId, iu->m_type, iu->m_number, g_zeroVector, iu->m_spread, -1.0, iu->m_routeId, iu->m_routeWaypointId );
            
            //
            // Is the waypoint in a Radar Dish?
            
            Vector3 targetPos( iu->m_waypointX, 0, iu->m_waypointZ);
            LList<int> *buildingIds = m_obstructionGrid->GetBuildings( iu->m_waypointX, iu->m_waypointZ );
            for( int i = 0; i < buildingIds->Size(); ++i )
            {
                Building *building = GetBuilding( buildingIds->GetData(i) );
                if( building && building->m_type == Building::TypeRadarDish )
                {
                    Vector3 waypointToBuilding = ( building->m_pos - targetPos );
                    waypointToBuilding.y = 0;
                    if( waypointToBuilding.Mag() < building->m_radius )
                    {
                        targetPos = building->m_pos;
                        break;
                    }
                }
            }

            if( iu->m_type == Entity::TypeDarwinian && iu->m_number == 1 && iu->m_state == Darwinian::StateFollowingOrders )
            {
                Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( spawnedId, Entity::TypeDarwinian );
                if( darwinian ) darwinian->GiveOrders( targetPos );
            }
            if( iu->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( spawnedId, Entity::TypeOfficer );
                if( iu->m_state == Officer::OrderGoto )
                {                
                    officer->SetOrders( targetPos );
                }
                else
                {
                    officer->m_orders = iu->m_state;
                }
            }
            if( iu->m_type == Entity::TypeArmour )
            {
                Armour *armour = (Armour *) g_app->m_location->GetEntitySafe( spawnedId, Entity::TypeArmour );
                armour->SetWayPoint( targetPos );
                armour->m_state = iu->m_state;
            }
        }
	}

    //
    // Attach all units to the ground

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = g_app->m_location->m_teams[t];
        if( team )
        {
            for( int i = 0; i < team->m_others.Size(); ++i )
            {
                if( team->m_others.ValidIndex(i) )
                {
                    Entity *entity = team->m_others[i];
                    entity->m_pos.y = m_landscape.m_heightMap->GetValue( entity->m_pos.x, entity->m_pos.z );
                }
            }
        }
    }

    //
    // Are there any Running programs that need to be started?

    if( _teamType == TeamTypeLocalPlayer )
    {
        for( int i = 0; i < m_levelFile->m_runningPrograms.Size(); ++i )
        {
            RunningProgram *program = m_levelFile->m_runningPrograms[i];
            
            if( program->m_type == Entity::TypeEngineer )
            {
                Vector3 pos( program->m_positionX[0], 0, program->m_positionZ[0] );
                pos.y = m_landscape.m_heightMap->GetValue( pos.x, pos.z );
                WorldObjectId objId = SpawnEntities( pos, _teamId, -1, Entity::TypeEngineer, 1, g_zeroVector, 0.0 );
                Engineer *engineer = (Engineer *) GetEntitySafe( objId, Entity::TypeEngineer );
                engineer->m_state = program->m_state;
                engineer->m_wayPoint.Set( program->m_waypointX, 0, program->m_waypointZ );
                engineer->m_wayPoint.y = m_landscape.m_heightMap->GetValue( program->m_waypointX, program->m_waypointZ );
                engineer->m_stats[Entity::StatHealth] = program->m_health[0];

                // m_data = num spirits carried 
                for( int s = 0; s < program->m_data; ++s )
                {
                    int index = SpawnSpirit( engineer->m_pos, g_zeroVector, 1, WorldObjectId() );
                    engineer->CollectSpirit( index );
                }

                Task *task = new Task(team->m_teamId);
                task->m_type = GlobalResearch::TypeEngineer;
                task->m_objId = objId;
                task->m_state = Task::StateRunning;
                team->m_taskManager->RegisterTask( task );
            }

            if( program->m_type == Entity::TypeInsertionSquadie )
            {
                Vector3 pos( program->m_positionX[0], 0, program->m_positionZ[0] );
                pos.y = m_landscape.m_heightMap->GetValue( pos.x, pos.z );

                int unitId;
                InsertionSquad *squad = (InsertionSquad *) GetMyTeam()->NewUnit( Entity::TypeInsertionSquadie, program->m_count, &unitId, pos );
                
                Vector3 waypoint( program->m_waypointX, 0, program->m_waypointZ );
                waypoint.y = m_landscape.m_heightMap->GetValue( program->m_waypointX, program->m_waypointZ );                   
                //squad->SetWayPoint( waypoint );
                squad->SetWeaponType( program->m_data );
                
                SpawnEntities( pos, _teamId, unitId, Entity::TypeInsertionSquadie, program->m_count, g_zeroVector, 10.0 );
                
                for( int s = 0; s < squad->m_entities.Size(); ++s )
                {
                    AppDebugAssert( squad->m_entities.ValidIndex(s) );                    
                    Entity *entity = squad->m_entities[s];
                    entity->m_stats[Entity::StatHealth] = program->m_health[s];                    
                }

                Task *task = new Task(team->m_teamId);
                task->m_type = GlobalResearch::TypeSquad;
                task->m_objId.Set( _teamId, unitId, -1, -1 );
                task->m_state = Task::StateRunning;
                team->m_taskManager->RegisterTask( task );
            }
        }
    }
}


/*
void Location::RemoveTeam( int _teamId )
{
    if( _teamId < NUM_TEAMS )
    {
        Team *team = m_teams[_teamId];
        m_teams[_teamId]->m_teamType = TeamTypeUnused;
    }

    if( _teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_globalWorld->m_myTeamId = 255;
    }	
}
*/

bool Location::SelectDarwinians ( unsigned char teamId, Vector3 const &pos, double radius )
{
    bool myTeam = ( teamId == g_app->m_globalWorld->m_myTeamId );

    int numFound;
    bool include[NUM_TEAMS];
    memset( include, false, sizeof(include) );
    include[teamId] = true;
    m_entityGrid->GetNeighbours( s_neighbours, pos.x, pos.z, radius, &numFound, include );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Darwinian *darwinian = (Darwinian *)GetEntitySafe( id, Entity::TypeDarwinian );
        if( darwinian &&
            darwinian->m_state != Darwinian::StateOperatingPort &&
            darwinian->m_state != Darwinian::StateBeingSacraficed &&
            darwinian->m_state != Darwinian::StateOnFire &&
            darwinian->m_state != Darwinian::StateInsideArmour &&
            !darwinian->m_underPlayerControl )
        {
            darwinian->m_underPlayerControl = true;
            if( myTeam ) g_app->m_soundSystem->TriggerEntityEvent( darwinian, "Selected" );
        }
    }   

    return( numFound > 0 );
}


void Location::IssueDarwinianOrders( unsigned char teamId, Vector3 const &pos, bool directRoute )
{
    Team *team = m_teams[teamId];

    Vector3 targetPos = pos;
    Vector3 teleportEntrance;

    LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( pos.x, pos.z );
    for( int i = 0; i < nearbyBuildings->Size(); ++i )
    {
        int buildingId = nearbyBuildings->GetData(i);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building->m_type == Building::TypeRadarDish )
        {
            double distance = ( building->m_pos - pos ).Mag();
            Teleport *teleport = (Teleport *) building;
            if( distance < teleport->m_radius && teleport->Connected() )
            {
				Vector3 frontVector; // ignored
                targetPos = teleport->m_pos;
                teleport->GetEntrance(teleportEntrance, frontVector);
                break;
            }
        }
    }


    //
    // Find centre position

    Vector3 centrePos;
    int numFound = 0;

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];
            if( entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *)entity;
                if( darwinian->m_underPlayerControl )
                {
                    centrePos += darwinian->m_pos;
                    numFound++;
                }
            }
        }
    }

    centrePos /= numFound;


    //
    // Build a route

    int routeId = m_routingSystem.GenerateRoute( centrePos, pos, directRoute );
    if( teamId == g_app->m_globalWorld->m_myTeamId )
    {
        //g_app->m_location->m_routingSystem.SetRouteIntensity( routeId, 1.0 );
        g_app->m_location->m_routingSystem.HighlightRoute( routeId, teamId );
    }


    //
    // Issue orders

    numFound = 0;
    m_entityGrid->GetFriends( s_neighbours, targetPos.x, targetPos.z, 30.0f, &numFound, teamId );
    WorldObjectId armourId;
    for( int i = 0; i < numFound; ++i )
    {
        Armour *a = (Armour *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeArmour );
        if( a )
        {
            if( a->GetNumPassengers() < a->Capacity() )
            {
                armourId = a->m_id;
                break;
            }
        }
    }

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];
            if( entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *)entity;
                if( darwinian->m_underPlayerControl )
                {
                    if( armourId.IsValid() )
                    {
                        darwinian->m_state = Darwinian::StateApproachingArmour;
                        darwinian->m_manuallyApproachingArmour = true;
                        darwinian->m_armourId = armourId;

                        Armour *a = (Armour *)g_app->m_location->GetEntitySafe( armourId, Entity::TypeArmour );
                        if( a->m_state == Armour::StateUnloading )
                        {
                            a->m_state = Armour::StateIdle;
                        }
                    }
                    else
                    {
                        darwinian->GiveOrders( targetPos, routeId );
                        darwinian->m_followingPlayerOrders = true;
                    }
                    g_app->m_soundSystem->TriggerEntityEvent( darwinian, "GivenOrdersFromPlayer" );
                }
            }
        }
    }


    //
    // Particle effect

    if( teamId == g_app->m_globalWorld->m_myTeamId &&
        g_app->IsSinglePlayer() )
    {
        OfficerOrders orders;
        orders.m_pos = centrePos;
        orders.m_wayPoint = targetPos;
        if( teleportEntrance != g_zeroVector ) orders.m_wayPoint = teleportEntrance;

        RGBAColour colour( 255, 100, 255, 255 );

        if( g_app->Multiplayer() )
        {
            Team *team = g_app->m_location->m_teams[teamId];
            colour = team->m_colour;            
        }

        int sanity = 0;

        while( sanity < 1000 )
        {   
            sanity++;

            if( orders.m_arrivedTimer < 0.0 )
            {
                g_app->m_particleSystem->CreateParticle( orders.m_pos, g_upVector*10, Particle::TypeMuzzleFlash, 75.0, colour );
                g_app->m_particleSystem->CreateParticle( orders.m_pos, g_upVector*10, Particle::TypeMuzzleFlash, 60.0, colour );
            }
            if( orders.Advance() ) break;
        }    
    }


    if( teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_gameCursor->CreateMarker( pos );    
    }

    //
    // Deselect

    g_app->m_location->m_teams[ teamId ]->SelectUnit(-1,-1,-1);        
}


// *** UpdateTeam
void Location::UpdateTeam( unsigned char teamId, TeamControls const& teamControls )
{
    if( g_app->Multiplayer() )
    {
        if( g_app->GamePaused() ||
            g_app->m_multiwinia->GameOver() )
        {
            return;
        }
    }
	Team *team = m_teams[teamId];
    team->m_currentMousePos = teamControls.m_mousePos;

	// Note: Any reference to the current state of the input manager (g_inputManager) 
	//		 in this code is incorrect, since this works on data sent over the network
	//		 (and may not even correspond to your team!)

	// TODO: This is sent by the server. ACK!!!

	bool unitMove = teamControls.m_unitMove;
	bool primaryFire = teamControls.m_primaryFireTarget;
	bool secondaryFire = teamControls.m_primaryFireTarget && teamControls.m_secondaryFireTarget;

	static bool unitMoved = true;

    Unit *unit = team->GetMyUnit();
    Entity *entity = team->GetMyEntity();

    if( unit )
    {
        if( unit->m_troopType == Entity::TypeInsertionSquadie )
        {
            unitMove = ( teamControls.m_leftButtonPressed &&
                        !primaryFire );
        }

        unit->DirectControl( teamControls );
        if( unitMove )
        {
            unit->SetWayPoint( teamControls.m_mousePos);
            unit->m_targetDir = ( teamControls.m_mousePos - unit->m_centrePos ).Normalise();
            unit->RecalculateOffsets();
			
			//
			// TODO: When do we want to create a marker ?
			//		(doing it all the time for now)

			unitMoved = true;
        }

		if( unitMoved && teamControls.m_endSetTarget &&
            teamId == g_app->m_globalWorld->m_myTeamId ) 
        {
			g_app->m_gameCursor->CreateMarker( teamControls.m_mousePos );
        }

		unitMoved = unitMove;

        if( g_app->IsSinglePlayer() )
        {
            if( teamControls.m_unitSecondaryMode )
            {
                if( teamControls.m_consoleController && unit->m_troopType == Entity::TypeInsertionSquadie )
                {
                    ((InsertionSquad *)unit)->CycleSecondary();
                }
            }
        }

		if( !(teamControls.m_consoleController && unit->m_troopType == Entity::TypeInsertionSquadie) )
		{
			if( primaryFire )
			{
				unit->Attack( teamControls.m_mousePos, false );
				// TODO: Not network safe (modify condition to ensure that the unit
				//       belongs to the local player)
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
				g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadUse );
#endif
			}
			        
			if( secondaryFire )
			{
				unit->Attack( teamControls.m_mousePos, true );
			}
        }
    }
    else if( entity )
    {
        if( teamControls.m_endSetTarget &&
            teamId == g_app->m_globalWorld->m_myTeamId) 
        {
			g_app->m_gameCursor->CreateMarker( teamControls.m_mousePos );
        }

        entity->DirectControl( teamControls );
        switch( entity->m_type )
        {
            case Entity::TypeEngineer:
            {
                Engineer *eng = (Engineer *) entity;
                if( unitMove )
                {
                    eng->SetWaypoint( teamControls.m_mousePos );                    
                    team->SelectUnit(-1,-1,-1);
                    team->m_taskManager->SelectTask(-1);
                }
                if( secondaryFire ) eng->BeginBridge( teamControls.m_mousePos );                                                                    
                break;
            }
        
            case Entity::TypeOfficer:
            {
                Officer *officer = (Officer *) entity;
                if( unitMove )
                {
                    //officer->SetWaypoint( teamControls.m_mousePos ); 
                }
                //if( primaryFire ) officer->SetOrders( teamControls.m_mousePos );
                //if( teamControls.m_secondaryFireDirected ) officer->SetOrders( teamControls.m_mousePos );
                if( officer->m_orders == Officer::OrderPrepareGoto &&
                    officer->m_vel.Mag() < 5.0f )
                {
                    officer->m_front = ( teamControls.m_mousePos - officer->m_pos ).Normalise();
                }
                break;
            }
/*
    NOTE BY CHRIS
    Armour control is now handled in LocationInput::AdvanceTeamControl

            case Entity::TypeArmour:
            {
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::UseArmour );
#endif
                
                Armour *armour = (Armour *) entity;
                if( unitMove )  armour->SetWayPoint( teamControls.m_mousePos );

                if( !teamControls.m_consoleController )
                {
                    if( primaryFire )
                    {
                        armour->SetOrders( teamControls.m_mousePos );

                        if( teamId == g_app->m_globalWorld->m_myTeamId )
                        {
                            int routeId = g_app->m_location->m_routingSystem.GenerateRoute( armour->m_pos, teamControls.m_mousePos, true );
                            g_app->m_location->m_routingSystem.HighlightRoute( routeId, teamId );

                            g_app->m_clientToServer->RequestSelectUnit( teamId, -1, -1, -1 );
                            g_app->m_soundSystem->TriggerEntityEvent( armour, "SetOrders" );
                            g_app->m_markerSystem->RegisterMarker_Orders( armour->m_id );
                        }
                    }
                }
				else //if( g_app->IsSinglePlayer() )
                {
					if( teamControls.m_unitSecondaryMode )
					{
						armour->SetOrders( teamControls.m_mousePos );
					}
					else if( g_app->IsSinglePlayer() )
					{
						if( teamControls.m_deployTurret )
						{
							armour->m_awaitingDeployment = true;
						}
						else if( teamControls.m_unitMove && armour->m_awaitingDeployment )
						{
							armour->SetOrders( armour->m_pos );
						}
					}
                }
                break;
            }
*/

			case Entity::TypeShaman:
			{
				Shaman *shaman = (Shaman *) entity;
				if( unitMove ) shaman->SetWaypoint( teamControls.m_mousePos );
                if( primaryFire ) shaman->ChangeMode( teamControls.m_mousePos );
                if( secondaryFire ) shaman->TeleportToPos( teamControls.m_mousePos );

                if( !primaryFire && !secondaryFire ) shaman->m_renderTeleportTargets = false;
				break;
			}

            case Entity::TypeHarvester:
            {
                Harvester *harvester = (Harvester *)entity;
                if( unitMove ) 
                {
                    harvester->SetWaypoint( teamControls.m_mousePos );
                    team->SelectUnit(-1,-1,-1);
                    team->m_taskManager->SelectTask(-1);
                }
                break;
            }

            case Entity::TypeTank:
            {
                Tank *tank = (Tank *)entity;
                if( unitMove ) tank->SetWayPoint( teamControls.m_mousePos );
                if( !teamControls.m_consoleController )
                {
                    if( primaryFire ) tank->PrimaryFire();
                }
                else
                {
                    if( teamControls.m_unitSecondaryMode ) tank->PrimaryFire();
                }
                tank->SetAttackTarget( teamControls.m_mousePos );
                break;
            }
        }
    }
    else
    {
        Building *b = g_app->m_location->GetBuilding( team->m_currentBuildingId );
        if( b )
        {
            if( b->m_type == Building::TypeGunTurret )
            {
                GunTurret *gt = (GunTurret *)b;
                gt->SetTarget(teamControls.m_mousePos);
                
                if( unitMove || 
                    primaryFire || 
                    teamControls.m_directUnitMove ||
                    teamControls.m_leftButtonPressed )
                {
                    gt->PrimaryFire();
                }
            }
        }
    }    
}


int Location::GetUnitId( Vector3 const &startRay, Vector3 const &direction, unsigned char team, double *_range )
{
    if( team == 255 ) return -1;
            
    double closestRangeSqd = FLT_MAX;
    int unitId = -1;

    //
    // Perform quick preselection by doing ray-sphere intersection tests
    // against the units centrepos and radius.  However we do then need to
    // zoom in and perform ray-sphere checks against each entity, because a unit
    // can become seperated so its bounding sphere covers a very large area.

    for( int unit = 0; unit < m_teams[team]->m_units.Size(); ++unit )
    {
        if( m_teams[team]->m_units.ValidIndex(unit) )
        {
            Unit *theUnit = m_teams[team]->m_units.GetData( unit );
            bool rayHit = RaySphereIntersection( startRay, direction, theUnit->m_centrePos, theUnit->m_radius*1.5 );
            if( rayHit && theUnit->NumAliveEntities() > 0 )
            {
                for( int i = 0; i < theUnit->m_entities.Size(); ++i )
                {
                    if( theUnit->m_entities.ValidIndex(i) )
                    {
                        Entity *entity = theUnit->m_entities[i];
                        Vector3 spherePos = entity->m_pos+entity->m_centrePos;
                        double sphereRadius = entity->m_radius * 1.5;             
                        Vector3 hitPos;

                        bool entityHit = RaySphereIntersection( startRay, direction, spherePos, sphereRadius, 1e10, &hitPos );
                        if( entityHit && !entity->m_dead )
                        {
                            float centrePosX, centrePosY, rayHitX, rayHitY;
                            g_app->m_camera->Get2DScreenPos( spherePos, &centrePosX, &centrePosY );
                            g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );
            
                            double rangeSqd = iv_pow(centrePosX - rayHitX, 2) + iv_pow(centrePosY - rayHitY, 2);
                            if( rangeSqd < closestRangeSqd )
                            {
                                closestRangeSqd = rangeSqd;
                                unitId = unit;
                            }
                        }
                    }
                }
            }
        }
    }

    if( _range && unitId != -1 )
    {
        *_range = iv_sqrt( closestRangeSqd );
    }

    return unitId;
}


WorldObjectId Location::GetEntityId( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, double *_range, bool _ignoreDarwinians )
{
    if( teamId == 255 ) return WorldObjectId();

    double closestRangeSqd = FLT_MAX;
    WorldObjectId entId;

	int numEntities = m_teams[teamId]->m_others.Size();
    for( int i = 0; i < numEntities; ++i )
    {
        if( m_teams[teamId]->m_others.ValidIndex(i) )
        {
            Entity *ent = m_teams[teamId]->m_others.GetData(i);
            if( ent->m_type == Entity::TypeDarwinian && _ignoreDarwinians ) continue;

            if( !ent->m_dead )
            {
			    if(ent->m_type == Entity::TypeOfficer)
			    {
					Vector3 spherePos;
					double sphereRadius;                                   
                    ((Officer *)ent)->CalculateBoundingSphere( spherePos, sphereRadius );

					Vector3 hitPos;

					//RenderSphere( spherePos, sphereRadius );

					bool rayHit = RaySphereIntersection( startRay, direction, spherePos, sphereRadius, 1e10, &hitPos );
					if( rayHit )
					{
						float centrePosX, centrePosY, rayHitX, rayHitY;
						g_app->m_camera->Get2DScreenPos( spherePos, &centrePosX, &centrePosY );
						g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

						double rangeSqd = iv_pow(centrePosX - rayHitX, 2) + iv_pow(centrePosY - rayHitY, 2);
						if( rangeSqd < closestRangeSqd )
						{
							closestRangeSqd = rangeSqd;
							entId = ent->m_id;
						}
					}

					// Note : detecting rayhit with the flag has been removed from this code
                    // This is now handled by the marker system, which has a marker above every officer
                    // on your team					
			    }
                else if( ent->m_type == Entity::TypeArmour )
                {
                    Armour *armour = (Armour *)ent;

                    if( armour->RayHit( startRay, direction ) )
                    {
                        closestRangeSqd = 0.001;
                        entId = ent->m_id;
                    }
                }
			    else
			    {
				    Vector3 spherePos = ent->m_pos+ent->m_centrePos;
				    double sphereRadius = ent->m_radius * 1.0;                                   
				    Vector3 hitPos;

				    bool rayHit = RaySphereIntersection( startRay, direction, spherePos, sphereRadius, 1e10, &hitPos );
				    if( rayHit )
				    {
					    float centrePosX, centrePosY, rayHitX, rayHitY;
					    g_app->m_camera->Get2DScreenPos( spherePos, &centrePosX, &centrePosY );
					    g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

					    double rangeSqd = iv_pow(centrePosX - rayHitX, 2) + pow(centrePosY - rayHitY, 2);
					    if( rangeSqd < closestRangeSqd )
					    {
						    closestRangeSqd = rangeSqd;
						    entId = ent->m_id;
					    }
				    }
			    }
            }
        }
    }

    if( _range && entId.IsValid() )
    {
        *_range = iv_sqrt(closestRangeSqd);
    }

    return entId;
}


int Location::GetBuildingId(Vector3 const &rayStart, Vector3 const &rayDir, unsigned char teamId, double _maxDistance, double *_range )
{    
    double closestRangeSqd = FLT_MAX;
    int buildingId = -1;

    for (int i = 0; i < m_buildings.Size(); i++)
    {
        if (m_buildings.ValidIndex(i))
        {
            Building *building = m_buildings.GetData(i);
            bool teamMatch = ( teamId == 255 ||
                               building->m_id.GetTeamId() == 255 ||
                               teamId == building->m_id.GetTeamId() );

            if( building->m_type != Building::TypeControlTower && teamMatch )
            {
                Vector3 hitPos;
                bool rayHit = false;
                
                if( building->m_type == Building::TypeRadarDish ||
                    building->m_type == Building::TypeGunTurret )
                {
                    // Do exact ray hits with gun turrets/dishes, so they are easy to select
                    rayHit = building->DoesRayHit( rayStart, rayDir );

                    if( rayHit )
                    {
                        RaySphereIntersection( rayStart, rayDir, building->m_centrePos, building->m_radius, _maxDistance, &hitPos );
    
                        if( _range ) *_range = 0.0f;

                        return building->m_id.GetUniqueId();
                    }
                }
                else
                {
                    rayHit = RaySphereIntersection( rayStart, rayDir, building->m_centrePos, building->m_radius * 0.75, _maxDistance, &hitPos );
                }
                
                if( rayHit )
                {
                    float centrePosX, centrePosY, rayHitX, rayHitY;
                    g_app->m_camera->Get2DScreenPos( building->m_centrePos, &centrePosX, &centrePosY );
                    g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

                    double rangeSqd = iv_pow(centrePosX - rayHitX, 2) + iv_pow(centrePosY - rayHitY, 2);
                    if( rangeSqd < closestRangeSqd )
                    {
				        buildingId = building->m_id.GetUniqueId();                    
                        closestRangeSqd = rangeSqd;
                    }
                }
            }
        }
    }

    if( _range )
    {
        *_range = iv_sqrt( closestRangeSqd );
    }

	return buildingId;
}


int Location::ThrowWeapon( Vector3 const &_pos, Vector3 const &_target, int _type, 
                          unsigned char _fromTeamId, double _force, double _lifeTime, bool _explodeOnImpact )
{
    double distance = ( _target - _pos ).Mag();
    double force = iv_sqrt(distance) * 8.0;
    
    int grenadeResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade );
    if( _fromTeamId == 1 && !g_app->Multiplayer() ) grenadeResearch = 4;
    
	double maxForce = ThrowableWeapon::GetMaxForce( grenadeResearch );

    if( force > maxForce ) force = maxForce;
    if( _force != 0.0 ) force = _force;

    Vector3 front = ( _target - _pos ).Normalise();
    front.y = 1.0;
    front.Normalise();

    ThrowableWeapon *weapon = NULL;
    
    switch( _type )
    {
        case WorldObject::EffectThrowableGrenade:
        {
            weapon = new Grenade( _pos, front, force );  
            ((Grenade *)weapon)->m_explodeOnImpact = _explodeOnImpact;
        }
        break;

        case WorldObject::EffectThrowableAirstrikeMarker:     weapon = new AirStrikeMarker( _pos, front, force );     break;
        case WorldObject::EffectThrowableControllerGrenade:
        {
            weapon = new ControllerGrenade( _pos, front, force );   
            Task *task = g_app->m_location->m_teams[ _fromTeamId ]->m_taskManager->GetCurrentTask();
            if( task && task->m_type == GlobalResearch::TypeSquad )
            {
                ControllerGrenade *controller = (ControllerGrenade *)weapon;
                controller->m_squadTaskId = task->m_id;
            }
            break;
        }
    }

    int weaponId = m_effects.PutData( weapon );
    weapon->m_id.Set( _fromTeamId, UNIT_EFFECTS, weaponId, -1 );
    weapon->m_id.GenerateUniqueId();
    weapon->Initialise();

    if( _type == WorldObject::EffectThrowableGrenade && _lifeTime != 0.0 )
    {
        ((Grenade *)weapon)->m_life = _lifeTime;
    }


    //
    // Create muzzle flash

    Vector3 flashFront = front;
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 20.0, 3.0);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _fromTeamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();

    return weaponId;
}


void Location::FireRocket( Vector3 const &_pos, Vector3 const &_target, unsigned char _teamId, double _lifeTime, int _fromBuildingId, bool _firework )
{
    Rocket *r = new Rocket(_pos, _target);
    r->m_fromTeamId = _teamId;
    r->m_fromBuildingId = _fromBuildingId;
	r->m_firework = _firework;

    int weaponId = m_effects.PutData( r );    
    r->m_id.Set( _teamId, UNIT_EFFECTS, weaponId, -1 );
    r->m_id.GenerateUniqueId();
    r->Initialise();

    if( _lifeTime != -1.0 ) r->m_lifeTime = _lifeTime;


    //
    // Create muzzle flash

    Vector3 flashFront = _target - _pos;
    flashFront.Normalise();
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 20.0, 3.0);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


void Location::FireTurretShell( Vector3 const &_pos, Vector3 const &_vel, int _teamId, int _fromBuildingId )
{
    int armourResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeArmour );
    double lifeTime = 0.0;
    switch( armourResearch )
    {
        case 0 :    lifeTime = 0.5;            break;
        case 1 :    lifeTime = 0.5;            break;
        case 2 :    lifeTime = 1.0;            break;
        case 3 :    lifeTime = 1.5;            break;
        case 4 :    lifeTime = 2.0;            break;
    }

    int teamId = _teamId;
    if( g_app->Multiplayer() ) lifeTime = 0.6;
    else teamId = 255;

    TurretShell *shell = new TurretShell(lifeTime);
    shell->m_pos = _pos;
    shell->m_vel = _vel;
    shell->m_fromBuildingId = _fromBuildingId;

    int weaponId = m_effects.PutData( shell );
    shell->m_id.Set( teamId, UNIT_EFFECTS, weaponId, -1 );
    shell->m_id.GenerateUniqueId();


    //
    // Create muzzle flash

    Vector3 flashFront = _vel;
    flashFront.Normalise();
    MuzzleFlash *mf = new MuzzleFlash( _pos + _vel * SERVER_ADVANCE_PERIOD * 0.5, flashFront, 60.0, 2.0);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


void Location::LaunchFireball(const Vector3 &_pos, const Vector3 &_vel, double _life, int _fromBuildingId )
{
    Fireball *fireball = new Fireball();
    fireball->m_pos = _pos;
    fireball->m_vel = _vel;
    fireball->m_life = _life;
    fireball->m_fromBuildingId = _fromBuildingId;

    int weaponId = m_effects.PutData( fireball );
    fireball->m_id.Set( -1, UNIT_EFFECTS, weaponId, -1 );
    fireball->m_id.GenerateUniqueId();
}

void Location::FireLaser( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, bool _mindControl )
{
    int laserResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeLaser );
    double lifetime = 0.0;
    switch( laserResearch )
    {
        case 0 :        lifetime = 0.2;            break;
        case 1 :        lifetime = 0.4;            break;
        case 2 :        lifetime = 0.6;            break;
        case 3 :        lifetime = 0.8;            break;
        case 4 :        lifetime = 1.0;            break;
    }

    if( _mindControl) lifetime *= 2.0;    // subversion lasers are half speed, so they need double life to get the same range
    
    Laser *l = m_lasers.GetPointer();
    l->m_pos = _pos;// + _vel * 0.05;
    l->m_vel = _vel;
    l->m_fromTeamId = _teamId;
    l->m_mindControl = _mindControl;
    l->Initialise(lifetime);

    
    //
    // Create muzzle flash

    Vector3 flashFront = _vel;
    flashFront.Normalise();
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 25.0 * lifetime, 1.5 );
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


int Location::PromoteDarwinian( unsigned char teamId, int _entityId )
{
    Entity *entity = g_app->m_location->m_teams[teamId]->m_others[_entityId];
    if( !entity ) return -1;

    if( ((Darwinian *)entity)->IsOnFire() ) return -1;

    //
    // Spawn an Officer

    WorldObjectId spawnedId = g_app->m_location->SpawnEntities( entity->m_pos, g_app->IsSinglePlayer() ? g_app->m_globalWorld->m_myTeamId : teamId, -1, Entity::TypeOfficer, 1, entity->m_vel, 0 );
    Officer *officer = (Officer *) g_app->m_location->GetEntity( spawnedId );
    AppDebugAssert( officer );


    //
    // Particle effect

    int numFlashes = 5 + AppRandom() % 5;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(5.0), frand(15.0), sfrand(5.0) );
        g_app->m_particleSystem->CreateParticle( entity->m_pos, vel, Particle::TypeControlFlash );
    }


    Darwinian *darwinian = (Darwinian *) entity;
    darwinian->m_promoted = true;    

    //
    // Marker

    if( teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_markerSystem->RegisterMarker_Officer( spawnedId );
    }

    return spawnedId.GetIndex();
}


int Location::DemoteOfficer( unsigned char teamId, int _entityId )
{
    Entity *entity = g_app->m_location->m_teams[teamId]->m_others[_entityId];
    if( !entity ) return -1;
    if( entity->m_type != Entity::TypeOfficer ) return -1;

    ((Officer *)entity)->m_demoted = true;



    //
    // Spawn a Darwinian

    WorldObjectId spawnedId = g_app->m_location->SpawnEntities( entity->m_pos, teamId, -1, Entity::TypeDarwinian, 1, entity->m_vel, 0 );


    //
    // Particle effect

    int numFlashes = 5 + AppRandom() % 5;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(5.0), frand(15.0), sfrand(5.0) );
        g_app->m_particleSystem->CreateParticle( entity->m_pos, vel, Particle::TypeControlFlash );
    }


    return spawnedId.GetIndex();
}


Team *Location::GetMyTeam()
{
    if (g_app->m_globalWorld->m_myTeamId == 255)
    {
        return NULL;
    }
    return m_teams[g_app->m_globalWorld->m_myTeamId];
}


TaskManager *Location::GetMyTaskManager()
{
    if (g_app->m_globalWorld->m_myTeamId == 255)
    {
        return NULL;
    }
    return m_teams[g_app->m_globalWorld->m_myTeamId]->m_taskManager;
}


// ****************************************************************************
// Private Methods
// ****************************************************************************


int Location::Bang( Vector3 const &_pos, double _range, double _damage, int _teamId, int _fromBuildingId )
{
    int numCores = _range * _damage * 0.01f;
    numCores += frand(numCores);
    for( int p = 0; p < numCores; ++p )
    {
        Vector3 vel( sfrand( 30.0 ), 
                     10.0 + frand( 10.0 ), 
                     sfrand( 30.0 ) );
        double size = 120.0 + frand(120.0);
        g_app->m_particleSystem->CreateParticle( _pos + g_upVector * _range * 0.3, vel, 
    											 Particle::TypeExplosionCore, size );
    }

    int numDebris = max(2.0, _range * _damage * 0.01);
    numDebris = min( numDebris, 30 );
    for( int p = 0; p < numDebris; ++p )
    {
        double angle = 2.0 * M_PI * p/(double)numDebris;

        Vector3 vel(1,0,0);
        vel.RotateAroundY( angle );
        vel.y = 2.0 + frand(2.0);
        vel.SetLength( 20.0 + frand(30.0) );

        double size = 20.0 + frand(20.0);

        g_app->m_particleSystem->CreateParticle( _pos + g_upVector * _range * 0.2, vel, 
                                                 Particle::TypeExplosionDebris, size );
    }


    //
    // Damage everyone nearby

    int numFound;

    // Possibly recursive, use local std::vector<WorldObjectId> if static std::vector<WorldObjectId> already in use
    std::vector<WorldObjectId> l_neighbours;
    std::vector<WorldObjectId> *ids = &l_neighbours;
    bool useStaticNeighbours = false;
    if( !s_neighboursInUse )
    {
        s_neighboursInUse = true;
        useStaticNeighbours = true;
        ids = &s_neighbours;
    }
    g_app->m_location->m_entityGrid->GetNeighbours( *ids, _pos.x, _pos.z, _range*2.0, &numFound );

    int numKilled = 0;
    int darwinianKills = 0;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = (*ids)[i];
        WorldObject *obj = g_app->m_location->GetEntity( id );
        Entity *entity = (Entity *) obj;

        double distance = (entity->m_pos - _pos).Mag();
        double fraction = (_range*2.0 - distance) / _range*2.0;
        //fraction *= (1.0 + syncfrand(0.3));
        //fraction *= 1.5;
        fraction = min( 1.0, fraction );
        fraction = max( 0.0, fraction );

        bool dead = entity->m_dead;

        bool success = entity->ChangeHealth( _damage * fraction * -1.0, Entity::DamageTypeExplosive );
        
        if( success )
        {
            Vector3 push( entity->m_pos - _pos );
            push.Normalise();

            if( entity->m_onGround )
            {
                push.y = _damage;
            }
            else
            {
                push.y = -1;
            }

            push.Normalise();
            push *= fraction;     

            if( entity->m_type != Entity::TypeInsertionSquadie &&
                entity->m_type != Entity::TypeOfficer &&
                entity->m_type != Entity::TypeShaman ) 
            {
                entity->m_vel += push;
                entity->m_onGround = false;
            }
        }

        if( !dead && entity->m_dead && entity->m_id.GetTeamId() != _teamId )
        {
            numKilled++;
            if( entity->m_type == Entity::TypeDarwinian )
            {
                darwinianKills++;
            }
        }
    }

    if( useStaticNeighbours )
    {
        s_neighboursInUse = false;
    }


	//
	// Is visible?

	//Vector3 tmp;
	bool isVisible = true;
	// This test is reported to often fail on explosions from rockets.
	// To be fixed later...could be caused for example by rockets exploding bit below ground.
	// !m_landscape.RayHit(g_app->m_camera->GetPos(),_pos-g_app->m_camera->GetPos(),&tmp)
	//	|| (tmp-g_app->m_camera->GetPos()).Mag()>(_pos-g_app->m_camera->GetPos()).Mag()-0.3;

    //
    // Shockwave

    if(isVisible) CreateShockwave( _pos, _range/20.0, _teamId, _fromBuildingId );

	//
	// Punch effect

#ifdef USE_DIRECT3D
	if(g_deformEffect && isVisible) g_deformEffect->AddPunch( _pos, _range );
#endif
    
    
    // 
    // Wow, that was a big bang. Maybe we killed a building

    double maxBuildingRange = _range * 3.0;
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        if( m_buildings.ValidIndex(i) )
        {	    
    		Building *building = m_buildings[i];
    		double dist = (_pos - building->m_pos).Mag();
            
            if( dist < maxBuildingRange )
            {
                //double fraction = (_range*3.0 - dist) / _range*3.0;
                double fraction = 1.0 - dist / maxBuildingRange;
                fraction = max( 0.0, fraction );
                fraction = min( 1.0, fraction );
                building->Damage( _damage * fraction * -1.0 );
            }
        }
    }

    if( _teamId != -1 && _teamId != 255)
    {
		g_app->m_location->m_teams[_teamId]->m_teamKills += g_app->IsSinglePlayer() ? numKilled : darwinianKills;
    }

    return numKilled;
}


void Location::CreateShockwave( Vector3 const &_pos, double _size, unsigned char _teamId, int _fromBuildingId )
{
    Shockwave *s = new Shockwave( _teamId, _size );
    s->m_pos = _pos;
    s->m_fromBuildingId = _fromBuildingId;
    int index = m_effects.PutData( s );
    s->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    s->m_id.GenerateUniqueId();
}


void Location::SetupFog()
{
    float fogCol[] = {  g_app->m_backgroundColour.r/255.0, 
                        g_app->m_backgroundColour.g/255.0, 
                        g_app->m_backgroundColour.b/255.0, 
                        0 };
    
    float fogEnd = 4000;
    if( m_landscape.GetWorldSizeX() + 1000 > fogEnd ) fogEnd = m_landscape.GetWorldSizeX() + 1000;
    if( m_landscape.GetWorldSizeZ() + 1000 > fogEnd ) fogEnd = m_landscape.GetWorldSizeZ() + 1000;

	glHint		( GL_FOG_HINT, GL_DONT_CARE );
    glFogf      ( GL_FOG_DENSITY, 1.0 );
    glFogf      ( GL_FOG_START, fogEnd/3.0 );
    glFogf      ( GL_FOG_END, fogEnd );
    glFogfv     ( GL_FOG_COLOR, fogCol );
    glFogi      ( GL_FOG_MODE, GL_LINEAR );
    glDisable   ( GL_FOG );
}

void Location::WaterReflect()
{
	for (int i = 0; i < m_lights.Size(); i++)
	{
		Light *light = m_lights.GetData(i);
		//light->m_front[0] = -light->m_front[0];
		//light->m_front[1] = -light->m_front[1];
		//light->m_front[2] = -light->m_front[2];
		//light->m_front[3] = -light->m_front[3];
	}
}

void Location::SetupLights()
{
	float tmp[] = { 0, 0, -1, 0 };
	glLightfv(GL_LIGHT1, GL_POSITION, tmp);
	float black[] = { 0, 0, 0, 0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, black);
	glLightfv(GL_LIGHT1, GL_SPECULAR, black);
	glLightfv(GL_LIGHT1, GL_AMBIENT, black);

	for (int i = 0; i < m_lights.Size(); i++)
	{
		Light *light = m_lights.GetData(i);
	    
		GLfloat ambCol[] = { 0.0, 0.0, 0.0, 1.0 };
		
		Vector3 front(light->m_front[0], light->m_front[1], light->m_front[2]);
        front.Normalise();
		GLfloat frontAsFourFloats[] = { front.x, front.y, front.z, 0.0 };
        GLfloat colourAsFourFloats[] = { light->m_colour[0], light->m_colour[1], light->m_colour[2], light->m_colour[3] };

        if( ChristmasModEnabled() == 1 )
        {
            colourAsFourFloats[0] = 1.3;
            colourAsFourFloats[1] = 1.2;
            colourAsFourFloats[2] = 1.2;
        }


		glLightfv(GL_LIGHT0 + i, GL_POSITION, frontAsFourFloats);
		glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, colourAsFourFloats);
		glLightfv(GL_LIGHT0 + i, GL_SPECULAR, colourAsFourFloats);
		glLightfv(GL_LIGHT0 + i, GL_AMBIENT, ambCol);
	}

	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHT1);
	glDisable(GL_LIGHTING);
}


void Location::SetupSpecialLights()
{
    float spec = 1.0;
    float diffuse = 1.0;
    float amb = 0.0;
    GLfloat materialShininess[] = { 10.0 };
    GLfloat materialSpecular[] = { spec, spec, spec, 1.0 };
    GLfloat materialDiffuse[] = { diffuse, diffuse, diffuse, 1.0 };
    GLfloat ambCol[] = { amb, amb, amb, 1.0 };

    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
    glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

    float black[] = { 0, 0, 0, 0 };
    float colour1[] = { 2.0, 1.5, 0.75, 1.0 };

    Vector3 light0(0, 1, 0.5);
    light0.Normalise();
    GLfloat light0AsFourFloats[] = { light0.x, light0.y, light0.z, 0.0 };

    glLightfv(GL_LIGHT0, GL_POSITION, light0AsFourFloats);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, colour1);
    glLightfv(GL_LIGHT0, GL_SPECULAR, colour1);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black);

    glEnable        (GL_LIGHTING);
    glEnable        (GL_LIGHT0);
    glDisable       (GL_LIGHT1);
}


bool Location::ChristmasModEnabled()
{    
    return m_christmasMode;
}


bool Location::IsFriend( unsigned char _teamId1, unsigned char _teamId2 )
{
    if( _teamId1 == 255 || _teamId2 == 255 ) return false;

	if( g_app->Multiplayer() )
    {
        if( g_app->m_multiwinia->m_coopMode )
        {
            return ( m_coopGroups[_teamId1] == m_coopGroups[_teamId2] );
        }
		return( _teamId1 == _teamId2 );
    }


	/* **** IMPORTANT ****
	 * The following 2D bool array must be updated so that enemies in single player do not break.
	 * Merely add an extra column and row full of falses. Thanks! Leander
	 */

                                        /*  green   red     player  player  n/a		n/a		*/

    bool friends[NUM_TEAMS][NUM_TEAMS] = {  
                                            true,   false,  true,   false,	false,	false,	// Green
                                            false,  true,   false,  false,	false,	false,	// Red
                                            true,   false,  true,   false,	false,	false,	// Player
                                            true,   false,  false,  true,	false,	false,	// Player
											false,	false,	false,	false,	false,	false,	// Nothing in single player
											false,	false,	false,	false,	false,	false	// Nothing in single player
                                         };

    return friends[ _teamId1 ][ _teamId2 ];
}


void Location::FlushOpenGlState()
{
	int treeTypeId = Building::GetTypeId("Tree");
	for (int i = 0; i < m_buildings.Size(); ++i)
	{
		if (m_buildings.ValidIndex(i))
		{
			Building *building = m_buildings[i];
			if (building->m_type == treeTypeId)
			{
				Tree *tree = (Tree*)building;
				tree->DeleteDisplayLists();
			}
		}
	}
}


void Location::RegenerateOpenGlState()
{
	// Tell the landscape
	g_app->m_location->m_landscape.BuildOpenGlState();

	// Tell the water
	g_app->m_location->m_water->BuildOpenGlState();
}

void Location::RecalculateAITargets()
{
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_type == Building::TypeAITarget &&
                !building->m_destroyed )
            {
                AITarget *aiTarget = (AITarget *) building;
                aiTarget->RecalculateNeighbours();
            }
        }
    }

    //
    // Cull nonsense links
    // eg if link A -> B exists, and link B -> C exists, then don't allow
    // link A -> C unless it is much shorter distance    

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_type == Building::TypeAITarget )
            {
                AITarget *a = (AITarget *) building;
                if( a->m_turretTarget ) continue;
                for( int n = 0; n < a->m_neighbours.Size(); ++n )
                {
                    //int cId = a->m_neighbours[n];
                    NeighbourInfo *info = a->m_neighbours[n];
                    if( info->m_neighbourId == a->GetBuildingLink() ) continue; // never cull manual links

                    AITarget *c = (AITarget *) g_app->m_location->GetBuilding(info->m_neighbourId);
                    AppDebugAssert( c && c->m_type == Building::TypeAITarget );
                    if( c->m_turretTarget ) continue; // dont cull for gunturrets
                    double distanceAtoC = a->IsNearTo( info->m_neighbourId );

                    for( int x = 0; x < a->m_neighbours.Size(); ++x )
                    {
                        if( x != n )
                        {
                            NeighbourInfo *info2 = a->m_neighbours[x];
                            AITarget *b = (AITarget *) g_app->m_location->GetBuilding( info2->m_neighbourId );
                            AppDebugAssert( b && b->m_type == Building::TypeAITarget );
                            double distanceAtoB = a->IsNearTo( info2->m_neighbourId );
                            double distanceBtoC = b->IsNearTo( info->m_neighbourId );
                            if( distanceBtoC > 0.0 && 
                                distanceAtoC > (distanceAtoB + distanceBtoC) * 0.8 )
                            {
                                delete info;
                                a->m_neighbours.RemoveData(n);
                                --n;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
}

void Location::AddBurnPatch( const Vector3 &_pos, double _radius )
{
    BurnPatch patch;
    patch.m_pos = _pos;
    patch.m_radius = _radius;
    g_app->m_location->m_burnPatches.PutData( patch );
    g_app->m_location->m_landscape.BuildOpenGlState();
}

void Location::NewChatMessage( UnicodeString _msg, int _fromTeamId )
{
    //if( _fromTeamId < 0 || _fromTeamId > 3 ) return;

    ChatMessage *message = new ChatMessage;
    message->m_msg = _msg;
    message->m_clientId = _fromTeamId;
    message->m_recievedTime = GetHighResTime();
    m_chatMessages.PutDataAtStart( message );

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_teams[i]->m_clientId == _fromTeamId )
        {
            message->m_senderName = m_teams[i]->m_lobbyTeam->m_teamName;
            message->m_colour = m_teams[i]->m_colour;
            break;
        }
    }

    g_app->m_soundSystem->TriggerOtherEvent( NULL, "ChatMessage", SoundSourceBlueprint::TypeMultiwiniaInterface );
}


void Location::SetCurrentMessage(const UnicodeString &_message, int _teamId, bool _noOverwrite )
{
    if( m_messageNoOverwrite ) return;

    m_message = _message;
    m_messageTeam = _teamId;
    m_messageTimer = GetHighResTime() + 3.0;
    m_messageNoOverwrite = _noOverwrite;
}

bool Location::ViablePosition( Vector3 const &_pos )
{
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        if( m_buildings.ValidIndex( i ) )
        {
            Building *b = m_buildings[i];
            if( b->m_type == Building::TypeAITarget )
            {
                if( IsWalkable( _pos, b->m_pos, false, true ) )
                {
                    return true;
                }
            }
        }
    }
    return false;
}

void Location::DumpWorldToSyncLog( const char *_filename )
{
    FILE *file = fopen( _filename, "a" );
    if( !file ) return;

    fprintf( file, "\n\nBegin Final Object Positions\n" );

    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        Team *team = m_teams[t];
        if( team->m_teamType != TeamTypeUnused )
        {
            for( int u = 0; u < team->m_units.Size(); ++u )
            {
                if( team->m_units.ValidIndex(u) )
                {
                    Unit *unit = team->m_units[u];                    
                    for( int e = 0; e < unit->m_entities.Size(); ++e )
                    {
                        if( unit->m_entities.ValidIndex(e) )
                        {
                            Entity *ent = unit->m_entities[e];
                            fprintf( file, "%s\n", ent->LogState() );
                        }
                    }
                }
            }

            for( int e = 0; e < team->m_others.Size(); ++e )
            {
                if( team->m_others.ValidIndex(e) )
                {
                    Entity *entity = team->m_others[e];
                    fprintf( file, "%s\n", entity->LogState() );
                }
            }
        }
    }


    for( int e = 0; e < m_effects.Size(); ++e )
    {
        if( m_effects.ValidIndex(e) )
        {
            WorldObject *wobj = m_effects[e];
			if( wobj->m_type != WorldObject::EffectSnow )
				fprintf( file, "%s\n", wobj->LogState() );
        }
    }


	for( int b = 0; b < m_buildings.Size(); ++b )
	{
		if( m_buildings.ValidIndex(b) )
		{
            WorldObject *wobj = m_buildings[b];
            fprintf( file, "%s\n", wobj->LogState() );
		}
	}

    fclose(file);
}


void Location::SetLastProcessedSlice( int _slice )
{
    m_lastSliceProcessed = _slice;
}

int Location::GetLastProcessedSlice()
{
    return m_lastSliceProcessed;
}

#ifdef TRACK_SYNC_RAND
#include <map>
#include <string>

typedef std::map<Entity *, std::string> EntityStateMap;

EntityStateMap s_trackedEntities;

void TrackEntity( Entity *_entity )
{
	//if( _entity->m_id.GetUniqueId() == 3045 )
	//	s_trackedEntities[ _entity ] = "";
}

void UntrackEntity( Entity *_entity )
{
	EntityStateMap::iterator i = s_trackedEntities.find( _entity );

	if( i != s_trackedEntities.end() )
	{
		SyncRandLog( "Watch Deleted: %s\n", i->second.c_str() );
		s_trackedEntities.erase( i );
	}
}

bool WatchEntities()
{
	bool result = false;

	for( EntityStateMap::iterator i = s_trackedEntities.begin(); i != s_trackedEntities.end(); i++ )
	{
		Entity *e = i->first;
			
		std::string curState = e->LogState();
		std::string &oldState = i->second;

		if( oldState != curState )
		{
			SyncRandLog( "Watch Changed From: %s", oldState.c_str() );
			SyncRandLog( "Watch Changed   To: %s", curState.c_str() );
			oldState = curState;
			result = true;
		}
	}

	return result;
}
#endif
