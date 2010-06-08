#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/math/random_number.h"

#include "particle_system.h"
#include "app.h"
#include "main.h"
#include "location_editor.h"

#include "worldobject/eruptionmarker.h"

EruptionMarker::EruptionMarker()
:   Building(),
    m_erupting(false),
    m_timer(0.0)
{
    m_type = Building::TypeEruptionMarker;
}

bool EruptionMarker::Advance()
{
    if( m_erupting )
    {
        m_timer -= SERVER_ADVANCE_PERIOD;
        if( m_timer <= 0.0 )
        {
            m_erupting = false;
            m_timer = 0.0;
        }

        for( int i = 0; i < 10; ++i )
        {
            Vector3 vel = g_upVector;
            vel.x += sfrand();
            vel.z += sfrand();
            vel.SetLength( 100.0 );

            double size = 300.0 + frand(200.0);

            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeVolcano, size );
        }
    }
    else
    {
        if( syncrand() % 1000 == 0 )
        {
            m_erupting = true;
            m_timer = 3.0 + syncfrand(4.0);
        }
    }

    return false;
}

void EruptionMarker::RenderAlphas( double _predictionTime )
{
    if( g_app->m_editing )
    {
#ifdef LOCATION_EDITOR
        Building::RenderAlphas( _predictionTime );
        RenderSphere( m_pos, 30.0 );
        if( g_app->m_editing &&
            g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            RenderSphere( m_pos, 35.0 );
        }
#endif
    }
}

bool EruptionMarker::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool EruptionMarker::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool EruptionMarker::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                double _rayLen, Vector3 *_pos, Vector3 *_norm)
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
