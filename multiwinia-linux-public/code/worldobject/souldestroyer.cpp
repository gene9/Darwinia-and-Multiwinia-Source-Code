#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/matrix34.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"
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

#include "worldobject/souldestroyer.h"

Shape *SoulDestroyer::s_shapeHead[NUM_TEAMS];
Shape *SoulDestroyer::s_shapeTail[NUM_TEAMS];
ShapeMarker *SoulDestroyer::s_tailMarker[NUM_TEAMS];

int SoulDestroyer::s_numSoulDestroyers = 0;


SoulDestroyer::SoulDestroyer()
:   Entity(),
    m_panic(0.0),
    m_retargetTimer(0.0)
{
    m_type = TypeSoulDestroyer;

    /*if( !s_shapeTail || !s_shapeHead )
    {
        s_shapeTail = g_app->m_resource->GetShape( "souldestroyertail.shp" );
        s_shapeHead = g_app->m_resource->GetShape( "souldestroyerhead.shp" );

        const char tailMarkerName[] = "MarkerTail";
        s_tailMarker = s_shapeHead->m_rootFragment->LookupMarker( tailMarkerName );
        AppReleaseAssert( s_tailMarker, "SoulDestroyer: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", tailMarkerName, s_shapeHead->m_name );
    }

    m_shape = s_shapeHead;*/

    double range = 10.0;
    for( int i = 0; i < SOULDESTROYER_MAXSPIRITS; ++i )
    {
        m_spiritPosition[i] = Vector3( sfrand(range), sfrand(range), sfrand(range) );
    }

	m_routeTriggerDistance = 50.0;
    s_numSoulDestroyers++;
}

SoulDestroyer::~SoulDestroyer()
{
    s_numSoulDestroyers--;
}

void SoulDestroyer::Begin()
{
    if( !s_shapeHead[m_id.GetTeamId()] || !s_shapeTail[m_id.GetTeamId()] )
    {
        char headfilename[256], headshapename[256], tailfilename[256], tailshapename[256];

        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        int colourId = team->m_lobbyTeam->m_colourId;
        int groupId = team->m_lobbyTeam->m_coopGroupPosition;

        strcpy( headshapename, "souldestroyerhead.shp" );
        strcpy( tailshapename, "souldestroyertail.shp");

        sprintf( headfilename, "%s_%d_%d", headshapename, colourId, groupId );
        sprintf( tailfilename, "%s_%d_%d", tailshapename, colourId, groupId );

        Shape *head = g_app->m_resource->GetShape( headfilename, false );
        if( !head )
        {
            head = g_app->m_resource->GetShapeCopy( headshapename, false );
            ConvertShapeColoursToTeam( head, m_id.GetTeamId(), false );
            g_app->m_resource->AddShape( head, headfilename );
        }
        s_shapeHead[m_id.GetTeamId()] = head;

        Shape *tail = g_app->m_resource->GetShape( tailfilename, false );
        if( !tail )
        {
            tail = g_app->m_resource->GetShapeCopy( tailshapename, false );
            ConvertShapeColoursToTeam( tail, m_id.GetTeamId(), false );
            g_app->m_resource->AddShape( tail, tailfilename );
        }
        s_shapeTail[m_id.GetTeamId()] = tail;

        const char tailMarkerName[] = "MarkerTail";
        s_tailMarker[m_id.GetTeamId()] = s_shapeHead[m_id.GetTeamId()]->m_rootFragment->LookupMarker( tailMarkerName );
        AppReleaseAssert( s_tailMarker[m_id.GetTeamId()], "SoulDestroyer: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", tailMarkerName, s_shapeHead[m_id.GetTeamId()]->m_name );
    }
    m_shape = s_shapeTail[ m_id.GetTeamId() ];

    Entity::Begin();

    m_up = g_upVector;
    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );

    // This line to make it a bit easier to hit them with missiles
    m_radius *= 1.5;
}


bool SoulDestroyer::ChangeHealth( int _amount, int _damageType )
{
    if( _damageType == DamageTypeLaser ) return false;

    if( !m_dead && _amount < 0 )
    {
        Entity::ChangeHealth(_amount);

        double fractionDead = 1.0 - (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( TypeSoulDestroyer, StatHealth );
        fractionDead = max( fractionDead, 0.5 );
        fractionDead = min( fractionDead, 1.0 );
        if( m_dead ) fractionDead = 1.0;

        Panic( 2.0 + syncfrand(2.0) );

        Vector3 predictedRight = m_up ^ m_front;
        Vector3 predictedFront = predictedRight ^ m_up;
        Matrix34 transform(predictedFront, m_up, m_pos);

		g_explosionManager.AddExplosion( m_shape, transform, fractionDead );

        if( fractionDead )
        {
            // We just died
            for( int i = 1; i < m_positionHistory.Size(); i+=1 )
            {
                Vector3 pos1 = *m_positionHistory.GetPointer(i);
                Vector3 pos2 = *m_positionHistory.GetPointer(i-1);

                Vector3 pos = pos1 + (pos2 - pos1);
                Vector3 front = (pos2 - pos1).Normalise();
                Vector3 right = front ^ g_upVector;
                Vector3 up = right ^ front;

                double scale = 1.0 - ( (double) i / (double) m_positionHistory.Size() );
                scale *= 1.5;
                if( i == m_positionHistory.Size()-1 )   scale = 0.8;            
                scale = max( scale, 0.5 );
        
                Matrix34 tailMat( front, up, pos );            
                tailMat.u *= scale;
                tailMat.r *= scale;
                tailMat.f *= scale;

                g_explosionManager.AddExplosion( m_shape, tailMat, 1.0 );
            }            
        }
    }

    return true;
}


bool SoulDestroyer::Advance( Unit *_unit )
{
    if( m_dead ) return AdvanceDead( _unit );
        
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
            double distance = (target->m_pos - m_pos).Mag();
            m_targetPos = target->m_pos;            

            if( distance > SOULDESTROYER_MAXSEARCHRANGE )
            {
                m_targetEntity.SetInvalid();
            }
        }
        else
        {
            m_targetEntity.SetInvalid();
        }
    }

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;

    bool arrived = AdvanceToTargetPosition();
    if( arrived || m_targetPos == g_zeroVector || m_retargetTimer < 0.0)
    {
        m_retargetTimer = 5.0;
        bool found = false;
        if( !found && m_panic < 0.1 ) found = SearchForTargetEnemy();
        if( !found ) found = SearchForRandomPosition();
    }

    RecordHistoryPosition();

    if( m_panic < 0.1 ) Attack( m_pos );

#ifdef USE_DIRECT3D
	if(g_deformEffect)
	{
		Vector3* pos1 = m_positionHistory.GetPointer(m_positionHistory.Size()-1);
		Vector3* pos2 = m_positionHistory.GetPointer(m_positionHistory.Size()-2);
		//if(pos1) g_deformEffect->AddTearing(*pos1,0.4);
		//if(pos1 && pos2) g_deformEffect->AddTearing((*pos1+*pos2)*0.5,0.4);
		if(pos1 && pos2) g_deformEffect->AddTearingPath(*pos1,*pos2,0.4);
	}
#endif

	return Entity::Advance(_unit);
}


void SoulDestroyer::Attack( Vector3 const &_pos )
{
    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, _pos.x, _pos.z, SOULDESTROYER_DAMAGERANGE, &numFound, m_id.GetTeamId() );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = (Entity *) g_app->m_location->GetEntity( id );        
        bool killed = false;

        Vector3 pushVector = ( entity->m_pos - _pos );
        double distance = pushVector.Mag();       
        if( distance < SOULDESTROYER_DAMAGERANGE )
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );

            pushVector.SetLength( SOULDESTROYER_DAMAGERANGE - distance );
                                        
            g_app->m_location->m_entityGrid->RemoveObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            entity->m_pos += pushVector;
            g_app->m_location->m_entityGrid->AddObject( id, entity->m_pos.x, entity->m_pos.z, entity->m_radius );
            
            bool dead = entity->m_dead;
            entity->ChangeHealth( (SOULDESTROYER_DAMAGERANGE - distance) * -50.0 );            
            if( !dead && entity->m_dead ) killed = true;
        }

        if( killed )
        {
            // Eat the spirit
            int spiritIndex = g_app->m_location->GetSpirit( id );
            if( spiritIndex != -1 )
            {
                g_app->m_location->m_spirits.RemoveData( spiritIndex );
                if( m_spirits.NumUsed() < SOULDESTROYER_MAXSPIRITS )
                {
                    m_spirits.PutData( (double) GetHighResTime() );
                }
                else
                {
                    // Doesnt need to be sync safe
                    int index = AppRandom() % SOULDESTROYER_MAXSPIRITS;
                    m_spirits.PutData( (double) GetHighResTime(), index );
                }
            }
            
			if(entity->m_type == TypeDarwinian )
            {
				// Create a zombie
				Zombie *zombie = new Zombie();
				zombie->m_pos = entity->m_pos;
				zombie->m_front = entity->m_front;
				zombie->m_up = g_upVector;
				zombie->m_up.RotateAround( zombie->m_front * syncsfrand(1) );
				zombie->m_vel = m_vel * 0.5;
				zombie->m_vel.y = 20.0 + syncfrand(25.0);
				int index = g_app->m_location->m_effects.PutData( zombie );
				zombie->m_id.Set( id.GetTeamId(), UNIT_EFFECTS, index, -1 );
				zombie->m_id.GenerateUniqueId();
			}
        }
    } 
}


void SoulDestroyer::Panic( double _time )
{
    if( m_panic <= 0.0 )
    {
        g_app->m_soundSystem->TriggerEntityEvent( this, "Panic" );
    }

    m_panic = max( _time, m_panic );
}


bool SoulDestroyer::SearchForRetreatPosition()
{
    WorldObjectId targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 0.0, SOULDESTROYER_MAXSEARCHRANGE, m_id.GetTeamId() );
    
    if( targetId.IsValid() )
    {
        WorldObject *obj = g_app->m_location->GetEntity( targetId );
        AppDebugAssert( obj );

        double distance = 50.0;
        Vector3 retreatVector = ( m_pos - obj->m_pos ).Normalise();
        double angle = syncsfrand( M_PI * 1.0 );
        retreatVector.RotateAroundY( angle );
        m_targetPos = m_pos + retreatVector * distance;
        m_targetPos.y = min( m_targetPos.y, 300.0 );
        return true;
    }
     
    return false;
}


bool SoulDestroyer::SearchForTargetEnemy()
{
	if( m_routeId != -1 )
	{
		// dont attack darwinians if on a pre-set route
		return false;
	}
    // If we are too close to the ground, we MUST take off
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( iv_abs( m_pos.y - landHeight ) < 50.0 ) 
    {
        m_targetEntity.SetInvalid();
        return false;
    }

    // If we are too high up we don't see any enemies
//    if( m_pos.y > landHeight + 100.0 )
//    {
//        m_targetEntity.SetInvalid();
//        return false;
//    }

    double maxRange = SOULDESTROYER_MAXSEARCHRANGE;
    double minRange = SOULDESTROYER_MINSEARCHRANGE;

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


bool SoulDestroyer::SearchForRandomPosition()
{
    Vector3 toSpawnPoint = ( m_pos - m_spawnPoint );
    toSpawnPoint.y = 0.0;
    double distToSpawnPoint = toSpawnPoint.Mag();
    double chanceOfReturn = ( distToSpawnPoint / m_roamRange );
    if( chanceOfReturn >= 1.0 || syncfrand(1.0) <= chanceOfReturn )
    {
        // We have strayed too far from our spawn point
        // So head back there now       
        Vector3 targetPos = m_spawnPoint;
        targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );
        targetPos.y += 100.0 + syncsfrand( 100.0 );
        
        Vector3 returnVector = ( targetPos - m_pos );
        returnVector.SetLength( 160.0 );
        m_targetPos = m_pos + returnVector;
    }
    else
    {
        double distance = 160.0;
        double angle = syncsfrand(2.0 * M_PI);

        m_targetPos = m_pos + Vector3( iv_sin(angle) * distance,
                                       0.0,
                                       iv_cos(angle) * distance );  
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        m_targetPos.y += (100.0 + syncsfrand( 100.0 ));        
    }
    
    return true;
}


void SoulDestroyer::RecordHistoryPosition()
{    
    Matrix34 mat( m_front, m_up, m_pos );
    Vector3 tailPos = s_tailMarker[m_id.GetTeamId()]->GetWorldMatrix(mat).pos;
    m_positionHistory.PutDataAtStart( tailPos );
    
    //int maxHistorys = 11;
    int maxHistorys = m_roamRange / 30.0;
    maxHistorys = max( 9, maxHistorys );
    maxHistorys = min( 25, maxHistorys );

    for( int i = maxHistorys; i < m_positionHistory.Size(); ++i )
    {
        m_positionHistory.RemoveData(i);
    }
}


bool SoulDestroyer::GetTrailPosition( Vector3 &_pos, Vector3 &_vel )
{
    if( m_positionHistory.Size() < 2 ) return false; 

    Vector3 pos1 = *m_positionHistory.GetPointer(1);
    Vector3 pos2 = *m_positionHistory.GetPointer(0);

    _pos = pos1 + (pos2 - pos1);
    _vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;

    return true;
}


bool SoulDestroyer::AdvanceToTargetPosition()
{
    double amountToTurn = SERVER_ADVANCE_PERIOD * 2.0;
    Vector3 targetDir = (m_targetPos - m_pos).Normalise();
    if( !m_targetEntity.IsValid() )
    {
        Vector3 right1 = m_front ^ m_up;    
        targetDir.RotateAround( right1 * iv_sin(GetNetworkTime() * 6.0) * 1.5 );
    }

    // Look ahead to see if we're about to hit the ground
    Vector3 forwardPos = m_pos + targetDir * 50.0;
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(forwardPos.x, forwardPos.z);
    if( forwardPos.y <= landHeight )
    {
        targetDir = g_upVector;
    }

    Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();
    double speed = m_stats[StatSpeed];
    speed = 130.0;
    
    Vector3 oldPos = m_pos;
    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    //newPos.y = max( newPos.y, landHeight );
    
    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;       
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = actualDir;
                
    Vector3 right = m_front ^ g_upVector;
    m_up = right ^ m_front;
    
    return ( m_pos - m_targetPos ).Mag() < 40.0;
}


void SoulDestroyer::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "EnemySighted" );
    _list->PutData( "Panic" );
}


void SoulDestroyer::RenderShapes( double _predictionTime )
{    
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;    
    Vector3 predictedFront = m_front;
    Vector3 predictedUp = m_up;
    Vector3 predictedRight = predictedUp ^ predictedFront;
    predictedFront = predictedRight ^ predictedUp;
    predictedFront.Normalise();
    predictedUp.Normalise();
	Matrix34 mat(predictedFront, predictedUp, predictedPos);

    glEnable( GL_NORMALIZE );
    glDisable( GL_TEXTURE_2D );

    g_app->m_renderer->SetObjectLighting();
    
    s_shapeHead[m_id.GetTeamId()]->Render(_predictionTime, mat);   

    
    for( int i = 1; i < m_positionHistory.Size(); i+=1 )
    {
        Vector3 pos1 = *m_positionHistory.GetPointer(i);
        Vector3 pos2 = *m_positionHistory.GetPointer(i-1);

        Vector3 pos = pos1 + (pos2 - pos1);
        Vector3 front = (pos2 - pos1).Normalise();
        Vector3 right = front ^ g_upVector;
        Vector3 up = right ^ front;
        Vector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
        pos += vel * _predictionTime;

        double scale = 1.0 - ( (double) i / (double) m_positionHistory.Size() );
        scale *= 1.5;
        if( i == m_positionHistory.Size()-1 )   scale = 0.8;            
        scale = max( scale, 0.5 );
        
        Matrix34 tailMat( front, up, pos );            
        tailMat.u *= scale;
        tailMat.r *= scale;
        tailMat.f *= scale;
        
        s_shapeTail[m_id.GetTeamId()]->Render(_predictionTime, tailMat );
    }

    g_app->m_renderer->UnsetObjectLighting();

    glDisable( GL_NORMALIZE );
}


void SoulDestroyer::RenderShapesForPixelEffect( double _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;    
    Vector3 predictedFront = m_front;
    Vector3 predictedUp = m_up;
    Vector3 predictedRight = predictedUp ^ predictedFront;
    predictedFront = predictedRight ^ predictedUp;
	Matrix34 mat(predictedFront, predictedUp, predictedPos);

    g_app->m_renderer->MarkUsedCells(s_shapeHead[m_id.GetTeamId()], mat);   
    
    for( int i = 1; i < m_positionHistory.Size(); i+=1 )
    {
        Vector3 pos1 = *m_positionHistory.GetPointer(i);
        Vector3 pos2 = *m_positionHistory.GetPointer(i-1);

        Vector3 pos = pos1 + (pos2 - pos1);
        Vector3 front = (pos2 - pos1).Normalise();
        Vector3 right = front ^ g_upVector;
        Vector3 up = right ^ front;
        Vector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
        pos += vel * _predictionTime;

        double scale = 1.0 - ( (double) i / (double) m_positionHistory.Size() );
        scale *= 1.5;
        if( i == m_positionHistory.Size()-1 )   scale = 0.8;            
        scale = max( scale, 0.5 );
        
        Matrix34 tailMat( front, up, pos );            
        tailMat.u *= scale;
        tailMat.r *= scale;
        tailMat.f *= scale;
        
        g_app->m_renderer->MarkUsedCells(s_shapeTail[m_id.GetTeamId()], tailMat);
    }
}


void SoulDestroyer::Render( double _predictionTime )
{        
    _predictionTime -= SERVER_ADVANCE_PERIOD;

    if( !m_dead )
    {
        glDisable( GL_TEXTURE_2D );
        
        Vector3 predictedPos = m_pos + m_vel * _predictionTime;
        Vector3 predictedFront = m_front;
        Vector3 predictedUp = m_up;
        Vector3 predictedRight = predictedUp ^ predictedFront;
        predictedFront = predictedRight ^ predictedUp;
	    Matrix34 mat(predictedFront, predictedUp, predictedPos);
             
//        RenderSphere( m_targetPos, 5.0 );
//        RenderSphere( predictedPos, 300.0 );
//        RenderArrow( m_pos, m_pos+predictedFront * 200.0, 3.0, RGBAColour(0,255,0) );
//        RenderArrow( m_pos, m_pos+predictedUp * 100.0, 2.0, RGBAColour(255,0,0) );
//        RenderArrow( m_pos, m_pos+predictedRight * 100.0, 2.0, RGBAColour(0,0,255) );

        RenderShapes( _predictionTime );


        //
        // Render shadows

        BeginRenderShadow();
        RenderShadow( predictedPos, 50.0 );

        for( int i = 1; i < m_positionHistory.Size(); i+=1 )
        {
            Vector3 pos1 = *m_positionHistory.GetPointer(i);
            Vector3 pos2 = *m_positionHistory.GetPointer(i-1);

            Vector3 pos = pos1 + (pos2 - pos1);
            Vector3 vel = (pos2 - pos1) / SERVER_ADVANCE_PERIOD;
            pos += vel * _predictionTime;

            double scale = 1.0 - ( (double) i / (double) m_positionHistory.Size() );
            scale *= 1.5;
            if( i == m_positionHistory.Size()-1 )   scale = 0.8;            
            scale = max( scale, 0.5 );

            RenderShadow( pos, scale*20.0 );
        }
        
        EndRenderShadow();


        //
        // Render our spirits

        glDisable       ( GL_DEPTH_TEST );
        glDisable       ( GL_CULL_FACE );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_BLEND );
        glDepthMask     ( false );
        glBegin         ( GL_QUADS );

        double timeNow = GetHighResTime();
        for( int i = 0; i < m_spirits.Size(); ++i )
        {
            if( m_spirits.ValidIndex(i) )
            {
                double birthTime = m_spirits[i];
                if( timeNow >= birthTime + 60.0 )
                {
                    m_spirits.RemoveData(i);
                }
                else
                {
                    double alpha = 1.0 - (timeNow - m_spirits[i]) / 60.0;
                    alpha = min( alpha, 1.0 );
                    alpha = max( alpha, 0.0 );
                    Vector3 pos = m_pos + m_spiritPosition[i];
                    pos += m_vel * _predictionTime;
                    RenderSpirit( pos, alpha );
                }
            }
        }

        glEnd			();
        glDepthMask     ( true );
        glDisable       ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glEnable        ( GL_CULL_FACE );
        glEnable        ( GL_DEPTH_TEST );


        //glColor4f( 1.0, 1.0, 1.0, 1.0 );
        //g_editorFont.DrawText3DCentre( predictedPos+Vector3(0,50,0), 20, "%2.2", m_panic );
    }
}


void SoulDestroyer::RenderSpirit( Vector3 const &_pos, double _alpha )
{
    Vector3 pos = _pos;

    int innerAlpha = 200 * _alpha;
    int outerAlpha = 100 * _alpha ;
    int spiritInnerSize = 4 * _alpha;
    int spiritOuterSize = 12 * _alpha;

    RGBAColour colour( 100, 50, 50 );
    double distToParticle = (g_app->m_camera->GetPos() - pos).Mag();			

    double size = spiritInnerSize / iv_sqrt(iv_sqrt(distToParticle));
    glColor4ub(colour.r, colour.g, colour.b, innerAlpha );            

    //glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    //glEnd();    

    size = spiritOuterSize / iv_sqrt(iv_sqrt(distToParticle));
    glColor4ub(colour.r, colour.g, colour.b, outerAlpha );            
    //glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    //glEnd();    
}


bool SoulDestroyer::RenderPixelEffect ( double _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;

    if( !m_dead )
    {
        RenderShapes( _predictionTime );
	    RenderShapesForPixelEffect(_predictionTime);
        return true;
    }

    return false;
}

void SoulDestroyer::SetWaypoint( const Vector3 _waypoint )
{
	m_targetPos = _waypoint;
}


Zombie::Zombie()
:   WorldObject(),
    m_life(0.0)
{
    m_positionOffset = syncfrand(10.0);
    m_xaxisRate = syncfrand(2.0);
    m_yaxisRate = syncfrand(2.0);
    m_zaxisRate = syncfrand(2.0);
    
    m_type = EffectZombie;
}


bool Zombie::Advance()
{
    m_life += SERVER_ADVANCE_PERIOD;

    if( m_life > 600.0 )
    {
        return true;
    }

    m_vel *= 0.9;

    //
    // Make me hover around a bit

    m_positionOffset += SERVER_ADVANCE_PERIOD;
    m_xaxisRate += syncsfrand(1.0);
    m_yaxisRate += syncsfrand(1.0);
    m_zaxisRate += syncsfrand(1.0);
    if( m_xaxisRate > 2.0 ) m_xaxisRate = 2.0;
    if( m_xaxisRate < 0.0 ) m_xaxisRate = 0.0;
    if( m_yaxisRate > 2.0 ) m_yaxisRate = 2.0;
    if( m_yaxisRate < 0.0 ) m_yaxisRate = 0.0;
    if( m_zaxisRate > 2.0 ) m_zaxisRate = 2.0;
    if( m_zaxisRate < 0.0 ) m_zaxisRate = 0.0;
    
    m_hover.x = iv_sin( m_positionOffset ) * m_xaxisRate;
    m_hover.y = iv_sin( m_positionOffset ) * m_yaxisRate;
    m_hover.z = iv_sin( m_positionOffset ) * m_zaxisRate;            

    m_pos += m_hover * SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        
    return false;
}


void Zombie::Render( double _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    predictedPos += m_hover * _predictionTime;

    Vector3 predictedFront = m_front;
    Vector3 predictedUp = m_up;
    Vector3 predictedRight = predictedFront ^ predictedUp;

    double size = 5.0;

    double alpha = 1.0 - (m_life/10.0);
    alpha = max( 0.1, alpha );
    alpha = min( 0.7, alpha );

    double outerAlpha = (0.7-alpha) * 0.1;

    double timeRemaining = 600.0 - m_life;
    if( timeRemaining < 100.0 )
    {
        alpha *= timeRemaining * 0.01;
        outerAlpha *= timeRemaining * 0.01;
    }

    glDisable       ( GL_CULL_FACE );
    glColor4f       ( 0.9, 0.9, 1.0, alpha );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/ghost.bmp" ) );

    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (predictedPos - size*predictedRight - size*predictedUp).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (predictedPos + size*predictedRight - size*predictedUp).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (predictedPos + size*predictedRight + size*predictedUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (predictedPos - size*predictedRight + size*predictedUp).GetData() );
    glEnd();

    size *= 2.0;
    glColor4f       ( 0.9, 0.9, 1.0, outerAlpha );
    glBegin( GL_QUADS );
        glTexCoord2i(0,0);      glVertex3dv( (predictedPos - size*predictedRight - size*predictedUp).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (predictedPos + size*predictedRight - size*predictedUp).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (predictedPos + size*predictedRight + size*predictedUp).GetData() );
        glTexCoord2i(0,1);      glVertex3dv( (predictedPos - size*predictedRight + size*predictedUp).GetData() );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );
}

