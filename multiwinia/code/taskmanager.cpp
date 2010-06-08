#include "lib/universal_include.h"

#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/vector2.h"
#include "lib/debug_utils.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/profiler.h"
#include "lib/hi_res_time.h"
#include "lib/language_table.h"
#include "lib/debug_render.h"
#include "lib/ogl_extensions.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/math/random_number.h"

#include "lib/preferences.h"
#include "lib/targetcursor.h"

#include "network/clienttoserver.h"

#include "app.h"
#include "gamecursor.h"
#include "gesture.h"
#include "taskmanager.h"
#include "global_world.h"
#include "routing_system.h"
#include "app.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "location.h"
#include "taskmanager_interface.h"
#include "team.h"
#include "particle_system.h"
#include "entity_grid.h"
#include "taskmanager_interface.h"
#include "unit.h"
#include "camera.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "gamecursor.h"
#include "location_input.h"
#include "markersystem.h"
#include "obstruction_grid.h"
#include "explosion.h"
#include "multiwinia.h"
#include "main.h"

#include "sound/soundsystem.h"

#include "interface/prefs_other_window.h"

#include "worldobject/insertion_squad.h"
#include "worldobject/officer.h"
#include "worldobject/darwinian.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/trunkport.h"
#include "worldobject/harvester.h"
#include "worldobject/nuke.h"
#include "worldobject/gunturret.h"
#include "worldobject/portal.h"
#include "worldobject/spam.h"
#include "worldobject/tree.h"
#include "worldobject/anthill.h"
#include "worldobject/triffid.h"
#include "worldobject/crate.h"
#include "worldobject/meteor.h"
#include "worldobject/restrictionzone.h"
#include "multiwiniahelp.h"

static std::vector<WorldObjectId> s_neighbours;


Task::Task( int _teamId )
:   m_type(GlobalResearch::TypeSquad),
    m_teamId(_teamId),
    m_id(-1),
    m_state(StateIdle),
    m_route(NULL),
    m_lifeTimer(0.0),
    m_startTimer(0.0),
    m_option(-1),
    m_screenX(0),
    m_screenY(0),
    m_screenH(0)
{
}


Task::~Task()
{
    delete m_route;
}


void Task::Start()
{
    m_state = StateStarted;    

    switch( m_type )
    {
        case GlobalResearch::TypeSquad:
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadSummon );
#endif
            m_lifeTimer = 60.0;
            break;

        case GlobalResearch::TypeEngineer:
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::EngineerSummon );
#endif
            break;

        case GlobalResearch::TypeBooster:       
        case GlobalResearch::TypeHotFeet:       
        case GlobalResearch::TypeSubversion: 
        case GlobalResearch::TypeRage:          
            m_lifeTimer = 30.0;
            break;

        case GlobalResearch::TypeTank:
            if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle )
            {
                m_lifeTimer = 30.0;    
            }
            break;
    }
}


void Task::Target( Vector3 const &_pos )
{
    if( m_startTimer > 0.0 )
    {
        return;
    }

    if( m_state != Task::StateStarted ) return;

    switch( m_type )
    {
        case GlobalResearch::TypeSquad:         TargetSquad( _pos );        break;                
        case GlobalResearch::TypeEngineer:      TargetEngineer( _pos );     break;        
        case GlobalResearch::TypeOfficer:       TargetOfficer( _pos );      break;
        case GlobalResearch::TypeArmour:        TargetArmour( _pos );       break;
        case GlobalResearch::TypeHarvester:     TargetHarvester( _pos );    break;
        case GlobalResearch::TypeAirStrike:     TargetAirStrike( _pos );    break;
        case GlobalResearch::TypeNuke:          TargetNuke( _pos );         break;
        case GlobalResearch::TypeSubversion:    TargetSubversion( _pos );   break;
        case GlobalResearch::TypeBooster:       TargetBooster( _pos );      break;
        case GlobalResearch::TypeHotFeet:       TargetHotFeet( _pos );      break;
        case GlobalResearch::TypeGunTurret:     TargetGunTurret( _pos );    break;
        case GlobalResearch::TypeShaman:        TargetShaman( _pos );       break;
        case GlobalResearch::TypeInfection:     TargetInfection( _pos );    break;
        case GlobalResearch::TypeMagicalForest: TargetMagicalForest( _pos );    break;
        case GlobalResearch::TypeDarkForest:    TargetDarkForest( _pos );   break;
        case GlobalResearch::TypeAntsNest:      TargetAntNest( _pos );      break;
        case GlobalResearch::TypePlague:        TargetPlague( _pos );       break;
        case GlobalResearch::TypeEggSpawn:      TargetEggs( _pos );         break;
        case GlobalResearch::TypeMeteorShower:  TargetMeteorShower( _pos ); break;
        case GlobalResearch::TypeExtinguisher:  TargetExtinguisher( _pos ); break;
        case GlobalResearch::TypeRage:          TargetRage( _pos );         break;
        case GlobalResearch::TypeTank:          TargetTank( _pos );         break;
        case GlobalResearch::TypeDropShip:      TargetDropShip( _pos );     break;
    }
}


void Task::TargetSquad( Vector3 const &_pos )
{
    int numEntities = 2 + g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeSquad );

    int unitId;
    g_app->m_location->m_teams[m_teamId]->NewUnit( Entity::TypeInsertionSquadie, numEntities, &unitId, _pos );
    g_app->m_location->SpawnEntities( _pos, m_teamId, unitId,                      
                                      Entity::TypeInsertionSquadie, numEntities, g_zeroVector, 10 );                        

    g_app->m_location->m_teams[m_teamId]->SelectUnit( unitId, -1, -1 );
    m_objId.Set( m_teamId, unitId, -1, -1 );

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadSummon );
#endif
    m_state = StateRunning;    
    
    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        int trackEntity = g_prefsManager->GetInt( OTHER_AUTOMATICCAM, 0 );
        if( trackEntity == 0 )
        {
            // work out if player is using control pad
		    if ( g_inputManager->getInputMode() == INPUT_MODE_GAMEPAD )
			    trackEntity = 2;
        }

        if( trackEntity == 2 )
        {
            g_app->m_camera->RequestEntityTrackMode( m_objId );
        }
    }

    if( g_app->Multiplayer() )
    {
        Crate::TriggerSoundEvent( Crate::CrateRewardSquad, _pos, m_teamId );
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
    }
}


void Task::TargetEngineer( Vector3 const &_pos )
{
	Vector3 pos = _pos;
	pos.y += 10.0;
    m_objId = g_app->m_location->SpawnEntities( pos, m_teamId, -1, Entity::TypeEngineer, 1, g_zeroVector, 0 );
    g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );

    m_state = StateRunning;    
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
}

void Task::TargetShaman( Vector3 const &_pos )
{
	Vector3 pos = _pos;	
    m_objId = g_app->m_location->SpawnEntities( pos, m_teamId, -1, Entity::TypeShaman, 1, g_zeroVector, 0 );
    g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );

    m_state = StateRunning;    
    g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
}


void Task::TargetArmour( Vector3 const &_pos )
{
#ifndef DEMOBUILD
    m_objId = g_app->m_location->SpawnEntities( _pos, m_teamId, -1, Entity::TypeArmour, 1, g_zeroVector, 0 );
    g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );

    m_state = StateRunning;

    if( g_app->Multiplayer() )
    {
        Crate::TriggerSoundEvent( Crate::CrateRewardArmour, _pos, m_teamId );
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
    }    
#endif
}

void Task::TargetHarvester(const Vector3 &_pos)
{
    Vector3 pos = _pos;
	pos.y += 50.0;
    m_objId = g_app->m_location->SpawnEntities( pos, m_teamId, -1, Entity::TypeHarvester, 1, g_zeroVector, 0 );
    g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );

    m_state = StateRunning;    

    if( g_app->Multiplayer() )
    {
        Crate::TriggerSoundEvent( Crate::CrateRewardHarvester, _pos, m_teamId );
    }
    else
    {
        g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
    }
}


WorldObjectId Task::Promote( WorldObjectId _id )
{    
    Entity *entity = g_app->m_location->GetEntity( _id );
    AppDebugAssert( entity );

    
    //
    // Spawn an Officer
    
    WorldObjectId spawnedId = g_app->m_location->SpawnEntities( entity->m_pos, m_teamId, -1, Entity::TypeOfficer, 1, entity->m_vel, 0 );
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

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    g_app->m_helpSystem->PlayerDoneAction( HelpSystem::OfficerCreate );
#endif
    
    return spawnedId;
}


WorldObjectId Task::Demote( WorldObjectId _id )
{
    // Make demoted officers return to green
    //int teamId = g_app->m_globalWorld->m_myTeamId;
    int teamId = 0;
    
    Entity *entity = g_app->m_location->GetEntity( _id );
    AppDebugAssert( entity );
    

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


    return spawnedId;
}


WorldObjectId Task::FindDarwinian( Vector3 const &_pos, int _teamId )
{
    bool includes[NUM_TEAMS];
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
		if (g_app->IsSinglePlayer())
		{
			includes[i] = g_app->m_location->IsFriend(i,_teamId);
		}
		else
		{
			includes[i] = (i == _teamId);
		}
    }
    int numFound;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, 10.0, &numFound, includes );
    WorldObjectId nearestId;
    float nearest = 99999.9;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && entity->m_type == Entity::TypeDarwinian )
        {
            float distance = ( entity->m_pos - _pos ).MagSquared();
            if( distance < nearest )
            {
                nearestId = id;
                nearest = distance;
            }
        }
    }

    return nearestId;
}


void Task::TargetOfficer( Vector3 const &_pos )
{
    //
    // We will not upgrade people if we're controlling something right now
    
    Team *myTeam = g_app->m_location->m_teams[m_teamId];
    if( myTeam->m_currentUnitId != -1 ||
        myTeam->m_currentEntityId != -1 ||
        myTeam->m_currentBuildingId != -1 )
    {
        return;
    }


    //
    // Find the nearest friendly Darwinian to upgrade to an Officer
    // If we found someone, promote them
    // Then shutdown this task
    // Then select them

    WorldObjectId nearestId = FindDarwinian( _pos, m_teamId );

    if( nearestId.IsValid() ) 
    {
        // DANGER: TerminateTask(m_id) releases our memory! To work around this, we cache m_teamId
        // local in a local, so we can reference it after the this pointer becomes invalid.
        int teamId = m_teamId;

        WorldObjectId id = Promote( nearestId );
        g_app->m_location->m_teams[ m_teamId ]->m_taskManager->TerminateTask( m_id );
        g_app->m_location->m_teams[ id.GetTeamId() ]->SelectUnit( id.GetUnitId(), id.GetIndex(), -1 );
        myTeam->m_taskManager->m_mostRecentEntity = id;

        //g_app->m_markerSystem->RegisterMarker_Entity( teamId, id );

        if( g_app->m_globalWorld->m_myTeamId == teamId )
        {
            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, GlobalResearch::TypeOfficer, 2.5 );
            g_app->m_soundSystem->TriggerOtherEvent( NULL, "GestureSuccess", SoundSourceBlueprint::TypeGesture );
        }
    }
}


void Task::TargetAirStrike( Vector3 const &_pos )
{
    AppDebugOut("AirStrike Launched by Team %d\n", m_teamId );
    m_objId = Crate::RunAirStrike( _pos, m_teamId, m_option == 1 );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    m_state = StateRunning;
    m_pos = _pos;

    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetDropShip( Vector3 const &_pos )
{
    float inset = 100.0;
    float startHeight = 500.0;
    float landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    
    DArray<Vector3> startPositions;
    startPositions.PutData( Vector3(inset,startHeight,inset) );
    startPositions.PutData( Vector3(inset,startHeight,landSizeZ-inset) );
    startPositions.PutData( Vector3(landSizeX-inset,startHeight,landSizeZ-inset) );
    startPositions.PutData( Vector3(landSizeX-inset,startHeight,inset) );

    int enterIndex = -1;
    float nearest = 99999.9;

    //
    // Choose the next nearest corner as the entry position
    for( int i = 0; i < startPositions.Size(); ++i )
    {
        Vector3 thisPos = startPositions[i];
        float thisDist = ( thisPos - _pos ).Mag();
        if( thisDist < nearest )
        {
            nearest = thisDist;
            enterIndex = i;
        }
    }

    m_objId = g_app->m_location->SpawnEntities( startPositions[ enterIndex ], m_teamId, -1, Entity::TypeDropShip, 1, Vector3(), 0.0 );
    Entity *e = g_app->m_location->GetEntity( m_objId );
    if( e )
    {
        e->SetWaypoint( _pos );
    }
    m_state = StateRunning;
}

void Task::TargetNuke( Vector3 const &_pos )
{
    Crate::RunNuke( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetSubversion(const Vector3 &_pos)
{
    Crate::RunSubversion( _pos, m_teamId, m_id );    
    m_state = StateRunning;
    SwitchTo();
}

void Task::TargetRage(const Vector3 &_pos)
{
    Crate::RunRage( _pos, m_teamId, m_id );    
    m_state = StateRunning;
    SwitchTo();
}

void Task::TargetBooster( Vector3 const &_pos )
{
    Crate::RunBooster( _pos, m_teamId, m_id );
    m_state = StateRunning;
    SwitchTo();
}

void Task::TargetHotFeet( Vector3 const &_pos )
{
    Crate::RunHotFeet( _pos, m_teamId, m_id );
    m_state = StateRunning;
    SwitchTo();
}

void Task::TargetGunTurret( Vector3 const &_pos )
{
    int turretType = m_option;
    if( turretType == -1 ) turretType = GunTurret::GunTurretTypeStandard;
    m_objId = Crate::RunGunTurret( _pos, m_teamId, turretType );
    m_pos = _pos;
    Team *team = g_app->m_location->m_teams[ m_teamId ];

    if( g_app->m_multiwinia->GetGameOption("MaxTurrets") == -1 )
    {
        team->m_taskManager->TerminateTask( m_id, false );
    }
    else
    {
        // this game has a turret limit, which means we can't just terminate the task
        m_state = StateRunning;
        team->m_taskManager->m_currentTaskId = -1;
    }
}

void Task::TargetInfection( Vector3 const &_pos )
{
    Crate::RunInfection( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetMagicalForest( Vector3 const &_pos )
{
    Crate::RunMagicalForest( _pos, m_teamId );    
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetDarkForest( Vector3 const &_pos )
{
    Crate::RunDarkForest( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false);
}

void Task::TargetAntNest( Vector3 const &_pos )
{
    Crate::RunAntNest( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetPlague( Vector3 const &_pos )
{
    Crate::RunPlague( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetEggs( Vector3 const &_pos )
{
    Crate::RunEggs( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetMeteorShower( Vector3 const &_pos )
{
    Crate::RunMeteorShower( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );
}

void Task::TargetExtinguisher( Vector3 const &_pos )
{
   /* Crate::RunExtinguisher( _pos, m_teamId );
    Team *team = g_app->m_location->m_teams[ m_teamId ];
    team->m_taskManager->TerminateTask( m_id, false );

    Crate::TriggerSoundEvent( Crate::CrateRewardExtinguisher, g_zeroVector, 255 );*/
}


void Task::TargetTank( Vector3 const &_pos )
{
   /* m_objId = g_app->m_location->SpawnEntities( _pos, m_teamId, -1, Entity::TypeTank, 1, g_zeroVector, 0 );
    g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );
    m_state = StateRunning;

    Crate::TriggerSoundEvent( Crate::CrateRewardTank, g_zeroVector, 255 );*/
}

bool Task::ShutdownController()
{
    Team *team = g_app->m_location->m_teams[ m_teamId ];

    bool found = false;

    for( int i = 0; i < team->m_others.Size(); ++i )
    {
        if( team->m_others.ValidIndex(i) )
        {
            Entity *entity = team->m_others[i];
            if( entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *)entity;
                if( darwinian->m_controllerId == m_id )
                {
                    darwinian->LoseControl();
                    found = true;
                }   
            }
        }
    }

    return found;
}

Vector3 Task::GetPosition()
{
    if( m_pos != g_zeroVector ) return m_pos;

    Unit *unit = g_app->m_location->GetUnit(m_objId);
    Entity *entity = g_app->m_location->GetEntity(m_objId);

    if( unit ) return unit->m_centrePos;
    if( entity ) return entity->m_pos;

    return g_zeroVector;
}


bool Task::Advance()
{    
    if( m_state == StateRunning )
    {
        switch( m_type )
        {
            case GlobalResearch::TypeSquad:
            {
                if( g_app->Multiplayer() )
                {
                    if( m_lifeTimer > 0.0f )
                    {
                        m_lifeTimer -= SERVER_ADVANCE_PERIOD;
                        if( m_lifeTimer <= 0.0f )
                        {
                            Stop();
                            return true;
                        }
                    }
                }
            }
            // deliberate fallthrough
            case GlobalResearch::TypeController:
            {
                Unit *unit = g_app->m_location->GetUnit( m_objId );
                if( !unit || unit->NumAliveEntities() == 0 )
                {
                    if( g_app->m_location->m_teams[m_teamId]->m_taskManager->m_currentTaskId == m_id )
                    {
                        if( g_app->m_globalWorld->m_myTeamId == m_teamId )
                        {
                            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, m_type, 3.0 );
                        }
                    }
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeTank:
                if( g_app->Multiplayer() && g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle )
                {
                    if( m_lifeTimer > 0.0f )
                    {
                        m_lifeTimer -= SERVER_ADVANCE_PERIOD;
                        if( m_lifeTimer <= 0.0f )
                        {
                            Stop();
                            return true;
                        }
                    }
                }

                // intentional drop through
         
            case GlobalResearch::TypeEngineer:
            case GlobalResearch::TypeArmour:
            case GlobalResearch::TypeHarvester:
            {
                Entity *entity = g_app->m_location->GetEntity( m_objId );
                if( !entity || entity->m_dead ) 
                {
                    if( g_app->m_location->m_teams[m_teamId]->m_taskManager->m_currentTaskId == m_id )
                    {
                        if( g_app->m_globalWorld->m_myTeamId == m_teamId )
                        {
                            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, m_type, 3.0 );
                        }
                    }
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeNuke:
            case GlobalResearch::TypeDropShip:
            {
                Entity *entity = g_app->m_location->GetEntity( m_objId );
                if( !entity || entity->m_dead ) 
                {
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeAirStrike:
            {
                WorldObject *w = g_app->m_location->GetEffect( m_objId );
                if( !w )
                {
                    return true;
                }
                break;
            }

            case GlobalResearch::TypeOfficer:
            {
                m_state = StateStarted;
                break;
            }

            case GlobalResearch::TypeBooster:
            case GlobalResearch::TypeHotFeet:
            case GlobalResearch::TypeSubversion:
            case GlobalResearch::TypeRage:
            {
                m_lifeTimer -= SERVER_ADVANCE_PERIOD;
                if( m_lifeTimer <= 0.0 )
                {
                    return true;
                }

                Vector3 pos;
                int num = 0;
                Team *team = g_app->m_location->m_teams[m_teamId];                
                for( int i = 0; i < team->m_others.Size(); ++i )
                {
                    if( team->m_others.ValidIndex( i ) )
                    {
                        Darwinian *d = (Darwinian *)team->m_others[i];
                        if( d && d->m_type == Entity::TypeDarwinian )
                        {
                            if( d->m_hotFeetTaskId == m_id ||
                                d->m_boosterTaskId == m_id ||
                                d->m_subversionTaskId == m_id ||
                                d->m_rageTaskId == m_id ) 
                            {
                                pos += d->m_pos;
                                num++;
                            }
                        }
                    }
                }

                if( num > 0 )
                {
                    pos /= num;
                    m_pos = pos;
                }
                else
                {
                    return true;
                }

                break;
            }

            case GlobalResearch::TypeGunTurret:
            {
                Building *turret = g_app->m_location->GetBuilding( m_objId.GetUniqueId() );
                if( !turret )
                {
                    return true;
                }
                else
                {
                    if( turret->m_id.GetTeamId() != m_teamId &&
                        turret->m_id.GetTeamId() != 255 )
                    {
                        turret->Damage(-1000);
                        return true;
                    }
                }

                break;
            }
        }
    }

    if( m_startTimer > 0.0 )
    {
        m_startTimer -= SERVER_ADVANCE_PERIOD;
    }
	
    return( m_state == StateStopping );
}


void Task::SwitchTo()
{
    if( m_teamId == g_app->m_globalWorld->m_myTeamId )
    {
        if( g_app->m_camera->IsInMode( Camera::ModeRadarAim ) ||
            g_app->m_camera->IsInMode( Camera::ModeTurretAim ) )
        {
            g_app->m_camera->RequestMode( Camera::ModeFreeMovement );
        }
    }

    switch( m_type )
    {
        case GlobalResearch::TypeSquad:            
        {
            g_app->m_location->m_teams[m_teamId]->SelectUnit( m_objId.GetUnitId(), -1, -1 );
            break;
        }

        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeArmour:
        case GlobalResearch::TypeHarvester:
        case GlobalResearch::TypeShaman:
        case GlobalResearch::TypeTank:
        {
            g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, m_objId.GetIndex(), -1 );
            break;
        }

        case GlobalResearch::TypeController:
        {
            for( int i = 0; i < g_app->m_location->m_teams[m_teamId]->m_taskManager->m_tasks.Size(); ++i )
            {
                Task *task = g_app->m_location->m_teams[m_teamId]->m_taskManager->m_tasks.GetData(i);
                if( task->m_type == GlobalResearch::TypeSquad &&
                    task->m_objId == m_objId )
                {
                    g_app->m_location->m_teams[m_teamId]->m_taskManager->SelectTask( task->m_id );
                    break;
                }
            }
            break;
        }

        case GlobalResearch::TypeOfficer:
        {
            g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
            break;            
        }

        case GlobalResearch::TypeHotFeet:
        {
            Team *team = g_app->m_location->m_teams[m_teamId];
            if( team->m_teamType != TeamTypeCPU )
            {
                for( int i = 0; i < team->m_others.Size(); ++i )
                {
                    if( team->m_others.ValidIndex( i ) )
                    {
                        Darwinian *d = (Darwinian *)team->m_others[i];
                        if( d && d->m_type == Entity::TypeDarwinian )
                        {
                            if( d->m_hotFeetTaskId == m_id )
                            {
                                d->m_underPlayerControl = true;
                            }
                        }
                    }
                }
            }
            team->SelectUnit( -1, -1, -1 );
            break;
        }

        case GlobalResearch::TypeBooster:
        {
            Team *team = g_app->m_location->m_teams[m_teamId];
            if( team->m_teamType != TeamTypeCPU )
            {
                for( int i = 0; i < team->m_others.Size(); ++i )
                {
                    if( team->m_others.ValidIndex( i ) )
                    {
                        Darwinian *d = (Darwinian *)team->m_others[i];
                        if( d && d->m_type == Entity::TypeDarwinian )
                        {
                            if( d->m_boosterTaskId == m_id )
                            {
                                d->m_underPlayerControl = true;
                            }
                        }
                    }
                }
            }
            team->SelectUnit( -1, -1, -1 );
            break;
        }

        case GlobalResearch::TypeSubversion:
        {
            Team *team = g_app->m_location->m_teams[m_teamId];
            if( team->m_teamType != TeamTypeCPU )
            {
                for( int i = 0; i < team->m_others.Size(); ++i )
                {
                    if( team->m_others.ValidIndex( i ) )
                    {
                        Darwinian *d = (Darwinian *)team->m_others[i];
                        if( d && d->m_type == Entity::TypeDarwinian )
                        {
                            if( d->m_subversionTaskId == m_id )
                            {
                                d->m_underPlayerControl = true;
                            }
                        }
                    }
                }
            }
            team->SelectUnit( -1, -1, -1 );
            break;

        }

        case GlobalResearch::TypeRage:
        {
            Team *team = g_app->m_location->m_teams[m_teamId];
            if( team->m_teamType != TeamTypeCPU )
            {
                for( int i = 0; i < team->m_others.Size(); ++i )
                {
                    if( team->m_others.ValidIndex( i ) )
                    {
                        Darwinian *d = (Darwinian *)team->m_others[i];
                        if( d && d->m_type == Entity::TypeDarwinian )
                        {
                            if( d->m_rageTaskId == m_id )
                            {
                                d->m_underPlayerControl = true;
                            }
                        }
                    }
                }
            }
            team->SelectUnit( -1, -1, -1 );
            break;
        }

        /*case GlobalResearch::TypeGunTurret:
        {
            Building *b = g_app->m_location->GetBuilding( m_objId.GetUniqueId() );
            if( b )
            {
                Team *team = g_app->m_location->m_teams[m_teamId];
                team->SelectUnit( -1, -1, m_objId.GetUniqueId() );
                g_app->m_camera->RequestTurretAimMode( b );
            }
            break;
        }*/

        default: g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
    }
}


void Task::Stop()
{
    bool stop = true;
    switch( m_type )
    {
        case GlobalResearch::TypeSquad:
        {    
            Unit *unit = g_app->m_location->GetUnit( m_objId );
            if( unit )
            {
                for( int i = 0; i < unit->m_entities.Size(); ++i )
                {
                    if( unit->m_entities.ValidIndex(i) )
                    {
                        Entity *entity = unit->m_entities[i];
                        int health = entity->m_stats[Entity::StatHealth];
                        entity->ChangeHealth( -1000 );
                    }
                }
            }
            break;
        }

        case GlobalResearch::TypeArmour:
            if( g_app->m_multiwiniaTutorial )
            {
                // stop the player killing their armour during the tutorial
                stop = false;
                break;
            }
        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeTank:
        {
            Entity *entity = (Entity *) g_app->m_location->GetEntity( m_objId );
            if( entity )
            {
                int health = entity->m_stats[Entity::StatHealth];
                entity->ChangeHealth( -1000 );
            }
            break;
        }

        case GlobalResearch::TypeHarvester:
        {
            Harvester *entity = (Harvester *) g_app->m_location->GetEntity( m_objId );
            if( entity && entity->m_type == Entity::TypeHarvester )
            {
                entity->m_vunerable = true;
                entity->ChangeHealth( -1000 );
            }
            break;
        }

        case GlobalResearch::TypeGunTurret:
        {
            Building *b = g_app->m_location->GetBuilding( m_objId.GetUniqueId() );
            if( b && b->m_id.GetTeamId() == m_teamId )
            {
                b->Damage( -1000.0f );
            }
            else
            {
                stop = false;
            }
            break;
        }
    }

    if( stop )
    {
        m_state = StateStopping;
    }
}


char *Task::GetTaskName( int _type )
{
    return GlobalResearch::GetTypeName( _type );
}


void Task::GetTaskNameTranslated( int _type, UnicodeString& _dest )
{
    return GlobalResearch::GetTypeNameTranslated( _type, _dest );
}


TaskManager::TaskManager( int _teamId )
:   m_nextTaskId(0),
    m_teamId(_teamId),
    m_currentTaskId(-1),
    m_verifyTargetting(true),
    m_mostRecentTaskId(-1),
    m_mostRecentTaskIdTimer(0.0),
    m_blockMostRecentPopup(false),
    m_previousTaskId(-1)
{
}


bool TaskManager::RunTask( Task *_task )
{
    if( CapacityUsed() < Capacity() )
    {
        _task->m_id = m_nextTaskId;
        ++m_nextTaskId;
        
        if( g_app->Multiplayer() )
        {
            // Insert the new task in front of all the not-yet-running tasks
            bool inserted = false;

            for( int i = 0; i < m_tasks.Size(); ++i )
            {
                if( m_tasks[i]->m_state == Task::StateStarted )
                {
                    m_tasks.PutDataAtIndex( _task, i );
                    inserted = true;
                    break;
                }
            }

            if( !inserted ) m_tasks.PutDataAtEnd( _task );
        }
        else
        {
            m_tasks.PutDataAtEnd( _task );
        }

        _task->Start();
        
        if( !g_app->Multiplayer() ) m_currentTaskId = _task->m_id;

        //if( !m_blockMostRecentPopup )
        {
            m_mostRecentTaskId = _task->m_id;
            m_mostRecentTaskIdTimer = GetHighResTime();

            TaskSource *ts = new TaskSource();
            ts->m_taskId = _task->m_id;
            ts->m_timer = GetHighResTime();
            ts->m_source = 0;
            m_recentTasks.PutData( ts );
        }

        if( g_app->Multiplayer() && 
            CapacityUsed() >= Capacity() &&
            m_teamId == g_app->m_globalWorld->m_myTeamId )
        {
            g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_powerup_limit"), 255, true );
        }

        return true;
    }
    else
    {
        if( g_app->Multiplayer() &&
            m_teamId == g_app->m_globalWorld->m_myTeamId )
        {
            g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_powerup_limit"), 255, true );
        }
        else if( m_teamId == g_app->m_globalWorld->m_myTeamId )
        {
            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5 );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_sepulveda->Say( "task_manager_full" );
#endif
        }

		delete _task;
    }

    return false;   
}


bool TaskManager::RunTask( int _type )
{
    switch( _type )
    {
        case GlobalResearch::TypeSquad:
        case GlobalResearch::TypeEngineer:
        case GlobalResearch::TypeOfficer:
#ifndef DEMOBUILD
        case GlobalResearch::TypeArmour:
#endif
        case GlobalResearch::TypeHarvester:
        case GlobalResearch::TypeShaman:
        case GlobalResearch::TypeTank:
        {
            Task *task = new Task( m_teamId );
            task->m_type = _type;
            bool success = RunTask( task );
            if( success )
            {
                if( !g_app->Multiplayer() ) g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
            }
            return success;
        }

        //case GlobalResearch::TypeController:
        //{
        //    Task *task = GetCurrentTask();
        //    if( task && task->m_type == GlobalResearch::TypeSquad )
        //    {
        //        Unit *unit = g_app->m_location->GetUnit( task->m_objId );
        //        if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
        //        {
        //            InsertionSquad *squad = (InsertionSquad *) unit;
        //            squad->SetWeaponType( _type );

        //            if( !GetTask( squad->m_controllerId ) )
        //            {
        //                Task *controller = new Task(m_teamId);
        //                controller->m_type = _type;
        //                controller->m_objId = WorldObjectId( squad->m_teamId, squad->m_unitId, -1, -1 );
        //                controller->m_route = new Route(-1);
        //                controller->m_route->AddWayPoint( squad->m_centrePos );
        //                bool success = RunTask( controller );
        //                if( success )
        //                {
        //                    squad->m_controllerId = controller->m_id;
        //                    SelectTask( task->m_id );
        //                    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, _type, 2.5 );
        //                }
        //                return success;
        //            }

        //            return true;
        //        }
        //    }

        //    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5 );
        //    g_app->m_sepulveda->Say( "task_manager_needsquadfirst" );
        //    return false;
        //}

        case GlobalResearch::TypeAirStrike:
            if( g_app->Multiplayer() )
            {
                Task *task = new Task( m_teamId );
                task->m_type = _type;
                bool success = RunTask( task );
                if( success )
                {
                    if( !g_app->Multiplayer() ) g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
                }
                return success;
                break;
            }
            // Deliberate fall through

        case GlobalResearch::TypeGrenade:
        case GlobalResearch::TypeRocket:
        case GlobalResearch::TypeController:
        {
            Task *task = GetCurrentTask();
            if( task && task->m_type == GlobalResearch::TypeSquad )
            {
                Unit *unit = g_app->m_location->GetUnit( task->m_objId );
                if( unit && unit->m_troopType == Entity::TypeInsertionSquadie )
                {
                    InsertionSquad *squad = (InsertionSquad *) unit;
                    squad->SetWeaponType( _type );
                    if( g_app->m_globalWorld->m_myTeamId == m_teamId )
                    {
                        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, _type, 2.5 );
                    }
                    return true;
                }
            }

            if( g_app->m_globalWorld->m_myTeamId == m_teamId )
            {
                g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageFailure, -1, 2.5 );
            }
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_sepulveda->Say( "task_manager_needsquadfirst" );
#endif
            return false;
        }

        case GlobalResearch::TypeNuke:
        case GlobalResearch::TypeSubversion:
        case GlobalResearch::TypeBooster:
        case GlobalResearch::TypeHotFeet:
        case GlobalResearch::TypeGunTurret:
        case GlobalResearch::TypeInfection:
        case GlobalResearch::TypeMagicalForest:
        case GlobalResearch::TypeDarkForest:
        case GlobalResearch::TypeAntsNest:
        case GlobalResearch::TypePlague:
        case GlobalResearch::TypeEggSpawn:
        case GlobalResearch::TypeMeteorShower:
        case GlobalResearch::TypeExtinguisher:
        case GlobalResearch::TypeRage:
        {
            Task *task = new Task( m_teamId );
            task->m_type = _type;
            bool success = RunTask( task );
            if( success )
            {
                if( !g_app->Multiplayer() ) g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
            }
            return success;
        }
    }

    return false;
}


bool TaskManager::RegisterTask( Task *_task )
{
    if( CapacityUsed() < Capacity() )
    {
        _task->m_id = m_nextTaskId;        
        ++m_nextTaskId;
        m_tasks.PutDataAtEnd( _task );        
        return true;
    }    

    return false;
}


bool TaskManager::TerminateTask( int _id, bool _showMessage )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_id == _id )
        {
            if( task->m_type == GlobalResearch::TypeShaman ) return false; // you can't terminate shamans
            if( task->m_type == GlobalResearch::TypeSquad )
            {
                bool controllerShutdown = task->ShutdownController();
                if( controllerShutdown ) 
                {
                    if( g_app->m_globalWorld->m_myTeamId == m_teamId )
                    {
                        g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, GlobalResearch::TypeController, 3.0 );
                    }
                    return true;
                }
            }

            if( task->m_type == GlobalResearch::TypeGunTurret )
            {
                if( g_app->m_multiwinia->GetGameOption("MaxTurrets") != -1 )
                {
                    Building *b = g_app->m_location->GetBuilding( task->m_objId.GetUniqueId() );
                    if( b && b->m_id.GetTeamId() != m_teamId )
                    {
                        return false;
                    }
                }
            }

            if( g_app->m_multiwiniaTutorial )
            {
                if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 ||
                    !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
                {
                    return false;
                }
            }

            if( _showMessage )
            {
                if( g_app->m_globalWorld->m_myTeamId == m_teamId )
                {
                    g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageShutdown, task->m_type, 3.0 );
                }
            }

            m_tasks.RemoveData(i);
            task->Stop();
            delete task;
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
            g_app->m_helpSystem->PlayerDoneAction( HelpSystem::TaskShutdown );
#endif

            if( m_currentTaskId == _id )
            {
                m_currentTaskId = -1;
            }

            return true;
        }
    }

    return false;
}


int TaskManager::GetTaskIndex( int _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_id == _id )
        {
            return i;
        }
    }

    return -1;
}


int TaskManager::Capacity()
{
    int taskManagerResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeTaskManager );
    int capacity = 1 + taskManagerResearch;

    if( g_app->Multiplayer() )
    {
        if( g_app->m_multiwinia->IsEliminated( m_teamId ) )
        {
            capacity = 2;
        }
        else
        {
            capacity = 6;
        }
    }

    return capacity;    
}


int TaskManager::CapacityUsed()
{
    return m_tasks.Size();    
}


int TaskManager::MapGestureToTask( int _gestureId )
{
    switch( _gestureId )
    {
        case 0 :        return GlobalResearch::TypeSquad;   
        case 1 :        return GlobalResearch::TypeEngineer;
        case 2 :        return GlobalResearch::TypeGrenade;
        case 3 :        return GlobalResearch::TypeRocket;
        case 4 :        return GlobalResearch::TypeAirStrike;
        case 5 :        return GlobalResearch::TypeController;
        case 6 :        return GlobalResearch::TypeOfficer;
        case 7 :        return GlobalResearch::TypeArmour;

        default :       return -1;
    }
}


void TaskManager::AdvanceTasks()
{
    //
    // Are we currently placing a task?

    Task *currentTask = GetCurrentTask();
    if( currentTask && currentTask->m_state == Task::StateStarted )
    {
        if( g_app->m_globalWorld->m_myTeamId == m_teamId )
        {
            g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageSuccess, currentTask->m_type, 2.5 );
        }
    }


    //
    // Advance all other tasks

    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        bool amIDone = task->Advance();
        if( amIDone )
        {
            if( m_currentTaskId == task->m_id )
            {
                m_currentTaskId = -1;
                g_app->m_location->m_teams[m_teamId]->SelectUnit( -1, -1, -1 );
            }
    
            m_tasks.RemoveData(i);
            --i;
            delete task;
        }
    }
}


void TaskManager::StopAllTasks()
{
    m_tasks.EmptyAndDelete();
    m_currentTaskId = -1;
}


void TaskManager::Advance()
{
    //
    // Sort the tasks so that running tasks are at the front
    // and not-yet-run tasks are at the back

    if( g_app->Multiplayer() )
    {
        for( int i = m_tasks.Size()-1; i > 0; --i )
        {
            Task *thisTask = m_tasks[i];
            Task *prevTask = m_tasks[i-1];

            if( thisTask->m_state == Task::StateRunning &&
                prevTask->m_state == Task::StateStarted )
            {
                m_tasks.RemoveData(i-1);
                m_tasks.PutDataAtIndex( prevTask, i );
            }
        }
    }



    AdvanceTasks();

    g_app->m_globalWorld->m_research->AdvanceResearch();    

    if( !g_app->Multiplayer() &&
        m_mostRecentTaskId != -1 )
    {
        double timeNow = GetHighResTime();
        if( timeNow > m_mostRecentTaskIdTimer + 1.0 )
        {
            m_mostRecentTaskId = -1;
            m_mostRecentTaskIdTimer = 0.0;
        }
    }

    if( g_app->Multiplayer() && m_mostRecentTaskId != -1 )
    {
        double timeNow = GetHighResTime();
        if( m_mostRecentTaskIdTimer > 0.0f &&
            timeNow > m_mostRecentTaskIdTimer + 3.0f )
        {
            m_mostRecentTaskIdTimer = -timeNow;
        }
    }

    if( g_app->Multiplayer() )
    {
        double timeNow = GetHighResTime();

        for( int i = 0; i < m_recentTasks.Size(); ++i )
        {
            if( m_recentTasks.ValidIndex(i) )
            {
                TaskSource *ts = m_recentTasks[i];

                if( ts->m_timer > 0.0f && timeNow > ts->m_timer + 3.0f )
                {
                    ts->m_timer = -timeNow;
                }

                if( ts->m_timer < 0.0f && timeNow > (-ts->m_timer) + 1.0f )
                {
                    m_recentTasks.RemoveData(i);
                    delete ts;
                }
            }
        }
    }
}


void TaskManager::NotifyRecentTaskSource( int source )
{
    for( int i = 0; i < m_recentTasks.Size(); ++i )
    {
        if( m_recentTasks.ValidIndex(i) )
        {
            TaskSource *ts = m_recentTasks[i];

            if( ts->m_taskId == m_mostRecentTaskId )
            {
                ts->m_source = source;
            }
        }
    }
}


void TaskManager::SelectTask( int _id )
{
    m_currentTaskId = _id;

    if( m_currentTaskId != -1 )
    {
        m_previousTaskId = _id;

        int currentIndex = -1;
        for( int i = 0; i < m_tasks.Size(); ++i )
        {
            Task *task = m_tasks[i];
            if( task->m_id == m_currentTaskId )
            {
                currentIndex = i;
                break;
            }
        }
 
        if( currentIndex == -1 ) return;
        AppReleaseAssert( currentIndex != -1, "Error in TaskManager::SelectTask. Tried to select a task that doesn't exist." );

        Task *task = m_tasks[ currentIndex ];
        //m_tasks.RemoveData(currentIndex);
        //m_tasks.PutDataAtStart( task );
        task->SwitchTo();

		g_app->m_gameCursor->BoostSelectionArrows(2.0);
    }
}


void TaskManager::SelectTask( WorldObjectId _id )
{    
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_objId.GetTeamId() == _id.GetTeamId() &&
            task->m_objId.GetUnitId() == _id.GetUnitId() &&
            task->m_objId.GetIndex() == _id.GetIndex() )
        {
            SelectTask( task->m_id );
            return;
        }
    }

    // Not found
    m_currentTaskId = -1;
}


int TaskManager::GetNextTaskId()
{
    if( m_tasks.Size() > 0 )
    {
        int currentIndex = -1;
        for( int i = 0; i < m_tasks.Size(); ++i )
        {
            Task *task = m_tasks[i];
            if( task->m_id == m_currentTaskId )
            {
                currentIndex = i;
                break;
            }
        }

        // It is ok for currentIndex to still be -1 here
        // (ie no previous selected task)
        // Which would result in task 0 being selected
        
        int nextIndex = currentIndex+1;
        if( nextIndex >= m_tasks.Size() )
        {
            nextIndex = 0;
        }

        Task *nextTask = m_tasks[nextIndex];
        return nextTask->m_id;  
    }

    return -1;
}


Task *TaskManager::GetCurrentTask ()
{
    return GetTask( m_currentTaskId );
}


Task *TaskManager::GetTask( int _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_id == _id )
        {
            return task;
        }
    }    

    return NULL;
}



Task *TaskManager::GetTask( WorldObjectId _id )
{
    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_objId.GetTeamId() == _id.GetTeamId() &&
            task->m_objId.GetUnitId() == _id.GetUnitId() &&
            task->m_objId.GetIndex() == _id.GetIndex() )
        {
            return task;
        }
    }    

    return NULL;    
}


bool TaskManager::TargetTask( int _id, Vector3 const &_pos )
{
	if (IsValidTargetArea(_id, _pos))
	{
		Task *task = GetTask( _id );
        task->Target( _pos );

        // You can't do anything more here with task
        // Because task->Target may have deleted this task object
        // So look it up again

        task = GetTask( _id );
        if( task && task->m_objId.IsValid() )
        {
            if( task->m_type != GlobalResearch::TypeGunTurret )
            {
                g_app->m_markerSystem->RegisterMarker_Task( task->m_objId );
            }
        }

		return true;
	}

	return false;
}


bool TaskManager::IsValidTargetArea( int _id, Vector3 const &_pos )
{
    Task *task = GetTask( _id );
    if( !task ) return false;

    if( !g_app->m_location || !g_app->m_location->m_landscape.IsInLandscape( _pos ) ) return false;
    
    if( m_verifyTargetting )
    {
        if( task->m_type == GlobalResearch::TypeOfficer )
        {
            int numFound;
            g_app->m_location->m_entityGrid->GetFriends( s_neighbours, _pos.x, _pos.z, 10.0, &numFound, m_teamId );
            bool foundDarwinian = false;
            for( int i = 0; i < numFound; ++i )
            {
                WorldObjectId id = s_neighbours[i];
                Entity *ent = g_app->m_location->GetEntity( id );
                if( ent && ent->m_type == Entity::TypeDarwinian )
                {
                    foundDarwinian = true;
                    break;
                }
            }
            return foundDarwinian;
        }
        else if( task->m_type == GlobalResearch::TypeGunTurret  )
            //g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig )
        {
            if( RestrictionZone::IsRestricted( _pos, true ) ) return false;

            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex( i ) )
                {
                    Building *b = g_app->m_location->m_buildings[i];
                    if( b )
                    {
                        if( b->DoesSphereHit( _pos, 10.0f ) )
                        {
                            return false;
                        }

                        if( b->m_id.GetTeamId() != 255)
                        {
                            bool invalid = false;
                            if( !g_app->m_location->IsFriend( b->m_id.GetTeamId(), m_teamId ) )
                            {
                                float dist = (b->m_pos - _pos).Mag();
                                if( b->m_type == Building::TypeTrunkPort && 
                                    dist < 400.0f )
                                {
                                    invalid = true;
                                }

                                if( b->m_type == Building::TypeSpawnPoint &&
                                    dist < 200.0f )
                                {
                                    invalid = true;
                                }

                                if( invalid ) 
                                {
                                    return false;
                                }
                            }
                        }
                    }
                }
            }
            return true;
        }
        else if( task->m_type == GlobalResearch::TypeBooster ||
                 task->m_type == GlobalResearch::TypeHotFeet ||
                 task->m_type == GlobalResearch::TypeSubversion ||
                 task->m_type == GlobalResearch::TypeRage )
        {
            bool include[NUM_TEAMS];
            memset(include, false, sizeof(include));
            include[m_teamId] = true;
            int numFriends = g_app->m_location->m_entityGrid->GetNumNeighbours( _pos.x, _pos.z, POWERUP_EFFECT_RANGE, include );

            if( numFriends > 0 )
            {
                return true;
            }
            else 
            {
                return false;
            }
        }
        else
        {
            LList <TaskTargetArea> *targetAreas = GetTargetArea( _id );
            bool success = false;

            for( int i = 0; i < targetAreas->Size(); ++i )
            {
                TaskTargetArea *targetArea = targetAreas->GetPointer(i);
                if( (_pos - targetArea->m_centre).Mag() <= targetArea->m_radius )
                {
                    if( targetArea->m_centre == Vector3(0,0,0) )
                    {
                        if( RestrictionZone::IsRestricted( _pos, true ) ) continue;
                    }
                    success = true;
                    break;
                }
            }
        
            delete targetAreas;
            return success;
        }
    }
    else
    {
        return true;
    }
    
    return false;
}


int TaskManager::CountNumTasks( int _type )
{
    int result =0;

    for( int i = 0; i < m_tasks.Size(); ++i )
    {
        Task *task = m_tasks[i];
        if( task->m_type == _type )
        {
            ++result;
        }
    }

    return result;
}


LList <TaskTargetArea> *TaskManager::GetTargetArea( int _id )
{
    LList<TaskTargetArea> *result = new LList<TaskTargetArea>();

    Task *task = GetTask( _id );
    if( task )
    {
        switch( task->m_type )
        {
            case GlobalResearch::TypeEngineer:
            case GlobalResearch::TypeHarvester:
            {
                Team *team = g_app->m_location->m_teams[m_teamId];
                for( int i = 0; i < team->m_units.Size(); ++i )
                {
                    if( team->m_units.ValidIndex(i) )
                    {
                        Unit *unit = team->m_units[i];
                        if( unit->m_troopType == Entity::TypeInsertionSquadie ||
                            unit->m_troopType == Entity::TypeEngineer )
                        {
                            TaskTargetArea tta;
                            tta.m_centre = unit->m_centrePos;
                            tta.m_radius = 100.0;
                            tta.m_stationary = false;
                            result->PutData( tta );
                        }
                    }
                }

				if (g_app->IsSinglePlayer())
				{
					for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
					{
						if( g_app->m_location->m_buildings.ValidIndex(i) )
						{
							Building *building = g_app->m_location->m_buildings[i];
							if( building &&
								building->m_id.GetTeamId() == m_teamId )
							{
								if( building->m_type == Building::TypeControlTower)
								{
									TaskTargetArea tta;
									tta.m_centre = building->m_pos;
									tta.m_radius = 75.0;
									tta.m_stationary = true;
									result->PutData( tta );
								}
							}                            
						}
					}
				}
                //break;                // DELIBERATE FALL THROUGH
            }

            case GlobalResearch::TypeArmour:
            case GlobalResearch::TypeTank:
            {
                if( !g_app->Multiplayer() )
                {
                    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                    {
                        if( g_app->m_location->m_buildings.ValidIndex(i) )
                        {
                            Building *building = g_app->m_location->m_buildings[i];
                            if( building &&
                                building->m_type == Building::TypeTrunkPort &&
                                ((TrunkPort *)building)->m_openTimer > 0.0 )
                            {
                                TaskTargetArea tta;
                                tta.m_centre = building->m_pos;
                                tta.m_radius = 120.0;
                                tta.m_stationary = true;
                                result->PutData( tta );
                            }                            
                        }
                    }
                    break;
                }
                // DELIBERATE FALL THROUGH
            }


            case GlobalResearch::TypeSquad:
                for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
                {
                    if( g_app->m_location->m_buildings.ValidIndex(i) )
                    {
                        Building *building = g_app->m_location->m_buildings[i];
                        if( building &&
                            g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_teamId ) )
                        {
                            bool match = false;

                            if( g_app->IsSinglePlayer() )
                            {
                                if( building->m_type == Building::TypeControlTower ) match = true;
                            }

                            if( g_app->Multiplayer() )
                            {
                                if( building->m_type == Building::TypeSpawnPoint ||
                                    building->m_type == Building::TypeGunTurret ||
                                    building->m_type == Building::TypeMultiwiniaZone ||
                                    building->m_type == Building::TypeTrunkPort)
                                {
                                    match = true;
                                }
                            }

                            if( match )
                            {
                                TaskTargetArea tta;
                                tta.m_centre = building->m_pos;
                                tta.m_radius = 75.0;
                                tta.m_stationary = true;
                                result->PutData( tta );
                            }
                        }                            
                    }
                }
                break;

            case GlobalResearch::TypeOfficer:
            case GlobalResearch::TypeAirStrike:
            case GlobalResearch::TypeNuke:
            case GlobalResearch::TypeSubversion:
            case GlobalResearch::TypeBooster:
            case GlobalResearch::TypeHotFeet:
            case GlobalResearch::TypeGunTurret:
            case GlobalResearch::TypeShaman:
            case GlobalResearch::TypeInfection:
            case GlobalResearch::TypeMagicalForest:
            case GlobalResearch::TypeDarkForest:
            case GlobalResearch::TypeAntsNest:
            case GlobalResearch::TypePlague:
            case GlobalResearch::TypeEggSpawn:
            case GlobalResearch::TypeMeteorShower:
            case GlobalResearch::TypeExtinguisher:
            case GlobalResearch::TypeRage:
            case GlobalResearch::TypeDropShip:
            {
                TaskTargetArea tta;
                tta.m_centre.Set(0, 0, 0);
                tta.m_radius = 99999.9;
                result->PutData( tta );
                break;
            }
        }
    }

    return result;
}
