#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/shape.h"
#include "lib/math_utils.h"
#include "lib/debug_render.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "globals.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "taskmanager.h"
#include "global_world.h"
#include "routing_system.h"
#include "particle_system.h"
#include "gametimer.h"

#include "worldobject/teleport.h"
#include "worldobject/insertion_squad.h"

LList<TeleportMap> Teleport::m_teleportMap;



// *** Constructor
Teleport::Teleport()
:   Building(),
    m_timeSync(0.0),
    m_sendPeriod(1.0),
    m_entrance(NULL)
{   
}


// *** Initialise
void Teleport::SetShape( Shape *_shape )
{
    Building::SetShape( _shape );
    
    const char entranceName[] = "MarkerTeleportEntrance";
    m_entrance = m_shape->m_rootFragment->LookupMarker( entranceName );
    AppReleaseAssert( m_entrance, "Teleport: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", entranceName, m_shape->m_name );
}


// *** Advance
bool Teleport::Advance ()
{    
    m_timeSync -= SERVER_ADVANCE_PERIOD;
    if( m_timeSync < 0.0 ) m_timeSync = 0.0;
    
    //
    // Advance people who are in transit

    for( int i = 0; i < m_inTransit.Size(); ++i )
    {
        WorldObjectId *id = m_inTransit.GetPointer(i);
        WorldObject *obj = g_app->m_location->GetEntity( *id );
        Entity *ent = (Entity *) obj;
        if( ent )
        {
            bool removeMe = UpdateEntityInTransit( ent );
            if( removeMe )
            {
                m_inTransit.RemoveData(i);
                --i;
            }
        }
        else
        {
            m_inTransit.RemoveData(i);
            --i;
        }
    }


    //
    // If a unit no longer exists, remove it from our teleport map
    // to prevent confusion (since unit ids are re-used)

    for( int i = 0; i < m_teleportMap.Size(); ++i )
    {
        TeleportMap *map = m_teleportMap.GetPointer(i);
        WorldObjectId unitId( map->m_teamId, map->m_fromUnitId, -1, -1 );
        Unit *unit = g_app->m_location->GetUnit( unitId );
        if( !unit )
        {
            m_teleportMap.RemoveData(i);
            --i;
        }
    }

    return Building::Advance();
}


bool Teleport::IsInView()
{
    Vector3 startPoint = GetStartPoint();
    Vector3 endPoint = GetEndPoint();

    Vector3 midPoint = ( startPoint + endPoint ) / 2.0;
    double radius = ( startPoint - endPoint ).Mag() / 2.0;
    radius += m_radius;

    if( g_app->m_camera->SphereInViewFrustum( midPoint, radius ) )
    {
        return true;
    }

    return Building::IsInView();
}


void Teleport::RenderAlphas ( double predictionTime )
{
//    Vector3 pos, front;
//    GetEntrance( pos, front );
//    RenderSphere( pos, 3.0, RGBAColour(255,0,0) );
//    RenderSphere( pos, 15.0, RGBAColour(255,0,0) );
    
    //RenderHitCheck();
    
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    for( int i = 0; i < m_inTransit.Size(); ++i )
    {
        WorldObjectId *id = m_inTransit.GetPointer(i);
        WorldObject *obj = g_app->m_location->GetEntity( *id );
        if( obj )
        {
            Entity *ent = (Entity *) obj;
            Vector3 pos = obj->m_pos + obj->m_vel * predictionTime;
            RenderSpirit( pos, ent->m_id.GetTeamId() );
        }
    }

    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    Building::RenderAlphas( predictionTime );
}


void Teleport::RenderSpirit( Vector3 const &_pos, int _teamId )
{
    Vector3 pos = _pos;

    int innerAlpha = 100;
    int outerAlpha = 50;
    int spiritInnerSize = 2;
    int spiritOuterSize = 6;

    RGBAColour colour;
    if( _teamId >= 0 ) colour = g_app->m_location->m_teams[ _teamId ]->m_colour;      

    double size = spiritInnerSize;
    glColor4ub(colour.r, colour.g, colour.b, innerAlpha );            

    glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    

    size = spiritOuterSize;
    glColor4ub(colour.r, colour.g, colour.b, outerAlpha );            
    glBegin( GL_QUADS );
        glVertex3dv( (pos - g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetRight()*size).GetData() );
        glVertex3dv( (pos + g_app->m_camera->GetUp()*size).GetData() );
        glVertex3dv( (pos - g_app->m_camera->GetRight()*size).GetData() );
    glEnd();    
}


bool Teleport::Connected()
{
    return false;
}


bool Teleport::ReadyToSend()
{
    return( m_timeSync <= 0.0 );
}

void Teleport::EnterTeleport( WorldObjectId _id, bool _relay )
{
    WorldObject *wobj = g_app->m_location->GetEntity( _id );
    Entity *entity = (Entity *) wobj;

    if( entity )
    {
        Vector3 oldPos = entity->m_pos;
        
        if( !_relay )
        {
            Unit *oldUnit = g_app->m_location->GetUnit( _id ); 
            if( oldUnit )
            {
                //
                // Look for the new unit that i'm going to
    
                int newUnitId = -1;
                for( int i = 0; i < m_teleportMap.Size(); ++i )
                {
                    TeleportMap *map = m_teleportMap.GetPointer(i);
                    if( map->m_teamId == _id.GetTeamId() &&
                        map->m_fromUnitId == _id.GetUnitId() )
                    {
                        newUnitId = map->m_toUnitId;                
                    }
                }

                AppDebugAssert( oldUnit );
        
                Unit *newUnit = NULL;

                if( newUnitId != -1 )
                {
                    if( g_app->m_location->m_teams[ _id.GetTeamId() ]->m_units.ValidIndex( newUnitId ) )
                    {
                        newUnit = g_app->m_location->m_teams[ _id.GetTeamId() ]->m_units[ newUnitId ];
                    }
                    else
                    {
                        newUnitId = -1;
                    }
                }
            
                if( newUnitId == -1 )
                {
                    //
                    // Oh well, i'm the first, so create a new unit
                    newUnit = g_app->m_location->m_teams[ _id.GetTeamId() ]->NewUnit( oldUnit->m_troopType, 
                                                                                  oldUnit->m_entities.NumUsed(), 
                                                                                  &newUnitId,
																			      m_pos);

                    //
                    // Special case :
                    // If this was an insertion squad we just broke up, 
                    // then we must create a Task to represent the new squad

                    if( oldUnit->m_troopType == Entity::TypeInsertionSquadie )
                    {
                        // Shut down the old task

                        float oldTimer = -1.0f;
                        for( int i = 0; i < g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->m_tasks.Size(); ++i )
                        {
                            Task *task = g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->m_tasks[i];
                            if( task->m_type == GlobalResearch::TypeSquad &&
                                task->m_objId == WorldObjectId( oldUnit->m_teamId, oldUnit->m_unitId, -1, -1 ) )
                            {
                                oldTimer = task->m_lifeTimer;
                                g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->m_tasks.RemoveData(i);
                                delete task;
                                break;
                            }
                        }
                        
                        Task *task = new Task( newUnit->m_teamId );
                        task->m_type = GlobalResearch::TypeSquad;
                        task->m_state = Task::StateRunning;
                        task->m_objId.Set( newUnit->m_teamId, newUnit->m_unitId, -1, -1 );    
                        if( g_app->Multiplayer() ) task->m_lifeTimer = oldTimer;
                        bool success = g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->RegisterTask( task );
                        if( success )  g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->SelectTask( task->m_id );

                        ((InsertionSquad *)newUnit)->m_weaponType = ((InsertionSquad *)oldUnit)->m_weaponType;
                        ((InsertionSquad *)newUnit)->m_controllerId = ((InsertionSquad *)oldUnit)->m_controllerId;
                        ((InsertionSquad *)oldUnit)->m_controllerId = -1;
                        Task *controller = g_app->m_location->m_teams[ _id.GetTeamId() ]->m_taskManager->GetTask( ((InsertionSquad *)newUnit)->m_controllerId );
                        if( controller )
                        {
                            controller->m_objId = task->m_objId;
                            controller->m_route->AddWayPoint( m_id.GetUniqueId() );
                        }
                    }

                    
                    if( oldUnit->m_routeId != -1 )
                    {
                        newUnit->m_routeId = oldUnit->m_routeId;
                        newUnit->m_routeWayPointId = oldUnit->m_routeWayPointId+1;                    
                    }
                    else
                    {
                        Vector3 exitPos, exitFront;
                        GetExit( exitPos, exitFront );                    
                        Vector3 newWayPoint(exitPos + exitFront * 50.0);
                        newWayPoint += Vector3( SFRAND(35.0), 0.0, SFRAND(35.0) );
                        newWayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( newWayPoint.x, newWayPoint.z );
				        newUnit->SetWayPoint(newWayPoint);
                    }
                
                    TeleportMap map;
                    map.m_teamId = entity->m_id.GetTeamId();
                    map.m_fromUnitId = oldUnit->m_unitId;
                    map.m_toUnitId = newUnit->m_unitId;
                    m_teleportMap.PutDataAtStart( map );
                }

                AppDebugAssert( newUnit );


                // Put me into the new unit

                int newUnitIndex;
                Entity *newEntity = newUnit->NewEntity( &newUnitIndex );
                int newUniqueId = newEntity->m_id.GetUniqueId();
                *newEntity = *entity;
                entity = newEntity;                                      
                entity->m_id.SetUnitId(newUnitId);
                entity->m_id.SetIndex(newUnitIndex);
                entity->m_id.SetUniqueId(newUniqueId);
                entity->m_onGround = false;
                entity->m_enabled = false;
                entity->m_vel.Zero();

                newUnit->RecalculateOffsets();
            }
        }

        entity->m_pos = GetStartPoint();   
        UpdateEntityInTransit( entity );                

        WorldObjectId newId( entity->m_id );
        m_inTransit.PutData(newId);
        m_timeSync = m_sendPeriod;
    }
}

bool Teleport::GetEntrance( Vector3 &_pos, Vector3 &_front )
{
    Matrix34 rootMat(m_front, m_up, m_pos);
    Matrix34 worldMat = m_entrance->GetWorldMatrix(rootMat);
    _pos = worldMat.pos;
    _front = worldMat.f;
    return true;
}

bool Teleport::GetExit( Vector3 &_pos, Vector3 &_front )
{
    AppDebugAssert( false );
    return false;
}

Vector3 Teleport::GetStartPoint()
{
    AppDebugAssert(false);
    return Vector3();
}

Vector3 Teleport::GetEndPoint()
{
    AppDebugAssert(false);
    return Vector3();
}

bool Teleport::UpdateEntityInTransit( Entity *_entity )
{
    AppDebugAssert(false);
    return false;
}
