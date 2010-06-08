#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "worldobject/ai.h"
#include "worldobject/armour.h"
#include "worldobject/darwinian.h"
#include "worldobject/gunturret.h"
#include "worldobject/aiobjective.h"
#include "worldobject/multiwiniazone.h"
#include "worldobject/laserfence.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "explosion.h"
#include "camera.h"
#include "global_world.h"
#include "obstruction_grid.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "team.h"
#include "entity_grid.h"
#include "multiwinia.h"
#include "rocket.h"

LList<int> Armour::s_armourObjectives;


Armour::Armour()
:   Entity(),
    m_speed(0.0),
	m_up(g_upVector),
    m_state(StateIdle),
    m_numPassengers(0),
    m_previousUnloadTimer(0.0),
    m_newOrdersTimer(0.0),
    m_lastOrdersSet(0.0),
    m_suicideBuildingId(-1),
    m_currentAIObjective(-1),
    m_registeredPassengers(0),
    m_turretRushing(false),
    m_passengerOverflow(0),
	m_awaitingDeployment(false),
    m_conversionOrder(StateIdle),
    m_smoothYpos(0.0),
    m_recheckPathTimer(0.0)
{
    SetType( TypeArmour );
}


Armour::~Armour()
{
    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->UnRegisterSpecial( m_id );
    }
}

void ConvertShapeColours ( ShapeFragment *_frag, RGBAColour _col )
{
    RGBAColour *newColours = new RGBAColour[_frag->m_numColours];

    for( int i = 0; i < _frag->m_numColours; ++i )
    {
        newColours[i].r = _col.r;
        newColours[i].g = _col.g;
        newColours[i].b = _col.b;

        // scale colours down - colours on 3d models appear brighter than 2d equivalents due to lighting
        newColours[i] *= 0.1;
    }
    _frag->RegisterColours( newColours, _frag->m_numColours );


    for( int i = 0; i < _frag->m_childFragments.Size(); ++i )
    {
        ConvertShapeColours( _frag->m_childFragments[i], _col );
    }
}

void Armour::Begin()
{
    char filename[256];
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    int colourId = team->m_lobbyTeam->m_colourId;
    int groupId = team->m_lobbyTeam->m_coopGroupPosition;
	if( g_app->IsSinglePlayer() )
	{
		sprintf( filename, "armour.shp" );
	}
	else
	{
		sprintf( filename, "armour.shp_%d_%d", colourId, groupId );
	}
    m_shape = g_app->m_resource->GetShape(filename, false);
    if( !m_shape )
    {
        m_shape = g_app->m_resource->GetShapeCopy( "armour.shp", false, false );

		if( !g_app->IsSinglePlayer() )
		{
			RGBAColour colour = team->m_colour;// * 0.3;
			ConvertShapeColours( m_shape->m_rootFragment, colour);

			m_shape->BuildDisplayList();
		}

		g_app->m_resource->AddShape( m_shape, filename );
    }

    const char markerEntranceName[] = "MarkerEntrance";
    m_markerEntrance    = m_shape->m_rootFragment->LookupMarker( markerEntranceName );
    AppReleaseAssert( m_markerEntrance, "Armour: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerEntranceName, m_shape->m_name );

	/*
    const char markerFlagName[] = "MarkerFlag";
    m_markerFlag        = m_shape->m_rootFragment->LookupMarker( markerFlagName );
    AppReleaseAssert( m_markerFlag, "Armour: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerFlagName, m_shape->m_name );
	*/

    m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius( g_identityMatrix34, m_centrePos );

    Entity::Begin();

    m_wayPoint = m_pos;

    if( m_id.GetTeamId() != 255 )
    {
        Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        team->RegisterSpecial( m_id );
    }

    m_flag.SetPosition( m_pos );
    m_flag.SetOrientation( m_front, m_up );
    m_flag.SetSize( 20.0 );
    m_flag.Initialise();
}


bool Armour::ChangeHealth( int _amount, int _damageType )
{
    if( _damageType == DamageTypeLaser ) return false;
    if( _damageType == DamageTypeTurretShell ) _amount /= 3;

    bool dead = m_dead;
    
    // I assume this is to make the armour immune to everything but gunturrets?
    // shouldnt do this in Multiplayer anyway, armour should be destroyable by grenades
    // - Gary

    if( !g_app->Multiplayer() )
    {
        if( _amount < 0 && _amount > -1000 ) _amount = -1;
    }

    if( !m_dead )
    {
        if( _amount < 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LoseHealth" );
        }

        int oldHealth = m_stats[StatHealth];
        int newHealth = oldHealth + _amount;
        newHealth = max( newHealth, 0 );
        newHealth = min( newHealth, 255 );
        m_stats[StatHealth] = newHealth;

        int healthBandBefore = int(oldHealth / 20.0);
        int healthBandAfter = int(newHealth / 20.0);

        if( healthBandBefore != healthBandAfter || newHealth == 0 )
        {
            double fractionDead = 1.0 - (double) newHealth / (double) EntityBlueprint::GetStat( TypeArmour, StatHealth );
            fractionDead *= 0.5;
            if( newHealth == 0 ) fractionDead = 1.0;
            Matrix34 bodyMat( m_front, m_up, m_pos );
            g_explosionManager.AddExplosion( m_shape, bodyMat, fractionDead ); 
        }
        else
        {
            // Always give some feedback when hitting armour with turret shell
            if( _damageType == DamageTypeTurretShell )
            {
                double fractionDead = 0.05;
                Matrix34 bodyMat( m_front, m_up, m_pos );
                g_explosionManager.AddExplosion( m_shape, bodyMat, fractionDead ); 
            }
        }

        if( newHealth == 0 )
        {
            m_dead = true;

            g_app->m_location->Bang( m_pos, 20, 50, m_id.GetTeamId() );
            g_app->m_soundSystem->TriggerEntityEvent( this, "Die" );
        }
    }

    return true;
}


void Armour::ConvertToGunTurret()
{
    GunTurret turretTemplate;
    turretTemplate.m_pos = m_pos;
    turretTemplate.m_front = m_front;
    turretTemplate.m_dynamic = true;
    
    GunTurret *turret = (GunTurret *) Building::CreateBuilding( Building::TypeGunTurret );
    g_app->m_location->m_buildings.PutData( turret );
    turret->Initialise((Building *)&turretTemplate);
    int id = g_app->m_globalWorld->GenerateBuildingId();
    turret->m_id.SetUnitId( UNIT_BUILDINGS );
    turret->m_id.SetUniqueId( id );
    g_app->m_location->m_obstructionGrid->CalculateAll();

    //
    // Explode some polys, to cover the ropey change
    Matrix34 bodyMat( m_front, m_up, m_pos );
    g_explosionManager.AddExplosion( m_shape, bodyMat ); 
    turret->ExplodeBody();
}


void Armour::AdvanceToTargetPos()
{
    //
    // Turn to face our waypoint

	Vector3 toTarget = m_wayPoint - m_pos;
	toTarget.HorizontalAndNormalise();
	double angle = iv_acos( toTarget * m_front );

    Vector3 horizDif = (m_wayPoint - m_pos);
	if ( (m_wayPoint - m_pos).Mag() > 10.0 )
    {
	    Vector3 rotation = m_front ^ toTarget;
        double rotateSpeed = angle*0.05;            
	    rotation.SetLength(rotateSpeed);
	    m_front.RotateAround(rotation);
		m_front.Normalise();
        if( m_front.y > 0.1 ) m_front.y = 0.1;
        if( m_front.y < -0.1 ) m_front.y = -0.1;
        m_front.Normalise();
    }

    //
    // Move towards waypoint
    
    toTarget = m_wayPoint - m_pos;
    toTarget.y = 0.0;
    double distance = toTarget.Mag();
    if( distance > 100.0 && angle < 1.0 )
    {
        m_speed += 10.0 * SERVER_ADVANCE_PERIOD;
        m_speed = min( m_speed, (double)m_stats[StatSpeed] );
    }
    else if( distance > 10.0 )
    {
        double targetSpeed = distance * 0.2;
        m_speed = m_speed * 0.95 + targetSpeed * 0.05;
        m_speed = min( m_speed, (double)m_stats[StatSpeed] );
        m_speed = max( m_speed, 0.0 );
    }
    else
    {
        m_speed -= 5.0 * SERVER_ADVANCE_PERIOD;
        m_speed = max( m_speed, 0.0 );
    }


    //
    // Hover above the ground
    
    Vector3 oldPos = m_pos;
    m_pos += m_front * m_speed * SERVER_ADVANCE_PERIOD;
    PushFromObstructions( m_pos );
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( landHeight < 0 ) landHeight = 0;

    if( m_pos.y <= landHeight )
    {
        double factor = SERVER_ADVANCE_PERIOD * 5.0;
        m_pos.y = m_pos.y * (1.0-factor) + landHeight * factor;
        
        factor = SERVER_ADVANCE_PERIOD * 5.0;
        Vector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        m_up = m_up * (1.0-factor) + landUp * factor;

        double distTravelled = ( m_pos - oldPos ).Mag();
        if( distTravelled > m_speed * SERVER_ADVANCE_PERIOD )
        {
            m_pos = oldPos + ( m_pos - oldPos ).SetLength( m_speed * SERVER_ADVANCE_PERIOD );
        }
    }
    else
    {
        double heightAboveGround = m_pos.y - landHeight;
        m_vel.y -= 5.0;
        m_pos.y += m_vel.y * SERVER_ADVANCE_PERIOD;
        m_pos.y = max( m_pos.y, landHeight );

        double factor = SERVER_ADVANCE_PERIOD * 0.5;
        Vector3 landUp = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        m_up = m_up * (1.0-factor) + landUp * factor;   
    }

    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;

}


void Armour::DetectCollisions()
{
	int numFound;
	g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours,
								m_pos.x, m_pos.z, 40.0, &numFound);

    Vector3 escapeVector;
    bool collisionDetected = false;

    for (int i = 0; i < numFound; ++i)
	{
		Entity *ent = g_app->m_location->GetEntity(s_neighbours[i]);

        if (ent &&
            ent->m_type == Entity::TypeArmour &&
            ent->m_id != m_id &&
            !ent->m_dead )
		{
            escapeVector = m_pos - ent->m_pos;			
            double distance = escapeVector.Mag();

            if( distance < 40 )
            {
                escapeVector.SetLength( 40 - distance );
                collisionDetected = true;            
                break;
            }
		}
	}

    //if( !collisionDetected )
    {
        Vector3 exitPos, exitDir, newExitPos;
        GetEntrance( exitPos, exitDir );
        newExitPos = exitPos;
        bool hit = false;

        LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( exitPos.x, exitPos.z );

        for( int b = 0; b < buildings->Size(); ++b )
        {
            int buildingId = buildings->GetData(b);
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building )
            {        
                if( building->DoesSphereHit( exitPos, 20.0 ) )
                {
                    hit = true;
                    while( building->DoesSphereHit( newExitPos, 5.0 ) )
                    {
                        Vector3 dir = (newExitPos - building->m_pos).Normalise();
                        dir.SetLength(3.0);
                        newExitPos += dir;
                    }
                    break;
                }
            }
        }

        if( hit )
        {
            // our entrance is obstructed, we need to move
            double distance = (exitPos - newExitPos).Mag();
            escapeVector += (newExitPos - exitPos).SetLength(distance);
            collisionDetected = true;
        }
    }

    if( collisionDetected )
    {        
        bool waypointNearby = ( m_pos - m_wayPoint ).MagSquared() < 40.0 * 40.0;

        m_pos += escapeVector * 0.02;      
        m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

        if( waypointNearby )
        {
            m_wayPoint += escapeVector * 0.02;        
            m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_wayPoint.x, m_wayPoint.z);
        }
    }
}


bool Armour::Advance( Unit *_unit )
{
    if( m_dead ) return true;

    m_newOrdersTimer += SERVER_ADVANCE_PERIOD;

    AdvanceToTargetPos();
    
    if( !g_app->Multiplayer() ||
        m_vel.Mag() < 5 )
    {        
        DetectCollisions();
    }


    //
    // Create some smoke

    double velocity = m_vel.Mag();
    int numSmoke = 1 + int( velocity / 20.0 );
    for( int i = 0; i < numSmoke; ++i )
    {
        Vector3 right = m_up ^ m_front;
        Vector3 vel = m_front;
        vel.RotateAround( m_up * syncfrand(2.0 * M_PI) );
        vel.SetLength( syncfrand(5.0) );
        Vector3 pos = m_pos + vel * 2;
        pos.y += 3.0;
        double size = 50.0 + (syncrand() % 50);
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileTrail, size );
    }
    

    if( g_app->Multiplayer() )
    {
        if( m_state == StateLoading && m_numPassengers == Capacity() )
        {
            m_state = StateIdle;
        }

        if( m_state == StateUnloading && m_numPassengers == 0 )
        {
            m_state = StateIdle;
        }
    }


    //
    // Are we supposed to be converting here?

    if( m_conversionPoint.MagSquared() > 0.1 )
    {
        double distance = ( m_conversionPoint - m_pos ).Mag();
       // if( g_app->Multiplayer() )
        {
			if( distance < 15.0 )
            {
				if( m_awaitingDeployment )
				{
					ConvertToGunTurret();
					return true;
				}
                else if( m_conversionOrder != StateIdle )
                {
                    m_state = m_conversionOrder;
                }
                
                m_newOrdersTimer = 2.0;
                m_conversionPoint = g_zeroVector;
                m_conversionOrder = StateIdle;
            }            
        }
        /*else
        {
            if( distance < 50.0 && m_numPassengers > 0 )
            {
                // We are close and need to unload our passengers
                m_state = StateUnloading;
            }
			else if( distance < 50.0 && !m_awaitingDeployment  )
			{
				m_state = StateLoading;
			}
			else if( distance < 10.0 && m_numPassengers == 0 )
			{
                ConvertToGunTurret();
                return true;
            }

			m_newOrdersTimer = 2.0;
            m_conversionPoint = g_zeroVector;
        }*/
    }
    
    m_numPassengers = m_registeredPassengers;
    m_registeredPassengers = 0;
    m_passengerOverflow = 0;
    
    return Entity::Advance( _unit );
}

void Armour::RunAI(AI *_ai)
{
    switch( g_app->m_multiwinia->m_gameType )
    {
        case Multiwinia::GameTypeRocketRiot:    RunRocketRiotAI( _ai );     break;
        case Multiwinia::GameTypeAssault:       RunAssaultAI( _ai );        break;
        case Multiwinia::GameTypeBlitzkreig:    RunBlitzkriegAI( _ai );     break;

        default:    RunStandardAI( _ai );
    }    

    if( m_speed > 0.0 )
    {
        m_recheckPathTimer -= SERVER_ADVANCE_PERIOD;
        if( m_recheckPathTimer <= 0.0 )
        {
            m_recheckPathTimer = 1.0;
            if( !IsRouteClear( m_wayPoint ) )
            {
                // our path is no longer clear
                // maybe a laser fence has been reactivated, or maybe our turning circle has moved us into the path
                // of a fence
                // in any case, replot our path
                AISetConversionPoint( m_wayPoint );
            }

        }
    }
}

void Armour::RunBlitzkriegAI( AI *_ai )
{
	if( m_numPassengers > 0 )
    {
        if( (m_pos - m_conversionPoint).Mag() < 100.0 )
        {
            m_state = StateUnloading;
            if( m_turretRushing ) 
            {
                // keep moving beyond the turret so that our darwinians drop on top of it
                m_wayPoint = m_pos + (m_front ^g_upVector) * 50 + m_front * 100.0;
            }
        }
    }

	m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0 ) return;
    m_aiTimer = 1.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ) m_aiTimer = 20.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard ) m_aiTimer = 10.0;

    int aiType = g_app->m_multiwinia->m_aiType;

    int numPassengers = GetNumPassengers();
    if( numPassengers == 0 )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeTrunkPort &&
                    b->m_id.GetTeamId() == m_id.GetTeamId() )
                {
                    int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( b->m_pos.x, b->m_pos.z, 200.0, m_id.GetTeamId() );
                    if( numFriends > 20 )
                    {
                        //if( !dontLoad )
                        {
                            AISetConversionPoint( b->m_pos + b->m_front * 100.0 );
                            break;
                        }
                    }
                }
            }
        }
    }

    bool wait = false;
    if( numPassengers > 20 && m_vel.Mag() == 0.0 )
    {
        wait = ( CountApproachingDarwinians() > 0 );

        if( !wait )
        {
            int targetId = -1;
            LList<int> validZones;

            
            bool stop = false;
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i) 
            {
                if( g_app->m_location->m_buildings.ValidIndex( i ) )
                {
                    MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->m_buildings[i];
                    if( zone && zone->m_type == Building::TypeMultiwiniaZone )
                    {
                        if( zone->m_id.GetTeamId() == m_id.GetTeamId() )
                        {
                            for( int j = 0; j < zone->m_blitzkriegLinks.Size(); ++j )
                            {
                                MultiwiniaZone *linkedZone = (MultiwiniaZone *)g_app->m_location->GetBuilding( zone->m_blitzkriegLinks[j] );
                                if( linkedZone && linkedZone->m_type == Building::TypeMultiwiniaZone )
                                {
                                    if( linkedZone->m_id.GetTeamId() != m_id.GetTeamId() )
                                    {
                                        validZones.PutData( linkedZone->m_id.GetUniqueId() );
                                    }

                                    int teamId = linkedZone->GetBaseTeamId();
                                    if( teamId != -1 && linkedZone->m_id.GetTeamId() != m_id.GetTeamId() )
                                    {                                        
                                        if( !g_app->m_multiwinia->m_eliminated[teamId] )
                                        {
                                            validZones.Empty();
                                            validZones.PutData( linkedZone->m_id.GetUniqueId() );
                                            stop = true;
                                            break;
                                        }
                                    }
                                }
                            }
                            if( stop ) break;
                        }
                    }
                }
            }

            if( !stop && g_app->m_multiwinia->m_teams[m_id.GetTeamId()].m_score == BLITZKRIEGSCORE_UNDERTHREAT )
            {
                bool doIt = false;
                if( aiType == Multiwinia::AITypeEasy ) doIt = true;
                if( aiType == Multiwinia::AITypeStandard ) doIt = (syncfrand(1.0) < 0.5);
                if( doIt )
                {
                    validZones.Empty();
                    MultiwiniaZone *base = (MultiwiniaZone *)g_app->m_location->GetBuilding( MultiwiniaZone::s_blitzkriegBaseZone[m_id.GetTeamId()] );
                    if( base && base->m_type == Building::TypeMultiwiniaZone )
                    {
                        // we are under threat, so go capture one of the zones attached to ours
                        for( int i = 0; i < base->m_blitzkriegLinks.Size(); ++i )
                        {
                            MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( base->m_blitzkriegLinks[i] );
                            if( zone && zone->m_type == Building::TypeMultiwiniaZone )
                            {
                                if( zone->m_id.GetTeamId() != m_id.GetTeamId() )
                                {
                                    validZones.PutData( zone->m_id.GetUniqueId() );
                                }
                            }
                        }
                    }
                }
            }

            if( validZones.Size() > 0 )
            {
                bool random = false;
                if( aiType == Multiwinia::AITypeEasy ) random = true;
                if( aiType == Multiwinia::AITypeStandard ) random = syncfrand(1) < 0.5;

                targetId = validZones[ syncrand() % validZones.Size() ];
            }
            
            Building *target = g_app->m_location->GetBuilding( targetId );
            if( target )
            {
                if( aiType == Multiwinia::AITypeEasy )
                {
                    AISetConversionPoint( target->m_pos );
                }
                else
                {
                    GetBlitzkriegDropPoint( target );
                }
            }
        }
    }
}

void Armour::GetBlitzkriegDropPoint( Building *_b )
{
    int numTurrets = 0;
    Vector3 turretPos;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i) 
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            double range = 200.0;
            if( _b->m_type == Building::TypeMultiwiniaZone ) range = ((MultiwiniaZone *)_b)->m_size * 1.2;
            Building *building = g_app->m_location->m_buildings[i];
            if( (building->m_pos - _b->m_pos).Mag() < range &&
                building->m_type == Building::TypeGunTurret &&
                building->m_id.GetTeamId() != m_id.GetTeamId() )
            {
                numTurrets++;
                turretPos = building->m_pos;
            }
        }
    }

    if( numTurrets == 0 )
    {
        AISetConversionPoint(_b->m_pos);
    }
    else if( numTurrets == 1 )
    {
        AISetConversionPoint(turretPos);
        m_turretRushing = true;
    }
    else
    {
        // TEMP - THIS NEEDS TO BE UPDATED TO SOMETHING NOT STUPID
        AISetConversionPoint(turretPos);
        m_turretRushing = true;
    }
}

void Armour::RunMobileBombAI( AI *_ai )
{
    m_state = StateIdle;
    if( m_suicideBuildingId == -1 )
    {
        int buildingId = -1;
        double distance = FLT_MAX;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i ) 
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeGunTurret ||
                    b->m_type == Building::TypeSpawnPoint )
                {
                    if( b->m_id.GetTeamId() != 255 &&
                        b->m_id.GetTeamId() != m_id.GetTeamId() )
                    {
                        if( (m_pos - b->m_pos).Mag() < distance )
                        {
                            distance = (m_pos - b->m_pos).Mag();
                            buildingId = b->m_id.GetUniqueId();
                        }
                    }
                }
            }
        }

        Building *b = g_app->m_location->GetBuilding( buildingId );
        if( b )
        {
            m_suicideBuildingId = buildingId;
            SetWayPoint( b->m_pos );
        }
    }

    if( m_suicideBuildingId != -1 )
    {
        Building *b = g_app->m_location->GetBuilding(m_suicideBuildingId);
        if( b )
        {
            if( (m_pos - b->m_pos).Mag() < 20.0)
            {
                ChangeHealth(-1000);
            }
        }
    }
}

void Armour::FindPickupPoint( AI *_ai )
{
    Vector3 pickupPos = g_zeroVector;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];

            if( b->m_type == Building::TypeAIObjectiveMarker )
            {
                AIObjectiveMarker *marker = (AIObjectiveMarker *)b;
                if( marker->m_pickupAvailable &&
                    marker->m_armourObjective == 1 &&
                    (m_pos - marker->m_pos).Mag() > DARWINIAN_SEARCHRANGE_ARMOUR * 1.5) // make sure we dont move 3 inches to the left and pick up all the guys we just dropped off
                {
                    bool include[NUM_TEAMS];
                    memset( include, false, sizeof(include) );
                    include[m_id.GetTeamId()] = true;
                    if( g_app->m_location->m_entityGrid->GetNumNeighbours( marker->m_pos.x, marker->m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, include ) > 25 )
                    {
                        pickupPos = marker->m_pos;
                        break;
                    }
                }
            }

            if( b->m_type == Building::TypeTrunkPort &&
                b->m_id.GetTeamId() == m_id.GetTeamId()  )
            {
                pickupPos = b->m_pos + b->m_front * 75.0;
            }
        }
    }
    if( pickupPos != g_zeroVector )
    {
        m_pickupPoint = pickupPos;
        m_pickupPoint += Vector3( SFRAND(10.0), 0.0, SFRAND(10.0) );
        m_wayPoint = pickupPos;
        m_state = StateIdle;
        //SetConversionPoint( pickupPos );
    }
}

int Armour::CountApproachingDarwinians()
{
    int numFound;
    g_app->m_location->m_entityGrid->GetFriends( s_neighbours, m_pos.x, m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, &numFound, m_id.GetTeamId() );

    int numApproaching = 0;
    for( int i = 0; i < numFound; ++i )
    {
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
        if( d )
        {
            if( d->m_state == Darwinian::StateApproachingArmour )
            {
                numApproaching++;
            }
        }
    }

    return numApproaching;
}

void Armour::RunAssaultAI( AI *_ai )
{
	m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0 ) return;
    m_aiTimer = 1.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ) m_aiTimer = 10.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard ) m_aiTimer = 5.0;

    //if( _ai->m_currentObjective == -1 ) return;
    //if( g_app->m_location->IsDefending(m_id.GetTeamId()) ) return;

    if( s_armourObjectives.Size() > 0 && m_numPassengers == 0 )
    {
        AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding( _ai->m_currentObjective );
        if( objective && objective->m_type == Building::TypeAIObjective )
        {
            if( objective->m_armourMarker == -1 ) 
            {
                RunMobileBombAI( _ai );
                return;
            }
        }
    }

    if( m_currentAIObjective == -1 ) m_currentAIObjective = _ai->m_currentObjective;

    if( m_currentAIObjective != _ai->m_currentObjective )
    {
        // stop so we can find a new objective, no sense dropping off at an out of date one
        SetWayPoint( m_pos );
        m_currentAIObjective = _ai->m_currentObjective;
    }

    bool dontLoad = false;    

    if( m_numPassengers == 0 && 
        m_state == StateUnloading )
    {
        FindPickupPoint( _ai );
    }

    int numApproaching = CountApproachingDarwinians();

    if( m_speed == 0.0) 
    {
        if( m_numPassengers > 20 && m_state != StateUnloading )
        {
            if( numApproaching == 0 )
            {
                AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding( _ai->m_currentObjective );
                if( objective )
                {
                    int index = -1;
                    if( objective->m_armourMarker != -1 )
                    {
                        index = objective->m_armourMarker;
                    }
                    else 
                    {
                        LList<int> validMarkers;
                        for( int i = 0; i < objective->m_objectiveMarkers.Size(); ++i )
                        {
                            AIObjectiveMarker *marker = (AIObjectiveMarker *)g_app->m_location->GetBuilding( objective->m_objectiveMarkers[i] );
                            if( marker )
                            {
                                if( g_app->m_location->IsAttacking( m_id.GetTeamId() ) &&
                                    !g_app->m_location->IsFriend(marker->m_id.GetTeamId(), m_id.GetTeamId() ) )
                                {
                                    validMarkers.PutData(marker->m_id.GetUniqueId());
                                }

                                if( g_app->m_location->IsDefending( m_id.GetTeamId() ) )
                                {
                                    validMarkers.PutData(marker->m_id.GetUniqueId());
                                }
                            }
                        }
                        if( validMarkers.Size() > 0 )
                        {
                            index = validMarkers[ syncrand() % validMarkers.Size() ];
                        }

                    }

                    AIObjectiveMarker *marker = (AIObjectiveMarker *)g_app->m_location->GetBuilding( index );
                    if( marker )
                    {
                        AISetConversionPoint( marker->m_pos );
                    }
                }
            }
        }
        else if( m_numPassengers == 0 || (numApproaching == 0 && m_state != StateUnloading) )
        {
            m_state = StateIdle;
            //if( m_state == StateIdle )
            {
                /*if( m_numPassengers + g_app->m_location->m_entityGrid->GetNumFriends( m_pos.x, m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, m_id.GetTeamId() ) >= 30 )
                {
                    m_state = StateLoading;
                }
                else*/
                {
                    FindPickupPoint( _ai );
                }
            }
        }
    }

    if( (m_pos - m_pickupPoint).Mag() < DARWINIAN_SEARCHRANGE_ARMOUR )
    {
        m_state = StateLoading;
        m_pickupPoint = g_zeroVector;
    }
    else if( m_numPassengers > 0 )
    {
        if( (m_pos - m_conversionPoint).Mag() < 75.0 )
        {
            m_state = StateUnloading;
        }
    }
}

void Armour::RunStandardAI( AI *_ai )
{
	m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0 ) return;
    m_aiTimer = 1.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ) m_aiTimer = 20.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard ) m_aiTimer = 10.0;

    if( m_suicideBuildingId != -1 )
    {
        Building *b = g_app->m_location->GetBuilding( m_suicideBuildingId );
        if( b ) 
        {
            if( (m_pos - b->m_pos).Mag() < 10.0 )
            {
                // bit of a cheat, but easier than terminating the task via the task manager
                ChangeHealth( -1000 );
            }
        }
        else 
        {
            m_suicideBuildingId = -1;
        }
        return;
    }

    // find the nearest spawn point and set up shop there
    if( m_numPassengers == 0 && m_state == StateIdle )
    {
        double distance = FLT_MAX;
        int buildingId = -1;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeSpawnPoint &&
                    b->m_id.GetTeamId() == m_id.GetTeamId() )
                {
                    double dist = (m_pos - b->m_pos).Mag();
                    if( dist < distance )
                    {
                        distance = dist;
                        buildingId = b->m_id.GetUniqueId();
                    }
                }
            }
        }
        
        Building *b = g_app->m_location->GetBuilding( buildingId );
        if( b )
        {
            AISetConversionPoint( b->m_pos + b->m_front * 100.0 );
        }
    }

    if( m_numPassengers > 50 )
    {
        if( m_conversionPoint == g_zeroVector )
        {
            // look for an enemy spawn point to go attack
            double distance = FLT_MAX;
            int buildingId = -1;
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *b = g_app->m_location->m_buildings[i];
                    if( b->m_type == Building::TypeSpawnPoint &&
                        b->m_id.GetTeamId() != m_id.GetTeamId() &&
                        b->m_id.GetTeamId() != 255 )
                    {
                        double dist = (m_pos - b->m_pos).Mag();
                        if( dist < distance )
                        {
                            distance = dist;
                            buildingId = b->m_id.GetUniqueId();
                        }
                    }
                }
            }

            Building *b = g_app->m_location->GetBuilding( buildingId );
            if( b )
            {
                // set our conversion  point behind the building, its much less likely to be defended
                AISetConversionPoint( b->m_pos - b->m_front * 100.0 );
            }
        }
    }

    if( m_numPassengers == 0 && m_state == StateUnloading )
    {
        // 2 options here, go back to one of our spawn points to pick up more darwinians
        // of drive to another enemy spawn point and explode

        bool explode = syncrand() % 2;
        if( explode )
        {
            double distance = FLT_MAX;
            int buildingId = -1;
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *b = g_app->m_location->m_buildings[i];
                    if( b->m_type == Building::TypeSpawnPoint &&
                        b->m_id.GetTeamId() != m_id.GetTeamId() &&
                        b->m_id.GetTeamId() != 255 )
                    {
                        double dist = (m_pos - b->m_pos).Mag();
                        if( dist < distance &&
                            dist > 200.0)
                        {
                            distance = dist;
                            buildingId = b->m_id.GetUniqueId();
                        }
                    }
                }
            }

            m_suicideBuildingId = buildingId;
            Building *b = g_app->m_location->GetBuilding( m_suicideBuildingId );
            if( b ) 
            {
                SetWayPoint( b->m_pos );
                m_state = StateIdle;
            }
        }
        else
        {
            // dont need to do anything else in this case, the next ai run will move us
            m_state = StateIdle;
        }
    }
}

bool Armour::PreventLoadOverlap()
{
    if( m_numPassengers == 0 )
    {
        // make sure we dont try to load while another armour nearby is trying to as well
        bool includes[NUM_TEAMS];
        memset( includes, false, sizeof(bool) * NUM_TEAMS );
        includes[m_id.GetTeamId()] = true;
        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, &numFound, includes );
        for( int i = 0; i < numFound; ++i )
        {
            Armour *a = (Armour*)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeArmour );
            if( a )
            {
                if( a == this ) continue;

                if( a->m_id.GetTeamId() == m_id.GetTeamId() && 
                    a->m_state == StateLoading )
                {
                    m_state = StateIdle;
                    return true;
                }
            }
        }
    }

    return false;
}

bool Armour::RRValidPickupPoint( Vector3 const &_pos )
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        WorldObjectId id = *team->m_specials.GetPointer(i);
        Armour *a = (Armour *)g_app->m_location->GetEntitySafe( id, Entity::TypeArmour );
        if( a )
        {
            if( ( a->m_conversionPoint - _pos ).Mag() < 100.0 ) 
            {
                return false;
            }

            if( a->m_state == Armour::StateLoading &&
                ( a->m_pos - _pos ).Mag() < 100.0 )
            {
                return false;
            }
                
        }
    }

    return true;
}

void Armour::FindIdleDarwinians()
{
    LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_id.GetTeamId()];
    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

    Vector3 pos;
    Vector3 trunkportPos;
    int num = 10;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeAITarget )
            {
                AITarget *t = (AITarget *)b;
                if( t->m_idleCount[m_id.GetTeamId()] > num &&
                    (t->m_pos - rocket->m_pos).Mag() > 400.0 && 
                    RRValidPickupPoint( t->m_pos ) )
                {
                    num = t->m_idleCount[m_id.GetTeamId()];
                    pos = t->m_pos;
                    break;
                }
            }

            if( b->m_type == Building::TypeTrunkPort &&
                b->m_id.GetTeamId() == m_id.GetTeamId() )
            {
                trunkportPos = b->m_pos + b->m_front * 75.0;
            }
        }
    }
    
    if( pos != g_zeroVector )
    {
        AISetConversionPoint( pos );
    }
    else
    {
        AISetConversionPoint( trunkportPos );
    }
}

void Armour::RRFindPickupPoint()
{
    LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_id.GetTeamId()];
    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( (b->m_type == Building::TypeTrunkPort &&
                b->m_id.GetTeamId() == m_id.GetTeamId() ) ||
                (b->m_type == Building::TypeAITarget &&
                rocket &&
                ( b->m_pos - rocket->m_pos ).Mag() < 250.0 ) ||
                (b->m_type == Building::TypeSpawnPoint &&
                b->m_id.GetTeamId() == m_id.GetTeamId() ) )
            {
                int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( b->m_pos.x, b->m_pos.z, 200.0, m_id.GetTeamId() );
                if( numFriends > 20 )
                {
                    AISetConversionPoint( b->m_pos + b->m_front * 100.0 );
                    break;
                }
            }
        }
    }
}

void Armour::RRFindDropoffPoint( AI *_ai )
{
    double distance = FLT_MAX;
    int numPanels = 0;
    int buildingId = -1;
    bool pointSet = false;
    bool rocketAttack = false;

    LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_id.GetTeamId()];
    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( i == m_id.GetTeamId() ) continue;
        // check each enemy teams rocket to make sure they arent close to launch
        EscapeRocket *theirRocket = (EscapeRocket *)g_app->m_location->GetBuilding( g_app->m_multiwinia->m_teams[i].m_rocketId );
        if( theirRocket )
        {
            if( theirRocket->m_fuel >= 75.0 &&
                theirRocket->m_fuel > rocket->m_fuel )
            {
                rocketAttack = true;
                buildingId = theirRocket->m_id.GetUniqueId();
                break;
            }
        }
    }

    if( buildingId == -1 )
    {
        int bestReinforcementTarget = -1;
        int reinforcementNum = INT_MAX;
        for( int i = 0; i < _ai->m_rrObjectiveList.Size(); ++i )
        {
            RRSolarPanelInfo *info = _ai->m_rrObjectiveList[i];
            int numFriends = g_app->m_location->m_entityGrid->GetNumFriends( info->m_pos.x, info->m_pos.z, info->m_range, m_id.GetTeamId() );
            int numEnemies = g_app->m_location->m_entityGrid->GetNumEnemies( info->m_pos.x, info->m_pos.z, info->m_range, m_id.GetTeamId() );

            if( numEnemies == 0 && numFriends == 0 )
            {
                // uncontrolled panels, so lets just go grab those
                buildingId = -1;
                AISetConversionPoint( info->m_pos );
                pointSet = true;
                break;
            }

            if( numEnemies > numFriends * 1.3 )
            {
                // if we're going to attack some occupied panels, we might as well find the best target
                if( info->m_numPanels > numPanels )
                {
                    numPanels = info->m_numPanels;
                    buildingId = info->m_objectiveId;
                }
            }

            if( numEnemies == 0 )
            {
                if( numFriends <= info->m_numPanels * 5 )
                {
                    reinforcementNum = numFriends;
                    bestReinforcementTarget = info->m_objectiveId;
                }
                else
                {
                    if( numFriends < reinforcementNum )
                    {
                        reinforcementNum = numFriends;
                        bestReinforcementTarget = info->m_objectiveId;
                    }

                }
            }
        }

        if( buildingId == -1 && !pointSet )
        {
            buildingId = bestReinforcementTarget;
        }
    }

    if( buildingId != -1 )
    {
        Building *b = g_app->m_location->GetBuilding( buildingId );
        if( b )
        {
            Vector3 pos = b->m_pos;
            if( g_app->m_multiwinia->m_aiType > Multiwinia::AITypeEasy )
            {
                pos.x += iv_cos(syncfrand(M_PI*2.0)) * 100.0;
                pos.z += iv_sin(syncfrand(M_PI*2.0)) * 100.0;
                while( !g_app->m_location->IsWalkable( pos, b->m_pos, true, false ) )
                {
                    pos = b->m_pos;
                    pos.x += iv_cos(syncfrand(M_PI*2.0)) * 100.0;
                    pos.z += iv_sin(syncfrand(M_PI*2.0)) * 100.0;
                }
            }
            AISetConversionPoint( pos );
        }
    }
}

void Armour::RunRocketRiotAI( AI *_ai )
{
    bool dontLoad = PreventLoadOverlap();

	m_aiTimer -= SERVER_ADVANCE_PERIOD;
    if( m_aiTimer > 0.0 ) return;
    m_aiTimer = 1.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ) m_aiTimer = 20.0;
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard ) m_aiTimer = 10.0;

    LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_id.GetTeamId()];
    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

    bool findIdleDarwinians = false; // if true, the armour should collect as many darwinians as possible and bring them tot he rocket
    int limit = 20;
    if( rocket && rocket->m_state == EscapeRocket::StateLoading )
    {
        findIdleDarwinians = true;
        limit = 0;
    }

    if( m_numPassengers == 0 && m_speed == 0.0 )
    {
        // look for some darwinians to collect
        if( findIdleDarwinians )
        {
            FindIdleDarwinians();
        }
        else
        {
            // look for our trunk port, as this is likely to be the only place with new darwinians
            if( !dontLoad )
            {
                RRFindPickupPoint();
            }
        }
    }
    else if( m_numPassengers > limit && m_speed == 0.0 )
    {
        int numApproaching = CountApproachingDarwinians();
        // we've got passengers, so go somewhere and drop them off
        if( findIdleDarwinians )
        {
            if( numApproaching == 0 )
            {
                Vector3 pos = rocket->m_pos;
                if( g_app->m_multiwinia->m_aiType > Multiwinia::AITypeEasy )
                {
                    pos.x += iv_cos(syncfrand(M_PI*2.0)) * 100.0;
                    pos.z += iv_sin(syncfrand(M_PI*2.0)) * 100.0;
                }
                AISetConversionPoint( pos );
            }
        }
        else
        {
            // make sure we have all the new darwinians before leaving
            if( numApproaching == 0 )
            {
                RRFindDropoffPoint( _ai );
            }
        }
    }
}

void Armour::SetOrders( Vector3 const &_orders )
{
    if( !g_app->m_location->m_landscape.IsInLandscape( _orders ) )  return;
    m_newOrdersTimer = 0.0;

    double distance = ( _orders - m_pos ).Mag();


    if( distance > 50.0 || m_awaitingDeployment)
    {
        SetConversionPoint( _orders );
    }
    else
    {
        //static double lastOrdersSet = 0.0;
        /*double timeNow = GetNetworkTime();

        if( timeNow > m_lastOrdersSet + 0.3 && !m_awaitingDeployment )
        {
            ToggleLoading();
            m_lastOrdersSet = timeNow;
        }*/

        // Note by chris
        // We dont handle this here anymore
        // Instead we handle right-clicking on the unit in LocationInput::AdvanceTeamControl
        // Search for comment "This should toggle orders"
    }
}

void Armour::SetDirectOrders()
{
    static double lastOrdersSet = 0.0;
    double timeNow = GetNetworkTime();

    if( timeNow > lastOrdersSet + 0.3 )
    {
        ToggleLoading();
        lastOrdersSet = timeNow;
    }
}



void Armour::SetWayPoint( Vector3 const &_wayPoint )
{
    if( !g_app->m_location->m_landscape.IsInLandscape( _wayPoint ) )  return;

    m_conversionPoint.Zero();
    m_conversionOrder = StateIdle;

    m_wayPoint = _wayPoint;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );

    if( m_state == StateLoading ) 
    {
        m_state = StateIdle;
    }
}


void Armour::SetConversionPoint ( Vector3 const &_conversionPoint )
{
    m_conversionPoint.Zero();
    
    //Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( _conversionPoint.x, _conversionPoint.z );
    //if( landNormal.y > 0.95 || g_app->Multiplayer() )
    {
        SetWayPoint( _conversionPoint );

        m_conversionPoint = _conversionPoint;
        m_conversionPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_conversionPoint.x, m_conversionPoint.z );

        m_state = StateIdle;

        if( m_numPassengers > 0 )
        {
            m_conversionOrder = StateUnloading;
        }
        else
        {
            m_conversionOrder = StateLoading;
        }    
    }
}

void Armour::AISetConversionPoint(Vector3 const &_pos)
{
    Vector3 posPushAway = PushFromObstructions( _pos );

    m_conversionPoint.Zero();
    m_conversionPoint = posPushAway;
    m_conversionPoint += Vector3( SFRAND(10.0), 0.0, SFRAND(10.0) );
    m_conversionPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_conversionPoint.x, m_conversionPoint.z );
    m_state = StateIdle;

    if( m_numPassengers > 0 )
    {
        m_conversionOrder = StateUnloading;
    }
    else
    {
        m_conversionOrder = StateLoading;
    }    

    CreateRoute(posPushAway);
}


bool Armour::IsLoading()
{
    return( m_state == StateLoading && 
            m_numPassengers < Capacity() &&
            m_newOrdersTimer > 1.0 );
}


bool Armour::IsUnloading()
{
    return( m_state == StateUnloading && 
            m_numPassengers > 0 &&
            m_newOrdersTimer > 1.0 &&
            GetNetworkTime() >= m_previousUnloadTimer + ARMOUR_UNLOADPERIOD );
}


int Armour::Capacity()
{
    if( g_app->Multiplayer() ) return 100;

    int research = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeArmour );
    switch( research )
    {
        case 0:
        case 1:     return 10;
        case 2:     return 20;
        case 3:     return 30;
        case 4:     return 40;
    }

    return 10;
}

int Armour::GetNumPassengers()
{
    return m_numPassengers + m_passengerOverflow;
}

double Armour::GetSpeed()
{
    return m_speed;
}

bool Armour::IsSelectable()
{
    return true;
}

void Armour::ToggleLoading()
{
    if( g_app->Multiplayer() )
    {
        if( m_state == StateIdle )
        {
            if( m_numPassengers > 0 )   m_state = StateUnloading;
            else                        m_state = StateLoading;
        }
        else if( m_state == StateUnloading ) 
        {
            m_state = StateLoading;
        }
        else if( m_state == StateLoading )
        {
            m_state = StateUnloading;            
        }

        m_conversionOrder = StateIdle;
    }
    else
    {
        switch( m_state )
        {
            case StateIdle:
                if( m_numPassengers > 0 )   m_state = StateUnloading;
                else                        m_state = StateLoading;
                break;

            case StateUnloading:            m_state = StateLoading;
                break;

            case StateLoading:              m_state = StateIdle;
                break;
        }
    }
}


void Armour::ToggleConversionAction()
{
    bool passengers = ( m_numPassengers > 0 );
    bool canLoad = ( m_numPassengers < Capacity() );
    bool canUnload = ( m_numPassengers > 0 );

    switch( m_conversionOrder )
    {
        case StateIdle:   
            if( canUnload ) m_conversionOrder = StateUnloading;         
            else            m_conversionOrder = StateLoading;
            break;

        case StateLoading:                  
            m_conversionOrder = StateIdle;              
            break;

        case StateUnloading:    
            if( canLoad )   m_conversionOrder = StateLoading;           
            else            m_conversionOrder = StateIdle;
            break;
    }

    if( m_conversionOrder == StateIdle )
    {
        m_conversionPoint = g_zeroVector;
    }
    else
    {
        m_conversionPoint = m_wayPoint;
        m_state = StateIdle;
    }
}


void Armour::AddPassenger()
{
    RegisterPassenger();
    m_passengerOverflow++;

    g_app->m_soundSystem->TriggerEntityEvent( this, "LoadDarwinian" );
}


void Armour::RemovePassenger()
{
    --m_numPassengers;
    m_previousUnloadTimer = GetNetworkTime();

    g_app->m_soundSystem->TriggerEntityEvent( this, "UnloadDarwinian" );
}

void Armour::GetEntrance( Vector3 &_exitPos, Vector3 &_exitDir )
{
    Matrix34 mat( m_front, m_up, m_pos );
    Matrix34 entranceMat = m_markerEntrance->GetWorldMatrix(mat);
    _exitPos = entranceMat.pos;
    _exitDir = entranceMat.f;
}


void Armour::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "LoadDarwinian" );
    _list->PutData( "UnloadDarwinian" );
    _list->PutData( "SetOrders" );
}



/*
void Armour::SetMissileTarget( Vector3 const &_startRay, Vector3 const &_rayDir )
{
    //
    // Look for enemy entities

    WorldObjectId id = g_app->m_location->GetEntityId( _startRay, _rayDir, 1 );
    Entity *entity = g_app->m_location->GetEntity( id );
    if( entity )
    {
        m_missileTarget = entity->m_pos+entity->m_centrePos;
        return;
    }


    //
    // Hit against the landscape

    Vector3 landscapeHit;
    bool hit = g_app->m_location->m_landscape.RayHit( _startRay, _rayDir, &landscapeHit );
    if( hit )
    {
        m_missileTarget = landscapeHit;
        return;
    }
}


Vector3 Armour::GetMissileTarget()
{
    return m_missileTarget;
}


void Armour::LaunchMissile()
{
    Missile *missile = new Missile();
    missile->m_pos = m_pos + Vector3(0,30,0);    
    
    Vector3 front = (m_front + m_up).Normalise();
    Vector3 right = front ^ g_upVector;
    Vector3 up = (right ^ front).Normalise();
    front = (up ^ right).Normalise();
    missile->m_front = front;
    missile->m_up = up;
    missile->m_vel = front * 200.0;
    missile->m_tankId = m_id;
    missile->m_target = m_missileTarget;
    g_app->m_location->m_effects.PutData( missile );
}*/



void Armour::Render( double _predictionTime )
{       
    if( m_dead ) return;

//#ifdef DEBUG_RENDER_ENABLED
//    glDisable( GL_DEPTH_TEST );
//    RenderArrow( m_pos+Vector3(0,10,0), m_wayPoint, 1.0, RGBAColour(255,255,255,155) );
//    if( m_attackTarget != g_zeroVector )
//    {
//        RenderArrow( m_pos+Vector3(0,10,0), m_attackTarget, 1.0, RGBAColour(255,50,50,255) );
//    }
//    glEnable( GL_DEPTH_TEST );
//#endif
    
    //RenderArrow( m_pos, m_wayPoint, 1.0, RGBAColour(255,255,255,155) );
    //RenderArrow( m_pos, m_pos + m_front * 100, 1.0f, RGBAColour(255,0,0,255) );

    //
    // Work out our predicted position

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    //predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    //predictedPos.y = max( predictedPos.y, 0.0 );
    double gameTime = g_gameTimer.GetGameTime();
    predictedPos.y += sinf(gameTime + m_id.GetUniqueId() ) * 2;
    Vector3 predictedUp = m_up;         //g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    predictedUp.x += sinf( (gameTime + m_id.GetUniqueId() ) * 2 ) * 0.05;
    predictedUp.z += cosf( gameTime + m_id.GetUniqueId() ) * 0.05;
    
    Vector3 predictedFront = m_front;
    Vector3 right = predictedUp ^ predictedFront;
    predictedUp.Normalise();
    predictedFront = right ^ predictedUp;
    predictedFront.Normalise();
    
    //
    // Smooth out those vectors to make it look nicer

    double smoothFactor = g_advanceTime * 4;

    if( m_smoothFront == g_zeroVector ) m_smoothFront = predictedFront;
    if( m_smoothUp == g_zeroVector ) m_smoothUp = predictedUp;
    if( m_smoothYpos == 0.0 ) m_smoothYpos = predictedPos.y;

    m_smoothFront = ( m_smoothFront * (1-smoothFactor) ) + predictedFront * smoothFactor;
    m_smoothUp =    ( m_smoothUp    * (1-smoothFactor) ) + predictedUp    * smoothFactor;
    m_smoothYpos =  ( m_smoothYpos  * (1-smoothFactor) ) + predictedPos.y * smoothFactor;

    m_smoothFront.Normalise();
    m_smoothUp.Normalise();
    
    predictedPos.y = m_smoothYpos;


    //
    // Render the tank body

    Matrix34 bodyMat(m_smoothFront, m_smoothUp, predictedPos );

    if( m_renderDamaged )
    {
        glBlendFunc( GL_ONE, GL_ONE );
        glEnable( GL_BLEND );

        if( frand() > 0.5 )
        {
            bodyMat.r *= ( 1.0 + sinf(g_gameTime) * 0.5 );
        }
        else
        {
            bodyMat.f *= ( 1.0 + sinf(g_gameTime) * 0.5 );
        }
    }

    g_app->m_renderer->SetObjectLighting();    
    m_shape->Render( _predictionTime, bodyMat );
    g_app->m_renderer->UnsetObjectLighting();

    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    
    
    //
    // Render the flag

    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;

    /*    
    Matrix34 flagMat = m_markerFlag->GetWorldMatrix(bodyMat);
    m_flag.SetPosition( flagMat.pos );    
    m_flag.SetOrientation( predictedFront * -1, predictedUp );
    m_flag.SetSize( 20.0 );

    char filename[256];
    switch( m_state )
    {
        case StateIdle :        sprintf( filename, "icons/banner_none.bmp" );              break;
        case StateUnloading :   sprintf( filename, "icons/banner_unload.bmp" );            break;
        case StateLoading :     sprintf( filename, "icons/banner_follow.bmp" );            break;
    }

    m_flag.SetTexture( g_app->m_resource->GetTexture( filename ) );
    m_flag.Render( g_app->Multiplayer() ? m_id.GetTeamId() : -1, filename );
    
    if( m_numPassengers > 0 )
    {
        char caption[16];
        sprintf( caption, "%d", m_numPassengers );
        m_flag.RenderText( 2, 2, caption );
    }
    */


    //
    // If we are about to deploy, render a flag at the target
    // Note by Chris : now replaced with Marker system

    /*if( m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
    {
        if( m_conversionPoint != g_zeroVector )
        {
            Vector3 front( 0, 0, -1 );
            front.RotateAroundY( g_gameTime * 0.5 );
            Vector3 up = g_upVector;
            up.RotateAround( front * sinf(timeIndex*3) * 0.3 );
            
            m_deployFlag.SetPosition( m_conversionPoint );
            m_deployFlag.SetOrientation( front, up );
            m_deployFlag.SetSize( 30.0 );

            if( m_conversionOrder == StateUnloading )
            {
                m_deployFlag.Render( m_id.GetTeamId(), "icons/banner_unload.bmp" );
            }
            else
            {
                m_deployFlag.Render( m_id.GetTeamId(), "icons/banner_follow.bmp" );
            }
        }
    }*/
}

bool Armour::IsRouteClear(Vector3 const &_pos)
{
	double jump = 20.0;
	Vector3 dir = ( _pos - m_pos ).Normalise();
	int steps = (( _pos - m_pos ).Mag() / jump) + 1;
	Vector3 pos = m_pos;
	for( int i = 0; i < steps; ++i )
	{
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
		LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( pos.x, pos.z );

        for( int b = 0; b < buildings->Size(); ++b )
		{
			Building *building = g_app->m_location->GetBuilding( buildings->GetData(b) );

            if( building && 
                building->m_type == Building::TypeLaserFence &&
                !g_app->m_location->IsFriend( building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                LaserFence *fence = (LaserFence *)building;
                
                if( fence->IsEnabled() &&
                    building->DoesRayHit(pos, dir, jump ) )
			    {
					return false;
				}
			}
		}

        pos += jump * dir;
	}

	return true;
}

bool Armour::IsRouteClear( Vector3 const &_fromPos, Vector3 const &_toPos )
{
	double jump = 20.0;
	Vector3 dir = ( _toPos - _fromPos ).Normalise();
	int steps = (( _toPos - _fromPos ).Mag() / jump) + 1;
	Vector3 pos = _fromPos;
	for( int i = 0; i < steps; ++i )
	{
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
		LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( pos.x, pos.z );
		for( int b = 0; b < buildings->Size(); ++b )
		{
			Building *building = g_app->m_location->GetBuilding( buildings->GetData(b) );
			if( building && building->m_type == Building::TypeLaserFence &&
                building->DoesRayHit(pos, dir, jump ) )
			{
				return false;
			}
		}
        pos += jump * dir;
	}

	return true;
}

void Armour::CreateRoute(Vector3 const &_pos)
{
	if( IsRouteClear( _pos ) )
	{
		m_wayPoint = _pos;
		return;
	}

    m_routeId = g_app->m_location->m_seaRoutingSystem.GenerateRoute( m_pos, _pos, false, true, true ); 
}

void Armour::FollowRoute()
{
    if( !g_app->Multiplayer() ) return;
    if(m_routeId == -1 )
    {
        return;
    }

    Route *route = g_app->m_location->m_seaRoutingSystem.GetRoute( m_routeId );
    if(!route)
    {
        m_routeId = -1;
        return;
    }
    g_app->m_location->m_seaRoutingSystem.SetRouteIntensity( m_routeId, 1.0 );

    bool waypointChange = false;

    if (m_routeWayPointId == -1)
    {
        m_routeWayPointId = 1;
        waypointChange = true;
    }
    else
    {
		WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId+1);
		if( waypoint )
		{
			if( IsRouteClear(waypoint->m_pos) )//|| (m_pos-waypoint->m_pos).Mag() < 5.0 )
			{
				++m_routeWayPointId;    
				waypointChange = true;
			}
		}
		else
		{
			++m_routeWayPointId;    
			waypointChange = true;
		}

    }


    if( waypointChange )
    {
        if (m_routeWayPointId >= route->m_wayPoints.Size())
        {
			g_app->m_location->m_seaRoutingSystem.DeleteRoute(m_routeId);
            m_routeWayPointId = -1;
            m_routeId = -1;
            return;
        }

        WayPoint *waypoint = route->m_wayPoints.GetData(m_routeWayPointId);
        if( !waypoint )
        {
			g_app->m_location->m_seaRoutingSystem.DeleteRoute(m_routeId);
            m_routeId = -1;
            return;
        }

        m_wayPoint = waypoint->GetPos();
        m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
    }   
}

void Armour::RegisterPassenger()
{
    m_registeredPassengers++;
}
    

bool Armour::RayHit( Vector3 const &rayStart, Vector3 const &rayDir )
{
    double gameTime = g_gameTimer.GetGameTime();

    Vector3 predictedPos = m_pos;
    predictedPos.y += iv_sin(gameTime + m_id.GetUniqueId() ) * 2;

    Vector3 predictedUp = m_up;
    predictedUp.x += iv_sin( (gameTime + m_id.GetUniqueId() ) * 2 ) * 0.05;
    predictedUp.z += iv_cos( gameTime + m_id.GetUniqueId() ) * 0.05;

    Vector3 predictedFront = m_front;
    Vector3 right = predictedUp ^ predictedFront;
    predictedUp.Normalise();
    predictedFront = right ^ predictedUp;
    predictedFront.Normalise();

    Matrix34 mat( predictedFront, predictedUp, predictedPos );                    
    RayPackage rayPackage( rayStart, rayDir);
    bool rayHit = m_shape->RayHit( &rayPackage, mat, true );

    return rayHit;
}

