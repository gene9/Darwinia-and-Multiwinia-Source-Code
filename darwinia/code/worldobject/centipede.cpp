#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/profiler.h"

#include "app.h"
#include "camera.h"
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

#include "worldobject/centipede.h"

Shape *Centipede::s_shapeBody = NULL;
Shape *Centipede::s_shapeHead = NULL;


Centipede::Centipede()
:   Entity(),
    m_size(1.0f),
    m_linked(false),
    m_panic(0.0f),
    m_numSpiritsEaten(0),
    m_lastAdvance(0.0f)
{
    m_type = TypeCentipede;

    if( !s_shapeBody )
    {
        s_shapeBody         = g_app->m_resource->GetShape( "centipede.shp" );
        s_shapeHead         = g_app->m_resource->GetShape( "centipedehead.shp" );
    }

    m_shape = s_shapeBody;
}


void Centipede::Begin()
{
    Entity::Begin();
    m_onGround = true;
    
    if( !m_next.IsValid() )
    {
        //
        // Link every centipede in this unit into one long centipede
        
        Team *myTeam = &g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Unit *myUnit = NULL;
        if( myTeam->m_units.ValidIndex(m_id.GetUnitId()) ) 
        {
            myUnit = myTeam->m_units[ m_id.GetUnitId() ];
        }

        if( myUnit )
        {
            float size = 0.2f * pow(1.1f, myUnit->m_entities.Size() );
            size = min( size, 10.0f );

            Centipede *prev = NULL;

            for( int i = 0; i < myUnit->m_entities.Size(); ++i )
            {
                if( myUnit->m_entities.ValidIndex(i) )
                {
                    Centipede *centipede = (Centipede *) myUnit->m_entities[i];
                    centipede->m_size = size;
                    size /= 1.1f;
                    if( prev )
                    {
                        prev->m_prev = centipede->m_id;
                        centipede->m_next = prev->m_id;
                    }
                    prev = centipede;
                }
            }
        }
    }

    int health = m_stats[StatHealth];
    health *= m_size * 2;
    if( health < 0 ) health = 0;
    if( health > 255 ) health = 255;
    m_stats[StatHealth] = health;

    m_radius = m_size * 10.0f;
}


void Centipede::ChangeHealth( int _amount )
{
    float maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;


    bool dead = m_dead;    
    int oldHealthBand = 3 * (m_stats[StatHealth] / maxHealth);
    Entity::ChangeHealth( _amount );
    int newHealthBand = 3 * (m_stats[StatHealth] / maxHealth);

    if( newHealthBand < oldHealthBand )
    {
        // We just took some bad damage
        Panic( 2.0f + syncfrand(2.0f) );
    }

    if( m_dead && !dead )
    {
        // We just died
        Matrix34 transform( m_front, g_upVector, m_pos );
        transform.f *= m_size;
        transform.u *= m_size;
        transform.r *= m_size;

        g_explosionManager.AddExplosion( m_shape, transform );         
        
        Centipede *next = (Centipede *) g_app->m_location->GetEntitySafe( m_next, TypeCentipede );
        if( next ) next->m_prev.SetInvalid();

        m_next.SetInvalid();        
        m_prev.SetInvalid();
    }
}


void Centipede::Panic( float _time )
{
    if( m_panic <= 0.0f )
    {
        g_app->m_soundSystem->TriggerEntityEvent( this, "Panic" );
    }

    m_panic = max( _time, m_panic );

    if( m_next.IsValid() )
    {
        //
        // We're not the head, so pass on towards the head
        WorldObject *wobj = g_app->m_location->GetEntity( m_next );
        Centipede *centipede = (Centipede *) wobj;
        centipede->Panic( _time );
    }    
}


bool Centipede::Advance( Unit *_unit )
{
    DarwiniaReleaseAssert( _unit, "Centipedes must be created in a unit" );
    
    if( m_dead ) return AdvanceDead( _unit );
        
    m_onGround = true;
    m_lastAdvance = g_gameTime;
    
    bool recordPositionHistory = false;

    if( m_next.IsValid() )
    {
        //
        // We are trailing, so just follow the leader

        m_shape = s_shapeBody;

        Centipede *centipede = (Centipede *) g_app->m_location->GetEntitySafe( m_next, TypeCentipede );
        if( centipede && !centipede->m_dead )
        {
            if( centipede->m_linked ) recordPositionHistory = true;
            m_linked = centipede->m_linked && (m_positionHistory.Size() >= 2);
            Vector3 trailPos, trailVel;
            int numSteps = 1;
            if( centipede->m_id.GetIndex() > m_id.GetIndex() ) numSteps = 0;
            bool success = centipede->GetTrailPosition( trailPos, trailVel, numSteps );
            if( success )
            {
                m_pos = trailPos;
                m_vel = trailVel;
                m_front = m_vel;
                m_front.Normalise();
                m_panic = centipede->m_panic;
            }
        }
        else
        {
            m_next.SetInvalid();
        }
    }
    else
    {
        //
        // We are a leader, so look for enemies
        
        EatSpirits();
        m_shape = s_shapeHead;       
        m_linked = true;
        recordPositionHistory = true;

        if( m_panic > 0.0f )
        {
            m_targetEntity.SetInvalid();
            if( syncfrand(10.0f) < 5.0f )
            {
                SearchForRetreatPosition();
            }
            m_panic -= SERVER_ADVANCE_PERIOD;
        }
        else if( m_targetEntity.IsValid() )
        {
            WorldObject *target = g_app->m_location->GetEntity( m_targetEntity );
            if( target )
            {
                m_targetPos = target->m_pos;
                m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
            }
            else
            {
                m_targetEntity.SetInvalid();
            }
        }

        bool arrived = AdvanceToTargetPosition();
        if( arrived || m_targetPos == g_zeroVector )
        {
            bool found = false;
            if( !found ) found = SearchForTargetEnemy();
            if( !found ) found = SearchForSpirits();
            if( !found ) found = SearchForRandomPosition();
        }
    }

    
    //
    // Make sure we are roughly the right size

    float maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;
    float healthFraction = (float) m_stats[StatHealth] / maxHealth;

    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75f) * (1.0f - fabs(sinf(timeIndex))*0.8f) > healthFraction );

    float targetSize = 0.0f;
    if( !m_prev.IsValid() )
    {
        targetSize = 0.2f;                
    }
    else
    {
        Centipede *prev = (Centipede *) g_app->m_location->GetEntitySafe( m_prev, TypeCentipede );
        targetSize = prev->m_size * 1.1f;
        targetSize = min( targetSize, 1.0f );
    }

    if( fabs( targetSize - m_size ) > 0.01f )
    {
        m_size = m_size * 0.9f + targetSize * 0.1f; 
        float maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
        maxHealth *= m_size * 2;
        if( maxHealth < 0 ) maxHealth = 0;
        if( maxHealth > 255 ) maxHealth = 255;
        float newHealth = maxHealth * healthFraction;
        newHealth = max( newHealth, 0 );
        newHealth = min( newHealth, 255 );
        m_stats[StatHealth] = newHealth;
    }
           
    if( recordPositionHistory )
    {
        RecordHistoryPosition();
    }
    
    if (_unit)
	{		
		_unit->UpdateEntityPosition( m_pos, m_radius );	
	}


    Attack( m_pos );
   

    return false;
}


void Centipede::Attack( Vector3 const &_pos )
{
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( _pos.x, _pos.z, m_radius, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = (Entity *) g_app->m_location->GetEntity( id );        
        Vector3 pushVector = ( entity->m_pos - _pos );
        float distance = pushVector.Mag();       
        if( distance < m_radius )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );

            pushVector.SetLength( m_radius - distance );
                                            
            g_app->m_location->m_entityGrid->RemoveObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            entity->m_pos += pushVector;
            g_app->m_location->m_entityGrid->AddObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );

            entity->ChangeHealth( (m_radius - distance) * -10.0f );
        }
    }
}


void Centipede::EatSpirits()
{
    //
    // Are we already too big to eat spirits?

    int size = g_app->m_location->GetUnit( m_id )->NumAliveEntities();
    if( size > CENTIPEDE_MAXSIZE ) return;

    LList<int> m_eaten;

    //
    // Find all spirits that we could potentially eat

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *spirit = g_app->m_location->m_spirits.GetPointer(i);            

            if( spirit->m_state == Spirit::StateFloating )
            {
                Vector3 theVector = ( spirit->m_pos - m_pos );
                theVector.y = 0.0f;
                if( theVector.Mag() < CENTIPEDE_SPIRITEATRANGE )
                {
                    m_eaten.PutData( i );
                }
            }
        }
    }


    //
    // Swallow all spirits

    float eatChance = m_size / 2.0f;

    for( int i = 0; i < m_eaten.Size(); ++i )
    {        
        if( syncfrand(1.0f) < eatChance )
        {
            int eatenIndex = m_eaten[i];
            g_app->m_location->m_spirits.MarkNotUsed( eatenIndex );
            ++m_numSpiritsEaten;
            break;
        }
    }
    

    //
    // Now try to grow

    if( m_numSpiritsEaten >= CENTIPEDE_NUMSPIRITSTOREGROW )
    {
        //
        // Find the tail centipede

        Centipede *tail = this;
        while( true )
        {
            Centipede *centipede = (Centipede *) g_app->m_location->GetEntitySafe( tail->m_prev, TypeCentipede );
            if( !centipede ) break;
            tail = centipede;
        }


        //
        // Add one segment for every 3 spirits

        Team *myTeam = &g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Unit *myUnit = myTeam->m_units[ m_id.GetUnitId() ];

        while( m_numSpiritsEaten >= CENTIPEDE_NUMSPIRITSTOREGROW )
        {        
            int index;
            Centipede *centipede = (Centipede *) myUnit->NewEntity(&index);
            centipede->SetType( TypeCentipede );
            centipede->m_id.SetTeamId( m_id.GetTeamId() );
            centipede->m_id.SetUnitId( m_id.GetUnitId() );
            centipede->m_id.SetIndex( index );
            centipede->m_next = tail->m_id;    
            centipede->m_prev.SetInvalid();
            tail->m_prev = centipede->m_id;
                        
            centipede->m_pos = m_spawnPoint;
            centipede->m_size = tail->m_size;
            centipede->m_size = max( 0.2f, centipede->m_size );
            centipede->m_spawnPoint = m_spawnPoint;
            centipede->m_roamRange = m_roamRange;
            centipede->Begin();            

            g_app->m_location->m_entityGrid->AddObject( centipede->m_id, centipede->m_pos.x, centipede->m_pos.z, centipede->m_radius );
            g_app->m_soundSystem->TriggerEntityEvent( this, "Grow" );
            
            tail = centipede;
            m_numSpiritsEaten -= CENTIPEDE_NUMSPIRITSTOREGROW;
        }
    }
}


bool Centipede::SearchForRetreatPosition()
{
    float maxRange = CENTIPEDE_MAXSEARCHRANGE * m_size;

    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, maxRange, &numFound, m_id.GetTeamId() );

    WorldObjectId targetId;
    float bestDistance = 99999.9f;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        WorldObject *entity = g_app->m_location->GetEntity( id );
        float distance = ( entity->m_pos - m_pos ).Mag();
        if( distance < bestDistance )
        {
            bestDistance = distance;
            targetId = id;
        }
    }


    if( targetId.IsValid() )
    {
        WorldObject *obj = g_app->m_location->GetEntity( targetId );
        DarwiniaDebugAssert( obj );

        float distance = 50.0f;
        Vector3 retreatVector = ( m_pos - obj->m_pos ).Normalise();
        float angle = syncsfrand( M_PI * 1.0f );
        retreatVector.RotateAroundY( angle );
        m_targetPos = m_pos + retreatVector * distance;
        m_targetPos = PushFromObstructions( m_targetPos );
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        return true;
    }
     
    return false;
}


bool Centipede::SearchForTargetEnemy()
{
    float maxRange = CENTIPEDE_MAXSEARCHRANGE * m_size;
    float minRange = CENTIPEDE_MINSEARCHRANGE * m_size;

    WorldObjectId targetId = g_app->m_location->m_entityGrid->GetBestEnemy(
							m_pos.x, m_pos.z, minRange, maxRange, m_id.GetTeamId());

    if( targetId.IsValid() )
    {
        m_targetEntity = targetId;
        g_app->m_soundSystem->TriggerEntityEvent( this, "EnemySighted" );
        return true;
    }
    else
    {
        m_targetEntity.SetInvalid();
        return false;
    }
}


bool Centipede::SearchForSpirits()
{   
    //
    // Are we already too big to eat spirits?

    int size = g_app->m_location->GetUnit( m_id )->NumAliveEntities();
    if( size > CENTIPEDE_MAXSIZE ) return false;

    START_PROFILE(g_app->m_profiler, "SearchSpirits");
    Spirit *found = NULL;
    float nearest = 9999.9f;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            float theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= CENTIPEDE_MAXSEARCHRANGE &&
                theDist >= CENTIPEDE_MINSEARCHRANGE &&
                theDist < nearest &&
                s->m_state == Spirit::StateFloating )
            {                
                found = s;
                nearest = theDist;            
            }
        }
    }            

    if( found )
    {
        m_targetPos = found->m_pos;
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
    }    

	END_PROFILE(g_app->m_profiler, "SearchSpirits");
    return found;
}


bool Centipede::SearchForRandomPosition()
{
    float distToSpawnPoint = ( m_pos - m_spawnPoint ).Mag();
    float chanceOfReturn = ( distToSpawnPoint / m_roamRange );
    if( chanceOfReturn >= 1.0f || syncfrand(1.0f) <= chanceOfReturn )
    {
        // We have strayed too far from our spawn point
        // So head back there now
        Vector3 returnVector = m_spawnPoint - m_pos;
        returnVector.SetLength( 100.0f );
        m_targetPos = m_pos + returnVector;

    }
    else
    {
        float distance = 100.0f;
        float angle = syncsfrand(2.0f * M_PI);

        m_targetPos = m_pos + Vector3( sinf(angle) * distance,
                                       0.0f,
                                       cosf(angle) * distance );
        m_targetPos = PushFromObstructions( m_targetPos );

    }
    
    m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
    return true;
}


void Centipede::RecordHistoryPosition()
{    
    m_positionHistory.PutDataAtStart( m_pos );
    
    int maxHistorys = 3;

    for( int i = maxHistorys; i < m_positionHistory.Size(); ++i )
    {
        m_positionHistory.RemoveData(i);
    }
}


bool Centipede::GetTrailPosition( Vector3 &_pos, Vector3 &_vel, int _numSteps )
{
    if( m_positionHistory.Size() < 3 ) return false; 

    float timeSinceAdvance = g_gameTime - m_lastAdvance;

    Vector3 pos1 = *m_positionHistory.GetPointer(_numSteps+1);
    Vector3 pos2 = *m_positionHistory.GetPointer(_numSteps);
    _pos = pos1 + (pos2 - pos1) * (1.0f - m_size);    
    _vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;

    return true;
}


bool Centipede::AdvanceToTargetPosition()
{
    float amountToTurn = SERVER_ADVANCE_PERIOD * 3.0f;
    if( m_next.IsValid() ) amountToTurn *= 1.5f;
    Vector3 targetDir = (m_targetPos - m_pos).Normalise();
    Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();
    float speed = m_stats[StatSpeed];
        
    Vector3 oldPos = m_pos;
    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    

    //
    // Slow us down if we're going up hill
    // Speed up if going down hill

    float currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
    float nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    float factor = 1.0f - (currentHeight - nextHeight) / -10.0f;
    if( factor < 0.6f ) factor = 0.6f;
    if( factor > 1.0f ) factor = 1.0f;
    speed *= factor;
    
    newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;       
    newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );    

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = actualDir;
    
    if( m_targetPos.y < 0.0f )
    {
        // We're about to go into the water
        return true;
    }
    
    int nearestBuildingId = g_app->m_location->GetBuildingId( m_pos, m_front, 255, 150.0f );
    if( nearestBuildingId != -1 )
    {
        // We're on track to run into a building
        return true;
    }

        
    return ( m_pos - m_targetPos ).Mag() < 20.0f;
}


void Centipede::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Panic" );
    _list->PutData( "EnemySighted" );
    _list->PutData( "Grow" );
}


void Centipede::Render( float _predictionTime )
{       
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
 
    float maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;

    Shape *shape = m_shape;

    if( !m_dead && m_linked )
    {
        glDisable( GL_TEXTURE_2D );
        //RenderSphere( m_targetPos, 5.0f );

        Vector3 predictedFront = m_front;
        Vector3 predictedUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
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
}


bool Centipede::IsInView()
{
    return g_app->m_camera->SphereInViewFrustum( m_pos+m_centrePos, m_radius );
}


bool Centipede::RenderPixelEffect(float _predictionTime)
{
	Render(_predictionTime);

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );

    if( !m_dead && m_linked )
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
