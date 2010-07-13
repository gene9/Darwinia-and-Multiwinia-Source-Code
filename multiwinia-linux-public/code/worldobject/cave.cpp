#include "lib/universal_include.h"

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_utils.h"
#include "lib/math/random_number.h"

#include "explosion.h"
#include "app.h"
#include "entity_grid.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "gametimer.h"

#include "worldobject/cave.h"



Cave::Cave()
:   Building(),
    m_troopType(-1),
    m_unitId(-1),
    m_spawnTimer(0.0),
    m_dead(false)
{
    m_type = TypeCave;
    m_troopType = Entity::TypeVirii;

    SetShape( g_app->m_resource->GetShape( "cave.shp" ) );

    const char spawnPointName[] = "MarkerSpawnPoint";
    m_spawnPoint = m_shape->m_rootFragment->LookupMarker( spawnPointName );
    AppReleaseAssert( m_spawnPoint, "Cave: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", spawnPointName, m_shape->m_name );
}


bool Cave::Advance()
{
    if( m_dead ) return true;

    if( m_id.GetTeamId() < 0 || m_id.GetTeamId() >= NUM_TEAMS ) return false;
    if( g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_teamType == TeamTypeUnused ) return false;

    m_spawnTimer -= SERVER_ADVANCE_PERIOD;

    if( m_spawnTimer <= 0.0 )
    {        
        //
        // Only spawn if the area is sufficiently empty

        Matrix34 rootMat(m_front, g_upVector, m_pos);
        Matrix34 worldMat = m_spawnPoint->GetWorldMatrix(rootMat);
        Vector3 spawnPoint = worldMat.pos;
        spawnPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( spawnPoint.x, spawnPoint.z );

        int numFound = g_app->m_location->m_entityGrid->GetNumFriends( spawnPoint.x, spawnPoint.z, 100.0, m_id.GetTeamId() );
        if( numFound < 30 )
        {
            //int numToCreate = int( syncfrand(5.0) + 3.0 );
            Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];

            //Unit *unit = team->NewUnit( m_troopType, numToCreate, &m_unitId );                        
            //unit->m_wayPoint = spawnPoint + worldMat.f * 25.0;
            //unit->m_wayPoint += Vector3( SFRAND(25.0), 0.0, SFRAND(25.0) );
            //unit->m_wayPoint.y = g_app->m_location->m_landscape.m_heightMap->GetValue( unit->m_wayPoint.x, unit->m_wayPoint.z );

            g_app->m_location->SpawnEntities( spawnPoint, m_id.GetTeamId(), m_id.GetUniqueId(), m_troopType, 1, g_zeroVector, 0.0 );
        }

        m_spawnTimer = syncfrand(0.5);
    }

    return false;
}


void Cave::Damage( double _damage )
{
    if( _damage > 80.0 )
    {
        Matrix34 mat( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat );
        m_dead = true;
    }
}
