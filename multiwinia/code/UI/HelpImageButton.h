#ifndef __HELPIMAGEBUTTON__
#define __HELPIMAGEBUTTON__

class HelpImageButton : public DarwiniaButton
{
public:

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *main = (GameMenuWindow *)m_parent;

        char imageName[512];
        sprintf( imageName, "mwhelp/helppage_%d_%d." TEXTURE_EXTENSION, main->m_helpPage, main->m_currentHelpImage );

        if( g_app->m_resource->DoesTextureExist( imageName ) )
        {        
            int texId = g_app->m_resource->GetTexture( imageName, false );

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, texId );

            glBegin( GL_QUADS );
                glTexCoord2i(0,1);      glVertex2f( realX, realY );
                glTexCoord2i(1,1);      glVertex2f( realX + m_w, realY );
                glTexCoord2i(1,0);      glVertex2f( realX + m_w, realY + m_h );
                glTexCoord2i(0,0);      glVertex2f( realX, realY + m_h );
            glEnd();

            glDisable( GL_TEXTURE_2D );
        }     

        glBegin( GL_LINE_LOOP );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();

    }
};

#endif
