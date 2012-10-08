#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"
#include "lib/preferences.h"

#include "worldobject/uplink.h"

#include "main.h"
#include "app.h"
#include "location.h"
#include "camera.h"
#include "sepulveda.h"

#define SPIRIT_SLOW		5.0f

Uplink::Uplink()
:   Building(),
	m_direction(0)
{
    m_type = TypeUplink;
    SetShape( g_app->m_resource->GetShape("goddish.shp") );
	while ( m_packets.Size() < 30 )
	{
		UplinkDataPacket *packet = new UplinkDataPacket();
		packet->m_currentProgress = 0.0f - frand();
		m_packets.PutData(packet);
	}
}


void Uplink::Initialise( Building *_template )
{
    Building::Initialise( _template );

    Uplink *uplink = (Uplink *) _template;

	m_direction = uplink->m_direction;
	clamp(m_direction,-1,1);
}

Vector3 Uplink::GetDataNode()
{
    Vector3 m_node = m_pos + m_front * 200 + m_up * -300;
	return m_node;
}

void Uplink::Render( float _predictionTime )
{
	Building::Render( _predictionTime);
}

void Uplink::RenderAlphas( float _predictionTime )
{
	if ( m_direction == 0 )
	{
		Building::RenderAlphas( _predictionTime );
		return;
	}
	Vector3 theirPos = GetDataNode();
	Vector3 ourPos = (GetDataNode() + m_front * -1000) + m_up * 1500;

	Vector3 camToOurPos = g_app->m_camera->GetPos() - ourPos;
    Vector3 ourPosRight = camToOurPos ^ ( theirPos - ourPos );

    Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    Vector3 theirPosRight = camToTheirPos ^ ( theirPos - ourPos );

    glDisable   ( GL_CULL_FACE );
    glDepthMask ( false );
    float blue = 0.5f + fabs( sinf(g_gameTime) ) * 0.5f;
    glColor4f   ( 0.25f, 0.25f, blue, 0.8f );
//    glColor4f   ( 0.25f, 0.25f, 0.5f, 0.8f );
//    glColor4f   ( 0.9f, 0.9f, 0.5f, 1.0f );

    float size = m_radius/2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    if( buildingDetail == 1 )
    {
        glEnable    ( GL_BLEND );
        glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
        glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        size = m_radius;
    }

	//size = 1.0f;

    theirPosRight.SetLength( size );
    ourPosRight.SetLength( size );

    glBegin( GL_QUADS );
        glTexCoord2f(0.1f, 0);      glVertex3fv( (ourPos - ourPosRight).GetData() );
        glTexCoord2f(0.1f, 1);      glVertex3fv( (ourPos + ourPosRight).GetData() );
        glTexCoord2f(0.9f, 1);      glVertex3fv( (theirPos + theirPosRight).GetData() );
        glTexCoord2f(0.9f, 0);      glVertex3fv( (theirPos - theirPosRight).GetData() );
    glEnd();

    glDisable       ( GL_TEXTURE_2D );


 	theirPos = GetDataNode();
	ourPos = (GetDataNode() + m_front * -1000) + m_up * 1500;
    for( int j = 0; j < m_packets.Size(); ++j )
 	{
 		UplinkDataPacket *packet = m_packets[j];
		float predictedProgress = packet->m_currentProgress + (_predictionTime/SPIRIT_SLOW* m_direction);
 		if( predictedProgress >= 0.0f && predictedProgress <= 1.0f )
		{
			Vector3 position = ourPos + ( theirPos - ourPos ) * predictedProgress;
			RenderPacket( position );
		}
	}
    Building::RenderAlphas( _predictionTime );
}

void Uplink::RenderPacket( Vector3 const &_pos )
{
    Vector3 pos = _pos;

    float packetSize = 30.0f;
    Vector3 camRight = g_app->m_camera->GetRight() * packetSize;
    Vector3 camUp = g_app->m_camera->GetUp() * packetSize;

//    glColor4ub(150, 50, 25, innerAlpha );
    glColor4ub(255, 255, 255, 200 );

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );

    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3fv( (pos - camUp - camRight).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3fv( (pos - camUp + camRight).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3fv( (pos + camUp + camRight).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3fv( (pos + camUp - camRight).GetData() );
    glEnd();


    glDepthMask ( true );
    glDisable   ( GL_TEXTURE_2D );
    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable   ( GL_BLEND );
    glEnable    ( GL_CULL_FACE );

    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
}

void Uplink::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_direction = atoi( _in->GetNextToken() );
	clamp(m_direction,-1,1);

}


void Uplink::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d  ", m_direction );
}

bool Uplink::Advance ()
{
	if ( m_direction != 0 )
	{
		for( int j = 0; j < m_packets.Size(); ++j )
		{
			UplinkDataPacket *packet = m_packets.GetData(j);
			packet->m_currentProgress += (SERVER_ADVANCE_PERIOD/SPIRIT_SLOW * m_direction);
			if( m_direction == 1 && packet->m_currentProgress >= 1.0f )
			{
				packet->m_currentProgress = 0.0f - frand();
			}
			else if ( m_direction == -1 && packet->m_currentProgress <= 0.0f )
			{
				packet->m_currentProgress = 1.0f + frand();
			}
		}
	}
	return Building::Advance();
}
