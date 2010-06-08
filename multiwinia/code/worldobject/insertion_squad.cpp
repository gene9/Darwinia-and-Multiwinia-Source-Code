#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"

#include "limits.h"

#include "lib/bitmap.h"
#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/rgb_colour.h"

#include "lib/input/input.h"
#include "lib/input/input_types.h"

#include "app.h"
#include "camera.h"
#include "explosion.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "user_input.h"
#include "taskmanager.h"
#include "routing_system.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "particle_system.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "main.h"
#include "gamecursor.h"

#include "global_world.h"

#include "sound/soundsystem.h"

#include "worldobject/ai.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/teleport.h"


unsigned int HistoricWayPoint::s_lastId = 0;

static std::vector<WorldObjectId> s_neighbours;


//*****************************************************************************
// Class InsertionSquad
//*****************************************************************************

InsertionSquad::InsertionSquad( int teamId, int unitId, int numEntities, Vector3 const &_pos )
:   Unit( Entity::TypeInsertionSquadie, teamId, unitId, numEntities, _pos ),
    m_weaponType(GlobalResearch::TypeGrenade),
    m_controllerId(-1),
    m_teleportId(-1),
    m_numControlled(0),
    m_numControlledThisFrame(0)
{
	SetWayPoint(g_zeroVector);
	SetWayPoint(_pos);
}


InsertionSquad::~InsertionSquad()
{
	m_positionHistory.EmptyAndDelete();
}


bool InsertionSquad::Advance( int _slice )
{
    if( !g_app->Multiplayer() )
    {
        m_numControlled = m_numControlledThisFrame;
        m_numControlledThisFrame = 0;
    }

    return Unit::Advance(_slice);
}


Entity *InsertionSquad::GetPointMan()
{
	for (int i = 0; i < m_entities.Size(); ++i)
	{
		if (m_entities.ValidIndex(i))
		{
			Entity *entity = m_entities.GetData(i);
			if (entity->m_formationIndex == 0)
			{
				return entity;
			}
		}
	}

	return NULL;
}

/*Entity *InsertionSquad::GetFirstValidUnit()
{
	for (int i = 0; i < m_entities.Size(); ++i)
	{
		if (m_entities.ValidIndex(i))
		{
			Entity *entity = m_entities.GetData(i);
			if ( !entity->m_dead &&
				 entity->m_enabled )
			{
				return entity;
			}
		}
	}
	return NULL;
}
*/

void InsertionSquad::SetWayPoint(Vector3 const &_pos)
{    
	m_wayPoint = _pos;
    m_focusPos = _pos;

	Entity *pointMan = GetPointMan();

	// If we found the point man add his position to the position history
	// otherwise add the position that was passed in as an argument
	HistoricWayPoint *newWayPoint;
	if (pointMan && pointMan->m_enabled)
	{
		newWayPoint = new HistoricWayPoint(pointMan->m_pos);
	}
	else
	{
		newWayPoint = new HistoricWayPoint(_pos);
	}
	m_positionHistory.PutDataAtStart(newWayPoint);


    // If this squad is using a Controller, update the Route
    // that the Controller points to
    if( g_app->m_location->m_teams[m_teamId]->m_taskManager )
    {
        Task *controller = g_app->m_location->m_teams[m_teamId]->m_taskManager->GetTask( m_controllerId );
        if( controller ) 
        {
            Vector3 lastAddedPos = controller->m_route->m_wayPoints[ controller->m_route->m_wayPoints.Size()-1 ]->GetPos();
            double distance = ( lastAddedPos - newWayPoint->m_pos ).Mag();
            if( distance > 20.0 )
            {
                controller->m_route->AddWayPoint( newWayPoint->m_pos );
            }
        }
    }


    //
    // If we clicked near a teleport, tell the unit to go into it
    m_teleportId = -1;
    LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( _pos.x, _pos.z );
    for( int i = 0; i < nearbyBuildings->Size(); ++i )
    {
        int buildingId = nearbyBuildings->GetData(i);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building->m_type == Building::TypeRadarDish ||
            building->m_type == Building::TypeBridge )
        {
            Teleport *teleport = (Teleport *) building;
            double distance = ( building->m_pos - _pos ).Mag();
            if( distance < 5.0 && teleport->Connected() )
            {
                m_teleportId = building->m_id.GetUniqueId();
                Vector3 entrancePos, entranceFront;
                teleport->GetEntrance( entrancePos, entranceFront );
                m_wayPoint = entrancePos;
                break;
            }
        }
    }
}


// Returns a point on the path that the squad are walking along. The point
// man is the leader of the squad. He always heads towards m_wayPoint.
// When the user clicks to create a new m_wayPoint, the current pos of the
// leader is added to the list of historic wayPoints.
Vector3 InsertionSquad::GetTargetPos(double _distFromPointMan)
{
	if (_distFromPointMan <= 0.0)
	{
		return m_wayPoint;
	}

    if (m_teleportId != -1)
    {
        return m_wayPoint;
    }

	Entity *pointMan = GetPointMan();
    if( !pointMan ) return m_wayPoint;

	// The first section of the route is the line from the point man to
	// the most recent HistoricWayPoint.

	Vector3 lineEnd = pointMan->m_pos;
	
	int i = 0;
	while (i < m_positionHistory.Size() && _distFromPointMan > 0.0)
	{
		Vector3 lineStart = lineEnd;
		lineEnd = m_positionHistory[i]->m_pos;

		Vector3 delta = lineEnd - lineStart;
		double magDelta = delta.Mag();
		if (magDelta > _distFromPointMan)
		{
			delta.SetLength(_distFromPointMan);
			return lineStart + delta;
		}
		else
		{
			_distFromPointMan -= magDelta;
		}
		i++;
	}

	return lineEnd;
}

void InsertionSquad::CycleSecondary()
{
	int grenadeLevel	=	g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade ),
		rocketLevel		=	g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeRocket ),
		airStrikeLevel	=	g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeAirStrike ),
		controllerLevel	=	g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeController );

	switch( m_weaponType )
	{
	    case GlobalResearch::TypeGrenade:
			AppDebugOut("Current on weapon type grenade\n");
			m_weaponType =	rocketLevel > 0		?	GlobalResearch::TypeRocket :
							airStrikeLevel > 0	?	GlobalResearch::TypeAirStrike :
							controllerLevel > 0	?	GlobalResearch::TypeController :
													GlobalResearch::TypeGrenade;
			break;
        case GlobalResearch::TypeRocket:
			AppDebugOut("Current on weapon type rocket\n");
			m_weaponType =	airStrikeLevel > 0	?	GlobalResearch::TypeAirStrike :
							controllerLevel > 0	?	GlobalResearch::TypeController :
							grenadeLevel > 0	?	GlobalResearch::TypeGrenade :
													GlobalResearch::TypeRocket;
			break;
		case GlobalResearch::TypeAirStrike:
			AppDebugOut("Current on weapon type airstrike\n");
			m_weaponType =	controllerLevel > 0	?	GlobalResearch::TypeController :
							grenadeLevel > 0	?	GlobalResearch::TypeGrenade :
							rocketLevel > 0		?	GlobalResearch::TypeRocket :
													GlobalResearch::TypeAirStrike;
			break;
        case GlobalResearch::TypeController:
			AppDebugOut("Current on weapon type controller\n");
			m_weaponType =	grenadeLevel > 0	?	GlobalResearch::TypeGrenade :
							rocketLevel > 0		?	GlobalResearch::TypeRocket :
							airStrikeLevel > 0	?	GlobalResearch::TypeAirStrike :
													GlobalResearch::TypeController;
			break;
		default:
			AppDebugOut("Cycling secondary weapon on squad has caused poopers\n");
			m_weaponType = GlobalResearch::TypeGrenade;
			break;
	}
	AppDebugOut("Weapon type is now %d\n", m_weaponType);
}

void InsertionSquad::SetWeaponType( int _weaponType )
{
    m_weaponType = _weaponType;
}


void InsertionSquad::Attack( Vector3 pos, bool withGrenade )
{    
    m_focusPos = pos;

	// 
	// Deal with grenades

	if (withGrenade)
	{
        double nearest = 999999.9;
        Squadie *nearestEnt = NULL;

        //
        // Find the entity nearest to the target that has a grenade

        for( int i = 0; i < m_entities.Size(); ++i )
        {
            if( m_entities.ValidIndex(i) )
            {
                Squadie *ent = (Squadie *) m_entities[i];
                if( !ent->m_dead && ent->m_enabled && ent->HasSecondaryWeapon() )
                {
                    double distance = (ent->m_pos - pos).Mag();
                    if( distance < nearest )
                    {
                        nearest = distance;
                        nearestEnt = ent;
                    }
                }
            }
        }
        
        if( nearestEnt )
        {
            nearestEnt->FireSecondaryWeapon( pos );
        }
	}

    
    //
    // Build a list of squadies that can attack now

    LList<int> canAttack;
    for( int i = 0; i < m_entities.Size(); ++i )
    {
        if( m_entities.ValidIndex(i) )
        {
            Squadie *ent = (Squadie *) m_entities[i];
            if( ent->m_enabled &&
                !ent->m_dead && 
                ent->m_reloading == 0.0 )
            {
                canAttack.PutData( i );
            }
        }
    }

    
    if( canAttack.Size() > 0 )
    {
        //
        // Decide the maximum number of entities 
        // that can attack now without pauses appearing in fire rate

        double reloadTime = EntityBlueprint::GetStat( m_troopType, Entity::StatRate );
        double timeToWait = (double) reloadTime / (double) canAttack.Size();
        m_attackAccumulator += ( (double) SERVER_ADVANCE_PERIOD / timeToWait );
        

        //
        // Pick guys randomly to attack

        while( canAttack.Size() > 0 && m_attackAccumulator >= 1.0 )
        {
            m_attackAccumulator -= 1.0;
            int randomIndex = syncfrand(canAttack.Size());
            int entityIndex = canAttack[randomIndex];
            canAttack.RemoveData(randomIndex);
            Entity *ent = m_entities[entityIndex];
    		ent->Attack( pos );	
        }
    }
}

void InsertionSquad::DirectControl( TeamControls const& _teamControls )
{
    m_focusPos = m_centrePos;

	if( !_teamControls.m_cameraEntityTracking && !g_app->m_camera->m_camLookAround ) 
	{
		return;
	}

	Entity *point = GetPointMan();
	if( !point || point->m_type != Entity::TypeInsertionSquadie )
	{
		return;
	}
	Squadie *pointMan = (Squadie *)point;

	Vector3 right = g_app->m_camera->GetControlVector();
	Vector3 front = g_upVector ^ -right;

	if( _teamControls.m_leftButtonPressed ||
		(_teamControls.m_consoleController && _teamControls.m_directUnitMove) )
    {
        Vector3 t = pointMan->m_pos;
        
		t.x+= _teamControls.m_directUnitMoveDx;
		t.z+= _teamControls.m_directUnitMoveDy;
        //t+= front * - _teamControls.m_directUnitMoveDy;

        LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( t.x, t.z );
        for( int i = 0; i < nearbyBuildings->Size(); ++i )
        {
            int buildingId = nearbyBuildings->GetData(i);
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building->m_type == Building::TypeRadarDish ||
                building->m_type == Building::TypeBridge )
            {
                Teleport *teleport = (Teleport *) building;
                if( teleport->Connected() )
                {
                    t = building->m_pos;
                }
            }
        }

        SetWayPoint( t );
    }
    else
    {
        if( m_vel.Mag() > 0 )
        {
            SetWayPoint( pointMan->m_pos );
        }
    }

	if( _teamControls.m_primaryFireTarget  )
	{
		Vector3 t = m_lastTarget;
		
		if( _teamControls.m_targetDirected )
		{
			/*int acceleration = 6;
			Vector3 delta(_teamControls.m_directUnitTargetDx, 0, _teamControls.m_directUnitTargetDy);

			if( delta.MagSquared() > 175 )
			{
				acceleration = 256;
			}

			delta.x *= acceleration;
			delta.z *= acceleration;
			g_app->m_camera->m_wantedTargetVector = g_app->m_camera->m_targetVector + delta;
			if( g_app->m_camera->m_wantedTargetVector.MagSquared() > 15625 )
			{
				g_app->m_camera->m_wantedTargetVector.SetLength(125);
			}

			//AppDebugOut("***** %d , %d\n", g_app->m_camera->m_targetVector.x, g_app->m_camera->m_targetVector.y);
			t = pointMan->m_pos + g_app->m_camera->m_targetVector;*/
			t = Vector3(_teamControls.m_directUnitTargetDx, 0, _teamControls.m_directUnitTargetDy);
			m_lastTarget = t;
		}
		else
		{
			//t = pointMan->m_pos+(pointMan->m_pos - g_app->m_camera->m_pos);
		}

        Attack(t, _teamControls.m_secondaryFireTarget );
	}
	else
	{
		g_app->m_camera->m_targetVector = g_zeroVector;
		g_app->m_camera->m_wantedTargetVector = g_zeroVector;
	}
}

bool InsertionSquad::IsSelectable()
{
    return true;
}

void InsertionSquad::RunAI(AI *_ai)
{
    m_aiTimer -= SERVER_ADVANCE_PERIOD;

    double force = ThrowableWeapon::GetMaxForce( g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade ) );
    double maxRange = ThrowableWeapon::GetApproxMaxRange( force );

    if( m_aiTimer <= 0.0 )
    {
        m_aiTimer = 1.0;

        // the squadie should check the surrounding area for concentrations of enemy darwinians
        // if it finds some, it should head towards them and start firing
        // it not, it should find the highest priority node it can find, and head towards it until it gets into range of any enemies

        int numFound;
        g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_centrePos.x, m_centrePos.z, 500.0, &numFound, m_teamId );

        Vector3 pos;
        double dist = FLT_MAX;
        for( int i = 0; i < numFound; ++i )
        {
            Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
            if(e)
            {
                if( (e->m_pos - m_centrePos).Mag() < dist )
                {
                    dist = (e->m_pos - m_centrePos).Mag();
                    int localnumfound = g_app->m_location->m_entityGrid->GetNumEnemies( e->m_pos.x, e->m_pos.z, 75.0, m_teamId );
                    if( localnumfound >= 20 )
                    {
                        // this is a good spot, go here
                        pos = e->m_pos;
                        break;
                    }
                }
            }
        }

        if( pos != g_zeroVector )
        {
            Vector3 dir = (pos - m_centrePos).Normalise();

            Vector3 targetPos = pos - ( 0.8 * (dir * maxRange));
            SetWayPoint(targetPos);
        }
        else
        {
            // no targets nearby, so query the ai grid and find the best place to go
            AITarget *t = (AITarget *)g_app->m_location->GetBuilding(_ai->FindNearestTarget( m_centrePos ));
            if( t && t->m_type == Building::TypeAITarget )
            {
                double priority = 0.0;
                int id = -1;
                for( int i = 0; i < t->m_neighbours.Size(); ++i )
                {
                    AITarget *target = (AITarget *)g_app->m_location->GetBuilding( t->m_neighbours[i]->m_neighbourId );
                    if( target )
                    {
                        if( g_app->m_location->IsWalkable( m_centrePos, target->m_pos, true ) )
                        {
                            if( target->m_priority[m_teamId] > priority )
                            {
                                priority = target->m_priority[m_teamId];
                                id = t->m_neighbours[i]->m_neighbourId;
                            }
                        }
                    }
                }

                if( id != -1 )
                {
                    AITarget *target = (AITarget *)g_app->m_location->GetBuilding( id );
                    SetWayPoint( target->m_pos );
                }
                else
                {
                    // cant get to any of those nodes, so just head for this one and hope the situation improves
                    SetWayPoint( t->m_pos );
                }
            }
        }
    }

    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_centrePos.x, m_centrePos.z, maxRange, &numFound, m_teamId );

    if( numFound > 0 )
    {
        Vector3 pos;
        for( int i = 0; i < numFound; ++i )
        {
            Entity *e = g_app->m_location->GetEntity( s_neighbours[i] );
            if( e )
            {
                pos += e->m_pos;
            }
        }
        pos /= numFound;
        pos.x += syncsfrand(30.0);
        pos.z += syncsfrand(30.0);
        if( (m_centrePos - pos).Mag() <= maxRange )
        {
            Attack(pos, syncrand() %5 == 0 );
        }
    }
}

//****************************************************************************
// Class Squadie
//****************************************************************************

Squadie::Squadie()
:   Entity(),
    m_justFired(false),
    m_secondaryTimer(0.0),
    m_retargetTimer(0.0)
{
    m_shape = g_app->m_resource->GetShape( "squad.shp" );
    AppDebugAssert( m_shape );

	m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centrePos );

    const char laserName[] = "MarkerLaser";
    m_laser = m_shape->m_rootFragment->LookupMarker( laserName );
    AppReleaseAssert( m_laser, "Squadie: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", laserName, m_shape->m_name );

    const char brassName[] = "MarkerBrass";
    m_brass = m_shape->m_rootFragment->LookupMarker( brassName );
    AppReleaseAssert( m_brass, "Squadie: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", brassName, m_shape->m_name );

/*
    const char eye1Name[] = "MarkerEye1";
    m_eye1 = m_shape->m_rootFragment->LookupMarker( eye1Name );
    AppReleaseAssert( m_eye1, "Squadie: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", eye1Name, m_shape->m_name );

    const char eye2Name[] = "MarkerEye2";
    m_eye2 = m_shape->m_rootFragment->LookupMarker( eye2Name );
    AppReleaseAssert( m_eye2, "Squadie: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", eye2Name, m_shape->m_name );
*/
}


void Squadie::Begin()
{
    Entity::Begin();
}


bool Squadie::ChangeHealth( int _amount, int _damageType )
{
    bool dead = m_dead;
    
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
        Matrix34 transform( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, transform );         
    }

    return false;
}


bool Squadie::Advance(Unit *_theUnit)
{
	// AppDebugOut( "Squadie %d, Health: %d, Dead %d\n", m_formationIndex, m_stats[StatHealth], (int)m_dead );
    if( m_secondaryTimer > 0.0 )
    {
        m_secondaryTimer -= SERVER_ADVANCE_PERIOD;
        if( m_secondaryTimer <= 0.0 )
        {
            // Secondary weapon is reloaded
            g_app->m_soundSystem->TriggerEntityEvent( this, "WeaponReturns" );
        }
    }

    InsertionSquad *_unit = (InsertionSquad *) _theUnit;
    
    Vector3 oldPos = m_pos;
    
    if( !m_dead && m_onGround && m_inWater < 0.0 )
    {        
		Vector3 targetPos = _unit->GetTargetPos(GAP_BETWEEN_MEN * m_formationIndex);
        Vector3 targetVector = targetPos - m_pos;
        targetVector.y = 0.0;

        
		double distance = targetVector.Mag();
		
		// If moving towards way point...
		if (distance > 0.01 || _unit->m_teleportId != -1 )
		{
			m_vel = targetVector;
			m_vel *= m_stats[StatSpeed] / distance;

            //
            // Slow us down if we're going up hill
            // Speed up if going down hill

            Vector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
            nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
            double currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
            double nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );
            Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
            double factor = 1.0 - (currentHeight - nextHeight) / -3.0;
            if( factor < 0.2 ) factor = 0.2;
            if( factor > 2.0 ) factor = 2.0;
            if( landNormal.y < 0.6 && factor < 1.0 ) factor = 0.0;
            m_vel *= factor;

			if (distance < m_stats[StatSpeed] * SERVER_ADVANCE_PERIOD)
			{                    
				m_vel.SetLength(distance / SERVER_ADVANCE_PERIOD);
			}
            m_pos += m_vel * SERVER_ADVANCE_PERIOD;
            m_pos = PushFromObstructions( m_pos );
			m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 0.1;
		}

    	Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
		if (m_id.GetUnitId() == team->m_currentUnitId)
		{            
			Vector3 toMouse = team->m_currentMousePos - m_pos;
			toMouse.HorizontalAndNormalise();
			m_angVel = (m_front ^ toMouse) * 4.0;
		}
		else
		{
            Vector3 vel = m_vel;            
            if( vel.Mag() > 0.0 )
            {
                m_angVel = (m_front ^ vel.Normalise()) * 4.0;
            }            
            else
            {			    
                m_angVel = m_front;
            }

            RunAI();

            Entity *enemy = g_app->m_location->GetEntity( m_enemyId );
            if( enemy )
            {
                m_angVel = ((m_pos - enemy->m_pos).Normalise() ^ m_front) * 4.0;
            }
		}

        m_angVel.x = 0.0;
        m_angVel.z = 0.0;
		double const maxTurnRate = 20.0; // radians per second
		if (m_angVel.Mag() > maxTurnRate) 
		{
			m_angVel.SetLength(maxTurnRate);
		}
		m_front.RotateAround(m_angVel * SERVER_ADVANCE_PERIOD);
		m_front.Normalise();
        
        if( _unit->m_teleportId != -1 )
        {
            m_front = (targetPos - m_pos);
            m_front.y = 0.0;
            m_front.Normalise();
        }
        
        if( m_pos.y <= 0.0 )
        {
            m_inWater = syncfrand(3.0);
        }
    }
    
	if( !m_onGround ) AdvanceInAir(NULL);
        
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    if( _unit->m_teleportId != -1 )
    {
        if( EnterTeleports(_unit->m_teleportId) != -1 )
        {            
            return true;    
        }
    }

    bool manDead = Entity::Advance( _unit );
	if (m_dead)
	{
		_unit->RecalculateOffsets();
	}
	return manDead;
}


void Squadie::Render( double _predictionTime )
{    
    if( !m_enabled ) return;

    
    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_onGround )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }

    double size = 0.5;

    Vector3 entityUp = g_upVector;          //g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    Vector3 entityFront = m_front;
    entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;
    
    if( !m_dead )
    {       
        //
        // 3d Shape

		g_app->m_renderer->SetObjectLighting();
        glDisable       (GL_TEXTURE_2D);
        glEnable        (GL_COLOR_MATERIAL);
        glDisable       (GL_BLEND);

        Matrix34 mat(entityFront, entityUp, predictedPos);

        //
        // If we are damaged, flicked in and out based on our health

        if( m_renderDamaged )
        {
            double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
            double thefrand = frand();
            if      ( thefrand > 0.7 ) mat.f *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
            else if ( thefrand > 0.4 ) mat.u *= ( 1.0 - iv_sin(timeIndex) * 0.2 );    
            else                        mat.r *= ( 1.0 - iv_sin(timeIndex) * 0.5 );
            glEnable( GL_BLEND );
            glBlendFunc( GL_ONE, GL_ONE );
        }
        
        
        m_shape->Render(_predictionTime, mat);

        glEnable        (GL_BLEND);
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       (GL_COLOR_MATERIAL);
        glEnable        (GL_TEXTURE_2D);
		g_app->m_renderer->UnsetObjectLighting();


        InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( m_id );
        AppDebugAssert(squad);

		/*
		// Apparently this is not longer needed for anything. And as it displays a zero above the squaddies 
		// in Darwinia, IT GOES!
        if( squad->GetPointMan() == this )
        {
            g_gameFont.DrawText3DCentre( predictedPos + g_upVector * 100, 10, "%d", squad->m_numControlled );
        }
		*/
    }    
}


bool Squadie::RenderPixelEffect( double _predictionTime )
{
    if( !m_enabled ) return false;
    if( g_app->Multiplayer() ) return false;

    Render( _predictionTime );

    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_onGround )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }

    double size = 0.5;

    Vector3 entityUp = g_upVector;          //g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    Vector3 entityFront = m_front;
    entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    Matrix34 mat(entityFront, entityUp, predictedPos);
    g_app->m_renderer->MarkUsedCells(m_shape, mat);

    return true;
}


void Squadie::RunAI()
{
    //
    // Look for a new enemy every now and then

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;

    if( m_retargetTimer <= 0.0 )
    {
        m_enemyId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0, 100.0, m_id.GetTeamId() );
        m_retargetTimer = 1.0;
    }


    //
    // Fire at the nearest enemy now and then
      
    Entity *enemy = g_app->m_location->GetEntity( m_enemyId );;
    if( enemy )
    {
        double distance = (enemy->m_pos - m_pos).Mag();
        if( syncfrand(distance*0.5) < 2.0 )
        {
            Attack( enemy->m_pos );
        }
    }
}


void Squadie::Attack( Vector3 const &_pos )
{
    if( m_reloading == 0.0 )
    {
        //
        // Fire laser

        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 laserPos = m_laser->GetWorldMatrix( mat ).pos;

        Vector3 fromPos = laserPos;
	    Vector3 toPos = _pos;
		toPos.x += syncsfrand(7.0);
		toPos.z += syncsfrand(7.0);
	    
		Vector3 velocity = (toPos - fromPos).SetLength(200.0);
        g_app->m_location->FireLaser( fromPos, velocity, m_id.GetTeamId() );
        m_justFired = true;

        //
        // Create ejected brass particle

        Matrix34 brass = m_brass->GetWorldMatrix( mat );
        Vector3 particleVel = brass.f * ( 5.0 + syncfrand(10.0));
        particleVel += Vector3( SFRAND(5.0), SFRAND(5.0), SFRAND(5.0) );
        g_app->m_particleSystem->CreateParticle( brass.pos, particleVel, Particle::TypeBrass );


        //
        // 
        m_reloading = m_stats[StatRate];            
        g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
        g_app->m_soundSystem->TriggerEntityEvent( this, "FireLaser" );
    }    
}


bool Squadie::HasSecondaryWeapon()
{
    return ( m_secondaryTimer <= 0.0 );
}


void Squadie::FireSecondaryWeapon( Vector3 const &_pos )
{
    InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( m_id );
    AppDebugAssert(squad);
    
    if( g_app->m_globalWorld->m_research->HasResearch( squad->m_weaponType ) )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 laserPos = m_laser->GetWorldMatrix( mat ).pos;

        switch( squad->m_weaponType )
        {
            case GlobalResearch::TypeGrenade:            
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowGrenade" );
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadThrowGrenade );
#endif
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableGrenade, m_id.GetTeamId() );
                m_secondaryTimer = 4.0;
                break;

            case GlobalResearch::TypeAirStrike:            
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowAirStrike" );
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableAirstrikeMarker, m_id.GetTeamId() );
                m_secondaryTimer = 20.0;
                break;

            case GlobalResearch::TypeController:            
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowController" );
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableControllerGrenade, m_id.GetTeamId() );
                m_secondaryTimer = 4.0;
                break;

            case GlobalResearch::TypeRocket:    
                g_app->m_soundSystem->TriggerEntityEvent( this, "FireRocket" );
                g_app->m_location->FireRocket( laserPos, _pos, m_id.GetTeamId() );
                m_secondaryTimer = 4.0;
                break;
        }
    }
}


void Squadie::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "FireLaser" );
    _list->PutData( "ThrowGrenade" );
    _list->PutData( "FireRocket" );
    _list->PutData( "ThrowAirStrike" );
    _list->PutData( "ThrowController" );
    _list->PutData( "WeaponReturns" );
}

Vector3 Squadie::GetCameraFocusPoint()
{
	/*if( g_inputManager->controlEvent( ControlUnitPrimaryFireDirected /* ControlUnitStartSecondaryFireDirected * ) &&
        g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) )
	{
		InputDetails details;
        g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

		Vector3 t = GetSecondaryWeaponTarget();

		return ( t + m_pos ) / 2;
	}*/
	
	return Entity::GetCameraFocusPoint();
}

Vector3 Squadie::GetSecondaryWeaponTarget()
{
    Vector3 t = m_pos;

	/*InputDetails details;
    g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

	Vector3 front = (m_pos - g_app->m_camera->GetPos() );
	front.y = 0.0;
	front.Normalise();
	Vector3 right = front;
	right.RotateAroundY( M_PI / 2.0 );

	double force = ThrowableWeapon::GetMaxForce( g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade ) );
	double maxRange = ThrowableWeapon::GetApproxMaxRange( force );

    const double rangeFactor = 0.75;

    double rMod = (rangeFactor * maxRange * double(double(details.x)/40.0));
    double fMod = (rangeFactor * maxRange * double(double(details.y)/40.0));
	t+= right * - rMod;
	t+= front * - fMod;
	t.y = g_app->m_location->m_landscape.m_heightMap->GetValue( t.x, t.z )+5.0;*/

	return t;
}

Vector3 Squadie::GetSecondaryWeaponTarget(TeamControls _teamControls)
{
    Vector3 t = m_pos;
	t += (m_front-m_pos)*5;
	return t;

	/*InputDetails details;
    g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

	Vector3 front = (m_pos - g_app->m_camera->GetPos() );
	front.y = 0.0;
	front.Normalise();
	Vector3 right = front;
	right.RotateAroundY( M_PI / 2.0 );*

	double force = ThrowableWeapon::GetMaxForce( g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeGrenade ) );
	double maxRange = ThrowableWeapon::GetApproxMaxRange( force );

	//double total = abs(details.x) + abs(details.y);

	/*char debugstring[512];
	double r = details.x * details.x + details.y * details.y;
	r = iv_sqrt(r);
	sprintf(debugstring, "x = %d, y = %d r = %f\n", details.x, details.y, r);
	AppDebugOut( debugstring );*

    // This should be dependent on distance of camera from units
    // Further away, rangeFactor = 1.0, closer rangeFactor closer to 0.66

    Vector3 range = Vector3(_teamControls.m_directUnitTargetDx, 0, _teamControls.m_directUnitTargetDy );
    range.SetLength( maxRange );

    /*const double rangeFactor = 0.75;

    double rMod = (rangeFactor * maxRange * double(double(_teamControls.m_directUnitFireDx)/40.0));
    double fMod = (rangeFactor * maxRange * double(double(_teamControls.m_directUnitFireDy)/40.0));
	t+= right * - rMod;
	t+= front * - fMod;*
    t += range;
	t.y = g_app->m_location->m_landscape.m_heightMap->GetValue( t.x, t.z )+5.0;

	return t;
	*/
}
