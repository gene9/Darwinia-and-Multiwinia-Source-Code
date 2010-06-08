#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"
#include "lib/preferences.h"
#include "lib/text_renderer.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "entity_grid.h"
#include "entity_grid_cache.h"
#include "main.h"
#include "unit.h"
#include "particle_system.h"
#include "taskmanager.h"
#include "routing_system.h"
#include "renderer.h"
#include "global_world.h"
#include "level_file.h"
#include "obstruction_grid.h"
#include "multiwinia.h"
#include "multiwiniahelp.h"

#include "sound/soundsystem.h"

#include "worldobject/darwinian.h"
#include "worldobject/officer.h"
#include "worldobject/teleport.h"
#include "worldobject/armyant.h"
#include "worldobject/armour.h"
#include "worldobject/ai.h"
#include "worldobject/laserfence.h"
#include "worldobject/goddish.h"
#include "worldobject/rocket.h"
#include "worldobject/carryablebuilding.h"
#include "worldobject/shaman.h"
#include "worldobject/souldestroyer.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/portal.h"
#include "worldobject/tree.h"
#include "worldobject/crate.h"
#include "worldobject/dustball.h"
#include "worldobject/spaceship.h"
#include "worldobject/jumppad.h"
#include "worldobject/multiwiniazone.h"



Darwinian::Darwinian()
:   Entity(),
    m_state(StateIdle),
    m_retargetTimer(0.0),
    m_spiritId(-1),
    m_buildingId(-1),
    m_portId(-1),
    m_controllerId(-1),
    m_teleportRequired(false),
    m_ordersBuildingId(-1),
    m_ordersSet(false),
    m_promoted(false),
    m_scared(true),
    m_shadowBuildingId(-1),
    m_threatRange(DARWINIAN_SEARCHRANGE_THREATS),
    m_grenadeTimer(0.0),
    m_officerTimer(0.0),
    m_sacraficeTimer(0.0),
    m_burning(false),
    m_boosterTaskId(-1),
    m_infectionState(InfectionStateClean),
    m_infectionTimer(0.0),
    m_paralyzeTimer(-1.0),
    m_insane(false),
    m_underPlayerControl(false),
    m_subversionTaskId(-1),
    m_previousTeamId(-1),
    m_colourTimer(-1.0),
    m_hotFeetTaskId(-1),
    m_targetPortal(-1),
    m_followingPlayerOrders(false),
    m_rageTaskId(-1),
    m_fireDamage(0.0),
    m_idleTimer(0.0),
    m_futurewinian(false),
	m_startedThreatSound(false),
    m_manuallyApproachingArmour(false),
    m_wallHits(0),
    m_closestAITarget(-1),
	m_scaleFactor(1.0)
{
    SetType( TypeDarwinian );
    m_grenadeTimer = syncfrand( 5.0 );
    m_idleTimer = 60 + syncfrand( 60.0 );
}

void Darwinian::Begin()
{
    Entity::Begin();
    m_onGround = true;   
    m_wayPoint = m_pos;
    m_centrePos.Set(0,2,0);
    m_radius = 4.0;
	
	m_scaleFactor = g_prefsManager->GetFloat( "DarwinianScaleFactor" );	// DEF: Optimization
	
    int futurewinianTeam = g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionFuturewinianTeam];
    if( m_id.GetTeamId() == g_app->m_location->GetFuturewinianTeamId() ||
        (futurewinianTeam != -1 &&
         futurewinianTeam == g_app->m_location->GetTeamPosition( m_id.GetTeamId() ) ) )
    {
        m_futurewinian = true;
    }
}


bool Darwinian::ChangeHealth( int _amount, int _damageType )
{
    if( m_state == StateInsideArmour ||
        m_state == StateBeingSacraficed )
    {
        if( _damageType != DamageTypeUnresistable )
        {
            // We are invincible in here
            return false;
        }
    }

    if( m_boosterTaskId != -1 && _damageType != DamageTypeUnresistable )
    {
        if( _damageType == DamageTypeExplosive )
        {
            m_boosterTaskId = -1;
        }
        return false;
    }

    if( IsInFormation() && !m_futurewinian )
    {
        if( _damageType == DamageTypeExplosive )
        {
            _amount *= 0.6;
        }
    }

    if( m_rageTaskId != -1 ) _amount *= 0.75;
    if( m_futurewinian ) _amount *= 2.0;

    bool dead = m_dead;

    Entity::ChangeHealth( _amount );

    if( !dead && m_dead )
    {
    }
	

    return true;
}

void Darwinian::FireDamage( int _amount, int _teamId )
{
	SyncRandLog( "Darwinian::FireDamage: amount = %d, teamId = %d, m_fireDamage0 = %f\n",
		_amount, _teamId, m_fireDamage );
    m_fireDamage += _amount;
    if( m_fireDamage >= 100.0 )
    {
        SetFire();

        if( _teamId >= 0 && _teamId < NUM_TEAMS )
        {
            g_app->m_location->m_teams[_teamId]->m_teamKills++;
        }
    }
	SyncRandLog( "Darwinian::FireDamage: m_fireDamage1 = %f\n", m_fireDamage );
}

bool Darwinian::SearchForNewTask()
{
    START_PROFILE( "SearchingForTask");
    if( m_state == StateIdle && m_controllerId != -1 )
    {
        m_state = StateUnderControl;
        END_PROFILE(  "SearchingForTask");
        return true;
    }
    
    if( m_followingPlayerOrders &&
        m_state != StateIdle  )
    {
        bool found = SearchForThreats();
        END_PROFILE(  "SearchingForTask");
        return found;
    }

    bool newTargetFound = false;
    
    switch( m_state )
    {
        case StateIdle:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            if( !newTargetFound )       newTargetFound = SearchForCarryableBuilding();
            if( !newTargetFound )       newTargetFound = SearchForCrates();
            if( !newTargetFound )       newTargetFound = SearchForArmour();
            if( !newTargetFound && g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig ) 
                                        newTargetFound = SearchForFlags();
            if( !newTargetFound )       newTargetFound = SearchForOfficers();
			//if( !newTargetFound )		newTargetFound = SearchForShamans();
            if( !newTargetFound )       newTargetFound = SearchForSpirits();
            if( !newTargetFound )       newTargetFound = SearchForRandomPosition();
            break;

        case StateWorshipSpirit:
        case StateWatchingSpectacle:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            if( !newTargetFound )       newTargetFound = SearchForCarryableBuilding();
            if( !newTargetFound )       newTargetFound = SearchForArmour();
            if( !newTargetFound )       newTargetFound = SearchForOfficers();
            break;
            
        case StateApproachingArmour:
        case StateFollowingOrders:
        case StateFollowingOfficer:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            if( !newTargetFound )       newTargetFound = SearchForCarryableBuilding();
            break;

        case StateOperatingPort:
            if( !g_app->Multiplayer() )
            {
                if( !newTargetFound )       newTargetFound = SearchForThreats();
            }
            break;

        case StateApproachingPort:
        case StateWatchingGodDish:
        case StateOpeningCrate:
        case StateUnderControl:
        case StateCombat:
        case StateApproachingCarriableBuilding:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            break;

        case StateBoardingRocket:
        case StateOnFire:
        case StateCarryingBuilding:
            break;
    }

    END_PROFILE(  "SearchingForTask");

    return newTargetFound;

}


bool Darwinian::Advance( Unit *_unit )
{
    if( m_destroyed ) return true;
    if( m_promoted )
    {
        return true;
    }
    
    bool amIDead = Entity::Advance( _unit );
    
    if( !amIDead && !m_dead && m_onGround && m_state != StateOnFire )
    {
        //
        // Has something higher priority come along?

        if( m_onGround )
        {
            m_retargetTimer -= SERVER_ADVANCE_PERIOD;
            if( m_retargetTimer <= 0.0 )
            {
                bool newTaskFound = SearchForNewTask();

                if( m_state == StateIdle )
                {
                    bool victoryDance = BeginVictoryDance();
                }

                m_retargetTimer = 1.0 + syncfrand(1.0);
            }
        }


        if( g_app->Multiplayer() )//&& m_state != StateIdle )
        {
            // Keep our idle timer high when they are busy
            m_idleTimer = 60.0;// + syncfrand(60.0);
        }

        //
        // Do what we're supposed to do

        switch( m_state )
        {
            case StateIdle :                amIDead = AdvanceIdle();                break;
            case StateApproachingPort :     amIDead = AdvanceApproachingPort();     break;
            case StateOperatingPort :       amIDead = AdvanceOperatingPort();       break;
            case StateApproachingArmour :   amIDead = AdvanceApproachingArmour();   break;
            case StateInsideArmour :        amIDead = AdvanceInsideArmour();        break;
            case StateWorshipSpirit :       amIDead = AdvanceWorshipSpirit();       break;
            case StateUnderControl :        amIDead = AdvanceUnderControl();        break;
            case StateFollowingOrders :     amIDead = AdvanceFollowingOrders();     break;
            case StateFollowingOfficer :    amIDead = AdvanceFollowingOfficer();    break;
            case StateCombat :              amIDead = AdvanceCombat();              break;
            case StateCapturedByAnt :       amIDead = AdvanceCapturedByAnt();       break;
            case StateWatchingSpectacle :   amIDead = AdvanceWatchingSpectacle();   break;
            case StateBoardingRocket :      amIDead = AdvanceBoardingRocket();      break;
            case StateAttackingBuilding :   amIDead = AdvanceAttackingBuilding();   break;
            case StateCarryingBuilding:     amIDead = AdvanceCarryingBuilding();    break;
			case StateFollowingShaman:		amIDead = AdvanceFollowingShaman();		break;
            case StateOpeningCrate:         amIDead = AdvanceOpeningCrate();        break;
            case StateRaisingFlag:          amIDead = AdvanceRaisingFlag();         break;

            case StateApproachingCarriableBuilding: amIDead = AdvanceApproachingCarriableBuilding();        break;
        }
    }

    if( m_state == StateBeingSacraficed ) amIDead = AdvanceSacraficing();
    if( m_state == StateInVortex )        amIDead = AdvanceInVortex();
    if( m_state == StateBeingAbducted )   amIDead = AdvanceBeingAbducted();

    if( m_boxKiteId.IsValid() && (m_state != StateWorshipSpirit || m_dead) )
    {
        BoxKite *boxKite = (BoxKite *) g_app->m_location->GetEffect( m_boxKiteId );
        if ( boxKite )
			boxKite->Release();
        m_boxKiteId.SetInvalid();
    }

    
    if( m_controllerId != -1 && !m_dead )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Task *task = NULL;
        if( g_app->Multiplayer() ) task = team->m_taskManager->GetTask( m_controllerId );
        else                       task = g_app->m_location->GetMyTaskManager()->GetTask( m_controllerId );

        InsertionSquad *controller = NULL;
        if( task ) controller = (InsertionSquad *)g_app->m_location->GetUnit( task->m_objId );    

        if( task && controller )
        {
            controller->m_numControlledThisFrame++;   
        }        
    }


    if( !m_onGround &&
        m_state != StateBeingSacraficed &&
        m_state != StateBeingAbducted ) AdvanceInAir( _unit );       

    if( m_state == StateOnFire && !amIDead && !m_dead )
    {
        amIDead = AdvanceOnFire();
    }

    if( m_state == StateInWater && !amIDead && !m_dead )
    {
        amIDead = AdvanceInWater();
    }

    if( m_infectionState == InfectionStateInfected && !amIDead && !m_dead )
    {
        amIDead = AdvanceInfection();
    }

    if( m_pos.y < 0.0 &&         
        m_state != StateInsideArmour ) 
    {
        m_state = StateInWater;
        m_onGround = false;
    }

    if( m_dead && m_onGround )
    {
        m_vel *= 0.9;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    }

    if( m_boosterTaskId != -1 )
    {
        Task *t = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTask( m_boosterTaskId );
        if( !t || t->m_type != GlobalResearch::TypeBooster )
        {
            m_boosterTaskId = -1;
        }
    }

    if( m_hotFeetTaskId != -1 )
    {
        // give us a fire trail
        AdvanceHotFeet();
        Task *t = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTask( m_hotFeetTaskId );
        if( !t || t->m_type != GlobalResearch::TypeHotFeet )
        {
            m_hotFeetTaskId = -1;
            m_stats[StatSpeed] = EntityBlueprint::GetStat( Entity::TypeDarwinian, StatSpeed );
        }
    }

    if( m_subversionTaskId != -1 )
    {
        Task *t = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTask( m_subversionTaskId );
        if( !t || t->m_type != GlobalResearch::TypeSubversion )
        {
            m_subversionTaskId = -1;
        }
    }

    if( m_rageTaskId != -1 )
    {
        Task *t = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTask( m_rageTaskId );
        if( !t || t->m_type != GlobalResearch::TypeRage )
        {
            m_rageTaskId = -1;
            m_stats[StatSpeed] = EntityBlueprint::GetStat( Entity::TypeDarwinian, StatSpeed );
        }
    }

    if( m_paralyzeTimer > 0.0 )
    {
        m_paralyzeTimer -= SERVER_ADVANCE_PERIOD;
        if( m_paralyzeTimer <= 0.0 )
        {
            m_paralyzeTimer = -1.0;
        }
    }

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault &&
        !m_dead )
    {
        if( PulseWave::s_pulseWave )
        {
            if( !g_app->m_location->IsFriend( PulseWave::s_pulseWave->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                if( (m_pos - PulseWave::s_pulseWave->m_pos).Mag() < PulseWave::s_pulseWave->m_radius )
                {
                    if( g_app->m_location->GetTeamPosition(PulseWave::s_pulseWave->m_id.GetTeamId()) == g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionFuturewinianTeam] )
                    {
                        g_app->m_location->SpawnEntities( m_pos, PulseWave::s_pulseWave->m_id.GetTeamId(), -1, Entity::TypeDarwinian, 1, m_vel, 0.0 );
                        amIDead = true;
                        m_dead = true;
                    }
                    else
                    {
                        m_vel = (m_pos - PulseWave::s_pulseWave->m_pos).SetLength(20.0);
                        ChangeHealth(-1000);
                    }
                }
            }
        }
    }

    if( amIDead && m_state == StateInsideArmour )
    {
        Armour *a = (Armour *)g_app->m_location->GetEntitySafe( m_armourId, Entity::TypeArmour );
        if( a )
        {
            a->RemovePassenger();
        }

    }

    return amIDead;
}


int Darwinian::FindNearestUsefulBuilding( double &distance )
{
    int nearest = -1;
    double nearestDistance = 999999;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];

            if( building->m_type == Building::TypeSpawnPoint ||
                building->m_type == Building::TypeMultiwiniaZone ||
                building->m_type == Building::TypeCrate ||
                building->m_type == Building::TypeStatue ||
                building->m_type == Building::TypeGunTurret ||
                building->m_type == Building::TypeSolarPanel )               
            {
                double distance = ( building->m_pos - m_pos ).Mag();
                if( distance < nearestDistance )
                {
                    nearest = building->m_id.GetUniqueId();
                    nearestDistance = distance;
                }
            }
        }
    }

    distance = nearestDistance;
    return nearest;
}


WorldObjectId Darwinian::FindNearestUsefulOfficer( double &distance )
{
    Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];


    //
    // Search for nearest walkable officer in GOTO mode

    double nearestDistance = 999999;
    WorldObjectId nearestId;

    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity && 
            !entity->m_dead && 
            entity->m_type == Entity::TypeOfficer )
        {
            Officer *officer = (Officer *)entity;

            if( officer->m_orders == Officer::OrderGoto ||
                officer->m_orders == Officer::OrderFollow )
            {
                double distance = ( entity->m_pos - m_pos ).Mag();

                if( distance < nearestDistance )
                {
                    nearestId = id;
                    nearestDistance = distance;
                }
            }
        }
    }    

    distance = nearestDistance;
    return nearestId;
}


bool Darwinian::AdvanceIdle()
{
    if( m_onGround )
    {
        AdvanceToTargetPosition();
    }


    //
    // Count down a seperate idle timer.
    // If that timer hits zero, start looking for something
    // useful to do. (Only in Multiwinia)

    if( g_app->Multiplayer() )
    {
        m_idleTimer -= SERVER_ADVANCE_PERIOD;
        if( m_idleTimer <= 0.0 )
        {
            m_idleTimer = 60.0 + syncfrand(60.0);
            bool somethingNearby = false;
            bool newTargetFound = false;
            
            //
            // Look for a useful building nearby 

            double distance;
            int nearestBuildingId = FindNearestUsefulBuilding( distance );

            if( nearestBuildingId != -1 && distance < 500 ) newTargetFound = true;

            if( distance > 100 && distance < 500 )
            {
                Building *building = g_app->m_location->GetBuilding( nearestBuildingId );
                if( building )
                {
                    m_wayPoint = building->m_pos;
                    newTargetFound = true;
                }
            }

            //
            // Look for a useful Officer nearby

            if( !newTargetFound && !somethingNearby )
            {
                WorldObjectId officerId = FindNearestUsefulOfficer( distance );

                if( officerId.IsValid() && distance > 100 && distance < 500 )
                {            
                    Officer *officer = (Officer *)g_app->m_location->GetEntitySafe( officerId, Entity::TypeOfficer );
                    m_wayPoint = officer->m_pos;
                    newTargetFound = true;
                }
            }

            if( newTargetFound )
            {
                double positionError = 40.0;
                double radius = syncfrand(positionError);
                double theta = syncfrand(M_PI * 2);
                m_wayPoint.x += radius * iv_sin(theta);
                m_wayPoint.z += radius * iv_cos(theta);            
                m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );               
    
                if( !g_app->m_location->IsWalkable( m_pos, m_wayPoint, true ) )
                {
                    m_wayPoint.Zero();
                }
                else
                {
                    GiveOrders( m_wayPoint );
                }
            }
        }   
    }


    //
    // If we were previously under control by a squad
    // go back to that now

    if( m_controllerId != -1 )
    {
        m_state = StateUnderControl;
    }

    return false;
}


bool Darwinian::AdvanceWatchingSpectacle()
{
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building || 
        (building->m_type != Building::TypeGodDish &&
         building->m_type != Building::TypeEscapeRocket) )
    {
        m_state = StateIdle;
        return false;
    }


    //
    // Face the spectacle
    
    double amountToTurn = SERVER_ADVANCE_PERIOD * 4.0;
    Vector3 targetPos = building->m_centrePos;
    targetPos += Vector3( iv_sin(g_gameTime) * 30.0,
                          iv_cos(g_gameTime) * 20.0,
                          iv_sin(g_gameTime) * 25.0 );
    Vector3 targetDir = (targetPos - m_pos).Normalise();


    //
    // Is it a god dish?

    if( building->m_type == Building::TypeGodDish )
    {
        GodDish *dish = (GodDish *) building;
        if( !dish->m_activated && dish->m_timer < 1.0 )
        {
            m_state = StateIdle;
            return false;
        }

        m_front = targetDir;
        m_vel.Zero();
    }
    
     
    //
    // Is it an escape rocket?

    if( building->m_type == Building::TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;
        if( !rocket->IsSpectacle() )
        {
            m_state = StateIdle;
            return false;
        }
        
        double minDistance = 200.0;
        if( g_app->Multiplayer() ) minDistance = 100.0;

        double distance = ( building->m_pos - m_pos ).Mag();
        if( distance < minDistance )
        {
            Vector3 moveVector = (m_pos - building->m_pos);
            moveVector.SetLength( minDistance + syncfrand(minDistance/2.0) );
            m_wayPoint = building->m_pos + moveVector;
        }

        bool arrived = AdvanceToTargetPosition();
        if( arrived )
        {
            m_front = targetDir;
            m_vel.Zero();
        }
    }
    
    return false;
}


void Darwinian::WatchSpectacle( int _buildingId )
{
    m_buildingId = _buildingId;
    m_state = StateWatchingSpectacle;
}

void Darwinian::CastShadow( int _buildingId )
{
    m_shadowBuildingId = _buildingId;
}


bool Darwinian::AdvanceApproachingCarriableBuilding()
{
    //
    // Does the building still exist?

    CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->GetBuilding( m_buildingId );

    if( !carryable || 
        !CarryableBuilding::IsCarryableBuilding(carryable->m_type) )
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }


    //
    // If the building is owned by us, that means there are enough to lift
    // So do so!

    if( carryable->m_id.GetTeamId() == m_id.GetTeamId() )
    {
        m_state = StateCarryingBuilding;
        return false;
    }


    //
    // Walk towards the building if we are too far away

    double distance = ( carryable->m_pos - m_wayPoint ).Mag();    
    if( distance > 60 )
    {
        m_wayPoint = carryable->m_pos;
        Vector3 offset( 30, 0, 0 );
        offset.RotateAroundY( syncfrand( 2.0 * M_PI ) );
        m_wayPoint += offset;        
    }


    bool arrived = AdvanceToTargetPosition();

    if( arrived )
    {
        SearchForRandomPosition();
    }

    return false;
}


bool Darwinian::AdvanceCarryingBuilding()
{
    //
    // Does the building still exist?

    CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->GetBuilding( m_buildingId );

    if( !carryable || 
        !CarryableBuilding::IsCarryableBuilding(carryable->m_type) )
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }

    if( carryable->m_id.GetTeamId() != m_id.GetTeamId() )
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }


    //
    // Walk towards the carryable building, try to keep up

    Vector3 targetPos = carryable->GetCarryPosition( m_id.GetUniqueId() );

    if( targetPos == g_zeroVector )
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }

    m_wayPoint = targetPos;
    bool lifting = AdvanceToTargetPosition();
    
    if( !lifting )
    {
        double distance = ( targetPos - m_pos ).Mag();
        double max = 20.0;
        max *= g_gameTimer.GetGameSpeed();
        max *= ((double)m_stats[Entity::StatSpeed] / EntityBlueprint::GetStat( Entity::TypeDarwinian, Entity::StatSpeed ));
        if( distance < max ) lifting = true;
    }
    
    if( lifting )
    {
        carryable->m_numLiftersThisFrame++;
        
        double percentLifted = carryable->m_numLifters / (double)carryable->m_maxLifters;
        if( percentLifted > 1.0 ) percentLifted = 1.0;
        int soundFrequency = 15 + int( ( 1.0 - percentLifted ) * 20.0 );

        if( g_lastProcessedSequenceId % soundFrequency == 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LiftBuilding" );
        }
        else if( g_lastProcessedSequenceId % soundFrequency == int(soundFrequency/2) )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrdersFromPlayer" );
        }
    }

    return false;
}

bool Darwinian::AdvanceApproachingArmour()
{
    //
    // Is our armour still alive / within range / open

    Armour *armour = (Armour *) g_app->m_location->GetEntity( m_armourId );
    if( !armour || (!armour->IsLoading() && !m_manuallyApproachingArmour) )
    {
        m_state = StateIdle;
        m_armourId.SetInvalid();
        m_manuallyApproachingArmour = false;
        return false;
    }

    if( armour->GetNumPassengers() >= armour->Capacity() )
    {
        m_state = StateIdle;
        m_armourId.SetInvalid();
        m_manuallyApproachingArmour = false;
        return false;
    }

    Vector3 exitPos, exitDir;
    armour->GetEntrance( exitPos, exitDir );

    double distance = ( exitPos - m_pos ).Mag();
    if( distance > DARWINIAN_SEARCHRANGE_ARMOUR && !m_manuallyApproachingArmour )
    {
        m_state = StateIdle;
        m_armourId.SetInvalid();
        m_manuallyApproachingArmour = false;
        return false;
    }


    //
    // Walk towards the armour until we are there

    m_wayPoint = exitPos;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    
    bool arrived = AdvanceToTargetPosition();
    if( arrived || distance < 20.0 )
    {
        armour->AddPassenger();
        m_state = StateInsideArmour;
        if( m_manuallyApproachingArmour )
        {
            m_manuallyApproachingArmour = false;
            if( armour->m_state == Armour::StateUnloading )
            {
                armour->m_state = Armour::StateIdle;
            }
        }
    }

    return false;
}


bool Darwinian::AdvanceInsideArmour()
{
    //
    // Is our armour still alive

    Armour *armour = (Armour *) g_app->m_location->GetEntity( m_armourId );
    if( !armour || armour->m_dead )
    {
        m_armourId.SetInvalid();

        /*Vector3 pos = m_pos;
	    double radius = syncfrand(20.0);
	    double theta = syncfrand(M_PI * 2);
    	m_pos.x += radius * iv_sin(theta);
	    m_pos.z += radius * iv_cos(theta);
        m_vel = ( pos - m_pos );
        m_vel.SetLength( syncfrand(50.0) );*/
        m_vel.Set(SFRAND(1), 1.0, SFRAND(1) );
        m_vel.SetLength( 20.0 + syncfrand(20.0) );
        m_onGround = false;
        m_state = StateOnFire;

        return false;
    }    
    
    m_pos = armour->m_pos;
    m_vel = armour->m_vel;
    

    //
    // Is our armour unloading
    // Only get out if we are over ground, not water

    if( armour->IsUnloading() )
    {
        Vector3 exitPos, exitDir;
        armour->GetEntrance( exitPos, exitDir );
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(exitPos.x, exitPos.z);
        if( landHeight > 0.0 )
        {
            // JUMP!
            armour->RemovePassenger();
            exitDir.RotateAroundY( syncsfrand(M_PI * 0.5) );
            m_vel = exitDir * 10.0;
            m_vel.y = 5.0 + syncfrand(10.0);
            m_pos = exitPos;
            m_onGround = false;
            m_ordersSet = false;
            m_state = StateIdle;
            m_armourId.SetInvalid();
        }
    }
    else
    {
        armour->RegisterPassenger();
    }

    return false;
}


bool Darwinian::AdvanceCapturedByAnt()
{
    ArmyAnt *ant = (ArmyAnt *) g_app->m_location->GetEntity( m_threatId );
    if( !ant || ant->m_dead )
    {
        m_state = StateIdle;
        m_threatId.SetInvalid();
        return false;
    }

    Vector3 carryPos, carryVel;
    ant->GetCarryMarker( carryPos, carryVel );

    m_pos = carryPos;
    m_vel = carryVel;
            
    return false;
}


bool Darwinian::AdvanceCombat()
{
    START_PROFILE( "AdvanceCombat" );

    //
    // Does our threat still exist?

    if( m_insane )
    {
        if( !m_threatId.IsValid() )
        {
            // we're insane, so just pick a random target
            m_threatId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0, DARWINIAN_SEARCHRANGE_THREATS, 255 );
        }
    }
    
    WorldObject *threat = g_app->m_location->GetWorldObject( m_threatId );    
    bool isEntity = threat && threat->m_id.GetUnitId() != UNIT_EFFECTS && threat->m_id.GetUnitId() != UNIT_BUILDINGS;
    Entity *entity = ( isEntity ? (Entity *)threat : NULL );
    
    if( !threat || ( entity && entity->m_dead) )
    {
        if( m_officerId.IsValid() ) m_state = StateFollowingOfficer;
        else m_state = StateIdle;
        m_retargetTimer = 0.0;
		if( m_startedThreatSound )
		{
			m_startedThreatSound = false;
			g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );
		}
        END_PROFILE(  "AdvanceCombat" );

        return false;
    }


    //
    // If this is a gun turret, look to see if we are out of
    // the plausable line of fire

    if( !isEntity && threat && threat->m_type == EffectGunTurretTarget )
    {
        double distance = ( threat->m_pos - m_pos ).Mag();
        if( distance > DARWINIAN_SEARCHRANGE_GRENADES * 1.5 )
        {
            if( m_officerId.IsValid() ) m_state = StateFollowingOfficer;
            else m_state = StateIdle;
            m_retargetTimer = 0.0;

			if( m_startedThreatSound )
			{
				m_startedThreatSound = false;
				g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );
			}
            END_PROFILE(  "AdvanceCombat" );
            return false;
        }
    }

    //
    // Move away from our threat if we're an ordinary Darwinian
    // Move towards our threat if we're a Soldier Darwinian
    // Stay still if we are in formation (unless its a grenade)

    bool soldier = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian ) > 2;
    if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) soldier = true;

    Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
    bool inFormation = IsInFormation();
    if( inFormation )
    {
        officer->RegisterWithFormation( m_id.GetUniqueId() );
    }

    if( m_boosterTaskId != -1 )
    {
        m_scared = false;
    }

    if( !inFormation )//|| !isEntity )
    {
        if( soldier && !m_scared )
        {
            double distance = ( m_pos - threat->m_pos ).Mag();
            double heightDiff = ( m_pos.y - threat->m_pos.y );
            if( distance < DARWINIAN_FEARRANGE/2.0 &&
                m_controllerId == -1 &&
                heightDiff > -25 )
            {
                Vector3 moveAwayVector = ( m_pos - threat->m_pos ).Normalise() * 30.0;
                double angle = syncsfrand( M_PI * 0.5 );
                moveAwayVector.RotateAroundY( angle );    
                m_wayPoint = m_pos + moveAwayVector;
            }
            else
            {
                Vector3 targetVector = ( threat->m_pos - m_pos );
                double angle = syncsfrand( M_PI * 0.5 );
                targetVector.RotateAroundY( angle );    
                double distance = targetVector.Mag();
                double ourDesiredRange = 20.0 + syncfrand(20.0);                
                targetVector.SetLength( distance - ourDesiredRange );
                m_wayPoint = m_pos + targetVector;
            }
            m_wayPoint = PushFromObstructions( m_wayPoint );
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );                        
            AdvanceToTargetPosition();
        }
        else
        {
            double distance = ( m_pos - threat->m_pos ).Mag();
            if( distance > DARWINIAN_FEARRANGE )
            {
                m_scared = false;
                m_threatId.SetInvalid();
            }

            Vector3 moveAwayVector = ( m_pos - threat->m_pos ).Normalise() * 30.0;
            double angle = syncsfrand( M_PI * 0.5 );
            moveAwayVector.RotateAroundY( angle );    
            m_wayPoint = m_pos + moveAwayVector;
            m_wayPoint = PushFromObstructions( m_wayPoint );
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );    
            AdvanceToTargetPosition();    
        }
    }
    else
    {
        m_wayPoint = officer->GetFormationPosition( m_id.GetUniqueId() );
        AdvanceToTargetPosition();
    }
    

    if( isEntity )
    {
        //
        // Shoot at our enemy

        double attackRate = 10;
        if( m_boosterTaskId != -1 )
        {
            attackRate = 7;
        }

        if( m_futurewinian )
        {
            attackRate = 3;
        }

        attackRate /= g_gameTimer.GetGameSpeed();
                        
        if( soldier )
        {
            bool fireInFormation = false;
            bool fireNormally = false;

            Officer *officer = IsInFormation();
            if( officer )
            {
                attackRate = 0.0;
                int formationIndex = officer->GetFormationIndex(m_id.GetUniqueId());
                int formationSize = min( 90, officer->m_formationEntities.Size() );
                double timeFactor = g_gameTimer.GetGameTime()*10.0;
            
                if( int(fmodf( timeFactor+formationIndex, formationSize/4.0 )) == 0 )
                {
                    fireInFormation = true;
                }

                if( HasRage() &&
                    int(fmod( timeFactor+formationIndex, formationSize/16.0 )) == 0 )
                {
                    fireInFormation = true;
                }
            }
            else
            {
                fireNormally = (attackRate >= 1.0 && (syncrand() % (int)attackRate) == 0);
                if( HasRage() ) fireNormally = true;
            }


            if( fireNormally || fireInFormation )
            {
                if( m_subversionTaskId != -1 ||
                    m_futurewinian )
                {
                    FireSubversion( threat->m_pos + Vector3(0,2,0) );
                }
                else
                {
                    Vector3 pos = threat->m_pos + Vector3(0,2,0);

                    if( HasRage() )
                    {
                        pos.x += syncsfrand(60.0);
                        pos.z += syncsfrand(60.0);
                    }
                    Attack( pos );
                    if( HasRage() ) 
                    {
                        m_reloading = 0.0;
                    }
                }
            }
        }


        //
        // Throw grenades if we have a good opportunity
        // ie lots of enemies near our target, and not many friends
        // Or if the enemy is the sort that responds well to grenades
        // NEVER throw grenades if the target area is too steep - Darwinians just can't fucking aim on cliffs
        // NEVER throw grenades if we are in formation, its just too risky

        bool hasGrenade = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian ) > 3;
        if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) hasGrenade = true;
        if( inFormation ) hasGrenade = false;
        if( m_subversionTaskId != -1 ) hasGrenade = false;
        if( m_futurewinian ) hasGrenade = false;
        if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 &&
            !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn ) hasGrenade = false;
        
        if( hasGrenade )
        {
            START_PROFILE( "ThrowGrenade" );
            m_grenadeTimer -= SERVER_ADVANCE_PERIOD;
            if( m_grenadeTimer <= 0.0  )
            {
                m_grenadeTimer = 12.0+syncfrand(8.0);
                //m_grenadeTimer = 1.0;
                double distanceToTarget = ( threat->m_pos - m_pos ).Mag();                
                if( distanceToTarget > 75.0 ||
                    m_insane )
                //if( distanceToTarget > 50.0 )
                {
                    int numPlayers = 0;
                    if( !g_app->Multiplayer() && m_id.GetTeamId() == 0 ) 
                    {
                        // NEVER throw grenades if there are people from team 2 nearby (ie the player's squad, officers, engineers)
                        bool includeTeams[] = { false, false, true, false, false, false, false, false };
                        numPlayers = g_app->m_location->m_entityGrid->GetNumNeighbours( threat->m_pos.x, threat->m_pos.z, 50.0, includeTeams );
                    }

                    if( numPlayers == 0 )
                    {
                        bool throwGrenade = false;
                        Vector3 ourLandNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
                        Vector3 targetLandNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(threat->m_pos.x, threat->m_pos.z);
                        double heightDiff = ( m_pos.y - threat->m_pos.y );

                        if( ourLandNormal.y > 0.7 && 
                            targetLandNormal.y > 0.7 &&
                            heightDiff > -50 )
                        {
                            bool grenadeRequired =  entity->m_type == TypeSporeGenerator ||                                            
                                                    entity->m_type == TypeSpider ||
                                                    entity->m_type == TypeTriffidEgg ||
                                                    entity->m_type == TypeInsertionSquadie ||
                                                    entity->m_type == TypeArmour ||
                                                    entity->m_type == TypeTank;

                            if( grenadeRequired )
                            {
                                double targetHeight = entity->m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( entity->m_pos.x, entity->m_pos.z );
                                if( targetHeight < 40.0 ) throwGrenade = true;
                            }
                            else
                            {
                                int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( threat->m_pos.x, threat->m_pos.z, 50.0, m_id.GetTeamId() );
                                int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( threat->m_pos.x, threat->m_pos.z, 50.0, m_id.GetTeamId() );
                                if( numEnemies > 5 && numFriends < 2 ) throwGrenade = true;                            
                                //if( numEnemies > 3 ) throwGrenade = true;
                            }
                        }

                        if( m_insane ) throwGrenade = true;

                        if( entity->m_type == TypeSpaceShip ) throwGrenade = false;

                        if( throwGrenade )
                        {
                            Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
                            if( team->m_evolutions[Team::EvolutionRockets] == 1) 
                            {
                                Vector3 targetPos = threat->m_pos;
                                targetPos.y += 10.0; // aim up a little to avoid hitting the floor
								if( m_insane)
								{
									targetPos.x += syncfrand(4)-2;
									targetPos.z += syncfrand(4)-2;
								}
                                g_app->m_location->FireRocket( m_pos, targetPos, m_id.GetTeamId(), -1.0, -1, (m_infectionState == InfectionStateInfected ) );
                            }
                            else
                            {
                                g_app->m_location->ThrowWeapon( m_pos, threat->m_pos, EffectThrowableGrenade, m_id.GetTeamId() );                                                               
                            }
                        }
                    }
                }
            }
            END_PROFILE(  "ThrowGrenade" );
        }
    }

    END_PROFILE(  "AdvanceCombat" );
    return false;    
}


bool Darwinian::AdvanceWorshipSpirit()
{
    START_PROFILE( "AdvanceWorship" );

    if( g_app->Multiplayer() )
    {
        // extra safety check - we should never even GET here in multiplayer, but just in case
        m_state = StateIdle;
        END_PROFILE(  "AdvanceWorship" );
        return false;
    }
    
    //
    // Check our spirit is still there and valid

    if( !g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
    {
        m_state = StateIdle;
        m_retargetTimer = 3.0;
        END_PROFILE(  "AdvanceWorship" );
        return false;
    }

    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    if( spirit->m_state != Spirit::StateBirth &&
        spirit->m_state != Spirit::StateFloating )
    {
        m_state = StateIdle;
        m_retargetTimer = 3.0;

        if( m_boxKiteId.IsValid() )
        {
            BoxKite *boxKite = (BoxKite *) g_app->m_location->GetEffect( m_boxKiteId );
            if ( boxKite )
				boxKite->Release();
            m_boxKiteId.SetInvalid();
        }
 
        END_PROFILE(  "AdvanceWorship" );
        return false;
    }


    //
    // Move to within range of our spirit

    Vector3 targetVector = ( spirit->m_pos - m_pos );
    double distance = targetVector.Mag();
    double ourDesiredRange = 20 + (m_id.GetUniqueId() % 20);
    targetVector.SetLength( distance - ourDesiredRange );
    Vector3 newWaypoint = m_pos + targetVector;
    newWaypoint = PushFromObstructions( newWaypoint );    
    newWaypoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newWaypoint.x, newWaypoint.z );    
    if( (newWaypoint - m_wayPoint).Mag() > 10.0 )
    {
        m_wayPoint = newWaypoint;
    }

    bool areWeThere = AdvanceToTargetPosition();
    if( areWeThere )
    {
        m_front = ( spirit->m_pos - m_pos ).Normalise();
    }


    //
    // Possibly spawn a boxkite to guide the spirit to heaven

    if( areWeThere && !m_boxKiteId.IsValid() )
    {
        bool existingKiteFound = false;

        for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
        {
            if( g_app->m_location->m_effects.ValidIndex(i))
            {
                WorldObject *obj = g_app->m_location->m_effects[i];
                if( obj->m_id.GetUnitId() == UNIT_EFFECTS &&
                    obj->m_type == EffectBoxKite )
                {
                    double distanceSqd = ( obj->m_pos - m_pos ).MagSquared();
                    if( distanceSqd < 2500.0 )
                    {
                        existingKiteFound = true;
                        break;
                    }
                }
            }
        }

        if( !existingKiteFound )
        {
            BoxKite *boxKite = new BoxKite();
            boxKite->m_pos = m_pos + m_front * 2 + g_upVector * 5;
            boxKite->m_front = m_front;
            int index = g_app->m_location->m_effects.PutData( boxKite );
            boxKite->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
            boxKite->m_id.GenerateUniqueId();
            m_boxKiteId = boxKite->m_id;
            //SyncRandLog( "Boxkite created at %2.2 %2.2 %2.2", boxKite->m_pos.x, boxKite->m_pos.y, boxKite->m_pos.z );
        }
    }


    END_PROFILE(  "AdvanceWorship" );
    return false;
}


bool Darwinian::AdvanceApproachingPort()
{
    //
    // Check the port is still available
    
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building ) 
    {
        m_state = StateIdle;
        return false;
    }        

    WorldObjectId occupant = building->GetPortOccupant(m_portId);
    bool otherOccupantFound = occupant.IsValid() &&          
                            !(building->GetPortOccupant(m_portId) == m_id);
    if( otherOccupantFound )
    {
        m_state = StateIdle;
        return false;
    }


    //
    // Move to within range of the port

    building->OperatePort(m_portId, m_id.GetTeamId() );
    bool areWeThere = AdvanceToTargetPosition();
    if( areWeThere )
    {
        Vector3 portPos, portFront;
        building->GetPortPosition( m_portId, portPos, portFront );
        m_front = portFront;
        m_vel.Zero();
        m_state = StateOperatingPort;
    }

    return false;
}


bool Darwinian::AdvanceOperatingPort()
{
    //
    // Check the port is still available
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building ) 
    {
        m_state = StateIdle;
        return false;
    }        


    if( building->GetPortOccupant(m_portId) != m_id )
    {
        m_state = StateIdle;
        return false;
    }

    m_vel.Zero();

    return false;
}


Vector3 GetCrateOffset( int id )
{
    double timeIndex = g_gameTime + id;
    
    Vector3 result( iv_sin(timeIndex*2.5)*3, 
                    iv_cos(timeIndex)*6, 
                    iv_sin(timeIndex*1.5)*4 );

    return result;
}


bool Darwinian::AdvanceOpeningCrate()
{
    //
    // Check the crate is still available
    
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building ) 
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }        


    //
    // Move towards it

    bool arrived = AdvanceToTargetPosition();

    if( arrived )
    {
        m_front = ( building->m_pos - m_pos ).Normalise();
    }


    //
    // Spawn particles

    //if( int( g_gameTimer.GetGameTime() * 10 ) % m_id.GetUniqueId() == 0 )
    {
        double distance = ( m_pos - building->m_pos ).Mag();
        if( distance < CRATE_SCAN_RANGE * 1.1 )
        {
            Vector3 offset = GetCrateOffset(m_id.GetUniqueId());
            Vector3 pos = building->m_pos + offset;            
            
            Vector3 vel = m_pos - pos;
            Vector3 right = vel ^ g_upVector;
            vel = right ^ vel;
            vel.y *= 1.1;

            vel.Normalise();
            //vel += Vector3( sfrand(0.3), sfrand(0.3), sfrand(0.3) );

            double percentDone = ((Crate *)building)->GetPercentCaptured();
            double length = 20 + 20 * percentDone;
            length *= ( 1.0 + offset.y/6.0 * 0.5 );
            vel.SetLength(length);
           
            double size = 20.0 + frand(10);

            RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
            g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeBlueSpark, size, col );            
        }
    }

    return false;
}

bool Darwinian::AdvanceRaisingFlag()
{
    //
    // Check the crate is still available
    
    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building || building->m_type != Building::TypeMultiwiniaZone ) 
    {
        m_state = StateIdle;
        m_buildingId = -1;
        return false;
    }
    else 
    {
        MultiwiniaZone *zone = (MultiwiniaZone *)building;
        if( zone->m_id.GetTeamId() == m_id.GetTeamId() || zone->m_blitzkriegLocked )
        {
            m_state = StateIdle;
            m_buildingId = -1;
            return false;
        }
    }


    //
    // Move towards it

    bool arrived = AdvanceToTargetPosition();

    if( arrived )
    {
        m_front = ( building->m_pos - m_pos ).Normalise();
    }

    return false;
}


bool Darwinian::AdvanceUnderControl()
{
    //
    // Try to lookup our controller

    Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
    Task *task = NULL;
    if( g_app->Multiplayer() ) task = team->m_taskManager->GetTask( m_controllerId );
    else                       task = g_app->m_location->GetMyTaskManager()->GetTask( m_controllerId );
    
    InsertionSquad *controller = NULL;
    if( task ) controller = (InsertionSquad *)g_app->m_location->GetUnit( task->m_objId );    

    if( !task || !controller )
    {
        LoseControl();
        return false;
    }


    //
    // Every few seconds, see if our squadies are still around
    // Retarget him in case he moved a lot

    m_officerTimer -= SERVER_ADVANCE_PERIOD;
    if( m_officerTimer <= 0.0 )
    {
        m_officerTimer = 0.5 + syncfrand(0.5);        

        Vector3 focusPos = ((InsertionSquad *)controller)->m_focusPos;
        Vector3 diff = ( focusPos - controller->m_centrePos );
        if( diff.Mag() > DARWINIAN_UNDERCONTROL_MAXPUSHRANGE )
        {
            diff.SetLength( DARWINIAN_UNDERCONTROL_MAXPUSHRANGE );
        }
        Vector3 targetPos = controller->m_centrePos + diff;

        bool walkable = g_app->m_location->IsWalkable( m_pos, targetPos );
        if( !walkable )
        {
            m_state = StateIdle;
            return false;
        }
        else
        {                        
            m_wayPoint = targetPos;
            double positionError = controller->m_numControlled;
            
            double radius = syncfrand(positionError);
            double theta = syncfrand(M_PI * 2);
            m_wayPoint.x += radius * iv_sin(theta);
            m_wayPoint.z += radius * iv_cos(theta);            
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        }
    }


    //
    // Head straight for him

    bool arrived = AdvanceToTargetPosition();


    return false;
}


bool Darwinian::AdvanceFollowingOrders ()
{
    bool arrived = AdvanceToTargetPosition();

    if( m_ordersBuildingId != -1 )
    {
        bool teleported = ( EnterTeleports(m_ordersBuildingId) != -1 );
        if( teleported )
        {
            m_underPlayerControl = false;
            m_ordersBuildingId = -1;
            arrived = true;
        }
    }
    
    if( arrived )
    {
        if( m_ordersBuildingId != -1 )
        {
            // We have arrived but are trying to enter a teleport
            // Just wiggle around until we are given entry
            Building *building = g_app->m_location->GetBuilding( m_ordersBuildingId );
            if( building )
            {
                Teleport *teleport = (Teleport *) building;
                if( !teleport->Connected() )
                {
                    m_ordersBuildingId = -1;
                }
                else
                {
                    Vector3 entrancePos, entranceFront;
                    teleport->GetEntrance( entrancePos, entranceFront );
	                m_wayPoint = entrancePos;
        	        double radius = syncfrand(10.0);
                    double theta = syncfrand(M_PI * 2);
    	            m_wayPoint.x += radius * iv_sin(theta);
	                m_wayPoint.z += radius * iv_cos(theta);
                    m_wayPoint = PushFromObstructions( m_wayPoint );
                    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
                }
            }
            else
            {
                m_ordersBuildingId = -1;
            }            
        }
        else
        {            
            m_state = StateIdle;
            m_retargetTimer = 0.0;
            m_ordersBuildingId = -1;
            m_ordersSet = false;
            m_followingPlayerOrders = false;
        }
    }

    return false;
}


bool Darwinian::AdvanceFollowingShaman()
{
	Shaman *shaman = (Shaman *) g_app->m_location->GetEntitySafe( m_shamanId, TypeShaman );
    if( !shaman || shaman->m_dead || !shaman->CallingDarwinians() )
    {
        m_shamanId.SetInvalid();
        m_state = StateIdle;
        return false;
    }


    //
    // Every few seconds, see if our officer is still around
    // Retarget him in case he moved a lot

    m_officerTimer -= SERVER_ADVANCE_PERIOD;
    if( m_officerTimer <= 0.0 )
    {
        m_officerTimer = 5.0;
        bool walkable = g_app->m_location->IsWalkable( m_pos, shaman->m_pos );
        if( !walkable )
        {
            m_shamanId.SetInvalid();
            m_state = StateIdle;
            return false;
        }
        else
        {
            m_wayPoint = shaman->m_pos;
            double positionError = 10.0;
            double radius = syncfrand(positionError);
	        double theta = syncfrand(M_PI * 2);
            m_wayPoint.x += radius * iv_sin(theta);
	        m_wayPoint.z += radius * iv_cos(theta);            
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        }
    }


    //
    // Head straight for him

    bool arrived = AdvanceToTargetPosition();
    
    if( arrived )
    {
        double positionError = 0.0;
        m_wayPoint = shaman->m_pos;            
        positionError = 40.0;

        double radius = syncfrand(positionError);
	    double theta = syncfrand(M_PI * 2);
        m_wayPoint.x += radius * iv_sin(theta);
	    m_wayPoint.z += radius * iv_cos(theta);   
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

    return false;
}


bool Darwinian::AdvanceFollowingOfficer()
{
    //
    // Look up our officer

    Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
    if( !officer || officer->m_dead || officer->m_orders != Officer::OrderFollow || !officer->m_enabled)
    {
        m_officerId.SetInvalid();
        m_state = StateIdle;
        return false;
    }

    if( officer->m_formation && officer->FormationFull() && IsInFormation() != officer )
    {
        // we were following this officer, but he switched to formation and is now full
        m_officerId.SetInvalid();
        m_state = StateIdle;
        return false;
    }



    //
    // Every few seconds, see if our officer is still around
    // Retarget him in case he moved a lot

    m_officerTimer -= SERVER_ADVANCE_PERIOD;
    if( m_officerTimer <= 0.0 )
    {
        m_officerTimer = 5.0;
        bool walkable = g_app->m_location->IsWalkable( m_pos, officer->m_pos );
        if( !walkable )
        {
            //
            // If our officer stepped into a teleport, try to follow
            if( officer->m_ordersBuildingId != -1 )
            {
                m_ordersBuildingId = officer->m_ordersBuildingId;
                Building *building = g_app->m_location->GetBuilding( m_ordersBuildingId );
                AppDebugAssert( building );        
                Teleport *teleport = (Teleport *) building;
                if( !teleport->Connected() )
                {
                    // Our officer went in but its no longer connected
                    m_officerId.SetInvalid();
                    m_state = StateIdle;
                    return false;
                }
                else
                {
                    Vector3 entrancePos, entranceFront;
                    teleport->GetEntrance( entrancePos, entranceFront );
	                m_wayPoint = entrancePos;
                    m_wayPoint = PushFromObstructions( m_wayPoint );
                    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );            
                }
            }
            else
            {
                m_officerId.SetInvalid();
                m_state = StateIdle;
                return false;
            }
        }
        else
        {
            m_wayPoint = officer->m_pos;
            double positionError = 40.0;
            double radius = syncfrand(positionError);
	        double theta = syncfrand(M_PI * 2);
            m_wayPoint.x += radius * iv_sin(theta);
	        m_wayPoint.z += radius * iv_cos(theta);            
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        }
    }
    

    if( officer->m_formation )
    {
        officer->RegisterWithFormation( m_id.GetUniqueId() );
        m_wayPoint = officer->GetFormationPosition( m_id.GetUniqueId() );
    }


    //
    // Head straight for him

    bool arrived = AdvanceToTargetPosition();

    if( m_ordersBuildingId != -1 )
    {
        bool teleported = ( EnterTeleports(m_ordersBuildingId) != -1 ); 
        if( teleported )
        {
            m_ordersBuildingId = -1;
            arrived = true;
        }
    }
    
    if( arrived && !officer->m_formation )
    {
        double positionError = 0.0;
        if( m_ordersBuildingId != -1 )
        {
            // We have arrived but are trying to enter a teleport
            // Just wiggle around until we are given entry
            Building *building = g_app->m_location->GetBuilding( m_ordersBuildingId );
            AppDebugAssert(building);
            Teleport *teleport = (Teleport *) building;
            if( !teleport->Connected() )
            {
                m_ordersBuildingId = -1;
            }
            else
            {
                Vector3 entrancePos, entranceFront;
                teleport->GetEntrance( entrancePos, entranceFront );
	            m_wayPoint = entrancePos;
        	    positionError = 10.0;
            }
        }
        else
        {
            m_wayPoint = officer->m_pos;            
            positionError = 40.0;
        }

        double radius = syncfrand(positionError);
	    double theta = syncfrand(M_PI * 2);
        m_wayPoint.x += radius * iv_sin(theta);
	    m_wayPoint.z += radius * iv_cos(theta);   
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

    if( arrived && officer->m_formation )
    {
        m_front = officer->m_front;
    }

    return false;
}


void Darwinian::BoardRocket( int _buildingId )
{
    m_state = StateBoardingRocket;
    m_buildingId = _buildingId;    
}


bool Darwinian::AdvanceBoardingRocket()
{
    //
    // Find our building

    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building || building->m_type != Building::TypeFuelStation )
    {
        m_state = StateIdle;
        return false;
    }

    //
    // Make sure we are still loading Darwinians

    FuelStation *station = (FuelStation *) building;
    if( !station->IsLoading() )
    {
        m_state = StateIdle;
        return false;
    }

    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( station->GetBuildingLink() );
    if( rocket && rocket->m_type == Building::TypeEscapeRocket )
    {
        if( rocket->m_passengers >= 100 )
        {
            m_state = StateIdle;
            return false;
        }
    }


    //
    // Head towards the building

    m_wayPoint = station->GetEntrance();
    m_wayPoint = PushFromObstructions( m_wayPoint );

    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        bool boarded = station->BoardRocket( m_id );
        if( boarded ) return true;
    }

    return false;
}


void Darwinian::AttackBuilding( int _buildingId )
{
    m_state = StateAttackingBuilding;
    m_buildingId = _buildingId;
}


bool Darwinian::AdvanceAttackingBuilding()
{
    //
    // Find our building

    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building || (building->m_type != Building::TypeEscapeRocket && !g_app->Multiplayer()) )
    {
        m_state = StateIdle;
        return false;
    }
    

    //
    // Run towards our building

    double distance = ( building->m_pos - m_pos ).Mag();
    Vector3 moveVector = (m_pos - building->m_pos);
    moveVector.SetLength( 100 + syncfrand(50.0) );
    m_wayPoint = building->m_pos + moveVector;
    bool arrived = AdvanceToTargetPosition();


    //
    // Shoot at the building if we are in range

    if( distance < 200.0 )
    {
        Vector3 targetPos = building->m_pos;
        targetPos.y += 50.0;
        g_app->m_location->ThrowWeapon( m_pos, targetPos, WorldObject::EffectThrowableGrenade, 1 );        
        m_state = StateIdle;
    }

    return false;
}


bool Darwinian::SearchForRandomPosition()
{
    START_PROFILE( "SearchRandomPos" );

    //
    // Search for a new random position
    // Sometimes just don't bother, so we stand still, 
    // pondering the meaning of the world

    if( syncfrand(1) < 0.7 )
    {
        double distance = 20.0;
        double angle = syncsfrand(2.0 * M_PI);

        m_wayPoint = m_pos + Vector3( iv_sin(angle) * distance,
                                       0.0,
                                       iv_cos(angle) * distance );

        m_wayPoint = PushFromObstructions( m_wayPoint );    
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }
    
    END_PROFILE(  "SearchRandomPos" );

    return true;
}


bool Darwinian::SearchForCrates()
{
	START_PROFILE(  "SearchCrate" );
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeCrate && b->m_onGround )
            {
                double distance = ( b->m_pos - m_pos ).Mag();
                if( distance < DARWINIAN_SEARCHRANGE_CRATES )
                {
                    if( g_app->m_location->IsWalkable( m_pos, b->m_pos ) )
                    {
                        m_state = StateOpeningCrate;
                        m_buildingId = b->m_id.GetUniqueId();

                        if( distance < CRATE_SCAN_RANGE )
                        {
                            m_wayPoint = m_pos;
                        }
                        else
                        {
                            m_wayPoint = b->m_pos;
                            
                            double distance = CRATE_SCAN_RANGE/3.0 + syncfrand(2.0*CRATE_SCAN_RANGE/3.0);

                            Vector3 offset = ( m_pos - m_wayPoint );
                            offset.SetLength( distance );
                            m_wayPoint += offset;

                            double angle = syncfrand(2.0 * M_PI);                        
                            offset.Set(syncfrand(CRATE_SCAN_RANGE/3.0),0,0);
                            offset.RotateAroundY(angle);                        
                            m_wayPoint += offset;       

                            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
                        }
					    END_PROFILE(  "SearchCrate" );
                        return true;
                    }
                }
            }
        }
    }
	END_PROFILE(  "SearchCrate" );
    return false;
}

bool Darwinian::SearchForFlags()
{
    START_PROFILE(  "SearchFlags" );
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeMultiwiniaZone && b->m_id.GetTeamId() != m_id.GetTeamId())
            {
                MultiwiniaZone *zone = (MultiwiniaZone *)b;
                if( !zone->m_blitzkriegLocked )
                {
                    double distance = ( b->m_pos - m_pos ).Mag();
                    if( distance < zone->m_size )
                    {
                        m_state = StateRaisingFlag;
                        m_buildingId = b->m_id.GetUniqueId();

                        if( distance < BLITZKRIEG_FLAGCAPTURE_RANGE )
                        {
                            m_wayPoint = m_pos;
                        }
                        else
                        {
                            m_wayPoint = b->m_pos;
                            m_wayPoint += Vector3( SFRAND(BLITZKRIEG_FLAGCAPTURE_RANGE), 0, SFRAND(BLITZKRIEG_FLAGCAPTURE_RANGE) );
                            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
                        }
					    END_PROFILE(  "SearchFlags" );
                        return true;
                    }
                }
            }
        }
    }
	END_PROFILE(  "SearchFlags" );
    return false;
}

bool Darwinian::SearchForArmour()
{
    //
    // Red Darwinians don't respond to armour

    if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) return false;


    START_PROFILE( "SearchArmour" );

    //
    // Build a list of nearby armour

    LList<WorldObjectId> m_armour;

    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    if( g_app->IsSinglePlayer() )
    {
        team = g_app->m_location->GetMyTeam();
    }

    if( team )
    {
        for( int i = 0; i < team->m_specials.Size(); ++i )
        {
            WorldObjectId id = *team->m_specials.GetPointer(i);
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && !entity->m_dead && entity->m_type == Entity::TypeArmour )
            {            
                Armour *armour = (Armour *) entity;
                double range = ( armour->m_pos - m_pos ).Mag();
                if( range <= DARWINIAN_SEARCHRANGE_ARMOUR &&
                    armour->IsLoading() )
                {
                    m_armour.PutData( id );
                }
            }
        }
    }

    //
    // Select armour randomly

    if( m_armour.Size() > 0 )
    {
        int chosenIndex = syncrand() % m_armour.Size();        
        m_armourId = *m_armour.GetPointer(chosenIndex);
        m_state = StateApproachingArmour;
    }

    END_PROFILE(  "SearchArmour" );
    return ( m_armour.Size() > 0 );
}


Officer *Darwinian::IsInFormation()
{
    Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
    if( officer && officer->m_formation )
    {
        if( officer->IsInFormation( m_id.GetUniqueId() ) )
        {
            return officer;
        }
    }

    return NULL;
}

bool Darwinian::IsSelectable()
{
    if( m_state == StateBeingSacraficed ||
        m_state == StateInsideArmour ||
        m_state == StateOnFire ||
        m_state == StateOperatingPort /*||
		g_app->IsSinglePlayer()*/)
    {
        return false;
    }

    return true;
}


void Darwinian::FollowRoute()
{
    if(m_routeId == -1 )
    {
        return;
    }

    if( m_state != StateFollowingOrders && 
        m_state != StateFollowingOfficer )
    {
        return;
    }

    Route *route = g_app->m_location->m_levelFile->GetRoute(m_routeId);
    if( g_app->Multiplayer() ) route = g_app->m_location->m_routingSystem.GetRoute( m_routeId );

    if(!route)
    {
        m_routeId = -1;
        return;
    }

    bool waypointChange = false;

    if (m_routeWayPointId == -1)
    {
        m_routeWayPointId = 1;
        waypointChange = true;
    }
    else
    {
        double distToWaypoint = ( m_pos - m_wayPoint ).Mag();
        if( distToWaypoint < 20 )
        {
            ++m_routeWayPointId;    
            waypointChange = true;
        }
    }


    if( waypointChange )
    {
        if (m_routeWayPointId >= route->m_wayPoints.Size())
        {
            m_routeWayPointId = -1;
            m_routeId = -1;
            return;
        }

        WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
        if( !waypoint )
        {
            m_routeId = -1;
            return;
        }

        m_wayPoint = waypoint->GetPos();
        
        double positionError = 40.0;
        double radius = syncfrand(positionError);
        double theta = syncfrand(M_PI * 2);
                
        m_wayPoint.x += radius * iv_sin(theta);
        m_wayPoint.z += radius * iv_cos(theta);

        m_wayPoint = PushFromObstructions( m_wayPoint, false );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );            

    }   
}


bool Darwinian::SearchForOfficers()
{
    //
    // Red Darwinians don't respond to officers
    // (in the single player game)

    if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) return false;

    
    START_PROFILE( "SearchOfficers" );

    //
    // Do we already have some orders that have yet to be completed?

    if( m_ordersSet )
    {
        m_wayPoint = m_orders;

        if( m_routeId != -1 )
        {
            Route *route = g_app->m_location->m_routingSystem.GetRoute(m_routeId);
            if( route )
            {
                WayPoint *wayPoint = route->GetWayPoint( m_routeWayPointId );
                if( wayPoint ) m_wayPoint = wayPoint->GetPos();
            }
        }

        double positionError = 20.0;
        double radius = syncfrand(positionError);
	    double theta = syncfrand(M_PI * 2);
    	
        m_wayPoint.x += radius * iv_sin(theta);
	    m_wayPoint.z += radius * iv_cos(theta);
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

        m_state = StateFollowingOrders;
        m_followingPlayerOrders = false;

        END_PROFILE(  "SearchOfficers" );
        return true;
    }


    //
    // Build a list of nearby officers with GOTO orders set
    // Also find the nearest officer with FOLLOW orders set

    Team *team = g_app->m_location->GetMyTeam();

	if( g_app->Multiplayer() )
    {
        team = g_app->m_location->m_teams[m_id.GetTeamId()];
    }

    if( team )
    {
        LList<WorldObjectId> officers;
        double nearest = 99999.9;
        WorldObjectId nearestId;

        for( int i = 0; i < team->m_specials.Size(); ++i )
        {
            WorldObjectId id = *team->m_specials.GetPointer(i);
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && 
               !entity->m_dead && 
                entity->m_type == Entity::TypeOfficer )
            {
                Officer *officer = (Officer *) entity;
                double distance = ( officer->m_pos - m_pos ).Mag();
                if( distance < DARWINIAN_SEARCHRANGE_OFFICERS &&
                    officer->m_orders == Officer::OrderGoto )
                {
                    officers.PutData( id );
                }
                else if( officer->m_orders == Officer::OrderFollow &&
                         distance < DARWINIAN_SEARCHRANGE_OFFICERFOLLOW &&
                         /*distance > 50.0 &&*/ distance < nearest )
                {
                    if( !officer->m_formation ||
                        !officer->FormationFull() )
                    {
                        nearest = distance;
                        nearestId = id;
                    }
                }
            }
        }   

        //
        // If there aren't any officers nearby, look for officers
        // with the FOLLOW order set and head for them
        // or find officers with the FORMATION order set and get into formation

        if( nearestId.IsValid() )
        {            
            m_officerId = nearestId;
            Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
            if( g_app->m_location->IsWalkable( m_pos, officer->m_pos, true ) )
            {
                m_wayPoint = officer->m_pos;

                double positionError = 40.0;
                double radius = syncfrand(positionError);
	            double theta = syncfrand(M_PI * 2);
                m_wayPoint.x += radius * iv_sin(theta);
	            m_wayPoint.z += radius * iv_cos(theta);            
                m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
            
                m_state = StateFollowingOfficer;
                m_officerTimer = 5.0;
                END_PROFILE(  "SearchOfficers" );
                return true;
            }
        }


        //
        // Select a GOTO officer randomly

        if( officers.Size() > 0 )
        {
            int chosenOfficer = syncrand() % officers.Size();
            WorldObjectId officerId = *officers.GetPointer(chosenOfficer);
            Officer *officer = (Officer *) g_app->m_location->GetEntitySafe(officerId, TypeOfficer);
            AppDebugAssert( officer );
        
            m_orders = officer->m_orderPosition;
            m_routeId = officer->m_orderRouteId;
            m_routeWayPointId = -1;
            m_ordersBuildingId = officer->m_ordersBuildingId;
            m_ordersSet = true;
            m_followingPlayerOrders = false;

            double positionError = 20.0;
            double radius = syncfrand(positionError);
            double theta = syncfrand(M_PI * 2);
            m_wayPoint = m_orders;
            m_wayPoint.x += radius * iv_sin(theta);
            m_wayPoint.z += radius * iv_cos(theta);

            m_wayPoint = PushFromObstructions( m_wayPoint, false );
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );            

            g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrders" );

            m_state = StateFollowingOrders;
            END_PROFILE(  "SearchOfficers" );
            return true;
        

            //if( g_app->m_location->IsWalkable( m_pos, officer->m_orderPosition ) )
            //{
            //    m_orders = officer->m_orderPosition;
            //    m_ordersBuildingId = officer->m_ordersBuildingId;
            //    m_ordersSet = true;

            //    double positionError = 20.0;
            //    double radius = syncfrand(positionError);
	           // double theta = syncfrand(M_PI * 2);
            //    m_wayPoint = m_orders;
    	       // m_wayPoint.x += radius * iv_sin(theta);
	           // m_wayPoint.z += radius * iv_cos(theta);

            //    m_wayPoint = PushFromObstructions( m_wayPoint, false );
            //    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

            //    g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrders" );

            //    m_state = StateFollowingOrders;
            //    END_PROFILE(  "SearchOfficers" );
            //    return true;
            //}
        }
    }

    END_PROFILE(  "SearchOfficers" );
    return false;
}

bool Darwinian::SearchForShamans()
{
    //
    // Red Darwinians don't respond to officers
    // (in the single player game)

	if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) return false;

    
    START_PROFILE( "SearchShaman" );

    Team *team = g_app->m_location->GetMyTeam();

	if( g_app->Multiplayer() )
    {
        team = g_app->m_location->m_teams[m_id.GetTeamId()];
    }

    if( team )
    {
        double nearest = 99999.9;
        WorldObjectId nearestId;

        for( int i = 0; i < team->m_specials.Size(); ++i )
        {
            WorldObjectId id = *team->m_specials.GetPointer(i);
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && 
               !entity->m_dead && 
                entity->m_type == Entity::TypeShaman)
            {
                Shaman *shaman = (Shaman *) entity;
                double distance = ( shaman->m_pos - m_pos ).Mag();
                if( shaman->CallingDarwinians() &&
                    distance < DARWINIAN_SEARCHRANGE_OFFICERFOLLOW &&
                    distance < nearest )
                {
                    nearest = distance;
                    nearestId = id;
                }
            }
        }    

        if( nearestId.IsValid() )
        {            
            m_shamanId = nearestId;
            Shaman *shaman = (Shaman *) g_app->m_location->GetEntitySafe( m_shamanId, TypeShaman);
            if( g_app->m_location->IsWalkable( m_pos, shaman->m_pos, true ) )
            {
                m_wayPoint = shaman->m_pos;

                double positionError = 40.0;
                double radius = syncfrand(positionError);
	            double theta = syncfrand(M_PI * 2);
                m_wayPoint.x += radius * iv_sin(theta);
	            m_wayPoint.z += radius * iv_cos(theta);            
                m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
            
                m_state = StateFollowingShaman;
                m_officerTimer = 5.0;
                END_PROFILE(  "SearchShaman" );
                return true;
            }
        }
    }

    END_PROFILE(  "SearchShaman" );
    return false;
}

void Darwinian::GiveOrders( Vector3 const &_targetPos, int routeId )
{
    m_orders = _targetPos;
    m_ordersBuildingId = -1;
    m_ordersSet = true;
    m_routeId = -1;  

    //
    // If we are carrying a building, pass on these orders
    // If we are carrying a wall buster, make us set a direct route always

    if( m_state == StateCarryingBuilding )
    {
        CarryableBuilding *building = (CarryableBuilding *)g_app->m_location->GetBuilding( m_buildingId );

        if( building && 
            CarryableBuilding::IsCarryableBuilding(building->m_type) )
        {
            if( building->m_type == Building::TypeWallBuster )
            {
                building->SetWaypoint( _targetPos, -1 );
            }
            else
            {
                building->SetWaypoint( _targetPos, routeId );
            }
            return;
        }
    }


    //
    // If there is a teleport nearby, 
    // assume he wants us to go in it

    bool foundTeleport = false;

    LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_orders.x, m_orders.z );
    for( int i = 0; i < nearbyBuildings->Size(); ++i )
    {
        int buildingId = nearbyBuildings->GetData(i);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building->m_type == Building::TypeRadarDish ||
            building->m_type == Building::TypeBridge )
        {
            double distance = ( building->m_pos - m_orders ).Mag();                    
            if( distance < 5 )
            {
                Teleport *teleport = (Teleport *) building;
                m_ordersBuildingId = building->m_id.GetUniqueId();
                Vector3 entrancePos, entranceFront;
                teleport->GetEntrance( entrancePos, entranceFront );
                m_orders = entrancePos;
                foundTeleport = true;
                break;
            }
        }
    }

    m_wayPoint = m_orders;

    if( !foundTeleport )
    {
        double positionError = 20.0;
        double radius = syncfrand(positionError);
	    double theta = syncfrand(M_PI * 2);
        m_wayPoint.x += radius * iv_sin(theta);
	    m_wayPoint.z += radius * iv_cos(theta);
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        m_wayPoint = PushFromObstructions(m_wayPoint, false);
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }
    
    //m_wayPoint = PushFromObstructions( m_wayPoint );
    //m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    //m_wayPoint = PushFromObstructions( m_wayPoint );    
    //g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrders" );

    m_routeId = routeId;
    m_routeWayPointId = -1;

    m_state = StateFollowingOrders;
}


bool Darwinian::SearchForSpirits()
{
    // Red darwinians don't worship spirits
    if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) return false;

    if( g_app->Multiplayer() ) return false;

    START_PROFILE( "SearchSpirits" );

    Spirit *found = NULL;
    int spiritId = -1;
    double closest = DARWINIAN_SEARCHRANGE_SPIRITS;

    if( syncrand() % 5 == 0 )
    {
        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            if( g_app->m_location->m_spirits.ValidIndex(i) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                double theDist = ( s->m_pos - m_pos ).Mag();

                if( theDist < closest &&                
                    ( s->m_state == Spirit::StateBirth ||
                      s->m_state == Spirit::StateFloating ) )
                {                
                    found = s;
                    spiritId = i;
                    closest = theDist;
                }
            }
        }            
    }

    if( found )
    {
        m_spiritId = spiritId;        
        m_state = StateWorshipSpirit;
    }    

    END_PROFILE(  "SearchSpirits" );
    return found;
}


bool Darwinian::SearchForThreats()
{
    START_PROFILE( "SearchThreats" );

    //
    // Allow our threat range to creep back up to the max

    double threatRangeChange = SERVER_ADVANCE_PERIOD;
    m_threatRange = ( DARWINIAN_SEARCHRANGE_THREATS * threatRangeChange ) +
                    ( m_threatRange * (1.0 - threatRangeChange) );
            

    Officer *formationOfficer = IsInFormation();

    if( formationOfficer ) m_threatRange = DARWINIAN_SEARCHRANGE_THREATS;
    //if( m_subversionTaskId != -1 ) m_threatRange = min( m_threatRange, DARWINIAN_SEARCHRANGE_THREATS * 0.75 );
    

    //
    // If we are running towards a Battle Cannon or a Carryable object,
    // this takes priority over everything else

    if( m_state == StateApproachingPort )
    {
        Building *building = g_app->m_location->GetBuilding( m_buildingId );
        if( building && building->m_type == Building::TypeGunTurret )
        {
            END_PROFILE(  "SearchThreats" );
            return false;
        }
    }
    if( m_state == StateCarryingBuilding )
    {
        END_PROFILE(  "SearchThreats" );
        return false;
    }


    //
    // Search for grenades, airstrikes nearby

    START_PROFILE( "FindThreatObjects" );

    int numEnemies = 0;
    double nearestThreatSqd = FLT_MAX;
    WorldObjectId threatId;
    bool throwableWeaponFound = false;

    double maxGrenadeRangeSqd = iv_pow( DARWINIAN_SEARCHRANGE_GRENADES, 2 );

    if( !HasRage() )
    {
        for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
        {
            if( g_app->m_location->m_effects.ValidIndex(i) )
            {
                WorldObject *wobj = g_app->m_location->m_effects[i];
                if( wobj->m_type == EffectThrowableGrenade ||
                    wobj->m_type == EffectThrowableAirstrikeMarker ||
                    (!g_app->Multiplayer() && wobj->m_type == EffectGunTurretTarget && m_id.GetTeamId() != wobj->m_id.GetTeamId() ) ||
                    (wobj->m_type == EffectSpamInfection && m_id.GetTeamId() != wobj->m_id.GetTeamId() ) )
                {
                    double distanceSqd = ( wobj->m_pos - m_pos ).MagSquared();

                    if( distanceSqd < maxGrenadeRangeSqd &&
                        distanceSqd < nearestThreatSqd )
                    {
                        nearestThreatSqd = distanceSqd;
                        threatId = wobj->m_id;
                        throwableWeaponFound = true;
                    }
                }
            }
        }
    }

    END_PROFILE( "FindThreatObjects" );

    //
    // If we found a grenade, run away immediately

    if( throwableWeaponFound )
    {
        m_state = StateCombat;
        m_scared = true;
        if( m_threatId != threatId )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatRunAway" );
			m_startedThreatSound = true;
            m_threatId = threatId;
        }
        END_PROFILE(  "SearchThreats" );       
        return true;
    }

    // Check for burning trees

    START_PROFILE( "Trees" );

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeTree )
            {
                Tree *tree = (Tree *)b;
                double dist = (m_pos - b->m_pos).Mag();
                if( tree->IsOnFire() )
                {
                    if( dist < tree->GetBurnRange() * 2.0 )
                    {
                        m_state = StateCombat;
                        m_threatId = b->m_id;
                        m_scared = true;

                        g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatRunAway" );
						m_startedThreatSound = true;

                        if( m_ordersSet )
                        {
                            // stop trying to run back into the tree constantly
                            m_ordersSet = false;
                            m_orders.Zero();
                        }
                        END_PROFILE( "Trees" );
                        END_PROFILE(  "SearchThreats" );
                        return true;
                    }
                }
            }
        }
    }

    END_PROFILE( "Trees" );

    double searchRange = m_threatRange;

    // search for nearby ants nests
    if( syncrand() % 500 == 0 )
    {
        START_PROFILE( "AntsNests" );
        double nearest = FLT_MAX;
        int id = -1;
        LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
        for( int i = 0; i < buildings->Size(); ++i )
        {
            Building *b = g_app->m_location->GetBuilding( buildings->GetData(i) );
            if( b )
            {
                if((b->m_type == Building::TypeAntHill ||
                    b->m_type == Building::TypeTriffid ) &&
					b->m_id.GetTeamId() != m_id.GetTeamId() )
                {
                    double dist = (m_pos - b->m_pos).Mag();
                    if( dist < searchRange &&
                        dist < nearest )
                    {
                        nearest = dist;
                        id = i;
                    }
                }
            }
        }

        END_PROFILE( "AntsNests" );

        if( id != -1 )
        {
            AttackBuilding( id );
            m_threatId.SetInvalid();
            END_PROFILE(  "SearchThreats" );
            return true;
        }
    }

    
    //
    // No explosives nearby.  Look for bad guys
    // Start with a quick evaluation of the area, by querying any AITarget buildings

    START_PROFILE( "AITargets" );

    if( !formationOfficer )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == Building::TypeAITarget )
                {
                    AITarget *target = (AITarget *) building;
                    double range = ( building->m_pos - m_pos ).Mag();
                    if( range < target->GetRange() )              // 100m
                    {
                        int numEnemiesNearby = target->m_enemyCount[ m_id.GetTeamId() ];
                        if( numEnemiesNearby == 0 )
                        {
                            END_PROFILE( "AITargets" );
                            END_PROFILE(  "SearchThreats" );
                            return false;
                        }
                    }
                }
            }
        }
    }
    
    END_PROFILE( "AITargets" );


    // Count the number of nearby friends and enemies
    // Also find the nearest enemy

    int numFound = 0;
    if( m_state == StateOperatingPort ) 
    {
        searchRange *= 0.5;
        Building *building = g_app->m_location->GetBuilding(m_buildingId);
        if( building && building->m_type == Building::TypeGunTurret )
        {
            searchRange = 0.0;
        }
    }
    
    Vector3 searchPos = m_pos;

        
    if( formationOfficer )
    {
        searchPos += formationOfficer->m_front * searchRange * 0.66;
    }
    
    if( m_controllerId != -1 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Task *task = team->m_taskManager->GetTask( m_controllerId );
        Unit *controller = NULL;
        if( task ) controller = g_app->m_location->GetUnit( task->m_objId );    
        if( controller && controller->m_troopType == Entity::TypeInsertionSquadie )
        {
            Vector3 focusPos = ((InsertionSquad *)controller)->m_focusPos;
            Vector3 diff = ( focusPos - m_pos );
            if( diff.Mag() > DARWINIAN_SEARCHRANGE_THREATS / 3.0 )
            {
                diff.SetLength( DARWINIAN_SEARCHRANGE_THREATS / 3.0 );
            }
            searchPos = m_pos + diff;
            searchRange = DARWINIAN_SEARCHRANGE_THREATS;
        }
    }

    START_PROFILE( "GetEnemies" );

    if( g_app->m_location->m_entityGrid->AreEnemiesPresent( searchPos.x, searchPos.z, searchRange, m_id.GetTeamId() ) )
    {
        std::vector<WorldObjectId> *ids = g_app->m_location->m_entityGridCache->GetEnemies( searchPos.x, searchPos.z, searchRange, &numFound, m_id.GetTeamId() );

        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = (*ids)[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity )
            {
                bool onFire = entity->m_type == TypeDarwinian && ((Darwinian *)entity)->IsOnFire();
                bool beingSacraficed = entity->m_type == TypeDarwinian && ((Darwinian *)entity)->m_state == StateBeingSacraficed ;

                if( !entity->m_dead && !onFire && !beingSacraficed &&
                    entity->m_type != TypeEgg &&
                    entity->m_type != Entity::TypeHarvester &&
                    entity->m_type != Entity::TypeEngineer &&
                    (entity->m_type != Entity::TypeArmour || ((Armour *)entity)->GetSpeed() < 15.0 ) )
                {
                    ++numEnemies;       

                    double distanceSqd = ( entity->m_pos - m_pos ).MagSquared();
                    if( distanceSqd < nearestThreatSqd )
                    {
                        nearestThreatSqd = distanceSqd;
                        threatId = id;
                    }
                }
            }
        }
    }

    END_PROFILE( "GetEnemies" );


    //
    // Decide what to do with our threat

    Entity *entity = g_app->m_location->GetEntity( threatId );

    if( entity && !entity->m_dead )
    {
        m_state = StateCombat;
        bool soldier = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian ) > 2;
        if( !g_app->Multiplayer() && m_id.GetTeamId() == 1 ) soldier = true;
                                
        if( !soldier ) m_scared = true;
        if( soldier )
        {            
            bool friendsPresent = g_app->m_location->m_entityGrid->AreFriendsPresent( m_pos.x, m_pos.z, searchRange, m_id.GetTeamId() );

            m_scared = numEnemies > 5 && !friendsPresent;
        }

        if( m_controllerId != -1 ) m_scared = false;

        if( m_boosterTaskId != -1 ) m_scared = false;

        if( entity->m_type == TypeSporeGenerator ||
            entity->m_type == TypeTripod ||
            entity->m_type == TypeSpider ||
            entity->m_type == TypeSoulDestroyer ||
            entity->m_type == TypeTriffidEgg ||
            entity->m_type == TypeInsertionSquadie ||
            entity->m_type == TypeArmour ||
            entity->m_type == TypeSpaceInvader )
        {
            m_scared = true;
        }

        if( m_threatId != threatId )
        {
            if( m_scared ) 
            {
                if( !m_startedThreatSound )
                {
                    g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatRunAway" );
				    m_startedThreatSound = true;
                }
            }
            else
            {
                if( !m_startedThreatSound )
                {
                    g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatAttack" );
                }

                static double lastAttackHorn = 0.0;
                double gameTimeNow = g_gameTimer.GetGameTime();
                if( frand(200) < 1.0 && 
                    !m_startedThreatSound &&
                    (gameTimeNow - lastAttackHorn) > 4.0 )
                {
                    lastAttackHorn = gameTimeNow;
                    g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatAttackHorn" );
                }
				m_startedThreatSound = true;
            }
            m_threatId = threatId;
        }

        if( m_ordersSet && (m_pos - m_orders).Mag() < 50.0 && !m_followingPlayerOrders)
        {
            // We got near enough
            m_ordersSet = false;
        }

        m_threatRange = iv_sqrt( nearestThreatSqd );
        END_PROFILE(  "SearchThreats" );
        return true;
    }    

    // There are no nearby threats
    m_threatId.SetInvalid();
	if( m_startedThreatSound )
	{
		g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );
		m_startedThreatSound = false;
	}
    
    END_PROFILE(  "SearchThreats" );
    return false;
}


bool Darwinian::SearchForPorts()
{
    START_PROFILE( "SearchPorts" );

    //
    // Build a list of available buildings

    LList<int> availableBuildings;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i]; 
            if( building->GetNumPorts() == 0 ) continue;
                
            double distanceToBuilding = ( building->m_pos - m_pos ).Mag();
            distanceToBuilding -= building->m_radius;
            if( distanceToBuilding < DARWINIAN_SEARCHRANGE_PORTS )
            {
                if( building->GetNumPortsOccupied() < building->GetNumPorts() &&
                    g_app->m_location->IsWalkable( m_pos, building->m_pos ) )
                {
                    availableBuildings.PutData( building->m_id.GetUniqueId() );
                }
            }
        }
    }

    if( availableBuildings.Size() == 0 )
    {
        END_PROFILE(  "SearchPorts" );
        return false;
    }
    

    //
    // Select a random building

    int chosenBuildingIndex = syncrand() % availableBuildings.Size();
    double distance = FLT_MAX;
    for( int i = 0; i < availableBuildings.Size(); ++i )
    {
        Building *b = g_app->m_location->GetBuilding( availableBuildings[i] );
        if( b )
        {
            double d = (m_pos - b->m_pos).Mag();
            if( d < distance )
            {
                distance = d;
                chosenBuildingIndex = i;
            }
        }
    }
    Building *chosenBuilding = g_app->m_location->GetBuilding( availableBuildings[chosenBuildingIndex] );
    AppDebugAssert(chosenBuilding);


    //
    // Build a list of available ports;

    LList<int> availablePorts;
    for( int p = 0; p < chosenBuilding->GetNumPorts(); ++p )
    {
        int limit = 20;
        if( g_app->Multiplayer() ) limit = 1;
        if( !chosenBuilding->GetPortOccupant(p).IsValid() &&
            chosenBuilding->GetPortOperatorCount(p,m_id.GetTeamId()) < limit )
        {	
            availablePorts.PutData(p);
        }
    }


    //
    // Select a random port

    if( availablePorts.Size() == 0 )
    {
        END_PROFILE(  "SearchPorts" );
        return false;
    }
        
    int randomSelection = syncrand() % availablePorts.Size();
    m_buildingId = chosenBuilding->m_id.GetUniqueId();
    m_portId = availablePorts[ randomSelection ];
    m_state = StateApproachingPort;
    Vector3 portPos, portFront;
    chosenBuilding->GetPortPosition( m_portId, portPos, portFront );    
    m_wayPoint = portPos;
    //m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );    

    END_PROFILE(  "SearchPorts" );
    return true;
}


bool Darwinian::SearchForCarryableBuilding()
{
    if( !g_app->Multiplayer() ) return false;

    // If game mode is not capture the statue
    //return false;

    START_PROFILE( "SearchCarryable" );

    //
    // Build a list of available buildings

    LList<int> availableBuildings;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( CarryableBuilding::IsCarryableBuilding( building->m_type ) )
            {
                CarryableBuilding *carriable = (CarryableBuilding *)building;
                                
                if( building->m_id.GetTeamId() == m_id.GetTeamId() &&
                    carriable->m_numLifters >= carriable->m_maxLifters )
                {
                    continue;
                }

                double distanceToBuilding = ( building->m_pos - m_pos ).Mag();
                distanceToBuilding -= carriable->m_radius;
                if( distanceToBuilding < DARWINIAN_SEARCHRANGE_CARRYABLE )
                {
                    availableBuildings.PutData( building->m_id.GetUniqueId() );
                }
            }
        }
    }

    if( availableBuildings.Size() == 0 )
    {
        END_PROFILE(  "SearchCarryable" );
        return false;
    }
    

    //
    // Select a random building

    int chosenBuildingIndex = syncrand() % availableBuildings.Size();
    Building *chosenBuilding = g_app->m_location->GetBuilding( availableBuildings[chosenBuildingIndex] );
    AppDebugAssert(chosenBuilding);


    //
    // Set us up to carry that building

    m_state = StateApproachingCarriableBuilding;
    m_buildingId = chosenBuilding->m_id.GetUniqueId();
    
    END_PROFILE(  "SeachCarryable" );
    return true;

}


bool Darwinian::BeginVictoryDance()
{
    if( m_onGround && m_id.GetTeamId() == 0 && syncfrand(5.0) < 1.0 && m_pos.y > 10.0 )
    {
	    LList <GlobalEventCondition *> *objectivesList = &g_app->m_location->m_levelFile->m_primaryObjectives;

        bool victory = true;
	    for (int i = 0; i < objectivesList->Size(); ++i)
	    {
		    GlobalEventCondition *gec = objectivesList->GetData(i);
		    if (!gec->Evaluate())
		    {
			    victory = false;
                break;
		    }
	    }    

    
        if( victory )
        {
            // jump!
            m_vel.y += 15.0 + syncfrand(15.0);
            m_onGround = false;
            g_app->m_soundSystem->TriggerEntityEvent( this, "VictoryJump" );
            return true;
        }
    }

    return false;
}


bool Darwinian::AdvanceToTargetPosition()
{
    if( m_paralyzeTimer > 0.0 ) return false; // we're paralyzed, no moving!
    if( m_wayPoint == g_zeroVector ) return false;

    START_PROFILE( "AdvanceToTargetPos" );

    Vector3 oldPos = m_pos;

    //
    // Are we there yet?

    if( m_infectionState == InfectionStateInfected )
    {
        if( syncrand() % 20 == 0 )
        {
            m_vel.Zero();
            m_wayPoint = m_pos;
            END_PROFILE(  "AdvanceToTargetPos" );
            return true;
        }
    }
    
    Vector3 vectorRemaining = m_wayPoint - m_pos;
    vectorRemaining.y = 0;
    double distance = vectorRemaining.Mag();
    if( distance == 0.0 )
    {
        m_vel.Zero();
        END_PROFILE(  "AdvanceToTargetPos" );
        return true;
    }
    else if (distance < 1.0)
    {
        m_pos = m_wayPoint;        
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
        END_PROFILE(  "AdvanceToTargetPos" );
        return false;
    }


    //
    // Work out where we want to be next

    double speed = m_stats[StatSpeed];
    if( m_state == StateIdle || 
        m_state == StateWorshipSpirit ) speed *= 0.2;

    if( m_state == StateUnderControl ) speed *= 1.75;

    double amountToTurn = SERVER_ADVANCE_PERIOD * 4.0;
    Vector3 targetDir = (m_wayPoint- m_pos).Normalise();
    Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();

    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;


    //
    // Slow us down if we're going up hill
    // Speed up if going down hill

    double currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
    double nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    double factor = 1.0 - (currentHeight - nextHeight) / -3.0;
    if( factor < 0.3 ) factor = 0.3;
    if( factor > 2.0 ) factor = 2.0;
    speed *= factor;


    //
    // Slow us down if we're near our objective

    if( distance < 10.0 )
    {
        double distanceFactor = distance / 10.0;
        speed *= distanceFactor;
    }

    newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    newPos = PushFromObstructions( newPos );
    //newPos = PushFromCliffs( newPos, oldPos );

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;
    
    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = ( newPos - oldPos ).Normalise();    

    END_PROFILE(  "AdvanceToTargetPos" );
    return false;    
}


Vector3 Darwinian::PushFromObstructions( Vector3 const &pos, bool killem )
{
    if( g_app->m_editing ) return pos;
    Vector3 result = pos;    
    if( m_onGround )
    {
        result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
    }

   // Matrix34 transform( m_front, g_upVector, result );

    //
    // Push from Water


    if( result.y <= 1.0 )
    {        
        START_PROFILE( "PushFromWater" );

        double pushAngle = syncsfrand(1.0);
        double distance = 40.0;
        while( distance < 100.0 )
        {
            double angle = distance * pushAngle * M_PI;
            Vector3 offset( iv_cos(angle) * distance, 0.0, iv_sin(angle) * distance );
            Vector3 newPos = result + offset;
            double height = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
            if( height > 1.0 )
            {
                result = newPos;
                result.y = height;
               // m_avoidObstruction = result;
                break;
            }
            distance += 5.0;
        }

        END_PROFILE(  "PushFromWater" );
    }
    

    //
    // Push from buildings

    START_PROFILE( "PushFromBuildings" );

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( result.x, result.z );

    int numHits = 0;
    Vector3 hitPosTotal;
    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building )
        {        
            if( building->m_type == Building::TypeLaserFence &&
				((LaserFence *) building)->IsEnabled() )
            {
                if( !g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId() ) )
                {
                    double closest = 5.0 + m_id.GetUniqueId() % 10;
                    if( building->DoesSphereHit( m_pos, 1.0 ) && killem )
                    {
                        //ChangeHealth( -999 );
					    SetFire();
                        ((LaserFence *) building)->Electrocute( m_pos );
                    }
                    else if( building->DoesSphereHit( result, closest ) )
                    {
                        LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( ((LaserFence *)building)->GetBuildingLink() );                    
                        Vector3 pushForce = (building->m_centrePos - result).SetLength(1.0);
                        if( nextFence )
                        {
                            Vector3 fenceVector = nextFence->m_pos - building->m_pos;
                            Vector3 rightAngle = fenceVector ^ g_upVector;
                            rightAngle.SetLength( (pushForce ^ fenceVector).y );
                            pushForce = rightAngle.SetLength(20.0);
                        }
                        while( building->DoesSphereHit( result, closest ) )
                        {
                            result -= pushForce;
                        }
                        //m_avoidObstruction = result;
                        //m_state = StateIdle;
                        //m_wayPoint = m_pos - pushForce;
                        //m_ordersSet = false;
                        //m_followingPlayerOrders = false;
                    }
                }
            }
            else if( building->m_type == Building::TypeWall )
            {
                if( building->DoesSphereHit( result, 4.0 ) )
                {
                    Vector3 toBuilding = building->m_pos - result; 

                    double sign = -1.0;
                    if( toBuilding * building->m_front >= 0 )
                        sign = 1.0;

                    Vector3 pushForce = 5.0 * sign * building->m_front;

                    while( building->DoesSphereHit( result, 1.0 ) )
                    {
                        result -= pushForce;
                    }

                    m_wallHits++;
                }
            }
            else
            {               
                if( building->DoesSphereHit( result, 30.0 ) )
                {
                    Vector3 pushForce = (building->m_pos - result);//.SetLength(3.0);
                    pushForce.y = 0.0;
                    pushForce.SetLength(4.0);
                    bool hit = false;
                    while( building->DoesSphereHit( result, 2.0 ) )
                    {
                        result -= pushForce;                
                        hit = true;
                        result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                    }

                    /*if( hit )
                    {
                        if( building->m_type == Building::TypeTree &&
                            m_hotFeetTaskId != -1 )
                        {
                            Tree *tree = (Tree *)building;
                            tree->Damage( -1.0 );
                        }
                    }*/
                }
            }
            //break;
        }
    }

    END_PROFILE(  "PushFromBuildings" );

    //
    // If we already have some avoidance rules, 
    // follow them above all else
    
    if( m_avoidObstruction != g_zeroVector )
    {
        double distance = ( m_avoidObstruction - pos ).Mag();
        if( distance < 10.0 )
        {
            m_avoidObstruction = g_zeroVector;
        }
        else
        {
            return m_avoidObstruction;
        }   
    }

    return result;
}


void Darwinian::TakeControl( int _controllerId )
{
    Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
    Task *controller = team->m_taskManager->GetTask( _controllerId );

    if( controller )
    {
        m_controllerId = _controllerId;
        m_state = StateUnderControl;
        m_ordersSet = false;
        m_officerTimer = 0.0;
        
        int numFlashes = 5 + AppRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(5.0), frand(15.0), sfrand(5.0) );
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash );
        }

        g_app->m_soundSystem->TriggerEntityEvent( this, "TakenControl" );
    }
}


void Darwinian::LoseControl()
{
    m_state = StateIdle;
    m_controllerId = -1;

    int numFlashes = 5 + AppRandom() % 5;
    for( int i = 0; i < numFlashes; ++i )
    {
        Vector3 vel( sfrand(5.0), frand(15.0), sfrand(5.0) );
        g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash );
    }        

    g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian TakenControl" );
    g_app->m_soundSystem->TriggerEntityEvent( this, "EscapedControl" );
}


void Darwinian::AntCapture( WorldObjectId _antId )
{
    m_threatId = _antId;
    m_state = StateCapturedByAnt;
}


bool Darwinian::IsInView()
{
    return g_app->m_camera->PosInViewFrustum( m_pos );
}


bool Darwinian::AdvanceOnFire()
{
	SyncRandLog("before waypoint calc: %s", LogState());
    m_wayPoint = m_pos;
    m_wayPoint += Vector3( SFRAND(100.0), 0.0, SFRAND(100.0) );
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );    
	SyncRandLog("after waypoint calc: %s", LogState());

    if( m_onGround ) AdvanceToTargetPosition();
	SyncRandLog("after advance: %s", LogState());

    int numFireParticles = AppRandom() % 8;
    for( int i = 0; i < numFireParticles; ++i )
    {
        Vector3 fireSpawn = m_pos + g_upVector * frand(5);
        //fireSpawn -= m_vel * 0.1;
        double fireSize = 20 + frand(30.0);
        Vector3 fireVel = m_vel * 0.3 + g_upVector * (3+frand(3));
        int particleType = Particle::TypeDarwinianFire;
        if( i > 4 ) particleType = Particle::TypeMissileTrail;
        g_app->m_particleSystem->CreateParticle( fireSpawn, fireVel, particleType, fireSize );
    }

    if( AppRandom() % 30 == 0 )
    {
        g_app->m_soundSystem->TriggerEntityEvent( this, "OnFire" );
    }

    if( !m_dead && 
        m_onGround &&
        syncfrand(10) < 2.0 / g_gameTimer.GetGameSpeed() ) 
    {
        ChangeHealth(-2);
    }
        
    return false;
}


bool Darwinian::AdvanceInWater()
{
    ChangeHealth( -999 );

    return false;
}


bool Darwinian::AdvanceSacraficing()
{
    Shaman *shaman = (Shaman *)g_app->m_location->GetEntitySafe( m_shamanId, Entity::TypeShaman );
    if( !shaman ||
        shaman->m_paralyzed ||
        shaman->m_dead )
    {
        if( m_burning )
        {
            m_state = StateOnFire;
        }
        else
        {
            m_state = StateIdle;
        }
        return false;
    }

    if( m_dead ) 
    {
        shaman->m_sacrafices--;
        m_state = StateIdle;
        return true;
    }

    if( m_sacraficeTimer < 5.0 )
    {
        m_vel.y = 1.5;
        m_vel.x *= 0.5;
        m_vel.z *= 0.5;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        m_sacraficeTimer += SERVER_ADVANCE_PERIOD;
    }
    else
    {
        OrbitPortal();
    }

    if( m_burning )
    {
        int numFireParticles = AppRandom() % 8;
        for( int i = 0; i < numFireParticles; ++i )
        {
            Vector3 fireSpawn = m_pos + g_upVector * frand(5);
            //fireSpawn -= m_vel * 0.1;
            double fireSize = 20 + frand(30.0);
            Vector3 fireVel = m_vel * 0.3 + g_upVector * (3+frand(3));
            int particleType = Particle::TypeDarwinianFire;
            if( i > 4 ) particleType = Particle::TypeMissileTrail;
            g_app->m_particleSystem->CreateParticle( fireSpawn, fireVel, particleType, fireSize );
        }

        if( AppRandom() % 100 == 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "OnFire" );
        }

        m_sacraficeTimer += SERVER_ADVANCE_PERIOD;
    }

    if( m_sacraficeTimer > 10.0 )
    {
        Portal *p = (Portal *)g_app->m_location->GetBuilding( m_targetPortal );
        if( p && p->m_type == Building::TypePortal )
        {
            // give spirit to shaman and kill this darwinian
            m_dead = true;
            m_state = StateIdle;
            m_spiritId = g_app->m_location->SpawnSpirit(m_pos, m_vel, m_id.GetTeamId(), m_id );
            Spirit *s = g_app->m_location->GetSpirit(m_spiritId);
            if( s )
            {
                s->m_state = Spirit::StateShaman;            

                //shaman->m_sacrafices--;
//                p->GiveSpirit( m_spiritId, shaman->m_id.GetTeamId() );
            }

            // Create a zombie
            Zombie *zombie = new Zombie();
            zombie->m_pos = m_pos;
            zombie->m_front = m_front;
            zombie->m_up = g_upVector;
            zombie->m_up.RotateAround( zombie->m_front * syncsfrand(1) );

            Vector3 direction = (m_pos - p->GetPortalCenter() ).Normalise();
            zombie->m_vel = direction * (30.0 + syncfrand(20.0));

            int index = g_app->m_location->m_effects.PutData( zombie );
            zombie->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
            zombie->m_id.GenerateUniqueId();
        }

        return true;
    }

    return false;
}

bool Darwinian::AdvanceInVortex()
{
    if( DustBall::s_vortexPos != g_zeroVector )
    {
        double speed = 100.0;
        speed *= (m_pos.y / 1000.0);
        speed = max( speed, 30.0 );
        Vector3 vortexPos = DustBall::s_vortexPos;

        double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;
        double radius = (300.0 * (m_pos.y / 1000.0) );

        Vector3 targetPos = vortexPos;
        targetPos.x += iv_cos( timeIndex ) * radius;
        targetPos.z += iv_sin( timeIndex ) * radius;
        targetPos.y = m_pos.y + 50.0;
        double maxHeight = 500.0 + (5.0 * (m_id.GetUniqueId() % 100 ) );
        targetPos.y = min ( maxHeight, targetPos.y );

        Vector3 oldPos = m_pos;
        Vector3 actualDir = (targetPos - m_pos).Normalise();

        Vector3 newPos = m_pos + (actualDir * speed * SERVER_ADVANCE_PERIOD);
        Vector3 moved = newPos - oldPos;
        if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
        
        m_pos = newPos;       
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    }
    else
    {
        m_state = StateIdle;
        m_vel.SetLength( 50.0 );
    }

    return false;
}

bool Darwinian::AdvanceBeingAbducted()
{
    m_onGround = false;
    SpaceShip *ship = (SpaceShip *)g_app->m_location->GetEntity( m_spaceshipId );
    if( ship && ship->m_type == Entity::TypeSpaceShip && ship->m_state == SpaceShip::ShipStateAbducting )
    {
        m_wayPoint = ship->m_pos;
        
        Vector3 targetVel = (m_wayPoint - m_pos).Normalise();
        targetVel.SetLength( m_stats[StatSpeed] * 2.0 );

        if( ship->m_vel.Mag() > 0 )
        {
            Vector3 shipPos = ship->m_pos;
            shipPos.y = m_pos.y;
            targetVel += ( shipPos - m_pos ).SetLength( ship->m_vel.Mag() );
        }

        Vector3 rightAngle = ( m_pos - ship->m_pos ) ^ g_upVector;
        rightAngle.HorizontalAndNormalise();
        targetVel += rightAngle * 50;

        double timeFactor = SERVER_ADVANCE_PERIOD * 0.25 + fmodf( m_id.GetUniqueId(), 10 ) * 0.1 * SERVER_ADVANCE_PERIOD;
        m_vel = m_vel * (1-timeFactor) + targetVel * timeFactor;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        if( syncfrand(1.0) < 0.5 )
        {
            g_app->m_particleSystem->CreateParticle( m_pos, m_vel * -0.1, Particle::TypeBlueSpark, 30, RGBAColour(150,150,255,155) );            
        }

        if( (m_pos - ship->m_pos).Mag() < 20.0 ||
            m_pos.y > ship->m_pos.y )
        {
            ship->IncreaseAbductees();
            return true;
        }
    }
    else
    {
        m_state = StateIdle;
    }

    return false;
}

/*bool Darwinian::AdvanceJumpPadLaunch()
{
    JumpPad *pad = (JumpPad *)g_app->m_location->GetBuilding(m_buildingId);
    if( pad && pad->m_type == Building::TypeJumpPad )
    {
        Vector3 oldPos = m_pos;
        Vector3 direction = (m_wayPoint - pad->m_pos).Normalise();
        double totalDistance = (m_wayPoint - pad->m_pos).Mag();
        double currentDistance = (m_wayPoint - oldPos).Mag();
        double flightTime = 5.0;

        double radius = totalDistance / 2.0;
        Vector3 centrePoint = (pad->m_pos + m_wayPoint) / 2.0;
        centrePoint.y = 0.0;
        Vector3 to = (m_wayPoint - centrePoint).Normalise();
        Vector3 from = (centrePoint - pad->m_pos).Normalise();
        //double startAngle = from * g_upVector;
        double startAngle = from * to;

        /*Vector3 fromPos = pad->m_pos;
        fromPos.y = 0.0;
        double angleToGround = ((centrePoint - pad->m_pos).Normalise()) * ((fromPos - centrePoint).Normalise());
        fromPos = m_wayPoint;
        fromPos.y = 0.0;
        angleToGround += ((centrePoint - m_wayPoint).Normalise()) * ((fromPos - centrePoint).Normalise());

        startAngle += angleToGround;

        startAngle = -1.5;

        double scale = (GetNetworkTime() - m_launchTime) / flightTime;
        double angle = -startAngle + ((2.0 * M_PI * scale) / 5.0);

        Vector3 point = centrePoint;
        point += (iv_cos(angle)) * (g_upVector * radius);
        point += (iv_sin(angle)) * (direction * radius);

        m_pos = point;
        m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

        if( m_pos.y <= g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z ) )
        {
            m_onGround = true;
            m_launchTime = 0.0;
            m_buildingId = -1;
            m_wayPoint = m_pos;
            m_orders = g_zeroVector;
            m_state = StateIdle;
            m_vel.Zero();
        }
    }

    return false;
}*/

void Darwinian::AdvanceHotFeet()
{
    int numFireParticles = AppRandom() % 8;
    for( int i = 0; i < numFireParticles; ++i )
    {
        Vector3 fireSpawn = m_pos;
        fireSpawn.y = g_app->m_location->m_landscape.m_heightMap->GetValue( fireSpawn.x, fireSpawn.z );
        //fireSpawn -= m_vel * 0.1;
        double fireSize = 20 + frand(30.0);
        Vector3 fireVel = m_vel * 0.3;// + g_upVector * (3+frand(3));
        int particleType = Particle::TypeHotFeet;
        if( i > 4 ) particleType = Particle::TypeMissileTrail;
        g_app->m_particleSystem->CreateParticle( fireSpawn, fireVel, particleType, fireSize );
    }

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building )
        { 
            if( building->m_type == Building::TypeTree &&
                building->DoesSphereHit( m_pos, 2.0 ) )
            {
                Tree *tree = (Tree *)building;
                tree->Damage( -1.0 );
            }
        }
    }
}

bool Darwinian::AdvanceInfection()
{
    if( rand() % 2 == 0 )
    {
        Vector3 vel = m_vel;
        vel.x += frand(1.0);
        vel.z += frand(1.0);
        vel.y += frand(3.0);

        double size = 20.0 + frand(20.0);

        Vector3 pos = m_pos;
        pos.y += 4.0;

        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypePlague, size, RGBAColour(140, 200, 5, 150) );
    }

    m_infectionTimer -= SERVER_ADVANCE_PERIOD;
    if( m_infectionTimer <= 0.0 )
    {
        m_infectionTimer = syncfrand(5.0) + 5.0;

        // can infect all Darwinians, regardless of team
        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, DARWINIAN_INFECTIONRANGE, &numFound );

        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = s_neighbours[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && entity->m_type == Entity::TypeDarwinian && !entity->m_dead )
            {
                Darwinian *d = (Darwinian *)entity;
                if( syncrand() % 2 == 0 )
                {
                    // 50% chance to infect nearby Darwinians
                    d->Infect();
                }
            }
        }

        int index = syncrand() % 20;
        switch( index )
        {
            case 0:
            {
                // worst case scenario - the darwinian explodes

                g_app->m_location->Bang(m_pos, 5.0, 25);
                ChangeHealth(-100);
                break;
            }

            case 1:
            {
                // pretty bad - the Darwinian dies
                ChangeHealth(-100);
                break;
            }

            case 2:
            {
                // the plague slows the darwinian
                m_stats[StatSpeed] /= 2.0;
                break;
            }

            case 3:
            {
                // the darwinian becomes paralyzed
                m_paralyzeTimer = 5.0 + syncfrand(10.0);
                m_vel.Zero();
                break;
            }

            case 4:
            {
                // the darwinian has gone insane and starts attacking randomly with grenades
                m_insane = true;
                m_state = StateCombat;
                break;
            }

            case 5:
            case 6:
            {
                // the darwinian becomes immune to the infection and is no longer contagious
                m_infectionState = InfectionStateImmune;
                m_stats[StatSpeed] = EntityBlueprint::GetStat( Entity::TypeDarwinian, StatSpeed );
                break;
            }
        }
    }

    return false;
}

void Darwinian::OrbitPortal()
{
    double timeIndex = GetNetworkTime() + m_id.GetUniqueId() * 20;

    Portal *portal  = (Portal *)g_app->m_location->GetBuilding( m_targetPortal );

    if( !portal ) return;

    Vector3 vortexPos = portal->GetPortalCenter();
    double radius = portal->m_portalRadius;
   
    
    Vector3 targetPos;
    double speed = 70.0;
    if( m_burning ) speed = 30.0;

    double size = 3.0;  
    size *= (1.0 + 0.03 * (( m_id.GetIndex() * m_id.GetUniqueId() ) % 10));

    targetPos = vortexPos;
    targetPos.x += iv_cos( timeIndex ) * radius;
    targetPos.z += iv_sin( timeIndex ) * radius;
    targetPos.y -= size;
    targetPos.y += iv_sin ( timeIndex * 3.0 ) * 10.0;

    m_wayPoint = targetPos;

    Vector3 oldPos = m_pos;    

    double amountToTurn = SERVER_ADVANCE_PERIOD * 8.0;
    Vector3 targetDir = (m_wayPoint- m_pos).Normalise();
    Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();

    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;
    if( newPos.y < g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z ) + 5.0 )
    {
        newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z ) + 5.0;
    }
    
    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = ( newPos - oldPos ).Normalise();  

    if( (m_pos - vortexPos).Mag() <= portal->m_portalRadius + 5.0 ) m_burning = true;
}

void Darwinian::SetFire()
{
    if( m_state != StateInsideArmour &&
        m_state != StateBeingSacraficed &&
        m_boosterTaskId == -1 &&
        m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() )
    {
		SyncRandLog( "Setting fire to Darwinian\n" );
        m_state = StateOnFire;    
    }
}


bool Darwinian::IsOnFire()
{
    return( m_state == StateOnFire );
}

void Darwinian::Abduct( WorldObjectId _shipId )
{
    m_state = Darwinian::StateBeingAbducted;
    m_spaceshipId = _shipId;
}

bool Darwinian::IsInvincible( bool _againstExplosions )
{
    if( _againstExplosions )
    {
        return ( m_state == StateInsideArmour ||
                 m_state == StateBeingSacraficed );
    }
    else
    {
        return ( m_boosterTaskId != -1 ||
                 m_state == StateInsideArmour ||
                 m_state == StateBeingSacraficed );
    }
}

inline void SetTexture( const char *texName )
{
    int texId = g_app->m_resource->GetTexture(texName);
    glBindTexture   ( GL_TEXTURE_2D, texId );
}

void SafeSetTexture( const char *texName )
{
    glEnd           ();
	SetTexture( texName );
    glBegin         ( GL_QUADS );
}

inline void SetBlend( int src, int dst, bool depthmask = true )
{
    glBlendFunc ( src, dst );
    glDepthMask(depthmask);
}

void SafeSetBlend( int src, int dst, bool depthmask = true )
{
    glEnd       ();
	SetBlend( src, dst, depthmask );
    glBegin     ( GL_QUADS );
}

void Darwinian::Render( double _predictionTime, double _highDetail )
{
    if( m_destroyed ) return;
    if( !m_enabled ) return;
    if( m_state == StateInsideArmour )
    {
        return;
    }
	
    bool monster = ( m_id.GetTeamId() == g_app->m_location->GetMonsterTeamId() );
    bool future = (m_futurewinian );

    RGBAColour colour;
    if( m_id.GetTeamId() >= 0 ) colour = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;

    if( m_colourTimer > 0.0 )
    {
        /*if((int)m_colourTimer % 2 == 0 )
        {
            colour = g_app->m_location->m_teams[m_previousTeamId]->m_colour;
        }*/
        float t = m_colourTimer / 4.0;
        RGBAColour oldColour = g_app->m_location->m_teams[m_previousTeamId]->m_colour;
        colour = (t * oldColour) + ((1.0 - t) * colour);
        m_colourTimer -= _predictionTime;
    }
    
    float camDist = ( g_app->m_camera->GetPos() - m_pos ).Mag();

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Vector3 entityUp = g_upVector;

    if( _highDetail > 0.0 && camDist < 1300 )
    {
        if( m_onGround  )
        {
            predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
        }
        else if( !m_onGround && 
                  m_state != StateBeingSacraficed &&
                  m_state != StateInWater &&
                  m_state != StateBeingAbducted )
        {
            predictedPos.y += 3.0;
        }

        entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
        if( m_state == StateBeingSacraficed ) 
        {
            entityUp = g_upVector;
        }

        if( m_state == StateWorshipSpirit ||
            m_state == StateOperatingPort ||
            m_state == StateWatchingSpectacle ||
            m_state == StateOpeningCrate )
        {
            entityUp.RotateAround( m_front * iv_sin(g_gameTimer.GetAccurateTime()) * 0.3 );
            entityUp.Normalise();
        }
    }
    
    if( !m_onGround && 
        m_state != StateBeingSacraficed &&
        m_state != StateInWater &&
        !future ) 
    {
        Vector3 rotateAmount = m_front * g_gameTimer.GetAccurateTime() * (m_id.GetUniqueId() % 10) * 0.1;

        if( m_id.GetUniqueId() % 2 == 0 )   entityUp.RotateAround( rotateAmount );        
        else                                entityUp.RotateAround( rotateAmount * -1.0 );        
    }
        
    Vector3 entityRight(m_front ^ entityUp);

    if( m_state == StateCarryingBuilding )
    {
        Building *building = g_app->m_location->GetBuilding(m_buildingId);

        if( building )
        {
            Vector3 front = (building->m_pos - predictedPos).Normalise();            
            entityRight = front ^ entityUp;
            entityUp.RotateAround( entityRight * 0.3 );
        }
    }

    if( m_state == StateCapturedByAnt )
    {
        Vector3 capturedRight = entityUp;
        entityUp = m_vel * -1;
        entityUp.Normalise();
        entityRight = capturedRight;
        predictedPos += Vector3( 0, 4, 0 );
    }

    double size = 3.0;  

    float scaleFactor = m_scaleFactor; // g_prefsManager->GetFloat( "DarwinianScaleFactor" ); DEF: Optimization.

    if( scaleFactor > 0.0 )
    {
        if( camDist > 100 ) size *= 1.0 + scaleFactor * ( camDist - 100 ) / 1000;

        if( camDist > 300 )
        {
            double factor = ( camDist - 300 ) / 1000;
            if( factor < 0.0 ) factor = 0.0;
            if( factor > 1.0 ) factor = 1.0;

            entityUp = ( entityUp * (1-factor) ) + ( g_app->m_camera->GetUp() * factor );                                                       
            if( m_front * g_app->m_camera->GetFront() < 0 )
            {
                entityRight = ( entityRight * (1-factor) ) + ( g_app->m_camera->GetRight() * factor );
            }
            else
            {
                entityRight = ( entityRight * (1-factor) ) + ( -g_app->m_camera->GetRight() * factor );
            }
        }
    }

    size *= (1.0 + 0.03 * (( m_id.GetIndex() * m_id.GetUniqueId() ) % 10));
    if( m_rageTaskId != -1 ) 
    {
        float factor = 1.0 + abs(sinf(g_gameTime)) * 0.5;
        size *= factor;
    }
        
    entityRight.Normalise();
    entityUp.Normalise();

    entityRight *= size;
    entityUp *= size * 2.0;

    
    //
    // Draw our shadow on the landscape
    // Optimisation : dont bother if we're a long way away

    float maxShadowDistance = 2000;

    if( _highDetail > 0.0 && 
        m_shadowBuildingId == -1 && 
        !m_dead && m_state != StateBeingAbducted &&
        camDist < maxShadowDistance )
    {
        if( m_futurewinian ) SetTexture( "sprites/futurewinian.bmp" );
        int alpha = 150 * _highDetail;
        if( monster ) alpha *= 0.3;
        alpha = min( alpha, 255 );
        glColor4ub( 0, 0, 0, alpha  );
        
        Vector3 pos1 = predictedPos - entityRight;
        Vector3 pos2 = predictedPos + entityRight;
        Vector3 pos4 = pos1 + Vector3( 0.0, 0.0, size * 2.0 );
        Vector3 pos3 = pos2 + Vector3( 0.0, 0.0, size * 2.0 );

        pos1.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos1.x, pos1.z );
        pos2.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos2.x, pos2.z );
        pos3.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos3.x, pos3.z );
        pos4.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos4.x, pos4.z );

        glDepthMask( false );
        glLineWidth( 1.0 );
        glBegin( GL_QUADS );
            glTexCoord2f(0.0, 0.0);       glVertex3dv(pos1.GetData());
            glTexCoord2f(1.0, 0.0);       glVertex3dv(pos2.GetData());
            glTexCoord2f(1.0, 1.0);       glVertex3dv(pos3.GetData());
            glTexCoord2f(0.0, 1.0);       glVertex3dv(pos4.GetData());
        glEnd();
        glDepthMask( true );
    }


    if( !m_dead || !m_onGround )
    {

        //
        // Draw our texture

        double maxHealth = EntityBlueprint::GetStat(m_type, StatHealth);
        double health = (double) m_stats[StatHealth] / maxHealth;
        if( health > 1.0 ) health = 1.0;
		colour *= 0.3 + 0.7 * health;
        colour.a = 255;
        
        bool ouchUsed = false;

        if( m_dead )
        {
            glEnable( GL_BLEND );
            colour.a = 2.5 * (double) m_stats[StatHealth];
            colour.r *= 0.2;
            colour.g *= 0.2;
            colour.b *= 0.2;
        }
        
        if( m_state == StateOnFire ||
            m_burning  )
        {
            //colour.r *= 0.01;
            //colour.g *= 0.01;
            //colour.b *= 0.01;
            
            SetTexture( "sprites/darwinian_ouch4.bmp" );
            ouchUsed = true;
        }

        if( m_paralyzeTimer > 0.0 )
        {
            colour.r = 150;
            colour.g = 150;
            colour.b = 150;
        }        

        if( !m_onGround && m_dead && m_id.GetUniqueId() % 10 < 3 )
        {
            int chosenIndex = 1 + m_id.GetUniqueId() % 10;
            char chosenOuch[256];
            sprintf( chosenOuch, "sprites/darwinian_ouch%d.bmp", chosenIndex );
            SetTexture( chosenOuch );            
            ouchUsed = true;
        }

		
		
        if( m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
            !m_futurewinian )
        {
            glColor4ubv(colour.GetData());
            glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight + entityUp).GetData() );
                glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight + entityUp).GetData() );
                glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight).GetData() );
                glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight).GetData() );
            glEnd();
        }
        
        if( future )
        {
            double maxHealth = EntityBlueprint::GetStat(m_type, StatHealth);
            double health = (double) m_stats[StatHealth] / maxHealth;
            if( health > 1.0 ) health = 1.0;
            double colourFactor = 0.3 + 0.7 * health;

            SetTexture( "sprites/futurewinian.bmp" );
            glColor4f   ( 0.5 * colourFactor, 0.5 * colourFactor, 0.5 * colourFactor, 1.0); 
            double factor = 1.3;
            glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight * factor + entityUp * factor).GetData() );
                glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight * factor + entityUp * factor ).GetData() );
                glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight * factor).GetData() );
                glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight * factor).GetData() );
            glEnd();
    
            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glDisable   ( GL_DEPTH_TEST );
            glEnable    ( GL_BLEND );                

            double scale = 2.0;
            Vector3 up = g_app->m_camera->GetUp();
            Vector3 right = g_app->m_camera->GetRight();
            /*right.SetLength( entityRight.Mag() );
            up.SetLength( entityUp.Mag() );
            right *= scale;
            up *= scale;*/
            double alpha = iv_abs(iv_sin(g_gameTime * 2.0)) * 0.3;
            RGBAColour col = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;

            Vector3 bubblePos = predictedPos;
            double bubbleSize = size * 2.0;
            bubblePos.y += bubbleSize / 2.0;
            
            glColor4f ( col.r, col.g, col.b, alpha );
            glBegin(GL_QUADS);
                glTexCoord2i( 0, 0 );       glVertex3dv( (bubblePos - right * bubbleSize - up * bubbleSize).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (bubblePos - right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (bubblePos + right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (bubblePos + right * bubbleSize - up * bubbleSize).GetData() );  
            glEnd();

            glEnable    ( GL_DEPTH_TEST );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );            
        }   

        if( ouchUsed )
        {            
            SetTexture( "sprites/darwinian.bmp" );            
        }

        //glDisable( GL_BLEND );
       
        //
        // Draw a blue glow if we are under control

        if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
        {
            double factor = 1.0;
            if( m_futurewinian ) 
            {
                SetTexture( "sprites/futurewinian.bmp" );
                factor = 1.3;
            }
            if( (m_controllerId != -1 || m_underPlayerControl) 
                && !m_dead )
            {
                SetBlend( GL_SRC_ALPHA, GL_ONE );
                glEnable    ( GL_BLEND );    
                glDisable   ( GL_DEPTH_TEST );
                double alpha = iv_abs(iv_sin(g_gameTime * 5.0));
                if( alpha < 0.3 ) alpha = 0.3;
                glColor4f   ( 0.1, 0.1, 1.0, alpha );
				glBegin(GL_QUADS);
                for( int lol = 0; lol < 10; ++lol )
                {
                    
                    glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight * factor + entityUp * factor).GetData() );
                    glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight * factor + entityUp * factor).GetData() );
                    glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight * factor).GetData() );
                    glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight * factor).GetData() );
                }
				glEnd();
                glEnable    ( GL_DEPTH_TEST );
                SetBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );                
            }
        }    

        if( m_rageTaskId != -1 && !m_dead )
        {
            float factor = 1.0;
            if( m_futurewinian ) 
            {
                SetTexture( "sprites/futurewinian.bmp" );
                factor = 1.3;
            }
            if( monster )
            {
                SetTexture( "sprites/darwinian_ouch4.bmp" );
                factor = 1.5;
            }
            SetBlend( GL_SRC_ALPHA, GL_ONE );
            glEnable    ( GL_BLEND );    
            glDisable   ( GL_DEPTH_TEST );
            float alpha = fabs(sinf(g_gameTime * 2.0));
            if( alpha < 0.3 ) alpha = 0.3;
            glColor4f( 200,200,200,alpha);
            //for( int lol = 0; lol < 10; ++lol )
            {
                glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight * factor + entityUp * factor ).GetData() );
                glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight * factor + entityUp * factor ).GetData() );
                glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight * factor ).GetData() );
                glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight * factor ).GetData() );
                glEnd();
            }
            glEnable    ( GL_DEPTH_TEST );
            SetBlend ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }    


        // Monster team

        if( m_infectionState == InfectionStateInfected && !m_dead )
        {
            SetTexture( "textures/glow.bmp" );
            SetBlend( GL_SRC_ALPHA, GL_ONE, false );
            glEnable    ( GL_BLEND );
            //glDisable( GL_ALPHA_TEST );

            double scale = 2.0;
            Vector3 up = g_app->m_camera->GetUp();
            Vector3 right = g_app->m_camera->GetRight();

            double alpha = fabs(sinf(g_gameTime * 2.0)) * 0.8;
            alpha = max( 0.0, alpha ) * 255.0;

            RGBAColour col( 90,130,10, alpha );

            Vector3 bubblePos = predictedPos;
            float bubbleSize = size * 2.0;
            bubblePos.y += bubbleSize / 2.0;
            
            glColor4ubv( col.GetData() );
            glBegin(GL_QUADS);
                glTexCoord2i( 0, 0 );       glVertex3dv( (bubblePos - right * bubbleSize - up * bubbleSize).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (bubblePos - right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (bubblePos + right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (bubblePos + right * bubbleSize - up * bubbleSize).GetData() );  
            glEnd();

            //glEnable( GL_ALPHA_TEST );
            SetBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            SetTexture( "sprites/darwinian.bmp" );
        }

        if( monster )
        {
            SetTexture( "sprites/darwinian_ouch4.bmp" );            
            glEnable    ( GL_BLEND );                
            glDisable   ( GL_ALPHA_TEST );
            glColor4f   ( 1.0, 1.0, 1.0, 1.0); 
            float factor = 1.5;
            glBegin(GL_QUADS);
            for( int i = 0; i < 2; ++i )
            {
                glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight * factor + entityUp * factor).GetData() );
                glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight * factor + entityUp * factor ).GetData() );
                glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight * factor).GetData() );
                glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight * factor).GetData() );
            }
            glEnd();
            glEnable( GL_ALPHA_TEST );
            SetBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            SetTexture( "sprites/darwinian.bmp" );            
        }
                
        // Draw a glow if we have a booster
        if( m_boosterTaskId != -1 && !m_dead )
        {
            SetTexture( "textures/bubble.bmp" );            
            SetBlend( GL_SRC_ALPHA, GL_ONE, false );
            glEnable    ( GL_BLEND );    
            glDisable   ( GL_ALPHA_TEST );
            double scale = 2.0;
            Vector3 up = g_app->m_camera->GetUp();
            Vector3 right = g_app->m_camera->GetRight();
            /*right.SetLength( entityRight.Mag() );
            up.SetLength( entityUp.Mag() );
            right *= scale;
            up *= scale;*/
            double alpha = iv_abs(iv_sin(g_gameTime * 2.0)) * 0.5;
            RGBAColour col = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;

            Vector3 bubblePos = predictedPos;
            double bubbleSize = size * 2.0;
            bubblePos.y += bubbleSize / 2.0;
            
            glColor4f ( col.r, col.g, col.b, alpha );
            glBegin(GL_QUADS);
                glTexCoord2i( 0, 0 );       glVertex3dv( (bubblePos - right * bubbleSize - up * bubbleSize).GetData() );
                glTexCoord2i( 0, 1 );       glVertex3dv( (bubblePos - right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 1 );       glVertex3dv( (bubblePos + right * bubbleSize + up * bubbleSize).GetData() );
                glTexCoord2i( 1, 0 );       glVertex3dv( (bubblePos + right * bubbleSize - up * bubbleSize).GetData() );  
            glEnd();
            glEnable    ( GL_ALPHA_TEST );
            SetBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );            
            SetTexture( "sprites/darwinian.bmp" );
        }

        //
        // Draw shadow if we are watching a God Dish

        if( _highDetail > 0.0 && m_shadowBuildingId != -1 )
        {
            Building *building = g_app->m_location->GetBuilding( m_shadowBuildingId );
            if( building )
            {
                double alpha = 1.0;
                if( building->m_type == Building::TypeGodDish )
                {
                    GodDish *dish = (GodDish *) building;
                    if( !dish->m_activated && dish->m_timer < 1.0 )
                    {
                        m_shadowBuildingId = -1;
                    }
                    alpha = dish->m_timer * 0.1;
                }
                else if( building->m_type == Building::TypeEscapeRocket )
                {
                    EscapeRocket *rocket = (EscapeRocket *) building;
                    if( !rocket->IsSpectacle() )
                    {
                        m_shadowBuildingId = -1;
                    }
                    float distance = ( building->m_pos - m_pos ).Mag();
                    if( g_app->Multiplayer() )
                    {
                        alpha = 1.0;
                    }
                    else
                    {
                        if( distance < 400.0 )
                        {
                            alpha = 1.0;
                        }
                        else if( distance < 700.0 )
                        {
                            alpha = ( 700 - distance ) / 300.0;
                        }
                        else
                        {
                            alpha = 0.0;
                        }
                    }
                }
                alpha = min( alpha, 1.0 );
                alpha = max( alpha, 0.0 );

                if( alpha > 0.0 )
                {
                    Vector3 length = ( predictedPos - building->m_pos ).SetLength( size * 10 );

                    // Shadow behind the Darwinian (green dudes only)
                    if( m_id.GetTeamId() == 0 || g_app->Multiplayer() )
                    {
                        SetBlend( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                        glEnable    ( GL_BLEND );    
                        glColor4f   ( 0.0, 0.0, 0.0, alpha);                    
                        Vector3 shadowVector = length;
                        shadowVector.SetLength( 0.05 );
                        predictedPos += shadowVector;
                        glBegin(GL_QUADS);
                            glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight + entityUp).GetData() );
                            glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight + entityUp).GetData() );
                            glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight).GetData() );
                            glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight).GetData() );
                        glEnd();
                    }
                    
                    // Shadow on the ground 
                                
                    Vector3 pos1 = predictedPos - entityRight;
                    Vector3 pos2 = predictedPos + entityRight;
                    Vector3 pos4 = pos1 + length;
                    Vector3 pos3 = pos2 + length;
                    pos4 -= entityRight;
                    pos3 += entityRight;
            
                    pos1.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos1.x, pos1.z );
                    pos2.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos2.x, pos2.z );
                    pos3.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos3.x, pos3.z );
                    pos4.y = 0.3 + g_app->m_location->m_landscape.m_heightMap->GetValue( pos4.x, pos4.z );

                    //glDisable( GL_DEPTH_TEST );
                    glShadeModel( GL_SMOOTH );
                    //glDepthMask ( false );
                    glBegin( GL_QUADS );
                        glColor4f   ( 0.0, 0.0, 0.0, alpha);
                        glTexCoord2f(0.0, 0.0);       glVertex3dv(pos1.GetData());
                        glTexCoord2f(1.0, 0.0);       glVertex3dv(pos2.GetData());
                        glColor4f   ( 0.0, 0.0, 0.0, 0.1*alpha);
                        glTexCoord2f(1.0, 1.0);       glVertex3dv(pos3.GetData());
                        glTexCoord2f(0.0, 1.0);       glVertex3dv(pos4.GetData());
                    glEnd();
                    glShadeModel( GL_FLAT );
                    //glDepthMask( true );
                }
            }
        }
        
    }


    //
    // If we are side on to the camera, render a side-line

    {
        Vector3 frontH = entityUp ^ entityRight;
        Vector3 camToPosH = (g_app->m_camera->GetPos() - m_pos);
        frontH.HorizontalAndNormalise();
        camToPosH.HorizontalAndNormalise();

        double crossProduct = ( frontH ^ camToPosH ).Mag();

        if( crossProduct > 0.95 )
        {
            SetTexture( "sprites/darwinian_side.bmp" );

            double factor = 20.0 * (crossProduct-0.95);
            double alpha = 255 * factor;
            glColor4ub( colour.r, colour.g, colour.b, alpha );          
            
            Vector3 camRight = g_app->m_camera->GetRight() * 0.25;
            glBegin( GL_QUADS );
            glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - camRight + entityUp ).GetData() );
            glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + camRight + entityUp ).GetData() );
            glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + camRight ).GetData() );
            glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - camRight ).GetData() );
			glEnd();
            SetTexture( "sprites/darwinian.bmp" );
        }
    }


    //
    // Render a santa hat if it's Christmas
    
    if( (m_id.GetTeamId() == 0 || g_app->Multiplayer() ) && g_app->m_location->ChristmasModEnabled() )
    {
        if( m_id.GetUniqueId() % 3 == 0 )
        {
            SetTexture( "sprites/santahat.bmp" );            
            glColor4f( 1.0, 1.0, 1.0, 1.0 );
            predictedPos += entityUp * 0.95;
            predictedPos += m_front * 0.01;
            entityRight *= 0.65;
            entityUp *= 0.65;
            glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight + entityUp).GetData() );
                glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight + entityUp).GetData() );
                glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight).GetData() );
                glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight).GetData() );
            glEnd();
            SetTexture( "sprites/darwinian.bmp" );            
        }
    }

    // We have the mind control power, render the brainwaves
    double oldA = colour.a;
    if( m_subversionTaskId != -1 && !m_dead)
    {
        SetTexture( "sprites/brainwaves.bmp" );
        glDisable( GL_ALPHA_TEST );
        glDepthMask(false);
        
        double baseSize = 2.8;
        double waveSize = baseSize + ((g_gameTime) - (int)(g_gameTime));

        colour.a = 255 * (1.0 - (waveSize - baseSize));
        glColor4ubv( colour.GetData() );
        predictedPos -= entityUp * 0.4;
        predictedPos += m_front * 0.01;
        entityRight -= entityRight * (waveSize / 2.0);
        entityRight *= waveSize;
        entityUp *= waveSize;
		glBegin(GL_QUADS);
            glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight + entityUp).GetData() );
            glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight + entityUp).GetData() );
            glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight).GetData() );
            glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight).GetData() );
		glEnd();

        SetTexture( "sprites/darwinian.bmp" );  
        glEnable( GL_ALPHA_TEST );
        glDepthMask(true);
    }
    colour.a = oldA;

    //
    // Render our box kite if we have one
  
    if( m_boxKiteId.IsValid() )
    {
        BoxKite *boxKite = (BoxKite *) g_app->m_location->GetEffect( m_boxKiteId );
        if( boxKite )
        {
            boxKite->m_up = entityUp;
            boxKite->m_up.Normalise();
            boxKite->m_front = m_front;
            boxKite->m_front.Normalise();
            boxKite->m_pos = predictedPos + m_front * 2 + boxKite->m_up * 3;
            boxKite->m_vel = m_vel;
        }
    }

    //
    // If we are capturing a crate, render some action lines

    if( m_state == StateOpeningCrate )
    {
        Building *building = g_app->m_location->GetBuilding( m_buildingId );
        if( building )
        {
            float distance = (m_pos - building->m_pos).Mag();
            if( distance < CRATE_SCAN_RANGE * 1.1 )
            {
                entityUp.Normalise();
                Vector3 fromPos = predictedPos + entityUp * 4;
                Vector3 toPos = building->m_pos + GetCrateOffset(m_id.GetUniqueId());
                
                Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0;
                Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
                Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

                rightAngle *= 0.5;
                float timeIndex = g_gameTime + m_id.GetUniqueId();
                int alpha = fabs(sinf(timeIndex*3)) * 255;

				
				SetBlend( GL_SRC_ALPHA, GL_ONE, false );
                glColor4ub( colour.r, colour.g, colour.b, alpha );                
                glEnable    ( GL_BLEND );
                SetTexture( "textures/laser.bmp" );				
                glBegin( GL_QUADS );	
				glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
                    glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
                    glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
                    glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     
                glEnd();
                SetBlend    ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, true );
                SetTexture( "sprites/darwinian.bmp" );
            }
        }
    }


    //
    // If we are dead render us in pieces

    if( m_dead )
    {
        Vector3 entityFront = m_front;
        entityFront.Normalise();
        entityUp = g_upVector;
        entityRight = entityFront ^ entityUp;
		entityUp *= size * 2.0;
        entityRight.Normalise();
		entityRight *= size;
        unsigned char alpha = (double)m_stats[StatHealth] * 2.55;
       
        glColor4ub( 0, 0, 0, alpha );

        entityRight *= 0.5;
        entityUp *= 0.5;
        double predictedHealth = m_stats[StatHealth];
        if( m_onGround ) predictedHealth -= 40 * _predictionTime;
        else             predictedHealth -= 20 * _predictionTime;
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );

        for( int i = 0; i < 3; ++i )
        {
            Vector3 fragmentPos = predictedPos;
            if( i == 0 ) fragmentPos.x += 10.0 - predictedHealth / 10.0;
            if( i == 1 ) fragmentPos.x += 10.0 - predictedHealth / 10.0;
            if( i == 1 ) fragmentPos.z += 10.0 - predictedHealth / 10.0;
            if( i == 2 ) fragmentPos.x -= 10.0 - predictedHealth / 10.0;            
            fragmentPos.y += ( fragmentPos.y - landHeight ) * i * 0.5;
            
            
            double left = 0.0;
            double right = 1.0;
            double top = 1.0;
            double bottom = 0.0;

            if( i == 0 )
            {
                right -= (right-left)/2;
                bottom -= (bottom-top)/2;
            }
            if( i == 1 )
            {
                left += (right-left)/2;
                bottom -= (bottom-top)/2;
            }
            if( i == 2 )
            {
                top += (bottom-top)/2;
                left += (right-left)/2;
            }

            glBegin(GL_QUADS);
                glTexCoord2f(left, bottom);     glVertex3dv( (fragmentPos - entityRight + entityUp).GetData() );
                glTexCoord2f(right, bottom);    glVertex3dv( (fragmentPos + entityRight + entityUp).GetData() );
                glTexCoord2f(right, top);       glVertex3dv( (fragmentPos + entityRight).GetData() );
                glTexCoord2f(left, top);        glVertex3dv( (fragmentPos - entityRight).GetData() );
            glEnd();
        }
    }  


    ////
    //// If we are about to be eliminated, render our timer

    //if( m_id.GetTeamId() >= 0 &&
    //    m_id.GetTeamId() < NUM_TEAMS ) 
    //{
    //    int elimTimer = g_app->m_multiwinia->m_eliminationTimer[ m_id.GetTeamId() ];
    //
    //    if( elimTimer != -1 && elimTimer < 30 )
    //    {
    //        glEnd       ();

    //        g_editorFont.DrawText3DCentre( predictedPos + g_upVector * 12, 4, "%d", elimTimer );

    //        SetTexture( "sprites/darwinian.bmp" );
    //        glBegin( GL_QUADS );
    //    }
    //}
}

void Darwinian::Sacrafice( WorldObjectId _shamanId, int _targetPortalId ) 
{
    m_state = StateBeingSacraficed;

    m_wayPoint = m_pos;
    m_spiritId = -1;

    m_sacraficeTimer = syncfrand(2.5);
    m_onGround = false;

    m_shamanId = _shamanId;
    m_targetPortal = _targetPortalId;
}

void Darwinian::SetBooster( int _taskId )
{
    if( m_state == Darwinian::StateInsideArmour ) return;
    m_boosterTaskId = _taskId;
    //m_stats[StatSpeed] *= 2.0;

    int numParticles = 20;
    for( int i = 0; i < numParticles; ++i )
    {
        Vector3 pos = m_pos + g_upVector * frand(5);
        double size = 20 + frand(30.0);
        Vector3 vel = m_vel * 0.3 + g_upVector * (3+frand(3));
        vel.x += sfrand(6.0);
        vel.z += sfrand(6.0);
        int particleType = Particle::TypeDarwinianFire;
        g_app->m_particleSystem->CreateParticle( pos, vel, particleType, size, g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour );
    }
}

void Darwinian::GiveHotFeet( int _taskId )
{
    if( m_state == Darwinian::StateInsideArmour ) return;
    m_hotFeetTaskId = _taskId;
    double currentSpeedRatio = (double)m_stats[Entity::StatSpeed] / EntityBlueprint::GetStat( Entity::TypeDarwinian, Entity::StatSpeed );
    if( currentSpeedRatio < 3.5 )
    {
        m_stats[StatSpeed] *= 2.0;
    }

    int numParticles = 20;
    for( int i = 0; i < numParticles; ++i )
    {
        Vector3 pos = m_pos + g_upVector * frand(5);
        double size = 20 + frand(30.0);
        Vector3 vel = m_vel * 0.3 + g_upVector * (3+frand(3));
        vel.x += sfrand(6.0);
        vel.z += sfrand(6.0);
        int particleType = Particle::TypeDarwinianFire;
        g_app->m_particleSystem->CreateParticle( pos, vel, particleType, size, g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour );
    }
}

void Darwinian::Infect()
{
    if( m_state == Darwinian::StateInsideArmour ) return;
    if( m_infectionState == InfectionStateClean )
    {
        m_infectionState = InfectionStateInfected;
        m_infectionTimer = syncfrand(10.0);
        m_stats[StatSpeed] *= 0.75;
    }
}

void Darwinian::GiveMindControl( int _taskId)
{
    if( m_state == Darwinian::StateInsideArmour ) return;
    m_subversionTaskId = _taskId;

    int numParticles = 20;
    for( int i = 0; i < numParticles; ++i )
    {
        Vector3 pos = m_pos + g_upVector * frand(5);
        double size = 20 + frand(30.0);
        Vector3 vel = m_vel * 0.3 + g_upVector * (3+frand(3));
        vel.x += sfrand(6.0);
        vel.z += sfrand(6.0);
        int particleType = Particle::TypeDarwinianFire;
        g_app->m_particleSystem->CreateParticle( pos, vel, particleType, size, g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour );
    }
}

void Darwinian::GiveRage( int _taskId )
{
    if( m_state == Darwinian::StateInsideArmour ) return;
    m_rageTaskId = _taskId;
    double currentSpeedRatio = (double)m_stats[Entity::StatSpeed] / EntityBlueprint::GetStat( Entity::TypeDarwinian, Entity::StatSpeed );
    if( currentSpeedRatio < 3.0 )
    {
        m_stats[StatSpeed] *= 1.5;
    }

    int numParticles = 20;
    for( int i = 0; i < numParticles; ++i )
    {
        Vector3 pos = m_pos + g_upVector * frand(5);
        double size = 20 + frand(30.0);
        Vector3 vel = m_vel * 0.3 + g_upVector * (3+frand(3));
        vel.x += sfrand(6.0);
        vel.z += sfrand(6.0);
        int particleType = Particle::TypeDarwinianFire;
        g_app->m_particleSystem->CreateParticle( pos, vel, particleType, size, g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour );
    }
}


void Darwinian::FireSubversion( Vector3 const &_pos )
{
    if( m_reloading == 0.0 )
    {
		Vector3 toPos = _pos;
		toPos.x += syncsfrand(15.0);
		toPos.z += syncsfrand(15.0);
		
		Vector3 fromPos = m_pos;
		fromPos.y += 2.0;
		Vector3 velocity = (toPos - fromPos).SetLength(75.0);
		g_app->m_location->FireLaser( fromPos, velocity, m_id.GetTeamId(), true );

        m_reloading = m_stats[StatRate];
        m_justFired = true;

        g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
    }
}

bool Darwinian::HasRage()
{
    return( m_rageTaskId != -1 ||
        g_app->m_location->m_bitzkreigTimer > 0.0 );
}

char *HashDouble( double value, char *buffer );


char *Darwinian::LogState( char *_message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32], buf3[32], buf4[32],
	            buf5[32], buf6[32], buf7[32], buf8[32];		

    sprintf( s_result, "%sDARWINIAN Id[%d] state[%d] pos[%s,%s,%s] vel[%s] front[%s] waypoint[%s,%s,%s] boxId[%d]",
                        (_message)?_message:"",
                        m_id.GetUniqueId(),
                        m_state,
                        HashDouble( m_pos.x, buf1),
						HashDouble( m_pos.y, buf2),
						HashDouble( m_pos.z, buf3),
                        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf4 ),
                        HashDouble( m_front.x + m_front.y + m_front.z, buf5 ),
                        HashDouble( m_wayPoint.x, buf6),
						HashDouble( m_wayPoint.y, buf7),
						HashDouble( m_wayPoint.z, buf8),
                        m_boxKiteId.GetUniqueId() );

    return s_result;
}


void Darwinian::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );
    
    _list->PutData( "SeenThreatAttack" );
    _list->PutData( "SeenThreatAttackHorn" );
    _list->PutData( "SeenThreatRunAway" );
    _list->PutData( "TakenControl" );
    _list->PutData( "EscapedControl" );
    _list->PutData( "GivenOrders" );
    _list->PutData( "GivenOrdersFromPlayer" );
    _list->PutData( "VictoryJump" );
    _list->PutData( "OnFire" );
    _list->PutData( "LiftBuilding" );
}



// ===========================================================================

BoxKite::BoxKite()
:   WorldObject(),
    m_state(StateHeld),
    m_birthTime(0.0),
    m_deathTime(0.0)
{
    m_shape = g_app->m_resource->GetShape( "boxkite.shp" );
    m_birthTime = GetNetworkTime();

    m_size = 1.0 + syncsfrand(1.0);
    m_type = EffectBoxKite;

    m_up = g_upVector;
}


bool BoxKite::Advance()
{
    if( m_state == StateReleased )
    {
        m_vel.Zero();
        m_vel.y = 2.0 + syncfrand(2.0);

        double theTime = GetNetworkTime();

        m_vel.x = iv_sin( theTime + m_id.GetIndex() );
        m_vel.z = iv_sin( theTime + m_id.GetIndex() );            
        
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;    

        double factor1 = SERVER_ADVANCE_PERIOD * 0.1;
        double factor2 = 1.0 - factor1;
        m_up = m_up * factor2 + g_upVector * factor1;
        m_front.y = m_front.y * factor2;
        m_front.Normalise();

        if( theTime > m_deathTime )
        {
            return true;
        }
    }

    m_brightness = 0.5 + syncfrand(0.5);

    return false;
}


void BoxKite::Release()
{
    m_state = StateReleased;
    m_deathTime = GetNetworkTime() + 180.0;
}


void BoxKite::Render( double _predictionTime )
{        
    glDisable   (GL_BLEND);
    glDepthMask (true);
        
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    
    double theTime = GetNetworkTime() + _predictionTime;

    double scale = ( theTime - m_birthTime ) / 10.0;
    if( scale > m_size ) scale = m_size;

    if( m_deathTime != 0.0 && theTime > m_deathTime - 20.0 )
    {
        double deathScale = (m_deathTime - theTime) / 20.0;
        if( deathScale < 0.0 ) deathScale = 0.0;
        scale = deathScale * m_size;
    }

    g_app->m_renderer->SetObjectLighting();
    Matrix34 mat( m_front, m_up, predictedPos );
    mat.u *= scale;
    mat.r *= scale;
    mat.f *= scale;
    m_shape->Render( _predictionTime, mat );
    g_app->m_renderer->UnsetObjectLighting();


    //
    // Candle in the middle
    
    glEnable    (GL_BLEND);
    glDepthMask (false);
    
    Vector3 camUp = g_app->m_camera->GetUp() * 3.0 * scale * m_brightness;
    Vector3 camRight = g_app->m_camera->GetRight() * 3.0 * scale * m_brightness;

    glColor4f( 1.0, 0.75, 0.2, m_brightness );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (predictedPos-camRight+camUp).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (predictedPos+camRight+camUp).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (predictedPos+camRight-camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (predictedPos-camRight-camUp).GetData() );
    glEnd();

    camUp *= 0.2;
    camRight *= 0.2;
    
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (predictedPos-camRight+camUp).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (predictedPos+camRight+camUp).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (predictedPos+camRight-camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (predictedPos-camRight-camUp).GetData() );
     glEnd();
     
    glDisable       ( GL_TEXTURE_2D );
}


char *BoxKite::LogState( char *_message )
{
    static char s_result[1024];

    static char buf1[32], buf2[32];

    sprintf( s_result, "%sBOXKITE Id[%d] state[%d] pos[%s], vel[%s]",
        (_message)?_message:"",
        m_id.GetUniqueId(),
        m_state,
        HashDouble( m_pos.x + m_pos.y + m_pos.z, buf1 ),
        HashDouble( m_vel.x + m_vel.y + m_vel.z, buf2 ) );

    return s_result;
}





