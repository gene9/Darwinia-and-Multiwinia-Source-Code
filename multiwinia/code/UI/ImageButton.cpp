#include "lib/universal_include.h"

#include "app.h"
#include "lib/resource.h"
#include "ImageButton.h"
#include "GameMenuWindow.h"

ImageButton::ImageButton( char *_filename )
:   DarwiniaButton(),
    m_filename( strdup(_filename) ),
    m_border(true)
{
}

void ImageButton::Render( int realX, int realY, bool highlighted, bool clicked )
{
    GameMenuWindow *main = (GameMenuWindow *)m_parent;

    if( g_app->m_resource->DoesTextureExist( m_filename ) )
    {        
        int texId = g_app->m_resource->GetTexture( m_filename, false );

        glEnable( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, texId );		
		glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
        glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

        glBegin( GL_QUADS );
            glTexCoord2i(0,1);      glVertex2f( realX, realY );
            glTexCoord2i(1,1);      glVertex2f( realX + m_w, realY );
            glTexCoord2i(1,0);      glVertex2f( realX + m_w, realY + m_h );
            glTexCoord2i(0,0);      glVertex2f( realX, realY + m_h );
        glEnd();

        glDisable( GL_TEXTURE_2D );
    }     

    if( m_border )
    {
        glBegin( GL_LINE_LOOP );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
    }

}