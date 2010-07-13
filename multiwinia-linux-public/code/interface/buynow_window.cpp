#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/window_manager.h"
#include "lib/resource.h"

#include "app.h"
#include "renderer.h"
#include "interface/buynow_window.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"

#include "lib/ambrosia.h"

class BuyNowButton : public DarwiniaButton
{
    void MouseUp()
    {
		EclRemoveWindow( m_parent->m_name );
    }
};


BuyNowWindow::BuyNowWindow()
	: DarwiniaWindow( "Registration" )
{
    int screenW = g_app->m_renderer->ScreenW();
    int screenH = g_app->m_renderer->ScreenH();
    
    SetSize( 370, 150 );
    SetPosition( screenW/2.0 - m_w/2.0,
                 screenH/2.0 - m_h/2.0 );
}

void BuyNowWindow::Create()
{
	DarwiniaWindow::Create();
	
	int y = m_h;
	int h = 30;
	
    DarwiniaButton *close = new CloseButton();
    close->SetShortProperties( "Later", 10, y-=h, m_w-20, 20, "", "" );
    close->m_fontSize = 13;
    close->m_centered = true;
    RegisterButton( close );
	
    DarwiniaButton *buy = new BuyNowButton();
    buy->SetShortProperties( "Register Now", 10, y-=h, m_w-20, 20, "", "" );
    buy->m_fontSize = 13;
    buy->m_centered = true;
    RegisterButton( buy );
}

void BuyNowWindow::Render(bool _hasFocus)
{
	DarwiniaWindow::Render(_hasFocus);
	
    glColor4f( 1.0, 1.0, 1.0, 1.0 );
	
	int y = m_y + 25;
	int h = 18;
	
	const char *line[] = {
		"We hope you've enjoyed playing Darwinia!",
		"To carry on playing please register the game.",
		NULL
	};

	for (int i = 0; line[i]; i++) 
		g_gameFont.DrawText2DCentre( m_x+m_w/2, y+=h, 13, line[i] );
	
}

