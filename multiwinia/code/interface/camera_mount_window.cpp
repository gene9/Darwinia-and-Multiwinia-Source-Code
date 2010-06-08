#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/math/random_number.h"
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
		sprintf(mount->m_name, "blah%d", AppRandom());

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

		AppDebugAssert(false);
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

		AppDebugAssert(false);
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

		AppDebugAssert(false);
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
	EclRemoveWindow("editor_cameraanims");
	EclRemoveWindow("editor_cameraanim");
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
}


void CameraMountEditWindow::Create()
{
	DarwiniaWindow::Create();

	int height = 5;
	int pitch = 17;

    NewMountButton *generate = new NewMountButton();
    generate->SetShortProperties( "editor_createnewmount", 10, height += pitch, -1, -1, LANGUAGEPHRASE("editor_createnewmount") );
    RegisterButton( generate );

	height += 10;

	for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraMounts.Size(); ++i)
	{
		CameraMount *mount = g_app->m_location->m_levelFile->m_cameraMounts.GetData(i);

		char buttonName[(CAMERA_MOUNT_MAX_NAME_LEN + 1) + 64];
		char temp[(CAMERA_MOUNT_MAX_NAME_LEN + 1) + 64];


		sprintf(temp, ":%s", mount->m_name);
		UnicodeString captionCat(temp);

		sprintf(buttonName, "Name:%s", mount->m_name);
		InputField *button = new InputField();
		button->SetShortProperties(buttonName, 10, height += pitch, 150,-1,LANGUAGEPHRASE("dialog_name"));
		button->RegisterString(mount->m_name, sizeof( mount->m_name ) );
		RegisterButton(button);

		sprintf(buttonName, "Delete:%s", mount->m_name);
		DarwiniaButton *delButton = new DeleteMountButton(mount->m_name);
		delButton->SetShortProperties(buttonName, 170, height,-1,-1,LANGUAGEPHRASE("editor_delete"));
		RegisterButton(delButton);

		sprintf(buttonName, "Goto:%s", mount->m_name);
		DarwiniaButton *gotoButton = new GotoMountButton(mount->m_name);
		gotoButton->SetShortProperties(buttonName, 220, height,-1,-1,LANGUAGEPHRASE("editor_goto"));
		RegisterButton(gotoButton);

		sprintf(buttonName, "Update:%s", mount->m_name);
		DarwiniaButton *updateButton = new UpdateMountButton(mount->m_name);
		updateButton->SetShortProperties(buttonName, 260, height,-1,-1,LANGUAGEPHRASE("editor_update"));
		RegisterButton(updateButton);
	}
}


#endif // LOCATION_EDITOR
