#include "lib/universal_include.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"
#include "lib/resource.h"
#include "lib/bitmap.h"
#include "lib/debug_render.h"

#include "lib/filesys/binary_stream_readers.h"

#include "worldobject/flag.h"

#include "app.h"
#include "main.h"
#include "camera.h"
#include "team.h"
#include "location.h"
#include "renderer.h"
#include "markersystem.h"
#include "multiwinia.h"
#include "taskmanager_interface_multiwinia.h"


Flag::Flag()
:   m_texId(-1),
    m_size(0),
    m_viewFlipped(false),
	m_isSelected(false),
    m_isHighlighted(false),
    m_radius(0.0f)
{
}


void Flag::Initialise()
{
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            Vector3 targetPos = m_pos + 
                                (double) x / (double) FLAG_RESOLUTION * m_front +
                                (double) z / (double) FLAG_RESOLUTION * m_up;
            
            m_flag[x][z] = targetPos;
        }
    }
}


void Flag::SetTexture( int _textureId )
{
    m_texId = _textureId;
}


void Flag::SetPosition( Vector3 const &_pos )
{
    m_pos = _pos;
}


void Flag::SetTarget( Vector3 const &_pos )
{
    m_target = _pos;
}


void Flag::SetOrientation( Vector3 const &_front, Vector3 const &_up )
{
    m_front = _front;
    m_up = _up;
}


void Flag::SetSize( double _size )
{
    m_size = _size;
}


void Flag::SetViewFlipped( bool _flipped )
{  
    m_viewFlipped = _flipped;
}


void Flag::Render( int _teamId, char *_fileName )
{
    START_PROFILE( "RenderFlag" );
	
	Vector3 targetPos;
    //
    // Advance the flag

       
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            float advanceTime = g_advanceTime;
            if( g_gameTimer.GetGameSpeed() == 0.0f )
            {
                advanceTime = 0.1f;
            }
            float factor1 = advanceTime * 10 * (FLAG_RESOLUTION - x) / FLAG_RESOLUTION;
            if( x == 0 ) factor1 = 1.0f;
            if( m_size > 40 ) factor1 = 0.7f;            
            float windFlapFactor = 1.0f;

            if( m_isHighlighted || m_isSelected )
            {
                windFlapFactor = 0.3f;
                factor1 = 0.7f;
            }

            float factor2 = 1.0f - factor1;

            if( x == 0 )
            {
                targetPos = m_pos + ((m_up * m_size) / 2.0f) + 
                                ((float) x / (float) (FLAG_RESOLUTION-1) * m_front * m_size) +
                                ((float) z / (float) (FLAG_RESOLUTION-1) * m_up * m_size);
            }
            else
            {
                targetPos = m_flag[x-1][z] + 
                                1.0f/(float) (FLAG_RESOLUTION-1) * m_front * m_size;

            }
            

            targetPos.x += windFlapFactor * cosf( g_gameTime + x * 1.5f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.y += windFlapFactor * 3.0f * sinf( g_gameTime + x * 1.1f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.z += windFlapFactor * cosf( g_gameTime + x * 1.3f ) * (float) x / (float) FLAG_RESOLUTION;

            m_flag[x][z] = m_flag[x][z] * factor2 + targetPos * factor1;


            if( x > 0 )
            {
                float distance = (m_flag[x-1][z] - m_flag[x][z]).Mag();
                float maxSize = m_size / 2.0f;
                if( distance > maxSize )
                {
                    Vector3 theVector = m_flag[x-1][z] - m_flag[x][z];
                    theVector.SetLength(maxSize);
                    m_flag[x][z] = m_flag[x-1][z] - theVector;
				}
            }
        }
    }


    //
    // Calculate our centre and radius values

    m_centre.Zero();
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION; ++y )
        {   
            m_centre += m_flag[x][y];
        }
    }
    m_centre /=  pow(FLAG_RESOLUTION, 2.0f);
	
    m_radius = 0.0f;
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION; ++y )
        {   
            float distToCentre = ( m_flag[x][y] - m_centre ).Mag();
            if( distToCentre > m_radius ) m_radius = distToCentre;
        }
    }
    

    //
    // Render the flag pole

    glColor4ub( 255, 255, 255, 255 );
    
    glDisable       ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );

    Vector3 right = m_up ^ ( g_app->m_camera->GetFront() );
    right.SetLength( 0.2f );

    glBegin( GL_QUADS );
        glVertex3dv( (m_pos-right).GetData() );
        glVertex3dv( (m_pos+right).GetData() );
        glVertex3dv( (m_pos+m_up*m_size*1.5f+right).GetData() );
        glVertex3dv( (m_pos+m_up*m_size*1.5f-right).GetData() );
    glEnd();

    //
    // Render the flag
        
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_BLEND );
    glEnable        ( GL_TEXTURE_2D );
    
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    
    if( _fileName && g_app->Multiplayer() )
    {
        Team *team = NULL;
        int colourId = -1;
        int groupId = -1;
        char fileName[256];
        RGBAColour teamColour;

        if( _teamId != -1 && _teamId != 255 )
        {
            team = g_app->m_location->m_teams[_teamId];
            colourId = team->m_lobbyTeam->m_colourId;
            groupId = team->m_lobbyTeam->m_coopGroupPosition;
        }

		if( !team )
        {
            sprintf( fileName, "%s_255", _fileName );
            teamColour.Set(128,128,128,255);
        }
        else if( m_isSelected || m_isHighlighted )
		{
			sprintf( fileName, "%s_selected_%d_%d", _fileName, colourId, groupId );
            teamColour = team->m_colour;

		}
        else if( _teamId >= 0 && _teamId < NUM_TEAMS )
        {
            sprintf( fileName, "%s_%d_%d", _fileName, colourId, groupId );
            teamColour = team->m_colour / 2.0f;
        }
        else
        {
            sprintf( fileName, _fileName );
        }

        if( !g_app->m_resource->DoesTextureExist( fileName ) )
        {
            BinaryReader *binReader = g_app->m_resource->GetBinaryReader( _fileName );
            BitmapRGBA bmp( binReader, "bmp" );
            SAFE_DELETE(binReader);
            bmp.ConvertRedChannel( teamColour );
            g_app->m_resource->AddBitmap( fileName, bmp );
        }

        m_texId = g_app->m_resource->GetTexture( fileName );
    }
    else
    {
        m_texId = g_app->m_resource->GetTexture( _fileName );
    }

    glBindTexture   ( GL_TEXTURE_2D, m_texId );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );


    glBegin( GL_QUADS );

    for( int x = 0; x < FLAG_RESOLUTION-1; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION-1; ++y )
        {   
            float texX = (float) x / (float) (FLAG_RESOLUTION-1);
            float texY = (float) y / (float) (FLAG_RESOLUTION-1);
            float texW = 1.0f / (FLAG_RESOLUTION-1);
            
            glTexCoord2f(texX,texY);            glVertex3dv( m_flag[x][y].GetData() );
            glTexCoord2f(texX+texW,texY);       glVertex3dv( m_flag[x+1][y].GetData() );
            glTexCoord2f(texX+texW,texY+texW);  glVertex3dv( m_flag[x+1][y+1].GetData() );
            glTexCoord2f(texX,texY+texW);       glVertex3dv( m_flag[x][y+1].GetData() );
        }
    }

    glEnd();
       

    //
    // If we're a goto order, render our arrow now

    if( m_target != g_zeroVector )
    {
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/arrow.bmp" ) );
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

        glEnable ( GL_POLYGON_OFFSET_FILL );
        glPolygonOffset( -1, -5 );


        float fromX, fromY, toX, toY;    
        bool fromOnscreen = g_app->m_markerSystem->CalculateScreenPosition( m_pos + g_upVector * 20, fromX, fromY );
        bool toOnscreen = g_app->m_markerSystem->CalculateScreenPosition( m_target, toX, toY );

        if( !fromOnscreen ) fromY = g_app->m_renderer->ScreenH() - fromY;
        if( !toOnscreen ) toY = g_app->m_renderer->ScreenH() - toY;
                
        //
        // Calculate the 2d angle between from and to

        g_app->m_renderer->SetupMatricesFor2D();

        Vector2 screenVector( fromX - toX, fromY - toY );
        float rotationAngle = atanf( screenVector.x / -screenVector.y );
        if( screenVector.y < 0 ) rotationAngle = M_PI + rotationAngle;  
        if( m_viewFlipped ) rotationAngle = 2.0f * M_PI - rotationAngle;

        g_app->m_renderer->SetupMatricesFor3D();
                
        //
        // Render the arrow at the right angle

        Vector3 texW( 1.0f / (FLAG_RESOLUTION-1), 0, 0 );
        Vector3 texH( 0, 0, 1.0f / (FLAG_RESOLUTION-1) );
        texW.RotateAroundY( rotationAngle );
        texH.RotateAroundY( rotationAngle );
            
        Vector2 texW2d( texW );
        Vector2 texH2d( texH );

        glBegin( GL_QUADS );

        for( int x = 0; x < FLAG_RESOLUTION-1; ++x )
        {
            for( int y = 0; y < FLAG_RESOLUTION-1; ++y )
            {   
                Vector3 texPos( (float) x / (float) (FLAG_RESOLUTION-1), 
                                 0, 
                                (float) y / (float) (FLAG_RESOLUTION-1) );
                texPos -= Vector3( 0.5f, 0, 0.5f );
                texPos.RotateAroundY( rotationAngle );
                texPos += Vector3( 0.5f, 0, 0.5f );
                Vector2 texPos2d( texPos );
                
                glTexCoord2dv((texPos2d).GetData());                    glVertex3dv( m_flag[x][y].GetData() );
                glTexCoord2dv((texPos2d + texW2d).GetData());           glVertex3dv( m_flag[x+1][y].GetData() );
                glTexCoord2dv((texPos2d + texW2d + texH2d).GetData());  glVertex3dv( m_flag[x+1][y+1].GetData() );
                glTexCoord2dv((texPos2d + texH2d).GetData());           glVertex3dv( m_flag[x][y+1].GetData() );
            }
        }

        glEnd();
        glDisable ( GL_POLYGON_OFFSET_FILL );
    }

    glEnable        ( GL_DEPTH_TEST );
    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

    END_PROFILE(  "RenderFlag" );
}


void Flag::RenderText( int _posX, int _posY, char *_caption )
{
    if( _posX < 0 || _posY < 0 ||
        _posX >= FLAG_RESOLUTION ||
        _posY >= FLAG_RESOLUTION )
    {
        return;
    }
    
    Vector3 pos = m_flag[_posX][_posY];
    Vector3 up = (m_flag[_posX][_posY+1] - pos);
    Vector3 right = (m_flag[_posX+1][_posY] - pos);
    
    pos += right * 0.5f;
    pos += up * 0.5f;    

    Vector3 front = (right ^ up).Normalise();
    up = (front ^ right).Normalise();
    
    g_gameFont.DrawText3D( pos + front * 0.1f, front, up, 4.0f, _caption );
    g_gameFont.DrawText3D( pos - front * 0.1f, -front, up, 4.0f, _caption );
}


bool Flag::RayHit( Vector3 const &rayStart, Vector3 const &rayDir )
{
    //
    // Fast test via sphere hit check
  
    if( !RaySphereIntersection( rayStart, rayDir, m_centre, m_radius ) )
    {
        return false;
    }


    //
    // Detailed intersect check

    for( int x = 0; x < FLAG_RESOLUTION-1; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION-1; ++y )
        {   
            Vector3 p1 = m_flag[x][y];
            Vector3 p2 = m_flag[x+1][y];
            Vector3 p3 = m_flag[x+1][y+1];
            Vector3 p4 = m_flag[x][y+1];
            
            if( RayTriIntersection( rayStart, rayDir, p1, p2, p3 ) )
            {
                return true;
            }

            if( RayTriIntersection( rayStart, rayDir, p3, p4, p1 ) )
            {
                return true;
            }           
        }
    }


    return false;
}

