#include "lib/universal_include.h"
#include "lib/math/random_number.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/filesys/text_file_writer.h"

#include "lib/filesys/text_stream_readers.h"

#include "worldobject/wall.h"
#include "worldobject/aiobjective.h"

#include "app.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"

#define WALL_LENGTH 100.0
#define WALL_DEPTH  20.0
#define WALL_HEIGHT 75.0f

Wall::Wall()
:   m_damage(0.0f),
    m_fallSpeed(0.0f),
    m_lastDamageTime(0.0f),
    m_scale(1.0f),
    m_objectiveLink(-1),
    m_registered(false)
{
    m_type = TypeWall;

    SetShape( g_app->m_resource->GetShape( "wall.shp" ) );
}

void Wall::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_objectiveLink = ((Wall *)_template)->m_objectiveLink;
}

void Wall::SetDetail( int _detail )
{
    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if( m_shape )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        mat.u *= m_scale;
        mat.r *= m_scale;
        mat.f *= m_scale;

        m_centrePos = m_shape->CalculateCentre( mat );        
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );
    }
}   

bool Wall::Advance()
{
    if( m_destroyed ) return true;

    if( m_objectiveLink != -1 && !m_registered )
    {
        AIObjective *ai = (AIObjective *)g_app->m_location->GetBuilding( m_objectiveLink );
        if( ai && ai->m_type == Building::TypeAIObjective )
        {
            ai->m_wallTarget = m_id.GetUniqueId();
            m_registered = true;
        }
    }

    /*float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    float targetY = landHeight + m_damage/10 + 0.01f;

    if( m_damage < 0.0 )
    {
        m_damage += 1.0;
    }

    if( m_lastDamageTime != 0.0 && 
        GetNetworkTime() > m_lastDamageTime + 20.0 )
    {
        m_fallSpeed += 2.0 * SERVER_ADVANCE_PERIOD;
        m_pos.y += m_fallSpeed * SERVER_ADVANCE_PERIOD;

        if( m_pos.y >= targetY )
        {
            m_lastDamageTime = 0.0;
        }        
    }
    else if( m_pos.y > targetY )
    {
        m_fallSpeed += 10.0 * SERVER_ADVANCE_PERIOD;
        m_pos.y -= m_fallSpeed * SERVER_ADVANCE_PERIOD;

        if( m_pos.y < targetY ) 
        {
            m_fallSpeed = 0.0;
            m_pos.y = targetY;

            Vector3 right = m_front ^ g_upVector;
            for( int i = -5; i < 5; ++i )
            {
                Vector3 particlePos = m_pos + right * i * frand(10.0);
                particlePos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( particlePos.x, particlePos.z ) + 10.0;
                Vector3 particleVel( sfrand(10.0), frand(10.0), sfrand(10.0) );
                
                g_app->m_particleSystem->CreateParticle( particlePos, particleVel, Particle::TypeRocketTrail, 50.0 );
            }
        }
    }*/
    
    return false;
}

void Wall::Damage( double _damage )
{
    if( _damage <= -300.0 )
    {
        Matrix34 transform( m_front, m_up, m_pos );
        g_explosionManager.AddExplosion( m_shape, transform, 3.0 );

        m_destroyed = true;
    }
    //m_damage += _damage;
    m_lastDamageTime = GetNetworkTime();
}

void Wall::Render( double _predictionTime )
{
    if( m_destroyed ) return;
#ifdef DEBUG_RENDER_ENABLED
	if (g_app->m_editing)
	{
		Vector3 pos(m_pos);
		pos.y += 5.0;
		RenderArrow(pos, pos + m_front * 20.0, 4.0);
	}
#endif

    Vector3 predictedPos = m_pos - Vector3(0,m_fallSpeed,0) * _predictionTime;
	Matrix34 mat(m_front, g_upVector, predictedPos);
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;
	m_shape->Render(_predictionTime, mat);
}

bool Wall::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    if(m_shape)
    {
        Vector3 right = m_front ^ g_upVector;
        right.Normalise();
        
        Vector3 t1, t2, t3, t4;

        t1 = m_pos - (right * (WALL_LENGTH / 2.0f)) - (m_front * (WALL_DEPTH / 2.0f));
        t2 = m_pos + (right * (WALL_LENGTH / 2.0f)) - (m_front * (WALL_DEPTH / 2.0f));
        t3 = m_pos - (right * (WALL_LENGTH / 2.0f)) + (m_front * (WALL_DEPTH / 2.0f));
        t4 = m_pos + (right * (WALL_LENGTH / 2.0f)) + (m_front * (WALL_DEPTH / 2.0f));

        t1.y = 0.0f;
        t2.y = 0.0f;
        t3.y = 0.0f;
        t4.y = 0.0f;

        Vector3 spherePos = _pos;
        spherePos.y = 0.0f;

        return ( SphereTriangleIntersection( spherePos, _radius, t1, t2, t3 ) || SphereTriangleIntersection( spherePos, _radius, t2, t3, t4 ) );

        /*SpherePackage sphere(_pos, _radius );
        Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
        return m_shape->SphereHit(&sphere, transform, true );*/
    }
    return true;
}


bool Wall::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *norm )
{
	if (m_shape)
	{
		/*RayPackage ray(_rayStart, _rayDir, _rayLen);
		Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
		return m_shape->RayHit(&ray, transform, true);*/
        Vector3 right = m_front ^ g_upVector;
        right.Normalise();
        
        Vector3 t1, t2, t3, t4;

        t1 = m_pos - (right * (WALL_LENGTH / 2.0f));// - (m_front * (WALL_DEPTH / 2.0f));
        t2 = m_pos + (right * (WALL_LENGTH / 2.0f));// - (m_front * (WALL_DEPTH / 2.0f));
        t3 = m_pos - (right * (WALL_LENGTH / 2.0f)) + (g_upVector * WALL_HEIGHT / 2.0f);
        t4 = m_pos + (right * (WALL_LENGTH / 2.0f)) + (g_upVector * WALL_HEIGHT / 2.0f);

        t1.y = g_app->m_location->m_landscape.m_heightMap->GetValue( t1.x, t1.z );
        t2.y = g_app->m_location->m_landscape.m_heightMap->GetValue( t2.x, t2.z );

        return ( RayTriIntersection(_rayStart, _rayDir, t1, t2, t3, _rayLen ) || RayTriIntersection(_rayStart, _rayDir, t2, t3, t4, _rayLen ) );
	}
	else
	{
		return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
	}	
}

bool Wall::DoesShapeHit(Shape *_shape, Matrix34 _theTransform)
{
    if( m_shape )
    {
        Matrix34 ourTransform(m_front, m_up, m_pos);
        ourTransform.u *= m_scale;
        ourTransform.r *= m_scale;
        ourTransform.f *= m_scale;

        //return m_shape->ShapeHit( _shape, _theTransform, ourTransform, true );
        return _shape->ShapeHit( m_shape, ourTransform, _theTransform, true );
    }
    else
    {
        SpherePackage package( m_pos, m_radius );
        return _shape->SphereHit( &package, _theTransform, true );
    }
}

void Wall::SetBuildingLink( int _buildingId )
{
    m_objectiveLink = _buildingId;
}

int Wall::GetBuildingLink()
{
    return m_objectiveLink;
}

void Wall::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    if( _in->TokenAvailable() )
    {
        m_objectiveLink = atoi( _in->GetNextToken() );
    }
}

void Wall::Write( TextWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%4d", m_objectiveLink );
}