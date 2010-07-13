#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include <fstream>
#include <sstream>
#include <string>

#include "lib/tosser/directory.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/filesys_utils.h"

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
#include "mapfile.h"

#ifdef LOCATION_EDITOR

// ****************************************************************************
// Class MainEditWindowButton
// ****************************************************************************

class MainEditWindowButton : public BorderlessButton
{
public:
	enum
	{
		TypeSave = LocationEditor::ModeNumModes,
        TypePublish = 255
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
            /*if( !g_app->m_resource->IsModLoaded() ) 
            {
                EclRegisterWindow( new MessageDialog( LANGUAGEPHRASE( "dialog_savelocationsfail1" ),
                                                      LANGUAGEPHRASE( "dialog_savelocationsfail2" ) ), 
                                                      m_parent );
                return;
            }*/
#endif
            g_app->m_location->m_levelFile->Save();
            return;
        }
        else if( m_type == TypePublish )
        {
            char filename[512];
            sprintf( filename, "%s%s", g_app->GetMapDirectory(), g_app->m_location->m_levelFile->m_mapFilename );
            MapFile *mapFile = new MapFile();
            mapFile->Save(filename);
            delete mapFile;
            
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
	button->SetShortProperties("editor_editlandtiles", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editlandtiles") );
	RegisterButton(button);

	button = new MainEditWindowButton(LocationEditor::ModeLandFlat);
	button->SetShortProperties("editor_editflattenareas", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editflattenareas") );
	RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeLight);
	button->SetShortProperties("editor_editlights", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editlights") );
	RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeBuilding);
    button->SetShortProperties("editor_editbuildings", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editbuildings") );
    RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeInstantUnit);
    button->SetShortProperties("editor_editinstantunits", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editinstantunits") );
    RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeCameraMount);
    button->SetShortProperties("editor_editcameramounts", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_editcameramounts") );
    RegisterButton(button);

    button = new MainEditWindowButton(LocationEditor::ModeMultiwinia);
    button->SetShortProperties( "editor_multiwiniaoptions", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("dialog_otheroptions") );
    RegisterButton( button );

    button = new MainEditWindowButton( LocationEditor::ModeInvalidCrates);
    button->SetShortProperties( "editor_invalidcrates", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("dialog_invalidcrates") );
    RegisterButton( button );

	y += 6;

#ifdef TARGET_DEBUG
	button = new MainEditWindowButton(MainEditWindowButton::TypeSave);
	button->SetShortProperties("editor_save", 7, y += buttonYPitch, m_w - 15, -1, LANGUAGEPHRASE("editor_save"));
	RegisterButton(button);
#endif

    UnicodeString caption;
#ifdef TARGET_DEBUG
    caption = LANGUAGEPHRASE("editor_publish");
#else
    caption = LANGUAGEPHRASE("editor_save");
#endif

    button = new MainEditWindowButton(MainEditWindowButton::TypePublish);
	button->SetShortProperties("editor_publish", 7, y += buttonYPitch, m_w - 15, -1, caption );
	RegisterButton(button);
}

#endif //LOCATION_EDITOR
