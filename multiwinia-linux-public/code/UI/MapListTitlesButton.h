#ifndef __MAPLISTITLESBUTTON__
#define __MAPLISTITLESBUTTON__

#include "LevelSelectButtonBase.h"

class MapListTitlesButton : public DarwiniaButton
{
public:
    int m_screenId;

public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        //RenderHighlightBeam( realX, realY + m_h/2, m_w, m_h );

        glColor4f( 0.0f, 0.0f, 0.0f, 0.5f );
        glBegin( GL_QUADS );
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
        

        float fontY = realY + m_h/2.0f - m_fontSize/2.0f + 7.0f;

        for( int i = 0; i < 3; ++i )
        {
            if( i == 0 )
            {
                g_titleFont.SetRenderOutline(true);
                glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            }
            else
            {
                g_titleFont.SetRenderOutline(false);
                //g_titleFont.SetRenderShadow(true);
                glColor4f( 0.6f, 0.6f, 0.6f, 1.0f );
            }

            float nameX, timeX, playersX, coopX, diffX;
            LevelSelectButtonBase::GetColumnPositions( realX, m_w, nameX, timeX, playersX, coopX, diffX );

            if( m_screenId == GameMenuWindow::PageMapSelect )
            {
                g_titleFont.DrawText2D  ( nameX, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_mapname") );
                g_titleFont.DrawText2D  ( timeX, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_playtime") );
                g_titleFont.DrawText2D  ( playersX, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_players") );
                //g_titleFont.DrawText2D  ( coopX, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_coop") );
            }

            if( m_screenId == GameMenuWindow::PageGameSelect )
            {
                g_titleFont.DrawText2D( realX + m_w * 0.05f, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_gamemode") );
            }

            //g_titleFont.DrawText2D( diffX, fontY, m_fontSize, LANGUAGEPHRASE("multiwinia_difficulty") );
        }

        g_titleFont.SetRenderOutline(false);
        g_titleFont.SetRenderShadow(false);
    }
};

#endif