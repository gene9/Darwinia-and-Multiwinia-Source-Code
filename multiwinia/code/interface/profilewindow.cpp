#include "lib/universal_include.h"


#include <stdio.h>
#include <string.h>

#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"
#include "lib/math/math_utils.h"

#include "network/server.h"
#include "network/clienttoserver.h"

#include "app.h"

#include "interface/profilewindow.h"



// ****************************************************************************
// Class ProfilerButton
// ****************************************************************************

class ProfilerButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
		if (m_caption.StrCmp(L"Toggle glFinish", false) == 0)
		{
			g_profiler->m_doGlFinish = !g_profiler->m_doGlFinish;
		}
		else if (m_caption.StrCmp(L"Reset History", false) == 0)
		{
			g_profiler->ResetHistory();
		}
		else if (m_caption.StrCmp(L"Min", false) == 0)
		{
			m_caption = UnicodeString("Avg");
		}
		else if (m_caption.StrCmp(L"Avg", false) == 0)
		{
			m_caption = UnicodeString("Max");
		}
		else if (m_caption.StrCmp(L"Max", false) == 0)
		{
			m_caption = UnicodeString("Min");
		}
    }
};



// ****************************************************************************
// Class ProfileWindow
// ****************************************************************************

ProfileWindow::ProfileWindow( char *name )
:   DarwiniaWindow( name ),
	m_totalPerSecond(true)
{
    Profiler::Start();
}


ProfileWindow::~ProfileWindow()
{
    Profiler::Stop();
}


void ProfileWindow::RenderElementProfile(ProfiledElement *_pe, unsigned int _indent)
{
	if (_pe->m_children.NumUsed() == 0) return;

    int left = m_x + 10;
    char caption[256];
	EclButton *minAvgMaxButton = GetButton("Avg");
	int minAvgMax = 0;
	if (minAvgMaxButton->m_caption.StrCmp(UnicodeString(L"Avg"), false) == 0)
	{
		minAvgMax = 1;
	}
	else if (minAvgMaxButton->m_caption.StrCmp(L"Max", false) == 0)
	{
		minAvgMax = 2;
	}


	float largestTime = 1000.0 * _pe->GetMaxChildTime();
	float totalTime = 0.0;

	short i = _pe->m_children.StartOrderedWalk();
	while (i != -1)
    {
        ProfiledElement *child = _pe->m_children[i];

        float time = float( child->m_lastTotalTime * 1000.0 );
		float avrgTime = 1000.0 * child->m_historyTotalTime / child->m_historyNumCalls;
		if (avrgTime > 0.0)
		{
			totalTime += time;

			char icon[] = " ";
			if (child->m_children.NumUsed() > 0) 
			{
				icon[0] = child->m_isExpanded ? '-' : '+';
			}

			float lastColumn;
			if (minAvgMax == 0)			lastColumn = child->m_shortest;
			else if (minAvgMax == 1)	lastColumn = avrgTime / 1000.0;
			else if (minAvgMax == 2)	lastColumn = child->m_longest;
			lastColumn *= 1000.0;

			sprintf(caption, 
					"%*s%-*s:%5d x%4.1 = %4.0", 
					_indent + 1,
					icon,
					24 - _indent,
					child->m_name, 
					child->m_lastNumCalls, 
					time/(float)child->m_lastNumCalls,
					time);
			int brightness = (time / largestTime) * 150.0 + 105.0;
			if (brightness < 105) brightness = 105;
			else if (brightness > 255) brightness = 255;
			glColor3ub( brightness, brightness, brightness );

			// Deal with mouse clicks to expand or unexpand a node
			if ( g_inputManager->controlEvent( ControlEclipseLMousePressed ) ) // g_inputManager->GetRawLmbClicked()
			{
				int x = g_target->X();
				int y = g_target->Y();
				if (x > m_x && x < m_x+m_w &&
					y > m_yPos + 5 && y < m_yPos + 17)
				{
					AppReleaseAssert(child != g_profiler->m_rootElement,
						"ProfileWindow::RenderElementProfile child==root");
					child->m_isExpanded = !child->m_isExpanded;
				}
			}

			g_editorFont.DrawText2D( left, m_yPos+=12, DEF_FONT_SIZE, caption );

            int lineLeft = left + 340;
            int lineY = m_yPos - 6;
            int lineWidth = iv_sqrt(time) * 10.0;
            int lineHeight = 11.0;
            glColor4ub( 150, 150, 250, brightness );

            if(child->m_isExpanded)
            {
                glBegin( GL_LINES );
                    glVertex2i( lineLeft, lineY);
                    glVertex2i( lineLeft + lineWidth, lineY);

                    glVertex2i( lineLeft + lineWidth, lineY);
                    glVertex2i( lineLeft + lineWidth, lineY + lineHeight );

                    glVertex2i( lineLeft, lineY);
                    glVertex2i( lineLeft, lineY + lineHeight );
                glEnd();                
            }
            else
            {
                glBegin( GL_QUADS );
                    glVertex2i( lineLeft, lineY);
                    glVertex2i( lineLeft + lineWidth, lineY);
                    glVertex2i( lineLeft + lineWidth, lineY + lineHeight );
                    glVertex2i( lineLeft, lineY + lineHeight );
                glEnd();
            }

			if (m_yPos > m_h)
			{
				m_h += 12;
			}

			if (child->m_isExpanded && child->m_children.NumUsed() > 0)
			{
				RenderElementProfile(child, _indent + 2);
			}
		}

		i = _pe->m_children.GetNextOrderedIndex();
    }

	//glColor3ub(255,255,255);
	//g_editorFont.DrawText2D( left + (_indent+1) * 7.5, m_yPos+=12, DEF_FONT_SIZE, "Total %.0f", totalTime );     
}


void ProfileWindow::RenderFrameTimes( float x, float y, float w, float h )
{
    if( g_profiler )
    {
        glColor4ub( 255,255,255,100 );

        glBegin( GL_LINES );
            glVertex2f( x, y );
            glVertex2f( x + w, y );

            glVertex2f( x, y + h/2.0 );
            glVertex2f( x + w, y + h/2.0 );

            glVertex2f( x, y + h * 3/4.0 );
            glVertex2f( x + w, y + h * 3/4.0 );
        glEnd();
        
        glColor4ub( 255,255,255,255 );

        g_editorFont.DrawText2D( x + 10, y-2, 10, "200ms per frame (5 fps)" );
        g_editorFont.DrawText2D( x + 10, y+h/2.0 - 4, 10, "100ms per frame (10 fps)" );
        g_editorFont.DrawText2D( x + 10, y+h*3/4.0 - 4, 10, "50ms per frame (20 fps)" );


        glColor4f( 1.0, 0.0, 0.0, 0.8 );

        float xPos = x + w;
        float wPerFrame = w / 200;

        glBegin( GL_LINES );

        for( int i = 0; i < g_profiler->m_frameTimes.Size(); ++i )
        {
            int thisFrameTime = g_profiler->m_frameTimes[i];

            float frameHeight = 0.5 * (thisFrameTime / 100.0) * h;

            glVertex2f( xPos, y + h );
            glVertex2f( xPos, y + h - frameHeight );

            xPos -= wPerFrame;
        }

        glEnd();
    }
}


void ProfileWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

    if (g_profiler->m_doGlFinish)
	{
		g_editorFont.DrawText2D( m_x + 130, m_y + 28, DEF_FONT_SIZE, "Yes" );
	}
	else
	{
		g_editorFont.DrawText2D( m_x + 130, m_y + 28, DEF_FONT_SIZE, "No" );
	}

    ProfiledElement *root = g_profiler->m_rootElement;
    int tableSize = root->m_children.Size();

    m_yPos = m_y + 42;

    g_editorFont.DrawText2DRight( m_x + 330, m_yPos, DEF_FONT_SIZE * 0.85, "calls x avrg = total" );

    START_PROFILE("render profile");
	RenderElementProfile(root, 0);
    END_PROFILE("render profile");

    RenderFrameTimes( m_x, m_y + m_h - 101, m_w, 100 );
}


void ProfileWindow::Create()
{
	DarwiniaWindow::Create();

	ProfilerButton *but = new ProfilerButton();
	but->SetShortProperties("Toggle glFinish", 10, 18, -1, -1, UnicodeString("Toggle glFinish"));
	RegisterButton(but);

	ProfilerButton *minAvgMax = new ProfilerButton();
	minAvgMax->SetShortProperties("Avg", 330, 18, -1, -1, UnicodeString("Avg"));
	RegisterButton(minAvgMax);

	ProfilerButton *resetHistBut = new ProfilerButton();
	resetHistBut->SetShortProperties("Reset History", 190, 18, -1, -1, UnicodeString("Reset History"));
	RegisterButton(resetHistBut);

	g_profiler->m_doGlFinish = true;
}


void ProfileWindow::Remove()
{
    DarwiniaWindow::Remove();
    
    RemoveButton( "Toggle glFinish" );
    RemoveButton( "Min" );
    RemoveButton( "Reset History" );
}

