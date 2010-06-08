#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/math/random_number.h"

#include "particle_system.h"
#include "app.h"
#include "main.h"
#include "location_editor.h"

#include "worldobject/smokemarker.h"

SmokeMarker::SmokeMarker()
:   Building()
{
    m_type = Building::TypeSmokeMarker;
}

bool SmokeMarker::Advance()
{
    for( int i = 0; i < 3; ++i )
    {
        Vector3 vel = g_upVector;
        vel.x += sfrand(0.4f);
        vel.z += sfrand(0.4f);
        vel.SetLength( 50.0f );

        float size = 200.0f + frand(100.0f);

        g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeMissileTrail, size );
    }

    return false;
}

void SmokeMarker::RenderAlphas( float _predictionTime )
{
    if( g_app->m_editing )
    {
#ifdef LOCATION_EDITOR
        Building::RenderAlphas( _predictionTime );
        RenderSphere( m_pos, 30.0f );
        if( g_app->m_editing &&
            g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            RenderSphere( m_pos, 35.0f );
        }
#endif
    }
}

bool SmokeMarker::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool SmokeMarker::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool SmokeMarker::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
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
