#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/random.h"
#include "lib/language_table.h"

#include "interface/camera_anim_window.h"
#include "interface/camera_mount_window.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"

#include "app.h"
#include "camera.h"
#include "location_editor.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "renderer.h"


#ifdef LOCATION_EDITOR


//*****************************************************************************
// Buttons for Main Window
//*****************************************************************************

class NewAnimButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
		CameraAnimation *anim = new CameraAnimation;
		sprintf(anim->m_name, "CamAnim%d", darwiniaRandom() & 0x3ff);
		g_app->m_location->m_levelFile->m_cameraAnimations.PutData(anim);
		
		CameraAnimMainEditWindow *parent = (CameraAnimMainEditWindow *)m_parent;
		parent->RemoveButtons();
		parent->AddButtons();
    }
};


class DeleteAnimButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		LList <CameraAnimation *> *anims = &g_app->m_location->m_levelFile->m_cameraAnimations;
		for (int i = 0; i < anims->Size(); ++i)
		{
			if (stricmp(anims->GetData(i)->m_name, m_name) == 0)
			{
				delete anims->GetData(i);
				anims->RemoveData(i);
				break;
			}
		}

		CameraAnimMainEditWindow *parent = (CameraAnimMainEditWindow *)m_parent;
		parent->RemoveButtons();
		parent->AddButtons();
	}
};


class SelectAnimButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		EclRemoveWindow(LANGUAGEPHRASE("editor_cameraanim"));

		char *animName = m_name + strlen("select:");
		int animId = g_app->m_location->m_levelFile->GetCameraAnimId(animName);
		g_app->m_locationEditor->m_selectionId = animId;

//		EclWindow *secondaryWin = new CameraAnimSecondaryEditWindow(
//										LANGUAGEPHRASE("editor_cameraanim"),
//										animId);
//		EclRegisterWindow(secondaryWin);
	}
};



// ****************************************************************************
// Main Edit Window Class
// ****************************************************************************

CameraAnimMainEditWindow::CameraAnimMainEditWindow( char *name )
:	DarwiniaWindow(name) 
{
}


CameraAnimMainEditWindow::~CameraAnimMainEditWindow()
{
	EclRemoveWindow(LANGUAGEPHRASE("editor_cameramounts"));
	EclRemoveWindow(LANGUAGEPHRASE("editor_cameraanim"));
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
}


void CameraAnimMainEditWindow::Create()
{
	DarwiniaWindow::Create();
	AddButtons();
}


void CameraAnimMainEditWindow::AddButtons()
{
	int height = 5;
	int pitch = 17;

    NewAnimButton *generate = new NewAnimButton();
    generate->SetShortProperties("Create New Anim", 10, height += pitch);
    RegisterButton( generate );

	height += 10;

	for (int i = 0; i < g_app->m_location->m_levelFile->m_cameraAnimations.Size(); ++i)
	{
		CameraAnimation *anim = g_app->m_location->m_levelFile->m_cameraAnimations.GetData(i);

		char buttonName[64];

		sprintf(buttonName, "name:%s", anim->m_name);
		InputField *button = new InputField();
		button->SetShortProperties("Name:", 10, height += pitch, 150);
		strcpy(button->m_name, buttonName);
		button->RegisterString(anim->m_name);
		RegisterButton(button);

		DarwiniaButton *delButton = new DeleteAnimButton();
		delButton->SetShortProperties("Del", 170, height);
		sprintf(delButton->m_name, "%s", anim->m_name);
		RegisterButton(delButton);

		sprintf(buttonName, "select:%s", anim->m_name);
		DarwiniaButton *selectButton = new SelectAnimButton();
		selectButton->SetShortProperties("Select", 210, height);
		strcpy(selectButton->m_name, buttonName);
		RegisterButton(selectButton);
	}
}


void CameraAnimMainEditWindow::RemoveButtons()
{
	while (m_buttons.Size())
	{
		RemoveButton(m_buttons[0]->m_name);
	}
}



//*****************************************************************************
// Buttons for Secondary Window
//*****************************************************************************


class NewNodeButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		CameraAnimSecondaryEditWindow *parent = 
			(CameraAnimSecondaryEditWindow *)m_parent;
		parent->m_newNodeArmed = !parent->m_newNodeArmed;
	}

	void Render(int x, int y, bool _hasFocus, bool _clicked)
	{
		CameraAnimSecondaryEditWindow *parent = 
			(CameraAnimSecondaryEditWindow *)m_parent;
		int halfSecond = (int)(g_gameTime * 2.0f) & 1;
		if (parent->m_newNodeArmed && halfSecond)
		{
			DarwiniaButton::Render(x, y, true, false);
		}
		else
		{
			DarwiniaButton::Render(x, y, false, false);
		}
	}
};


class CamBeforeMountButton: public DarwiniaButton
{
public:
	void MouseUp()
	{
		CameraAnimSecondaryEditWindow *parent = 
			(CameraAnimSecondaryEditWindow *)m_parent;
		if (parent->m_newNodeArmed)
		{
			CameraAnimation *anim = g_app->m_location->m_levelFile->m_cameraAnimations[parent->m_animId];
			DarwiniaDebugAssert(anim);
			
			CamAnimNode *node = new CamAnimNode;
			node->m_mountName = MAGIC_MOUNT_NAME_START_POS;

			anim->m_nodes.PutData(node);

			parent->m_newNodeArmed = false;
			parent->RemoveButtons();
			parent->AddButtons();
		}
	}
};


class StartPreviewButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		CameraAnimSecondaryEditWindow *parent = 
			(CameraAnimSecondaryEditWindow *)m_parent;
		CameraAnimation *anim = g_app->m_location->m_levelFile->m_cameraAnimations.GetData(
									parent->m_animId);
		g_app->m_camera->PlayAnimation(anim);
	}
};


class StopPreviewButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		g_app->m_camera->StopAnimation();
	}
};

		
class SelectMountButton : public DarwiniaButton
{
public:
	void Render(int x, int y, bool _hasFocus, bool _clicked)
	{
		DarwiniaButton::Render(x, y, false, false);
	}
};


class DeleteNodeButton : public DarwiniaButton
{
public:
	void MouseUp()
	{
		CameraAnimSecondaryEditWindow *parent = 
									(CameraAnimSecondaryEditWindow*)m_parent;
		CameraAnimation *anim = g_app->m_location->m_levelFile->
									m_cameraAnimations.GetData(parent->m_animId);
		
		char *mountName = m_name + 7;
		for (int i = 0; i < anim->m_nodes.Size(); ++i)
		{
			if (stricmp(anim->m_nodes[i]->m_mountName, mountName) == 0)
			{
				anim->m_nodes.RemoveData(i);
				break;
			}
		}
		
		parent->RemoveButtons();
		parent->AddButtons();
	}
};



// ****************************************************************************
// Secondary Edit Window Class
// ****************************************************************************

CameraAnimSecondaryEditWindow::CameraAnimSecondaryEditWindow(char *name, int _animId)
:	DarwiniaWindow(name),
	m_animId(_animId),
	m_newNodeArmed(false)
{
	m_w = 310;
	m_h = 100;
	m_x = g_app->m_renderer->ScreenW() - m_w;
	m_y = g_app->m_renderer->ScreenH() - m_h;
}


CameraAnimSecondaryEditWindow::~CameraAnimSecondaryEditWindow()
{
	g_app->m_locationEditor->m_selectionId = -1;
}


void CameraAnimSecondaryEditWindow::Create()
{
	DarwiniaWindow::Create();
	AddButtons();
}


void CameraAnimSecondaryEditWindow::AddButtons()
{
	NewNodeButton *newLinkButton = new NewNodeButton();
	newLinkButton->SetShortProperties("Add link", 10, 22);
	RegisterButton(newLinkButton);

	CamBeforeMountButton *camBeforeMountButton = new CamBeforeMountButton;
	camBeforeMountButton->SetShortProperties("CamStart", 85, 22);
	RegisterButton(camBeforeMountButton);

	StartPreviewButton *startPreviewButton = new StartPreviewButton;
	startPreviewButton->SetShortProperties("Preview", m_w - 112, 22);
	RegisterButton(startPreviewButton);

	StopPreviewButton *stopPreviewButton = new StopPreviewButton;
	stopPreviewButton->SetShortProperties("Stop", m_w - 46, 22);
	RegisterButton(stopPreviewButton);


	//
	// Create new buttons for each mount

	int height = 49;
	int pitch = 17;
	CameraAnimation *anim = g_app->m_location->m_levelFile->m_cameraAnimations.GetData(m_animId);
    if( anim )
    {
	    for (int j = 0; j < anim->m_nodes.Size(); ++j)
	    {
		    CamAnimNode *node = anim->m_nodes.GetData(j);

		    int x = 10;

		    DropDownMenu *modeBut = new DropDownMenu();
		    modeBut->SetShortProperties("Mode", x, height, 60);
		    modeBut->RegisterInt(&node->m_transitionMode);
		    for (int i = 0; i < CamAnimNode::TransitionNumModes; ++i)
		    {
			    modeBut->AddOption(CamAnimNode::GetTransitModeName(i));
		    }
		    modeBut->SelectOption(node->m_transitionMode);
		    x += 70;
		    sprintf(modeBut->m_name, "mode:%s", node->m_mountName);
		    RegisterButton(modeBut);

		    CreateValueControl(node->m_mountName, InputField::TypeFloat, &node->m_duration,
					    height, 0.5f, 0.1f, 100.0f, NULL, x, 90);
		    EclButton *b = GetButton(node->m_mountName);
		    sprintf(b->m_name, "duration:%s", node->m_mountName);
		    b->m_caption[0] = '\0';
		    x += 100;

		    DarwiniaButton *but;
		    
		    but = new SelectMountButton();
		    but->SetShortProperties(node->m_mountName, x, height, 80);
		    x += 90;
		    sprintf(but->m_name, "mount:%s", node->m_mountName);
		    RegisterButton(but);
		    
		    but = new DeleteNodeButton();
		    but->SetShortProperties("Del", x, height, 30);
		    sprintf(but->m_name, "delete:%s", node->m_mountName);
		    RegisterButton(but);
		    
		    height += pitch;
	    }
    }

	if (height > m_h)
	{
		SetSize(m_w, height);
	}
}


void CameraAnimSecondaryEditWindow::RemoveButtons()
{
	for (int i = 0; i < m_buttons.Size(); ++i)
	{
		EclButton *but = m_buttons[i];
		if (stricmp(but->m_name, "Close") != 0)
		{
			RemoveButton(m_buttons[i]->m_name);
			--i;
		}
	}
}


#endif // LOCATION_EDITOR
