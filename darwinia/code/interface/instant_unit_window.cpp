#include "lib/universal_include.h"

#include <stdio.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/language_table.h"

#include "interface/input_field.h"
#include "interface/instant_unit_window.h"

#include "app.h"
#include "camera.h"
#include "level_file.h"
#include "location.h"
#include "location_editor.h"
#include "renderer.h"
#include "user_input.h"
#include "team.h"

#include "worldobject/entity.h"

#ifdef LOCATION_EDITOR


// ****************************************************************************
// Class EditButton
// ****************************************************************************

class EditButton: public DarwiniaButton
{
public:
	EditButton() {}
	
	void MouseUp()
	{
		if (stricmp(m_name, LANGUAGEPHRASE("editor_move")) == 0)
		{
			g_app->m_locationEditor->m_tool = LocationEditor::ToolMove;
		}
	}
};


// ****************************************************************************
// Class TeamButton1
// ****************************************************************************

class TeamButton1 : public DarwiniaButton
{
public:
    int m_teamId;
    TeamButton1( int _teamId )
        :   m_teamId(_teamId)
    {
        if( m_teamId == -1 ) m_teamId = -1;
    }

    void MouseUp()
    {
		InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(g_app->m_locationEditor->m_selectionId);
        if( iu )
        {            
            iu->m_teamId = m_teamId;
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked)
    {
		InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(g_app->m_locationEditor->m_selectionId);
        if( iu )
        {            
            if( iu->m_teamId == m_teamId )
            {
                DarwiniaButton::Render( realX, realY, true, clicked );
            }
            else
            {
                DarwiniaButton::Render( realX, realY, highlighted, clicked );
            }
        }

        if( m_teamId == 255 )
        {
            glColor3ub( 100, 100, 100 );
        }
        else
        {
            RGBAColour col = g_app->m_location->m_teams[ m_teamId ].m_colour;
            glColor3ubv( col.GetData() );
        }

        glBegin( GL_QUADS );
            glVertex2i( realX + 30, realY + 4 );
            glVertex2i( realX + 40, realY + 4 );
            glVertex2i( realX + 40, realY + 12 );
            glVertex2i( realX + 30, realY + 12 );
        glEnd();
    }
};


// ****************************************************************************
// Class DeleteInstantUnitButton
// ****************************************************************************

class DeleteInstantUnitButton : public DarwiniaButton
{
public:
    bool m_safetyCatch;
    DeleteInstantUnitButton()
        : m_safetyCatch(true){}
    
    void MouseUp()
    {
        if( m_safetyCatch )
        {
            SetCaption(LANGUAGEPHRASE("editor_deletenow") );
            m_safetyCatch = false;
        }
        else
        {
			InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(g_app->m_locationEditor->m_selectionId);
			delete iu;

            g_app->m_location->m_levelFile->m_instantUnits.RemoveData(g_app->m_locationEditor->m_selectionId);
    	    EclRemoveWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
            g_app->m_locationEditor->m_tool = LocationEditor::ToolNone;
            g_app->m_locationEditor->m_selectionId = -1;
        }
    }
};



// ****************************************************************************
// Class InstantUnitEditWindow
// ****************************************************************************

InstantUnitEditWindow::InstantUnitEditWindow( char *name )
:	DarwiniaWindow(name)
{
}


InstantUnitEditWindow::~InstantUnitEditWindow()
{
	g_app->m_locationEditor->m_selectionId = -1;
}


void InstantUnitEditWindow::Create()
{
	DarwiniaWindow::Create();

	int y = 4;
	int buttonPitch = 18;

    EditButton *mb = new EditButton();
	mb->SetShortProperties(LANGUAGEPHRASE("editor_move"), 10, y+=buttonPitch, m_w-20);
	RegisterButton(mb);

    DeleteInstantUnitButton *db = new DeleteInstantUnitButton();
    db->SetShortProperties(LANGUAGEPHRASE("editor_delete"), 10, y+=buttonPitch, m_w-20);
    RegisterButton(db);

	y += buttonPitch;

    for( int i = 0; i < 3; ++i )
    {
        char name[64];
        int w = m_w/3 - 8;
        sprintf( name, "T%d", i);
        TeamButton1 *tb = new TeamButton1(i);
        tb->SetShortProperties(name, 10 + (i*w) + (i*2), y, w);
        RegisterButton(tb);
    }

	y += 7;

	InstantUnit *iu = g_app->m_location->m_levelFile->m_instantUnits.GetData(
						g_app->m_locationEditor->m_selectionId);
    CreateValueControl( LANGUAGEPHRASE("editor_numentities"), InputField::TypeInt, &iu->m_number, y+=buttonPitch, 1, 1, 1000 );
    CreateValueControl( LANGUAGEPHRASE("editor_spread"), InputField::TypeFloat, &iu->m_spread, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );
    CreateValueControl( LANGUAGEPHRASE("editor_inunit"), InputField::TypeChar, &iu->m_inAUnit, y+=buttonPitch, 1,0,1 );

	y += 7;
}


// ****************************************************************************
// Class CreateButton
// ****************************************************************************

class CreateButton: public DarwiniaButton
{
public:
	CreateButton() {}
	
	void MouseUp()
	{
		for (int i = 0; i < Entity::NumEntityTypes; ++i)
		{
			if (stricmp(m_name, Entity::GetTypeNameTranslated(i)) == 0)
			{
				g_app->m_locationEditor->m_tool = LocationEditor::ToolMove;

				// Where did we click?
				Vector3 rayStart, rayDir, hitPos;
	            g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
                                             g_app->m_renderer->ScreenH()/2, 
                                             &rayStart, &rayDir);
                g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &hitPos );
				
				// Make sure that any old edit window is removed
				EclWindow *ew = EclGetWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
				if (ew)
				{
					EclRemoveWindow(ew->m_name);
				}

				// Create the new instant unit
				InstantUnit *iu = new InstantUnit();
				iu->m_number = 40;
				iu->m_posX = hitPos.x;
				iu->m_posZ = hitPos.z;
				iu->m_teamId = 0;
				iu->m_type = i;
                iu->m_inAUnit = false;				
				g_app->m_locationEditor->m_selectionId = g_app->m_location->m_levelFile->m_instantUnits.Size();
				g_app->m_location->m_levelFile->m_instantUnits.PutData(iu);

				// Create an edit window for the new instant unit
				EclWindow *cw = EclGetWindow(LANGUAGEPHRASE("editor_instantunits"));
				DarwiniaDebugAssert(cw);
				ew = new InstantUnitEditWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
				ew->m_w = cw->m_w;
				ew->m_h = 160;
				ew->m_x = cw->m_x;
				EclRegisterWindow(ew);
				ew->m_y = cw->m_y - ew->m_h - 10;
			}
		}
	}
};


// ****************************************************************************
// Class InstantUnitCreateWindow
// ****************************************************************************

InstantUnitCreateWindow::InstantUnitCreateWindow( char *name )
:	DarwiniaWindow(name) 
{
}


InstantUnitCreateWindow::~InstantUnitCreateWindow()
{
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
	EclRemoveWindow(LANGUAGEPHRASE("editor_instantuniteditor"));
}


void InstantUnitCreateWindow::Create()
{
	DarwiniaWindow::Create();

	int y = 3;
	int const buttonYPitch = 18;
	char *lowerLimit = " ";
	char *best; 

	for (int i = 0; i < Entity::NumEntityTypes; ++i)
	{
		best = "~~~~"; // All reasonable strings come before this when alphabetically sorted
		for (int j = 0; j < Entity::NumEntityTypes; ++j)
		{
			char *typeName = Entity::GetTypeNameTranslated(j);
			if (stricmp(typeName, lowerLimit) > 0 &&
				stricmp(typeName, best) < 0)
			{
				best = typeName;
			}
		}
		lowerLimit = best;
		CreateButton *button = new CreateButton();
		button->SetShortProperties(best, 10, y += buttonYPitch, m_w - 20);
		RegisterButton(button);
	}
}

#endif // LOCATION_EDITOR
