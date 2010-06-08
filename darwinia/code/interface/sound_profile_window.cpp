#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/text_renderer.h"

#include "interface/scrollbar.h"
#include "interface/sound_profile_window.h"

#include "sound/sound_library_3d.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "renderer.h"


#ifdef SOUND_EDITOR


//*****************************************************************************
// Class ResetHistoryButton
//*****************************************************************************

class Profiler;

class ResetHistoryButton: public DarwiniaButton
{
protected:
	Profiler *m_profiler;

public:
	ResetHistoryButton(Profiler *_profiler): m_profiler(_profiler) {}

    void MouseUp()
    {
#ifdef PROFILER_ENABLED
        m_profiler->ResetHistory();
		g_soundLibrary3d->m_profiler->ResetHistory();
#endif
    }
};


//*****************************************************************************
// Class SoundProfileWindow
//*****************************************************************************

SoundProfileWindow::SoundProfileWindow( char *_name )
:   DarwiniaWindow( _name )
{
    m_w = 420;
    m_h = 550;
    m_x = g_app->m_renderer->ScreenW() - m_w - 20;
    m_y = 30;
}


SoundProfileWindow::~SoundProfileWindow()
{
}


void SoundProfileWindow::Create()
{
    DarwiniaWindow::Create();

	ResetHistoryButton *mainResetButton = new ResetHistoryButton(
									g_app->m_soundSystem->m_mainProfiler);
	mainResetButton->SetShortProperties("Reset", 10, 27);
	strcpy(mainResetButton->m_name, "Main Reset");
	RegisterButton(mainResetButton);

	ResetHistoryButton *eventResetButton = new ResetHistoryButton(
									g_app->m_soundSystem->m_eventProfiler);
	eventResetButton->SetShortProperties("Reset", 10, 283);
	strcpy(eventResetButton->m_name, "Event Reset");
	RegisterButton(eventResetButton);
}


void SoundProfileWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );
#ifdef PROFILER_ENABLED
    int xPos = m_x + 15;
    int yPos = m_y + 40;
    int yDif = 13;
    int textSize = DEF_FONT_SIZE;
      
	//
    // Print Main Profiles

	float total = 0.0f;

	{
		Profiler *profiler = g_app->m_soundSystem->m_mainProfiler;
		float maxTime = profiler->m_rootElement->GetMaxChildTime() * 1000.0f;

		int i = profiler->m_rootElement->m_children.StartOrderedWalk();
		while (i != -1)
		{
			ProfiledElement *pe = profiler->m_rootElement->m_children[i];
			float time = float( pe->m_lastTotalTime * 1000.0f );
			float timePerCall = 0.0f;
			if( pe->m_lastNumCalls > 0 )
			{
				timePerCall = time/(float)pe->m_lastNumCalls;
			}
			char caption[256];
			sprintf( caption, "%-28s:%3dx%2.1f = %2.0f  %2.0f", 
					 pe->m_name, 
					 pe->m_lastNumCalls, 
					 timePerCall,
					 time,
 					 pe->m_historyTotalTime * 1000.0 / pe->m_historyNumSeconds);
        
			float colour = 0.5f + 0.5f * time / maxTime;
			glColor4f( colour, colour, colour, 1.0f );

			g_editorFont.DrawText2D( xPos, yPos+=yDif, textSize, caption );

			total += time;

			i = profiler->m_rootElement->m_children.GetNextOrderedIndex();
		}

		ProfiledElement *pe = g_app->m_profiler->m_rootElement->m_children.GetData("Advance SoundSystem");
		float advanceTime = pe->m_lastTotalTime * 1000.0f;
		float other = advanceTime - total;
		if( other < 0 ) other = 0;
		float colour = 0.5f + 0.5f * other / maxTime;
		glColor4f( colour, colour, colour, 1.0f );
		g_editorFont.DrawText2D( xPos, yPos+=yDif, textSize, "Other         :        = %2.0f", other );
		total += other;
	}
																			
    
	yPos = m_y + 300;
   

	//
    // Print Event Profiles

	{
		Profiler *profiler = g_app->m_soundSystem->m_eventProfiler;
		float maxTime = profiler->m_rootElement->GetMaxChildTime() * 1000.0f;
		float total = 0.0f;
		if (maxTime < 1e-6) maxTime = 1e-6;

		int i = profiler->m_rootElement->m_children.StartOrderedWalk();
		while (i != -1)
		{
			ProfiledElement *pe = profiler->m_rootElement->m_children[i];
			float time = float( pe->m_lastTotalTime * 1000.0f );
			float timePerCall = 0.0f;
			if( pe->m_lastNumCalls > 0 )
			{
				timePerCall = time/(float)pe->m_lastNumCalls;
			}
			char caption[256];
			sprintf( caption, "%-28s:%3dx%2.1f = %2.0f  %2.0f",
					 pe->m_name, 
					 pe->m_lastNumCalls, 
					 timePerCall,
					 time,
					 pe->m_historyTotalTime * 1000.0 / pe->m_historyNumSeconds);
        
			float colour = 0.5f + 0.5f * time / maxTime;
			glColor4f( colour, colour, colour, 1.0f );

			g_editorFont.DrawText2D( xPos, yPos+=yDif, textSize, caption );

			total += time;

			i = profiler->m_rootElement->m_children.GetNextOrderedIndex();
		}
	}


	//
	// Print total						
	
	yPos += yDif;
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	g_editorFont.DrawText2D( xPos, m_y + m_h - 20, 15, "Total : %2.2fms", total );


	//
	// Print SoundLibrary3d profile

	{
		Profiler *profiler = g_soundLibrary3d->m_profiler;
		profiler->Advance();
		float maxTime = profiler->m_rootElement->GetMaxChildTime() * 1000.0f;
		float total = 0.0f;
		if (maxTime < 1e-6) maxTime = 1e-6;

		int i = profiler->m_rootElement->m_children.StartOrderedWalk();
		while (i != -1)
		{
			ProfiledElement *pe = profiler->m_rootElement->m_children[i];
			float time = float( pe->m_lastTotalTime * 1000.0f );
			float timePerCall = 0.0f;
			if( pe->m_lastNumCalls > 0 )
			{
				timePerCall = time/(float)pe->m_lastNumCalls;
			}
			char caption[256];
			sprintf( caption, "%-28s:%3dx%2.1f = %2.0f  %2.0f",
					 pe->m_name, 
					 pe->m_lastNumCalls, 
					 timePerCall,
					 time,
					 pe->m_historyTotalTime * 1000.0 / pe->m_historyNumSeconds);
        
			float colour = 0.5f + 0.5f * time / maxTime;
			glColor4f( colour, colour, colour, 1.0f );

			g_editorFont.DrawText2D( xPos, yPos+=yDif, textSize, caption );

			total += time;

			i = profiler->m_rootElement->m_children.GetNextOrderedIndex();
		}
	}
#endif
}

#endif // SOUND_EDITOR
