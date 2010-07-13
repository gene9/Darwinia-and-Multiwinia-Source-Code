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

#include "worldobject/centipede.h"

Shape *Centipede::s_shapeBody[NUM_TEAMS];
Shape *Centipede::s_shapeHead[NUM_TEAMS];


Centipede::Centipede()
:   Entity(),
    m_size(1.0),
    m_linked(false),
    m_panic(0.0),
    m_numSpiritsEaten(0),
    m_lastAdvance(0.0)
{
    m_type = TypeCentipede;

    /*if( !s_shapeBody )
    {
        s_shapeBody         = g_app->m_resource->GetShape( "centipede.shp" );
        s_shapeHead         = g_app->m_resource->GetShape( "centipedehead.shp" );
    }

    m_shape = s_shapeBody;*/
}


void Centipede::Begin()
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
            head = g_app->m_resource->GetShapeCopy( headshapename, false, false );
            ConvertShapeColoursToTeam( head, m_id.GetTeamId(), false);
            g_app->m_resource->AddShape( head, headfilename );
        }
        s_shapeHead[m_id.GetTeamId()] = head;

        Shape *body = g_app->m_resource->GetShape( bodyfilename, false );
        if( !body )
        {
            body = g_app->m_resource->GetShapeCopy( bodyshapename, false, false );
            ConvertShapeColoursToTeam( body, m_id.GetTeamId(), false );
            g_app->m_resource->AddShape( body, bodyfilename );
        }
        s_shapeBody[m_id.GetTeamId()] = body;
    }
    m_shape = s_shapeBody[ m_id.GetTeamId() ];

    Entity::Begin();
    m_onGround = true;
    
    if( !m_next.IsValid() )
    {
        //
        // Link every centipede in this unit into one long centipede
        
        Team *myTeam = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Unit *myUnit = NULL;
        if( myTeam->m_units.ValidIndex(m_id.GetUnitId()) ) 
        {
            myUnit = myTeam->m_units[ m_id.GetUnitId() ];
        }

        if( myUnit )
        {
            double size = 0.2 * iv_pow(1.1, myUnit->m_entities.Size() );
            size = min( size, 10.0 );

            Centipede *prev = NULL;

            for( int i = 0; i < myUnit->m_entities.Size(); ++i )
            {
                if( myUnit->m_entities.ValidIndex(i) )
                {
                    Centipede *centipede = (Centipede *) myUnit->m_entities[i];
                    centipede->m_size = size;
                    size /= 1.1;
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

    m_radius = m_size * 10.0;
}


bool Centipede::ChangeHealth( int _amount, int _damageType )
{
    double maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
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
        Panic( 2.0 + syncfrand(2.0) );
		if( g_app->IsSinglePlayer() )
		{
			Matrix34 pos( m_front, g_upVector, m_pos );
			g_explosionManager.AddExplosion( m_shape, pos );
		}
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

    return true;
}


void Centipede::Panic( double _time )
{
    if( m_panic <= 0.0 )
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
    AppReleaseAssert( _unit, "Centipedes must be created in a unit" );
    
    if( m_dead ) return AdvanceDead( _unit );
        
    m_onGround = true;
    m_lastAdvance = GetNetworkTime();
    
    bool recordPositionHistory = false;

    if( m_next.IsValid() )
    {
		//SyncRandLog( "m_next.IsValid()" );

        //
        // We are trailing, so just follow the leader

        m_shape = s_shapeBody[m_id.GetTeamId()];

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
/*
#ifdef USE_DIRECT3D
			if(g_deformEffect && !m_prev.IsValid())
			{
				g_deformEffect->AddTearing(m_pos,0.3);
				g_deformEffect->AddTearing(m_pos+m_vel*(0.5*SERVER_ADVANCE_PERIOD),0.3);
			}
#endif
*/
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
        m_shape = s_shapeHead[m_id.GetTeamId()];       
        m_linked = true;
        recordPositionHistory = true;

        if( m_panic > 0.0 )
        {
            m_targetEntity.SetInvalid();
            if( syncfrand(10.0) < 5.0 )
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
		//SyncRandLog( "arrived = %d", arrived );

        if( arrived || m_targetPos == g_zeroVector )
        {
            bool found = false;
            if( !found ) 
			{
				found = SearchForTargetEnemy();
				//SyncRandLog( "SearchForTargetEnemy() = %d", found );
			}
            if( !found ) 
			{
				found = SearchForSpirits();
				//SyncRandLog( "SearchForSpirits() = %d", found );
			}		
            if( !found ) 
			{
				found = SearchForRandomPosition();
				//SyncRandLog( "SearchForRandomPosition() = %d", found );
			}
        }
    }

    
    //
    // Make sure we are roughly the right size

    double maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;
    double healthFraction = (double) m_stats[StatHealth] / maxHealth;

    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
    m_renderDamaged = ( frand(0.75) * (1.0 - iv_abs(iv_sin(timeIndex))*0.8) > healthFraction );

    double targetSize = 0.0;
    if( !m_prev.IsValid() )
    {
        targetSize = 0.2;                
    }
    else
    {
        Centipede *prev = (Centipede *) g_app->m_location->GetEntitySafe( m_prev, TypeCentipede );
        if( prev )
        {
            targetSize = prev->m_size * 1.1;
            targetSize = min( targetSize, 1.0 );
        }
        else
        {
            targetSize = 0.2; 
            m_prev.SetInvalid();
        }
    }

    if( iv_abs( targetSize - m_size ) > 0.01 )
    {
        m_size = m_size * 0.9+ targetSize * 0.1; 
        double maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
        maxHealth *= m_size * 2;
        if( maxHealth < 0 ) maxHealth = 0;
        if( maxHealth > 255 ) maxHealth = 255;
        double newHealth = maxHealth * healthFraction;
        newHealth = max( newHealth, 0.0 );
        newHealth = min( newHealth, 255.0 );
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
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, _pos.x, _pos.z, m_radius, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = (Entity *) g_app->m_location->GetEntity( id );        
        Vector3 pushVector = ( entity->m_pos - _pos );
        double distance = pushVector.Mag();       
        if( distance < m_radius )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );

            pushVector.SetLength( m_radius - distance );
                                            
            g_app->m_location->m_entityGrid->RemoveObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            entity->m_pos += pushVector;
            g_app->m_location->m_entityGrid->AddObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );

            entity->ChangeHealth( (m_radius - distance) * -10.0 );
        }
    }
}

int Centipede::GetSize()
{
    int count = 1;
    if( m_prev.IsValid() )
    {
        Centipede *c = (Centipede *)g_app->m_location->GetEntitySafe( m_prev, Entity::TypeCentipede );
        while( c && c->m_prev.IsValid() )
        {
            count++;
            c = (Centipede *)g_app->m_location->GetEntitySafe( c->m_prev, Entity::TypeCentipede );
        }
    }
    return count;
}

void Centipede::EatSpirits()
{
    //
    // Are we already too big to eat spirits?

    int size = GetSize();
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
                theVector.y = 0.0;
                if( theVector.Mag() < CENTIPEDE_SPIRITEATRANGE )
                {
                    m_eaten.PutData( i );
                }
            }
        }
    }


    //
    // Swallow all spirits

    double eatChance = m_size / 2.0;

    for( int i = 0; i < m_eaten.Size(); ++i )
    {        
        if( syncfrand(1.0) < eatChance )
        {
            int eatenIndex = m_eaten[i];
            g_app->m_location->m_spirits.RemoveData( eatenIndex );
            ++m_numSpiritsEaten;
            break;
        }
    }
    

    //
    // Now try to grow

    int required = CENTIPEDE_NUMSPIRITSTOREGROW;
    if( g_app->Multiplayer() )
    {
        required = CENTIPEDE_NUMSPIRITSTOREGROW_MP;
    }

    if( m_numSpiritsEaten >= required )
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

        Team *myTeam = g_app->m_location->m_teams[ m_id.GetTeamId() ];
        Unit *myUnit = myTeam->m_units[ m_id.GetUnitId() ];

        int numRequiredToEat = CENTIPEDE_NUMSPIRITSTOREGROW;
        if( g_app->Multiplayer() ) numRequiredToEat = CENTIPEDE_NUMSPIRITSTOREGROW_MP;

        while( m_numSpiritsEaten >= numRequiredToEat )
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
            centipede->m_size = max( 0.2, centipede->m_size );
            centipede->m_spawnPoint = m_spawnPoint;
            centipede->m_roamRange = m_roamRange;
            centipede->Begin();            

            g_app->m_location->m_entityGrid->AddObject( centipede->m_id, centipede->m_pos.x, centipede->m_pos.z, centipede->m_radius );
            g_app->m_soundSystem->TriggerEntityEvent( this, "Grow" );
            
            tail = centipede;
            m_numSpiritsEaten -= numRequiredToEat;
        }
    }
}


bool Centipede::SearchForRetreatPosition()
{
    double maxRange = CENTIPEDE_MAXSEARCHRANGE * m_size;

    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, maxRange, &numFound, m_id.GetTeamId() );

    WorldObjectId targetId;
    double bestDistance = 99999.9;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        WorldObject *entity = g_app->m_location->GetEntity( id );
        double distance = ( entity->m_pos - m_pos ).Mag();
        if( distance < bestDistance )
        {
            bestDistance = distance;
            targetId = id;
        }
    }


    if( targetId.IsValid() )
    {
        WorldObject *obj = g_app->m_location->GetEntity( targetId );
        AppDebugAssert( obj );

        double distance = 50.0;
        Vector3 retreatVector = ( m_pos - obj->m_pos ).Normalise();
        double angle = syncsfrand( M_PI * 1.0 );
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
    double maxRange = CENTIPEDE_MAXSEARCHRANGE * m_size;
    double minRange = CENTIPEDE_MINSEARCHRANGE * m_size;

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

    int size = GetSize();
    if( size > CENTIPEDE_MAXSIZE ) return false;

    START_PROFILE( "SearchSpirits");
    Spirit *found = NULL;
    double nearest = 9999.9;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            double theDist = ( s->m_pos - m_pos ).Mag();

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

	END_PROFILE( "SearchSpirits");
    return found;
}


bool Centipede::SearchForRandomPosition()
{
    double distToSpawnPoint = ( m_pos - m_spawnPoint ).Mag();
    double chanceOfReturn = ( distToSpawnPoint / m_roamRange );
    if( chanceOfReturn >= 1.0 || syncfrand(1.0) <= chanceOfReturn )
    {
        // We have strayed too far from our spawn point
        // So head back there now
        Vector3 returnVector = m_spawnPoint - m_pos;
        returnVector.SetLength( 100.0 );
        m_targetPos = m_pos + returnVector;

    }
    else
    {
        double distance = 100.0;
        double angle = syncsfrand(2.0 * M_PI);

        m_targetPos = m_pos + Vector3( iv_sin(angle) * distance,
                                       0.0,
                                       iv_cos(angle) * distance );
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

    double timeSinceAdvance = GetNetworkTime() - m_lastAdvance;

    Vector3 pos1 = *m_positionHistory.GetPointer(_numSteps+1);
    Vector3 pos2 = *m_positionHistory.GetPointer(_numSteps);
    _pos = pos1 + (pos2 - pos1) * (1.0 - m_size);    
    _vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;

    return true;
}


bool Centipede::AdvanceToTargetPosition()
{
    double amountToTurn = SERVER_ADVANCE_PERIOD * 3.0;
    if( m_next.IsValid() ) amountToTurn *= 1.5;
    Vector3 targetDir = (m_targetPos - m_pos).Normalise();
    Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();
    double speed = m_stats[StatSpeed];
        
    Vector3 oldPos = m_pos;
    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    

    //
    // Slow us down if we're going up hill
    // Speed up if going down hill

    double currentHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( oldPos.x, oldPos.z );
    double nextHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    double factor = 1.0 - (currentHeight - nextHeight) / -10.0;
    if( factor < 0.6 ) factor = 0.6;
    if( factor > 1.0 ) factor = 1.0;
    speed *= factor;
    
    newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;       
    newPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );    

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = actualDir;
    
    if( m_targetPos.y < 0.0 )
    {
        // We're about to go into the water
        return true;
    }
    
    int nearestBuildingId = g_app->m_location->GetBuildingId( m_pos, m_front, 255, 150.0 );
    if( nearestBuildingId != -1 )
    {
        // We're on track to run into a building
        return true;
    }

        
    return ( m_pos - m_targetPos ).Mag() < 20.0;
}


void Centipede::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Panic" );
    _list->PutData( "EnemySighted" );
    _list->PutData( "Grow" );
}


void Centipede::Render( double _predictionTime )
{       
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( predictedPos.x, predictedPos.z );
 
    double maxHealth = EntityBlueprint::GetStat( TypeCentipede, StatHealth );
    maxHealth *= m_size * 2;
    if( maxHealth < 0 ) maxHealth = 0;
    if( maxHealth > 255 ) maxHealth = 255;

    Shape *shape = m_shape;

    if( !m_dead && m_linked )
    {
        glDisable( GL_TEXTURE_2D );
        //RenderSphere( m_targetPos, 5.0 );

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


bool Centipede::RenderPixelEffect(double _predictionTime)
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
