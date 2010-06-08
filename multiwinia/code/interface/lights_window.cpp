#include "lib/universal_include.h"

#include <stdio.h>

#include "lib/language_table.h"

#include "interface/input_field.h"
#include "interface/lights_window.h"

#include "app.h"
#include "location_editor.h"
#include "level_file.h"
#include "location.h"


#ifdef LOCATION_EDITOR


// ****************************************************************************
// Class LightButton
// ****************************************************************************

class LightButton: public DarwiniaButton
{
public:
	int m_lightNum;

	LightButton(int num): m_lightNum(num) {}
	
	void MouseUp()
	{
		if (m_lightNum == -1)
		{
			g_app->m_locationEditor->m_tool = LocationEditor::ToolNone;
		}
		else
		{
			g_app->m_locationEditor->m_tool = LocationEditor::ToolRotate;
		}
		g_app->m_locationEditor->m_selectionId = m_lightNum;
	}

	void Render(int realX, int realY, bool highlighted, bool clicked)
	{
		if (g_app->m_locationEditor->m_selectionId == m_lightNum) 
		{
			DarwiniaButton::Render(realX, realY, highlighted, true);
		}
		else
		{
			DarwiniaButton::Render(realX, realY, highlighted, clicked);
		}
	}
};


class LightGammaButton : public DarwiniaButton
{
public:
    int m_lightNum;
    float m_change;

    LightGammaButton(int num): m_lightNum(num) {}

    void MouseUp()
    {
        Light *light = g_app->m_location->m_lights[m_lightNum];
        light->m_colour[0] *= m_change;
        light->m_colour[1] *= m_change;
        light->m_colour[2] *= m_change;
    }
};


class NewLightButton : public DarwiniaButton
{
    void MouseUp()
    {
        Light *light = new Light();
        g_app->m_location->m_lights.PutData( light );

        EclWindow *parent = m_parent;
        parent->Remove();
        parent->Create();
    }
};


// ****************************************************************************
// Class LightsEditWindow
// ****************************************************************************

LightsEditWindow::LightsEditWindow( char *name )
:	DarwiniaWindow(name) 
{
}


LightsEditWindow::~LightsEditWindow()
{
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
}


void LightsEditWindow::Create()
{
	char buttonName[256];
	DarwiniaWindow::Create();

	int height = 5;
	int pitch = 17;
	int buttonWidth = m_w - 50;

    NewLightButton *newLight = new NewLightButton();
    newLight->SetShortProperties("editor_newlight" , 10, height += pitch, m_w - 20, -1, LANGUAGEPHRASE("editor_newlight") );
    RegisterButton( newLight );

	LightButton *button = new LightButton(-1);
	button->SetShortProperties("editor_deselectlights", 10, height += pitch, m_w - 20, -1, LANGUAGEPHRASE("editor_deselectlights"));
	RegisterButton(button);
	
    height += 6;

    for (int i = 0; i < g_app->m_location->m_lights.Size(); i++)
	{
		button = new LightButton(i);
        
        Light *light = g_app->m_location->m_lights.GetData(i);
		button->SetShortProperties("editor_selectlight", 10, height += pitch, m_w - 20, -1, LANGUAGEPHRASE("editor_selectlight"));
		RegisterButton(button);

        height += pitch;

        LabelButton *label = new LabelButton();
        label->SetShortProperties( "editor_adjustbrightness", 10, height, m_w - 50, -1, LANGUAGEPHRASE("editor_adjustbrightness") );
        RegisterButton( label );
        
        LightGammaButton *gammaDown = new LightGammaButton(i);
        sprintf(buttonName, "down %d", i);
		gammaDown->SetShortProperties( buttonName, m_w - 42, height, 15, -1, LANGUAGEPHRASE(buttonName) );
        gammaDown->SetCaption( "<" );
        gammaDown->m_change = 0.9;
        RegisterButton( gammaDown );

        LightGammaButton *gammaUp = new LightGammaButton(i);
        sprintf(buttonName, "up %d", i );
        gammaUp->SetShortProperties( buttonName, m_w - 25, height, 15, -1, LANGUAGEPHRASE(buttonName) );
        gammaUp->SetCaption( ">" );
        gammaUp->m_change = 1.1;
        RegisterButton( gammaUp );

 		sprintf(buttonName, "Y%d", i);
		CreateValueControl(buttonName, &(light->m_front[1]), height += pitch, 0.01, -20, 20, NULL );

		sprintf(buttonName, "R%d", i);
		CreateValueControl(buttonName, &(light->m_colour[0]), height += pitch, 0.02, 0, 5, NULL );
 		sprintf(buttonName, "G%d", i);
		CreateValueControl(buttonName, &(light->m_colour[1]), height += pitch, 0.02, 0, 5, NULL );
 		sprintf(buttonName, "B%d", i);
		CreateValueControl(buttonName, &(light->m_colour[2]), height += pitch, 0.02, 0, 5, NULL );

		height += 6;
	}
}


#endif // LOCATION_EDITOR
