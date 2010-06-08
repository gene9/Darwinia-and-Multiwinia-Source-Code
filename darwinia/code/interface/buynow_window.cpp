#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "app.h"
#include "renderer.h"
#include "interface/buynow_window.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"


class BuyNowButton : public DarwiniaButton
{
    void MouseUp()
    {
		g_app->m_requestQuit = true;
		EclRemoveWindow( m_parent->m_name );
    }
};


BuyNowWindow::BuyNowWindow()
: DarwiniaWindow( LANGUAGEPHRASE("dialog_buydarwinia" ) )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();
    
    SetSize( 370, 150 );
    SetPosition( screenW/2.0f - m_w/2.0f,
                 screenH/2.0f - m_h/2.0f );
}

void BuyNowWindow::Create()
{
	DarwiniaWindow::Create();
	
	int y = m_h;
	int h = 30;
	
    DarwiniaButton *close = new CloseButton();
    close->SetShortProperties( LANGUAGEPHRASE("dialog_later"), 10, y-=h, m_w-20, 20 );
    close->m_fontSize = 13;
    close->m_centered = true;
    RegisterButton( close );
	
    DarwiniaButton *buy = new BuyNowButton();
    buy->SetShortProperties( LANGUAGEPHRASE("dialog_buynow"), 10, y-=h, m_w-20, 20 );
    buy->m_fontSize = 13;
    buy->m_centered = true;
    RegisterButton( buy );
}

void BuyNowWindow::Render(bool _hasFocus)
{
	DarwiniaWindow::Render(_hasFocus);
	
    glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
	
	int y = m_y + 25;
	int h = 18;
	
	const char *line[] = {
		LANGUAGEPHRASE("dialog_buynow1"),
		LANGUAGEPHRASE("dialog_buynow2"),		
		NULL
	};

	for (int i = 0; line[i]; i++) 
		g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, 13, line[i] );
	
}

