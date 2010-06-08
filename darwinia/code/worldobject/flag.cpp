#include "lib/universal_include.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"

#include "worldobject/flag.h"

#include "app.h"
#include "main.h"
#include "camera.h"


Flag::Flag()
{
}


void Flag::Initialise()
{
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            Vector3 targetPos = m_pos + 
                                (float) x / (float) FLAG_RESOLUTION * m_front +
                                (float) z / (float) FLAG_RESOLUTION * m_up;
            
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


void Flag::SetOrientation( Vector3 const &_front, Vector3 const &_up )
{
    m_front = _front;
    m_up = _up;
}


void Flag::SetSize( float _size )
{
    m_size = _size;
}


void Flag::Render()
{
    START_PROFILE( g_app->m_profiler, "RenderFlag" );

    //
    // Advance the flag
    
    for( int x = 0; x < FLAG_RESOLUTION; ++x )
    {
        for( int z = 0; z < FLAG_RESOLUTION; ++z )
        {
            float factor1 = g_advanceTime * 10 * (FLAG_RESOLUTION - x) / FLAG_RESOLUTION;
            if( x == 0 ) factor1 = 1.0f;
            float factor2 = 1.0f - factor1;
            Vector3 targetPos;
            
            if( x == 0 )
            {
                targetPos = m_pos + m_up * m_size / 2.0f + 
                                (float) x / (float) (FLAG_RESOLUTION-1) * m_front * m_size +
                                (float) z / (float) (FLAG_RESOLUTION-1) * m_up * m_size;
            }
            else
            {
                targetPos = m_flag[x-1][z] + 
                                1.0f/(float) (FLAG_RESOLUTION-1) * m_front * m_size;
            }
            
            targetPos.x += 1 * cosf( g_gameTime + x * 1.5f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.y += 3 * sinf( g_gameTime + x * 1.1f ) * (float) x / (float) FLAG_RESOLUTION;
            targetPos.z += 1 * cosf( g_gameTime + x * 1.3f ) * (float) x / (float) FLAG_RESOLUTION;

            m_flag[x][z] = m_flag[x][z] * factor2 + targetPos * factor1;

            if( x > 0 )
            {
                float distance = (m_flag[x-1][z] - m_flag[x][z]).Mag();
                if( distance > 10.0f )
                {
                    Vector3 theVector = m_flag[x-1][z] - m_flag[x][z];
                    theVector.SetLength(10.0f);
                    m_flag[x][z] = m_flag[x-1][z] - theVector;
                }
            }
        }
    }

    //
    // Render the flag pole

    glColor4ub( 255, 255, 100, 255 );
    
    glDisable       ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D );

    Vector3 right = m_up ^ ( g_app->m_camera->GetFront() );
    right.SetLength( 0.2f );

    glBegin( GL_QUADS );
        glVertex3fv( (m_pos-right).GetData() );
        glVertex3fv( (m_pos+right).GetData() );
        glVertex3fv( (m_pos+m_up*m_size*1.5f+right).GetData() );
        glVertex3fv( (m_pos+m_up*m_size*1.5f-right).GetData() );
    glEnd();

    //
    // Render the flag
        
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable        ( GL_BLEND );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, m_texId );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    
    glColor4f( 1.0f, 1.0f, 1.0f, 0.8f );    
    for( int x = 0; x < FLAG_RESOLUTION-1; ++x )
    {
        for( int y = 0; y < FLAG_RESOLUTION-1; ++y )
        {
            float texX = (float) x / (float) (FLAG_RESOLUTION-1);
            float texY = (float) y / (float) (FLAG_RESOLUTION-1);
            float texW = 1.0f / (FLAG_RESOLUTION-1);
            
            glBegin( GL_QUADS );
                glTexCoord2f(texX,texY);            glVertex3fv( m_flag[x][y].GetData() );
                glTexCoord2f(texX+texW,texY);       glVertex3fv( m_flag[x+1][y].GetData() );
                glTexCoord2f(texX+texW,texY+texW);  glVertex3fv( m_flag[x+1][y+1].GetData() );
                glTexCoord2f(texX,texY+texW);       glVertex3fv( m_flag[x][y+1].GetData() );
            glEnd();
        }
    }
    
    glDepthMask     ( true );
    glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

    END_PROFILE( g_app->m_profiler, "RenderFlag" );
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

