#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "lib/vector3.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/language_table.h"

#include "interface/buildings_window.h"
#include "interface/drop_down_menu.h"
#include "interface/input_field.h"

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
#include "worldobject/researchcrate.h"

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
// Class ToolButton
// ****************************************************************************

class ToolButton : public DarwiniaButton
{
public:
	int m_toolType;

	ToolButton(int _toolType)
	:	m_toolType(_toolType)
	{
	}

    void MouseUp()
    {
        g_app->m_locationEditor->m_tool = m_toolType;
    }

	void Render(int realX, int realY, bool highlighted, bool clicked)
	{
		if(g_app->m_locationEditor->m_tool == m_toolType)
		{
			DarwiniaButton::Render(realX, realY, highlighted, true);
		}
		else
		{
			DarwiniaButton::Render(realX, realY, highlighted, clicked);
		}

        if(m_toolType == LocationEditor::ToolLink)
        {
            Building *b = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
            g_editorFont.DrawText2DRight( realX + m_w - 10, realY + 10, 14, "%d", b->GetBuildingLink() );
        }
	}
};


// ****************************************************************************
// Class DeleteBuildingButton
// ****************************************************************************

class DeleteBuildingButton : public DarwiniaButton
{
public:
    bool m_safetyCatch;
    DeleteBuildingButton()
        : m_safetyCatch(true){}

    void MouseUp()
    {
        if( m_safetyCatch )
        {
            SetCaption(LANGUAGEPHRASE("editor_deletenow"));
            m_safetyCatch = false;
        }
        else
        {
            g_app->m_location->m_levelFile->RemoveBuilding( g_app->m_locationEditor->m_selectionId );
    	    EclRemoveWindow(LANGUAGEPHRASE("editor_buildingid"));
            g_app->m_locationEditor->m_tool = LocationEditor::ToolNone;
            g_app->m_locationEditor->m_selectionId = -1;
        }
    }
};


// ****************************************************************************
// Class TeamButton
// ****************************************************************************

class TeamButton : public DarwiniaButton
{
public:
    int m_teamId;
    TeamButton( int _teamId )
        :   m_teamId(_teamId)
    {
        if( m_teamId == -1 ) m_teamId = 255;
    }

    void MouseUp()
    {
        Building *b = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
        if( b )
        {
            b->m_id.SetTeamId( m_teamId );
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked)
    {
        Building *b = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
        if( b )
        {
            if( b->m_id.GetTeamId() == m_teamId )
            {
                DarwiniaButton::Render( realX, realY, true, true );
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
// Class IsGlobalButton
// ****************************************************************************

class IsGlobalButton : public DarwiniaButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        DarwiniaButton::Render( realX, realY, highlighted, clicked );
        Building *b = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
        if( b )
        {
            g_editorFont.DrawText2DRight( realX + m_w - 10, realY+10, DEF_FONT_SIZE, "%d", b->m_isGlobal );
        }
    };

    void MouseUp()
    {
        Building *b = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
        if( b )
        {
            b->m_isGlobal = !b->m_isGlobal;
        }
    }
};


// ****************************************************************************
// Class CloneBuildingButton
// ****************************************************************************

class CloneBuildingButton : public DarwiniaButton
{
    void MouseUp()
    {
	    Vector3 rayStart;
	    Vector3 rayDir;
	    g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2,
									 g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
        Vector3 _pos;
        g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

        Building *building = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
        DarwiniaDebugAssert(building);

        Building *newBuilding = Building::CreateBuilding( building->m_type );
        newBuilding->Initialise( building );
        newBuilding->SetDetail( g_prefsManager->GetInt( "RenderBuildingDetail", 1 ) );
        newBuilding->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );
        newBuilding->m_pos = _pos;
        g_app->m_location->m_levelFile->m_buildings.PutData( newBuilding );
    }
};


// ****************************************************************************
// Class BuildingEditWindow
// ****************************************************************************

BuildingEditWindow::BuildingEditWindow( char *name )
:   DarwiniaWindow( name )
{
}


BuildingEditWindow::~BuildingEditWindow()
{
	g_app->m_locationEditor->m_selectionId = -1;
}

void BuildingEditWindow::Create()
{
	DarwiniaWindow::Create();

	Building *building = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
	DarwiniaDebugAssert(building);

	int buttonPitch = 18;
	int y = 6;

    ToolButton *mb = new ToolButton(LocationEditor::ToolMove);
	mb->SetShortProperties(LANGUAGEPHRASE("editor_move"), 10, y += buttonPitch, m_w-20);
	RegisterButton(mb);

    ToolButton *rb = new ToolButton(LocationEditor::ToolRotate);
	rb->SetShortProperties(LANGUAGEPHRASE("editor_rotate"), 10, y += buttonPitch, m_w-20);
	RegisterButton(rb);

    CloneBuildingButton *clone = new CloneBuildingButton();
    clone->SetShortProperties(LANGUAGEPHRASE("editor_clone"), 10, y+=buttonPitch, m_w-20);
    RegisterButton(clone);

    DeleteBuildingButton *db = new DeleteBuildingButton();
    db->SetShortProperties(LANGUAGEPHRASE("editor_delete"), 10, y += buttonPitch, m_w-20);
    RegisterButton(db);

    ToolButton *lb = new ToolButton(LocationEditor::ToolLink);
    lb->SetShortProperties(LANGUAGEPHRASE("editor_link"), 10, y += buttonPitch, m_w-20);
    RegisterButton(lb);

	y += buttonPitch;
/*
    for( int i = -1; i < 3; ++i )
    {
        char name[256];
        int w = m_w/4 - 5;
        sprintf( name, "T%d",i);
        TeamButton *tb = new TeamButton(i);
        tb->SetShortProperties(name, 61 + (float)i*((float)w + 1.0f), y, w - 2);
        RegisterButton(tb);
    }
*/
	int rows = 0;
    for( int i = 0; i < 3; ++i )
    {
		int j = 0;
		while ( i+(j*3) < NUM_TEAMS )
		{
			char name[64];
			int w = m_w/3 - 8;
			sprintf( name, "T%d", i + (j*3));
			if ( i + (j*3) < NUM_TEAMS ) {
				TeamButton *tb = new TeamButton(i+(j*3));
				tb->SetShortProperties(name, 10 + (i*w) + (i*2), y + (j*buttonPitch), w);
				RegisterButton(tb);
				if ( building->m_id.GetTeamId() == i+(j*3) ) { tb->m_highlightedThisFrame = true; }
			}
			rows = max(rows,j);
			j++;
		}
    }

	y = y + 7 + (rows*buttonPitch);

	CreateValueControl(LANGUAGEPHRASE("editor_dynamic"), InputField::TypeChar, &building->m_dynamic, y += buttonPitch,
					   1.0f, 0, 1);

	CreateValueControl(LANGUAGEPHRASE("editor_isglobal"), InputField::TypeChar, &building->m_isGlobal, y += buttonPitch,
					   1.0f, 0, 1);

    if (building->m_type == Building::TypeFactory)
	{
		InputField *intExtra = new InputField();
		intExtra->SetShortProperties(LANGUAGEPHRASE("editor_spirits"), 10, y += buttonPitch, m_w-20);
		Factory *factory = (Factory*)building;
		intExtra->RegisterInt(&factory->m_initialCapacity);
		RegisterButton(intExtra);
	}
    else if (building->m_type == Building::TypeTrunkPort)
    {
        InputField *inExtra = new InputField();
        inExtra->SetShortProperties(LANGUAGEPHRASE("editor_targetlocation"), 10, y += buttonPitch, m_w-20);
        TrunkPort *port = (TrunkPort *) building;
        inExtra->RegisterInt( &port->m_targetLocationId );
        RegisterButton(inExtra);
    }
    else if (building->m_type == Building::TypeLaserFence)
    {
        LaserFence *fence = (LaserFence *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_scale"), InputField::TypeFloat, &fence->m_scale, y+=buttonPitch, 0.01f, 0.0f, 100.0f );

       DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( LANGUAGEPHRASE("editor_mode"), 10, y+=buttonPitch, m_w-20 );
        menu->AddOption( LANGUAGEPHRASE("editor_disabled") );
        menu->AddOption( LANGUAGEPHRASE("editor_enabling") );
        menu->AddOption( LANGUAGEPHRASE("editor_enabled") );
        menu->AddOption( LANGUAGEPHRASE("editor_disabling") );
		menu->AddOption( LANGUAGEPHRASE("editor_neveron") );
        menu->RegisterInt( &fence->m_mode );
        RegisterButton( menu );
    }
    else if(building->m_type == Building::TypeAntHill)
    {
        AntHill *antHill = (AntHill *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_numants"), InputField::TypeInt, &antHill->m_numAntsInside, y+=buttonPitch, 1, 0, 1000 );
    }
    else if(building->m_type == Building::TypeSafeArea)
    {
        SafeArea *safeArea = (SafeArea *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_size"), InputField::TypeFloat, &safeArea->m_size, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_capacity"), InputField::TypeInt, &safeArea->m_entitiesRequired, y+=buttonPitch, 1, 0, 10000 );

        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( LANGUAGEPHRASE("editor_entitytype"), 10, y+=buttonPitch, m_w-20 );
        for( int i = 0; i < Entity::NumEntityTypes; ++i)
        {
            menu->AddOption( Entity::GetTypeNameTranslated(i), i );
        }
        menu->RegisterInt( &safeArea->m_entityTypeRequired );
        RegisterButton( menu );
    }
    else if(building->m_type == Building::TypeTrackStart)
    {
        TrackStart *trackStart = (TrackStart *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_toggledby"), InputField::TypeInt, &trackStart->m_reqBuildingId, y+=buttonPitch, 1.0f, -1.0f, 1000.0f );
    }
    else if(building->m_type == Building::TypeTrackEnd)
    {
        TrackEnd *trackEnd = (TrackEnd *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_toggledby"), InputField::TypeInt, &trackEnd->m_reqBuildingId, y+=buttonPitch, 1.0f, -1.0f, 1000.0f );
    }
    else if(building->m_type == Building::TypePylonStart)
    {
        PylonStart *pylonStart = (PylonStart *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_toggledby"), InputField::TypeInt, &pylonStart->m_reqBuildingId, y+=buttonPitch, 1.0f, -1.0f, 1000.0f );
    }
    else if(building->m_type == Building::TypeResearchItem )
    {
        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( LANGUAGEPHRASE("editor_research"), 10, y+=buttonPitch, m_w-20 );
        for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
        {
            menu->AddOption( GlobalResearch::GetTypeNameTranslated( i ), i );
        }
        menu->RegisterInt( &((ResearchItem *)building)->m_researchType );
        RegisterButton( menu );
        CreateValueControl( LANGUAGEPHRASE("editor_level"), InputField::TypeInt, &((ResearchItem *)building)->m_level, y+=buttonPitch, 1, 0, 4 );
    }
    else if(building->m_type == Building::TypeResearchCrate )
    {
        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( LANGUAGEPHRASE("editor_research"), 10, y+=buttonPitch, m_w-20 );
        for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
        {
            menu->AddOption( GlobalResearch::GetTypeNameTranslated( i ), i );
        }
        menu->RegisterInt( &((ResearchCrate *)building)->m_researchType );
        RegisterButton( menu );
        CreateValueControl( LANGUAGEPHRASE("editor_level"), InputField::TypeInt, &((ResearchCrate *)building)->m_level, y+=buttonPitch, 1, 0, 4 );
    }
    else if( building->m_type == Building::TypeTriffid )
    {
        Triffid *triffid = (Triffid *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_size"), InputField::TypeFloat, &triffid->m_size, y+=buttonPitch, 0.1f, 0.0f, 50.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_pitch"), InputField::TypeFloat, &triffid->m_pitch, y+=buttonPitch, 0.1f, -M_PI, M_PI );
        CreateValueControl( LANGUAGEPHRASE("editor_force"), InputField::TypeFloat, &triffid->m_force, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_variance"), InputField::TypeFloat, &triffid->m_variance, y+=buttonPitch, 0.01f, 0.0f, M_PI );
        CreateValueControl( LANGUAGEPHRASE("editor_reload"), InputField::TypeFloat, &triffid->m_reloadTime, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );

        for( int i = 0; i < Triffid::NumSpawnTypes; ++i )
        {
            CreateValueControl( Triffid::GetSpawnNameTranslated(i), InputField::TypeChar, &triffid->m_spawn[i], y+=buttonPitch, 1.0f, 0.0f, 1.0f );
        }

        CreateValueControl( LANGUAGEPHRASE("editor_usetrigger"), InputField::TypeChar, &triffid->m_useTrigger, y+=buttonPitch, 1, 0, 1 );
        CreateValueControl( LANGUAGEPHRASE("editor_triggerX"), InputField::TypeFloat, &triffid->m_triggerLocation.x, y+=buttonPitch, 1.0f, -10000.0f, 10000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_triggerZ"), InputField::TypeFloat, &triffid->m_triggerLocation.z, y+=buttonPitch, 1.0f, -10000.0f, 10000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_triggerrad"), InputField::TypeFloat, &triffid->m_triggerRadius, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );

    }
    else if( building->m_type == Building::TypeBlueprintRelay )
    {
        BlueprintRelay *relay = (BlueprintRelay *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_altitude"), InputField::TypeFloat, &relay->m_altitude, y+=buttonPitch, 1.0f, 0.0f, 1000.0f );
    }
    else if( building->m_type == Building::TypeBlueprintConsole )
    {
        BlueprintConsole *console = (BlueprintConsole *) building;
        CreateValueControl( LANGUAGEPHRASE("editor_segment"), InputField::TypeInt, &console->m_segment, y+=buttonPitch, 1, 0, 3 );
    }
    else if( building->m_type == Building::TypeAISpawnPoint )
    {
        AISpawnPoint *spawn = (AISpawnPoint *) building;
        DropDownMenu *menu = new DropDownMenu();
        menu->SetShortProperties( LANGUAGEPHRASE("editor_entitytype"), 10, y+=buttonPitch, m_w-20 );
        for( int i = 0; i < Entity::NumEntityTypes; ++i )
        {
            menu->AddOption( Entity::GetTypeNameTranslated(i), i );
        }
        menu->RegisterInt( &spawn->m_entityType );
        RegisterButton(menu);

        CreateValueControl( LANGUAGEPHRASE("editor_count"), InputField::TypeInt, &spawn->m_count, y+=buttonPitch, 1, 0, 1000 );
        CreateValueControl( LANGUAGEPHRASE("editor_period"), InputField::TypeInt, &spawn->m_period, y+=buttonPitch, 1, 0, 1000 );
        CreateValueControl( LANGUAGEPHRASE("editor_spawnlimit"), InputField::TypeInt, &spawn->m_spawnLimit, y+=buttonPitch, 1, 0, 1000 );
    }
    else if( building->m_type == Building::TypeSpawnPopulationLock )
    {
        SpawnPopulationLock *lock = (SpawnPopulationLock *) building;

        CreateValueControl( LANGUAGEPHRASE("editor_searchradius"), InputField::TypeFloat, &lock->m_searchRadius, y+=buttonPitch, 1.0f, 0.0f, 10000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_maxpopulation"), InputField::TypeInt, &lock->m_maxPopulation, y+=buttonPitch, 1, 0, 10000 );
    }
    else if( building->m_type == Building::TypeSpawnLink ||
             building->m_type == Building::TypeSpawnPointMaster ||
             building->m_type == Building::TypeSpawnPoint ||
             building->m_type == Building::TypeSpawnPointRandom )
    {
        class ClearLinksButton: public DarwiniaButton
        {
        public:
            int m_buildingId;
            void MouseUp()
            {
                SpawnBuilding *building = (SpawnBuilding *) g_app->m_location->GetBuilding( m_buildingId );
                building->ClearLinks();
            }
        };

        ClearLinksButton *button = new ClearLinksButton();
        button->SetShortProperties( LANGUAGEPHRASE("editor_clearlinks"), 10, y+=buttonPitch, m_w-20 );
        button->m_buildingId = building->m_id.GetUniqueId();
        RegisterButton( button );
    }
    else if( building->m_type == Building::TypeScriptTrigger )
    {
        ScriptTrigger *trigger = (ScriptTrigger *) building;

        CreateValueControl( LANGUAGEPHRASE("editor_range"), InputField::TypeFloat, &trigger->m_range, y+=buttonPitch, 0.5f, 0.0f, 1000.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_script"), InputField::TypeString, trigger->m_scriptFilename, y+=buttonPitch, 0,0,0 );

        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( LANGUAGEPHRASE("editor_entitytype"), 10, y+=buttonPitch, m_w-20 );
        menu->AddOption( LANGUAGEPHRASE("editor_always"), SCRIPTRIGGER_RUNALWAYS );
        menu->AddOption( LANGUAGEPHRASE("editor_never"), SCRIPTRIGGER_RUNNEVER );
        menu->AddOption( LANGUAGEPHRASE("editor_cameraenter"), SCRIPTRIGGER_RUNCAMENTER );
        menu->AddOption( LANGUAGEPHRASE("editor_cameraview"), SCRIPTRIGGER_RUNCAMVIEW );
        for( int i = 0; i < Entity::NumEntityTypes; ++i)
        {
            menu->AddOption( Entity::GetTypeNameTranslated(i), i );
        }
        menu->RegisterInt( &trigger->m_entityType);
        RegisterButton( menu );
    }
    else if( building->m_type == Building::TypeStaticShape )
    {
        StaticShape *staticShape = (StaticShape *) building;

        CreateValueControl( LANGUAGEPHRASE("editor_scale"), InputField::TypeFloat, &staticShape->m_scale, y+=buttonPitch, 0.1f, 0.0f, 100.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_shape"), InputField::TypeString, &staticShape->m_shapeName, y+=buttonPitch, 0,0,0 );
    }
    else if( building->m_type == Building::TypeIncubator )
    {
        CreateValueControl( LANGUAGEPHRASE("editor_spirits"),   InputField::TypeInt, &((Incubator *) building)->m_numStartingSpirits, y+=buttonPitch, 1, 0, 1000 );
        CreateValueControl( LANGUAGEPHRASE("editor_damaged"),   InputField::TypeInt, &((Incubator *) building)->m_renderDamaged, y+=buttonPitch, 1, 0, 1);
        CreateValueControl( LANGUAGEPHRASE("editor_teamspawn"), InputField::TypeInt, &((Incubator *) building)->m_teamSpawner, y+=buttonPitch, 1, 0, 1);
    }
    else if( building->m_type == Building::TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;

        CreateValueControl( LANGUAGEPHRASE("editor_fuel"), InputField::TypeFloat, &rocket->m_fuel, y+=buttonPitch, 0.1f, 0.0f, 100.0f );
        CreateValueControl( LANGUAGEPHRASE("editor_passengers"), InputField::TypeInt, &rocket->m_passengers, y+=buttonPitch, 1, 0, 100 );
        CreateValueControl( LANGUAGEPHRASE("editor_spawnport"), InputField::TypeInt, &rocket->m_spawnBuildingId, y+=buttonPitch, 1, 0, 9999 );
    }
    else if( building->m_type == Building::TypeDynamicHub )
    {
        DynamicHub *hub = (DynamicHub *)building;

        CreateValueControl( LANGUAGEPHRASE("editor_shape"), InputField::TypeString, &hub->m_shapeName, y+=buttonPitch, 0,0,0 );
        CreateValueControl( LANGUAGEPHRASE("editor_requiredscore"), InputField::TypeInt, &hub->m_requiredScore, y+=buttonPitch, 1, 0, 100000 );
        CreateValueControl( LANGUAGEPHRASE("editor_minlinks"), InputField::TypeInt, &hub->m_minActiveLinks, y+=buttonPitch, 1, 0, 100 );
    }
    else if( building->m_type == Building::TypeDynamicNode )
    {
        DynamicNode *node = (DynamicNode *)building;

        CreateValueControl( LANGUAGEPHRASE("editor_shape"), InputField::TypeString, &node->m_shapeName, y+=buttonPitch, 0,0,0 );
        CreateValueControl( LANGUAGEPHRASE("editor_pointspersec"), InputField::TypeInt, &node->m_scoreValue, y+=buttonPitch, 1, 0, 1000 );
    }
	else if( building->m_type == Building::TypeFenceSwitch )
	{
		FenceSwitch *fs = (FenceSwitch *)building;

		CreateValueControl( LANGUAGEPHRASE("editor_switchonce"), InputField::TypeInt, &fs->m_lockable, y+=buttonPitch, 0, 1, 0 );
        CreateValueControl( LANGUAGEPHRASE("editor_script"), InputField::TypeString, &fs->m_script, y+=buttonPitch, 0, 0, 0 );
	}

}


void BuildingEditWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

	Building *building = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
	DarwiniaDebugAssert(building);

    g_editorFont.SetRenderShadow(true);
    glColor4ub( 255, 255, 150, 30 );
    g_editorFont.DrawText2D( m_x + m_w - 43, m_y + 8, DEF_FONT_SIZE * 1.1f, "%d", building->m_id.GetUniqueId() );
    g_editorFont.DrawText2D( m_x + m_w - 43, m_y + 8, DEF_FONT_SIZE * 1.1f, "%d", building->m_id.GetUniqueId() );
    g_editorFont.SetRenderShadow(false);
}


// ****************************************************************************
// Class NewBuildingButton
// ****************************************************************************

class NewBuildingButton : public DarwiniaButton
{
public:
    void MouseUp()
    {
	    Vector3 rayStart;
	    Vector3 rayDir;
	    g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2,
									 g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
        Vector3 _pos;
        g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

        BuildingsCreateWindow *bcw = (BuildingsCreateWindow *) m_parent;
        Building *building = Building::CreateBuilding( bcw->m_buildingType );
        if( building )
        {
            building->m_pos = _pos;
            building->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );
            g_app->m_location->m_levelFile->m_buildings.PutData( building );
        }
    }
};


// ****************************************************************************
// Class BuildingsCreateWindow
// ****************************************************************************

BuildingsCreateWindow::BuildingsCreateWindow( char *_name )
:	DarwiniaWindow( _name ),
    m_buildingType(0)
{
}


BuildingsCreateWindow::~BuildingsCreateWindow()
{
	g_app->m_locationEditor->RequestMode(LocationEditor::ModeNone);
	EclRemoveWindow(LANGUAGEPHRASE("editor_buildingid"));
}


void BuildingsCreateWindow::Create()
{
	DarwiniaWindow::Create();

	int y = 25;
	int ySpacing = 18;

    DropDownMenu *menu = new DropDownMenu(true);
    menu->SetShortProperties( LANGUAGEPHRASE("editor_buildingtype"), 10, 25, m_w - 20 );
    menu->RegisterInt( &m_buildingType );
    RegisterButton( menu );
    for (int i = Building::TypeInvalid + 1; i < Building::NumBuildingTypes; ++i)
    {
        menu->AddOption( Building::GetTypeNameTranslated(i), i );
    }

    NewBuildingButton *b = new NewBuildingButton();
    b->SetShortProperties( LANGUAGEPHRASE("editor_createbuilding"), 10, 45, m_w - 20 );
    RegisterButton( b );

//	for (int i = Building::TypeInvalid + 1; i < Building::NumBuildingTypes; ++i)
//	{
//	    NewBuildingButton *n = new NewBuildingButton(i);
//        char *name = Building::GetTypeName( i );
//        n->SetShortProperties( name, 10, y, m_w - 20 );
//		RegisterButton( n );
//		y += ySpacing;
//	}
}

#endif // LOCATION_EDITOR
