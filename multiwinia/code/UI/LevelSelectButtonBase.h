#ifndef __LEVELSELECTBUTTONBASE__
#define __LEVELSELECTBUTTONBASE__

#include "GameMenuWindow.h"

class LevelSelectButtonBase : public DarwiniaButton
{
public:
    static void GetColumnPositions( float buttonX, float buttonW, 
                                    float &nameX, float &timeX, float &playersX, float &coopX, float &difficultyX )
    {
        nameX = buttonX + buttonW * 0.025f;
        timeX = buttonX + buttonW * 0.65f;
        playersX = buttonX + buttonW * 0.83f;

        coopX = playersX + buttonW * 0.1f;
        difficultyX = coopX + buttonW * 0.1f;        
    }

protected:	
    double  m_birthTime;

    LevelSelectButtonBase()
    {
        m_birthTime = GetHighResTime();
    }


	void RenderBackground( int realX, int realY, bool &highlighted, bool &clicked )
	{
		GameMenuWindow *main = (GameMenuWindow *) m_parent;

		if( highlighted && g_inputManager->getInputMode() != INPUT_MODE_KEYBOARD )
        {
            // ignore any mouse inputs if we are using the gamepad
            highlighted = false;
        }

        UpdateButtonHighlight();

        if( main->m_buttonOrder.ValidIndex(main->m_currentButton) &&
            main->m_buttonOrder[main->m_currentButton] == this )
        {
            highlighted = true;
        }

        if( highlighted )
        {
            glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
        }
        else
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
        }

        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glBegin( GL_QUADS);
            glVertex2f( realX, realY );
            glVertex2f( realX + m_w, realY );
            glVertex2f( realX + m_w, realY + m_h );
            glVertex2f( realX, realY + m_h );
        glEnd();
        
	}

	void RenderMapName( UnicodeString levelName, int realX, int realY, bool highlighted, bool clicked, bool demoRestricted )
	{
		GameMenuWindow *main = (GameMenuWindow *) m_parent;

        float realXf = (float)realX;
        double timeNow = GetHighResTime();
        double showTime = m_birthTime + 1.0f * (float)realY / (float)g_app->m_renderer->ScreenH();

        if( timeNow < showTime )
        {
            realXf -= ( showTime - timeNow ) * g_app->m_renderer->ScreenW() * 0.1f;            

            glColor4f( 1.0f, 1.0f, 1.0f, (showTime-timeNow) );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glBegin( GL_QUADS);
                glVertex2f( realXf, realY );
                glVertex2f( realXf + m_w, realY );
                glVertex2f( realXf + m_w, realY + m_h );
                glVertex2f( realXf, realY + m_h );
            glEnd();
        }

        float fontSize = m_fontSize;
        float fontY = realY + m_h/2.0f - fontSize/2.0f + 7.0f;

        float nameX, unused;
        GetColumnPositions( realXf, m_w, nameX, unused, unused, unused, unused );

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_editorFont.SetRenderOutline(true);        
        g_editorFont.DrawText2D( nameX, fontY, fontSize, levelName );

		if( highlighted )
        {
            glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
        } 
        else
        {
            glColor4f( 0.65f, 1.0f, 0.4f, 1.0f );
        }
		if( demoRestricted )
        {
            glColor4f( 0.3f, 0.3f, 0.3f, 1.0f );
        }
        
        g_editorFont.SetRenderOutline(false);
        g_editorFont.DrawText2D( nameX, fontY, fontSize, levelName );

	}
};

#endif