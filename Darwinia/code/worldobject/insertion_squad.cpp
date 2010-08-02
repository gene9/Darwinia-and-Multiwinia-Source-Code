#include "lib/universal_include.h"
#include "lib/hi_res_time.h"

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
#include "helpsystem.h"
#include "particle_system.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "main.h"
#include "gamecursor.h"

#include "global_world.h"

#include "sound/soundsystem.h"

#include "worldobject/insertion_squad.h"
#include "worldobject/teleport.h"


unsigned int HistoricWayPoint::s_lastId = 0;


//*****************************************************************************
// Class InsertionSquad
//*****************************************************************************

InsertionSquad::InsertionSquad( int teamId, int unitId, int numEntities, Vector3 const &_pos )
:   Unit( Entity::TypeInsertionSquadie, teamId, unitId, numEntities, _pos ),
    m_weaponType(GlobalResearch::TypeGrenade),
    m_controllerId(-1),
    m_teleportId(-1)
{
	SetWayPoint(g_zeroVector);
	SetWayPoint(_pos);
}


InsertionSquad::~InsertionSquad()
{
	m_positionHistory.EmptyAndDelete();
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
    Task *controller = g_app->m_taskManager->GetTask( m_controllerId );
    if( controller )
    {
        Vector3 lastAddedPos = controller->m_route->m_wayPoints[ controller->m_route->m_wayPoints.Size()-1 ]->GetPos();
        float distance = ( lastAddedPos - newWayPoint->m_pos ).Mag();
        if( distance > 20.0f )
        {
            controller->m_route->AddWayPoint( newWayPoint->m_pos );
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
            float distance = ( building->m_pos - _pos ).Mag();
            if( distance < 5.0f && teleport->Connected() )
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
Vector3 InsertionSquad::GetTargetPos(float _distFromPointMan)
{
	if (_distFromPointMan <= 0.0f)
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
	while (i < m_positionHistory.Size() && _distFromPointMan > 0.0f)
	{
		Vector3 lineStart = lineEnd;
		lineEnd = m_positionHistory[i]->m_pos;

		Vector3 delta = lineEnd - lineStart;
		float magDelta = delta.Mag();
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


void InsertionSquad::SetWeaponType( int _weaponType )
{
    m_weaponType = _weaponType;
}


void InsertionSquad::Attack( Vector3 pos, bool withGrenade )
{
	//
	// Deal with grenades

	if (withGrenade)
	{
        float nearest = 999999.9f;
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
                    float distance = (ent->m_pos - pos).Mag();
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
                ent->m_reloading == 0.0f )
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

        float reloadTime = EntityBlueprint::GetStat( m_troopType, Entity::StatRate );
        float timeToWait = (float) reloadTime / (float) canAttack.Size();
        m_attackAccumulator += ( (float) SERVER_ADVANCE_PERIOD / timeToWait );


        //
        // Pick guys randomly to attack

        while( canAttack.Size() > 0 && m_attackAccumulator >= 1.0f )
        {
            m_attackAccumulator -= 1.0f;
            int randomIndex = syncfrand(canAttack.Size());
            int entityIndex = canAttack[randomIndex];
            canAttack.RemoveData(randomIndex);
            Squadie *ent = (Squadie *)m_entities[entityIndex];
    		ent->Attack( pos );
        }
    }
}

void InsertionSquad::DirectControl( TeamControls const& _teamControls )
{
	if( !_teamControls.m_cameraEntityTracking )
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

	if( _teamControls.m_directUnitMove )
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

    if( _teamControls.m_primaryFireDirected )
    {
        Vector3 t = pointMan->GetSecondaryWeaponTarget();

        Attack(t, _teamControls.m_secondaryFireDirected );
    }
}


//****************************************************************************
// Class Squadie
//****************************************************************************

Squadie::Squadie()
:   Entity(),
    m_justFired(false),
    m_secondaryTimer(0.0f),
    m_retargetTimer(0.0f)
{
    m_shape = g_app->m_resource->GetShape( "squad.shp" );
    DarwiniaDebugAssert( m_shape );

	m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius(g_identityMatrix34, m_centrePos );

    m_laser = m_shape->m_rootFragment->LookupMarker( "MarkerLaser" );
    m_brass = m_shape->m_rootFragment->LookupMarker( "MarkerBrass" );
    m_eye1 = m_shape->m_rootFragment->LookupMarker( "MarkerEye1" );
    m_eye2 = m_shape->m_rootFragment->LookupMarker( "MarkerEye2" );
}


void Squadie::Begin()
{
    Entity::Begin();
}


void Squadie::ChangeHealth( int _amount )
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
}


bool Squadie::Advance(Unit *_theUnit)
{
	// DebugOut( "Squadie %d, Health: %d, Dead %d\n", m_formationIndex, m_stats[StatHealth], (int)m_dead );
    if( m_secondaryTimer > 0.0f )
    {
        m_secondaryTimer -= SERVER_ADVANCE_PERIOD;
        if( m_secondaryTimer <= 0.0f )
        {
            // Secondary weapon is reloaded
            g_app->m_soundSystem->TriggerEntityEvent( this, "WeaponReturns" );
        }
    }

    InsertionSquad *_unit = (InsertionSquad *) _theUnit;

    Vector3 oldPos = m_pos;

    if( !m_dead && m_onGround && m_inWater < 0.0f )
    {
		Vector3 targetPos = _unit->GetTargetPos(GAP_BETWEEN_MEN * m_formationIndex);
        Vector3 targetVector = targetPos - m_pos;
        targetVector.y = 0.0f;


		float distance = targetVector.Mag();

		// If moving towards way point...
		if (distance > 0.01f || _unit->m_teleportId != -1 )
		{
			m_vel = targetVector;
			m_vel *= m_stats[StatSpeed] / distance;

            //
            // Slow us down if we're going up hill
            // Speed up if going down hill

            Vector3 nextPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
            nextPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(nextPos.x, nextPos.z);
            float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
            float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( nextPos.x, nextPos.z );
            Vector3 landNormal = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
            float factor = 1.0f - (currentHeight - nextHeight) / -3.0f;
            if( factor < 0.2f ) factor = 0.2f;
            if( factor > 2.0f ) factor = 2.0f;
            if( landNormal.y < 0.6f && factor < 1.0f ) factor = 0.0f;
            m_vel *= factor;

			if (distance < m_stats[StatSpeed] * SERVER_ADVANCE_PERIOD)
			{
				m_vel.SetLength(distance / SERVER_ADVANCE_PERIOD);
			}
            m_pos += m_vel * SERVER_ADVANCE_PERIOD;
            m_pos = PushFromObstructions( m_pos );
			m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) + 0.1f;
		}

    	Team *team = &g_app->m_location->m_teams[m_id.GetTeamId()];
		if (m_id.GetUnitId() == team->m_currentUnitId)
		{
			Vector3 toMouse = team->m_currentMousePos - m_pos;
			toMouse.HorizontalAndNormalise();
			m_angVel = (m_front ^ toMouse) * 4.0f;
		}
		else
		{
            Vector3 vel = m_vel;
            if( vel.Mag() > 0.0f )
            {
                m_angVel = (vel.Normalise() ^ m_front) * 4.0f;
            }
            else
            {
                m_angVel = m_front;
            }

            RunAI();

            Entity *enemy = g_app->m_location->GetEntity( m_enemyId );
            if( enemy )
            {
                m_angVel = ((m_pos - enemy->m_pos).Normalise() ^ m_front) * 4.0f;
            }
		}

        m_angVel.x = 0.0f;
        m_angVel.z = 0.0f;
		float const maxTurnRate = 20.0f; // radians per second
		if (m_angVel.Mag() > maxTurnRate)
		{
			m_angVel.SetLength(maxTurnRate);
		}
		m_front.RotateAround(m_angVel * SERVER_ADVANCE_PERIOD);
		m_front.Normalise();

        if( _unit->m_teleportId != -1 )
        {
            m_front = (targetPos - m_pos);
            m_front.y = 0.0f;
            m_front.Normalise();
        }

        if( m_pos.y <= 0.0f )
        {
            m_inWater = syncfrand(3.0f);
        }
    }

	if( !m_onGround ) AdvanceInAir(NULL);

    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;

    float worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0f ) m_pos.x = 0.0f;
    if( m_pos.z < 0.0f ) m_pos.z = 0.0f;
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


void Squadie::Render( float _predictionTime )
{
    if( !m_enabled ) return;


    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_onGround )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }

    float size = 0.5f;

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
            float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
            float thefrand = frand();
            if      ( thefrand > 0.7f ) mat.f *= ( 1.0f - sinf(timeIndex) * 0.5f );
            else if ( thefrand > 0.4f ) mat.u *= ( 1.0f - sinf(timeIndex) * 0.2f );
            else                        mat.r *= ( 1.0f - sinf(timeIndex) * 0.5f );
            glEnable( GL_BLEND );
            glBlendFunc( GL_ONE, GL_ONE );
        }


        m_shape->Render(_predictionTime, mat);

        glEnable        (GL_BLEND);
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       (GL_COLOR_MATERIAL);
        glEnable        (GL_TEXTURE_2D);
		g_app->m_renderer->UnsetObjectLighting();
    }
}


bool Squadie::RenderPixelEffect( float _predictionTime )
{
    if( !m_enabled ) return false;

    Render( _predictionTime );

    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    if( m_onGround )
    {
        predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    }

    float size = 0.5f;

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

    if( m_retargetTimer <= 0.0f )
    {
        m_enemyId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0f, 100.0f, m_id.GetTeamId() );
        m_retargetTimer = 1.0f;
    }


    //
    // Fire at the nearest enemy now and then

    Entity *enemy = g_app->m_location->GetEntity( m_enemyId );;
    if( enemy )
    {
        float distance = (enemy->m_pos - m_pos).Mag();
        if( syncfrand(distance*0.5f) < 2.0f )
        {
            Attack( enemy->m_pos );
        }
    }
}


void Squadie::Attack( Vector3 const &_pos )
{
    if( m_reloading == 0.0f )
    {
        //
        // Fire laser

        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 laserPos = m_laser->GetWorldMatrix( mat ).pos;

        Vector3 fromPos = laserPos;
	    Vector3 toPos( _pos.x + syncsfrand(7.0f),
				       _pos.y,
				       _pos.z + syncsfrand(7.0f) );
	    Vector3 velocity = (toPos - fromPos).SetLength(200.0f);
        g_app->m_location->FireLaser( fromPos, velocity, m_id.GetTeamId() );
        m_justFired = true;

        //
        // Create ejected brass particle

        Matrix34 brass = m_brass->GetWorldMatrix( mat );
        Vector3 particleVel = brass.f * ( 5.0f + syncfrand(10.0f));
        particleVel += Vector3( syncsfrand(5.0f), syncsfrand(5.0f), syncsfrand(5.0f) );
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
    return ( m_secondaryTimer <= 0.0f );
}


void Squadie::FireSecondaryWeapon( Vector3 const &_pos )
{
    InsertionSquad *squad = (InsertionSquad *) g_app->m_location->GetUnit( m_id );
    DarwiniaDebugAssert(squad);

    if( g_app->m_globalWorld->m_research->HasResearch( m_id.GetTeamId(), squad->m_weaponType ) )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        Vector3 laserPos = m_laser->GetWorldMatrix( mat ).pos;

        switch( squad->m_weaponType )
        {
            case GlobalResearch::TypeGrenade:
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowGrenade" );
                g_app->m_helpSystem->PlayerDoneAction( HelpSystem::SquadThrowGrenade );
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableGrenade, m_id.GetTeamId() );
                m_secondaryTimer = 4.0f;
                break;

            case GlobalResearch::TypeAirStrike:
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowAirStrike" );
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableAirstrikeMarker, m_id.GetTeamId() );
                m_secondaryTimer = 20.0f;
                break;

            case GlobalResearch::TypeController:
                g_app->m_soundSystem->TriggerEntityEvent( this, "ThrowController" );
                g_app->m_location->ThrowWeapon( laserPos, _pos, EffectThrowableControllerGrenade, m_id.GetTeamId() );
                m_secondaryTimer = 4.0f;
                break;

            case GlobalResearch::TypeRocket:
                g_app->m_soundSystem->TriggerEntityEvent( this, "FireRocket" );
                g_app->m_location->FireRocket( laserPos, _pos, m_id.GetTeamId() );
                m_secondaryTimer = 4.0f;
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
	if( g_inputManager->controlEvent( ControlUnitPrimaryFireDirected /* ControlUnitStartSecondaryFireDirected */ ) &&
        g_app->m_camera->IsInMode( Camera::ModeEntityTrack ) )
	{
		InputDetails details;
        g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

		Vector3 t = GetSecondaryWeaponTarget();

		return ( t + m_pos ) / 2;
	}

	return Entity::GetCameraFocusPoint();
}

Vector3 Squadie::GetSecondaryWeaponTarget()
{
	InputDetails details;
    g_inputManager->controlEvent( ControlUnitPrimaryFireDirected, details );

	Vector3 t = m_pos;

	Vector3 front = (m_pos - g_app->m_camera->GetPos() );
	front.y = 0.0f;
	front.Normalise();
	Vector3 right = front;
	right.RotateAroundY( M_PI / 2.0f );

	float force = ThrowableWeapon::GetMaxForce( g_app->m_globalWorld->m_research->CurrentLevel( m_id.GetTeamId(), GlobalResearch::TypeGrenade ) );
	float maxRange = ThrowableWeapon::GetApproxMaxRange( force );

	//float total = abs(details.x) + abs(details.y);

	/*char debugstring[512];
	double r = details.x * details.x + details.y * details.y;
	r = sqrt(r);
	sprintf(debugstring, "x = %d, y = %d r = %f\n", details.x, details.y, r);
	DebugOut( debugstring );*/

    // This should be dependent on distance of camera from units
    // Further away, rangeFactor = 1.0, closer rangeFactor closer to 0.66
    const float rangeFactor = 0.75;

	float rMod = (rangeFactor * maxRange * float(float(details.x)/40.0f));
	float fMod = (rangeFactor * maxRange * float(float(details.y)/40.0f));
	t+= right * - rMod;
	t+= front * - fMod;
	t.y = g_app->m_location->m_landscape.m_heightMap->GetValue( t.x, t.z )+5.0f;

	return t;
}