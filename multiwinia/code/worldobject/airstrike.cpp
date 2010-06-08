#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/debug_render.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"
#include "lib/profiler.h"

#include "worldobject/airstrike.h"

#include "app.h"
#include "explosion.h"
#include "location.h"
#include "renderer.h"
#include "camera.h"
#include "particle_system.h"
#include "gametimer.h"

#include "sound/soundsystem.h"


AirstrikeUnit::AirstrikeUnit(int teamId, int unitId, int numEntities, Vector3 const &_pos)
:   Unit( Entity::TypeSpaceInvader, teamId, unitId, numEntities, _pos ),
    m_numInvaders(numEntities),
    m_state(StateApproaching),
    m_speed(0.0f),
    m_effectId(-1),
    m_napalm(false)
{
    m_attackPosition = _pos;
    m_attackPosition.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_attackPosition.x, m_attackPosition.z ) + 70.0f;

    m_speed = EntityBlueprint::GetStat( Entity::TypeSpaceInvader, Entity::StatSpeed ) * 10.0f;

    //AppDebugOut("New AirStrike Unit created with %d invaders, unit id = %d, x:%2.2f y:%2.2f z:%2.2f\n", numEntities, unitId, _pos.x, _pos.y, _pos.z );
}


void AirstrikeUnit::Begin()
{
    double inset = 100.0f;
    double startHeight = 500.0f;
    double landSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double landSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    
    DArray<Vector3> startPositions;
    startPositions.PutData( Vector3(inset,startHeight,inset) );
    startPositions.PutData( Vector3(inset,startHeight,landSizeZ-inset) );
    startPositions.PutData( Vector3(landSizeX-inset,startHeight,landSizeZ-inset) );
    startPositions.PutData( Vector3(landSizeX-inset,startHeight,inset) );

    int enterIndex = -1;
    int exitIndex = -1;
    double nearest = 99999.9;

    //
    // Choose the nearest corner as the exit position

    for( int i = 0; i < startPositions.Size(); ++i )
    {
        Vector3 thisPos = startPositions[i];
        double thisDist = ( thisPos - m_attackPosition ).Mag();
        if( thisDist < nearest )
        {
            nearest = thisDist;
            exitIndex = i;
        }
    }

    //
    // Choose the next nearest corner as the entry position

    nearest = 99999.9;
    for( int i = 0; i < startPositions.Size(); ++i )
    {
        if( i != exitIndex )
        {
            Vector3 thisPos = startPositions[i];
            double thisDist = ( thisPos - m_attackPosition ).Mag();
            if( thisDist < nearest )
            {
                nearest = thisDist;
                enterIndex = i;
            }
        }
    }
  

    //
    // Set up our flight path 

    m_enterPosition = startPositions[ enterIndex ];
    m_exitPosition = startPositions[ exitIndex ];

    Vector3 centre( landSizeX/2.0, 0.0, landSizeZ/2.0 );
    Vector3 front = ( centre - m_enterPosition ).Normalise();

    m_wayPoint = m_enterPosition;
    m_front = front;
    m_up = g_upVector;

    //
    // Spawn our space invaders
   
    g_app->m_location->SpawnEntities( m_enterPosition, m_teamId, m_unitId, Entity::TypeSpaceInvader, m_numInvaders, front, 0.0f );
    //AppDebugOut("Begining AirStrike Unit, enter pos = x:%2.2f y:%2.2f z:%2.2f,  exit pos = x:%2.2f y:%2.2f z:%2.2f \n", m_enterPosition.x, m_enterPosition.y, m_enterPosition.z, m_exitPosition.x, m_exitPosition.y, m_exitPosition.z );
}


bool AirstrikeUnit::AdvanceToTargetPosition( Vector3 _targetPos )
{
    Vector3 targetFront = ( _targetPos - m_wayPoint );
    targetFront.Normalise();

    double amountToTurn = SERVER_ADVANCE_PERIOD;
    Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetFront * amountToTurn;
    actualDir.Normalise();
    m_front = actualDir;
    
    Vector3 right = m_front ^ g_upVector;
    m_up = right ^ m_front;

    double distToTarget = ( m_attackPosition - m_wayPoint ).Mag();
    double desiredSpeed = EntityBlueprint::GetStat( Entity::TypeSpaceInvader, Entity::StatSpeed );
    if( ( m_state == StateApproaching && distToTarget > 600.0f) ||
        ( m_state == StateLeaving && distToTarget > 100.0f) )
    {
        desiredSpeed *= 10.0f;
    }
    m_speed = m_speed * (1.0f - amountToTurn) + desiredSpeed * amountToTurn;
        
    m_wayPoint += m_front * m_speed * SERVER_ADVANCE_PERIOD;

    double newDistance = (_targetPos - m_wayPoint).Mag();
    return( newDistance < 10.0f );
}


bool AirstrikeUnit::Advance( int _slice )
{
    //
    // Has our target marker moved?
    //AppDebugOut("Updating AirStrike Unit\n");

    if( g_app->m_location->m_effects.ValidIndex(m_effectId) )
    {
        WorldObject *targetMarker = g_app->m_location->m_effects[ m_effectId ];
        m_attackPosition = targetMarker->m_pos;
        m_attackPosition.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_attackPosition.x, m_attackPosition.z ) + 70.0f;
    }

    switch( m_state )
    {
        case StateApproaching:      
        {
            bool amIThere = AdvanceToTargetPosition( m_attackPosition );
            if( amIThere ) 
            {
                m_state = StateLeaving;            
            }
            break;
        }
        
        case StateLeaving:
        {
            bool amIThere = AdvanceToTargetPosition( m_exitPosition );
            break;
        }
    };
        
    return Unit::Advance( _slice );
}


bool AirstrikeUnit::IsInView()
{
    return true;
}


void AirstrikeUnit::Render( double _predictionTime )
{
//#ifdef DEBUG_RENDER_ENABLED
//    glDisable( GL_TEXTURE_2D );
//    Vector3 height(0,50,0);
//    RenderArrow     ( m_enterPosition, m_attackPosition, 10.0f, RGBAColour(255,50,50,255) );
//    RenderArrow     ( m_attackPosition, m_exitPosition, 10.0f, RGBAColour(255,50,50,255) );
//    
//    Vector3 right = m_front ^ m_up;
//    RenderSphere    ( m_wayPoint, 5.0f, RGBAColour(255,255,255,255) );
//    RenderArrow     ( m_wayPoint, m_wayPoint + m_front * 50.0f, 10.0f, RGBAColour(255,255,255,255) );
//    RenderArrow     ( m_wayPoint, m_wayPoint + m_up * 30.0f, 6.0f, RGBAColour(50,50,255,255) );    
//    RenderArrow     ( m_wayPoint, m_wayPoint + right * 30.0f, 6.0f, RGBAColour(50,255,50,255) );
//#endif

    Unit::Render( _predictionTime );
}


SpaceInvader::SpaceInvader()
:   Entity(),
    m_armed(true),
    m_napalmDrops(30)
{
    m_shape = g_app->m_resource->GetShape( "spaceinvader.shp" );
    m_bombShape = g_app->m_resource->GetShape( "throwable.shp" );
}

bool SpaceInvader::Advance( Unit *_unit )
{
    if( m_dead ) return true;
    //AppDebugOut("Upating SpaceInvader %d, formation index %d ", m_id.GetUniqueId(), m_formationIndex );

    AirstrikeUnit *airstrikeUnit = (AirstrikeUnit *) _unit;

    //
    // Move a little 

    Vector3 targetPos = _unit->GetWayPoint();

    START_PROFILE("Update Pos");
    if( targetPos != g_zeroVector )
    {
        targetPos += _unit->GetFormationOffset( Unit::FormationAirstrike, m_formationIndex );

        m_vel = ( targetPos - m_pos ) / SERVER_ADVANCE_PERIOD;
        m_pos = targetPos;
    
        m_front = airstrikeUnit->m_front;
        //AppDebugOut("target position x:%2.2f y:%2.2f z:%2.2f\n", targetPos.x, targetPos.y, targetPos.z );
    }
    END_PROFILE("Update Pos");
    
    //
    // Drop bombs if near enough

    if( m_armed )
    {
        double distToTarget = ( m_pos - airstrikeUnit->m_attackPosition ).Mag();
        if( distToTarget < 90.0f )
        {
            Grenade *weapon = NULL;
            if( airstrikeUnit->m_napalm )
            {
                weapon = new FlameGrenade( m_pos - g_upVector * 12.0f, m_front, m_vel.Mag() );
            }
            else
            {
                weapon = new Grenade( m_pos - g_upVector * 12.0f, m_front, m_vel.Mag() );                   
                weapon->m_type = EffectThrowableAirstrikeBomb;
            }
            weapon->m_life = 1.5;
            weapon->m_power = 50.0f;
            int index = g_app->m_location->m_effects.PutData( weapon );            
            weapon->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
            weapon->m_id.GenerateUniqueId();
            g_app->m_soundSystem->TriggerEntityEvent( this, "DropGrenade" );
            m_armed = false;
        }
    }


    //
    // Clip to edge of world

    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

    if( m_pos.x < 0.0f || 
        m_pos.z < 0.0f ||
        m_pos.x >= worldSizeX ||
        m_pos.z >= worldSizeZ )
    {
        return true;
    }

    return false;
}


bool SpaceInvader::ChangeHealth( int _amount, int _damageType )
{
    // Space invaders are now INVINCIBLE

//    bool dead = m_dead;
//    
//    Entity::ChangeHealth( _amount );
//    
//    if( m_dead && !dead )
//    {
//        // We just died
//        Matrix34 transform( m_front, g_upVector, m_pos );
//        g_explosionManager.AddExplosion( m_shape, transform );         
//    }

    if( _damageType != DamageTypeTurretShell ) return false;

    if( !m_dead )
    {
        int healthBandBefore = int( m_stats[StatHealth] / 50.0f );

        if( _amount < 0 )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "LoseHealth" );
        }

        if( m_stats[StatHealth] + _amount < 0 )
        {
            m_stats[StatHealth] = 0;
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
            g_app->m_particleSystem->CreateParticle( m_pos, g_zeroVector, Particle::TypeMuzzleFlash );
        }

        int healthBandAfter = int( m_stats[StatHealth] / 50.0f );

        double fractionDead = 1.0f - m_stats[StatHealth] / (double) EntityBlueprint::GetStat( TypeEngineer, StatHealth );
        fractionDead = max( fractionDead, 0.0 );
        fractionDead = min( fractionDead, 1.0 );
        
        if( fractionDead == 1.0f || healthBandAfter < healthBandBefore )
        {
            Matrix34 transform( m_front, g_upVector, m_pos );
            g_explosionManager.AddExplosion( m_shape, transform, fractionDead ); 
        }
    }

    return true;
}


void SpaceInvader::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );
    
    _list->PutData( "DropGrenade" );
}


void SpaceInvader::Render( double _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    glDisable( GL_TEXTURE_2D );

#ifdef DEBUG_RENDER_ENABLED
    //RenderSphere( m_targetPos, 5.0f );
#endif
    
    g_app->m_renderer->SetObjectLighting();

    Matrix34 mat(m_front, g_upVector, predictedPos);
    mat.f *= 2.0f;
    mat.u *= 2.0f;
    mat.r *= 2.0f;
    m_shape->Render(_predictionTime, mat);       
    
    if( m_armed )
    {
        mat = Matrix34(-g_upVector, m_front, predictedPos - g_upVector * 12.0f );
        m_bombShape->Render(_predictionTime, mat);
    }

    g_app->m_renderer->UnsetObjectLighting();  

}

