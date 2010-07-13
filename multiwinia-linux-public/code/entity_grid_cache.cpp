#include "lib/universal_include.h"

#include "app.h"
#include "location.h"
#include "entity_grid.h"
#include "entity_grid_cache.h"
#include "gametimer.h"
#include "team.h"

#include "lib/math/random_number.h"


EntityGridCacheElement::EntityGridCacheElement()
:   m_worldX(0),
    m_worldZ(0),
    m_rangeSqd(0),
    m_halfRangeSqd(0),
    m_numFound(0),
    m_timeIndex(0),
    m_teamId(255)
{
}


EntityGridCache::EntityGridCache()
{
    m_cache.SetSize( 20 );
    m_cache.SetStepSize( 20 );
}


EntityGridCache::~EntityGridCache()
{
	m_cache.EmptyAndDelete();
}


int EntityGridCache::FindCacheHit( double worldX, double worldZ, double range, unsigned char teamId )
{
    int bestIndex = -1;
    double bestDistSqd = DBL_MAX;
	int entityCacheSize = m_cache.Size();

    for( int i = 0; i < entityCacheSize; ++i )
    {
        if( m_cache.ValidIndex(i) )
        {
            EntityGridCacheElement *element = m_cache[i];

            if( element->m_teamId == teamId )
            {
				double distanceX = element->m_worldX - worldX;
				double distanceY = element->m_worldZ - worldZ;
                double distanceSqd = distanceX * distanceX;
				distanceSqd += distanceY * distanceY;

                if( distanceSqd < element->m_halfRangeSqd &&
                    distanceSqd < bestDistSqd )
                {
                    bestDistSqd = distanceSqd;
                    bestIndex = i;
                }
            }
        }
    }

    return bestIndex;
}


std::vector<WorldObjectId> *EntityGridCache::GetEnemies(double worldX, double worldZ,double range,
                                                        int *numFound, unsigned char myTeam)
{
    *numFound = 0;

    if( !g_app->m_location ) return NULL;
    if( !g_app->m_location->m_entityGrid ) return NULL;

    //
    // Look in our cache for the nearest/best hit first

    int cacheHitIndex = FindCacheHit( worldX, worldZ, range, myTeam );

    if( cacheHitIndex != -1 )
    {
        EntityGridCacheElement *element = m_cache[cacheHitIndex];
        *numFound = element->m_numFound;
        return &(element->m_ids);
    }

    //
    // Didn't find one, so query the entity grid
    // And put the results into the cache

    EntityGridCacheElement *element = new EntityGridCacheElement();

    g_app->m_location->m_entityGrid->GetEnemies( element->m_ids, worldX, worldZ, range, numFound, myTeam );

    element->m_worldX = worldX;
    element->m_worldZ = worldZ;
    element->m_rangeSqd = range * range;
	double halfRange = range / 2.0;
    element->m_halfRangeSqd = halfRange * halfRange;
    element->m_teamId = myTeam;
    element->m_numFound = *numFound;
    element->m_timeIndex = g_gameTimer.GetGameTime();
    m_cache.PutData( element );

    return &(element->m_ids);
}


void EntityGridCache::Render()
{
    float timeNow = g_gameTimer.GetGameTime();

    glEnable( GL_BLEND );

    for( int i = 0; i < m_cache.Size(); ++i )
    {
        if( m_cache.ValidIndex(i) )
        {
            EntityGridCacheElement *element = m_cache[i];

            float age = timeNow - element->m_timeIndex;
            float alpha = 1.0f - age;
            if( alpha < 0.0f ) alpha = 0.0f;
            RGBAColour teamCol = g_app->m_location->m_teams[ element->m_teamId ]->m_colour;

            glColor4ub( teamCol.r, teamCol.g, teamCol.b, alpha * 255 );

            glBegin( GL_LINE_LOOP );

            for( int j = 0; j < 20; ++j )
            {
                float angle = 2.0f * M_PI * j / 20.0f;
                float x = element->m_worldX + cosf(angle) * sqrtf(element->m_rangeSqd);
                float z = element->m_worldZ + sinf(angle) * sqrtf(element->m_rangeSqd);
                float y = g_app->m_location->m_landscape.m_heightMap->GetValue( x, z );

                y += 50;

                glVertex3f( x, y, z );
            }

            glEnd();
        }
    }
}


void EntityGridCache::Advance()
{
    double timeNow = g_gameTimer.GetGameTime();

    for( int i = 0; i < m_cache.Size(); ++i )
    {
        if( m_cache.ValidIndex(i) )
        {
            EntityGridCacheElement *element = m_cache[i];

            double age = timeNow - element->m_timeIndex;

            if( age > ENTITYGRIDCACHE_MAXAGE )
            {
                m_cache.RemoveData(i);
				delete element;
            }
        }
    }
}

