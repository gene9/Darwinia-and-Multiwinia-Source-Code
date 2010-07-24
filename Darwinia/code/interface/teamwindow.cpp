#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/vector3.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "interface/teamwindow.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"
#include "interface/editor_window.h"

#include "worldobject/factory.h"
#include "worldobject/trunkport.h"
#include "worldobject/laserfence.h"
#include "worldobject/anthill.h"
#include "worldobject/safearea.h"
#include "worldobject/mine.h"
#include "worldobject/generator.h"
#include "worldobject/researchitem.h"
#include "worldobject/triffid.h"
#include "worldobject/blueprintstore.h"
#include "worldobject/ai.h"
#include "worldobject/spawnpoint.h"
#include "worldobject/scripttrigger.h"
#include "worldobject/staticshape.h"
#include "worldobject/incubator.h"
#include "worldobject/rocket.h"
#include "worldobject/generichub.h"
#include "worldobject/switch.h"

#include "app.h"
#include "camera.h"
#include "global_world.h"
#include "location_editor.h"
#include "level_file.h"
#include "renderer.h"
#include "location.h"
#include "team.h"

#ifdef LOCATION_EDITOR
// ****************************************************************************
// Class TeamButton
// ****************************************************************************

class TeamAllianceButton : public DarwiniaButton
{
public:
    int m_teamId;
    TeamAllianceButton( int _teamId )
        :   m_teamId(_teamId)
    {
        if( m_teamId == -1 ) m_teamId = 255;
    }

    void MouseUp()
    {
		if ( g_app->m_location->m_levelFile->m_teamAlliances[g_app->m_locationEditor->m_selectionId][m_teamId] )
		{
			g_app->m_location->m_levelFile->SetAlliance(g_app->m_locationEditor->m_selectionId, m_teamId, false);
			sprintf(m_caption,"T%d (%s)",m_teamId, LANGUAGEPHRASE("editor_teamenemy"));
		}
		else
		{
			g_app->m_location->m_levelFile->SetAlliance(g_app->m_locationEditor->m_selectionId, m_teamId, true);
			sprintf(m_caption,"T%d (%s)",m_teamId, LANGUAGEPHRASE("editor_teamally"));
		}
    }

    void Render( int realX, int realY, bool highlighted, bool clicked)
    {
		DarwiniaButton::Render( realX, realY, highlighted, g_app->m_location->m_levelFile->m_teamAlliances[g_app->m_locationEditor->m_selectionId][m_teamId] );

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
            glVertex2i( realX + 20,			realY + 4 );
            glVertex2i( realX + 24,			realY + 4 );
            glVertex2i( realX + 24,			realY + 14 );
            glVertex2i( realX + 20,			realY + 14 );
        glEnd();

    }
};

// ****************************************************************************
// Class TeamSelectButton
// ****************************************************************************

class TeamSelectButton : public DarwiniaButton
{
public:
    int m_teamId;
    TeamSelectButton( int _teamId )
        :   m_teamId(_teamId)
    {
        if( m_teamId == -1 ) m_teamId = 255;
    }

    void MouseUp()
    {
		g_app->m_locationEditor->m_selectionId = m_teamId;

		MainEditWindow *mainWin = (MainEditWindow *)EclGetWindow(LANGUAGEPHRASE("editor_mainedit"));
		InputField *red = (InputField *) mainWin->m_currentEditWindow->GetButton(LANGUAGEPHRASE("editor_red"));
		red->m_type = InputField::TypeNowt;
		red->RegisterChar(&g_app->m_location->m_teams[m_teamId].m_colour.r);

		InputField *green = (InputField *)mainWin->m_currentEditWindow->GetButton(LANGUAGEPHRASE("editor_green"));
		green->m_type = InputField::TypeNowt;
		green->RegisterChar(&g_app->m_location->m_teams[m_teamId].m_colour.g);

		InputField *blue = (InputField *)mainWin->m_currentEditWindow->GetButton(LANGUAGEPHRASE("editor_blue"));
		blue->m_type = InputField::TypeNowt;
		blue->RegisterChar(&g_app->m_location->m_teams[m_teamId].m_colour.b);

		for ( int i = 0; i < NUM_TEAMS; i++ )
		{
			char *buttonName = new char[256];
			sprintf(buttonName, "T%d_alliance", i);

			TeamAllianceButton *button = (TeamAllianceButton *) mainWin->m_currentEditWindow->GetButton(buttonName);
			if ( button )
			{
				if ( g_app->m_location->m_levelFile->m_teamAlliances[m_teamId][button->m_teamId] ) {
					sprintf(button->m_caption,"T%d (%s)", button->m_teamId, LANGUAGEPHRASE("editor_teamally"));
				} else {
					sprintf(button->m_caption,"T%d (%s)", button->m_teamId, LANGUAGEPHRASE("editor_teamenemy"));
				}
			}
		}
    }

    void Render( int realX, int realY, bool highlighted, bool clicked)
    {
		DarwiniaButton::Render( realX, realY, false, g_app->m_locationEditor->m_selectionId == m_teamId );
    }
};


// ****************************************************************************
// Class TeamSelectButton
// ****************************************************************************

class TeamColourBar : public DarwiniaButton
{
public:

    void Render( int realX, int realY, bool highlighted, bool clicked)
    {
		DarwiniaButton::Render( realX, realY, false, false );

        RGBAColour col = g_app->m_location->m_teams[ g_app->m_locationEditor->m_selectionId ].m_colour;
        glColor3ubv( col.GetData() );

        glBegin( GL_QUADS );
            glVertex2i( realX + 4,       realY + 4 );
            glVertex2i( realX + m_w - 4, realY + 4 );
            glVertex2i( realX + m_w - 4, realY + 12 );
            glVertex2i( realX + 4,       realY + 12 );
        glEnd();
    }
};




// ****************************************************************************
// Class FlagToggleButton
// ****************************************************************************

class FlagToggleButton : public DarwiniaButton
{
public:
	int m_flag;
    FlagToggleButton( int _flag )
        :   m_flag(_flag)
	{ }


    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		if ( g_app->m_location->m_levelFile->m_teamFlags[g_app->m_locationEditor->m_selectionId] & m_flag )
		{
			DarwiniaButton::Render( realX, realY, highlighted, true );
		}
		else
		{
			DarwiniaButton::Render( realX, realY, highlighted, false );
		}
    };

    void MouseUp()
    {
		if ( g_app->m_location->m_levelFile->m_teamFlags[g_app->m_locationEditor->m_selectionId] & m_flag )
		{
			g_app->m_location->m_levelFile->SetFlag(g_app->m_locationEditor->m_selectionId,m_flag,false);
		}
		else
		{
			g_app->m_location->m_levelFile->SetFlag(g_app->m_locationEditor->m_selectionId,m_flag,true);
		}
    }
};


// ****************************************************************************
// Class TeamEditWindow
// ****************************************************************************

TeamEditWindow::TeamEditWindow( char *name )
:   DarwiniaWindow( name )
{
	g_app->m_locationEditor->m_selectionId = 0;
}


TeamEditWindow::~TeamEditWindow()
{
	g_app->m_locationEditor->m_selectionId = -1;
}

void TeamEditWindow::Create()
{
	DarwiniaWindow::Create();

	int buttonPitch = 18;
	int y = 24;

	RGBAColour *m_colour = &(g_app->m_location->m_teams[g_app->m_locationEditor->m_selectionId].m_colour);

	int rows = 0;
    for( int i = 0; i < 5; ++i )
    {
		int j = 0;
		while ( i+(j*5) < NUM_TEAMS )
		{
			char name[64];
			int w = m_w/5 - 8;
			sprintf( name, "T%d", i + (j*5));
			if ( i + (j*5) < NUM_TEAMS ) {
				TeamSelectButton *tsb = new TeamSelectButton(i+(j*5));
				tsb->SetShortProperties(name, 10 + (i*w) + (i*2), y + (j*buttonPitch), w);
				RegisterButton(tsb);
			}
			rows = max(rows,j);
			j++;
		}
    }

	y += (rows*buttonPitch);

    TeamColourBar *tcb = new TeamColourBar();
	tcb->SetShortProperties("editor_colourbar", 10, y += buttonPitch, m_w-20);
	tcb->SetCaption(" ");
	RegisterButton(tcb);

    CreateValueControl( LANGUAGEPHRASE("editor_red"),   InputField::TypeChar, &m_colour->r, y+=buttonPitch, 1, 0, 255 );
    CreateValueControl( LANGUAGEPHRASE("editor_green"), InputField::TypeChar, &m_colour->g, y+=buttonPitch, 1, 0, 255 );
    CreateValueControl( LANGUAGEPHRASE("editor_blue"),  InputField::TypeChar, &m_colour->b, y+=buttonPitch, 1, 0, 255 );

	FlagToggleButton *ftb1 = new FlagToggleButton(1);
	ftb1->SetShortProperties("editor_teamflag_playerspawnteam", 10, y += buttonPitch, m_w-20);
	ftb1->SetCaption( LANGUAGEPHRASE("editor_teamflag_playerspawnteam" ));
	RegisterButton(ftb1);

	FlagToggleButton *ftb2 = new FlagToggleButton(2);
	ftb2->SetShortProperties("editor_teamflag_eggwinians", 10, y += buttonPitch, m_w-20);
	ftb2->SetCaption( LANGUAGEPHRASE("editor_teamflag_eggwinians" ));
	RegisterButton(ftb2);

	FlagToggleButton *ftb3 = new FlagToggleButton(4);
	ftb3->SetShortProperties("editor_teamflag_soulharvest", 10, y += buttonPitch, m_w-20);
	ftb3->SetCaption( LANGUAGEPHRASE("editor_teamflag_soulharvest" ));
	RegisterButton(ftb3);

	FlagToggleButton *ftb4 = new FlagToggleButton(8);
	ftb4->SetShortProperties("editor_teamflag_spawnpointincubation", 10, y += buttonPitch, m_w-20);
	ftb4->SetCaption( LANGUAGEPHRASE("editor_teamflag_spawnpointincubation" ));
	RegisterButton(ftb4);

	FlagToggleButton *ftb5 = new FlagToggleButton(16);
	ftb5->SetShortProperties("editor_teamflag_patterncorruption", 10, y += buttonPitch, m_w-20);
	ftb5->SetCaption( LANGUAGEPHRASE("editor_teamflag_patterncorruption" ));
	RegisterButton(ftb5);

	FlagToggleButton *ftb6 = new FlagToggleButton(32);
	ftb6->SetShortProperties("editor_teamflag_eviltreespawnteam", 10, y += buttonPitch, m_w-20);
	ftb6->SetCaption( LANGUAGEPHRASE("editor_teamflag_eviltreespawnteam" ));
	RegisterButton(ftb6);

	FlagToggleButton *ftb7 = new FlagToggleButton(64);
	ftb7->SetShortProperties("editor_teamflag_soulless", 10, y += buttonPitch, m_w-20);
	ftb7->SetCaption( LANGUAGEPHRASE("editor_teamflag_soulless" ));
	RegisterButton(ftb7);


	y += buttonPitch;
	y += 7;

	rows = 0;
    for( int i = 0; i < 3; ++i )
    {
		int j = 0;
		while ( i+(j*3) < NUM_TEAMS )
		{
			char name[64];
			int w = m_w/3 - 8;
			sprintf( name, "T%d_alliance", i + (j*3));
			if ( i + (j*3) < NUM_TEAMS ) {
				TeamAllianceButton *tb = new TeamAllianceButton(i+(j*3));
				tb->SetShortProperties(name, 10 + (i*w) + (i*2), y + (j*buttonPitch), w);
				if ( g_app->m_location->m_levelFile->m_teamAlliances[g_app->m_locationEditor->m_selectionId][i+(j*3)] )
				{
					sprintf(tb->m_caption,"T%d (%s)",i+(j*3), LANGUAGEPHRASE("editor_teamally"));
				}
				else
				{
					sprintf(tb->m_caption,"T%d (%s)",i+(j*3), LANGUAGEPHRASE("editor_teamenemy"));
				}
				RegisterButton(tb);
			}
			rows = max(rows,j);
			j++;
		}
    }

	y = y + 7 + (rows*buttonPitch);
}

#endif // LOCATION_EDITOR
