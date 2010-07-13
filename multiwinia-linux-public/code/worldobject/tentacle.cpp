#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "deform.h"
#include "entity_grid.h"
#include "explosion.h"
#include "globals.h"
#include "location.h"
#include "team.h"
#include "unit.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"

#include "sound/soundsystem.h"

#include "worldobject/portal.h"
#include "worldobject/tentacle.h"

Shape *Tentacle::s_shapeBody[NUM_TEAMS];
Shape *Tentacle::s_shapeHead[NUM_TEAMS];


Tentacle::Tentacle()
:   Entity(),
    m_size(1.0),
    m_state(TentacleStateIdle),
    m_impactDetected(false),
    m_idleTimer(0.0)
{
    m_type = TypeCentipede;
    m_direction = g_upVector;
    m_targetDirection = g_upVector;
}


void Tentacle::Begin()
{
    if( !s_shapeBody[m_id.GetTeamId()] )
    {
        char headfilename[256], headshapename[256], bodyfilename[256], bodyshapename[256];
        bool headColourAll = false;
        bool bodyColourAll = false;

        strcpy( headshapename, "centipedehead.shp" );
        strcpy( bodyshapename, "centipede.shp");

        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        int colourId = team->m_lobbyTeam->m_colourId;
        int groupId = team->m_lobbyTeam->m_coopGroupPosition;

        sprintf( headfilename, "%s_%d_%d", headshapename, colourId, groupId );
        sprintf( bodyfilename, "%s_%d_%d", bodyshapename, colourId, groupId );

        Shape *head = g_app->m_resource->GetShape( headfilename, false );
        if( !head )
        {
            head = g_app->m_resource->GetShapeCopy( headshapename, false );
            ConvertShapeColoursToTeam( head, m_id.GetTeamId(), false);
            g_app->m_resource->AddShape( head, headfilename );
        }
        s_shapeHead[m_id.GetTeamId()] = head;

        Shape *body = g_app->m_resource->GetShape( bodyfilename, false );
        if( !body )
        {
            body = g_app->m_resource->GetShapeCopy( bodyshapename, false );
            ConvertShapeColoursToTeam( body, m_id.GetTeamId(), false );
            g_app->m_resource->AddShape( body, bodyfilename );
        }
        s_shapeBody[m_id.GetTeamId()] = body;
    }
    m_shape = s_shapeBody[ m_id.GetTeamId() ];

    Entity::Begin();
    m_onGround = false;
    
    if( !m_next.IsValid() )
    {        
        Team *myTeam = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Unit *myUnit = NULL;
        if( myTeam->m_units.ValidIndex(m_id.GetUnitId()) ) 
        {
            myUnit = myTeam->m_units[ m_id.GetUnitId() ];
        }

        if( myUnit )
        {
            double size = 1.0;// * iv_pow(1.1, myUnit->m_entities.Size() );
            size = min( size, 30.0 );

            Tentacle *prev = NULL;

            for( int i = 0; i < myUnit->m_entities.Size(); ++i )
            {
                if( myUnit->m_entities.ValidIndex(i) )
                {
                    Tentacle *tentacle = (Tentacle *) myUnit->m_entities[i];
                    tentacle->m_positionId = i;
                    tentacle->m_size = size;
                    size *= 1.05;
                    if( prev )
                    {
                        prev->m_prev = tentacle->m_id;
                        tentacle->m_next = prev->m_id;
                    }
                    prev = tentacle;
                }
            }
        }
    }

    int health = m_stats[StatHealth];
    health *= m_size * 2;
    if( health < 0 ) health = 0;
    if( health > 255 ) health = 255;
    m_stats[StatHealth] = health;

    m_radius = m_size * 25.0;
    m_vel = g_upVector;
}


bool Tentacle::ChangeHealth( int _amount, int _damageType )
{
    return false;
}

bool Tentacle::Advance( Unit *_unit )
{
    if( !g_app->m_location->GetBuilding( m_portalId ) ) return true;
    AppReleaseAssert( _unit, "Tentacles must be created in a unit" );
    
    if( m_dead ) return AdvanceDead( _unit );    

    if( m_next.IsValid() )
    {
        m_shape = s_shapeBody[m_id.GetTeamId()];
    }
    else
    {
        m_shape = s_shapeHead[m_id.GetTeamId()];
    }

    if( m_prev.IsValid() )
    {
        // just follow the chunk below us
        m_onGround = false;

        Tentacle *tentacle = (Tentacle *) g_app->m_location->GetEntitySafe( m_prev, TypeTentacle );
        if( tentacle )
        {
            Vector3 trailPos, trailVel;
            int numSteps = 1;
            bool success = tentacle->GetTrailPosition( trailPos, trailVel, 0 );
            if( success )
            {
                m_state = tentacle->m_state;
                if( trailPos.y - (m_size / 2.0) >= g_app->m_location->m_landscape.m_heightMap->GetValue( trailPos.x, trailPos.z ) ||
                    trailPos.y > m_pos.y ||
                    m_state == TentacleStateSwipeAcross ) 
                {
                    m_vel = ( trailPos - m_pos ) / SERVER_ADVANCE_PERIOD;
                    m_pos = trailPos;
                    m_direction = trailVel;
                    //m_front = m_vel;
                    //m_front.Normalise();
                }
                else
                {
                    m_direction = trailVel;
                    m_vel.Zero();
                }
            }
        }
        else
        {
            m_prev.SetInvalid();
        }
    }
    else
    {
        // bottom chunk, take control
        m_onGround = true;

        switch( m_state )
        {
            case TentacleStateIdle:         AdvanceIdle();  break;
            case TentacleStateSlamGround:   AdvanceSlam();  break;
            case TentacleStateSwipeAcross:  AdvanceSwipe(); break;
        }
    }

    if( m_idleTimer >= 0.0 )
    {
        m_idleTimer -= SERVER_ADVANCE_PERIOD;
        m_idleTimer = max( 0.0, m_idleTimer );
    }

    Attack( m_pos );
    HandleGroundCollision();           
    RecordHistoryRotation();
    
    if (_unit)
	{		
		_unit->UpdateEntityPosition( m_pos, m_radius );	
	}   

    return false;
}

bool Tentacle::AdvanceIdle()
{
    if( m_targetDirection == g_zeroVector )
    {
        double timeIndex = GetNetworkTime() + m_id.GetUniqueId();
        Vector3 direction = g_upVector + Vector3(iv_sin( timeIndex ) * 0.1, 
                                                 0.0,
                                                 iv_sin( timeIndex ) * 0.1 );
        direction.Normalise();
        m_direction += direction;
        m_direction.Normalise();

        if( syncrand() % 50 == 0 )
        {
            Portal *p = (Portal *)g_app->m_location->GetBuilding( m_portalId );
            if( p && p->m_type == Building::TypePortal &&
                p->m_finalSummonState != Portal::SummonStateExplode )
            {
                m_state = TentacleStateSlamGround;
                Vector3 dir = (m_pos - p->GetPortalCenter()).Normalise();
                Vector3 targetPos = m_pos + dir * 10.0;
                targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z ) - 5.0;
                m_targetDirection = ( targetPos - m_pos ).Normalise();
            }
        }
    }
    else if( m_idleTimer <= 0.0 )
    {
        double factor1 = 1.0 * SERVER_ADVANCE_PERIOD;
        double factor2 = 1.0 - factor1;

        m_direction = factor2 * m_direction + factor1 * m_targetDirection;
        m_direction.Normalise();

        if( (m_targetDirection ^ m_direction).Mag() < 0.1 )
        {
            m_targetDirection = g_zeroVector;
        }
    }

    return false;
}

bool Tentacle::AdvanceSlam()
{
    double factor1 = 2.0 * SERVER_ADVANCE_PERIOD;
    double factor2 = 1.0 - factor1;

    m_direction = factor2 * m_direction + factor1 * m_targetDirection;
    m_direction.Normalise();

    if( (m_targetDirection ^ m_direction).Mag() < 0.1 )
    {
        for( int i = 0; i < 20; ++i )
        {
            Vector3 pos = m_pos;
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
            Vector3 p = pos;
            p.x += sfrand(5.0);
            p.z += sfrand(5.0);
            Vector3 vel = p - pos;
            vel += g_upVector;
            vel.SetLength( 20.0 );
            double size = 15.0 + frand( 20.0 );
            g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeRocketTrail, size );
        }

        if( syncrand() % 3 < 2 )
        {
            Portal *p = (Portal *)g_app->m_location->GetBuilding( m_portalId );
            if( p && p->m_type == Building::TypePortal )
            {
                Vector3 pos = p->GetPortalCenter() + Vector3( SFRAND( 10.0) , 0.0, SFRAND(10.0) );
                Vector3 dir = (pos - p->GetPortalCenter()).Normalise();
                Vector3 targetPos = m_pos + dir * 10.0;
                targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
                m_targetDirection = ( targetPos - m_pos ).Normalise();
                m_state = TentacleStateSwipeAcross;
                m_idleTimer = 2.0;
            }
        }
        else
        {
            m_state = TentacleStateIdle;
            m_targetDirection = g_upVector;
            m_idleTimer = 4.0;
        }
    }

    return false;
}

bool Tentacle::AdvanceSwipe()
{
    if( m_idleTimer > 0.0 ) return false;
    double factor1 = 3.0 * SERVER_ADVANCE_PERIOD;
    double factor2 = 1.0 - factor1;

    m_direction = factor2 * m_direction + factor1 * m_targetDirection;
    m_direction.Normalise();

    if( (m_targetDirection ^ m_direction).Mag() < 0.1 )
    {
        m_state = TentacleStateIdle;
        m_targetDirection = g_upVector;
    }
    return false;
}

void Tentacle::HandleGroundCollision()
{
    if( m_onGround ) return;

    if( m_pos.y - m_size < g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z ) )
    {
        m_pos.y = (m_size / 2.0) + g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        m_vel.Zero();
        
        if( !m_impactDetected )
        {
            m_impactDetected = true;
            for( int i = 0; i < 20; ++i )
            {
                Vector3 pos = m_pos;
                pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
                Vector3 p = pos;
                p.x += sfrand(5.0);
                p.z += sfrand(5.0);
                Vector3 vel = p - pos;
                vel += g_upVector;
                vel.SetLength( 20.0 );
                double size = 15.0 + frand( 20.0 );
                g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeRocketTrail, size );
            }
        }
    }
    else
    {
        m_impactDetected = false;
    }
}

void Tentacle::Attack( Vector3 const &_pos )
{
    int numFound;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, _pos.x, _pos.z, m_radius, &numFound );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = (Entity *) g_app->m_location->GetEntity( id ); 
        if( entity->m_type != Entity::TypeTentacle &&
            entity->m_type != Entity::TypeShaman )
        {
            Vector3 pushVector = ( entity->m_pos - _pos );
            double distance = pushVector.Mag();       
            if( distance < m_radius )
            {
                entity->m_vel = ( entity->m_pos - m_pos );
                entity->m_vel.SetLength( 60.0 );
                entity->ChangeHealth( -255 );
            }
        }
    }
}

void Tentacle::SetPortalId( int _id )
{
    m_portalId = _id;
}

void Tentacle::RecordHistoryRotation()
{    
    m_rotationHistory.PutDataAtStart( m_direction );
    
    int maxHistorys = 3;

    for( int i = maxHistorys; i < m_rotationHistory.Size(); ++i )
    {
        m_rotationHistory.RemoveData(i);
    }
}


bool Tentacle::GetTrailPosition( Vector3 &_pos, Vector3 &_vel, int _numSteps )
{
    if( m_rotationHistory.Size() < 3 ) return false; 
    
    Vector3 previousRotation = *m_rotationHistory.GetPointer(0);
    _pos = m_pos + (previousRotation * 3.0 * m_positionId );
    _vel = m_direction;
    return true;
}

void Tentacle::Render( double _predictionTime )
{       
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    //predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
 
    double maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;

    Shape *shape = m_shape;

    if( !m_dead )
    {
        glDisable( GL_TEXTURE_2D );
        //RenderSphere( m_targetPos, 5.0 );

        Vector3 predictedFront = m_front;
        Vector3 predictedUp = m_direction;
        Vector3 predictedRight = predictedUp ^ predictedFront;
        predictedFront = predictedRight ^ predictedUp;
        predictedFront.Normalise();
        
	    Matrix34 mat(predictedFront, predictedUp, predictedPos);

        mat.f *= m_size;
        mat.u *= m_size;
        mat.r *= m_size;                

        g_app->m_renderer->SetObjectLighting();
        shape->Render(_predictionTime, mat);   
        g_app->m_renderer->UnsetObjectLighting();

        glDisable( GL_NORMALIZE );
    }

    if( !m_next.IsValid() &&
        m_state == TentacleStateIdle &&
        ( m_direction ^ g_upVector).Mag() < 0.2 )
    {
        Portal *p = (Portal *)g_app->m_location->GetBuilding( m_portalId );
        if( p )
        {
            if( p->m_finalSummonState == Portal::SummonStateExplode )
            {
                Vector3 pos = p->m_pos;
                pos.y += 100.0;
                Portal::RenderBeam( m_pos, pos );
            }
        }
    }
}

bool Tentacle::IsInView()
{
    return g_app->m_camera->SphereInViewFrustum( m_pos+m_centrePos, m_radius * 2.0 );
}


bool Tentacle::RenderPixelEffect(double _predictionTime)
{
	Render(_predictionTime);

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );

    if( !m_dead )
    {
        Vector3 predictedFront = m_front;
        Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
        Vector3 predictedRight = predictedUp ^ predictedFront;
        predictedFront = predictedRight ^ predictedUp;
        
	    Matrix34 mat(predictedFront, predictedUp, predictedPos);
        mat.f *= m_size;
        mat.u *= m_size;
        mat.r *= m_size;        

        g_app->m_renderer->MarkUsedCells(m_shape, mat);
    }

    return true;
}
