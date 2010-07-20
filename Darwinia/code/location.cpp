#include "lib/universal_include.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <float.h>

#ifdef USE_DIRECT3D
#include "lib/opengl_directx_internals.h"
#endif
#include "lib/bounded_array.h"
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
#include "interface/prefs_other_window.h"

#include "network/servertoclientletter.h"
#include "network/clienttoserver.h"

#include "app.h"
#include "camera.h"
#include "clouds.h"
#include "deform.h"
#include "entity_grid.h"
#include "global_world.h"
#include "landscape.h"
#include "level_file.h"
#include "location.h"
#include "helpsystem.h"
#include "main.h"
#include "obstruction_grid.h"
#include "particle_system.h"
#include "sepulveda.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "user_input.h"
#include "water.h"
#include "gamecursor.h"
#include "script.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"

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

#include "sound/soundsystem.h"

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
	m_teams(NULL),
    m_christmasTimer(-99.9f)
{
    m_spirits.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_lasers.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_effects.SetTotalNumSlices(NUM_SLICES_PER_FRAME);
    m_buildings.SetTotalNumSlices(NUM_SLICES_PER_FRAME);

    m_spirits.SetSize( 100 );

	m_lights.SetStepSize(1);
	m_buildings.SetStepSize(10);
    m_spirits.SetStepSize( 100 );
	m_lasers.SetStepSize(100);
	m_effects.SetSize(100);
}

// *** Destructor
Location::~Location()
{
	Empty();
}


void Location::Init( char const *_missionFilename, char const *_mapFilename )
{
    LoadLevel(_missionFilename, _mapFilename);
    darwiniaSeedRandom( 1 );

    InitLights();
    InitLandscape();

    m_water = new Water();

    if( !g_app->m_editing )
    {
        InitBuildings();

        m_entityGrid        = new EntityGrid( 8.0f, 8.0f );
        m_obstructionGrid   = new ObstructionGrid( 64.0f, 64.0f );
        m_clouds            = new Clouds();
    }
    else
    {
	    for (int i = 0; i < m_levelFile->m_buildings.Size(); i++)
	    {
		    Building *building = m_levelFile->m_buildings.GetData(i);
            building->m_pos.y = m_landscape.m_heightMap->GetValue( building->m_pos.x, building->m_pos.z );
        }
    }

	InitTeams();

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
}


void Location::Empty()
{
	m_landscape.Empty();

	m_lights.Empty();			// LList <Light *>
	m_buildings.Empty();		// LList <Building *>
	m_spirits.Empty();			// FastDArray <Spirit>
    m_lasers.Empty();
    m_effects.Empty();

	delete m_levelFile;			m_levelFile = NULL;
	delete [] m_teams;			m_teams = NULL;
	delete m_entityGrid;		m_entityGrid = NULL;
	delete m_obstructionGrid;	m_obstructionGrid = NULL;
	delete m_clouds;			m_clouds = NULL;
	delete m_water;				m_water = NULL;
}


void Location::LoadLevel(char const *missionFilename, char const *mapFilename)
{
	m_levelFile = new LevelFile(missionFilename, mapFilename);

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
	for (int i = 0; i < m_levelFile->m_buildings.Size(); i++)
	{
		Building *building = m_levelFile->m_buildings.GetData(i);
        Building *existing = g_app->m_location->GetBuilding(building->m_id.GetUniqueId());
        if( existing )
        {
            DarwiniaReleaseAssert( false,
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
        newBuilding->Initialise( building );
        newBuilding->SetDetail( g_prefsManager->GetInt( "RenderBuildingDetail", 1 ) );
	}
}


void Location::InitTeams()
{
	m_teams = new Team[NUM_TEAMS];
	for ( int t = 0; t < NUM_TEAMS; t += 1 )
	{
		m_teams[t].m_colour = m_levelFile->m_teamColours[t];
	}

}


// *** SpawnEntities
// Returns id of last entity spawned
// Only useful if we've only spawned one entity, eg an engineer
WorldObjectId Location::SpawnEntities( Vector3 const &_pos, unsigned char _teamId, int _unitId,
                                       unsigned char _type, int _numEntities, Vector3 const &_vel,
                                       float _spread, float _range, int _routeId, int _routeWaypointId )
{
    DarwiniaDebugAssert( _teamId < NUM_TEAMS &&
                m_teams[_teamId].m_teamType > Team::TeamTypeUnused );

    Team *team = &m_teams[_teamId];
    WorldObjectId entityId;

    for (int i = 0; i < _numEntities; i++)
    {
        int unitIndex;
        Entity *s = team->NewEntity( _type, _unitId, &unitIndex );
        DarwiniaDebugAssert( s );

        s->SetType( _type );
        s->m_pos         = FindValidSpawnPosition( _pos, _spread );
        s->m_onGround    = false;
        s->m_vel         = _vel;
        if (s->m_vel.MagSquared() > 0.0f)
		{
			s->m_front       = _vel;
			s->m_front.Normalise();
		}
		else
		{
			s->m_front.Set(1,0,0);
		}
        s->m_id.SetTeamId(_teamId);
        s->m_id.SetUnitId(_unitId);
        s->m_id.SetIndex(unitIndex);
        s->m_spawnPoint = _pos;
        s->m_roamRange = _range;
		s->m_routeId = _routeId;
        s->m_routeWayPointId = _routeWaypointId;
        if( _range == -1.0f ) s->m_roamRange = _spread;
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


Vector3 Location::FindValidSpawnPosition( Vector3 const &_pos, float _spread )
{
    int tries = 10;

    while( tries > 0 )
    {
        Vector3 randomPos = _pos;

        float radius = syncfrand(_spread);
	    float theta = syncfrand(M_PI * 2);
    	randomPos.x += radius * sinf(theta);
	    randomPos.z += radius * cosf(theta);

        bool onMap = randomPos.x >= 0 && randomPos.z >= 0 &&
                     randomPos.x < m_landscape.GetWorldSizeX() &&
                     randomPos.z < m_landscape.GetWorldSizeZ();

        if( onMap )
        {
            float landHeight = m_landscape.m_heightMap->GetValue( randomPos.x, randomPos.z );

            if( landHeight > 0.0f )
            {
                return randomPos;
            }
        }

        tries--;
    }

    //
    // Failed to find a valid pos

    Vector3 pos = _pos;
    pos.x = max( pos.x, 20 );
    pos.z = max( pos.z, 20 );
    pos.x = min( pos.x, m_landscape.GetWorldSizeX()-20 );
    pos.z = min( pos.z, m_landscape.GetWorldSizeZ()-20 );

    return pos;
}


int Location::SpawnSpirit( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId, WorldObjectId _id )
{
    DarwiniaDebugAssert( _teamId < NUM_TEAMS );

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
        m_teams[teamId].m_teamType == Team::TeamTypeUnused )
    {
        return NULL;
    }

    if( m_teams[teamId].m_units.ValidIndex(unitId) )
    {
        Unit *unit = m_teams[teamId].m_units[unitId];
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
        if( m_teams[teamId].m_others.ValidIndex(index) )
        {
            Entity *entity = m_teams[teamId].m_others[index];
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
		Entity *result = m_teams[i].RayHitEntity(_rayStart, _rayDir);
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
        m_teams[teamId].m_teamType == Team::TeamTypeUnused )
    {
        return NULL;
    }

    if( m_teams[teamId].m_units.ValidIndex(unitId) )
    {
        Unit *unit = m_teams[teamId].m_units[unitId];
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


bool Location::IsVisible( Vector3 const &_from, Vector3 const &_to )
{
    Vector3 rayDir = (_to - _from).Normalise();
    float tolerance = 20.0f;
    Vector3 startPos = _from + rayDir * tolerance;

    Vector3 hitPos;
    bool landHit = g_app->m_location->m_landscape.RayHit( startPos, rayDir, &hitPos );

    if( !landHit ) return true;

    float distanceToTarget = ( _to - _from ).Mag();
    float distanceToHit = ( hitPos - _from ).Mag();
    if( distanceToHit > distanceToTarget + tolerance ) return true;

    return false;
}


bool Location::IsWalkable( Vector3 const &_from, Vector3 const &_to, bool _evaluateCliffs )
{
    float waterLevel = -1.0f;

    if( _from.y <= waterLevel || _to.y <= waterLevel )
    {
        return false;
    }

    START_PROFILE( g_app->m_profiler, "QueryWalkable" );

    float stepSize = 50.0f;
    float totalDistance = ( _from - _to ).Mag();
    int numSteps = totalDistance / stepSize;
    Vector3 diff = ( _to - _from ) / (float)numSteps;
    float distanceUnderWater = 0.0f;

    Vector3 position = _from;
    Vector3 oldPosition = _from;
    for( int i = 0; i < numSteps; ++i )
    {
        position.y = m_landscape.m_heightMap->GetValue(position.x, position.z);
        if( position.y <= waterLevel )
        {
            distanceUnderWater += stepSize;
        }

        if( distanceUnderWater >= 100.0f )
        {
            END_PROFILE( g_app->m_profiler, "QueryWalkable" );
            return false;
        }

        if( _evaluateCliffs )
        {
            float gradient = ( position.y - oldPosition.y ) / stepSize;
            if( gradient > 2.3f )
            {
                END_PROFILE( g_app->m_profiler, "QueryWalkable" );
                return false;
            }
        }

        position += diff;
    }

    END_PROFILE( g_app->m_profiler, "QueryWalkable" );

    return true;
}


void Location::AdvanceWeapons( int _slice )
{
    START_PROFILE(g_app->m_profiler, "Advance Lasers");
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
                m_lasers.MarkNotUsed(i);
            }
        }
    }
    END_PROFILE(g_app->m_profiler, "Advance Lasers");


    START_PROFILE(g_app->m_profiler, "Advance Effects");
    m_effects.GetNextSliceBounds(_slice, &startIndex, &endIndex);
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_effects.ValidIndex(i) )
        {
            WorldObject *e = m_effects[i];
            bool remove = e->Advance();
            if( remove )
            {
                m_effects.MarkNotUsed(i);
                delete e;
            }
        }
    }
    END_PROFILE(g_app->m_profiler, "Advance Effects");
}


// *** AdvanceBuildings
void Location::AdvanceBuildings( int _slice )
{
    START_PROFILE(g_app->m_profiler, "Advance Buildings");
    bool obstructionGridChanged = false;

    int startIndex, endIndex;
    m_buildings.GetNextSliceBounds( _slice, &startIndex, &endIndex );
    for( int i = startIndex; i <= endIndex; ++i )
    {
        if( m_buildings.ValidIndex(i) )
        {
            Building *building = m_buildings.GetData(i);

            START_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
            bool removeBuilding = building->Advance();
            END_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );

            if( removeBuilding )
            {
                m_buildings.MarkNotUsed(i);
                obstructionGridChanged = true;
            }
        }
    }

    if( obstructionGridChanged )
    {
        // TODO: This is WAY too slow, should only recalculate the areas affected
        g_app->m_location->m_obstructionGrid->CalculateAll();
    }

    END_PROFILE(g_app->m_profiler, "Advance Buildings");
}
/*
void Location::AdvanceBuildings( int _slice )
{
    if( _slice == 5 )
    {
        START_PROFILE(g_app->m_profiler, "Advance Buildings");
        bool obstructionGridChanged = false;

        for( int i = 0; i < m_buildings.Size(); ++i )
        {
            if( m_buildings.ValidIndex(i) )
            {
                Building *building = m_buildings.GetData(i);

                START_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
                bool removeBuilding = building->Advance();
                END_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );

                if( removeBuilding )
                {
                    m_buildings.MarkNotUsed(i);
                    obstructionGridChanged = true;
                }
            }
        }

        if( obstructionGridChanged )
        {
            // TODO: This is WAY too slow, should only recalculate the areas affected
            g_app->m_location->m_obstructionGrid->CalculateAll();
        }

        END_PROFILE(g_app->m_profiler, "Advance Buildings");
    }
}*/



// *** AdvanceTeams
void Location::AdvanceTeams( int _slice )
{
    for (int i = 0; i < NUM_TEAMS; i++)
    {
        m_teams[i].Advance(_slice);
    }
}


// *** AdvanceSpirits
void Location::AdvanceSpirits( int _slice )
{
    START_PROFILE(g_app->m_profiler, "Advance Spirits");

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
                m_spirits.MarkNotUsed(i);
            }
        }
    }

    END_PROFILE(g_app->m_profiler, "Advance Spirits");
}


// *** AdvanceClouds
void Location::AdvanceClouds( int _slice )
{
    if( _slice == 3 )
    {
        START_PROFILE(g_app->m_profiler, "Advance Clouds");
        m_clouds->Advance();
        END_PROFILE(g_app->m_profiler, "Advance Clouds");
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

    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageObjectivesComplete, -1, 5.0f );


//    if( !g_app->m_camera->IsInteractive() ||
//        g_app->m_script->IsRunningScript() )
//    {
//        return;
//    }

    //
	// Get Sepulveda to tell you that the level is complete

    g_app->m_sepulveda->Say("PrimaryObjectivesComplete");
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
    if( g_app->m_paused ) return;

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


    if( ChristmasModEnabled() == 1 )
    {
        AdvanceChristmas();
    }
}

void Location::AdvanceChristmas()
{
    if( m_christmasTimer < -90.0f )
    {
        for( int i = 0; i < 150; ++i )
        {
            float sizeX = m_landscape.GetWorldSizeX();
            float sizeZ = m_landscape.GetWorldSizeZ();
            float posY = syncfrand(1000.0f);
            Vector3 spawnPos = Vector3( syncfrand(sizeX), posY, syncfrand(sizeZ) ) ;
            Snow *snow = new Snow();
            snow->m_pos = spawnPos;
            int index = m_effects.PutData( snow );
            snow->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
            snow->m_id.GenerateUniqueId();
        }
    }

    m_christmasTimer -= SERVER_ADVANCE_PERIOD;
    if( m_christmasTimer <= 0.0f )
    {
        m_christmasTimer = 0.7f;
        float sizeX = m_landscape.GetWorldSizeX();
        float sizeZ = m_landscape.GetWorldSizeZ();
        float posY = 700.0f + syncfrand(300.0f);
        Vector3 spawnPos = Vector3( syncfrand(sizeX), posY, syncfrand(sizeZ) ) ;
        Snow *snow = new Snow();
        snow->m_pos = spawnPos;
        int index = m_effects.PutData( snow );
        snow->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
        snow->m_id.GenerateUniqueId();
    }
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
	CHECK_OPENGL_STATE();

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        Team *team = &m_teams[i];
        team->Render();
    }

	CHECK_OPENGL_STATE();
}


// *** Render Spirits
void Location::RenderSpirits()
{
    START_PROFILE(g_app->m_profiler, "Render Spirits");

    glDisable       ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_BLEND );
    glDepthMask     ( false );

    float timeSinceAdvance = g_predictionTime;
    float numPerSlice = m_spirits.Size() / (float)NUM_SLICES_PER_FRAME;

    for( int i = 0; i < m_spirits.Size(); ++i )
    {
        if( m_spirits.ValidIndex(i) )
        {
            Spirit *r = m_spirits.GetPointer(i);

            if( i > m_spirits.GetLastUpdated() )
            {
                r->Render( timeSinceAdvance + 0.1f );
            }
            else
            {
                r->Render( timeSinceAdvance );
            }
        }
    }

    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_CULL_FACE );

    END_PROFILE(g_app->m_profiler, "Render Spirits");
}


// *** RenderWater
void Location::RenderWater()
{
    if( m_water ) m_water->Render();
}


// *** Render
void Location::Render(bool renderWaterAndClouds)
{
    //
    // Render all solid objects

    if( renderWaterAndClouds ) RenderClouds();
		else {glBegin( GL_QUADS ); for(unsigned i=0;i<4;i++) glVertex2f(0,0); glEnd();} // necessary for correct reflection. why???
	CHECK_OPENGL_STATE();
    RenderLandscape();
	CHECK_OPENGL_STATE();
    if( renderWaterAndClouds ) RenderWater();
#ifdef USE_DIRECT3D
		else glDisable(GL_CLIP_PLANE2);
#endif
	CHECK_OPENGL_STATE();

	// don't reflect buildings, teams etc.
	//  makes nearly no difference on GPU bound setups (22 -> 23fps on athlon_x2_3600 + x300)
	//  makes it considerably faster on CPU bound setups (60 -> 93fps on athlon_x2_3600 + 8800gts)
	//if( !renderWaterAndClouds ) return;

    RenderBuildings();
	CHECK_OPENGL_STATE();

    if (!g_app->m_editing && m_teams)
	{
		RenderTeams();
		CHECK_OPENGL_STATE();
	}

    //if( m_entityGrid ) m_entityGrid->Render();
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


	CHECK_OPENGL_STATE();
}


// *** Render Buildings
void Location::RenderBuildings()
{
    START_PROFILE(g_app->m_profiler, "Render Buildings");
    float timeSinceAdvance = g_predictionTime;

    SetupFog        ();
    glEnable        (GL_FOG);
	g_app->m_renderer->SetObjectLighting();
#ifdef USE_DIRECT3D
	OpenGLD3D::g_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, TRUE );
#endif


    //
    // Special lighting mode used for Demo2

    if( g_prefsManager->GetInt( "RenderSpecialLighting" ) == 1 )
    {
	    float spec = 1.0f;
	    float diffuse = 1.0f;
        float amb = 0.0f;
	    GLfloat materialShininess[] = { 10.0f };
	    GLfloat materialSpecular[] = { spec, spec, spec, 1.0f };
   	    GLfloat materialDiffuse[] = { diffuse, diffuse, diffuse, 1.0f };
	    GLfloat ambCol[] = { amb, amb, amb, 1.0f };

	    glMaterialfv(GL_FRONT, GL_SPECULAR, materialSpecular);
	    glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuse);
	    glMaterialfv(GL_FRONT, GL_SHININESS, materialShininess);
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambCol);

       	float black[] = { 0, 0, 0, 0 };
        float colour1[] = { 2.0f, 1.5f, 0.75f, 1.0f };

	    Vector3 light0(0, 1, 0.5);
	    light0.Normalise();
	    GLfloat light0AsFourFloats[] = { light0.x, light0.y, light0.z, 0.0f };

	    glLightfv(GL_LIGHT0, GL_POSITION, light0AsFourFloats);
	    glLightfv(GL_LIGHT0, GL_DIFFUSE, colour1);
	    glLightfv(GL_LIGHT0, GL_SPECULAR, colour1);
	    glLightfv(GL_LIGHT0, GL_AMBIENT, black);

        glEnable        (GL_LIGHTING);
        glEnable        (GL_LIGHT0);
        glDisable       (GL_LIGHT1);
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
                START_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
                if( i > m_buildings.GetLastUpdated() )
                {
                    building->Render( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
                }
                else
                {
                    building->Render( timeSinceAdvance );
                }
                END_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
            }
        }
    }

#ifdef USE_DIRECT3D
	OpenGLD3D::g_pd3dDevice->SetRenderState( D3DRS_SPECULARENABLE, FALSE );
#endif
    glDisable       (GL_FOG);
	g_app->m_renderer->SetObjectLighting();
	g_app->m_renderer->UnsetObjectLighting();

    END_PROFILE(g_app->m_profiler, "Render Buildings");

	CHECK_OPENGL_STATE();
}


struct DepthSortedBuilding
{
    int   m_buildingIndex;
    float m_distance;
};


int DepthSortedBuildingCompare(const void * elem1, const void * elem2 )
{
    DepthSortedBuilding *b1 = (DepthSortedBuilding *) elem1;
    DepthSortedBuilding *b2 = (DepthSortedBuilding *) elem2;

    if      ( b1->m_distance < b2->m_distance )     return -1;
    else if ( b1->m_distance > b2->m_distance )     return +1;
    else                                            return 0;
}


// *** Render Building Alphas
void Location::RenderBuildingAlphas()
{
    START_PROFILE(g_app->m_profiler, "Render Building Alphas");
    float timeSinceAdvance = g_predictionTime;

    //
    // Prepare data for depth sorting of our buildings

    #define MaxDepthSortedBuildings 256
    static DepthSortedBuilding s_sortedBuildings[MaxDepthSortedBuildings];
    static int s_nextSortedBuilding;
    s_nextSortedBuilding = 0;


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
                    float distance = ( centrePos - g_app->m_camera->GetPos() ).MagSquared();
                    s_sortedBuildings[s_nextSortedBuilding].m_buildingIndex = i;
                    s_sortedBuildings[s_nextSortedBuilding].m_distance = distance;
                    s_nextSortedBuilding++;
                    DarwiniaReleaseAssert( s_nextSortedBuilding < MaxDepthSortedBuildings, "More that 256 buildings require Depth Sorting!" );
                }
                else
                {
                    START_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );

                    if( i > m_buildings.GetLastUpdated() )
                    {
                        building->RenderAlphas( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
                    }
                    else
                    {
                        building->RenderAlphas( timeSinceAdvance );
                    }

                    END_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
                }
            }
        }
    }


    //
    // Sort the buildings that require sorting

    START_PROFILE(g_app->m_profiler, "Depth Sort" );
    qsort( s_sortedBuildings, s_nextSortedBuilding, sizeof(DepthSortedBuilding), DepthSortedBuildingCompare );
    END_PROFILE(g_app->m_profiler, "Depth Sort" );


    //
    // Render those sorted buildings in reverse depth order

    for( int i = s_nextSortedBuilding-1; i >= 0; i-- )
    {
        int buildingIndex = s_sortedBuildings[i].m_buildingIndex;
        Building *building = m_buildings.GetData(buildingIndex);

        START_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );

        if( buildingIndex > m_buildings.GetLastUpdated() )
        {
            building->RenderAlphas( timeSinceAdvance + SERVER_ADVANCE_PERIOD );
        }
        else
        {
            building->RenderAlphas( timeSinceAdvance );
        }

        END_PROFILE( g_app->m_profiler, Building::GetTypeName( building->m_type ) );
    }


    glDisable       (GL_FOG);

    END_PROFILE(g_app->m_profiler, "Render Building Alphas");
}


// *** Render Clouds
void Location::RenderClouds()
{
    START_PROFILE(g_app->m_profiler, "Render Clouds");

    if( m_clouds )
    {
        float timeSinceAdvance = g_predictionTime;
        if( m_lastSliceProcessed >= 3 )
        {
            m_clouds->Render( timeSinceAdvance );
        }
        else
        {
            m_clouds->Render( timeSinceAdvance + 0.1f );
        }
    }

    END_PROFILE(g_app->m_profiler, "Render Clouds");
}


// *** Render Effects
void Location::RenderWeapons()
{
    START_PROFILE(g_app->m_profiler,  "Render Weapons" );

    float timeSinceAdvance = g_predictionTime;


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
	glEnable        ( GL_CULL_FACE );



    //
    // Render lasers

    glEnable        (GL_LINE_SMOOTH );
	glDisable		(GL_CULL_FACE);
	glEnable		(GL_TEXTURE_2D);
	glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp", false));


	float nearPlaneStart = g_app->m_renderer->GetNearPlane();
	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.2f,
									 	   g_app->m_renderer->GetFarPlane());

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

	glDisable		(GL_TEXTURE_2D);
	glEnable		(GL_CULL_FACE);
	glDepthMask     ( true );
	glDisable       ( GL_BLEND );
	glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDisable       ( GL_LINE_SMOOTH );

	g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
										   g_app->m_renderer->GetFarPlane());

    END_PROFILE(g_app->m_profiler,  "Render Weapons" );
}


void Location::InitialiseTeam( unsigned char _teamId, unsigned char _teamType )
{
    DarwiniaDebugAssert( _teamId < NUM_TEAMS );
    DarwiniaDebugAssert( m_teams[_teamId].m_teamType == Team::TeamTypeUnused );

    Team *team = &m_teams[_teamId];
    team->Initialise(_teamId);
    team->SetTeamType( _teamType );

    DebugOut( "CLIENT : New team created, id %d, type %d\n", _teamId, _teamType );

    if( _teamType == Team::TeamTypeLocalPlayer )
    {
        DebugOut( "CLIENT : Assigned team %d\n", _teamId );
        g_app->m_globalWorld->m_myTeamId = _teamId;
//		g_target->SetMousePos(g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH());
//		g_app->m_camera->RequestMode(Camera::ModeFreeMovement);
    }

	// Create instant units that belong to this team
	for (int i = 0; i < m_levelFile->m_instantUnits.Size(); i++)
	{
		InstantUnit *iu = m_levelFile->m_instantUnits.GetData(i);
		if (team->m_teamId != iu->m_teamId) continue;

		Team *team = &g_app->m_location->m_teams[iu->m_teamId];
		Vector3 pos(iu->m_posX, 0, iu->m_posZ);
		pos.y = m_landscape.m_heightMap->GetValue(pos.x, pos.z);
		int unitId = -1;
        Unit *newUnit = NULL;
        if( iu->m_inAUnit )
        {
            newUnit = team->NewUnit(iu->m_type, iu->m_number, &unitId, pos);
		    newUnit->SetWayPoint(pos);
			newUnit->m_routeId = iu->m_routeId;
			iu->m_routeId = -1;
        }

        WorldObjectId spawnedId = SpawnEntities(pos, iu->m_teamId, unitId, iu->m_type, iu->m_number, g_zeroVector, iu->m_spread, -1.0f, iu->m_routeId, iu->m_routeWaypointId );

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


    //
    // Are there any Running programs that need to be started?

    if( _teamType == Team::TeamTypeLocalPlayer )
    {
        for( int i = 0; i < m_levelFile->m_runningPrograms.Size(); ++i )
        {
            RunningProgram *program = m_levelFile->m_runningPrograms[i];

            if( program->m_type == Entity::TypeEngineer )
            {
                Vector3 pos( program->m_positionX[0], 0, program->m_positionZ[0] );
                pos.y = m_landscape.m_heightMap->GetValue( pos.x, pos.z );
                WorldObjectId objId = SpawnEntities( pos, _teamId, -1, Entity::TypeEngineer, 1, g_zeroVector, 0.0f );
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

                Task *task = new Task();
                task->m_type = GlobalResearch::TypeEngineer;
                task->m_objId = objId;
                task->m_state = Task::StateRunning;
                g_app->m_taskManager->RegisterTask( task );
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

                SpawnEntities( pos, _teamId, unitId, Entity::TypeInsertionSquadie, program->m_count, g_zeroVector, 10.0f );

                for( int s = 0; s < squad->m_entities.Size(); ++s )
                {
                    DarwiniaDebugAssert( squad->m_entities.ValidIndex(s) );
                    Entity *entity = squad->m_entities[s];
                    entity->m_stats[Entity::StatHealth] = program->m_health[s];
                }

                Task *task = new Task();
                task->m_type = GlobalResearch::TypeSquad;
                task->m_objId.Set( _teamId, unitId, -1, -1 );
                task->m_state = Task::StateRunning;
                g_app->m_taskManager->RegisterTask( task );
            }
        }
    }
}


void Location::RemoveTeam( unsigned char _teamId )
{
    if( _teamId < NUM_TEAMS )
    {
        Team *team = &m_teams[_teamId];
        m_teams[_teamId].m_teamType = Team::TeamTypeUnused;
    }

    if( _teamId == g_app->m_globalWorld->m_myTeamId )
    {
        g_app->m_globalWorld->m_myTeamId = 255;
    }
}

// *** UpdateTeam
void Location::UpdateTeam( unsigned char teamId, TeamControls const& teamControls )
{
	Team *team = &m_teams[teamId];
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
    if( unit )
    {
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

		if( unitMoved && teamControls.m_endSetTarget )
			g_app->m_gameCursor->CreateMarker( teamControls.m_mousePos );

		unitMoved = unitMove;

        if( primaryFire )
        {
            unit->Attack( teamControls.m_mousePos, false );
			// TODO: Not network safe (modify condition to ensure that the unit
			//       belongs to the local player)
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadUse );
        }

        if( secondaryFire )
        {
            unit->Attack( teamControls.m_mousePos, true );
        }
    }
    else
    {
        Entity *entity = team->GetMyEntity();
        if( entity )
        {
            if( teamControls.m_endSetTarget )
				g_app->m_gameCursor->CreateMarker( teamControls.m_mousePos );

            entity->DirectControl( teamControls );
            switch( entity->m_type )
            {
                case Entity::TypeEngineer:
                {
                    Engineer *eng = (Engineer *) entity;
                    if( unitMove ) eng->SetWaypoint( teamControls.m_mousePos );
                    if( secondaryFire ) eng->BeginBridge( teamControls.m_mousePos );
                    break;
                }

                case Entity::TypeOfficer:
                {
                    Officer *officer = (Officer *) entity;
                    if( unitMove ) officer->SetWaypoint( teamControls.m_mousePos );
                    if( primaryFire ) officer->SetOrders( teamControls.m_mousePos );
                    if( teamControls.m_secondaryFireDirected ) officer->SetOrders( teamControls.m_mousePos );
                    break;
                }

                case Entity::TypeArmour:
                {
                    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::UseArmour );

                    Armour *armour = (Armour *) entity;
                    if( unitMove ) armour->SetWayPoint( teamControls.m_mousePos );
                    if( primaryFire ) armour->SetOrders( teamControls.m_mousePos );
					if( teamControls.m_unitSecondaryMode ) armour->SetOrders( teamControls.m_mousePos );
                    break;
                }
            }
        }
    }
}


int Location::GetUnitId( Vector3 const &startRay, Vector3 const &direction, unsigned char team, float *_range )
{
    if( team == 255 ) return -1;

    float closestRangeSqd = FLT_MAX;
    int unitId = -1;

    //
    // Perform quick preselection by doing ray-sphere intersection tests
    // against the units centrepos and radius.  However we do then need to
    // zoom in and perform ray-sphere checks against each entity, because a unit
    // can become seperated so its bounding sphere covers a very large area.

    for( int unit = 0; unit < m_teams[team].m_units.Size(); ++unit )
    {
        if( m_teams[team].m_units.ValidIndex(unit) )
        {
            Unit *theUnit = m_teams[team].m_units.GetData( unit );
            bool rayHit = RaySphereIntersection( startRay, direction, theUnit->m_centrePos, theUnit->m_radius*1.5f );
            if( rayHit && theUnit->NumAliveEntities() > 0 )
            {
                for( int i = 0; i < theUnit->m_entities.Size(); ++i )
                {
                    if( theUnit->m_entities.ValidIndex(i) )
                    {
                        Entity *entity = theUnit->m_entities[i];
                        Vector3 spherePos = entity->m_pos+entity->m_centrePos;
                        float sphereRadius = entity->m_radius * 1.5f;
                        Vector3 hitPos;

                        bool entityHit = RaySphereIntersection( startRay, direction, spherePos, sphereRadius, 1e10, &hitPos );
                        if( entityHit && !entity->m_dead )
                        {
                            float centrePosX, centrePosY, rayHitX, rayHitY;
                            g_app->m_camera->Get2DScreenPos( spherePos, &centrePosX, &centrePosY );
                            g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

                            float rangeSqd = pow(centrePosX - rayHitX, 2) + pow(centrePosY - rayHitY, 2);
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
        *_range = sqrtf( closestRangeSqd );
    }

    return unitId;
}


WorldObjectId Location::GetEntityId( Vector3 const &startRay, Vector3 const &direction, unsigned char teamId, float *_range )
{
    if( teamId == 255 ) return WorldObjectId();

    float closestRangeSqd = FLT_MAX;
    WorldObjectId entId;

	int numEntities = m_teams[teamId].m_others.Size();
    for( int i = 0; i < numEntities; ++i )
    {
        if( m_teams[teamId].m_others.ValidIndex(i) )
        {
            Entity *ent = m_teams[teamId].m_others.GetData(i);
            if( !ent->m_dead )
            {
                Vector3 spherePos = ent->m_pos+ent->m_centrePos;
                float sphereRadius = ent->m_radius * 1.5f;
                Vector3 hitPos;
                bool rayHit = RaySphereIntersection( startRay, direction, spherePos, sphereRadius, 1e10, &hitPos );
                if( rayHit )
                {
                    float centrePosX, centrePosY, rayHitX, rayHitY;
                    g_app->m_camera->Get2DScreenPos( spherePos, &centrePosX, &centrePosY );
                    g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

                    float rangeSqd = pow(centrePosX - rayHitX, 2) + pow(centrePosY - rayHitY, 2);
                    if( rangeSqd < closestRangeSqd )
                    {
                        closestRangeSqd = rangeSqd;
                        entId = ent->m_id;
                    }
                }
            }
        }
    }

    if( _range && entId.IsValid() )
    {
        *_range = sqrtf(closestRangeSqd);
    }

    return entId;
}


int Location::GetBuildingId(Vector3 const &rayStart, Vector3 const &rayDir, unsigned char teamId, float _maxDistance, float *_range )
{
    float closestRangeSqd = FLT_MAX;
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

                if( building->m_type == Building::TypeRadarDish )
                {
                    rayHit = building->DoesRayHit( rayStart, rayDir, 1e10 );
                    if( rayHit ) RaySphereIntersection( rayStart, rayDir, building->m_centrePos, building->m_radius, _maxDistance, &hitPos );
                    // Have to do the raySphereIntersection in order to calculate the hitPos
                }
                else
                {
                    rayHit = RaySphereIntersection( rayStart, rayDir, building->m_centrePos, building->m_radius, _maxDistance, &hitPos );
                }

                if( rayHit )
                {
                    float centrePosX, centrePosY, rayHitX, rayHitY;
                    g_app->m_camera->Get2DScreenPos( building->m_centrePos, &centrePosX, &centrePosY );
                    g_app->m_camera->Get2DScreenPos( hitPos, &rayHitX, &rayHitY );

                    float rangeSqd = pow(centrePosX - rayHitX, 2) + pow(centrePosY - rayHitY, 2);
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
        *_range = sqrtf( closestRangeSqd );
    }

	return buildingId;
}


void Location::ThrowWeapon( Vector3 const &_pos, Vector3 const &_target, int _type, unsigned char _fromTeamId )
{
    float distance = ( _target - _pos ).Mag();
    float force = sqrtf(distance) * 8.0f;

    int grenadeResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade );
    if( _fromTeamId == 1 ) grenadeResearch = 4;

	float maxForce = ThrowableWeapon::GetMaxForce( grenadeResearch );

    if( force > maxForce ) force = maxForce;

    Vector3 front = ( _target - _pos ).Normalise();
    front.y = 1.0f;
    front.Normalise();

    ThrowableWeapon *weapon = NULL;

    switch( _type )
    {
        case WorldObject::EffectThrowableGrenade:             weapon = new Grenade( _pos, front, force );             break;
        case WorldObject::EffectThrowableAirstrikeMarker:     weapon = new AirStrikeMarker( _pos, front, force );     break;
        case WorldObject::EffectThrowableControllerGrenade:   weapon = new ControllerGrenade( _pos, front, force );   break;
    }

    int weaponId = m_effects.PutData( weapon );
    weapon->m_id.Set( _fromTeamId, UNIT_EFFECTS, weaponId, -1 );
    weapon->m_id.GenerateUniqueId();
    weapon->Initialise();


    //
    // Create muzzle flash

    Vector3 flashFront = front;
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 20.0f, 3.0f);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _fromTeamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();

}


void Location::FireRocket( Vector3 const &_pos, Vector3 const &_target, unsigned char _teamId )
{
    Rocket *r = new Rocket(_pos, _target);
    r->m_fromTeamId = _teamId;

    int weaponId = m_effects.PutData( r );
    r->m_id.Set( _teamId, UNIT_EFFECTS, weaponId, -1 );
    r->m_id.GenerateUniqueId();
    r->Initialise();


    //
    // Create muzzle flash

    Vector3 flashFront = _target - _pos;
    flashFront.Normalise();
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 20.0f, 3.0f);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


void Location::FireTurretShell( Vector3 const &_pos, Vector3 const &_vel )
{
    int armourResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeArmour );
    float lifeTime = 0.0f;
    switch( armourResearch )
    {
        case 0 :    lifeTime = 0.5f;            break;
        case 1 :    lifeTime = 0.5f;            break;
        case 2 :    lifeTime = 1.0f;            break;
        case 3 :    lifeTime = 1.5f;            break;
        case 4 :    lifeTime = 2.0f;            break;
    }

    TurretShell *shell = new TurretShell(lifeTime);
    shell->m_pos = _pos;
    shell->m_vel = _vel;

    int weaponId = m_effects.PutData( shell );
    shell->m_id.Set( -1, UNIT_EFFECTS, weaponId, -1 );
    shell->m_id.GenerateUniqueId();


    //
    // Create muzzle flash

    Vector3 flashFront = _vel;
    flashFront.Normalise();

    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 40.0f, 2.0f);
    int index = m_effects.PutData( mf );
    mf->m_id.Set( -1, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


void Location::FireLaser( Vector3 const &_pos, Vector3 const &_vel, unsigned char _teamId )
{
    int laserResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeLaser );
    float lifetime = 0.0f;
    switch( laserResearch )
    {
        case 0 :        lifetime = 0.2f;            break;
        case 1 :        lifetime = 0.4f;            break;
        case 2 :        lifetime = 0.6f;            break;
        case 3 :        lifetime = 0.8f;            break;
        case 4 :        lifetime = 1.0f;            break;
    }

    Laser *l = m_lasers.GetPointer();
    l->m_pos = _pos;
    l->m_vel = _vel;
    l->m_fromTeamId = _teamId;
    l->Initialise(lifetime);


    //
    // Create muzzle flash

    Vector3 flashFront = _vel;
    flashFront.Normalise();
    MuzzleFlash *mf = new MuzzleFlash( _pos, flashFront, 20.0f * lifetime, 1.0f );
    int index = m_effects.PutData( mf );
    mf->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    mf->m_id.GenerateUniqueId();
}


Team *Location::GetMyTeam()
{
    if (g_app->m_globalWorld->m_myTeamId == 255)
    {
        return NULL;
    }
    return &m_teams[g_app->m_globalWorld->m_myTeamId];
}


// ****************************************************************************
// Private Methods
// ****************************************************************************

void Location::Bang( Vector3 const &_pos, float _range, float _damage )
{
    int numCores = _range * _damage * 0.01f;
	numCores += syncfrand(numCores);
    for( int p = 0; p < numCores; ++p )
    {
        Vector3 vel( syncsfrand( 20.0f ),
                     10.0f + syncfrand( 10.0f ),
                     syncsfrand( 20.0f ) );
        float size = 120.0f + syncfrand(60.0f);
        g_app->m_particleSystem->CreateParticle( _pos + g_upVector * _range * 0.3f, vel,
												 Particle::TypeExplosionCore, size );
    }

    int numDebris = max(1, _range * _damage * 0.005f);
    for( int p = 0; p < numDebris; ++p )
    {
        Vector3 vel( syncsfrand( 30.0f ),
                     20.0f + syncfrand( 20.0f ),
                     syncsfrand( 30.0f ) );
        float size = 30.0f + syncfrand(20.0f);
        g_app->m_particleSystem->CreateParticle( _pos + g_upVector * _range * 0.5f, vel,
                                                 Particle::TypeExplosionDebris, size );
    }


    //
    // Damage everyone nearby

    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetNeighbours( _pos.x, _pos.z, _range*2.0f, &numFound );
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        WorldObject *obj = g_app->m_location->GetEntity( id );
        Entity *entity = (Entity *) obj;

        float distance = (entity->m_pos - _pos).Mag();
        float fraction = (_range*2.0f - distance) / _range*2.0f;
        //fraction *= (1.0f + syncfrand(0.3f));
        //fraction *= 1.5f;
        fraction = min( 1.0f, fraction );

        entity->ChangeHealth( _damage * fraction * -1.0f );

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

        entity->m_vel += push;
        entity->m_onGround = false;
    }


	//
	// Is visible?

	Vector3 tmp;
	bool isVisible = !m_landscape.RayHit(g_app->m_camera->GetPos(),_pos-g_app->m_camera->GetPos(),&tmp)
		|| (tmp-g_app->m_camera->GetPos()).Mag()>(_pos-g_app->m_camera->GetPos()).Mag()-0.3f;

    //
    // Shockwave

    if(isVisible) CreateShockwave( _pos, _range/20.0f );


	//
	// Punch effect

#ifdef USE_DIRECT3D
	if(g_deformEffect && isVisible) g_deformEffect->AddPunch( _pos, _range );
#endif


	//
	// Wow, that was a big bang. Maybe we killed a building

    float maxBuildingRange = _range * 3.0f;
    for( int i = 0; i < m_buildings.Size(); ++i )
    {
        if( m_buildings.ValidIndex(i) )
        {
		    Building *building = m_buildings[i];
		    float dist = (_pos - building->m_pos).Mag();

            if( dist < maxBuildingRange )
            {
                //float fraction = (_range*3.0f - dist) / _range*3.0f;
                float fraction = 1.0f - dist / maxBuildingRange;
                fraction = max( 0.0f, fraction );
                fraction = min( 1.0f, fraction );
                building->Damage( _damage * fraction * -1.0f );
            }
        }
    }
}


void Location::CreateShockwave( Vector3 const &_pos, float _size, unsigned char _teamId )
{
    Shockwave *s = new Shockwave( _teamId, _size );
    s->m_pos = _pos;
    int index = m_effects.PutData( s );
    s->m_id.Set( _teamId, UNIT_EFFECTS, index, -1 );
    s->m_id.GenerateUniqueId();
}


void Location::SetupFog()
{
    float fogCol[] = {  g_app->m_backgroundColour.r/255.0f,
                        g_app->m_backgroundColour.g/255.0f,
                        g_app->m_backgroundColour.b/255.0f,
                        0 };

	glHint		( GL_FOG_HINT, GL_DONT_CARE );
    glFogf      ( GL_FOG_DENSITY, 1.0f );
    glFogf      ( GL_FOG_START, 1000.0f );
    glFogf      ( GL_FOG_END, 4000.0f );
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

		GLfloat ambCol[] = { 0.0f, 0.0f, 0.0f, 1.0f };

		Vector3 front(light->m_front[0], light->m_front[1], light->m_front[2]);
        front.Normalise();
		GLfloat frontAsFourFloats[] = { front.x, front.y, front.z, 0.0f };
        GLfloat colourAsFourFloats[] = { light->m_colour[0], light->m_colour[1], light->m_colour[2], light->m_colour[3] };

        if( ChristmasModEnabled() == 1 )
        {
            colourAsFourFloats[0] = 1.3f;
            colourAsFourFloats[1] = 1.2f;
            colourAsFourFloats[2] = 1.2f;
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


int Location::ChristmasModEnabled()
{
#ifdef DEMOBUILD
    return 0;
#endif

    if( g_app->m_editing ) return 0;

    // Last 2 weeks in December only
    // Also allow user to disable if he wishes

	time_t now = time(NULL);
    tm *theTime = localtime(&now);

    if( theTime->tm_mon == 11 &&
        (theTime->tm_mday == 25 || theTime->tm_mday == 26) )
    {
        int prefValue = g_prefsManager->GetInt( "ChristmasEnabled", 1 );
        if( prefValue == 1 ) return 1;
        return 2;
    }

    return 0;
}


bool Location::IsFriend( unsigned char _teamId1, unsigned char _teamId2 )
{
    if( _teamId1 == 255 || _teamId2 == 255 ) return false;


                                        /*  green   red     player  ally    ally    enemy   enemy  rogue  */

 /*   bool friends[NUM_TEAMS][NUM_TEAMS] = {
                                            true,   false,  true,   true,   true,   false,  false,  false,    // Green
                                            false,  true,   false,  false,  false,  false,  false,  false,    // Red
                                            true,   false,  true,   true,   true,   false,  false,  false,    // Player
                                            true,   false,  true,   true,   true,   false,  false,  false,    // Ally
                                            true,   false,  true,   true,   true,   false,  false,  false,    // Ally
                                            false,  true,   false,  false,  false,  true,   true,   false,    // Enemy
                                            false,  true,   false,  false,  false,  true,   true,   false,    // Enemy
                                            false,  false,  false,  false,  false,  false,  false,  true      // Rogue
                                         };
*/

//      STRESS TEST SETTINGS
//    bool friends[NUM_TEAMS][NUM_TEAMS] = {
//                                            true,   false,  false,   false,  false,  false,  false,  false,      // Green
//                                            false,  true,   false,  false,  false,  false,  false,  false,      // Red
//                                            false,   false,  true,   true,   false,  false,  false,  false,      // Player
//                                            true,   false,  true,   true,   false,  false,  false,  false,      // Player
//                                            false,  false,  false,  false,  true,   false,  false,  false,      // blue
//                                            false,  false,  false,  false,  false,  true,   false,  false,      // green
//                                            false,  false,  false,  false,  false,  false,  true,   false,      // orange
//                                            false,  false,  false,  false,  false,  false,  false,  true        // grey
//                                         };

    //return friends[ _teamId1 ][ _teamId2 ];
	return m_levelFile->m_teamAlliances[ _teamId1 ][ _teamId2 ];
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
