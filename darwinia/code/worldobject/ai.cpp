#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/file_writer.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/hi_res_time.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "entity_grid.h"
#include "level_file.h"
#include "location_editor.h"
#include "global_world.h"

#include "worldobject/ai.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/darwinian.h"
#include "worldobject/blueprintstore.h"


AI::AI()
:   Entity(),
    m_timer(0.0f)
{
    SetType( TypeAI );

    
}


void AI::ChangeHealth( int _amount )
{
}


void AI::Begin()
{
    Entity::Begin();


    //
    // Rebuild the AI node grid thingy from scratch
    // This will occur once per AI, which isn't required, but hey.

    float startTime = GetHighResTime();

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *aiTarget = (AITarget *) building;
                aiTarget->RecalculateNeighbours();
            }
        }
    }


    //
    // Cull nonsense links
    // eg if link A -> B exists, and link B -> C exists, then don't allow
    // link A -> C unless it is much shorter distance

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *a = (AITarget *) building;
                for( int n = 0; n < a->m_neighbours.Size(); ++n )
                {
                    int cId = a->m_neighbours[n];
                    AITarget *c = (AITarget *) g_app->m_location->GetBuilding(cId);
                    DarwiniaDebugAssert( c && c->m_type == Building::TypeAITarget );
                    float distanceAtoC = a->IsNearTo( cId );

                    for( int x = 0; x < a->m_neighbours.Size(); ++x )
                    {
                        if( x != n )
                        {
                            int bId = a->m_neighbours[x];
                            AITarget *b = (AITarget *) g_app->m_location->GetBuilding( bId );
                            DarwiniaDebugAssert( b && b->m_type == Building::TypeAITarget );
                            float distanceAtoB = a->IsNearTo( bId );
                            float distanceBtoC = b->IsNearTo( cId );
                            if( distanceBtoC > 0.0f && 
                                distanceAtoC > (distanceAtoB + distanceBtoC) * 0.8f )
                            {
                                a->m_neighbours.RemoveData(n);
                                --n;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }


    float timeTaken = GetHighResTime() - startTime;
    DebugOut( "AI Node graph rebuilt in %dms\n", int( timeTaken * 1000.0f ) );
}


int AI::FindTargetBuilding( int _fromTargetId, int _fromTeamId )
{
    AITarget *fromBuilding = (AITarget *) g_app->m_location->GetBuilding(_fromTargetId);
    DarwiniaDebugAssert( fromBuilding && fromBuilding->m_type == Building::TypeAITarget );


    // Note by Chris
    // Although it makes sense to always head for the highest priority target,
    // in practice it makes the Darwinians cluster exclusively at crucial nodes, which is boring.
    // Introduce some randomness.
    // Make Darwinians move randomly some of the time.
    // Another note : Don't do this on the Rocket demo, as it makes the Red darwinians look stupid
    // when they walk away from the fight during a cutscene

    int id = -1;
    bool randomMovement = syncfrand() < 0.25f;
    
#ifdef DEMO2
    randomMovement = false;
#endif

    if( randomMovement )
    {
        int numNeighbours = fromBuilding->m_neighbours.Size();
		if( numNeighbours > 0 )
		{
			int neighbourIndex = syncrand() % numNeighbours;
			id = fromBuilding->m_neighbours[neighbourIndex];
		}
    }
    else
    {
        float bestPriority = fromBuilding->m_priority[_fromTeamId];

        for( int i = 0; i < fromBuilding->m_neighbours.Size(); ++i )
        {
            int toBuildingId = fromBuilding->m_neighbours[i];
            AITarget *target = (AITarget *) g_app->m_location->GetBuilding(toBuildingId);
            DarwiniaDebugAssert( target && target->m_type == Building::TypeAITarget );

            float thisPriority = target->m_priority[_fromTeamId];
            if( thisPriority > bestPriority )
            {
                bestPriority = thisPriority;
                id = toBuildingId;
            }
        }
    }

    return id;
}


int AI::FindNearestTarget( Vector3 const &_fromPos )
{
    float nearest = FLT_MAX;
    int id = -1;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *target = (AITarget *) building;
                float distance = ( target->m_pos - _fromPos ).Mag();
                if( distance < nearest )
                {
                    if( g_app->m_location->IsWalkable( _fromPos, target->m_pos, true ) )
                    {
                        id = building->m_id.GetUniqueId();
                        nearest = distance;                    
                    }
                }
            }
        }
    }
    
    return id;
}


bool AI::Advance( Unit *_unit )
{    
    //
    // Try to get Darwinians to stay near AI Targets
    // We can't do this for every darwinian every frame, so just do it for some

    Team *team = &g_app->m_location->m_teams[m_id.GetTeamId()];
    int numRemaining = team->m_others.Size() * 0.02f;
    numRemaining = max( numRemaining, 1 );

    while( numRemaining > 0 )
    {
        int index = syncrand() % team->m_others.Size();
        if( team->m_others.ValidIndex(index) )
        {
            Entity *entity = team->m_others[index];
            if( entity && entity->m_type == TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                if( darwinian->m_state == Darwinian::StateIdle ||
                    darwinian->m_state == Darwinian::StateWorshipSpirit ||
                    darwinian->m_state == Darwinian::StateWatchingSpectacle )
                {
                    Building *nearestTarget = g_app->m_location->GetBuilding( FindNearestTarget(darwinian->m_pos) );
                    if( nearestTarget )
                    {
                        float distance = ( nearestTarget->m_pos - darwinian->m_pos ).Mag();
                        if( distance > 70.0f )
                        {
                            Vector3 targetPos = nearestTarget->m_pos;
                            float positionError = 20.0f;
                            float radius = 20.0f+syncfrand(positionError);
	                        float theta = syncfrand(M_PI * 2);                
                            targetPos.x += radius * sinf(theta);
	                        targetPos.z += radius * cosf(theta);
                            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
                            darwinian->GiveOrders( targetPos );
                        }
                    }
                }
            }
        }

        --numRemaining;
    }


    //
    // Time to re-evaluate?

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer > 0.0f ) return false;
    m_timer = 1.0f;
    

    //
    // Look for buildings that are well defended

    LList<int> m_wellDefendedIds;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget &&
                building->m_id.GetTeamId() == m_id.GetTeamId() )
            {
                AITarget *target = (AITarget *) building;
                int idleCount = target->m_idleCount[m_id.GetTeamId()];
                int enemyCount = target->m_enemyCount[m_id.GetTeamId()];
                int friendCount = target->m_friendCount[m_id.GetTeamId()];

                if( idleCount > 30 && enemyCount < friendCount * 0.33f )
                {
                    m_wellDefendedIds.PutData(i);
                }
            }
        }
    }

    
    //
    // For each well defended building, choose a valid nearby target
    // and send some troops to it

    for( int i = 0; i < m_wellDefendedIds.Size(); ++i )
    {
        int buildingIndex = m_wellDefendedIds[i];
        AITarget *target = (AITarget *) g_app->m_location->m_buildings[ buildingIndex ];
        int numFriends = target->m_friendCount[ m_id.GetTeamId() ];
        int numIdle = target->m_idleCount[ m_id.GetTeamId() ];

        int targetBuildingId = FindTargetBuilding( target->m_id.GetUniqueId(), m_id.GetTeamId() );
        if( targetBuildingId != -1 )
        {
            AITarget *targetBuilding = (AITarget *) g_app->m_location->GetBuilding( targetBuildingId );
            int enemyCount = targetBuilding->m_enemyCount[ m_id.GetTeamId() ];
            float sendChance = 1.5f * (float) enemyCount / (float) numIdle;
            sendChance = max( sendChance, 0.6f );
            sendChance = min( sendChance, 1.0f );

            Vector3 targetPos = targetBuilding->m_pos;
            float positionError = 20.0f;
            float radius = 20.0f+syncfrand(positionError);
	        float theta = syncfrand(M_PI * 2);                
            targetPos.x += radius * sinf(theta);
	        targetPos.z += radius * cosf(theta);
            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);

            int numFound;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( target->m_pos.x, target->m_pos.z, 150.0f, &numFound, m_id.GetTeamId() );
            for( int j = 0; j < numFound; ++j )
            {
                WorldObjectId id = ids[j];
                Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( id, TypeDarwinian );
                if( darwinian && syncfrand(1.0f) < sendChance &&
                    (darwinian->m_state == Darwinian::StateIdle ||
                     darwinian->m_state == Darwinian::StateWorshipSpirit ||
                     darwinian->m_state == Darwinian::StateWatchingSpectacle ) )
                {
                    darwinian->GiveOrders( targetPos );
                }
            }

            target->RecountTeams();
        }
    }


    return false;
}


void AI::Render( float _predictionTime )
{
    if( g_app->m_editing )
    {
        RGBAColour teamCol = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;
 
        Vector3 pos = m_pos;
        pos.y = 400.0f;
        RenderSphere( pos, 20.0f, teamCol );

        int numGreen = g_app->m_location->m_teams[0].m_others.NumUsed();
        int numRed = g_app->m_location->m_teams[1].m_others.NumUsed();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.DrawText3DCentre( pos-Vector3(0,30,0), 25, "Green : %d", numGreen );
        g_editorFont.DrawText3DCentre( pos-Vector3(0,60,0), 25, "Red  : %d", numRed );
    }
}


// ============================================================================


AITarget::AITarget()
:   Building(),
    m_teamCountTimer(0.0f)
{
    m_type = Building::TypeAITarget;

    memset( m_friendCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_enemyCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_idleCount, 0, NUM_TEAMS*sizeof(int) );    
    memset( m_priority, 0, NUM_TEAMS*sizeof(float) );
    
    SetShape( g_app->m_resource->GetShape("aitarget.shp") );

    m_teamCountTimer = syncfrand( 2.0f );
}


void AITarget::RecalculateNeighbours()
{
    m_neighbours.Empty();
    
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget &&
                building != this )
            {
                float distance = ( building->m_pos - m_pos ).Mag();
                bool isWalkable = g_app->m_location->IsWalkable( m_pos, building->m_pos, true );
                if( distance <= AITARGET_LINKRANGE && isWalkable )
                {
                    m_neighbours.PutData( building->m_id.GetUniqueId() );
                }
            }
        }
    }
}


void AITarget::RecountTeams()
{
    memset( m_friendCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_enemyCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_idleCount, 0, NUM_TEAMS*sizeof(int) );
        
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetNeighbours( m_pos.x, m_pos.z, 100.0f, &numFound );
    for( int j = 0; j < numFound; ++j )
    {
        WorldObjectId id = ids[j];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity )
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( g_app->m_location->IsFriend( id.GetTeamId(), t ) ) 
                    ++m_friendCount[t];
                else
                    ++m_enemyCount[t];
            }

            if( entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                if( darwinian->m_state == Darwinian::StateIdle ||
                    darwinian->m_state == Darwinian::StateWorshipSpirit ||
                    darwinian->m_state == Darwinian::StateWatchingSpectacle )
                {
                    m_idleCount[id.GetTeamId()] ++;
                }
            }
        }
    }
}


void AITarget::RecalculateOwnership()
{
    int team = -1;
    int maxCount = 0;

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( m_friendCount[i] > maxCount )
        {
            team = i;
            maxCount = m_friendCount[i];
        }
    }

    if( team == -1 )
    {
        SetTeamId( 255 );
    }
    else
    {
        SetTeamId( team );
    }
}


float AITarget::IsNearTo( int _aiTargetId )
{
    for( int i = 0; i < m_neighbours.Size(); ++i )
    {
        int thisBuildingId = m_neighbours[i];
        if( thisBuildingId == _aiTargetId )
        {
            Building *building = g_app->m_location->GetBuilding(thisBuildingId);
            if( building && building->m_type == TypeAITarget )
            {
                return (m_pos - building->m_pos).Mag();
            }
        }
    }

    return -1.0f;
}


bool AITarget::Advance()
{   
    RecalculatePriority();


    //
    // Time to re-evaluate who is nearby?

    m_teamCountTimer -= SERVER_ADVANCE_PERIOD;
    if( m_teamCountTimer <= 0.0f )
    {
        m_teamCountTimer = 1.0f;
        RecountTeams();
        RecalculateOwnership();
    }

    return false;
}


void AITarget::RecalculatePriority()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_id.GetTeamId() == 255 )
        {
            // Owned by nobody, so grab it quick
            m_priority[t] = 0.9f;
        }
        else if( !g_app->m_location->IsFriend( m_id.GetTeamId(), t ) )
        {
            // Owned by the enemy
            m_priority[t] = 0.8f;

            if( m_friendCount[t] > m_enemyCount[t] * 0.5f )
            {
                // We are attacking, so continue to concentrate effort here
                m_priority[t] = 1.0f;
            }
        }
        else
        {
            // Owned by us
            if( m_enemyCount[t] > m_friendCount[t] * 0.5f )
            {
                // We are under attack, give us a very high priority
                m_priority[t] = 1.0f;
            }
            else
            {
                float prioritySum = 0.0f;
                for( int i = 0; i < m_neighbours.Size(); ++i )
                {
                    int neighbourId = m_neighbours[i];
                    AITarget *target = (AITarget *) g_app->m_location->GetBuilding(neighbourId);
                    if( target && target->m_type == TypeAITarget )
                    {
                        prioritySum += target->m_priority[t];
                    }
                }            
                prioritySum /= (float) (m_neighbours.Size() + 1);
                m_priority[t] = prioritySum;
            }
        }

        m_priority[t] += (float) m_neighbours.Size() * 0.01f;
        m_priority[t] = min( m_priority[t], 1.0f );        
    }
}


void AITarget::Render( float _predictionTime )
{    
    if( g_app->m_editing )
    {
        Building::Render( _predictionTime );
    }       

//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,80,0), 10.0f, "Green  : f%d e%d i%d", m_friendCount[0], m_enemyCount[0], m_idleCount[0] );
//    g_editorFont.DrawText3DCentre( m_pos+Vector3(0,70,0), 10.0f, "Red    : f%d e%d i%d", m_friendCount[1], m_enemyCount[1], m_idleCount[1] );

    //for( int t = 0; t < NUM_TEAMS; ++t )
//    {
//        int t = 1;
//        Team *team = &g_app->m_location->m_teams[t];
//        if( team->m_teamType != Team::TeamTypeUnused )
//        {            
//            glColor3ubv( team->m_colour.GetData() );
//            g_editorFont.DrawText3DCentre( m_pos+Vector3(0,60+t*15,0), 10.0f, "%2.2f", m_priority[t] );
//        }
//    }
}


void AITarget::RenderAlphas( float _predictionTime )
{   
    //Building::RenderAlphas( _predictionTime );
        

/*
        Vector3 height(0,20,0);
        glLineWidth( 2.0f );
        glShadeModel( GL_SMOOTH );
        glEnable( GL_BLEND );
    
        for( int i = 0; i < m_neighbours.Size(); ++i )
        {
            int buildingId = m_neighbours[i];
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building && building->m_type == TypeAITarget )
            {
                glBegin( GL_LINES );
                if( m_id.GetTeamId() == 255 )
                {
                    glColor4ub( 128, 128, 128, 255 );
                }
                else
                {
                    RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
                    glColor4ub( col.r, col.g, col.b, 255 );
                }
    
                glVertex3fv( (m_pos+height).GetData() );
    
                if( building->m_id.GetTeamId() == 255 )
                {
                    glColor4ub( 128, 128, 128, 255 );
                }
                else
                {
                    RGBAColour col = g_app->m_location->m_teams[ building->m_id.GetTeamId() ].m_colour;
                    glColor4ub( col.r, col.g, col.b, 255 );
                }
    
                glVertex3fv( (building->m_pos+height).GetData() );
                glEnd();
            }
        }
    
        glShadeModel( GL_FLAT );*/
    


#ifdef LOCATION_EDITOR
    if( g_app->m_editing &&
        g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
        g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
    {
        glDisable( GL_CULL_FACE );
        RenderSphere( m_pos, AITARGET_LINKRANGE, RGBAColour(255,100,100,255) );
    }
#endif
}


bool AITarget::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool AITarget::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


// ============================================================================


AISpawnPoint::AISpawnPoint()
:   Building(),
    m_entityType( Entity::TypeDarwinian ),
    m_count(20),
    m_period(60),
    m_timer(0.0f),
    m_numSpawned(0.0f),
    m_activatorId(-1),
    m_online(false),
    m_populationLock(-1),
    m_spawnLimit(-1),
	m_routeId(-1)
{
    m_type = TypeAISpawnPoint;
}


void AISpawnPoint::Initialise( Building *_template )
{
    Building::Initialise( _template );
    
    AISpawnPoint *spawnPoint = (AISpawnPoint *) _template;

    m_entityType = spawnPoint->m_entityType;
    m_count = spawnPoint->m_count;
    m_period = spawnPoint->m_period;
    m_activatorId = spawnPoint->m_activatorId;
    m_spawnLimit = spawnPoint->m_spawnLimit;
	m_routeId = spawnPoint->m_routeId;

    m_timer = m_period;//syncfrand(m_period);
}


bool AISpawnPoint::PopulationLocked()
{
    //
    // If we haven't yet looked up a nearby Pop lock,
    // do so now

    if( m_populationLock == -1 )
    {
        m_populationLock = -2;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeSpawnPopulationLock )
                {
                    SpawnPopulationLock *lock = (SpawnPopulationLock *) building;
                    float distance = ( building->m_pos - m_pos ).Mag();
                    if( distance < lock->m_searchRadius )
                    {
                        m_populationLock = lock->m_id.GetUniqueId();
                        break;
                    }
                }
            }
        }
    }


    //
    // If we are inside a Population Lock, query it now

    if( m_populationLock > 0 )
    {
        SpawnPopulationLock *lock = (SpawnPopulationLock *) g_app->m_location->GetBuilding( m_populationLock );
        if( lock && m_id.GetTeamId() != 255 &&
            lock->m_teamCount[ m_id.GetTeamId() ] >= lock->m_maxPopulation )
        {
            return true;
        }
    }


    return false;
}


bool AISpawnPoint::Advance()
{
    //
    // Is our activator building online?
    // (Assuming it exists)

    if( !m_online )
    {
        Building *building = g_app->m_location->GetBuilding( m_activatorId );
        if( building && building->m_type == TypeBlueprintStore )
        {
            BlueprintStore *bpStore = (BlueprintStore *) building;
            int numInfected = bpStore->GetNumInfected();
            int numClean = bpStore->GetNumClean();
            if( numClean < BLUEPRINTSTORE_NUMSEGMENTS && numInfected <= 2 )
            {
                m_online = true;
            }
        }
        else if( building && building->m_type == TypeBlueprintConsole )
        {
            BlueprintConsole *console = (BlueprintConsole *) building;
            if( console->m_id.GetTeamId() == 0 )
            {
                m_online = true;
            }
        }
        else
        {
            GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_activatorId, g_app->m_locationId );
            if( !gb ) m_online = true;
            if( gb && gb->m_online ) m_online = true;
        }
    }

    bool greenVictory = false;
    if( m_online )
    {
        LList <GlobalEventCondition *> *objectivesList = &g_app->m_location->m_levelFile->m_primaryObjectives;
        greenVictory = true;
	    for (int i = 0; i < objectivesList->Size(); ++i)
	    {
		    GlobalEventCondition *gec = objectivesList->GetData(i);
		    if (!gec->Evaluate())
		    {
			    greenVictory = false;
                break;
		    }
	    }    
    }

    if( m_online && !greenVictory && !PopulationLocked() && m_spawnLimit != 0)
    {
        //
        // Count down to next spawn batch

        if( m_timer > 0.0f )
        {
            m_timer -= SERVER_ADVANCE_PERIOD;
            if( m_timer <= 0.0f )
            {
                m_timer = -1.0f;
                m_numSpawned = 0;
            }
        }


        //
        // Spawn entities if we're in the middle of a batch

        if( m_timer <= 0.0f )
        {
            g_app->m_location->SpawnEntities( m_pos, m_id.GetTeamId(), -1, m_entityType, 1, g_zeroVector, 20.0f, 100.0f, m_routeId );
            ++m_numSpawned;
        
            if( m_numSpawned >= m_count )
            {
                m_timer = m_period + syncsfrand(m_period/3.0f);
                m_spawnLimit--;
            }
        }
    }

    return false;
}


void AISpawnPoint::RenderAlphas( float _predictionTime )
{    
    if( g_app->m_editing )
    {
        RGBAColour colour;
        if( m_id.GetTeamId() == 255 ) colour.Set( 100, 100, 100, 255 );
        else colour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;

        RenderSphere( m_pos, 10.0f, colour );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,30,0), 5.0f, "Spawn %d %s's", m_count, Entity::GetTypeName(m_entityType) );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,25,0), 5.0f, "Every %d seconds", m_period );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,20,0), 5.0f, "Next spawn in %d seconds", (int) m_timer );
    }
}


int AISpawnPoint::GetBuildingLink()
{
    return m_activatorId;
}


void AISpawnPoint::SetBuildingLink( int _buildingId )
{
    m_activatorId = _buildingId;
}


void AISpawnPoint::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_activatorId = atoi( _in->GetNextToken() );
    m_entityType = atoi( _in->GetNextToken() );
    m_count = atoi( _in->GetNextToken() );
    m_period = atoi( _in->GetNextToken() );
    if( _in->TokenAvailable() )
    {
        m_spawnLimit = atoi( _in->GetNextToken() );
		if( _in->TokenAvailable() )
		{
			m_routeId = atoi( _in->GetNextToken() );
		}
    }
}


void AISpawnPoint::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-6d %-6d %-6d %-6d %-6d %-6d", m_activatorId, m_entityType, m_count, m_period, m_spawnLimit, m_routeId );
}


bool AISpawnPoint::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool AISpawnPoint::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

