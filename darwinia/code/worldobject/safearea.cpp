#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/file_writer.h"
#include "lib/text_stream_readers.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "worldobject/safearea.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "entity_grid.h"
#include "main.h"
#include "particle_system.h"
#include "global_world.h"



SafeArea::SafeArea()
:   Building(),
    m_size(50.0f),
    m_entitiesRequired(0),
    m_recountTimer(0.0f),
    m_entitiesCounted(0),
    m_entityTypeRequired(Entity::TypeDarwinian)
{
    m_type = TypeSafeArea;
}


void SafeArea::Initialise ( Building *_template )
{
    Building::Initialise( _template );

    m_size = ((SafeArea *) _template)->m_size;
    m_entitiesRequired = ((SafeArea *) _template)->m_entitiesRequired;
    m_entityTypeRequired = ((SafeArea *) _template)->m_entityTypeRequired;

    m_radius = m_size;
    m_centrePos = m_pos + Vector3(0,m_radius/2,0);
}


bool SafeArea::Advance()
{
    //
    // Is the world awake yet ?

    if( !g_app->m_location ) return false;
    if( !g_app->m_location->m_teams ) return false;
    if( g_app->m_location->m_teams[ m_id.GetTeamId() ].m_teamType == Team::TeamTypeUnused ) return false;

    
    if( GetHighResTime() > m_recountTimer )
    {
        int numFound;
        WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( m_pos.x, m_pos.z, m_size, &numFound, m_id.GetTeamId() );
        m_entitiesCounted = 0;
        
        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = ids[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && 
                entity->m_type == m_entityTypeRequired && 
                !entity->m_dead )
            {
                ++m_entitiesCounted;
            }
        }

        m_recountTimer = GetHighResTime() + 2.0f;
    
        if( (m_id.GetTeamId() == 1 && m_entitiesCounted <= m_entitiesRequired) ||
            (m_id.GetTeamId() != 1 && m_entitiesCounted >= m_entitiesRequired) )
        {
            GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
            if( gb && !gb->m_online )
            {
                gb->m_online = true;
                g_app->m_globalWorld->EvaluateEvents();
            }        
        }

    }

    return false;
}

 
void SafeArea::Render( float predictionTime )
{
    if( g_app->m_editing )
    {
        RGBAColour colour;

        if( m_id.GetTeamId() != 255 )
        {
            colour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;    
        }
        colour.a = 255;

#ifdef DEBUG_RENDER_ENABLED
        RenderSphere( m_pos, 20.0f, colour );
#endif
        int numSteps = 30;
        float angle = 0.0f;

        glColor4ubv(colour.GetData() );
        glLineWidth( 2.0f );
        glBegin( GL_LINE_LOOP );
        for( int i = 0; i <= numSteps; ++i )
        {
            float xDiff = m_size * sinf(angle);
            float zDiff = m_size * cosf(angle);
            Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
	        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z) + 10.0f;
            if( pos.y < 2 ) pos.y = 2;
            glVertex3fv( pos.GetData() );
            angle += 2.0f * M_PI / (float) numSteps;
        }
        glEnd();
    }
    else
    {
/*
        float angle = g_gameTime * 2.0f;
        Vector3 dif( m_size * sinf(angle), 0.0f, m_size * cosf(angle) );
        
        Vector3 pos = m_pos + dif;
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
        g_app->m_particleSystem->CreateParticle( pos, g_upVector*2 + dif/30, Particle::TypeMuzzleFlash, 100.0f );

        pos = m_pos - dif;
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0f;
        g_app->m_particleSystem->CreateParticle( pos, g_upVector*2 - dif/30, Particle::TypeMuzzleFlash, 100.0f );
*/
    }
    

    //char *entityTypeRequired = Entity::GetTypeName( m_entityTypeRequired );
    //g_editorFont.DrawText3DCentre( m_pos + Vector3(0,m_size/2,0), 10.0f, "%d / %d %ss", m_entitiesCounted, m_entitiesRequired, entityTypeRequired );
}


bool SafeArea::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool SafeArea::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool SafeArea::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          float _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( g_app->m_editing )
    {
        return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );
    }
    else
    {
        return false;
    }
}


char *SafeArea::GetObjectiveCounter()
{
    static char result[256];
    sprintf( result, "%s : %d", LANGUAGEPHRASE("objective_currentcount"), m_entitiesCounted );
    return result;
}


void SafeArea::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_size = atof( _in->GetNextToken() );
    m_entitiesRequired = atoi( _in->GetNextToken() );
    m_entityTypeRequired = atoi( _in->GetNextToken() );
}


void SafeArea::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-6.2f", m_size );
    _out->printf( " %d", m_entitiesRequired );
    _out->printf( " %d", m_entityTypeRequired );
}

