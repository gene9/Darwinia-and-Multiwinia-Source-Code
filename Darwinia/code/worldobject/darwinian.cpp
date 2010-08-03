#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "camera.h"
#include "entity_grid.h"
#include "main.h"
#include "unit.h"
#include "particle_system.h"
#include "taskmanager.h"
#include "routing_system.h"
#include "renderer.h"
#include "global_world.h"
#include "level_file.h"
#include "obstruction_grid.h"

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
#include "worldobject/incubator.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/egg.h"

Darwinian::Darwinian()
:   Entity(),
    m_state(StateIdle),
    m_retargetTimer(0.0f),
    m_spiritId(-1),
    m_buildingId(-1),
    m_portId(-1),
    m_controllerId(-1),
    m_wayPointId(-1),
    m_teleportRequired(false),
    m_ordersBuildingId(-1),
    m_ordersSet(false),
    m_promoted(false),
    m_scared(true),
    m_shadowBuildingId(-1),
    m_threatRange(DARWINIAN_SEARCHRANGE_THREATS),
    m_grenadeTimer(0.0f),
    m_officerTimer(0.0f),
	m_corrupted(0),
	m_soulId(0),
	m_soulless(false),
	m_soulHarvested(false),
	m_subversive(false),
	m_oldTeamId(255),
	m_colourTimer(0)
{
    SetType( TypeDarwinian );
    m_grenadeTimer = syncfrand( 5.0f );
}


void Darwinian::Begin()
{
    Entity::Begin();
    m_onGround = true;
    m_wayPoint = m_pos;
    m_centrePos.Set(0,2,0);
    m_radius = 4.0f;

	float subversionLevel = g_app->m_globalWorld->m_research->CurrentLevel(m_id.GetTeamId(),GlobalResearch::TypeController);
	float laserLevel = g_app->m_globalWorld->m_research->CurrentLevel(m_id.GetTeamId(),GlobalResearch::TypeLaser);

	if ( frand() < subversionLevel/(subversionLevel+laserLevel) ) { m_subversive = true; }

	// DEBUG
	//if ( m_id.GetTeamId() == 0 ) { m_subversive = true; }
}


void Darwinian::ChangeHealth( int _amount )
{
    if( m_state == StateInsideArmour )
    {
        // We are invincible in here
        return;
    }

    bool dead = m_dead;

    //Entity::ChangeHealth( _amount );
    if( !m_dead )
    {
        if( _amount < 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LoseHealth" );
        }

        if( m_stats[StatHealth] + _amount <= 0 )
        {
            m_stats[StatHealth] = 100;
            m_dead = true;
            g_app->m_soundSystem->TriggerEntityEvent( this, "Die" );
			if ( !m_soulless ) { g_app->m_location->SpawnSpirit( m_pos, m_vel * 0.5f, m_id.GetTeamId(), m_id ); }
        }
        else if( m_stats[StatHealth] + _amount > 255 )
        {
            m_stats[StatHealth] = 255;
        }
        else
        {
            m_stats[StatHealth] += _amount;
        }
    }

    if( !dead && m_dead )
    {
        // We just died
    }
}


bool Darwinian::SearchForNewTask()
{
//
//            switch( m_state )                   // Deliberate fall through here - check all states above our priority
//            {
//                case StateIdle:                 SearchForRandomPosition();
//                                                SearchForSpirits();
//                case StateWorshipSpirit:        SearchForOfficers();
//                                                SearchForArmour();
//                case StateApproachingArmour:
//                case StateFollowingOrders:
//                case StateFollowingOfficer:     SearchForPorts();
//                case StateApproachingPort:
//                case StateOperatingPort:
//                case StateWatchingGodDish:
//                case StateCombat:               SearchForThreats();
//                //case StateUnderControl:
//                //case StateCapturedByAnt:
//                //case StateInsideArmour
//            }

    bool newTargetFound = false;

    switch( m_state )
    {
        case StateIdle:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            if( !newTargetFound )       newTargetFound = SearchForArmour();
            if( !newTargetFound )       newTargetFound = SearchForOfficers();
            if( !newTargetFound )       newTargetFound = SearchForSouls();
            if( !newTargetFound )       newTargetFound = SearchForSpirits();
            if( !newTargetFound )       newTargetFound = SearchForRandomPosition();
            break;

        case StateWorshipSpirit:
        case StateWatchingSpectacle:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            if( !newTargetFound )       newTargetFound = SearchForArmour();
            if( !newTargetFound )       newTargetFound = SearchForOfficers();
            break;

        case StateApproachingArmour:
        case StateFollowingOrders:
        case StateFollowingOfficer:
		case StateHarvestingSoul:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            if( !newTargetFound )       newTargetFound = SearchForPorts();
            break;

        case StateApproachingPort:
        case StateOperatingPort:
        case StateCombat:
        case StateCarryingSpirit:
        case StateToEgg:
            if( !newTargetFound )       newTargetFound = SearchForThreats();
            break;

        case StateBoardingRocket:
        case StateOnFire:
            break;
    }


    return newTargetFound;

}


bool Darwinian::Advance( Unit *_unit )
{
    if( m_promoted )
    {
        return true;
    }

	if ( m_fearless > 0 ) { m_fearless -= SERVER_ADVANCE_PERIOD; }

	if ( m_colourTimer > 0 ) { m_colourTimer -= SERVER_ADVANCE_PERIOD; }

    bool amIDead = Entity::Advance( _unit );

    if( !amIDead && !m_dead && m_onGround && m_inWater == -1.0f )
    {
        //
        // Has something higher priority come along?

        m_retargetTimer -= SERVER_ADVANCE_PERIOD;
        if( m_retargetTimer <= 0.0 )
        {
            bool newTaskFound = SearchForNewTask();

            if( m_state == StateIdle )
            {
                bool victoryDance = BeginVictoryDance();
            }

            m_retargetTimer = 1.0f + syncfrand(1.0f);
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
            case StateCarryingSpirit :      amIDead = AdvanceCarryingSpirit();      break;
            case StateToEgg :               amIDead = AdvanceToEgg();	            break;
            case StateHarvestingSoul :      amIDead = AdvanceHarvestingSoul();      break;
        }
    }

	// If we have a kite and we are no longer worshipping, release the kite
    if( m_boxKiteId.IsValid() && (m_state != StateWorshipSpirit || m_dead) )
    {
        BoxKite *boxKite = (BoxKite *) g_app->m_location->GetEffect( m_boxKiteId );
        boxKite->Release();
        m_boxKiteId.SetInvalid();
    }

	// If we have a soul and are no longer carrying it back, release the soul
	if ( g_app->m_location->m_spirits.ValidIndex( m_soulId ) ) {
		Spirit *s = g_app->m_location->m_spirits.GetPointer( m_soulId );
		if( s && s->m_state == Spirit::StateAttached && ((m_state != StateCarryingSpirit && m_state != StateToEgg) || m_dead) )
		{
			//Spirit *s = g_app->m_location->m_spirits.GetPointer( m_spiritId );
			if ( m_soulHarvested ) { s->CollectorDrops(); }
			m_soulHarvested = false;
			m_soulId = -1;
		}
	} else { m_soulId = -1; m_soulHarvested = false; }

	// Move the soul to us if carrying one
    if( g_app->m_location->m_spirits.ValidIndex(m_soulId) )
    {
        Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_soulId );
        if( spirit && spirit->m_state == Spirit::StateAttached )
        {
            spirit->m_pos = m_pos;
			spirit->m_pos.y += 10.0f;
            spirit->m_vel = m_vel;
		}
    }

    if( !m_onGround ) AdvanceInAir( _unit );

    if( m_state == StateOnFire && !amIDead && !m_dead )
    {
        amIDead = AdvanceOnFire();
    }

    if( m_pos.y < 0.0f &&
        m_inWater == -1.0f &&
        m_state != StateInsideArmour ) m_inWater = syncfrand(3.0f);

    if( m_dead && m_onGround )
    {
        m_vel *= 0.9f;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    }

    return amIDead;
}


bool Darwinian::AdvanceIdle()
{
    if( m_onGround )
    {
        AdvanceToTargetPosition();
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

    float amountToTurn = SERVER_ADVANCE_PERIOD * 4.0f;
    Vector3 targetPos = building->m_centrePos;
    targetPos += Vector3( sinf(g_gameTime) * 30.0f,
                          cosf(g_gameTime) * 20.0f,
                          sinf(g_gameTime) * 25.0f );
    Vector3 targetDir = (targetPos - m_pos).Normalise();


    //
    // Is it a god dish?

    if( building->m_type == Building::TypeGodDish )
    {
        GodDish *dish = (GodDish *) building;
        if( !dish->m_activated && dish->m_timer < 1.0f )
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

        float distance = ( building->m_pos - m_pos ).Mag();
        if( distance < 200.0f )
        {
            Vector3 moveVector = (m_pos - building->m_pos);
            moveVector.SetLength( 200 + syncfrand(100.0f) );
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


bool Darwinian::AdvanceApproachingArmour()
{
    //
    // Is our armour still alive / within range / open

    Armour *armour = (Armour *) g_app->m_location->GetEntity( m_armourId );
    if( !armour || !armour->IsLoading() )
    {
        m_state = StateIdle;
        m_armourId.SetInvalid();
        return false;
    }

    Vector3 exitPos, exitDir;
    armour->GetEntrance( exitPos, exitDir );

    float distance = ( exitPos - m_pos ).Mag();
    if( distance > DARWINIAN_SEARCHRANGE_ARMOUR )
    {
        m_state = StateIdle;
        m_armourId.SetInvalid();
        return false;
    }


    //
    // Walk towards the armour until we are there

    m_wayPoint = exitPos;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

    bool arrived = AdvanceToTargetPosition();
    if( arrived || distance < 20.0f )
    {
        armour->AddPassenger();
        m_state = StateInsideArmour;
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
        m_state = StateIdle;
        m_armourId.SetInvalid();

        Vector3 pos = m_pos;
	    float radius = syncfrand(20.0f);
	    float theta = syncfrand(M_PI * 2);
    	m_pos.x += radius * sinf(theta);
	    m_pos.z += radius * cosf(theta);
        m_vel = ( pos - m_pos );
        m_vel.SetLength( syncfrand(50.0f) );

        ChangeHealth( -500 );

        return false;
    }


    m_pos = armour->m_pos;
    m_vel = armour->m_vel;
    m_inWater = -1.0f;


    //
    // Is our armour unloading
    // Only get out if we are over ground, not water

    if( armour->IsUnloading() )
    {
        Vector3 exitPos, exitDir;
        armour->GetEntrance( exitPos, exitDir );
        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(exitPos.x, exitPos.z);
        if( landHeight > 0.0f )
        {
            // JUMP!
            armour->RemovePassenger();
            exitDir.RotateAroundY( syncsfrand(M_PI * 0.5f) );
            m_vel = exitDir * 10.0f;
            m_vel.y = 5.0f + syncfrand(10.0f);
            m_pos = exitPos;
            m_onGround = false;
            m_state = StateIdle;
            m_armourId.SetInvalid();
        }
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
    START_PROFILE( g_app->m_profiler, "AdvanceCombat" );

    //
    // Does our threat still exist?

    WorldObject *threat = g_app->m_location->GetWorldObject( m_threatId );
    bool isEntity = threat && threat->m_id.GetUnitId() != UNIT_EFFECTS;
    Entity *entity = ( isEntity ? (Entity *)threat : NULL );

    if( !threat || ( entity && entity->m_dead) )
    {
        m_state = StateIdle;
        m_retargetTimer = 0.0;
        g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );
        END_PROFILE( g_app->m_profiler, "AdvanceCombat" );
        return false;
    }


    //
    // If this is a gun turret, look to see if we are out of
    // the plausable line of fire

    if( !isEntity && threat && threat->m_type == EffectGunTurretTarget )
    {
        float distance = ( threat->m_pos - m_pos ).Mag();
        if( distance > DARWINIAN_SEARCHRANGE_TURRETS )
        {
            m_state = StateIdle;
            m_retargetTimer = 0.0;
            g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );
            END_PROFILE( g_app->m_profiler, "AdvanceCombat" );
            return false;
        }
    }

    //
    // Move away from our threat if we're an ordinary Darwinian
    // Move towards our threat if we're a Soldier Darwinian

    bool soldier = g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeDarwinian ) > 2;
	//soldier = true; // DEBUG

    if( soldier && !m_scared )
    {
//        bool arrived = AdvanceToTargetPosition();
//        if( arrived )
//        {
/*
            Vector3 targetVector = ( threat->m_pos - m_pos );
            float angle = syncsfrand( M_PI * 0.5f );
            targetVector.RotateAroundY( angle );
            float distance = targetVector.Mag();
            float ourDesiredRange = 20.0f + syncfrand(20.0f);
            targetVector.SetLength( distance - ourDesiredRange );
            m_wayPoint = m_pos + targetVector;
            m_wayPoint = PushFromObstructions( m_wayPoint );
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
*/
        float distance = ( m_pos - threat->m_pos ).Mag();
        if( distance < DARWINIAN_FEARRANGE/2.0f )
        {
            Vector3 moveAwayVector = ( m_pos - threat->m_pos ).Normalise() * 30.0f;
            float angle = syncsfrand( M_PI * 0.5f );
            moveAwayVector.RotateAroundY( angle );
            m_wayPoint = m_pos + moveAwayVector;
        }
        else
        {
            Vector3 targetVector = ( threat->m_pos - m_pos );
            float angle = syncsfrand( M_PI * 0.5f );
            targetVector.RotateAroundY( angle );
            float distance = targetVector.Mag();
            float ourDesiredRange = 20.0f + syncfrand(20.0f);
            targetVector.SetLength( distance - ourDesiredRange );
            m_wayPoint = m_pos + targetVector;
        }
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        AdvanceToTargetPosition();
    }
    else
    {
        float distance = ( m_pos - threat->m_pos ).Mag();
        if( distance > DARWINIAN_FEARRANGE )
        {
            m_scared = false;
            m_threatId.SetInvalid();
        }

        Vector3 moveAwayVector = ( m_pos - threat->m_pos ).Normalise() * 30.0f;
        float angle = syncsfrand( M_PI * 0.5f );
        moveAwayVector.RotateAroundY( angle );
        m_wayPoint = m_pos + moveAwayVector;
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        AdvanceToTargetPosition();
    }


    if( isEntity )
    {
        //
        // Shoot at our enemy

        if( soldier && (syncrand() % 10) == 0 )
        {
			if ( m_subversive )
			{
				Attack( threat->m_pos + Vector3(0,2,0), true );
			}
			else
			{
				Attack( threat->m_pos + Vector3(0,2,0) );
			}
        }


        //
        // Throw grenades if we have a good opportunity
        // ie lots of enemies near our target, and not many friends
        // Or if the enemy is the sort that responds well to grenades
        // NEVER throw grenades if there are people from team 2 nearby (ie the player's squad, officers, engineers)
        // NEVER throw grenades if the target area is too steep - Darwinians just can't fucking aim on cliffs

		bool hasGrenade = g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeDarwinian ) > 3;
		if ( m_subversive ) { hasGrenade = false; }

        //bool hasGrenade = (g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian ) > 3 && !m_subversive);

        if( hasGrenade )
        {
            START_PROFILE( g_app->m_profiler, "ThrowGrenade" );
            m_grenadeTimer -= SERVER_ADVANCE_PERIOD;
            if( m_grenadeTimer <= 0.0f )
            {
                m_grenadeTimer = 12.0f+syncfrand(8.0f);
                float distanceToTarget = ( threat->m_pos - m_pos ).Mag();
                if( distanceToTarget > 75.0f )
                {
					bool *includeTeams = new bool[NUM_TEAMS];
					for ( int i = 0; i < NUM_TEAMS; i++ ) { includeTeams[i] = false; }
					includeTeams[2] = true;
                    //bool includeTeams[] = { false, false, true, false, false, false, false, false };
                    int numPlayers = g_app->m_location->m_entityGrid->GetNumNeighbours( threat->m_pos.x, threat->m_pos.z, 50.0f, includeTeams );
                    if( numPlayers == 0 )
                    {
                        bool throwGrenade = false;
                        Vector3 ourLandNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
                        Vector3 targetLandNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(threat->m_pos.x, threat->m_pos.z);

                        if( ourLandNormal.y > 0.7f && targetLandNormal.y > 0.7f )
                        {
                            bool grenadeRequired =  entity->m_type == TypeSporeGenerator ||
                                                    entity->m_type == TypeSpider ||
                                                    entity->m_type == TypeTriffidEgg ||
                                                    entity->m_type == TypeInsertionSquadie ||
                                                    entity->m_type == TypeArmour;

                            if( grenadeRequired )
                            {
                                float targetHeight = entity->m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue( entity->m_pos.x, entity->m_pos.z );
                                if( targetHeight < 40.0f ) throwGrenade = true;
                            }
                            else
                            {
                                int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( threat->m_pos.x, threat->m_pos.z, 50.0f, m_id.GetTeamId() );
                                int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( threat->m_pos.x, threat->m_pos.z, 50.0f, m_id.GetTeamId() );
                                if( numEnemies > 5 && numFriends < 2 ) throwGrenade = true;
                            }
                        }

                        if( throwGrenade )
                        {
                            g_app->m_location->ThrowWeapon( m_pos, threat->m_pos, EffectThrowableGrenade, m_id.GetTeamId() );
                        }
                    }
                }
            }
            END_PROFILE( g_app->m_profiler, "ThrowGrenade" );
        }
    }

    END_PROFILE( g_app->m_profiler, "AdvanceCombat" );
    return false;
}


bool Darwinian::AdvanceWorshipSpirit()
{
    START_PROFILE( g_app->m_profiler, "AdvanceWorship" );

    //
    // Check our spirit is still there and valid

    if( !g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
    {
        m_state = StateIdle;
		m_spiritId = -1;
        m_retargetTimer = 3.0f;
        END_PROFILE( g_app->m_profiler, "AdvanceWorship" );
        return false;
    }

    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    if( spirit->m_state != Spirit::StateBirth &&
        spirit->m_state != Spirit::StateFloating )
    {
        m_state = StateIdle;
		m_spiritId = -1;
        m_retargetTimer = 3.0f;

        if( m_boxKiteId.IsValid() )
        {
            BoxKite *boxKite = (BoxKite *) g_app->m_location->GetEffect( m_boxKiteId );
            boxKite->Release();
            m_boxKiteId.SetInvalid();
        }

        END_PROFILE( g_app->m_profiler, "AdvanceWorship" );
        return false;
    }


    //
    // Move to within range of our spirit

    Vector3 targetVector = ( spirit->m_pos - m_pos );
    float distance = targetVector.Mag();
    float ourDesiredRange = 20 + (m_id.GetUniqueId() % 20);
    targetVector.SetLength( distance - ourDesiredRange );
    Vector3 newWaypoint = m_pos + targetVector;
    newWaypoint = PushFromObstructions( newWaypoint );
    newWaypoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newWaypoint.x, newWaypoint.z );
    if( (newWaypoint - m_wayPoint).Mag() > 10.0f )
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
                    float distanceSqd = ( obj->m_pos - m_pos ).MagSquared();
                    if( distanceSqd < 2500.0f )
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
        }
    }


    END_PROFILE( g_app->m_profiler, "AdvanceWorship" );
    return false;
}

bool Darwinian::AdvanceHarvestingSoul()
{
	bool captureSpirit = false;
	if ( (SearchForIncubator() || (SearchForEggs() && g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_EGGWINIANS) ) &&
		(g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_SOULHARVEST) ) { captureSpirit = true; }

	if ( !captureSpirit )
	{
		m_state = StateIdle;
		m_soulId = -1;
		return false;
	}
    START_PROFILE( g_app->m_profiler, "AdvanceHarvest" );

    //
    // Check our spirit is still there and valid

    if( !g_app->m_location->m_spirits.ValidIndex(m_soulId) )
    {
        m_state = StateIdle;
		m_soulId = -1;
        m_retargetTimer = 3.0f;
        END_PROFILE( g_app->m_profiler, "AdvanceHarvest" );
        return false;
    }

    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_soulId);
    if( spirit->m_state != Spirit::StateBirth &&
        spirit->m_state != Spirit::StateFloating )
    {
        m_state = StateIdle;
		m_soulId = -1;
        m_retargetTimer = 3.0f;

        END_PROFILE( g_app->m_profiler, "AdvanceHarvest" );
        return false;
    }


    //
    // Move to within range of our spirit

    Vector3 targetVector = ( spirit->m_pos - m_pos );
    float distance = targetVector.Mag();
    targetVector.SetLength( distance );
    Vector3 newWaypoint = m_pos + targetVector;
    newWaypoint = PushFromObstructions( newWaypoint );
    newWaypoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newWaypoint.x, newWaypoint.z );
    if( (newWaypoint - m_wayPoint).Mag() > 10.0f )
    {
        m_wayPoint = newWaypoint;
    }

    bool areWeThere = AdvanceToTargetPosition();
    if( areWeThere )
    {
        m_front = ( spirit->m_pos - m_pos ).Normalise();
    }

	Vector3 vectorRemaining = m_wayPoint - m_pos;
	vectorRemaining.y = 0;
	distance = vectorRemaining.Mag();
	if ( distance < 10.0f && (spirit->m_state == Spirit::StateBirth || spirit->m_state == Spirit::StateFloating) )
	{
		spirit->CollectorArrives();
		//m_spiritId = spirit->m_id.GetUniqueId();
		if ( SearchForIncubator() ) { m_state = StateCarryingSpirit; }
		else if ( SearchForEggs() ) { m_state = StateToEgg; }
		m_soulHarvested = true;
		END_PROFILE( g_app->m_profiler, "AdvanceHarvest" );
		return false;
	}

    END_PROFILE( g_app->m_profiler, "AdvanceHarvest" );
    return false;
}

bool Darwinian::AdvanceApproachingPort()
{
    //
    // Check the port is still available

    Building *building = g_app->m_location->GetBuilding( m_buildingId );
    if( !building || building->GetNumPortsOccupied() >= building->GetNumPorts() )
    {
		m_wayPoint = m_pos;
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

    return false;
}


bool Darwinian::AdvanceUnderControl()
{
    //
    // Try to lookup our controller

    Task *task = g_app->m_taskManager->GetTask( m_controllerId );
    Unit *controller = NULL;
    if( task ) controller = g_app->m_location->GetUnit( task->m_objId );

    if( !task || !controller )
    {
        m_state = StateIdle;
        int numFlashes = 5 + darwiniaRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(5.0f), frand(15.0f), sfrand(5.0f) );
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash );
        }
        g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian TakenControl" );
        g_app->m_soundSystem->TriggerEntityEvent( this, "EscapedControl" );
        return false;
    }


    //
    // Follow the route owned by our controller

    bool arrived = AdvanceToTargetPosition();

    if( m_teleportRequired )
    {
        bool teleported = ( EnterTeleports() != -1 );
        if( teleported )
        {
            m_teleportRequired = false;
            arrived = true;
        }
    }

    if( arrived )
    {
        float positionError = 0.0f;

        if( m_teleportRequired )
        {
            //
            // We are trying to wriggle our way into a teleport that is very nearby
            // Just wander a bit until we are picked up
            WayPoint *wp = task->m_route->GetWayPoint( m_wayPointId );
            positionError = 10.0f;
            m_wayPoint = wp->GetPos();
        }
        else
        {
            WayPoint *wp = task->m_route->GetWayPoint( m_wayPointId+1 );
            if( wp )
            {
                //
                // Our next waypoint is available
                // So head there immediately
                ++m_wayPointId;
                m_wayPoint = wp->GetPos();
                positionError = 40.0f;
                if( wp->m_type == WayPoint::TypeBuilding ) m_teleportRequired = true;
            }
            else
            {
                //
                // There are no more waypoints
                // So just head directly for the squad that is controlling us
                m_wayPoint = controller->m_centrePos;
                positionError = 70.0f;
            }
        }

        //
        // Add some randomness to our waypoint

	    float radius = syncfrand(positionError);
	    float theta = syncfrand(M_PI * 2);
    	m_wayPoint.x += radius * sinf(theta);
	    m_wayPoint.z += radius * cosf(theta);

        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

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
        	        float radius = syncfrand(10.0f);
                    float theta = syncfrand(M_PI * 2);
    	            m_wayPoint.x += radius * sinf(theta);
	                m_wayPoint.z += radius * cosf(theta);
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
            m_retargetTimer = 0.0f;
            m_ordersBuildingId = -1;
            m_ordersSet = false;
        }
    }

    return false;
}


bool Darwinian::AdvanceFollowingOfficer()
{
    //
    // Look up our officer

    Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
    if( !officer || officer->m_dead || officer->m_orders != Officer::OrderFollow )
    {
        m_officerId.SetInvalid();
        m_state = StateIdle;
        return false;
    }


    //
    // Every few seconds, see if our officer is still around
    // Retarget him in case he moved a lot

    m_officerTimer -= SERVER_ADVANCE_PERIOD;
    if( m_officerTimer <= 0.0f )
    {
        m_officerTimer = 5.0f;
        bool walkable = g_app->m_location->IsWalkable( m_pos, officer->m_pos );
        if( !walkable )
        {
            //
            // If our officer stepped into a teleport, try to follow
            if( officer->m_ordersBuildingId != -1 )
            {
                m_ordersBuildingId = officer->m_ordersBuildingId;
                Building *building = g_app->m_location->GetBuilding( m_ordersBuildingId );
                DarwiniaDebugAssert( building );
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
            float positionError = 40.0f;
            float radius = syncfrand(positionError);
	        float theta = syncfrand(M_PI * 2);
            m_wayPoint.x += radius * sinf(theta);
	        m_wayPoint.z += radius * cosf(theta);
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        }
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

    if( arrived )
    {
        float positionError = 0.0f;
        if( m_ordersBuildingId != -1 )
        {
            // We have arrived but are trying to enter a teleport
            // Just wiggle around until we are given entry
            Building *building = g_app->m_location->GetBuilding( m_ordersBuildingId );
            DarwiniaDebugAssert(building);
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
        	    positionError = 10.0f;
            }
        }
        else
        {
            m_wayPoint = officer->m_pos;
            positionError = 40.0f;
        }

        float radius = syncfrand(positionError);
	    float theta = syncfrand(M_PI * 2);
        m_wayPoint.x += radius * sinf(theta);
	    m_wayPoint.z += radius * cosf(theta);
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

    return false;
}

bool Darwinian::AdvanceCarryingSpirit()
{
    Building *building = (Building *) g_app->m_location->GetBuilding( m_buildingId );

	// Check the building is still friendly now too
    if( !building || !g_app->m_location->IsFriend(m_id.GetTeamId(), building->m_id.GetTeamId()) )
    {
        bool found = SearchForIncubator();
        building = (Building *) g_app->m_location->GetBuilding( m_buildingId );
        if( !building )
        {
            // We can't find a friendly incubator, so go into holding pattern
            m_state = StateIdle;
            return false;
        }
    }

	if( building && building->m_type == Building::TypeIncubator )
    {
        Incubator *incubator = (Incubator *) building;
		Vector3 tempVector3;
		incubator->GetExitPoint( m_wayPoint, tempVector3 );

		bool arrived = AdvanceToTargetPosition();
		if( arrived )
		{
			// Arrived at our incubator, drop spirit off here one at a time
			if( g_app->m_location->m_spirits.ValidIndex(m_soulId) )
			{
				Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_soulId );
				incubator->AddSpirit( spirit );
				g_app->m_location->m_spirits.MarkNotUsed( m_soulId );
				m_soulId = -1;
				m_soulHarvested = false;
			}
			m_state = StateIdle;
		}
	}

	if( building && (building->m_type == Building::TypeSpawnPoint || building->m_type == Building::TypeSpawnPointRandom) )
    {
        SpawnPoint *spawnPoint = (SpawnPoint *)building;
        m_wayPoint = building->m_pos + building->m_front * 50;
        m_wayPoint = PushFromObstructions( m_wayPoint, false );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        
        bool arrived = AdvanceToTargetPosition();
        if( arrived )
        {
			// Arrived at our incubator, drop spirit off here one at a time
			if( g_app->m_location->m_spirits.ValidIndex(m_soulId) )
			{
				Spirit *spirit = g_app->m_location->m_spirits.GetPointer( m_soulId );
				spawnPoint->SpawnDarwinian();
				g_app->m_location->m_spirits.MarkNotUsed( m_soulId );
				m_soulId = -1;
                g_app->m_soundSystem->TriggerEntityEvent( this, "DropSpirit" );
				m_soulHarvested = false;
			}
			m_state = StateIdle;
        }    
    }

    return false;
}



bool Darwinian::SearchForIncubator()
{
    //
    // Source building lost, look for another incubator to bind to
    float nearest = DARWINIAN_INCUBATOR_SEARCH_RADIUS;
    bool found = false;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeIncubator &&
                g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                float distance = ( building->m_pos - m_pos ).Mag();
                int population = ((Incubator *)building)->NumSpiritsInside();
                distance += population * 10;

                if( distance < nearest && g_app->m_location->IsWalkable(m_pos, building->m_pos) )
                {
                    m_buildingId = building->m_id.GetUniqueId();
                    nearest = distance;
                    found = true;
                }
            }

            if( (building->m_type == Building::TypeSpawnPoint || building->m_type == Building::TypeSpawnPointRandom) &&
				g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_SPAWNPOINTINCUBATION &&
                g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                double distance = ( building->m_pos - m_pos ).Mag();
                if( distance < nearest && g_app->m_location->IsWalkable(m_pos, building->m_pos) )
                {
                    m_buildingId = building->m_id.GetUniqueId();                    
                    nearest = distance;
                    found = true;
                }                    
            }
        }
	}

    return found;
}
WorldObjectId Darwinian::FindNearbyEgg( Vector3 const &_pos )
{
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( _pos.x, _pos.z, DARWINIAN_INCUBATOR_SEARCH_RADIUS, &numFound, m_id.GetTeamId() );

    //
    // Build a list of candidates within range

    LList<WorldObjectId> eggIds;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Egg *egg = (Egg *) g_app->m_location->GetEntitySafe( id, Entity::TypeEgg );

        if( egg &&
            egg->m_state == Egg::StateDormant &&
            egg->m_onGround )
        {
            eggIds.PutData( id );
        }
    }


    //
    // Chose one randomly

    if( eggIds.Size() > 0 )
    {
        int chosenIndex = syncfrand( eggIds.Size() );
        WorldObjectId *eggId = eggIds.GetPointer(chosenIndex);
        return *eggId;
    }
    else
    {
        return WorldObjectId();
    }
}


bool Darwinian::SearchForEggs()
{
	START_PROFILE(g_app->m_profiler, "SearchForEggs");

    WorldObjectId eggId = FindNearbyEgg( m_pos );

    if( eggId.IsValid() )
    {
        m_eggId = eggId;
		Egg *theEgg = (Egg *) g_app->m_location->GetEntitySafe( m_eggId, Entity::TypeEgg );
		m_wayPoint = theEgg->m_pos;
        //m_state = StateToEgg;
		END_PROFILE(g_app->m_profiler, "SearchForEggs");
        return true;
    }

	END_PROFILE(g_app->m_profiler, "SearchFprEggs");
    return false;
}

bool Darwinian::AdvanceToEgg()
{
	START_PROFILE(g_app->m_profiler, "AdvanceToEgg");
    Egg *theEgg = (Egg *) g_app->m_location->GetEntitySafe( m_eggId, Entity::TypeEgg );

    if( !theEgg || theEgg->m_state == Egg::StateFertilised )
    {
        bool found = SearchForEggs();
        if( !found )
        {
            m_state = StateIdle;
			m_wayPoint = m_pos;
            END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
            return false;
        }
    }

    if( !g_app->m_location->m_spirits.ValidIndex( m_soulId ) )
    {
        m_soulId = -1;
        m_state = StateIdle;
        END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
        return false;
    }

    //
    // At this point we MUST have found an egg, otherwise we'd have returned by now

    theEgg = (Egg *) g_app->m_location->GetEntitySafe( m_eggId, Entity::TypeEgg );
    DarwiniaDebugAssert( theEgg );

	m_wayPoint = theEgg->m_pos;
    bool arrived = AdvanceToTargetPosition();

    if( arrived )
    {
        theEgg->Fertilise( m_soulId );
        m_soulId = -1;
        m_eggId.SetInvalid();
		m_state = StateIdle;
		m_soulHarvested = false;
    }

	END_PROFILE(g_app->m_profiler, "AdvanceToEgg");
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
    if( !building || building->m_type != Building::TypeEscapeRocket )
    {
        m_state = StateIdle;
        return false;
    }


    //
    // Run towards our building

    float distance = ( building->m_pos - m_pos ).Mag();
    Vector3 moveVector = (m_pos - building->m_pos);
    moveVector.SetLength( 100 + syncfrand(50.0f) );
    m_wayPoint = building->m_pos + moveVector;
    bool arrived = AdvanceToTargetPosition();


    //
    // Shoot at the building if we are in range

    if( distance < 200.0f )
    {
        Vector3 targetPos = building->m_pos;
        targetPos.y += 50.0f;
        g_app->m_location->ThrowWeapon( m_pos, targetPos, WorldObject::EffectThrowableGrenade, 1 );
        m_state = StateIdle;
    }

    return false;
}


bool Darwinian::SearchForRandomPosition()
{
    START_PROFILE( g_app->m_profiler, "SearchRandomPos" );

    //
    // Search for a new random position
    // Sometimes just don't bother, so we stand still,
    // pondering the meaning of the world

    if( syncfrand() < 0.7f )
    {
        float distance = 20.0f;
        float angle = syncsfrand(2.0f * M_PI);

        m_wayPoint = m_pos + Vector3( sinf(angle) * distance,
                                       0.0f,
                                       cosf(angle) * distance );

        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }

    END_PROFILE( g_app->m_profiler, "SearchRandomPos" );

    return true;
}


bool Darwinian::SearchForArmour()
{
    //
    // Red Darwinians don't respond to armour

    if( m_id.GetTeamId() == 1 ) return false;


    START_PROFILE( g_app->m_profiler, "SearchArmour" );

    //
    // Build a list of nearby armour

    LList<WorldObjectId> m_armour;

    Team *team = g_app->m_location->GetMyTeam();

    if( team )
    {
        for( int i = 0; i < team->m_specials.Size(); ++i )
        {
            WorldObjectId id = *team->m_specials.GetPointer(i);
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && !entity->m_dead && entity->m_type == Entity::TypeArmour )
            {
                Armour *armour = (Armour *) entity;
                float range = ( armour->m_pos - m_pos ).Mag();
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
        int chosenIndex = rand() % m_armour.Size();
        m_armourId = *m_armour.GetPointer(chosenIndex);
        m_state = StateApproachingArmour;
    }

    END_PROFILE( g_app->m_profiler, "SearchArmour" );
    return ( m_armour.Size() > 0 );
}


bool Darwinian::SearchForOfficers()
{
    //
    // Red Darwinians don't respond to officers

    //if( m_id.GetTeamId() == 1 ) return false;


    START_PROFILE( g_app->m_profiler, "SearchOfficers" );

    //
    // Do we already have some orders that have yet to be completed?

    if( m_ordersSet )
    {
        float positionError = 20.0f;
        float radius = syncfrand(positionError);
	    float theta = syncfrand(M_PI * 2);
        m_wayPoint = m_orders;
    	m_wayPoint.x += radius * sinf(theta);
	    m_wayPoint.z += radius * cosf(theta);
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

        m_state = StateFollowingOrders;
        END_PROFILE( g_app->m_profiler, "SearchOfficers" );
        return true;
    }


    //
    // Build a list of nearby officers with GOTO orders set
    // Also find the nearest officer with FOLLOW orders set

	LList<WorldObjectId> officers;
	float nearest = 99999.9f;
	WorldObjectId nearestId;

	for ( int t = 0; t < NUM_TEAMS; t++ )
	{
		Team *team = &g_app->m_location->m_teams[t];

		if( team && g_app->m_location->IsFriend(t,m_id.GetTeamId()) )
		{

			for( int i = 0; i < team->m_specials.Size(); ++i )
			{
				WorldObjectId id = *team->m_specials.GetPointer(i);
				Entity *entity = g_app->m_location->GetEntity( id );
				if( entity &&
				   !entity->m_dead &&
					entity->m_type == Entity::TypeOfficer )
				{
					Officer *officer = (Officer *) entity;
					float distance = ( officer->m_pos - m_pos ).Mag();
					if( distance < DARWINIAN_SEARCHRANGE_OFFICERS &&
						officer->m_orders == Officer::OrderGoto )
					{
						officers.PutData( id );
					}
					else if( officer->m_orders == Officer::OrderFollow &&
							 distance > 50.0f && distance < nearest )
					{
						nearest = distance;
						nearestId = id;
					}
				}
			}
		}

        //
        // Select a GOTO officer randomly

        if( officers.Size() > 0 )
        {
            int chosenOfficer = syncrand() % officers.Size();
            WorldObjectId officerId = *officers.GetPointer(chosenOfficer);
            Officer *officer = (Officer *) g_app->m_location->GetEntitySafe(officerId, TypeOfficer);
            DarwiniaDebugAssert( officer );

            if( g_app->m_location->IsWalkable( m_pos, officer->m_orderPosition ) )
            {
                m_orders = officer->m_orderPosition;
                m_ordersBuildingId = officer->m_ordersBuildingId;
                m_ordersSet = true;

                float positionError = 20.0f;
                float radius = syncfrand(positionError);
	            float theta = syncfrand(M_PI * 2);
                m_wayPoint = m_orders;
    	        m_wayPoint.x += radius * sinf(theta);
	            m_wayPoint.z += radius * cosf(theta);

                m_wayPoint = PushFromObstructions( m_wayPoint, false );
                m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

                g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrders" );

                m_state = StateFollowingOrders;
                END_PROFILE( g_app->m_profiler, "SearchOfficers" );
                return true;
            }
        }


        //
        // If there aren't any officers nearby, look for officers
        // with the FOLLOW order set and head for them

        if( officers.Size() == 0 && nearestId.IsValid() )
        {
            m_officerId = nearestId;
            Officer *officer = (Officer *) g_app->m_location->GetEntitySafe( m_officerId, TypeOfficer );
            if( g_app->m_location->IsWalkable( m_pos, officer->m_pos, true ) )
            {
                m_wayPoint = officer->m_pos;

                float positionError = 40.0f;
                float radius = syncfrand(positionError);
	            float theta = syncfrand(M_PI * 2);
                m_wayPoint.x += radius * sinf(theta);
	            m_wayPoint.z += radius * cosf(theta);
                m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

                m_state = StateFollowingOfficer;
                m_officerTimer = 5.0f;
                END_PROFILE( g_app->m_profiler, "SearchOfficers" );
                return true;
            }
        }
    }

    END_PROFILE( g_app->m_profiler, "SearchOfficers" );
    return false;
}


void Darwinian::GiveOrders( Vector3 const &_targetPos )
{
    m_orders = _targetPos;
    m_ordersBuildingId = -1;
    m_ordersSet = true;

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
            float distance = ( building->m_pos - m_orders ).Mag();
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
        float positionError = 20.0f;
        float radius = syncfrand(positionError);
	    float theta = syncfrand(M_PI * 2);
        m_wayPoint.x += radius * sinf(theta);
	    m_wayPoint.z += radius * cosf(theta);
    }

    m_wayPoint = PushFromObstructions( m_wayPoint );
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    m_wayPoint = PushFromObstructions( m_wayPoint );

    g_app->m_soundSystem->TriggerEntityEvent( this, "GivenOrders" );

    m_state = StateFollowingOrders;
}


bool Darwinian::SearchForSpirits()
{
    // Red darwinians don't worship spirits
    //if( m_id.GetTeamId() == 1 ) return false;

    START_PROFILE( g_app->m_profiler, "SearchSpirits" );

    Spirit *found = NULL;
    int spiritId = -1;

    float closest = DARWINIAN_SEARCHRANGE_SPIRITS;

    if( syncrand() % 5 == 0 )
    {
        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            if( g_app->m_location->m_spirits.ValidIndex(i) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                float theDist = ( s->m_pos - m_pos ).Mag();

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

    END_PROFILE( g_app->m_profiler, "SearchSpirits" );
    return found;
}

bool Darwinian::SearchForSouls()
{
	if ( (g_app->m_location->m_levelFile->m_teamFlags[m_id.GetTeamId()] & TEAM_FLAG_SOULHARVEST) == 0 ) { return false; }

    // Red darwinians don't worship spirits
    //if( m_id.GetTeamId() == 1 ) return false;

    START_PROFILE( g_app->m_profiler, "SearchSouls" );

    Spirit *found = NULL;
    int spiritId = -1;

    float closest = DARWINIAN_SEARCHRANGE_SPIRITS;

    if( syncrand() % 5 == 0 )
    {
        for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
        {
            if( g_app->m_location->m_spirits.ValidIndex(i) )
            {
                Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
                float theDist = ( s->m_pos - m_pos ).Mag();

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
        m_soulId = spiritId;
        m_state = StateHarvestingSoul;
    }

    END_PROFILE( g_app->m_profiler, "SearchSouls" );
    return found;
}

bool Darwinian::SearchForThreats()
{
    START_PROFILE( g_app->m_profiler, "SearchThreats" );

    //
    // Allow our threat range to creep back up to the max

    float threatRangeChange = SERVER_ADVANCE_PERIOD;
    m_threatRange = ( DARWINIAN_SEARCHRANGE_THREATS * threatRangeChange ) +
                    ( m_threatRange * (1.0f - threatRangeChange) );


    //
    // If we are running towards a Battle Cannon, this takes
    // priority over everything else

    if( m_state == StateApproachingPort || m_state == StateOperatingPort )
    {
        Building *building = g_app->m_location->GetBuilding( m_buildingId );
        if( building && building->m_type == Building::TypeGunTurret )
        {
            END_PROFILE( g_app->m_profiler, "SearchThreats" );
            return false;
        }
    }

    //
    // Search for grenades, airstrikes nearby

    int numEnemies = 0;
    float nearestThreatSqd = FLT_MAX;
    WorldObjectId threatId;
    bool throwableWeaponFound = false;

    float maxGrenadeRangeSqd = pow( DARWINIAN_SEARCHRANGE_GRENADES, 2 );

    for( int i = 0; i < g_app->m_location->m_effects.Size(); ++i )
    {
        if( g_app->m_location->m_effects.ValidIndex(i) )
        {
            WorldObject *wobj = g_app->m_location->m_effects[i];
            if( wobj->m_type == EffectThrowableGrenade ||
                wobj->m_type == EffectThrowableAirstrikeMarker ||
                wobj->m_type == EffectGunTurretTarget ||
                (wobj->m_type == EffectSpamInfection && m_id.GetTeamId() == 0) )
            {
                float distanceSqd = ( wobj->m_pos - m_pos ).MagSquared();

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

    //
    // If we found a grenade, run away immediately

    if( throwableWeaponFound )
    {
        m_state = StateCombat;
        m_scared = true;
        if( m_threatId != threatId )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatRunAway" );
            m_threatId = threatId;
        }
        END_PROFILE( g_app->m_profiler, "SearchThreats" );
        return true;
    }


    //
    // No explosives nearby.  Look for bad guys
    // Start with a quick evaluation of the area, by querying any AITarget buildings

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building && building->m_type == Building::TypeAITarget )
            {
                float range = ( building->m_pos - m_pos ).MagSquared();
                if( range < 40000.0f )              // 200m
                {
                    AITarget *target = (AITarget *) building;
                    int numEnemiesNearby = target->m_enemyCount[ m_id.GetTeamId() ];
                    if( numEnemiesNearby == 0 )
                    {
                        END_PROFILE( g_app->m_profiler, "SearchThreats" );
                        return false;
                    }
                }
            }
        }
    }


    // Count the number of nearby friends and enemies
    // Also find the nearest enemy

    int numFound = 0;
    float searchRange = m_threatRange;
    if( m_state == StateOperatingPort )
    {
        searchRange *= 0.5f;
        Building *building = g_app->m_location->GetBuilding(m_buildingId);
        if( building && building->m_type == Building::TypeGunTurret )
        {
            searchRange = 0.0f;
        }
    }

    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, searchRange, &numFound, m_id.GetTeamId() );
    bool friendsPresent = g_app->m_location->m_entityGrid->AreFriendsPresent( m_pos.x, m_pos.z, searchRange, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        bool onFire = entity->m_type == TypeDarwinian && ((Darwinian *)entity)->IsOnFire();

        if( !entity->m_dead && !onFire && entity->m_type != TypeEgg )
        {
            ++numEnemies;

            float distanceSqd = ( entity->m_pos - m_pos ).MagSquared();
            if( distanceSqd < nearestThreatSqd )
            {
                nearestThreatSqd = distanceSqd;
                threatId = id;
            }
        }
    }


    //
    // Decide what to do with our threat

	if ( m_fearless > 0 ) { m_scared = false; }

    Entity *entity = g_app->m_location->GetEntity( threatId );

    if( entity && !entity->m_dead )
    {
        m_state = StateCombat;
        bool soldier = g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeDarwinian ) > 2;

		if( !soldier && m_fearless <= 0.0f ) m_scared = true;
        if( soldier )
        {
            m_scared = numEnemies > 5 && !friendsPresent && m_fearless <= 0.0f;
        }

        if( entity->m_type == TypeSporeGenerator ||
            entity->m_type == TypeTripod ||
            entity->m_type == TypeSpider ||
            entity->m_type == TypeSoulDestroyer ||
            entity->m_type == TypeTriffidEgg ||
            entity->m_type == TypeInsertionSquadie ||
            entity->m_type == TypeArmour )
        {
			if ( m_fearless <= 0.0f ) { m_scared = true; }
        }

        if( m_threatId != threatId )
        {
            if( m_scared )
            {
                g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatRunAway" );
            }
            else
            {
                g_app->m_soundSystem->TriggerEntityEvent( this, "SeenThreatAttack" );
            }
            m_threatId = threatId;
        }

        if( m_ordersSet && (m_pos - m_orders).Mag() < 50.0f )
        {
            // We got near enough
            m_ordersSet = false;
        }

        m_threatRange = sqrtf( nearestThreatSqd );
        END_PROFILE( g_app->m_profiler, "SearchThreats" );
        return true;
    }
    else
    {
        // There are no nearby threats
        m_threatId.SetInvalid();
        g_app->m_soundSystem->StopAllSounds( m_id, "Darwinian SeenThreat" );

        END_PROFILE( g_app->m_profiler, "SearchThreats" );
        return false;
    }
}


bool Darwinian::SearchForPorts()
{
    START_PROFILE( g_app->m_profiler, "SearchPorts" );

    //
    // Build a list of available buildings

    LList<int> availableBuildings;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            float distanceToBuilding = ( building->m_pos - m_pos ).Mag();
            distanceToBuilding -= building->m_radius;
            if( distanceToBuilding < DARWINIAN_SEARCHRANGE_PORTS )
            {
                if( building->GetNumPortsOccupied() < building->GetNumPorts() )
                {
                    availableBuildings.PutData( building->m_id.GetUniqueId() );
                }
            }
        }
    }

    if( availableBuildings.Size() == 0 )
    {
        END_PROFILE( g_app->m_profiler, "SearchPorts" );
        return false;
    }


    //
    // Select a random building

    int chosenBuildingIndex = syncrand() % availableBuildings.Size();
    Building *chosenBuilding = g_app->m_location->GetBuilding( availableBuildings[chosenBuildingIndex] );
    DarwiniaDebugAssert(chosenBuilding);


    //
    // Build a list of available ports;

    LList<int> availablePorts;
    for( int p = 0; p < chosenBuilding->GetNumPorts(); ++p )
    {
        if( !chosenBuilding->GetPortOccupant(p).IsValid() &&
            chosenBuilding->GetPortOperatorCount(p,m_id.GetTeamId()) < 20 )
        {
            availablePorts.PutData(p);
        }
    }


    //
    // Select a random port

    if( availablePorts.Size() == 0 )
    {
        END_PROFILE( g_app->m_profiler, "SearchPorts" );
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

    END_PROFILE( g_app->m_profiler, "SearchPorts" );
    return true;
}


bool Darwinian::BeginVictoryDance()
{
    if( m_onGround && m_id.GetTeamId() == 0 && syncfrand(5.0f) < 1.0f && m_pos.y > 10.0f )
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
            m_vel.y += 15.0f + syncfrand(15.0f);
            m_onGround = false;
            g_app->m_soundSystem->TriggerEntityEvent( this, "VictoryJump" );
            return true;
        }
    }

    return false;
}


bool Darwinian::AdvanceToTargetPosition()
{
    START_PROFILE( g_app->m_profiler, "AdvanceToTargetPos" );

    Vector3 oldPos = m_pos;

    //
    // Are we there yet?

    Vector3 vectorRemaining = m_wayPoint - m_pos;
    vectorRemaining.y = 0;
    float distance = vectorRemaining.Mag();
    if( distance == 0.0f )
    {
        m_vel.Zero();
        END_PROFILE( g_app->m_profiler, "AdvanceToTargetPos" );
        return true;
    }
    else if (distance < 1.0f)
    {
        m_pos = m_wayPoint;
        m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
        END_PROFILE( g_app->m_profiler, "AdvanceToTargetPos" );
        return false;
    }


    //
    // Work out where we want to be next

    float speed = m_stats[StatSpeed];
    if( m_state == StateIdle ||
        m_state == StateWorshipSpirit ) speed *= 0.2f;

    float amountToTurn = SERVER_ADVANCE_PERIOD * 4.0f;
    Vector3 targetDir = (m_wayPoint- m_pos).Normalise();
    Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();

    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;


    //
    // Slow us down if we're going up hill
    // Speed up if going down hill

    float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
    float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    float factor = 1.0f - (currentHeight - nextHeight) / -3.0f;
    if( factor < 0.3f ) factor = 0.3f;
    if( factor > 2.0f ) factor = 2.0f;
    speed *= factor;


    //
    // Slow us down if we're near our objective

    if( distance < 10.0f )
    {
        float distanceFactor = distance / 10.0f;
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

    END_PROFILE( g_app->m_profiler, "AdvanceToTargetPos" );
    return false;
}


Vector3 Darwinian::PushFromObstructions( Vector3 const &pos, bool killem )
{
    Vector3 result = pos;
    if( m_onGround )
    {
        result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
    }

    Matrix34 transform( m_front, g_upVector, result );

    //
    // Push from Water


    if( result.y <= 1.0f )
    {
        START_PROFILE( g_app->m_profiler, "PushFromWater" );

        float pushAngle = syncsfrand(1.0f);
        float distance = 40.0f;
        while( distance < 100.0f )
        {
            float angle = distance * pushAngle * M_PI;
            Vector3 offset( cosf(angle) * distance, 0.0f, sinf(angle) * distance );
            Vector3 newPos = result + offset;
            float height = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
            if( height > 1.0f )
            {
                result = newPos;
                result.y = height;
                m_avoidObstruction = result;
                break;
            }
            distance += 5.0f;
        }

        END_PROFILE( g_app->m_profiler, "PushFromWater" );
    }


    //
    // Push from buildings

    START_PROFILE( g_app->m_profiler, "PushFromBuildings" );

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( result.x, result.z );

    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building )
        {
            if( building->m_type == Building::TypeLaserFence &&
				((LaserFence *) building)->IsEnabled() )
            {
                float closest = 5.0f + m_id.GetUniqueId() % 10;
                if( building->DoesSphereHit( m_pos, 1.0f ) && killem )
                {
                    //ChangeHealth( -999 );
					SetFire();
                    ((LaserFence *) building)->Electrocute( m_pos );
                }
                else if( building->DoesSphereHit( result, closest ) )
                {
                    LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( ((LaserFence *)building)->GetBuildingLink() );
                    Vector3 pushForce = (building->m_centrePos - result).SetLength(1.0f);
                    if( nextFence )
                    {
                        Vector3 fenceVector = nextFence->m_pos - building->m_pos;
                        Vector3 rightAngle = fenceVector ^ g_upVector;
                        rightAngle.SetLength( (pushForce ^ fenceVector).y );
                        pushForce = rightAngle.SetLength(20.0f);
                    }
                    result -= pushForce;
                    m_avoidObstruction = result;
                    m_state = StateIdle;
                    m_wayPoint = m_pos - pushForce;
                    m_ordersSet = false;
                }
            }
            else
            {
                if( building->DoesSphereHit( result, 30.0f ) )
                {
                    Vector3 pushForce = (building->m_pos - result).SetLength(2.0f);
                    while( building->DoesSphereHit( result, 1.0f ) )
                    {
                        result -= pushForce;
                        //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                    }
                }
            }
            break;
        }
    }

    END_PROFILE( g_app->m_profiler, "PushFromBuildings" );

    //
    // If we already have some avoidance rules,
    // follow them above all else

    if( m_avoidObstruction != g_zeroVector )
    {
        float distance = ( m_avoidObstruction - pos ).Mag();
        if( distance < 10.0f )
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
    Task *controller = g_app->m_taskManager->GetTask( _controllerId );
    if( controller )
    {
        m_controllerId = _controllerId;
        m_wayPointId = controller->m_route->GetIdOfNearestWayPoint(m_pos);
        m_wayPoint = controller->m_route->GetWayPoint( m_wayPointId )->GetPos();
        m_wayPoint += Vector3( syncsfrand(30.0f), 0.0f, syncsfrand(30.0f) );
        m_wayPoint = PushFromObstructions( m_wayPoint );
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
        m_state = StateUnderControl;
        m_ordersSet = false;

        int numFlashes = 5 + darwiniaRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(5.0f), frand(15.0f), sfrand(5.0f) );
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash );
        }

        g_app->m_soundSystem->TriggerEntityEvent( this, "TakenControl" );
    }
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
    m_wayPoint = m_pos;
    m_wayPoint += Vector3( syncsfrand(100.0f), 0.0f, syncsfrand(100.0f) );
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

    if( m_onGround ) AdvanceToTargetPosition();

    int numFireParticles = syncrand() % 8;
    for( int i = 0; i < numFireParticles; ++i )
    {
        Vector3 fireSpawn = m_pos + g_upVector * syncfrand(5);
        //fireSpawn -= m_vel * 0.1f;
        float fireSize = 20 + syncfrand(30.0f);
        Vector3 fireVel = m_vel * 0.3f + g_upVector * (3+syncfrand(3));
        int particleType = Particle::TypeDarwinianFire;
        if( i > 4 ) particleType = Particle::TypeMissileTrail;
        g_app->m_particleSystem->CreateParticle( fireSpawn, fireVel, particleType, fireSize );
    }

    if( syncrand() % 50 == 0 )
    {
        g_app->m_soundSystem->TriggerEntityEvent( this, "OnFire" );
    }

    if( !m_dead && syncfrand(10) < 2 && m_onGround) ChangeHealth(-2);

    if( m_inWater > 0.0f )
    {
        m_state = StateIdle;
    }

    return false;
}


void Darwinian::SetFire()
{
    m_state = StateOnFire;
}


bool Darwinian::IsOnFire()
{
    return( m_state == StateOnFire );
}


inline void SetTexture( const char *texName )
{
    int texId = g_app->m_resource->GetTexture(texName);
    glBindTexture   ( GL_TEXTURE_2D, texId );
}

void Darwinian::Render( float _predictionTime, float _highDetail )
{
    if( !m_enabled ) return;

    if( m_state == StateInsideArmour )
    {
        return;
    }

    RGBAColour colour = RGBAColour(255,255,255);
    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS ) colour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
	if ( m_corrupted && frand() < 0.025 )
	{
		int t = floor(frand(NUM_TEAMS));
		if ( t < 0 || t >= NUM_TEAMS ) { t = 0; }
		colour = g_app->m_location->m_teams[ t ].m_colour;
	}
	if ( m_colourTimer > 0 && m_oldTeamId != 255 && m_oldTeamId != m_id.GetTeamId() )
	{
		float timeLeft = m_colourTimer / 4.0;
		RGBAColour c1 = g_app->m_location->m_teams[ m_oldTeamId ].m_colour * timeLeft;
		RGBAColour c2 = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour * (1 - timeLeft);
		colour = c1 + c2;
	}

	if ( m_soulless )
	{
        SetTexture( "sprites/soulless.bmp" ); 
		colour.a = 192;
	}
	else if ( m_corrupted )
	{
        int chosenIndex = 1 + m_id.GetUniqueId() % 4;
        char chosenOuch[256];
        sprintf( chosenOuch, "sprites/darwinian_ouch%d.bmp", chosenIndex );
        SetTexture( chosenOuch );            
	}

	Vector3 predictedPos = m_pos + m_vel * _predictionTime;;
    Vector3 entityUp = g_upVector;

    if( _highDetail > 0.0f )
    {
        if( m_onGround && m_inWater==-1 )
        {
            predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
        }
        else if( !m_onGround )
        {
            predictedPos.y += 3.0f;
        }

        entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );

        if( m_state == StateWorshipSpirit ||
            m_state == StateOperatingPort ||
            m_state == StateWatchingSpectacle )
        {
            entityUp.RotateAround( m_front * sinf(g_gameTime) * 0.3f );
        }
    }

    if( !m_onGround ) entityUp.RotateAround( m_front * g_gameTime * (m_id.GetUniqueId() % 10) * 0.1f );

    Vector3 entityRight(m_front ^ entityUp);

    if( m_state == StateCapturedByAnt )
    {
        Vector3 capturedRight = entityUp;
        entityUp = m_vel * -1;
        entityUp.Normalise();
        entityRight = capturedRight;
        predictedPos += Vector3( 0, 4, 0 );
    }

    float size = 3.0f;
    size *= (1.0f + 0.03f * (( m_id.GetIndex() * m_id.GetUniqueId() ) % 10));
    entityRight *= size;
    entityUp *= size * 2.0f;


/*
    #ifdef DEBUG_RENDER_ENABLED
        glDisable( GL_TEXTURE_2D );
    //    if( m_avoidObstruction != g_zeroVector )
    //    {
    //        RenderArrow( predictedPos, m_avoidObstruction, 1.0f );
    //    }
        ////    RenderArrow( predictedPos+entityUp*0.5f, predictedPos+entityUp*0.5f+entityFront*5.0f, 5.0f );
        RenderArrow( predictedPos+entityUp*0.5f, m_wayPoint, 1.0f, RGBAColour(255,0,0,255) );

    //    if( m_id.GetUniqueId() % 15 == 0 )
    //    {
    //        RenderSphere( predictedPos, m_threatRange, RGBAColour(255,0,0,255) );
    //    }

    //    if( m_state == StateCombat &&
    //        GetHighResTime() < m_grenadeTimer + 1.0f )
    //    {
    //        RenderArrow( predictedPos, m_grenadeTarget, 1.0f, RGBAColour(255,0,0,255) );
    //        RenderSphere( m_grenadeTarget, 50.0f, RGBAColour(255,0,0,255) );
    //    }
          glEnable( GL_TEXTURE_2D );
          glEnable( GL_BLEND );
    #endif*/



    //
    // Draw our shadow on the landscape

    if( _highDetail > 0.0f && m_shadowBuildingId == -1 && !m_dead )
    {
        int alpha = 150 * _highDetail;
        alpha = min( alpha, 255 );
        glColor4ub( 0, 0, 0, alpha  );

        Vector3 pos1 = predictedPos - entityRight;
        Vector3 pos2 = predictedPos + entityRight;
        Vector3 pos4 = pos1 + Vector3( 0.0f, 0.0f, size * 2.0f );
        Vector3 pos3 = pos2 + Vector3( 0.0f, 0.0f, size * 2.0f );

        pos1.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos1.x, pos1.z );
        pos2.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos2.x, pos2.z );
        pos3.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos3.x, pos3.z );
        pos4.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos4.x, pos4.z );

        glDepthMask( false );
        glLineWidth( 1.0f );
        glBegin( GL_QUADS );
            glTexCoord2f(0.0f, 0.0f);       glVertex3fv(pos1.GetData());
            glTexCoord2f(1.0f, 0.0f);       glVertex3fv(pos2.GetData());
            glTexCoord2f(1.0f, 1.0f);       glVertex3fv(pos3.GetData());
            glTexCoord2f(0.0f, 1.0f);       glVertex3fv(pos4.GetData());
        glEnd();
        glDepthMask( true );
    }

    if( !m_dead || !m_onGround )
    {

        //
        // Draw our texture

        float maxHealth = EntityBlueprint::GetStat(m_type, StatHealth);
        float health = (float) m_stats[StatHealth] / maxHealth;
        if( health > 1.0f ) health = 1.0f;
		colour *= 0.3f + 0.7f * health;
		if ( !m_soulless ) { colour.a = 255; }

        if( m_dead )
        {
            glEnable( GL_BLEND );
            colour.a = 2.5f * (float) m_stats[StatHealth];
            colour.r *= 0.2f;
            colour.g *= 0.2f;
            colour.b *= 0.2f;
        }

        if( m_state == StateOnFire )
        {
            colour.r *= 0.01f;
            colour.g *= 0.01f;
            colour.b *= 0.01f;

        }

        glColor4ubv(colour.GetData());
        glBegin(GL_QUADS);
            glTexCoord2i(0, 1);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
            glTexCoord2i(1, 1);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
            glTexCoord2i(1, 0);     glVertex3fv( (predictedPos + entityRight).GetData() );
            glTexCoord2i(0, 0);     glVertex3fv( (predictedPos - entityRight).GetData() );
        glEnd();

        if( m_corrupted || m_soulless )
        {            
            SetTexture( "sprites/darwinian.bmp" );            
        }
		
		//glDisable( GL_BLEND );

        //
        // Draw a blue glow if we are under control

        if( m_state == StateUnderControl )
        {
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
            glEnable    ( GL_BLEND );
            glDisable   ( GL_DEPTH_TEST );
            float scale = 1.1f;
            entityRight *= scale;
            entityUp *= scale;
            float alpha = fabs(sinf(g_gameTime * 2.0f));
            glColor4f   ( 0.0f, 0.0f, 1.0f, alpha );
            glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
                glTexCoord2i(1, 1);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
                glTexCoord2i(1, 0);     glVertex3fv( (predictedPos + entityRight).GetData() );
                glTexCoord2i(0, 0);     glVertex3fv( (predictedPos - entityRight).GetData() );
            glEnd();
            glEnable    ( GL_DEPTH_TEST );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        }

        //
        // Draw shadow if we are watching a God Dish

        if( _highDetail > 0.0f && m_shadowBuildingId != -1 )
        {
            Building *building = g_app->m_location->GetBuilding( m_shadowBuildingId );
            if( building )
            {
                float alpha = 1.0f;
                if( building->m_type == Building::TypeGodDish )
                {
                    GodDish *dish = (GodDish *) building;
                    if( !dish->m_activated && dish->m_timer < 1.0f )
                    {
                        m_shadowBuildingId = -1;
                    }
                    alpha = dish->m_timer * 0.1f;
                }
                else if( building->m_type == Building::TypeEscapeRocket )
                {
                    EscapeRocket *rocket = (EscapeRocket *) building;
                    if( !rocket->IsSpectacle() )
                    {
                        m_shadowBuildingId = -1;
                    }
                    float distance = ( building->m_pos - m_pos ).Mag();
                    if( distance < 400.0f )
                    {
                        alpha = 1.0f;
                    }
                    else if( distance < 700.0f )
                    {
                        alpha = ( 700 - distance ) / 300.0f;
                    }
                    else
                    {
                        alpha = 0.0f;
                    }
                }
                alpha = min( alpha, 1.0f );
                alpha = max( alpha, 0.0f );

                if( alpha > 0.0f )
                {
                    Vector3 length = ( predictedPos - building->m_pos ).SetLength( size * 10 );

                    // Shadow behind the Darwinian (green dudes only)
                    if( m_id.GetTeamId() == 0 )
                    {
                        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                        glEnable    ( GL_BLEND );
                        glColor4f   ( 0.0f, 0.0f, 0.0f, alpha);
                        Vector3 shadowVector = length;
                        shadowVector.SetLength( 0.05f );
                        predictedPos += shadowVector;
                        glBegin(GL_QUADS);
                            glTexCoord2i(0, 1);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
                            glTexCoord2i(1, 1);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
                            glTexCoord2i(1, 0);     glVertex3fv( (predictedPos + entityRight).GetData() );
                            glTexCoord2i(0, 0);     glVertex3fv( (predictedPos - entityRight).GetData() );
                        glEnd();
                    }

                    // Shadow on the ground

                    Vector3 pos1 = predictedPos - entityRight;
                    Vector3 pos2 = predictedPos + entityRight;
                    Vector3 pos4 = pos1 + length;
                    Vector3 pos3 = pos2 + length;
                    pos4 -= entityRight;
                    pos3 += entityRight;

                    pos1.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos1.x, pos1.z );
                    pos2.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos2.x, pos2.z );
                    pos3.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos3.x, pos3.z );
                    pos4.y = 0.3f + g_app->m_location->m_landscape.m_heightMap->GetValue( pos4.x, pos4.z );

                    //glDisable( GL_DEPTH_TEST );
                    glShadeModel( GL_SMOOTH );
                    glDepthMask ( false );
                    glBegin( GL_QUADS );
                        glColor4f   ( 0.0f, 0.0f, 0.0f, alpha);
                        glTexCoord2f(0.0f, 0.0f);       glVertex3fv(pos1.GetData());
                        glTexCoord2f(1.0f, 0.0f);       glVertex3fv(pos2.GetData());
                        glColor4f   ( 0.0f, 0.0f, 0.0f, 0.1f*alpha);
                        glTexCoord2f(1.0f, 1.0f);       glVertex3fv(pos3.GetData());
                        glTexCoord2f(0.0f, 1.0f);       glVertex3fv(pos4.GetData());
                    glEnd();
                    glShadeModel( GL_FLAT );
                    glDepthMask( true );
                }
            }
        }

    }



    //
    // Render a santa hat if it's Christmas

    if( m_id.GetTeamId() == 0 && Location::ChristmasModEnabled() == 1 )
    {
        if( m_id.GetUniqueId() % 3 == 0 )
        {
            int santaHatId = g_app->m_resource->GetTexture( "sprites/santahat.bmp" );
            glBindTexture( GL_TEXTURE_2D, santaHatId );
            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            predictedPos += entityUp * 0.95f;
            predictedPos += m_front * 0.01f;
            entityRight *= 0.65f;
            entityUp *= 0.65f;
            glBegin(GL_QUADS);
                glTexCoord2i(0, 1);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
                glTexCoord2i(1, 1);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
                glTexCoord2i(1, 0);     glVertex3fv( (predictedPos + entityRight).GetData() );
                glTexCoord2i(0, 0);     glVertex3fv( (predictedPos - entityRight).GetData() );
            glEnd();
            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
        }
    }

	//
	// Render subversion head gear
    if( m_subversive )
    {
		if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS ) colour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
        int santaHatId = g_app->m_resource->GetTexture( "sprites/brainwaves.bmp" );
        glBindTexture( GL_TEXTURE_2D, santaHatId );
        //glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glColor4ubv(colour.GetData());
        predictedPos += entityUp * 0.75f;
        predictedPos += m_front * 0.01f;
        entityRight *= 0.65f;
        entityUp *= 0.65f;
        glBegin(GL_QUADS);
            glTexCoord2i(0, 1);     glVertex3fv( (predictedPos - entityRight + entityUp).GetData() );
            glTexCoord2i(1, 1);     glVertex3fv( (predictedPos + entityRight + entityUp).GetData() );
            glTexCoord2i(1, 0);     glVertex3fv( (predictedPos + entityRight).GetData() );
            glTexCoord2i(0, 0);     glVertex3fv( (predictedPos - entityRight).GetData() );
        glEnd();
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/darwinian.bmp" ) );
    }
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
    // If we are dead render us in pieces

    if( m_dead )
    {
        Vector3 entityFront = m_front;
        entityFront.Normalise();
        entityUp = g_upVector;
        entityRight = entityFront ^ entityUp;
		entityUp *= size * 2.0f;
        entityRight.Normalise();
		entityRight *= size;
        unsigned char alpha = (float)m_stats[StatHealth] * 2.55f;

        glColor4ub( 0, 0, 0, alpha );

        entityRight *= 0.5f;
        entityUp *= 0.5;
        float predictedHealth = m_stats[StatHealth];
        if( m_onGround ) predictedHealth -= 40 * _predictionTime;
        else             predictedHealth -= 20 * _predictionTime;
        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );

        for( int i = 0; i < 3; ++i )
        {
            Vector3 fragmentPos = predictedPos;
            if( i == 0 ) fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if( i == 1 ) fragmentPos.x += 10.0f - predictedHealth / 10.0f;
            if( i == 1 ) fragmentPos.z += 10.0f - predictedHealth / 10.0f;
            if( i == 2 ) fragmentPos.x -= 10.0f - predictedHealth / 10.0f;
            fragmentPos.y += ( fragmentPos.y - landHeight ) * i * 0.5f;


            float left = 0.0f;
            float right = 1.0f;
            float top = 1.0f;
            float bottom = 0.0f;

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
                glTexCoord2f(left, bottom);     glVertex3fv( (fragmentPos - entityRight + entityUp).GetData() );
                glTexCoord2f(right, bottom);    glVertex3fv( (fragmentPos + entityRight + entityUp).GetData() );
                glTexCoord2f(right, top);       glVertex3fv( (fragmentPos + entityRight).GetData() );
                glTexCoord2f(left, top);        glVertex3fv( (fragmentPos - entityRight).GetData() );
            glEnd();
        }
    }
}


void Darwinian::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "SeenThreatAttack" );
    _list->PutData( "SeenThreatRunAway" );
    _list->PutData( "TakenControl" );
    _list->PutData( "EscapedControl" );
    _list->PutData( "GivenOrders" );
    _list->PutData( "VictoryJump" );
    _list->PutData( "OnFire" );
}



// ===========================================================================

BoxKite::BoxKite()
:   WorldObject(),
    m_state(StateHeld),
    m_birthTime(0.0f),
    m_deathTime(0.0f)
{
    m_shape = g_app->m_resource->GetShape( "boxkite.shp" );
    m_birthTime = GetHighResTime();

    m_size = 1.0f + syncsfrand(1.0f);
    m_type = EffectBoxKite;

    m_up = g_upVector;
}


bool BoxKite::Advance()
{
    if( m_state == StateReleased )
    {
        m_vel.Zero();
        m_vel.y = 2.0f + syncfrand(2.0f);

        m_vel.x = sinf( g_gameTime + m_id.GetIndex() );
        m_vel.z = sinf( g_gameTime + m_id.GetIndex() );

        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        float factor1 = SERVER_ADVANCE_PERIOD * 0.1f;
        float factor2 = 1.0f - factor1;
        m_up = m_up * factor2 + g_upVector * factor1;
        m_front.y = m_front.y * factor2;
        m_front.Normalise();

        if( GetHighResTime() > m_deathTime )
        {
            return true;
        }
    }

    m_brightness = 0.5f + syncfrand(0.5f);

    return false;
}


void BoxKite::Release()
{
    m_state = StateReleased;
    m_deathTime = GetHighResTime() + 180.0f;
}


void BoxKite::Render( float _predictionTime )
{
    glDisable   (GL_BLEND);
    glDepthMask (true);

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    float scale = ( GetHighResTime() - m_birthTime ) / 10.0f;
    if( scale > m_size ) scale = m_size;

    if( m_deathTime != 0.0f && GetHighResTime() > m_deathTime - 20.0f )
    {
        float deathScale = (m_deathTime - GetHighResTime()) / 20.0f;
        if( deathScale < 0.0f ) deathScale = 0.0f;
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

    Vector3 camUp = g_app->m_camera->GetUp() * 3.0f * scale * m_brightness;
    Vector3 camRight = g_app->m_camera->GetRight() * 3.0f * scale * m_brightness;

    glColor4f( 1.0f, 0.75f, 0.2f, m_brightness );

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (predictedPos-camRight+camUp).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (predictedPos+camRight+camUp).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (predictedPos+camRight-camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (predictedPos-camRight-camUp).GetData() );
    glEnd();

    camUp *= 0.2f;
    camRight *= 0.2f;

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3fv( (predictedPos-camRight+camUp).GetData() );
        glTexCoord2i(1,0);      glVertex3fv( (predictedPos+camRight+camUp).GetData() );
        glTexCoord2i(1,1);      glVertex3fv( (predictedPos+camRight-camUp).GetData() );
        glTexCoord2i(0,1);      glVertex3fv( (predictedPos-camRight-camUp).GetData() );
     glEnd();

    glDisable       ( GL_TEXTURE_2D );
}
