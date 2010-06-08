#include "lib/universal_include.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"

#include "sound/soundsystem.h"

#include "interface/save_on_quit_window.h"

#include "app.h"
#include "renderer.h"


class YesButton : public DarwiniaButton
{
    void MouseUp()
    {
		g_app->m_soundSystem->SaveBlueprints();
		g_app->m_requestQuit = true;
    }
};


class NoButton : public DarwiniaButton
{
    void MouseUp()
    {
		g_app->m_requestQuit = true;
		g_app->m_soundSystem->m_quitWithoutSave = true;
    }
};


class CancelButton : public DarwiniaButton
{
    void MouseUp()
    {
		EclRemoveWindow(LANGUAGEPHRASE("editor_savesettings"));
    }
};


SaveOnQuitWindow::SaveOnQuitWindow( char *_name )
:   DarwiniaWindow( _name )
{
	m_w = 200;
	m_h = 100;
	m_x = g_app->m_renderer->ScreenW()/2 - m_w/2;
	m_y = g_app->m_renderer->ScreenH()/2 - m_h/2;
}


void SaveOnQuitWindow::Create()
{
	DarwiniaWindow::Create();

    DarwiniaButton *button;
	int width = 55;
	int pitch = width + 8;
	int x = 1 - width;
	int y = m_h - 25;
	
	button = new YesButton();
    button->SetShortProperties( "Yes", x += pitch, y, width);
    RegisterButton( button );

	button = new NoButton();
    button->SetShortProperties( "No", x += pitch, y, width );
    RegisterButton( button );

	button = new CancelButton();
    button->SetShortProperties( "Cancel", x += pitch, y, width );
    RegisterButton( button );
}


void SaveOnQuitWindow::Render(bool _hasFocus)
{
	DarwiniaWindow::Render(_hasFocus);
	g_editorFont.DrawText2D(m_x + 15, m_y + 30, DEF_FONT_SIZE * 4, "!");
	g_editorFont.DrawText2D(m_x + 55, m_y + 38, DEF_FONT_SIZE, "Save changes to");
	g_editorFont.DrawText2D(m_x + 55, m_y + 52, DEF_FONT_SIZE, "sounds.txt?");
}
