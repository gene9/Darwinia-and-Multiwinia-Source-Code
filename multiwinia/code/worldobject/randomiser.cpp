#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/3d_sprite.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "worldobject/randomiser.h"
#include "worldobject/darwinian.h"

#include "main.h"
#include "globals.h"
#include "app.h"
#include "location.h"
#include "level_file.h"
#include "team.h"
#include "entity_grid.h"
#include "particle_system.h"

#define RANDOMISER_RANGE 100.0


static std::vector<WorldObjectId> s_neighbours;

  
Randomiser::Randomiser()
:   WorldObject(),
    m_timer(0.0),
    m_life(15.0)
{    
    m_type = EffectRandomiser;
}
    

bool Randomiser::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0 ) return true;

    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0 )
    {
        m_timer = 0.5 + syncfrand( 2.5 );

        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, RANDOMISER_RANGE, &numFound );

        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = s_neighbours[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && entity->m_type == Entity::TypeDarwinian &&
                ((Darwinian *)entity)->m_state != Darwinian::StateInsideArmour )
            {
                int teamId = syncrand() % ( g_app->m_location->m_levelFile->m_numPlayers );
                g_app->m_location->SpawnEntities( entity->m_pos, teamId, -1, Entity::TypeDarwinian, 1, entity->m_vel, 0.0 );            
                entity->m_destroyed = true;
            }
        }
    }   

    return false;
}


void Randomiser::Render( double _predictionTime )
{
    double area = RANDOMISER_RANGE;
    double angle = g_gameTime * 3.0;
    Vector3 dif( area * iv_sin(angle), 0.0, area * iv_cos(angle) );

    int teamId = rand() % NUM_TEAMS;
    RGBAColour col = g_app->m_location->m_teams[teamId]->m_colour;
    
    Vector3 pos = m_pos + dif;
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0;
    g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0, col );

    teamId = rand() % NUM_TEAMS;
    col = g_app->m_location->m_teams[teamId]->m_colour;

    pos = m_pos - dif;
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0;
    g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeMuzzleFlash, 60.0, col );
}

