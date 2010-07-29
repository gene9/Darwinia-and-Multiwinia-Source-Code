#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/3d_sprite.h"
#include "lib/preferences.h"

#include "worldobject/crate.h"

#include "explosion.h"
#include "main.h"
#include "app.h"
#include "global_world.h"
#include "camera.h"
#include "renderer.h"
#include "team.h"
#include "globals.h"
#include "location.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"

#include "sound/soundsystem.h"


Crate::Crate()
:   Building(),
    m_contents(0)
{
    m_type = TypeCrate;

    SetShape( g_app->m_resource->GetShape( "researchitem.shp" ) );

    m_front.RotateAroundY( frand(2.0f * M_PI) );
}


void Crate::Initialise ( Building *_template )
{
    Building::Initialise( _template );
    m_contents = ((Crate *) _template)->m_contents;
}

bool Crate::Advance()
{
    if( m_vel.Mag() > 1.0f )
    {
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        m_vel *= ( 1.0f - SERVER_ADVANCE_PERIOD * 0.5f );

        Matrix34 mat( m_front, g_upVector, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
    }
    else
    {
        m_vel.Zero();
    }

    return false;
}


void Crate::Render( float _predictionTime )
{
    Vector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX( g_gameTime * 1.0f );
    rotateAround.RotateAroundZ( g_gameTime * 0.7f );
    rotateAround.Normalise();

    m_front.RotateAround( rotateAround * g_advanceTime );
    m_up.RotateAround( rotateAround * g_advanceTime );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
	Matrix34 mat(m_front, m_up, predictedPos);

	m_shape->Render(0.0f, mat);
}


void Crate::RenderAlphas( float _predictionTime )
{
    Building::RenderAlphas( _predictionTime );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );

    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 0;

    float alpha = 1.0f;

    for( int i = 0; i < maxBlobs; ++i )
    {
        Vector3 pos = m_centrePos;
        pos.x += sinf(timeIndex+i) * i * 0.3f;
        pos.y += cosf(timeIndex+i) * sinf(i*10) * 5;
        pos.z += cosf(timeIndex+i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex+i*10) * 7.0f;
        size = max( size, 2.0f );

        //glColor4f( 0.6f, 0.2f, 0.1f, alpha);
        glColor4f( 0.1f, 0.2f, 0.8f, alpha);

		if ( m_id.GetTeamId() != 255 )
		{
			RGBAColour colour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;
			glColor4ub(colour.r,colour.g,colour.b,alpha);
		}
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    //
    // Starbursts

    alpha = 0.3f;

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    if( alpha > 0.0f )
    {
        int numStars = 10;
        if( buildingDetail == 2 ) numStars = 5;
        if( buildingDetail == 3 ) numStars = 2;

        for( int i = 0; i < numStars; ++i )
        {
            Vector3 pos = m_centrePos;
            pos.x += sinf(timeIndex+i) * i * 0.3f;
            pos.y += (cosf(timeIndex+i) * cosf(i*10) * 2);
            pos.z += cosf(timeIndex+i) * i * 0.3f;

            float size = i * 10 * alpha;
            if( i > numStars - 2 ) size = i * 20 * alpha;

            //glColor4f( 1.0f, 0.4f, 0.2f, alpha );
            glColor4f( 0.1f, 0.2f, 0.8f, alpha);

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
            glEnd();
        }
    }

    glShadeModel    ( GL_FLAT );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

}


bool Crate::RenderPixelEffect(float _predictionTime)
{
//	Matrix34 mat(m_front, m_up, m_pos);
//	m_shape->Render(0.0f, mat);
//	g_app->m_renderer->MarkUsedCells(m_shape, mat);
    return false;
}


void Crate::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_contents = GlobalResearch::GetType( _in->GetNextToken() );
}


void Crate::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d", m_contents );
}


void Crate::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "AquireResearch" );
}

bool Crate::IsInView()
{
    if( Building::IsInView() ) return true;

    if( g_app->m_camera->PosInViewFrustum( m_pos+Vector3(0,g_app->m_camera->GetPos().y,0) ))
    {
        return true;
    }

    return false;
}

