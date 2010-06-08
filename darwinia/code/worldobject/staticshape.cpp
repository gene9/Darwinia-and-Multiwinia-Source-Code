#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"

#include "worldobject/staticshape.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "sepulveda.h"


StaticShape::StaticShape()
:   Building(),    
    m_scale(1.0f)
{
    m_type = TypeStaticShape;

    strcpy( m_shapeName, "none" );
}


void StaticShape::Initialise( Building *_template )
{
    Building::Initialise( _template );

    StaticShape *staticShape = (StaticShape *) _template;

    m_scale = staticShape->m_scale;
    SetShapeName( staticShape->m_shapeName );
}


void StaticShape::SetDetail( int _detail )
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


void StaticShape::SetShapeName( char *_shapeName )
{
    strcpy( m_shapeName, _shapeName );

    if( strcmp( m_shapeName, "none" ) != 0 )
    {
        SetShape( g_app->m_resource->GetShape( m_shapeName ) );

        Matrix34 mat( m_front, m_up, m_pos );
        mat.u *= m_scale;
        mat.r *= m_scale;
        mat.f *= m_scale;

        m_centrePos = m_shape->CalculateCentre( mat );        
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );
    }
}


bool StaticShape::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          float _rayLen, Vector3 *_pos, Vector3 *norm )
{
	if (m_shape)
	{
		RayPackage ray(_rayStart, _rayDir, _rayLen);
		Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
		return m_shape->RayHit(&ray, transform, true);
	}
	else
	{
		return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
	}	
}

bool StaticShape::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    if(m_shape)
    {
        SpherePackage sphere(_pos, _radius);
        Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
        return m_shape->SphereHit(&sphere, transform);
    }
    else
    {
        float distance = (_pos - m_pos).Mag();
        return( distance <= _radius + m_radius );
    }
}

bool StaticShape::DoesShapeHit(Shape *_shape, Matrix34 _theTransform)
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


void StaticShape::Render( float _predictionTime )
{
    if( m_shape )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        mat.u *= m_scale;
        mat.r *= m_scale;
        mat.f *= m_scale;
    
        glEnable( GL_NORMALIZE );
        m_shape->Render( _predictionTime, mat );
        glDisable( GL_NORMALIZE );
    }        
    else
    {
        RenderSphere( m_pos, 40.0f );
    }
}


bool StaticShape::Advance()
{
    return Building::Advance();
}


void StaticShape::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_scale = atof( _in->GetNextToken() );

    SetShapeName( _in->GetNextToken() );
}


void StaticShape::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6.2f  ", m_scale );
    _out->printf( "%s  ", m_shapeName );
}

