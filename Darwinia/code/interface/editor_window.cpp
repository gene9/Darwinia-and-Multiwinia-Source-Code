#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "interface/buildings_window.h"
#include "interface/camera_mount_window.h"
#include "interface/editor_window.h"
#include "interface/input_field.h"
#include "interface/instant_unit_window.h"
#include "interface/landscape_window.h"
#include "interface/lights_window.h"
#include "interface/message_dialog.h"

#include "worldobject/building.h"
#include "worldobject/factory.h"
#include "worldobject/cave.h"
#include "worldobject/teleport.h"
#include "worldobject/laserfence.h"
#include "worldobject/powerstation.h"

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "level_file.h"
#include "renderer.h"
#include "location.h"

#ifdef LOCATION_EDITOR

// ****************************************************************************
// Class MainEditWindowButton
// ****************************************************************************

class MainEditWindowButton : public BorderlessButton
{
public:
	enum
	{
		TypeSave = LocationEditor::ModeNumModes
	};

	int m_type;

	MainEditWindowButton(int type)
	:	m_type(type)
	{
	}

    void MouseUp()
    {
		if (m_type == TypeSave)
		{
            //
            // If a MOD hasn't been set, don't allow this to happen
            // as it will try to save into darwinia/data/levels, which is clearly wrong
            // for the end user (but allow it for us)

#ifndef TARGET_DEBUG
            if( !g_app->m_resource->IsModLoaded() )
            {
                EclRegisterWindow( new MessageDialog( LANGUAGEPHRASE( "dialog_savelocationsfail1" ),
                                                      LANGUAGEPHRASE( "dialog_savelocationsfail2" ) ),
                                                      m_parent );
                return;
            }
#endif

            g_app->m_location->m_levelFile->Save();

			return;
		}

		g_app->m_locationEditor->RequestMode(m_type);
    }

	void Render(int realX, int realY, bool highlighted, bool clicked)
	{
		LocationEditor *editor = g_app->m_locationEditor;
		if (editor->GetMode() == m_type)
		{
			BorderlessButton::Render(realX, realY, highlighted, true);
		}
		else
		{
			BorderlessButton::Render(realX, realY, highlighted, clicked);
		}
	}
};


// ****************************************************************************
// Class MainEditWindow
// ****************************************************************************

MainEditWindow::MainEditWindow(char *name)
:	DarwiniaWindow(name),
	m_currentEditWindow(NULL)
{
}


void MainEditWindow::Create()
{
	int y = 3;
	int buttonYPitch = 18;

	MainEditWindowButton *button;
	button = new MainEditWindowButton(LocationEditor::ModeLandTile);
	button->SetShortProperties(LANGUAGEPHRASE("editor_editlandtiles"), 7, y += buttonYPitch, m_w - 15 );
	RegisterButton(button);

	button = new MainEditWindowButton(LocationEditor::ModeLandFlat);
	button->SetShortProperties(LANGUAGEPHRASE("editor_editflattenareas"), 7, y += buttonYPitch, m_w - 15 );
	RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeLight);
	button->SetShortProperties(LANGUAGEPHRASE("editor_editlights"), 7, y += buttonYPitch, m_w - 15 );
	RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeBuilding);
    button->SetShortProperties(LANGUAGEPHRASE("editor_editbuildings"), 7, y += buttonYPitch, m_w - 15 );
    RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeInstantUnit);
    button->SetShortProperties(LANGUAGEPHRASE("editor_editinstantunits"), 7, y += buttonYPitch, m_w - 15 );
    RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeCameraMount);
    button->SetShortProperties(LANGUAGEPHRASE("editor_editcameramounts"), 7, y += buttonYPitch, m_w - 15 );
    RegisterButton(button);

	y += 6;

	button = new MainEditWindowButton(MainEditWindowButton::TypeSave);
	button->SetShortProperties(LANGUAGEPHRASE("editor_save"), 7, y += buttonYPitch, m_w - 15);
	RegisterButton(button);
}

#endif //LOCATION_EDITOR
