#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/random.h"
#include "lib/language_table.h"

#include "interface/camera_mount_window.h"
#include "interface/camera_anim_window.h"
#include "interface/input_field.h"

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "level_file.h"
#include "location.h"


#ifdef LOCATION_EDITOR


//*****************************************************************************
// Buttons for CameraMountEditWindow
//*****************************************************************************

class NewMountButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
        CameraMount *mount = new CameraMount();
		mount->m_pos = g_app->m_camera->GetPos();
		mount->m_front = g_app->m_camera->GetFront();
		mount->m_up = g_app->m_camera->GetUp();
		sprintf(mount->m_name, "blah%d", darwiniaRandom());

        g_app->m_location->m_levelFile->m_cameraMounts.PutDataAtEnd( mount );

        EclWindow *parent = m_parent;
        parent->Remove();
        parent->Create();
    }
};


class GotoMountButton: public DarwiniaButton
{
public:
	char *m_mountName;

	GotoMountButton(char *_mountName) : m_mountName(_mountName) {}

	void MouseUp()
	{
		for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
		{
			CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
			if (stricmp(mount->m_name, m_mountName) == 0)
			{
				g_app->m_camera->SetTarget(mount->m_pos, mount->m_front, mount->m_up);
				g_app->m_camera->CutToTarget();
				return;
			}
		}

		DarwiniaDebugAssert(false);
	}
};


class DeleteMountButton: public DarwiniaButton
{
public:
	char *m_mountName;

	DeleteMountButton(char *_mountName) : m_mountName(_mountName) {}

	void MouseUp()
	{
		for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
		{
			CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
			if (stricmp(mount->m_name, m_mountName) == 0)
			{
				g_app->m_location->m_levelFile->m_cameraMounts.RemoveData(i);
				delete mount;

				EclWindow *parent = m_parent;
				parent->Remove();
				parent->Create();

				return;
			}
		}

		DarwiniaDebugAssert(false);
	}
};


class UpdateMountButton: public DarwiniaButton
{
public:
	char *m_mountName;

	UpdateMountButton(char *_mountName) : m_mountName(_mountName) {}

	void MouseUp()
	{
		for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
		{
			CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts[i];
			if (stricmp(mount->m_name, m_mountName) == 0)
			{
				mount->m_pos = g_app->m_camera->GetPos();
				mount->m_front = g_app->m_camera->GetFront();
				mount->m_up = g_app->m_camera->GetUp();

				return;
			}
		}

		DarwiniaDebugAssert(false);
	}
};



// ****************************************************************************
// Class CameraMountEditWindow
// ****************************************************************************

CameraMountEditWindow::CameraMountEditWindow( char *name )
:	DarwiniaWindow(name)
{
}


CameraMountEditWindow::~CameraMountEditWindow()
{
	EclRemoveWindow(LANGUAGEPHRASE("editor_cameraanims"));
	EclRemoveWindow(LANGUAGEPHRASE("editor_cameraanim"));
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
}


void CameraMountEditWindow::Create()
{
	DarwiniaWindow::Create();

	int height = 5;
	int pitch = 17;

    NewMountButton *generate = new NewMountButton();
    generate->SetShortProperties( LANGUAGEPHRASE("editor_createnewmount"), 10, height += pitch );
    RegisterButton( generate );

	height += 10;

	for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
	{
		CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts.GetData(i);

		char buttonName[64];

		sprintf(buttonName, "%s:%s", LANGUAGEPHRASE("dialog_name"), mount->m_name);
		InputField *button = new InputField();
		button->SetShortProperties(LANGUAGEPHRASE("dialog_name"), 10, height += pitch, 150);
		strcpy(button->m_name, buttonName);
		button->RegisterString(mount->m_name);
		RegisterButton(button);

		sprintf(buttonName, "%s:%s", LANGUAGEPHRASE("dialog_delete"), mount->m_name);
		DarwiniaButton *delButton = new DeleteMountButton(mount->m_name);
		delButton->SetShortProperties(LANGUAGEPHRASE("editor_del"), 170, height);
		strcpy(delButton->m_name, buttonName);
		RegisterButton(delButton);

		sprintf(buttonName, "%s:%s", LANGUAGEPHRASE("editor_goto"), mount->m_name);
		DarwiniaButton *gotoButton = new GotoMountButton(mount->m_name);
		gotoButton->SetShortProperties(LANGUAGEPHRASE("editor_goto"), 210, height);
		strcpy(gotoButton->m_name, buttonName);
		RegisterButton(gotoButton);

		sprintf(buttonName, "%s:%s", LANGUAGEPHRASE("editor_update"), mount->m_name);
		DarwiniaButton *updateButton = new UpdateMountButton(mount->m_name);
		updateButton->SetShortProperties(LANGUAGEPHRASE("editor_update"), 256, height);
		strcpy(updateButton->m_name, buttonName);
		RegisterButton(updateButton);
	}
}


#endif // LOCATION_EDITOR
