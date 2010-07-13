#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"
#include "lib/preferences.h"

#include "app.h"
#include "team.h"
#include "location.h"
#include "entity_grid.h"
#include "level_file.h"
#include "location_editor.h"
#include "global_world.h"
#include "obstruction_grid.h"
#include "unit.h"
#include "taskmanager.h"
#include "multiwinia.h"
#include "gametimer.h"
#include "multiwiniahelp.h"

#include "worldobject/ai.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/darwinian.h"
#include "worldobject/blueprintstore.h"
#include "worldobject/radardish.h"
#include "worldobject/officer.h"
#include "worldobject/statue.h"
#include "worldobject/multiwiniazone.h"
#include "worldobject/rocket.h"
#include "worldobject/aiobjective.h"
#include "worldobject/wallbuster.h"
#include "worldobject/laserfence.h"
#include "worldobject/armour.h"

AI *AI::s_ai[NUM_TEAMS];


AI::AI()
:   Entity(),
    m_timer(0.0),
	m_radarReposTimer(10.0),
    m_currentObjective(-1),
    m_tutorialCentreZoneCaptured(false)
{
    SetType( TypeAI );
    Entity::s_entityPopulation--;   // AIs dont increase the population, and they never die

	m_radarReposTimer = syncfrand(10.0); // stop the ai from all moving radars at the same time    
	if( g_app->Multiplayer() )
	{
		if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy )
		{
			m_timer = syncfrand(20.0);
		}
		else if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard )
		{
			m_timer = syncfrand(10.0);
		}
		else
		{
			m_timer = syncfrand(1.0);
		}
	}
}

AI::~AI()
{
    m_rrObjectiveList.EmptyAndDelete();
}


bool AI::ChangeHealth( int _amount, int _damageType )
{
    return false;
}


void AI::Begin()
{
    Entity::Begin();

    s_ai[m_id.GetTeamId()] = this;

    //
    // Rebuild the AI node grid thingy from scratch
    // This will occur once per AI, which isn't required, but hey.

    double startTime = GetHighResTime();

    //g_app->m_location->RecalculateAITargets();

    double timeTaken = GetHighResTime() - startTime;
    AppDebugOut( "AI Node graph rebuilt in %dms\n", int( timeTaken * 1000.0 ) );
}


int AI::FindTargetBuilding( int _fromTargetId, int _fromTeamId )
{
    AITarget *fromBuilding = (AITarget *) g_app->m_location->GetBuilding(_fromTargetId);
    AppDebugAssert( fromBuilding && fromBuilding->m_type == Building::TypeAITarget );


    // Note by Chris
    // Although it makes sense to always head for the highest priority target,
    // in practice it makes the Darwinians cluster exclusively at crucial nodes, which is boring.
    // Introduce some randomness.
    // Make Darwinians move randomly some of the time.
    // Another note : Don't do this on the Rocket demo, as it makes the Red darwinians look stupid
    // when they walk away from the fight during a cutscene

    int id = -1;
    bool randomMovement = false;

	if( g_app->Multiplayer() ) 
    {
        if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ||
            g_app->m_multiwiniaTutorial )
        {
            randomMovement = true;
        }
        else if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard )
        {
            randomMovement = syncfrand(1) < 0.5;
        }
        else
        {
            randomMovement = false;
        }

        if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault ||
            g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot ) 
        {
            randomMovement = false;
        }

    }
    else
    {
        randomMovement = syncfrand(1) < 0.25;
    }
    
	if( g_app->m_gameMode == App::GameModePrologue )
	{
		randomMovement = false;
	}

    if( randomMovement )
    {
        int numNeighbours = fromBuilding->m_neighbours.Size();
		if( numNeighbours > 0 )
		{
			int neighbourIndex = syncrand() % numNeighbours;
            int sanity = 0;
            while( true )
            {
                sanity++;
                if( sanity > 100 ) return -1;
                if( g_app->m_multiwiniaTutorial )
                {
                    if( g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial1 )
                    {
                        AITarget *target = (AITarget *)g_app->m_location->GetBuilding( fromBuilding->m_neighbours[neighbourIndex]->m_neighbourId );
                        if( target && target->m_type == Building::TypeAITarget )
                        {
                            if( !m_tutorialCentreZoneCaptured )
                            {
                                int scoreZoneId = target->m_scoreZone;
                                if( scoreZoneId == 9 ) 
                                {
                                    MultiwiniaZone *zone = (MultiwiniaZone*)g_app->m_location->GetBuilding(scoreZoneId);
                                    if( zone && zone->m_type == Building::TypeMultiwiniaZone && zone->m_id.GetTeamId() == 255 ) 
                                    {
                                        neighbourIndex = syncrand() % numNeighbours;
                                        continue;
                                    }
                                    else
                                    {
                                        m_tutorialCentreZoneCaptured = true;
                                    }
                                }
                                else if( ( target->IsImportant() && target->m_id.GetTeamId() != 255 ) ||
                                         target->m_id.GetUniqueId() == 41 ||
                                         target->m_crateTarget )
                                {
                                    neighbourIndex = syncrand() % numNeighbours;
                                    continue;
                                }
                            }
                        }
                    }
                    else
                    {
                        if( !g_app->m_multiwiniaHelp->m_tutorial2AICanAct ) 
                        {
                            return id;
                        }
                        else if( !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
                        {
                            if( m_id.GetTeamId() == 1 ) return id;
                            AITarget *target = (AITarget *)g_app->m_location->GetBuilding( fromBuilding->m_neighbours[neighbourIndex]->m_neighbourId );
                            if( target && target->m_radarTarget )
                            {
                                if( !g_app->m_multiwiniaHelp->m_tutorial2AICanAct || m_id.GetTeamId() == 1 )
                                {
                                    return id;
                                }
                            }
                            else
                            {
                                return id;
                            }
                        }
                    }
                }
                AITarget *t = (AITarget *)g_app->m_location->GetBuilding( fromBuilding->m_neighbours[neighbourIndex]->m_neighbourId );
                if( fromBuilding->m_radarTarget )
                {
                    if( t && t->m_radarTarget ) return _fromTargetId;
                }
                if( t && t->m_waterTarget ) continue;
                if( fromBuilding->m_neighbours[neighbourIndex]->m_laserFenceId == -1 ) break;
                LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( fromBuilding->m_neighbours[neighbourIndex]->m_laserFenceId );
                if( !fence || fence->m_id.GetTeamId() == m_id.GetTeamId() ) break;
                neighbourIndex = syncrand() % numNeighbours;
            }
            AITarget *t = (AITarget *)g_app->m_location->GetBuilding(fromBuilding->m_neighbours[neighbourIndex]->m_neighbourId);
            if( t && t->m_type == Building::TypeAITarget )
            {
                if( t->m_priority[m_id.GetTeamId()] > 0.0 || g_app->Multiplayer() )
                {
                    id = fromBuilding->m_neighbours[neighbourIndex]->m_neighbourId;
                }
            }
		}
    }
    else
    {
        double bestPriority = fromBuilding->m_priority[_fromTeamId];

        bool turretTarget = false;
        LList<int> turretList;

        for( int i = 0; i < fromBuilding->m_neighbours.Size(); ++i )
        {
            int toBuildingId = fromBuilding->m_neighbours[i]->m_neighbourId;
            AITarget *target = (AITarget *) g_app->m_location->GetBuilding(toBuildingId);
            AppDebugAssert( target && target->m_type == Building::TypeAITarget );

            if( target->m_waterTarget ) continue;

            double thisPriority = target->m_priority[_fromTeamId];
            if( thisPriority > bestPriority )
            {
                LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( fromBuilding->m_neighbours[i]->m_laserFenceId );
                if( !fence || g_app->m_location->IsFriend(fence->m_id.GetTeamId(), _fromTeamId ) || !fence->IsEnabled())
                {
                    bestPriority = thisPriority;
                    id = toBuildingId;
                    if( target->m_turretTarget ) turretTarget = true;
                    else turretTarget = false;
                    if( fromBuilding->m_radarTarget && target->m_radarTarget ) id = _fromTargetId;
                }
            }

            if( target->m_turretTarget &&
                !g_app->m_location->IsFriend( target->m_id.GetTeamId(), _fromTeamId ) &&
                target->m_id.GetTeamId() != 255 )
            {
                turretList.PutData(i);
            }

			if( target->m_statueTarget != -1 )
			{
				id = toBuildingId;
				//break;
			}
        }

        if( turretTarget )
        {
            // we're headed towards a turret, so we should check to see if there are any others next to this target
            // if there are, we should pick randomly between them to make sure we dont all just get mowed down in a straight line
            if( turretList.Size() > 1 )
            {
                int sanity = 0;
                AITarget *best = (AITarget *)g_app->m_location->GetBuilding(id);
                while( sanity++ < 1000 )
                {
                    NeighbourInfo *info = fromBuilding->m_neighbours[ turretList[ syncrand() % turretList.Size() ] ];
                    AITarget *target = (AITarget *)g_app->m_location->GetBuilding(info->m_neighbourId);
                    if( (target->m_pos - best->m_pos).Mag() < 200.0 )
                    {
                        LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( info->m_laserFenceId );
                        if( !fence || !fence->IsEnabled() || g_app->m_location->IsFriend( fence->m_id.GetTeamId(), _fromTeamId ) )
                        {
                            id = info->m_neighbourId; 
                            break;
                        }
                    }
                }
            }
        }
    }

    return id;
}


int AI::FindNearestTarget( Vector3 const &_fromPos, bool _evaluateCliffs, bool _allowSteepDownhill )
{
    double nearest = FLT_MAX;
    int id = -1;

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget )
            {
                AITarget *target = (AITarget *) building;
                double distance = ( target->m_pos - _fromPos ).Mag();
                if( distance < nearest )
                {
                    int laserFenceId = -1;
                    if( g_app->m_location->IsWalkable( _fromPos, target->m_pos, _evaluateCliffs, _allowSteepDownhill, &laserFenceId ) )
                    {
                        /*LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( laserFenceId );
                        if( !fence || fence->m_id.GetTeamId() == target->m_id.GetTeamId() || !fence->IsEnabled() ) */
                        {
                            id = building->m_id.GetUniqueId();
                            nearest = distance;                    
                        }
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

    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    int numRemaining = team->m_others.Size() * 0.02;
    numRemaining = max( numRemaining, 1 );

    while( numRemaining > 0 )
    {
        int index = syncrand() % team->m_others.Size();
        if( team->m_others.ValidIndex(index) )
        {
            Entity *entity = team->m_others[index];
            if( entity && entity->m_type == TypeDarwinian && entity->m_onGround)
            {
                Darwinian *darwinian = (Darwinian *) entity;
                if( darwinian->m_wallHits > 10 &&
                    darwinian->m_state != Darwinian::StateInsideArmour )
                {
                    darwinian->m_state = Darwinian::StateIdle;
                    darwinian->m_wallHits = 0;
                }
                if( darwinian->m_state == Darwinian::StateIdle ||
                    darwinian->m_state == Darwinian::StateWorshipSpirit ||
                    darwinian->m_state == Darwinian::StateWatchingSpectacle )
                {
                    Building *nearestTarget = g_app->m_location->GetBuilding( FindNearestTarget(darwinian->m_pos, false, true) );
                    if( nearestTarget )
                    {
                        double distance = ( nearestTarget->m_pos - darwinian->m_pos ).Mag();
                        double minDistance = 70.0;
                        if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig ) minDistance = 20.0;
                        if( distance > 70.0 )
                        {
                            Vector3 targetPos = nearestTarget->m_pos;
                            double positionError = 20.0;
                            double range = 20.0;
                            if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig ) range = 0.0;
                            double radius = range+syncfrand(positionError);
	                        double theta = syncfrand(M_PI * 2);                
                            targetPos.x += radius * iv_sin(theta);
	                        targetPos.z += radius * iv_cos(theta);
                            //targetPos = darwinian->PushFromObstructions(targetPos, false);
                            //targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
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
    m_radarReposTimer -= SERVER_ADVANCE_PERIOD;
    m_radarReposTimer = max( m_radarReposTimer, 0.0 );

    if( g_app->Multiplayer() )
    {
        // these things all operate their own timers, so run them outside of the normal ai timer
        AdvanceRadarDishAI();
        AdvanceSpecialsAI();
        AdvanceUnitAI();
    }

    if( m_timer > 0.0 ) return false;
    m_timer = 1.0;

    if( g_app->Multiplayer() ) 
    {
        AdvanceMultiwiniaAI();
    }
    else
    {
        AdvanceSinglePlayerAI();
    }

    return false;
}

void AI::AdvanceSinglePlayerAI()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    LList<int> m_wellDefendedIds;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget &&
				g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                AITarget *target = (AITarget *) building;
                int idleCount = target->m_idleCount[m_id.GetTeamId()];
                int enemyCount = target->m_enemyCount[m_id.GetTeamId()];
                int friendCount = target->m_friendCount[m_id.GetTeamId()];

                int limit = 30;

                bool noAttack = enemyCount < friendCount * 0.33;

                if( (idleCount > limit && noAttack )
                    || !target->IsImportant() 
				   )
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
            double sendChance = 1.5 * (double) enemyCount / (double) numIdle;
            sendChance = max( sendChance, 0.6 );
            sendChance = min( sendChance, 1.0 );

            Vector3 targetPos = targetBuilding->m_pos;
            double positionError = 20.0;
            double radius = 20.0+syncfrand(positionError);
	        double theta = syncfrand(M_PI * 2);                
            targetPos.x += radius * iv_sin(theta);
	        targetPos.z += radius * iv_cos(theta);
            targetPos = PushFromObstructions(targetPos);
            targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);

            bool includes[NUM_TEAMS];
            memset( includes, false, sizeof(bool) * NUM_TEAMS );
            includes[m_id.GetTeamId()] = true;
            int numFound;
            g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, target->m_pos.x, target->m_pos.z, 150.0, &numFound, includes );

            int numSent = 0;

            for( int j = 0; j < numFound; ++j )
            {
                WorldObjectId id = s_neighbours[j];
                Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( id, TypeDarwinian );
                if( darwinian && syncfrand(1.0) < sendChance &&
                    (darwinian->m_state == Darwinian::StateIdle ||
                     darwinian->m_state == Darwinian::StateWorshipSpirit ||
                     darwinian->m_state == Darwinian::StateWatchingSpectacle ) &&
                     !darwinian->IsInFormation() && // Dont take Darwinians out of a formation
                     darwinian->m_onGround) 
                {
					darwinian->GiveOrders( targetPos );
                    numSent++;
                }
            }

            target->RecountTeams();
        }
    }
}

void AI::AdvanceMultiwiniaAI()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy )
    {
        m_timer = 20.0;
    }
    else if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeStandard )
    {
        m_timer = 10.0;
    }

    AdvanceOfficerAI();
    AdvanceCurrentTasks();
	AdvanceCaptureTheStatueAI();
    AdvanceRocketRiotAI();

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
    {
        AdvanceAssaultAI();
        //return;
    }

    LList<int> m_wellDefendedIds;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget &&
				g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                AITarget *target = (AITarget *) building;
                int idleCount = target->m_idleCount[m_id.GetTeamId()];
                int enemyCount = target->m_enemyCount[m_id.GetTeamId()];
                int friendCount = target->m_friendCount[m_id.GetTeamId()];

                int limit = 10;

               /* if( target->m_scoreZone != -1 )
                {
                    limit = 100;
                }*/

                for( int n = 0; n < target->m_neighbours.Size(); ++n )
                {
                    AITarget *neighbour = (AITarget *)g_app->m_location->GetBuilding( target->m_neighbours[n]->m_neighbourId );
                    int importantBuildingId = -1;
                    if( neighbour && neighbour->IsImportant( importantBuildingId ) )
                    {
                        Building *importantB = g_app->m_location->GetBuilding( importantBuildingId );
                        if( importantB && importantB->m_id.GetTeamId() == 255 )
                        {
                            limit = 10;
                            break;
                        }
                    }
                }

                bool noAttack = enemyCount < friendCount * 0.33;
                if( g_app->m_multiwinia->m_aiType != Multiwinia::AITypeEasy &&
                    g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault )
                {
                    if( enemyCount > 0 )
                    {
                        // never send darwinians away if there are enemy present in multiplayer, as another imminent attack is likely
                        noAttack = false;
                    }
                }

                if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault &&
                    target->m_radarTarget &&
                    g_app->m_location->IsAttacking( m_id.GetTeamId() ) ) limit *= 3;

                if( (idleCount > limit && noAttack )
                    || !target->IsImportant() 
				   )
                {
                    /*bool tooClose = false;
                    for( int i = 0; i < m_wellDefendedIds.Size(); ++i )
                    {
                        Building *b = g_app->m_location->m_buildings[ m_wellDefendedIds[i] ];
                        if( (b->m_pos - target->m_pos).Mag() < 150.0 )
                        {
                            tooClose = true;
                            break;
                        }
                    }*/

                    //if( !tooClose )
                    {
                        m_wellDefendedIds.PutData(i);
                    }
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

        bool useOfficer = false;
        WorldObjectId spawnPoint;
        for( int j = 0; j < g_app->m_location->m_buildings.Size(); ++j )
        {
            if( g_app->m_location->m_buildings.ValidIndex(j) )
            {
                Building *b = g_app->m_location->m_buildings[j];
                if( b->m_type == Building::TypeSpawnPoint &&
                    (target->m_pos - b->m_pos).Mag() < 200.0 &&
                    b->m_id.GetTeamId() == m_id.GetTeamId() &&
                    FindNearestTarget( b->m_pos ) == target->m_id.GetUniqueId() )
                {
                    spawnPoint = b->m_id;
                    break;
                }
            }
        }

        if( spawnPoint.IsValid() )
        {
            if( SearchForOfficers(target->m_pos, 200.0, false, true ) == 0 ) 
                useOfficer = true;
            else continue;
        }

        int targetBuildingId = FindTargetBuilding( target->m_id.GetUniqueId(), m_id.GetTeamId() );
        if( targetBuildingId != -1 )
        {
            AITarget *targetBuilding = (AITarget *) g_app->m_location->GetBuilding( targetBuildingId );
            int enemyCount = targetBuilding->m_enemyCount[ m_id.GetTeamId() ];
            double sendChance = 1.5 * (double) enemyCount / (double) numIdle;
            sendChance = max( sendChance, 0.6 );
            sendChance = min( sendChance, 1.0 );

            if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
            {
                if( g_app->m_location->IsAttacking( m_id.GetTeamId() ) )
                {
                    sendChance = 1.1;
                }
                for( int i = 0; i < team->m_specials.Size(); ++i )
                {
                    // see if we have any armour unloading nearby - if we do, wait till it is fully unloaded before moving
                    Armour *a = (Armour *)g_app->m_location->GetEntitySafe(*team->m_specials.GetPointer(i), Entity::TypeArmour );
                    if( a )
                    {
                        if( (a->m_pos - target->m_pos).Mag() < 100.0 &&
                            a->m_state == Armour::StateUnloading )
                        {
                            sendChance = 0.0;
                        }
                    }
                }
            }

            int importantBuildingId = -1;
            bool important = targetBuilding->IsImportant(importantBuildingId);
            if( !important )
            {
                // dont bother hanging around ai points in the middle of nowhere, send everyone
                sendChance = 1.1;
            }

            bool includes[NUM_TEAMS];
            memset( includes, false, sizeof(bool) * NUM_TEAMS );
            includes[m_id.GetTeamId()] = true;
            int numFound;
            g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, target->m_pos.x, target->m_pos.z, 150.0, &numFound, includes );

            int numSent = 0;

            if( useOfficer )
            {
                //if(SearchForOfficers(target->m_pos, 200.0 ) == 0 )
                {
                    team->m_taskManager->RunTask( GlobalResearch::TypeOfficer );
                    int id = team->m_taskManager->m_mostRecentTaskId;
                    for( int j = 0; j < numFound; ++j )
                    {
                        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[j], Entity::TypeDarwinian );
                        if( d && d->m_state == Darwinian::StateIdle )
                        {
                            team->m_taskManager->TargetTask( id, d->m_pos );
                            Officer *o = (Officer *)g_app->m_location->GetEntitySafe( team->m_taskManager->m_mostRecentEntity, Entity::TypeOfficer );
                            if( o )
                            {
                                Vector3 targetPos = targetBuilding->m_pos;
                                double positionError = 20.0;
                                double radius = 20.0+syncfrand(positionError);
	                            double theta = syncfrand(M_PI * 2);                
                                targetPos.x += radius * iv_sin(theta);
	                            targetPos.z += radius * iv_cos(theta);
                                targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(targetPos.x, targetPos.z);
                                if( targetBuilding->m_radarTarget )
					            {
						            LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( targetBuilding->m_pos.x, targetBuilding->m_pos.z );
						            for( int i = 0; i < nearbyBuildings->Size(); ++i )
						            {
							            int buildingId = nearbyBuildings->GetData(i);
							            Building *building = g_app->m_location->GetBuilding( buildingId );
							            if( building->m_type == Building::TypeRadarDish ||
								            building->m_type == Building::TypeBridge )
							            {
								            targetPos = building->m_pos;
							            }
						            }
						            //darwinian->GiveOrders( pos );
					            }
                                o->m_noFormations = true;
                                o->SetOrders( targetPos );
                                Building *b = g_app->m_location->GetBuilding( spawnPoint.GetUniqueId() );
                                o->SetWaypoint( b->m_pos + b->m_front * 60.0 );
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                int limit = 50;
                if( important )
                {
                    Building *importantB = g_app->m_location->GetBuilding( importantBuildingId );
                    if( importantB && importantB->m_id.GetTeamId() == 255 )
                    {
                        limit = 10;
                    }
                }
                for( int j = 0; j < numFound; ++j )
                {
                    if( g_app->m_multiwinia->m_aiType != Multiwinia::AITypeEasy &&
                        target->m_scoreZone != -1 &&
                        target->m_friendCount[m_id.GetTeamId()] - numSent < limit )
                    {
                        // make sure we dont leave a score zone we are occupying empty
                        break;
                    }

                    WorldObjectId id = s_neighbours[j];
                    Darwinian *darwinian = (Darwinian *) g_app->m_location->GetEntitySafe( id, TypeDarwinian );
                    if( darwinian && syncfrand(1.0) < sendChance &&
                        (darwinian->m_state == Darwinian::StateIdle ||
                         darwinian->m_state == Darwinian::StateWorshipSpirit ||
                         darwinian->m_state == Darwinian::StateWatchingSpectacle ) &&
                         !darwinian->IsInFormation() && // Dont take Darwinians out of a formation
                         darwinian->m_onGround &&
                         darwinian->m_closestAITarget == target->m_id.GetUniqueId() ) 
                    {
					    if( targetBuilding->m_radarTarget )
					    {
						    Vector3 pos;
						    LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( targetBuilding->m_pos.x, targetBuilding->m_pos.z );
						    for( int i = 0; i < nearbyBuildings->Size(); ++i )
						    {
							    int buildingId = nearbyBuildings->GetData(i);
							    Building *building = g_app->m_location->GetBuilding( buildingId );
							    if( building->m_type == Building::TypeRadarDish ||
								    building->m_type == Building::TypeBridge )
							    {
								    pos = building->m_pos;
							    }
						    }
						    darwinian->GiveOrders( pos );
					    }
					    else
					    {
                            Vector3 targetPos = targetBuilding->m_pos;
                            double positionError = 20.0;
                            double radius = 20.0+syncfrand(positionError);
	                        double theta = syncfrand(M_PI * 2);                
                            targetPos.x += radius * iv_sin(theta);
	                        targetPos.z += radius * iv_cos(theta);
						    darwinian->GiveOrders( targetPos );
					    }
                        numSent++;
                    }
                }
            }


            //target->RecountTeams();
        }
    }
}

void AI::AdvanceAssaultAI()
{
    if( g_app->m_multiwinia->m_aiType != Multiwinia::AITypeHard )
    {
        m_timer = 3.0;
    }
    if( !AIObjective::s_objectivesInitialised ) return;
    
    bool attacking = g_app->m_location->IsAttacking( m_id.GetTeamId() );

    if( m_currentObjective == -1 )
    {
        // we've just started (in theory), so look for a starting objective
        LList<int> activeObjectives;
        LList<int> alliedObjectives;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeAIObjective )
                {
                    if( ((AIObjective *)b)->m_active )
                    {
                        if( b->m_id.GetTeamId() == m_id.GetTeamId() )
                        {
                            activeObjectives.PutData( b->m_id.GetUniqueId() );
                            /*m_currentObjective = b->m_id.GetUniqueId();
                            b->SetTeamId(255);
                            break;*/
                        }
                        else if( g_app->m_location->IsFriend( b->m_id.GetTeamId(), m_id.GetTeamId() ) )
                        {
                            alliedObjectives.PutData( b->m_id.GetUniqueId() );
                        }

                    }
                }
            }
        }

        if( activeObjectives.Size() > 0 && m_currentObjective == -1 )
        {
            if( !attacking && activeObjectives.Size() > 1 )
            {
                // if there are multiple start points, we dont want to commit to an objective until the enemy has started moving towards one
                for( int i = 0; i < activeObjectives.Size(); ++i )
                {
                    AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding( activeObjectives[i] );
                    if( objective )
                    {
                        for( int j = 0; j < objective->m_objectiveMarkers.Size(); ++j )
                        {
                            AIObjectiveMarker *marker = (AIObjectiveMarker *)g_app->m_location->GetBuilding( objective->m_objectiveMarkers[j] );
                            if( marker )
                            {
                                bool underAttack = g_app->m_location->m_entityGrid->AreEnemiesPresent( marker->m_pos.x, marker->m_pos.z, 450.0, m_id.GetTeamId() );
                                if( underAttack )
                                {
                                    m_currentObjective = objective->m_id.GetUniqueId();
                                    break;
                                }
                            }
                        }
                        if( m_currentObjective != -1 )
                        {
                            break;
                        }
                    }
                }
            }
            else
            {
                m_currentObjective = activeObjectives[ syncrand() % activeObjectives.Size() ];
                /*for( int i = 0; i < activeObjectives.Size(); ++i )
                {
                    if( activeObjectives[i] != m_currentObjective )
                    {
                        AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding( activeObjectives[i] );
                        objective->m_active = false;
                    }
                }*/
            }

            if( m_currentObjective != -1 )
            {
                for( int i = 0; i < activeObjectives.Size(); ++i )
                {
                    if( activeObjectives[i] != m_currentObjective )
                    {
                        AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding(activeObjectives[i]);
                        if( objective )
                        {
                            objective->m_active = false;
                        }
                    }
                }

            }
        }
        else if( m_currentObjective == -1 )
        {
            // no objective specifically for us, so look for an allies objective and use that
            if( alliedObjectives.Size() > 0 )
            {
                m_currentObjective = alliedObjectives[ syncrand() % alliedObjectives.Size() ];
            }
        }
    }

    if( m_currentObjective != -1 )
    {
        AIObjective *currentObjective = (AIObjective *)g_app->m_location->GetBuilding( m_currentObjective );
        if( !currentObjective->m_active )
        {
            m_currentObjective = currentObjective->m_nextObjective;
        }
        if( attacking )
        {
            LList<int> wallBusters;
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    WallBuster *b = (WallBuster *)g_app->m_location->m_buildings[i];
                    if( b->m_type == Building::TypeWallBuster &&
                        b->m_id.GetTeamId() == m_id.GetTeamId() )
                    {
                        b->RunAI( this );
                    }
                }
            }
        }
    }
}

void AI::Render( double _predictionTime )
{
    if( g_app->m_editing )
    {
        RGBAColour teamCol = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
 
        Vector3 pos = m_pos;
        pos.y = 400.0;
        RenderSphere( pos, 20.0, teamCol );

        int numGreen = g_app->m_location->m_teams[0]->m_others.NumUsed();
        int numRed = g_app->m_location->m_teams[1]->m_others.NumUsed();

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.DrawText3DCentre( pos-Vector3(0,30,0), 25, "Green : %d", numGreen );
        g_editorFont.DrawText3DCentre( pos-Vector3(0,60,0), 25, "Red  : %d", numRed );
    }
}

void AI::AdvanceRadarDishAI()
{
    if( g_app->m_location->m_levelFile->m_levelOptions[LevelFile::LevelOptionNoRadarControl] == 1 ) return;
	if( m_radarReposTimer <= 0.0 )
	{
		m_radarReposTimer = 10.0;

		LList<RadarDish *> ourRadars;
		LList<RadarDish *> radars;

		for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
		{
			if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
			    if( b && b->m_type == Building::TypeRadarDish )
			    {
				    RadarDish *dish = (RadarDish *)b;
				    if( b->m_id.GetTeamId() == m_id.GetTeamId() )
				    {
					    bool usable = true;
                        int num = 0;
					    for( int j = 0; j < dish->m_inTransit.Size(); ++j )
					    {
						    // make sure we dont have anything belonging to us in transit before moving the dish
						    WorldObjectId *id = dish->m_inTransit.GetPointer(j);
						    if( id->GetTeamId() == m_id.GetTeamId() )
						    {
                                num++;
                            }

                            if( num > 20 )
                            {
							    usable = false;
							    break;
						    }
					    }

					    if( usable ) ourRadars.PutData(dish);
				    }
				    radars.PutData(dish);
			    }
            }
		}

		for( int i = 0; i < ourRadars.Size(); ++i )
		{
			RadarDish *d = ourRadars[i];
			double highestPriority = -1.0;
			int priorityId = -1;
			for( int j = 0; j < radars.Size(); ++j )
			{
				if( radars[j] == d ) continue;
				Vector3 theirFront = radars[j]->GetDishFront(0.0);
                double dotProd = (radars[j]->m_pos - d->m_pos).Normalise() * theirFront;
				if (dotProd < 0.0)
				{
					if (g_app->m_location->IsVisible( d->GetDishPos(0.0), radars[j]->GetDishPos(0.0)) &&
                        d->ValidReceiverDish(radars[j]->m_id.GetUniqueId() ) )
					{
                        if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaHelp->m_tutorial2AICanAct )
                        {
                            AITarget *t = (AITarget *)g_app->m_location->GetBuilding(FindNearestTarget( radars[j]->m_pos, false, true ));
                            if( t->m_priority[m_id.GetTeamId()] == 0.0 ) continue;

                            bool valid = false;
                            static const int validDishes[] = { 7, 9, 10, 12, 13, 15, -1 };
                            for( int x = 0; validDishes[x] != -1; ++x )
                            {
                                if( radars[j]->m_id.GetUniqueId() == validDishes[x] ) 
                                {
                                    valid = true;
                                }
                            }

                            if( !valid )
                            {
                                if( t && t->m_id.GetTeamId() != 255 ) 
                                {
                                    if( t->m_id.GetTeamId() != m_id.GetTeamId() ||
                                        t->m_friendCount[m_id.GetTeamId()] > 30 )
                                    {
                                        continue;
                                    }
                                }
                            }
                        }

						if( radars[j]->m_aiTarget )
						{
							if( radars[j]->m_aiTarget->m_priority[m_id.GetTeamId()] > highestPriority )
							{
								highestPriority = radars[j]->m_aiTarget->m_priority[m_id.GetTeamId()];
								priorityId = j;//radars[j]->m_id.GetUniqueId();
							}
						}
						else
						{
                            AITarget *t = (AITarget *)g_app->m_location->GetBuilding(FindNearestTarget(radars[j]->m_pos, false, true));
                            if( t && t->m_priority[m_id.GetTeamId()] > highestPriority )
							{
								highestPriority = t->m_priority[m_id.GetTeamId()];
								priorityId = j;
							}
						}
					}
				}				
			}

			if( priorityId != -1 )
			{
				if( d->GetConnectedDishId() != radars[priorityId]->m_id.GetUniqueId() )
				{
					d->Aim( radars[priorityId]->GetDishPos(0.0) );
				}
			}
		}
	}

}

int AI::SearchForOfficers( Vector3 _pos, double _range, bool _ignoreGotoOfficers, bool _gotoOnly )
{
    int num = 0;
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        Entity *e = g_app->m_location->GetEntity( *team->m_specials.GetPointer(i) );
        if( e && e->m_type == Entity::TypeOfficer && !e->m_dead )
        {
            if( (e->m_pos - _pos).Mag() <= _range )
            {
                if( _gotoOnly )
                {
                    if(((Officer *)e)->m_orders == Officer::OrderGoto ) 
                    {
                        num++;
                    }
                }
                else if( !_ignoreGotoOfficers || ((Officer *)e)->m_orders != Officer::OrderGoto )
                {
                    num++;
                }
            }
        }
    }

    return num;
}

void AI::AdvanceOfficerAI()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot ||
        g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault ||
        g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig ) return; // no officers in rocket riot or assault
    if( team->m_futurewinianTeam ) return;
    if( g_app->m_multiwiniaTutorial ) return;

    // Check to see if there's anywhere we need to make a new officer
    int limit = 70;
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeCaptureTheStatue) limit = 150;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            if( building->m_type == Building::TypeAITarget &&
				g_app->m_location->IsFriend(building->m_id.GetTeamId(), m_id.GetTeamId() ) )
            {
                AITarget *target = (AITarget *) building;
                bool needsDefending = target->NeedsDefending( m_id.GetTeamId() );
                int friendCount = target->m_friendCount[m_id.GetTeamId()];
                if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeCaptureTheStatue )
                {
                    friendCount = target->m_idleCount[m_id.GetTeamId()];
                }
                int numOfficers = SearchForOfficers(target->m_pos, 300.0, true);
                if( //target->IsImportant() &&
                    needsDefending &&
                    friendCount >= limit &&
                    //target->m_idleCount[m_id.GetTeamId()] >= 65 &&
                    numOfficers == 0 ) 
                {
                    team->m_taskManager->RunTask( GlobalResearch::TypeOfficer );

                    // find our new officer task
                    int taskId = -1;
                    for( int j = 0; j < team->m_taskManager->m_tasks.Size(); ++j )
                    {
                        if( team->m_taskManager->m_tasks[j]->m_type == GlobalResearch::TypeOfficer )
                        {
                            taskId = team->m_taskManager->m_tasks[j]->m_id;
                            break;
                        }
                    }

                    // find a darwinian to promote
                    bool includes[NUM_TEAMS];
                    memset( includes, false, sizeof(bool) * NUM_TEAMS );
                    includes[m_id.GetTeamId()] = true;
                    int numFound;
                    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, target->m_pos.x, target->m_pos.z, 100.0, &numFound, includes );
                    for( int j = 0; j < numFound; ++j )
                    {
                        // scan through the list until we find the first darwinian
                        WorldObjectId id = s_neighbours[j];
                        Entity *e = g_app->m_location->GetEntity( id );
                        if( e && e->m_type == Entity::TypeDarwinian )
                        {
                            Darwinian *d = (Darwinian *)e;
                            if( d->m_state != Darwinian::StateBeingSacraficed &&
                                d->m_state != Darwinian::StateCapturedByAnt &&
                                d->m_state != Darwinian::StateInsideArmour &&
                                d->m_state != Darwinian::StateOperatingPort )
                            {
                                team->m_taskManager->TargetTask( taskId, e->m_pos );
                                break;
                            }
                        }
                    }                    
                }
            }
        }
    }
}

void AI::AdvanceSpecialsAI()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        Entity *e = g_app->m_location->GetEntity( *team->m_specials.GetPointer(i) );
        if( e )
        {
            e->RunAI( this );
        }
    }
}

void AI::AdvanceUnitAI()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_units.Size(); ++i )
    {
        if( team->m_units.ValidIndex(i) )
        {
            team->m_units[i]->RunAI( this );
        }
    }
}

void AI::AdvanceCurrentTasks()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_taskManager->m_tasks.Size(); ++i )
    {
		Task *t = team->m_taskManager->m_tasks[i];
        if( t )
        {
            if( t->m_state != Task::StateRunning &&
                t->m_startTimer <= 0.0 )
            {
                switch( t->m_type )
                {
					case GlobalResearch::TypeSquad:         RunSquad(t->m_id);				break;
                    case GlobalResearch::TypeArmour:        RunArmour(t->m_id);				break;
                    case GlobalResearch::TypeHarvester:     RunHarvester(t->m_id);			break;
                    case GlobalResearch::TypeEngineer:      RunEngineer(t->m_id);           break;
                    case GlobalResearch::TypeAirStrike:     RunAirStrike(t->m_id);			break;
                    case GlobalResearch::TypeNuke:          RunNuke(t->m_id);				break;
					case GlobalResearch::TypeGunTurret:		RunGunTurret(t->m_id);			break;
					case GlobalResearch::TypeInfection:		RunSpam(t->m_id);				break;
					case GlobalResearch::TypeMagicalForest: RunForest(t->m_id);				break;
					case GlobalResearch::TypePlague:		RunPlague(t->m_id);				break;
					case GlobalResearch::TypeAntsNest:		RunAntNest(t->m_id);			break;
					case GlobalResearch::TypeDarkForest:	RunDarkForest(t->m_id);			break;
					case GlobalResearch::TypeEggSpawn:		RunEggs(t->m_id);				break;
					case GlobalResearch::TypeMeteorShower:	RunMeteor(t->m_id);				break;
                    case GlobalResearch::TypeTank:          RunTank(t->m_id);               break;

                    case GlobalResearch::TypeBooster:
                    case GlobalResearch::TypeSubversion:
                    case GlobalResearch::TypeHotFeet:
                    case GlobalResearch::TypeRage:
                        RunPowerup();
                        break;
                }
            }
        }
    }
}

int CountPanels( Vector3 const &_pos, double const _range )
{
    int num = 0;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeSolarPanel &&
                (b->m_pos - _pos).Mag() <= _range )
            {
                num++;
            }
        }
    }

    return num;
}

void AI::AdvanceRocketRiotAI()
{
    if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeRocketRiot ) return;

    if( m_rrObjectiveList.Size() == 0 )
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	    {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];		
                if( b && b->m_type == Building::TypeAIObjectiveMarker &&
                    b->m_id.GetTeamId() == m_id.GetTeamId() )
                {
                    double range = ((AIObjectiveMarker *)b)->m_scanRange;
                    int numPanels = CountPanels( b->m_pos, range );
                    RRSolarPanelInfo *info = new RRSolarPanelInfo;
                    info->m_pos = b->m_pos;
                    info->m_numPanels = numPanels;
                    info->m_range = range;
                    info->m_objectiveId = b->m_id.GetUniqueId();
                    m_rrObjectiveList.PutData( info );
                }
            }
        }
    }
}

void AI::AdvanceCaptureTheStatueAI()
{
    if( g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeCaptureTheStatue ) return;

	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];		
		    if( b &&
			    b->m_type == Building::TypeStatue &&
			    b->m_id.GetTeamId() == m_id.GetTeamId() )
		    {
			    Statue *statue = (Statue *)b;
			    /*if( statue->m_waypoint == statue->m_pos )
			    {
                    bool includes[NUM_TEAMS];
                    memset( includes, false, sizeof(bool) * NUM_TEAMS );
                    includes[m_id.GetTeamId()] = true;
				    int numFound;
				    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, statue->m_pos.x, statue->m_pos.z, 50.0, &numFound, includes );

				    for( int j = 0; j < numFound; ++j )
				    {
					    Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[j], Entity::TypeDarwinian );
					    if( d )
					    {
						    d->GiveOrders( GetClosestScoreZonePos( statue->m_pos ) );
					    }
				    }
			    }*/

                if( statue->m_vel.Mag() == 0.0 )
                {
                    // we aren't moving
                    int numCollisions;
                    double currentCollisionFactor;
                    int *collisionIds = statue->CalculateCollisions( statue->m_pos, numCollisions, currentCollisionFactor );

                    if( numCollisions > 0 )
                    {
                        // we're stuck on another statue
                        Vector3 pos;
                        for( int i = 0; i < numCollisions; ++i )
                        {
                            int id = collisionIds[i];
                            Building *b = g_app->m_location->GetBuilding( id );
                            if( b && CarryableBuilding::IsCarryableBuilding(b->m_type) )
                            {
                                pos += b->m_pos;
                            }
                        }

                        pos /= numCollisions;

                        Vector3 dir = (statue->m_pos - pos).Normalise();
                        Vector3 targetPos = statue->m_pos + dir * 20.0;
                        targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( targetPos.x, targetPos.z );

                        bool includes[NUM_TEAMS];
                        memset( includes, false, sizeof(bool) * NUM_TEAMS );
                        includes[m_id.GetTeamId()] = true;
				        int numFound;
				        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, statue->m_pos.x, statue->m_pos.z, 50.0, &numFound, includes );

				        for( int j = 0; j < numFound; ++j )
				        {
					        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[j], Entity::TypeDarwinian );
                            if( d && d->m_state == Darwinian::StateCarryingBuilding && d->m_buildingId == statue->m_id.GetUniqueId() )
					        {
						        d->GiveOrders( targetPos );
					        }
				        }
                    }
                }
		    }
        }
	}
}

Vector3 AI::GetClosestScoreZonePos( Vector3 _pos )
{
	if( m_ctsScoreZones.Size() == 0 )
	{
		// find the score zones and add them to a list for quick access
		for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
		{
			if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
			    if( b &&
				    b->m_type == Building::TypeMultiwiniaZone &&
				    b->m_id.GetTeamId() == m_id.GetTeamId() )
			    {
				    m_ctsScoreZones.PutData( b->m_pos );
			    }
		    }
        }
	}

	double distance = FLT_MAX;
	Vector3 pos;

	for( int i = 0; i < m_ctsScoreZones.Size(); ++i )
	{
		if( (*m_ctsScoreZones.GetPointer(i) - _pos).Mag() < distance )
		{
			distance = (*m_ctsScoreZones.GetPointer(i) - _pos).Mag();
			pos = *m_ctsScoreZones.GetPointer(i);
		}
	}

	return pos;
}

void AI::RunSquad( int _taskId )
{
    AITarget *target = NULL;
    int maxEnemies = 0;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            AITarget *t = (AITarget *)g_app->m_location->m_buildings[i];
            if( t && t->m_type == Building::TypeAITarget &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, t->m_pos ) )
            {
                if( t->m_enemyCount[m_id.GetTeamId()] > maxEnemies )
                {
                    maxEnemies = t->m_enemyCount[m_id.GetTeamId()];
                    target = t;
                }
            }
        }
    }

    if( target )
    {
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        double dist = FLT_MAX;
        int areaId = -1;
        LList <TaskTargetArea> *validAreas = team->m_taskManager->GetTargetArea( _taskId );
        for( int i = 0; i < validAreas->Size(); ++i )
        {
            TaskTargetArea *area = validAreas->GetPointer(i);
            if( (target->m_pos - area->m_centre).Mag() < dist )
            {
                dist = (target->m_pos - area->m_centre).Mag();
                areaId = i;
            }
        }

        if( areaId != -1 )
        {
            TaskTargetArea *area = validAreas->GetPointer(areaId);
            Vector3 pos = area->m_centre;
            while( !team->m_taskManager->IsValidTargetArea( _taskId, pos ) )
            {
                pos = area->m_centre;
                pos.x += syncsfrand( area->m_radius );
                pos.z += syncsfrand( area->m_radius );
            }

            team->m_taskManager->TargetTask( _taskId, pos );            
        }

		delete validAreas;
    }
}

// armour and harvester can be placed at any random point - harvesters automatically move towards souls and armour are quick enough anyway
void AI::RunArmour( int _taskId )
{
    LList <TaskTargetArea> *validAreas = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTargetArea( _taskId );
    if( validAreas->Size() == 0 ) 
	{
		delete validAreas;
		return;
	}
    int id = syncrand() % validAreas->Size();

    TaskTargetArea *area = validAreas->GetPointer(id);
    if( area )
    {
        Vector3 pos = area->m_centre;
        pos.x += syncsfrand( area->m_radius );
        pos.z += syncsfrand( area->m_radius );
        pos = PushFromObstructions(pos);
        while( !g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, pos ) )
        {
            pos = area->m_centre;
            pos.x += syncsfrand( area->m_radius );
            pos.z += syncsfrand( area->m_radius );
            pos = PushFromObstructions( pos );
        }
        g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
    }

	delete validAreas;
}

void AI::RunHarvester( int _taskId )
{
    LList <TaskTargetArea> *validAreas = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTargetArea( _taskId );
    if( validAreas->Size() == 0 ) 
	{
		delete validAreas;
		return;
	}
    int id = syncrand() % validAreas->Size();

    TaskTargetArea *area = validAreas->GetPointer(id);
    if( area )
    {
        Vector3 pos = area->m_centre;
        while( !g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, pos ) )
        {
            pos = area->m_centre;
            pos.x += syncsfrand( area->m_radius );
            pos.z += syncsfrand( area->m_radius );
        }
        g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
    }
	delete validAreas;
}


void AI::RunEngineer( int _taskId )
{
    LList <TaskTargetArea> *validAreas = g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->GetTargetArea( _taskId );
    if( validAreas->Size() == 0 ) 
	{
		delete validAreas;
		return;
	}
    int id = syncrand() % validAreas->Size();

    TaskTargetArea *area = validAreas->GetPointer(id);
    if( area )
    {
        Vector3 pos = area->m_centre;
        while( !g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, pos ) )
        {
            pos = area->m_centre;
            pos.x += syncsfrand( area->m_radius );
            pos.z += syncsfrand( area->m_radius );
        }
        g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
    }

	delete validAreas;
}


void AI::RunAirStrike( int _taskId )
{
    // target the most heavily populated enemy area
    Vector3 pos;
    int maxEnemies = 0;
	LList<int> turretList;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
			Building *b = g_app->m_location->m_buildings[i];
            if( !g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) ) continue;
            if( b && b->m_type == Building::TypeAITarget &&
                b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() )
            {
				AITarget *t = (AITarget *)g_app->m_location->m_buildings[i];
                if( t->m_enemyCount[m_id.GetTeamId()] > maxEnemies )
                {
                    maxEnemies = t->m_enemyCount[m_id.GetTeamId()];
                    pos = t->m_pos;
                }
            }

			if( b->m_type == Building::TypeGunTurret )
			{
				if( b->m_id.GetTeamId() != 255 &&
					!g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) )
				{
					turretList.PutData( b->m_id.GetUniqueId() );
				}
			}
        }
    }


    if( turretList.Size() > 0 )
	{
		pos = g_app->m_location->GetBuilding( turretList[ syncrand() % turretList.Size() ] )->m_pos;
	}

	if( pos != g_zeroVector )
    {
        g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
    }
}

void AI::RunNuke( int _taskId )
{
    // target the most heavily populated enemy area
    AITarget *target = NULL;
    int maxEnemies = 0;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            AITarget *t = (AITarget *)g_app->m_location->m_buildings[i];
            if( t && t->m_type == Building::TypeAITarget &&
                t->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, t->m_pos ) )
            {
                if( t->m_enemyCount[m_id.GetTeamId()] > maxEnemies )
                {
                    maxEnemies = t->m_enemyCount[m_id.GetTeamId()];
                    target = t;
                }
            }
        }
    }

    if( target )
    {
        g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, target->m_pos );
    }
}

void AI::RunTank( int _taskId )
{
    AITarget *target = NULL;
    int maxEnemies = 0;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            AITarget *t = (AITarget *)g_app->m_location->m_buildings[i];
            if( t && t->m_type == Building::TypeAITarget )
            {
                if( t->m_enemyCount[m_id.GetTeamId()] > maxEnemies )
                {
                    maxEnemies = t->m_enemyCount[m_id.GetTeamId()];
                    target = t;
                }
            }
        }
    }

    if( target )
    {
        Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
        double dist = FLT_MAX;
        int areaId = -1;
        LList <TaskTargetArea> *validAreas = team->m_taskManager->GetTargetArea( _taskId );
        for( int i = 0; i < validAreas->Size(); ++i )
        {
            TaskTargetArea *area = validAreas->GetPointer(i);
            if( (target->m_pos - area->m_centre).Mag() < dist )
            {
                dist = (target->m_pos - area->m_centre).Mag();
                areaId = i;
            }
        }

        if( areaId != -1 )
        {
            TaskTargetArea *area = validAreas->GetPointer(areaId);
            Vector3 pos = area->m_centre;
            while( !team->m_taskManager->IsValidTargetArea( _taskId, pos ) )
            {
                pos = area->m_centre;
                pos.x += syncsfrand( area->m_radius );
                pos.z += syncsfrand( area->m_radius );
            }

            team->m_taskManager->TargetTask( _taskId, pos );            
        }
		delete validAreas;
    }
}

void AI::RunPowerup()
{
    // we need to look for a group of darwinians, in combat or on their way to an enemy location
    for( int i = 0; i < g_app->m_location->m_buildings.Size();++i ) 
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if ( b->m_type == Building::TypeAITarget )
            {
                AITarget *target = (AITarget *)b;
                if( target->m_friendCount[m_id.GetTeamId()] > 30 &&
                    target->m_enemyCount[m_id.GetTeamId()] > target->m_friendCount[m_id.GetTeamId()] )
                {
                    int numFound;
                    double range = 100.0;
                    if( target->m_scoreZone != -1 )
                    {
                        MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( target->m_scoreZone );
                        if( zone && zone->m_type == Building::TypeMultiwiniaZone )
                        {
                            range = zone->m_size;
                        }
                    }
                    g_app->m_location->m_entityGrid->GetFriends( s_neighbours, target->m_pos.x, target->m_pos.z, range, &numFound, m_id.GetTeamId() );
                    Vector3 pos;
                    for( int e = 0; e < numFound; ++e )
                    {
                        WorldObjectId id = s_neighbours[e];
                        Entity *entity = g_app->m_location->GetEntity(id);
                        if( entity && entity->m_type == Entity::TypeDarwinian )
                        {
                            pos += entity->m_pos;
                        }
                    }
                    pos /= numFound;

                    RunPowerup(pos);
                    break;
                }
            }
        }
    }

}

void AI::RunPowerup( Vector3 _pos )
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_taskManager->m_tasks.Size(); ++i )
    {
        Task *t = team->m_taskManager->m_tasks[i];
        if( t )
        {
            if( t->m_state != Task::StateRunning )
            {
                if( t->m_type == GlobalResearch::TypeSubversion ||
                    t->m_type == GlobalResearch::TypeBooster ||
                    t->m_type == GlobalResearch::TypeHotFeet ||
                    t->m_type == GlobalResearch::TypeRage )
                {
                    team->m_taskManager->TargetTask( t->m_id, _pos );
                }
            }
        }
    }
}

void AI::RunGunTurretRocketRiot( int _taskId )
{
    LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[m_id.GetTeamId()];
    EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );

	if( !rocket ) return;

    Vector3 trunkPortTotalPos;
    //if( rocket->m_fuel >= 10.0 )
    {
        // rocket has begun to refuel, so start protecting it
        int numTurretsFound = 0;
        int trunkPortId = -1;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeGunTurret )
                {
                    if( b->m_id.GetTeamId() == m_id.GetTeamId() &&
                        ( rocket->m_pos - b->m_pos ).Mag() < 300.0 )
                    {
                        numTurretsFound++;
                    }
                }

                if( b->m_type == Building::TypeTrunkPort &&
                    b->m_id.GetTeamId() != m_id.GetTeamId() )
                {
                    trunkPortTotalPos += b->m_pos;
                }
            }
        }

        if( numTurretsFound < 2)
        {
            Vector3 pos = rocket->m_pos;
            double angle = syncfrand(M_PI / 2.0 );
            Vector3 direction;
            direction.x += iv_cos(angle);
            direction.z += iv_sin(angle);
            direction.Normalise();
            
            double distance = 75.0 + syncfrand(50.0);
            pos += direction * distance;

            g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
            return;
        }
    }

    // rocket doesnt need defending, so find an AI target we currently control and put it there for defense

    LList<int> ownedTargets;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeAITarget &&
                b->m_id.GetTeamId() == m_id.GetTeamId() )
            {
                ownedTargets.PutData( b->m_id.GetUniqueId() );
            }
        }
    }

    if( ownedTargets.Size() > 0 )
    {
        int index = syncrand() % ownedTargets.Size();
        Building *t = g_app->m_location->GetBuilding( ownedTargets[index] );

        int numFound;
        g_app->m_location->m_entityGrid->GetFriends( s_neighbours, t->m_pos.x, t->m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, &numFound, m_id.GetTeamId() );

        Vector3 pos;
        int num = 0;
        for( int i = 0; i < numFound; ++i )
        {
            Entity *e = g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
            if( e )
            {
                pos += e->m_pos;
                num++;
            }
        }
        if( num > 0 )
        {
            pos /= num;
            pos.x += syncsfrand(200.0);
            pos.z += syncsfrand(200.0);
            pos = PushFromObstructions( pos, false );

			g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
        }
    }
}

Vector3 PushFromTurrets( Vector3 pos )
{
    Vector3 result = pos;
    for( int b = 0; b < g_app->m_location->m_buildings.Size(); ++b )
    {
        if( !g_app->m_location->m_buildings.ValidIndex(b) ) continue;
        Building *building = g_app->m_location->m_buildings[b];
        if( building && building->m_type == Building::TypeGunTurret )
        {
            double d = 60.0;
            if( (building->m_pos - result).Mag() < d )
            {                         
                Vector3 pushForce = (building->m_pos - result).SetLength(10.0);
                while( (building->m_pos - result).Mag() < d )
                {
                    result -= pushForce;                
                    //result.y = g_app->m_location->m_landscape.m_heightMap->GetValue( result.x, result.z );
                }
            }
        }
    }

    return result;
}

void AI::RunGunTurret( int _taskId )
{
    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeAssault )
    {
        if( g_app->m_location->IsAttacking( m_id.GetTeamId() ) )
        {
            // we dont want to bother with turrets if we are attacking in assault
            // so just terminate it
            g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TerminateTask(_taskId, false );
            return;
        }
    }
	if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeBlitzkreig )
	{
		RunGunTurretBlitzkrieg( _taskId );
        return;
	}

    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot &&
        g_app->m_multiwinia->m_aiType > Multiwinia::AITypeEasy ) 
    {
        RunGunTurretRocketRiot( _taskId );
        return;
    }

	int targetBuilding = -1;
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeAITarget &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) )
            {
                if( b->m_id.GetTeamId() == m_id.GetTeamId() )
                {
                    AITarget *t = (AITarget *)b;
                    int hostileTarget = -1;
                    int neutralTarget = -1;

                    int hostileCount = 0;

                    for( int j = 0; j < t->m_neighbours.Size(); ++j )
                    {
                        AITarget *n = (AITarget *)g_app->m_location->GetBuilding( t->m_neighbours[j]->m_neighbourId );
                        if( n )
                        {
                            if( !g_app->m_location->IsFriend( m_id.GetTeamId(), n->m_id.GetTeamId() ) )
                            {
                                if( n->m_id.GetTeamId() == 255 )
                                {
                                    neutralTarget = t->m_id.GetUniqueId();
                                }
                                else
                                {
                                    if( n->m_enemyCount[m_id.GetTeamId()] > hostileCount )
                                    {
                                        hostileTarget = t->m_neighbours[j]->m_neighbourId;
                                        hostileCount = n->m_enemyCount[m_id.GetTeamId()];
                                    }
                                }
                            }
                        }
                    }

                    if( hostileTarget != -1 )   targetBuilding = hostileTarget;
                    else if( neutralTarget != -1 ) targetBuilding = neutralTarget;
                }
            }
        }
    }

    Building *b = g_app->m_location->GetBuilding( targetBuilding );
    if( b )
    {
        if( b->m_type == Building::TypeAITarget )
        {
            AITarget *t = (AITarget *)b;
            // make sure we pick a spot where our own guys are
            int numFound;
            g_app->m_location->m_entityGrid->GetFriends( s_neighbours, t->m_pos.x, t->m_pos.z, DARWINIAN_SEARCHRANGE_ARMOUR, &numFound, m_id.GetTeamId() );

            Vector3 pos;
            int num = 0;
            for( int i = 0; i < numFound; ++i )
            {
                Darwinian *e = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
                if( e )
                {
                    if( e->m_state != Darwinian::StateInsideArmour &&
                        e->m_state != Darwinian::StateOnFire &&
                        e->m_onGround &&
                        !e->m_dead )
                    {
                        pos += e->m_pos;
                        num++;
                    }
                }
            }
            if( num > 0 )
            {
                pos /= num;
                //pos.x += syncsfrand(15.0);
                //pos.z += syncsfrand(15.0);
                pos = PushFromTurrets(pos);
                pos = PushFromObstructions(pos);
				g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
            }
        }
    }
}

void AI::RunGunTurretBlitzkrieg( int _taskId )
{
	MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( MultiwiniaZone::s_blitzkriegBaseZone[m_id.GetTeamId()]);
	if( zone && zone->m_type == Building::TypeMultiwiniaZone )
	{
		int numTurrets = 0;	
		Vector3 turretPos = g_zeroVector;
		LList<int> zoneList;
		for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i) 
		{
			if( g_app->m_location->m_buildings.ValidIndex( i ) )
			{
				Building *building = g_app->m_location->m_buildings[i];
				if( (building->m_pos - zone->m_pos).Mag() < zone->m_size &&
					building->m_type == Building::TypeGunTurret )
				{
					numTurrets++;
					turretPos = building->m_pos;
				}

				if( building->m_type == Building::TypeMultiwiniaZone &&
					building->m_id.GetUniqueId() != MultiwiniaZone::s_blitzkriegBaseZone[m_id.GetTeamId()] &&
					building->m_id.GetTeamId() == m_id.GetTeamId() )
				{
					zoneList.PutData( building->m_id.GetUniqueId() );
				}
			}
		}

		Vector3 pos;

		int turretLimit = 2;
		if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy ) turretLimit = 1;
		if( numTurrets < turretLimit )
		{
			pos = zone->m_pos;
			Vector3 dir = zone->m_front;
			dir.RotateAroundY( syncfrand(M_PI*2.0) );
			pos += dir * zone->m_size;
			pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
			pos = PushFromObstructions(pos, false);
			if( turretPos != g_zeroVector )
			{
				while( (pos - turretPos).Mag() < 50.0 )
				{
					pos = zone->m_pos;
					Vector3 dir = zone->m_front;
					dir.RotateAroundY( syncfrand(M_PI*2.0) );
					pos += dir * zone->m_size;
					pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
					pos = PushFromObstructions(pos, false);
				}
			}
		}
		else
		{
			if( zoneList.Size() > 0 )
			{
				int buildingId = zoneList[ syncrand() % zoneList.Size() ];
				MultiwiniaZone *b = (MultiwiniaZone *)g_app->m_location->GetBuilding( buildingId );
				if( b && b->m_type == Building::TypeMultiwiniaZone )
				{
					pos = b->m_pos;
					Vector3 dir = b->m_front;
					dir.RotateAroundY( syncfrand(M_PI*2.0) );
					pos += dir * b->m_size;
					pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
					pos = PushFromObstructions(pos, false);
				}
			}
		}

		if( pos != g_zeroVector )
		{
			g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );
		}
	}
}

void AI::RunSpam( int _taskId )
{
	int goodTarget = -1; // a good target is a target which has no neighbours owned by us (ie, far away from our own forces
	int acceptableTarget = -1;	// an acceptable target is one which the enemy controls but is close to an area we control. used if there are no good targets available

	int goodPopulation = 0;
	int acceptablePopulation = 0;

	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeAITarget )
            {
                if( !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) )
                {
                    AITarget *t = (AITarget *)b;
				    bool good = true;
				    for( int j = 0; j < t->m_neighbours.Size(); ++j )
				    {
                        Building *neighbour = g_app->m_location->GetBuilding( t->m_neighbours[j]->m_neighbourId );
					    if( neighbour->m_id.GetTeamId() == m_id.GetTeamId() )
					    {
						    good = false;
						    break;
					    }
				    }

				    if( good )
				    {
					    if( t->m_enemyCount[m_id.GetTeamId()] > goodPopulation )
					    {
						    goodPopulation = t->m_enemyCount[m_id.GetTeamId()];
                            goodTarget = t->m_id.GetUniqueId();
					    }
				    }
				    else
				    {
					    if( t->m_enemyCount[m_id.GetTeamId()] > acceptablePopulation )
					    {
						    acceptablePopulation = t->m_enemyCount[m_id.GetTeamId()];
						    acceptableTarget = m_id.GetUniqueId();
					    }
				    }
			    }
		    }
        }
	}

	int target = goodTarget;
	if( target == -1 ) target = acceptableTarget;

	if( target != -1 )
	{
		Building *b = g_app->m_location->GetBuilding( target );
		g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, b->m_pos );
	}
}

void AI::RunForest( int _taskId )
{
	Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];

	bool hasHarvester = false;
	for( int i = 0; i < team->m_specials.Size(); ++i )
	{
		Entity *e = g_app->m_location->GetEntity( *team->m_specials.GetPointer(i) );
		if( e->m_type == Entity::TypeHarvester )
		{
			hasHarvester = true;
			break;
		}
	}

	if( hasHarvester )
	{
		for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
		{
			if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
			    if( b &&
				    b->m_type == Building::TypeAITarget &&
				    b->m_id.GetTeamId() == m_id.GetTeamId() )
			    {
				    AITarget *target = (AITarget *)b;
				    for( int j = 0; j < target->m_neighbours.Size(); ++j )
				    {
                        Building *neighbour = g_app->m_location->GetBuilding( target->m_neighbours[j]->m_neighbourId );
					    if( neighbour->m_id.GetTeamId() == m_id.GetTeamId() )
					    {
						    Vector3 pos = target->m_pos + neighbour->m_pos;
						    pos /= 2.0;

						    team->m_taskManager->TargetTask( _taskId, pos );
						    break;
					    }
				    }
			    }
		    }
        }
	}
}

void AI::RunAntNest( int _taskId )
{
	LList<int>	enemySpawnPoints;
	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
		if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
		    if( b &&
			    b->m_type == Building::TypeSpawnPoint &&
			    !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) &&
			    b->m_id.GetTeamId() != 255 &&
                b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) )
		    {
			    enemySpawnPoints.PutData( b->m_id.GetUniqueId() );
		    }
        }
	}

	if( enemySpawnPoints.Size() > 0 )
	{
		int id = syncrand() % enemySpawnPoints.Size();
		Building *b = g_app->m_location->GetBuilding( enemySpawnPoints[id] );
		if( b )
		{
			g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, b->m_pos );

		}
	}
}

void AI::RunEggs( int _taskId )
{
	LList<int>	enemyAITargets;
	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
		if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
		    if( b &&
			    b->m_type == Building::TypeAITarget &&
			    !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) &&
			    b->m_id.GetTeamId() != 255 &&
                b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) )
		    {
			    enemyAITargets.PutData( b->m_id.GetUniqueId() );
		    }
        }
	}

	if( enemyAITargets.Size() > 0 )
	{
		int id = syncrand() % enemyAITargets.Size();
		Building *b = g_app->m_location->GetBuilding( enemyAITargets[id] );
		if( b )
		{
			Vector3 direction;
			direction.x += syncsfrand(1.0);
			direction.z += syncsfrand(1.0);
			direction.Normalise();
			Vector3 pos = b->m_pos + direction * 150.0;
			pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
			g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, pos );

		}
	}
}

void AI::RunPlague( int _taskId )
{
	int goodTarget = -1; // a good target is a target which has no neighbours owned by us (ie, far away from our own forces
	int acceptableTarget = -1;	// an acceptable target is one which the enemy controls but is close to an area we control. used if there are no good targets available

	int goodPopulation = 0;
	int acceptablePopulation = 0;

	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b && b->m_type == Building::TypeAITarget )
            {
                if( !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) &&
                    b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() )
                {
                    AITarget *t = (AITarget *)b;
				    bool good = true;
				    for( int j = 0; j < t->m_neighbours.Size(); ++j )
				    {
					    Building *neighbour = g_app->m_location->GetBuilding( t->m_neighbours[j]->m_neighbourId );
					    if( neighbour->m_id.GetTeamId() == m_id.GetTeamId() )
					    {
						    good = false;
						    break;
					    }
				    }

				    if( good )
				    {
					    if( t->m_enemyCount[m_id.GetTeamId()] > goodPopulation )
					    {
						    goodPopulation = t->m_enemyCount[m_id.GetTeamId()];
						    goodTarget = i;
					    }
				    }
				    else
				    {
					    if( t->m_enemyCount[m_id.GetTeamId()] > acceptablePopulation )
					    {
						    acceptablePopulation = t->m_enemyCount[m_id.GetTeamId()];
						    acceptableTarget = i;
					    }
				    }
			    }
		    }
        }
	}

	int target = goodTarget;
	if( target == -1 ) target = acceptableTarget;

	if( target != -1 )
	{
		Building *b = g_app->m_location->GetBuilding( target );
		g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, b->m_pos );
	}
}

void AI::RunMeteor( int _taskId )
{
	double area = POWERUP_EFFECT_RANGE * 6.0;
	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
		if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
		    if( b &&
			    b->m_type == Building::TypeAITarget &&
			    !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) &&
			    b->m_id.GetTeamId() != 255 &&
                b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) )
		    {
			    if( g_app->m_location->m_entityGrid->GetNumFriends( b->m_pos.x, b->m_pos.z, area, m_id.GetTeamId() ) < 30 &&
				    g_app->m_location->m_entityGrid->GetNumEnemies( b->m_pos.x, b->m_pos.z, area, m_id.GetTeamId() ) > 50 )
			    {
				    g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, b->m_pos );
				    break;
			    }
		    }
        }
	}
}

void AI::RunDarkForest( int _taskId )
{
	LList<int>	enemySpawnPoints;
	for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
	{
		if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *b = g_app->m_location->m_buildings[i];
		    if( b &&
			    b->m_type == Building::TypeSpawnPoint &&
			    !g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) &&
			    b->m_id.GetTeamId() != 255 &&
                b->m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() &&
                g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->IsValidTargetArea( _taskId, b->m_pos ) )
		    {
                enemySpawnPoints.PutData( b->m_id.GetUniqueId() );
		    }
        }
	}

	if( enemySpawnPoints.Size() > 0 )
	{
		int id = syncrand() % enemySpawnPoints.Size();
		Building *b = g_app->m_location->GetBuilding( enemySpawnPoints[id] );
		if( b )
		{
			g_app->m_location->m_teams[m_id.GetTeamId()]->m_taskManager->TargetTask( _taskId, b->m_pos );

		}
	}
}

bool AI::HasDarwinianPowerup()
{
    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_taskManager->m_tasks.Size(); ++i )
    {
        Task *t = team->m_taskManager->m_tasks[i];
        if( t )
        {
            if( t->m_type == GlobalResearch::TypeSubversion ||
                t->m_type == GlobalResearch::TypeBooster ||
                t->m_type == GlobalResearch::TypeHotFeet ||
                t->m_type == GlobalResearch::TypeRage )
            {
                return true;
            }
        }
    }
    return false;
}


// ============================================================================


AITarget::AITarget()
:   Building(),
    m_teamCountTimer(0.0),
    m_linkId(-1),
    m_priorityModifier(0.0),
	m_radarTarget(false),
	m_statueTarget(-1),
    m_scoreZone(-1),
    m_checkCliffs(1),
    m_turretTarget(false),
    m_crateTarget(false),
    m_waterTarget(false)
{
    m_type = Building::TypeAITarget;

    memset( m_friendCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_enemyCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_idleCount, 0, NUM_TEAMS*sizeof(int) );    
    memset( m_priority, 0, NUM_TEAMS*sizeof(double) );
    memset( m_aiObjectiveTarget, false, NUM_TEAMS * sizeof(bool) );
    
    SetShape( g_app->m_resource->GetShape("aitarget.shp") );

    m_teamCountTimer = syncfrand( 2.0 );
}

AITarget::~AITarget()
{
    m_neighbours.EmptyAndDelete();
}

void AITarget::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_linkId = ((AITarget *) _template)->m_linkId;
    m_priorityModifier = ((AITarget *) _template)->m_priorityModifier;
    m_checkCliffs = ((AITarget *)_template)->m_checkCliffs;

    if( m_pos.y <= 0.0 ) m_waterTarget = true;
}

void AITarget::SetBuildingLink(int _buildingId)
{
    Building *b = g_app->m_location->GetBuilding( _buildingId );
    if( b && b->m_type == Building::TypeAITarget )
    {
        m_linkId = _buildingId;
    }
}

void AITarget::RecalculateNeighbours()
{
    m_neighbours.EmptyAndDelete();
    if( m_pos.y <= 0.0 ) m_waterTarget = true;

    if( g_app->m_editing )
    {
        for( int i = 0; i < g_app->m_location->m_levelFile->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_levelFile->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_levelFile->m_buildings[i];

                if( building->m_type == Building::TypeAITarget &&
                    building != this )
                {
                    double distance = ( building->m_pos - m_pos ).Mag();
                    bool checkCliffs = ( m_checkCliffs && ((AITarget *)building)->m_checkCliffs );
                    bool isWalkable = g_app->m_location->IsWalkable( m_pos, building->m_pos, checkCliffs );
                    if( (distance <= AITARGET_LINKRANGE ||
                        m_linkId == building->m_id.GetUniqueId() )
                        && (isWalkable || m_waterTarget || ((AITarget *)building)->m_waterTarget ) )
                    {
                        NeighbourInfo *ni = new NeighbourInfo;
                        ni->m_neighbourId = building->m_id.GetUniqueId();
                        ni->m_laserFenceId = -1;
                        m_neighbours.PutData( ni );
                    }
                }
            }
        }
    }
    else
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building->m_type == Building::TypeAITarget &&
                    building != this &&
                    !building->m_destroyed )
                {
                    double distance = ( building->m_pos - m_pos ).Mag();
                    if( (distance <= AITARGET_LINKRANGE || m_linkId == building->m_id.GetUniqueId() ) )
                    {
                        bool checkCliffs = ( m_checkCliffs && ((AITarget *)building)->m_checkCliffs );
                        if( m_linkId == building->m_id.GetUniqueId() ) checkCliffs = false;
                        int fenceId = -1;
                        bool radar = m_radarTarget && ((AITarget *)building)->m_radarTarget;
                        if( g_app->m_location->IsWalkable( m_pos, building->m_pos, checkCliffs, false, &fenceId ) 
                            || radar || m_waterTarget || ((AITarget *)building)->m_waterTarget )
                        {
                            NeighbourInfo *ni = new NeighbourInfo;
                            ni->m_neighbourId = building->m_id.GetUniqueId();
                            ni->m_laserFenceId = fenceId;
                            m_neighbours.PutData( ni );
                        }
                    }
                }

                if( building->m_type == Building::TypeMultiwiniaZone )
                {
                    if( (m_pos - building->m_pos).Mag() <= building->m_radius )
                    {
                        if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeCaptureTheStatue )
                        {
                            if( building->m_id.GetTeamId() == 255 )
                            {
                                // this is a cts statue spawn zone
                                m_scoreZone = building->m_id.GetUniqueId();
                            }
                        }
                        else if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeKingOfTheHill )
                        {
                             if( building->m_id.GetTeamId() == 255 )
                            {
                                // this is a koth score zone
                                m_scoreZone = building->m_id.GetUniqueId();
                            }
                        }
                    }
                }
            }
        }
    }
}

double AITarget::GetRange()
{
    double range = 100.0;
    if( g_app->Multiplayer() )
    {
        range = 200.0;
        if( m_scoreZone != -1 )
        {
            MultiwiniaZone *zone = (MultiwiniaZone *)g_app->m_location->GetBuilding( m_scoreZone );
            if( zone && zone->m_type == Building::TypeMultiwiniaZone )
            {
                range = zone->m_size;
            }
        }
    }

    return range;
}

void AITarget::RecountTeams()
{
    memset( m_friendCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_enemyCount, 0, NUM_TEAMS*sizeof(int) );
    memset( m_idleCount, 0, NUM_TEAMS*sizeof(int) );
        
    int numFound;
    double range = GetRange();
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, range, &numFound );
    for( int j = 0; j < numFound; ++j )
    {
        WorldObjectId id = s_neighbours[j];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity )
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
				if( g_app->m_gameMode == App::GameModeMultiwinia )
				{
					if( id.GetTeamId() == t )
						++m_friendCount[t];
					else if( !g_app->m_location->IsFriend( id.GetTeamId(), t ) )
						++m_enemyCount[t];
				}
				else
				{
					if( g_app->m_location->IsFriend( id.GetTeamId(), t ) ) 
						++m_friendCount[t];
					else
						++m_enemyCount[t];
				}
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
                Building *closest = g_app->m_location->GetBuilding( darwinian->m_closestAITarget );
                if( closest )
                {
                    if( (darwinian->m_pos - m_pos).Mag() < (darwinian->m_pos - closest->m_pos).Mag() )
                    {
                        darwinian->m_closestAITarget = m_id.GetUniqueId();
                    }
                }
                else
                {
                    darwinian->m_closestAITarget = m_id.GetUniqueId();
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


double AITarget::IsNearTo( int _aiTargetId )
{
    for( int i = 0; i < m_neighbours.Size(); ++i )
    {
        int thisBuildingId = m_neighbours[i]->m_neighbourId;
        if( thisBuildingId == _aiTargetId )
        {
            Building *building = g_app->m_location->GetBuilding(thisBuildingId);
            if( building && building->m_type == TypeAITarget )
            {
                return (m_pos - building->m_pos).Mag();
            }
        }
    }

    return -1.0;
}

bool AITarget::IsImportant()
{
    int id = -1;
    return IsImportant(id);
}

bool AITarget::IsImportant( int &_buildingId )
{
    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex( i ) )
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_ports.Size() > 0 ||
                b->m_type == Building::TypeCrate ||
                b->m_type == Building::TypeRadarDish ||
                (g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot &&
                 b->m_type == Building::TypeTrunkPort ) ||
                (g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeCaptureTheStatue &&
                 b->m_type == Building::TypeMultiwiniaZone ) ) 
            {
                if( (m_pos - b->m_pos ).Mag() < 100.0 )
                {
                    _buildingId = b->m_id.GetUniqueId();
                    return true;
                }
            }
        }
     }
     return false;
}

bool AITarget::NeedsDefending( int _teamId )
{
    if( m_id.GetTeamId() != _teamId ) return false;

    int numOfficers = AI::s_ai[_teamId]->SearchForOfficers(m_pos, 300.0);
    if( m_scoreZone != -1 && g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeCaptureTheStatue ) return true;

    for( int i = 0; i < m_neighbours.Size(); ++i )
    {
        AITarget *t = (AITarget *)g_app->m_location->GetBuilding(m_neighbours[i]->m_neighbourId);
        if( t && t->m_type == Building::TypeAITarget )
        {
            if( !g_app->m_location->IsFriend( m_id.GetTeamId(), t->m_id.GetTeamId() ) && 
                t->m_id.GetTeamId() != 255 &&
                t->m_enemyCount[m_id.GetTeamId()] > 30 &&
                numOfficers  == 0) 
            {
                return true;  // connected to a point we dont own - bad
            }
        }
    }
    return false;
}

bool AITarget::Advance()
{
    //
    // Time to re-evaluate who is nearby?

    m_teamCountTimer -= SERVER_ADVANCE_PERIOD;
    if( m_teamCountTimer <= 0.0 )
    {
        RecalculatePriority();

        m_teamCountTimer = 2.0;
        RecountTeams();
        RecalculateOwnership();
    }

    return m_destroyed;
}

void AITarget::RecalculatePriorityAssault()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_aiObjectiveTarget[t] ) continue;

        // in assault mode we dont want any special priority calculations
        // all priorities should be controlled by AIObjectives, and the priority from those objectives should propogate through teh rest of the network
        // this should theoretically lead all the darwinians straight to their objectives
        if( !m_aiObjectiveTarget[t] )
        {
            m_priority[t] = 0.0;
            double prioritySum = 0.0;
            int num = 0;
            bool radarNeighbour = false;
            for( int i = 0; i < m_neighbours.Size(); ++i )
            {
                LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( m_neighbours[i]->m_laserFenceId );
                if( !fence || g_app->m_location->IsFriend(fence->m_id.GetTeamId(), t) || !fence->IsEnabled() )
                {
                    int neighbourId = m_neighbours[i]->m_neighbourId;
                    AITarget *target = (AITarget *) g_app->m_location->GetBuilding(neighbourId);
                    if( target && target->m_type == TypeAITarget )
                    {
                        if( target->m_priority[t] > 0.0 )
                        {
                            prioritySum += target->m_priority[t];
                            num++;
                        }

                        if( target->m_radarTarget )
                        {
                            radarNeighbour = true;
                        }
                    }
                }
            }           
            if( num > 0 )
            {
                prioritySum /= (double) num + 1;// (m_neighbours.Size() + 1);
                m_priority[t] = prioritySum;
            }

            if( m_radarTarget && !radarNeighbour)
            {
                // this dish is connected but the other dish isnt, which means it wont have an ai target
                // this means we wont get propogated priorities from the other side, so just boost it artificially
                m_priority[t] += 0.35;
            }
            m_priority[t] += m_priorityModifier;

            if( m_crateTarget )
            {
                if( AI::s_ai[t] )
                {
                    AI *ai = AI::s_ai[t];
                    AIObjective *objective = (AIObjective *)g_app->m_location->GetBuilding( ai->m_currentObjective );
                    if( objective && objective->m_nextObjective == -1 )
                    {
                        // this is our final objective, so just ignore all crates and go right for it
                        m_priority[t] = 0.0;
                    }
                }
            }
        }
        //m_aiObjectiveTarget[t] = false;
    }
}

void AITarget::RecalculatePriorityCTS()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_aiObjectiveTarget[t] ) continue;

        double prioritySum = 0.0;
        int num = 0;
        for( int i = 0; i < m_neighbours.Size(); ++i )
        {
            LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( m_neighbours[i]->m_laserFenceId );
            if( !fence || fence->m_id.GetTeamId() == t || !fence->IsEnabled() )
            {
                int neighbourId = m_neighbours[i]->m_neighbourId;
                AITarget *target = (AITarget *) g_app->m_location->GetBuilding(neighbourId);
                if( target && target->m_type == TypeAITarget )
                {
                    num++;
                    prioritySum += max( 0.0, target->m_priority[t] );
                }
            }
        }            
        if( num > 0 )
        {
            prioritySum /= (double) num;
            m_priority[t] = prioritySum;
        }

        if( g_app->m_location->IsFriend( m_id.GetTeamId(), t ) ) m_priority[t] /= 2.0;

        m_priority[t] += m_priorityModifier;

        bool important = IsImportant();
        if( important )
        {
            if( m_id.GetTeamId() == 255 )
            {
                m_priority[t] = 0.95;   
            }
        }

        if( m_statueTarget != -1 ) 
        {
            Statue *statue = (Statue *)g_app->m_location->GetBuilding( m_statueTarget );
            if( statue )
            {
                if( statue->m_id.GetTeamId() != t ||
                    statue->m_numLifters < statue->m_maxLifters ||
                    m_enemyCount[t] > 10 )
                {
                    m_priority[t] = 1.0;
                }
            }
        }

        m_priority[t] = min( 1.0, m_priority[t] );
    }
}

void AITarget::RecalculatePriorityStandard()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_aiObjectiveTarget[t] ) continue;

        int importantBuildingId = -1;
        bool important = IsImportant(importantBuildingId);
        Building *importantBuilding = g_app->m_location->GetBuilding( importantBuildingId );

        bool grabIt = false;
        if( !g_app->Multiplayer() ) grabIt = true;
        if( importantBuilding )
        {
            if( importantBuilding->m_id.GetTeamId() != t &&
                importantBuilding->m_type != Building::TypeTrunkPort ) 
            {
                grabIt = true;
            }
        }
        if( grabIt )
        {                
            // Owned by nobody, so grab it quick
            m_priority[t] = 0.95;                
        }
        else if( !g_app->m_location->IsFriend( m_id.GetTeamId(), t ) )
        {
            // Owned by the enemy
            m_priority[t] = 0.8;

            if( m_friendCount[t] > m_enemyCount[t] * 0.5 )
            {
                // We are attacking, so continue to concentrate effort here
                m_priority[t] = 1.0;
            }
        }
        else
        {
            // Owned by us
            if( m_enemyCount[t] > m_friendCount[t] * 0.5 )
            {
                // We are under attack, give us a very high priority
                m_priority[t] = 1.0;
            }
            else
            {
                double prioritySum = 0.0;
                int num = 0;
                for( int i = 0; i < m_neighbours.Size(); ++i )
                {
                    LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( m_neighbours[i]->m_laserFenceId );
                    if( !fence || fence->m_id.GetTeamId() == t || !fence->IsEnabled() )
                    {
                        int neighbourId = m_neighbours[i]->m_neighbourId;
                        AITarget *target = (AITarget *) g_app->m_location->GetBuilding(neighbourId);
                        if( target && target->m_type == TypeAITarget )
                        {
                            num++;
                            prioritySum += target->m_priority[t];
                        }
                    }
                }            
                prioritySum /= (double) num;
                m_priority[t] = prioritySum;
                if( g_app->m_location->IsFriend( m_id.GetTeamId(), t ) ) m_priority[t] /= 2.0;
            }
        }

	    if( m_radarTarget) m_priority[t] += 0.35;

        
        m_priority[t] += (double) m_neighbours.Size() * 0.01;
        m_priority[t] += m_priorityModifier;
        m_priority[t] = min( m_priority[t], 1.0 );

        if( g_app->Multiplayer() )
        {
            if( m_scoreZone != -1 )
            {
                MultiwiniaZone *b = (MultiwiniaZone *)g_app->m_location->GetBuilding( m_scoreZone );
                if( b )
                {
                    if( g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeKingOfTheHill )
                    {
                        // koth zones are normally large enough that we could own the target but not the zone, so check for zone control
                        if( b->m_id.GetTeamId() != t ||
                            b->m_totalCount - b->m_teamCount[t] > 10 )
                        {
                            m_priority[t] = 0.95;
                        }
                    }
                }
            }
        }
    }
}

void AITarget::RecalculatePriorityRocketRiot()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
        if( m_aiObjectiveTarget[t] ) continue;

        int importantBuildingId = -1;
        bool important = IsImportant(importantBuildingId);
        Building *importantBuilding = g_app->m_location->GetBuilding( importantBuildingId );

        bool grabIt = false;
        double mod = 0.0;
        if( importantBuilding )
        {
            if( importantBuilding->m_id.GetTeamId() != t &&
                importantBuilding->m_type != Building::TypeTrunkPort ) 
            {
                grabIt = true;
            }

            if( importantBuilding->m_type == Building::TypeTrunkPort && 
                g_app->m_multiwinia->m_gameType == Multiwinia::GameTypeRocketRiot &&
                importantBuilding->m_id.GetTeamId() == t )
            {
                // in rocket riot, we want to get all our darwinians to gather at our trunkport if 
                // our rocket is loading passengers, so that the armours can collect them easily
                LobbyTeam *lobbyTeam = &g_app->m_multiwinia->m_teams[t];
                EscapeRocket *rocket = (EscapeRocket *)g_app->m_location->GetBuilding( lobbyTeam->m_rocketId );
                if( rocket )
                {
                    if( rocket->m_state == EscapeRocket::StateLoading )
                    {
                        grabIt = true;
                    }
                    else
                    {
                        mod = 0.1;
                    }
                }
            }
        }
        if( grabIt )
        {                
            // Owned by nobody, so grab it quick
            m_priority[t] = 0.95;                
        }
        else
        {
            double prioritySum = 0.0;
            int num = 0;
            for( int i = 0; i < m_neighbours.Size(); ++i )
            {
                LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( m_neighbours[i]->m_laserFenceId );
                if( !fence || fence->m_id.GetTeamId() == t || !fence->IsEnabled() )
                {
                    int neighbourId = m_neighbours[i]->m_neighbourId;
                    AITarget *target = (AITarget *) g_app->m_location->GetBuilding(neighbourId);
                    if( target && target->m_type == TypeAITarget )
                    {
                        num++;
                        prioritySum += target->m_priority[t];
                    }
                }
            }            
            prioritySum /= (double) num + 1;
            m_priority[t] = prioritySum;
            if( g_app->m_location->IsFriend( m_id.GetTeamId(), t ) ) m_priority[t] /= 2.0;
        }

	    if( m_radarTarget) m_priority[t] += 0.35;

        
        //m_priority[t] += (double) m_neighbours.Size() * 0.01;
        m_priority[t] += m_priorityModifier + mod;
        m_priority[t] = min( m_priority[t], 1.0 );
    }
}

void AITarget::RecalculatePriority()
{
    // no point doing all this with easy ai as its not used, except in assault mode
    if( m_id.GetUniqueId() == 92 )
    {
        if( g_app->m_multiwiniaTutorial && g_app->m_multiwiniaTutorialType == App::MultiwiniaTutorial2 )
        {
            if( !g_app->m_multiwiniaHelp->m_tutorial2AICanSpawn )
            {
                m_priority[2] = 1.0;
            }
            else if( SpawnPoint::s_numSpawnPoints[2] == 1 )
            {
                m_priority[2] = 0.0;
                return;
            }
        }
    }

    if( g_app->m_multiwinia->m_aiType == Multiwinia::AITypeEasy &&
        g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault) return;
    
    switch( g_app->m_multiwinia->m_gameType )
    {
        case Multiwinia::GameTypeAssault:
        case Multiwinia::GameTypeBlitzkreig:
            RecalculatePriorityAssault();
            break;

        case Multiwinia::GameTypeCaptureTheStatue:
            RecalculatePriorityCTS();
            break;

        case Multiwinia::GameTypeRocketRiot:
            RecalculatePriorityRocketRiot();
            break;

        default:
            RecalculatePriorityStandard();
            break;
    }    
}


void AITarget::Render( double _predictionTime )
{    
    if( g_app->m_editing )
    {
        Building::Render( _predictionTime );
        RecalculateNeighbours();
    }       
    
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,80,0), 10.0, "Green  : f%d e%d i%d", m_friendCount[0], m_enemyCount[0], m_idleCount[0] );
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,70,0), 10.0, "Red    : f%d e%d i%d", m_friendCount[1], m_enemyCount[1], m_idleCount[1] );

//    for( int t = 0; t < NUM_TEAMS; ++t )
//    {
//        Team *team = g_app->m_location->m_teams[t];
//        if( team->m_teamType != TeamTypeUnused )
//        {            
//            glColor3ubv( team->m_colour.GetData() );
//            g_editorFont.DrawText3DCentre( m_pos+Vector3(0,60+t*15,0), 10.0, "%2.2f", m_priority[t] );
//        }
//    }


    if( g_prefsManager->GetInt( "RenderAIInfo", 0 ) == 1 && g_app->m_location && g_app->m_location ) 
    {
        int t = g_app->m_location->GetTeamFromPosition( g_prefsManager->GetInt( "AIInfoTeam", 1 ) );
        if( t != 255 )
        {
            Team *team = g_app->m_location->m_teams[t];
            if( team->m_teamType != TeamTypeUnused )
            {            
                glColor4f(1.0f, 1.0f, 1.0f, 0.0f );
                g_editorFont.SetRenderOutline(true);
                g_editorFont.DrawText3DCentre( m_pos+Vector3(0,60+t*15,0), 10.0, "%2.2f", m_priority[t] );
                RGBAColour col = team->m_colour;
                col.a = 255;
                glColor3ubv( col.GetData() );
                g_editorFont.SetRenderOutline(false);
                g_editorFont.DrawText3DCentre( m_pos+Vector3(0,60+t*15,0), 10.0, "%2.2f", m_priority[t] );
            }
        }
        //g_editorFont.DrawText3DCentre( m_pos + Vector3( 0,110, 0 ), 10.0f, "Neighbours: %d", m_neighbours.Size() );
    }
}


void AITarget::RenderAlphas( double _predictionTime )
{   
    //Building::RenderAlphas( _predictionTime );
    //if( !g_app->m_editing ) return;
    if( !g_app->m_editing && !g_prefsManager->GetInt( "RenderAIInfo", 0 ) ) return;

    //g_gameFont.DrawText3D( m_pos, 30.0, "Priority: %2.1f", m_priority[1] );

    Vector3 height(0,20,0);
    glLineWidth( 2.0f );
    glShadeModel( GL_SMOOTH );
    glEnable( GL_BLEND );

    for( int i = 0; i < m_neighbours.Size(); ++i )
    {
        int buildingId = m_neighbours[i]->m_neighbourId;
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building && building->m_type == TypeAITarget )
        {
            RGBAColour col;
            if( g_app->m_editing )
            {
                col.Set( 255, 0, 0, 255 );
            }
            else if( m_id.GetTeamId() == 255 )
            {
                col.Set( 128, 128, 128, 255 );
            }
            else
            {
                col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
                //glColor4ub( col.r, col.g, col.b, 255 );
            }
            Vector3 pos = m_pos;
            pos.y += 30.0;
            Vector3 dir = (building->m_pos - m_pos).Normalise();
            RenderArrow( pos, pos + dir * (m_pos - building->m_pos).Mag() / 2.0, 3.0, col );

            /*glBegin( GL_LINES );
            if( g_app->m_editing )
            {
                glColor4ub( 255, 0, 0, 255 );
            }
            else if( m_id.GetTeamId() == 255 )
            {
                glColor4ub( 128, 128, 128, 255 );
            }
            else
            {
                RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
                glColor4ub( col.r, col.g, col.b, 255 );
            }

            glVertex3fv( (m_pos+height).GetData() );

            if( g_app->m_editing )
            {
                glColor4ub( 255, 0, 0, 255 );
            }
            else if( building->m_id.GetTeamId() == 255 )
            {
                glColor4ub( 128, 128, 128, 255 );
            }
            else
            {
                RGBAColour col = g_app->m_location->m_teams[ building->m_id.GetTeamId() ]->m_colour;
                glColor4ub( col.r, col.g, col.b, 255 );
            }

            glVertex3fv( (building->m_pos+height).GetData() );
            glEnd();*/
        }
    }

    glShadeModel( GL_FLAT );
    


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


bool AITarget::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool AITarget::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


int AITarget::GetBuildingLink()
{
    return m_linkId;
}

void AITarget::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    if( _in->TokenAvailable() )
    {
        m_linkId = atoi( _in->GetNextToken() );
        if( _in->TokenAvailable() )
        {
            m_priorityModifier = atof( _in->GetNextToken() );
            if( _in->TokenAvailable() )
            {
                m_checkCliffs = atoi( _in->GetNextToken() );
            }
        }
    }
}

void AITarget::Write( TextWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-6d", m_linkId );
    _out->printf( "%-1.2f ", m_priorityModifier );
    _out->printf( "%-6d", m_checkCliffs );
}



// ============================================================================


AISpawnPoint::AISpawnPoint()
:   Building(),
    m_entityType( Entity::TypeDarwinian ),
    m_count(20),
    m_period(60),
    m_timer(0.0),
    m_numSpawned(0),
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

    if( g_app->Multiplayer() )
    {
        m_timer = 0;
    }
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
                    double distance = ( building->m_pos - m_pos ).Mag();
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

        if( m_timer > 0.0 )
        {
            m_timer -= SERVER_ADVANCE_PERIOD;
            if( m_timer <= 0.0 )
            {
                m_timer = -1.0;
                m_numSpawned = 0;
            }
        }


        //
        // Spawn entities if we're in the middle of a batch

        if( m_timer <= 0.0 )
        {
            g_app->m_location->SpawnEntities( m_pos, m_id.GetTeamId(), -1, m_entityType, 1, g_zeroVector, 20.0, 100.0, m_routeId );
            ++m_numSpawned;
        
            if( m_numSpawned >= m_count )
            {
                m_timer = m_period + syncsfrand(m_period/3.0);
                m_spawnLimit--;
            }
        }
    }

    return false;
}


void AISpawnPoint::RenderAlphas( double _predictionTime )
{    
    //if( g_app->m_editing )
    {
        RGBAColour colour;
        if( m_id.GetTeamId() == 255 ) colour.Set( 100, 100, 100, 255 );
        else colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;

        RenderSphere( m_pos, 10.0, colour );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,30,0), 5.0, "Spawn %d %s's", m_count, Entity::GetTypeName(m_entityType) );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,25,0), 5.0, "Every %d seconds", m_period );
        g_editorFont.DrawText3DCentre( m_pos+Vector3(0,20,0), 5.0, "Next spawn in %d seconds", (int) m_timer );
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


void AISpawnPoint::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-6d %-6d %-6d %-6d %-6d %-6d", m_activatorId, m_entityType, m_count, m_period, m_spawnLimit, m_routeId );
}


bool AISpawnPoint::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool AISpawnPoint::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

