#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/math/random_number.h"

#include "worldobject/tank.h"
#include "worldobject/ai.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "renderer.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "explosion.h"
#include "camera.h"
#include "team.h"
#include "multiwinia.h"
#include "entity_grid.h"


Tank::Tank()
:   Entity(),
    m_markerTurret(NULL),
    m_shapeTurret(NULL),
    m_markerBarrelEnd(NULL),
    m_speed(0.0),
    m_attackTimer(0.0),
    m_state(StateCombat),
    m_numPassengers(0),
    m_previousUnloadTimer(0.0),
    m_missileFired(false),
    m_mineTimer(0.0)
{
    SetType( TypeTank );
}


Tank::~Tank()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    team->UnRegisterSpecial( m_id );
}


void ConvertShape ( ShapeFragment *_frag, RGBAColour _col )
{
    RGBAColour *newColours = new RGBAColour[_frag->m_numColours];

    for( int i = 0; i < _frag->m_numColours; ++i )
    {
        newColours[i].r = _col.r;
        newColours[i].g = _col.g;
        newColours[i].b = _col.b;

        // scale colours down - colours on 3d models appear brighter than 2d equivalents due to lighting
        newColours[i] *= 0.5;
    }
    _frag->RegisterColours( newColours, _frag->m_numColours );


    for( int i = 0; i < _frag->m_childFragments.Size(); ++i )
    {
        ConvertShape( _frag->m_childFragments[i], _col );
    }
}


void Tank::Begin()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    RGBAColour colour = team->m_colour;
    int colourId = team->m_lobbyTeam->m_colourId;
    int groupId = team->m_lobbyTeam->m_coopGroupPosition;

    char filename[256];
    sprintf( filename, "tank_body.shp_%d_%d", colourId, groupId );
    m_shape = g_app->m_resource->GetShape(filename, false);
    if( !m_shape )
    {
        m_shape = g_app->m_resource->GetShapeCopy( "tank_body.shp", false, false );
        ConvertShape( m_shape->m_rootFragment, colour);
        m_shape->BuildDisplayList();
        g_app->m_resource->AddShape( m_shape, filename );
    }

    sprintf( filename, "tank_turret.shp_%d_%d", colourId, groupId );
    m_shapeTurret = g_app->m_resource->GetShape(filename, false);
    if( !m_shapeTurret )
    {
        m_shapeTurret = g_app->m_resource->GetShapeCopy( "tank_turret.shp", false, false );
        ConvertShape( m_shapeTurret->m_rootFragment, colour);
        m_shapeTurret->BuildDisplayList();
        g_app->m_resource->AddShape( m_shapeTurret, filename );
    }

    const char markerTurretName[] = "MarkerTurret";
    m_markerTurret      = m_shape->m_rootFragment->LookupMarker( markerTurretName );
    AppReleaseAssert( m_markerTurret, "Tank: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerTurretName, m_shape->m_name );

    const char markerEntranceName[] = "MarkerEntrance";
    m_markerEntrance    = m_shape->m_rootFragment->LookupMarker( markerEntranceName );
    AppReleaseAssert( m_markerEntrance, "Tank: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerEntranceName, m_shape->m_name );

    const char markerBarrelEndName[] = "MarkerBarrelEnd";
    m_markerBarrelEnd   = m_shapeTurret->m_rootFragment->LookupMarker( markerBarrelEndName );
    AppReleaseAssert( m_markerBarrelEnd, "Tank: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", markerBarrelEndName, m_shapeTurret->m_name );

    m_centrePos = m_shape->CalculateCentre(g_identityMatrix34);
    m_radius = m_shape->CalculateRadius( g_identityMatrix34, m_centrePos );

    Entity::Begin();

    m_wayPoint = g_zeroVector;
    m_turretFront = m_front;

    team->RegisterSpecial( m_id );

    if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle )
    {
        m_missileFired = true;
    }
}


bool Tank::ChangeHealth( int _amount, int _damageType )
{
    if( _damageType == DamageTypeLaser ) return false;

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
            g_app->m_soundSystem->StopAllSounds( m_id );
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

        double fraction = _amount / EntityBlueprint::GetStat( Entity::TypeTank, StatHealth ) ;

        Matrix34 bodyMat( m_front, m_up, m_pos );

        Vector3 turretPos = m_markerTurret->GetWorldMatrix( bodyMat ).pos;
        Matrix34 turretMat( m_turretFront, m_up, turretPos );

        g_explosionManager.AddExplosion( m_shape, bodyMat ); 
        g_explosionManager.AddExplosion( m_shapeTurret, turretMat, fraction );   
    }

    if( m_dead && !dead )
    {
        // We just died

        Matrix34 bodyMat( m_front, m_up, m_pos );

        Vector3 turretPos = m_markerTurret->GetWorldMatrix( bodyMat ).pos;
        Matrix34 turretMat( m_turretFront, m_up, turretPos );

        g_explosionManager.AddExplosion( m_shape, bodyMat ); 
        g_explosionManager.AddExplosion( m_shapeTurret, turretMat );        
    }

    return true;
}


bool Tank::Advance( Unit *_unit )
{
    if( !m_dead )
    {
        //
        // Turn to face our waypoint

	    Vector3 toTarget = m_wayPoint - m_pos;
	    toTarget.HorizontalAndNormalise();
	    double dotProd = toTarget * m_front;

	    if (dotProd < 0.999)
        {
	        Vector3 rotation = m_front ^ toTarget;
	        rotation.SetLength(0.05);
	        m_front.RotateAround(rotation);
        }

        //
        // Move towards waypoint
        if( m_wayPoint != g_zeroVector )
        {
            toTarget = m_wayPoint - m_pos;
            toTarget.y = 0.0;
            double distance = toTarget.Mag();
            if( distance > 50.0 )
            {
                m_speed += 10.0 * SERVER_ADVANCE_PERIOD;
                m_speed = min( m_speed, (double)m_stats[StatSpeed] );
            }
            else
            {
                m_speed -= 20.0 * SERVER_ADVANCE_PERIOD;
                m_speed = max( m_speed, 0.0 );
            }

            m_vel = m_front * m_speed;
            m_pos += m_vel * SERVER_ADVANCE_PERIOD;
            m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
            m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
            if( (m_pos - m_wayPoint).Mag() < 5.0 )
            {
                m_wayPoint = g_zeroVector;
                m_vel.Zero();
            }
        }

    
        //
        // Rotate our turret to face the target
        // Fire if we're in line
    
        if( m_attackTarget != g_zeroVector && m_state == StateCombat )
        {
            m_attackTimer -= SERVER_ADVANCE_PERIOD;
            m_attackTimer = max( m_attackTimer, 0.0 );
            Matrix34 bodyMat(m_front, m_up, m_pos );
            Vector3 turretPos = m_markerTurret->GetWorldMatrix( bodyMat ).pos;
	        toTarget = m_attackTarget - turretPos;
	        toTarget.HorizontalAndNormalise();
	        dotProd = toTarget * m_turretFront;

	        if (dotProd < 0.999)
            {
	            Vector3 rotation = m_turretFront ^ toTarget;
                double rotateAmount = 0.05 + 0.15 * (1.0 - dotProd);
	            rotation.SetLength(rotateAmount);
	            m_turretFront.RotateAround(rotation);
            }
        }

        m_mineTimer -= SERVER_ADVANCE_PERIOD;
        m_mineTimer = max( m_mineTimer, 0.0 );
    }

    
    return Entity::Advance( _unit );
}


void Tank::SetWayPoint( Vector3 const &_wayPoint )
{
    m_wayPoint = _wayPoint;
    m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_wayPoint.x, m_wayPoint.z );
}


void Tank::SetAttackTarget( Vector3 const &_attackTarget )
{
    /*double distance = ( m_pos - _attackTarget ).Mag();
    if( distance < 50.0 )
    {
        if( m_attackTarget != Vector3(0,0,0) )
        {
            // Player wishes to reset our attack target
            m_attackTarget.Zero();                
        }
        else
        {
            // Player wishes to toggle our loading state
            switch( m_state )
            {
                case StateCombat:               m_state = StateUnloading;           break;
                case StateUnloading:            m_state = StateLoading;             break;
                case StateLoading:              m_state = StateCombat;              break;
            }
        }
    }
    else*/
    {
        m_attackTarget = _attackTarget;
        m_attackTarget.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_attackTarget.x, m_attackTarget.z );
        m_state = StateCombat;
    }
}


bool Tank::IsLoading()
{
    return( m_state == StateLoading && m_numPassengers < TANK_CAPACITY );
}


bool Tank::IsUnloading()
{
    return( m_state == StateUnloading && m_numPassengers > 0 &&
            GetHighResTime() >= m_previousUnloadTimer + TANK_UNLOADPERIOD );
}


void Tank::AddPassenger()
{
    ++m_numPassengers;
}


void Tank::RemovePassenger()
{
    --m_numPassengers;
    m_previousUnloadTimer = GetHighResTime();
}

void Tank::GetEntrance( Vector3 &_exitPos, Vector3 &_exitDir )
{
    Matrix34 mat( m_front, m_up, m_pos );
    Matrix34 entranceMat = m_markerEntrance->GetWorldMatrix(mat);
    _exitPos = entranceMat.pos;
    _exitDir = entranceMat.f;
}


void Tank::SetMissileTarget( Vector3 const &_startRay, Vector3 const &_rayDir )
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

void Tank::SetMissileTarget(const Vector3 &_target)
{
    m_missileTarget = _target;
}


Vector3 Tank::GetMissileTarget()
{
    return m_missileTarget;
}


void Tank::LaunchMissile()
{
    if( !m_missileFired )
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
        int index = g_app->m_location->m_effects.PutData( missile );
        missile->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
        missile->m_id.GenerateUniqueId();

        m_missileFired = true;
    }
}

void Tank::PrimaryFire()
{
    if( m_attackTimer <= 0.0 )
    {
        Matrix34 bodyMat(m_front, m_up, m_pos );
        Vector3 turretPos = m_markerTurret->GetWorldMatrix( bodyMat ).pos;
        Vector3 toTarget = m_attackTarget - turretPos;
        toTarget.HorizontalAndNormalise();
        double dotProd = toTarget * m_turretFront;

        Vector3 target;
        Matrix34 turretMat( m_turretFront, m_up, turretPos );
        Vector3 barrelEndPos = m_markerBarrelEnd->GetWorldMatrix( turretMat ).pos;

        if (dotProd < 0.75)
        {
            target = ( barrelEndPos - m_pos ).Normalise();
            target = m_pos + (m_turretFront * 300.0);
        }
        else 
        {
            target = m_attackTarget;
        }

        g_app->m_location->FireRocket( barrelEndPos-Vector3(0,2,0), target, m_id.GetTeamId(), 2.0 );
        m_attackTimer = m_stats[StatRate];
        if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle ) 
        {
            m_attackTimer /= 4.0;
        }
    }
}

void Tank::DropMine()
{
    if( m_mineTimer <= 0.0 )
    {
        LandMine *mine = new LandMine();
        mine->m_pos = m_pos;
        int weaponId = g_app->m_location->m_effects.PutData( mine );    
        mine->m_id.Set(m_id.GetTeamId(), UNIT_EFFECTS, weaponId, -1 );
        mine->m_id.GenerateUniqueId();

        mine->m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        mine->m_front = m_front;

        m_mineTimer = 1.0;
        m_attackTimer = m_stats[StatRate];
        if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeTankBattle ) 
        {
            m_attackTimer /= 4.0;
        }
    }
}

void Tank::Render( double _predictionTime )
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
    

    //
    // Work out our predicted position

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
    predictedPos.y = max( predictedPos.y, 0.0 );
    predictedPos.y += iv_sin(g_gameTime + m_id.GetUniqueId() ) * 2;
    Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    predictedUp.x += iv_sin( (g_gameTime + m_id.GetUniqueId() ) * 2 ) * 0.05;
    predictedUp.z += iv_cos( g_gameTime + m_id.GetUniqueId() ) * 0.05;
    
    Vector3 predictedFront = m_front;
    Vector3 right = predictedUp ^ predictedFront;
    predictedFront = right ^ predictedUp;

    Vector3 toTarget = m_wayPoint - m_pos;
    toTarget.HorizontalAndNormalise();
    double dotProd = toTarget * m_front;

    if (dotProd < 0.999)
    {
        Vector3 rotation = m_front ^ toTarget;
        rotation.SetLength(0.05 * _predictionTime * 10);
        predictedFront.RotateAround(rotation);
    }
    
    g_app->m_renderer->SetObjectLighting();    


    //
    // Render the tank body

    Matrix34 bodyMat(predictedFront, predictedUp, predictedPos );
    if( m_renderDamaged )
    {
        glBlendFunc( GL_ONE, GL_ONE );
        glEnable( GL_BLEND );

        if( frand() > 0.5 )
        {
            bodyMat.r *= ( 1.0 + iv_sin(g_gameTime) * 0.5 );
        }
        else
        {
            bodyMat.f *= ( 1.0 + iv_sin(g_gameTime) * 0.5 );
        }
    }
    m_shape->Render( _predictionTime, bodyMat );


    //
    // Render the tank turret
    
    Vector3 turretPos = m_markerTurret->GetWorldMatrix( bodyMat ).pos;
    toTarget = m_attackTarget - turretPos;
    toTarget.HorizontalAndNormalise();
    dotProd = toTarget * m_turretFront;

    Vector3 turretFront = m_turretFront;
    if (dotProd < 0.999)
    {
        Vector3 rotation = m_turretFront ^ toTarget;
        double rotateAmount = 0.05 + 0.15 * (1.0 - dotProd);
        rotation.SetLength(rotateAmount * _predictionTime * 10);
        turretFront.RotateAround(rotation);
    }
    Matrix34 turretMat( turretFront, predictedUp, turretPos );
    if( m_renderDamaged )
    {
        glBlendFunc( GL_ONE, GL_ONE );
        glEnable( GL_BLEND );

        if( frand() > 0.5 )
        {
            turretMat.r *= ( 1.0 + iv_sin(g_gameTime) * 0.5 );
        }
        else
        {
            turretMat.f *= ( 1.0 + iv_sin(g_gameTime) * 0.5 );
        }
    }
    m_shapeTurret->Render( _predictionTime, turretMat );
    

    //
    // Render our state

    g_app->m_renderer->UnsetObjectLighting();
}


bool Tank::RenderPixelEffect( double _predictionTime )
{
    //Render( _predictionTime );
    return false;
}

bool Tank::IsSelectable()
{
    return true;
}

void Tank::RunAI( AI *_ai )
{
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeTankBattle ) RunTankBattleAI( _ai );
    else RunStandardAI( _ai );
}

void Tank::RunStandardAI( AI *_ai )
{
    // we dont need to do anything fancy here, just find the nearest group of enemies and start blasting
    if( m_wayPoint == g_zeroVector )
    {
        double dist = FLT_MAX;
        int id = -1;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeAITarget &&
                    b->m_id.GetTeamId() != m_id.GetTeamId() &&
                    b->m_id.GetTeamId() != 255 )
                {
                    if( (m_pos - b->m_pos).Mag() < dist &&
                        (m_pos - b->m_pos).Mag() > 50.0)
                    {
                        dist = (m_pos - b->m_pos).Mag();
                        id = b->m_id.GetUniqueId();
                    }
                }
            }
        }

        Building *b = g_app->m_location->GetBuilding(id);
        if( b )
        {
            SetWayPoint( b->m_pos );
        }
    }

    WorldObjectId enemy = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, 50.0, 300.0, m_id.GetTeamId() );
    Entity *e = g_app->m_location->GetEntity( enemy );
    if( e )
    {
        SetAttackTarget( e->m_pos );
        PrimaryFire();
    }
}

void Tank::RunTankBattleAI( AI *_ai )
{
    WorldObjectId enemy = g_app->m_location->m_entityGrid->GetBestEnemy(m_pos.x, m_pos.z, 50.0, 3000.0, m_id.GetTeamId() );
    Entity *e = g_app->m_location->GetEntity( enemy );
    if( e )
    {
        Vector3 pos = e->m_pos;
        pos.x += syncsfrand(250.0);
        pos.z += syncsfrand(250.0); 
        SetWayPoint( pos );
        SetAttackTarget( e->m_pos );

        double dist = (m_pos - e->m_pos).Mag();
        if( dist > 50.0 &&
            dist < 300.0 )
        {
            PrimaryFire();
        }
    }
}