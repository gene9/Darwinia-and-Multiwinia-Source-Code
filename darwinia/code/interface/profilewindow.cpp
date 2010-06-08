#include "lib/universal_include.h"


#include <stdio.h>
#include <string.h>

#include "lib/input/input.h"
#include "lib/targetcursor.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"

#include "network/server.h"
#include "network/clienttoserver.h"

#include "app.h"

#include "interface/profilewindow.h"


#ifdef PROFILER_ENABLED


// ****************************************************************************
// Class ProfilerButton
// ****************************************************************************

class ProfilerButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
		if (stricmp(m_caption, "Toggle glFinish") == 0)
		{
			g_app->m_profiler->m_doGlFinish = !g_app->m_profiler->m_doGlFinish;
		}
		else if (stricmp(m_caption, "Reset History") == 0)
		{
			g_app->m_profiler->ResetHistory();
		}
		else if (stricmp(m_caption, "Min") == 0)
		{
			strcpy(m_caption, "Avg");
		}
		else if (stricmp(m_caption, "Avg") == 0)
		{
			strcpy(m_caption, "Max");
		}
		else if (stricmp(m_caption, "Max") == 0)
		{
			strcpy(m_caption, "Min");
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
}


ProfileWindow::~ProfileWindow()
{
	g_app->m_profiler->m_doGlFinish = false;
}


void ProfileWindow::RenderElementProfile(ProfiledElement *_pe, unsigned int _indent)
{
	if (_pe->m_children.NumUsed() == 0) return;

    int left = m_x + 10;
    char caption[256];
	EclButton *minAvgMaxButton = GetButton("Avg");
	int minAvgMax = 0;
	if (stricmp(minAvgMaxButton->m_caption, "Avg") == 0)
	{
		minAvgMax = 1;
	}
	else if (stricmp(minAvgMaxButton->m_caption, "Max") == 0)
	{
		minAvgMax = 2;
	}


	float largestTime = 1000.0f * _pe->GetMaxChildTime();
	float totalTime = 0.0f;

	short i = _pe->m_children.StartOrderedWalk();
	while (i != -1)
    {
        ProfiledElement *child = _pe->m_children[i];

        float time = float( child->m_lastTotalTime * 1000.0f );
		float avrgTime = 1000.0f * child->m_historyTotalTime / child->m_historyNumCalls;
		if (avrgTime > 0.0f)
		{
			totalTime += time;

			char icon[] = " ";
			if (child->m_children.NumUsed() > 0) 
			{
				icon[0] = child->m_isExpanded ? '-' : '+';
			}

			float lastColumn;
			if (minAvgMax == 0)			lastColumn = child->m_shortest;
			else if (minAvgMax == 1)	lastColumn = avrgTime / 1000.0f;
			else if (minAvgMax == 2)	lastColumn = child->m_longest;
			lastColumn *= 1000.0f;

			sprintf(caption, 
					"%*s%-*s:%5d x%4.2f = %4.0f %4.2f", 
					_indent + 1,
					icon,
					24 - _indent,
					child->m_name, 
					child->m_lastNumCalls, 
					time/(float)child->m_lastNumCalls,
					time,
					lastColumn);
			int brightness = (time / largestTime) * 150.0f + 105.0f;
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
					DarwiniaReleaseAssert(child != g_app->m_profiler->m_rootElement,
						"ProfileWindow::RenderElementProfile child==root");
					child->m_isExpanded = !child->m_isExpanded;
				}
			}

			g_editorFont.DrawText2D( left, m_yPos+=12, DEF_FONT_SIZE, caption );

            int lineLeft = left + 360;
            int lineY = m_yPos - 6;
            int lineWidth = sqrtf(time) * 10.0f;
            int lineHeight = 11.0f;
            glColor4ub( 150, 150, 250, brightness );
            glBegin( GL_QUADS );
                glVertex2i( lineLeft, lineY);
                glVertex2i( lineLeft + lineWidth, lineY);
                glVertex2i( lineLeft + lineWidth, lineY + lineHeight );
                glVertex2i( lineLeft, lineY + lineHeight );
            glEnd();

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

	glColor3ub(255,255,255);
	g_editorFont.DrawText2D( left + (_indent+1) * 7.5f, m_yPos+=12, DEF_FONT_SIZE, "Total %.0f", totalTime );
}


void ProfileWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

    if (g_app->m_profiler->m_doGlFinish)
	{
		g_editorFont.DrawText2D( m_x + 130, m_y + 28, DEF_FONT_SIZE, "Yes" );
	}
	else
	{
		g_editorFont.DrawText2D( m_x + 130, m_y + 28, DEF_FONT_SIZE, "No" );
	}

    ProfiledElement *root = g_app->m_profiler->m_rootElement;
    int tableSize = root->m_children.Size();

    m_yPos = m_y + 42;

    g_editorFont.DrawText2DRight( m_x + 330, m_yPos, DEF_FONT_SIZE * 0.85f, "calls x avrg = total" );

START_PROFILE(g_app->m_profiler, "render profile");
	RenderElementProfile(root, 0);
END_PROFILE(g_app->m_profiler, "render profile");
}


void ProfileWindow::Create()
{
	DarwiniaWindow::Create();

	ProfilerButton *but = new ProfilerButton();
	but->SetShortProperties("Toggle glFinish", 10, 18);
	RegisterButton(but);

	ProfilerButton *minAvgMax = new ProfilerButton();
	minAvgMax->SetShortProperties("Avg", 330, 18);
	RegisterButton(minAvgMax);

	ProfilerButton *resetHistBut = new ProfilerButton();
	resetHistBut->SetShortProperties("Reset History", 190, 18);
	RegisterButton(resetHistBut);

	g_app->m_profiler->m_doGlFinish = true;
}


void ProfileWindow::Remove()
{
    DarwiniaWindow::Remove();
    
    RemoveButton( "Toggle glFinish" );
    RemoveButton( "Min" );
    RemoveButton( "Reset History" );
}


#endif // PROFILER_ENABLED
