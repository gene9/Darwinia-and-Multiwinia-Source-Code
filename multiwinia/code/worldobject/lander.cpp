#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/shape.h"
#include "lib/math/random_number.h"

#include "camera.h"
#include "app.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "gametimer.h"

#include "worldobject/lander.h"



Lander::Lander()
:   Entity(),
    m_state(StateSailing),
    m_spawnTimer(0.0)
{
    m_type = Entity::TypeLander;

    m_shape = g_app->m_resource->GetShape( "lander.shp" );
    AppDebugAssert( m_shape );
}

bool Lander::Advance( Unit *_unit )
{    
    m_front.Set( -1, 0, 0 );

    if( m_dead ) 
    {
        bool amIDead = AdvanceDead( _unit );
        if( amIDead ) 
		{		
            g_app->m_location->Bang( m_pos, 30.0, 50.0 );
			return true;
		}
    }

    switch( m_state )
    {
        case StateSailing:              return AdvanceSailing();                break;
        case StateLanded:               return AdvanceLanded();                 break;
    }
    
    double worldSizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double worldSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();
    if( m_pos.x < 0.0 ) m_pos.x = 0.0;
    if( m_pos.z < 0.0 ) m_pos.z = 0.0;
    if( m_pos.x >= worldSizeX ) m_pos.x = worldSizeX;
    if( m_pos.z >= worldSizeZ ) m_pos.z = worldSizeZ;

    return false;    
}

void Lander::ChangeHealth( int amount )
{
    g_app->m_particleSystem->CreateParticle( m_pos, g_zeroVector, Particle::TypeMuzzleFlash );
}

bool Lander::AdvanceSailing()
{
    m_vel = m_front * m_stats[StatSpeed];
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    
    double groundLevel = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    m_pos.y = groundLevel;
    if( m_pos.y < 0.0 ) m_pos.y = 0.0;
    
    if( groundLevel > 0.0 )
    {
        m_vel.Zero();
        m_state = StateLanded;
    }

    return false;
}

bool Lander::AdvanceLanded()
{
    m_vel.Zero();
    
    m_spawnTimer -= SERVER_ADVANCE_PERIOD;
    if( m_spawnTimer <= 0.0 )
    {
        int unitId;
        int numToSpawn = syncfrand(2.0) + 2.0;
        Unit *unit = g_app->m_location->m_teams[0]->NewUnit( Entity::TypeLaserTroop, numToSpawn, &unitId, m_pos );
        g_app->m_location->SpawnEntities( m_pos, m_id.GetTeamId(), unitId, Entity::TypeLaserTroop, numToSpawn, g_zeroVector, 0 );

        Vector3 offset( 0.0, 0.0, syncsfrand(200.0) );
        unit->SetWayPoint( m_pos + m_front * 750.0 + offset );
        
        m_spawnTimer = m_stats[StatRate];
    }

    return false;
}

void Lander::Render( double predictionTime, int teamId )
{
    //
    // Work out our predicted position and orientation

    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    
    Vector3 entityUp = g_app->m_location->m_landscape.m_normalMap->GetValue( predictedPos.x, predictedPos.z );
    Vector3 entityFront = m_front;
    entityFront.Normalise();
    Vector3 entityRight = entityFront ^ entityUp;
    entityUp = entityRight ^ entityFront;

    if( !m_dead )
    {
        RGBAColour colour = g_app->m_location->m_teams[ teamId ]->m_colour;
        
        if( m_reloading > 0.0 )
        {
            colour.r = ( colour.r == 255 ? 255 : 100 );
            colour.g = ( colour.g == 255 ? 255 : 100 );
            colour.b = ( colour.b == 255 ? 255 : 100 );
        }
        glColor3ubv( colour.GetData() );
        
        //
        // 3d Shape

		g_app->m_renderer->SetObjectLighting();

        glEnable        (GL_CULL_FACE);
        glDisable       (GL_TEXTURE_2D);
        glEnable        (GL_COLOR_MATERIAL);
        glDisable       (GL_BLEND);

        Matrix34 mat(entityFront, entityUp, predictedPos);
    	m_shape->Render(predictionTime, mat);

        glEnable        (GL_BLEND);
        glDisable       (GL_COLOR_MATERIAL);
        glEnable        (GL_TEXTURE_2D);
		g_app->m_renderer->UnsetObjectLighting();
        glEnable        (GL_CULL_FACE);
     
    }
}

