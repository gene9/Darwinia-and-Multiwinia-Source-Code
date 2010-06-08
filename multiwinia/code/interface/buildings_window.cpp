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
#include "worldobject/multiwiniazone.h"
#include "worldobject/chess.h"
#include "worldobject/radardish.h"
#include "worldobject/portal.h"
#include "worldobject/pulsebomb.h"
#include "worldobject/restrictionzone.h"
#include "worldobject/gunturret.h"
#include "worldobject/jumppad.h"
#include "worldobject/aiobjective.h"

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
            if( b )
            {
                g_editorFont.DrawText2DRight( realX + m_w - 10, realY + 10, 14, "%d", b->GetBuildingLink() );
            }
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
    	    EclRemoveWindow("editor_buildingid");
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
            RGBAColour col = g_app->m_location->m_teams[ m_teamId ]->m_colour;
            glColor3ubv( col.GetData() );
        }

        glBegin( GL_QUADS );
            glVertex2i( realX + 20, realY + 4 );
            glVertex2i( realX + 30, realY + 4 );
            glVertex2i( realX + 30, realY + 12 );
            glVertex2i( realX + 20, realY + 12 );
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
        AppDebugAssert(building);

        Building *newBuilding = Building::CreateBuilding( building->m_type );   
        newBuilding->Initialise( building );
        newBuilding->SetDetail( 1 );
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
	AppDebugAssert(building);

	int buttonPitch = 18;
	int y = 6;
    
    ToolButton *mb = new ToolButton(LocationEditor::ToolMove);
	mb->SetShortProperties("editor_move", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_move"));
	RegisterButton(mb);

    ToolButton *rb = new ToolButton(LocationEditor::ToolRotate);
	rb->SetShortProperties("editor_rotate", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_rotate"));
	RegisterButton(rb);

    CloneBuildingButton *clone = new CloneBuildingButton();
    clone->SetShortProperties("editor_clone", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_clone"));
    RegisterButton(clone);

    DeleteBuildingButton *db = new DeleteBuildingButton();
    db->SetShortProperties("editor_delete", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_delete"));
    RegisterButton(db);

    ToolButton *lb = new ToolButton(LocationEditor::ToolLink);
    lb->SetShortProperties("editor_link", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_link"));    
    RegisterButton(lb);

	y += buttonPitch;

    for( int i = -1; i < 4; ++i )
    {
        char name[256];
        int w = m_w/5 - 5;
        sprintf( name, "T%d",i);
        TeamButton *tb = new TeamButton(i);
        tb->SetShortProperties(name, 50 + (float)i*((float)w + 1.0), y, w - 2,-1,  UnicodeString(name));
        RegisterButton(tb);
    }

	CreateValueControl("editor_dynamic", &building->m_dynamic, y += buttonPitch, 
					   1.0, 0, 1);

	CreateValueControl("editor_isglobal", &building->m_isGlobal, y += buttonPitch, 
					   1.0, 0, 1);

    if (building->m_type == Building::TypeFactory)
	{
		InputField *intExtra = new InputField();
		intExtra->SetShortProperties("editor_spirits", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_spirits") );
		Factory *factory = (Factory*)building;
		intExtra->RegisterInt(&factory->m_initialCapacity);
		RegisterButton(intExtra);
	}
    else if (building->m_type == Building::TypeTrunkPort)
    {
        InputField *inExtra = new InputField();
        inExtra->SetShortProperties("editor_targetlocation", 10, y += buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_targetlocation"));
        TrunkPort *port = (TrunkPort *) building;
        inExtra->RegisterInt( &port->m_targetLocationId );
        RegisterButton(inExtra);
    }
    else if (building->m_type == Building::TypeLaserFence)
    {
        LaserFence *fence = (LaserFence *) building;
        CreateValueControl( "editor_scale", &fence->m_scale, y+=buttonPitch, 0.01, 0.0, 100.0 );
        
       DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( "editor_mode", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_mode") );
        menu->AddOption( "editor_disabled" );
        menu->AddOption( "editor_enabling" );
        menu->AddOption( "editor_enabled" );
        menu->AddOption( "editor_disabling" );
		menu->AddOption( "editor_neveron" );
        menu->AddOption( "editor_alwayson" );
        menu->RegisterInt( &fence->m_mode );
        RegisterButton( menu );
    }
    else if(building->m_type == Building::TypeAntHill)
    {
        AntHill *antHill = (AntHill *) building;
        CreateValueControl( "editor_numants", &antHill->m_numAntsInside, y+=buttonPitch, 1, 0, 1000 );
    }
    else if(building->m_type == Building::TypeSafeArea)
    {
        SafeArea *safeArea = (SafeArea *) building;
        CreateValueControl( "editor_size", &safeArea->m_size, y+=buttonPitch, 1.0, 0.0, 1000.0 );
        CreateValueControl( "editor_capacity", &safeArea->m_entitiesRequired, y+=buttonPitch, 1, 0, 10000 );
        
        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( "editor_entitytype", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_entitytype") );
        for( int i = 0; i < Entity::NumEntityTypes; ++i)
        {
            menu->AddOption( Entity::GetTypeName(i), i );
        }
        menu->RegisterInt( &safeArea->m_entityTypeRequired );
        RegisterButton( menu );
    }
    else if(building->m_type == Building::TypeTrackStart)
    {
        TrackStart *trackStart = (TrackStart *) building;
        CreateValueControl( "editor_toggledby", &trackStart->m_reqBuildingId, y+=buttonPitch, 1.0, -1.0, 1000.0 );
    }
    else if(building->m_type == Building::TypeTrackEnd)
    {
        TrackEnd *trackEnd = (TrackEnd *) building;
        CreateValueControl( "editor_toggledby", &trackEnd->m_reqBuildingId, y+=buttonPitch, 1.0, -1.0, 1000.0 );
    }
    else if(building->m_type == Building::TypePylonStart)
    {
        PylonStart *pylonStart = (PylonStart *) building;
        CreateValueControl( "editor_toggledby", &pylonStart->m_reqBuildingId, y+=buttonPitch, 1.0, -1.0, 1000.0 );
    }
    else if(building->m_type == Building::TypeResearchItem )
    {
        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( "editor_research", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_research") );
        for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
        {
            menu->AddOption( GlobalResearch::GetTypeName( i ), i );
        }
        menu->RegisterInt( &((ResearchItem *)building)->m_researchType );        
        RegisterButton( menu );
        CreateValueControl( "editor_level", &((ResearchItem *)building)->m_level, y+=buttonPitch, 1, 0, 4 );
    }
    else if( building->m_type == Building::TypeTriffid )
    {
        Triffid *triffid = (Triffid *) building;
        CreateValueControl( "editor_size", &triffid->m_size, y+=buttonPitch, 0.1, 0.0, 50.0 );
        CreateValueControl( "editor_pitch", &triffid->m_pitch, y+=buttonPitch, 0.1, -M_PI, M_PI );
        CreateValueControl( "editor_force", &triffid->m_force, y+=buttonPitch, 1.0, 0.0, 1000.0 );
        CreateValueControl( "editor_variance", &triffid->m_variance, y+=buttonPitch, 0.01, 0.0, M_PI );
        CreateValueControl( "editor_reload", &triffid->m_reloadTime, y+=buttonPitch, 1.0, 0.0, 1000.0 );

        for( int i = 0; i < Triffid::NumSpawnTypes; ++i )
        {
            CreateValueControl( (char*)Triffid::GetSpawnName(i), &triffid->m_spawn[i], y+=buttonPitch, 1.0, 0.0, 1.0 );
        }

        CreateValueControl( "editor_usetrigger", &triffid->m_useTrigger, y+=buttonPitch, 1, 0, 1 );
        CreateValueControl( "editor_triggerX", &triffid->m_triggerLocation.x, y+=buttonPitch, 1.0, -10000.0, 10000.0 );
        CreateValueControl( "editor_triggerZ", &triffid->m_triggerLocation.z, y+=buttonPitch, 1.0, -10000.0, 10000.0 );
        CreateValueControl( "editor_triggerrad", &triffid->m_triggerRadius, y+=buttonPitch, 1.0, 0.0, 1000.0 );

    }
    else if( building->m_type == Building::TypeBlueprintRelay )
    {
        BlueprintRelay *relay = (BlueprintRelay *) building;
        CreateValueControl( "editor_altitude", &relay->m_altitude, y+=buttonPitch, 1.0, 0.0, 1000.0 );
    }
    else if( building->m_type == Building::TypeBlueprintConsole )
    {
        BlueprintConsole *console = (BlueprintConsole *) building;
        CreateValueControl( "editor_segment", &console->m_segment, y+=buttonPitch, 1, 0, 3 );
    }
    else if( building->m_type == Building::TypeAISpawnPoint )
    {
        AISpawnPoint *spawn = (AISpawnPoint *) building;
        DropDownMenu *menu = new DropDownMenu();
        menu->SetShortProperties( "editor_entitytype", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_entitytype") );
        for( int i = 0; i < Entity::NumEntityTypes; ++i )
        {
            menu->AddOption( Entity::GetTypeName(i), i );
        }
        menu->RegisterInt( &spawn->m_entityType );
        RegisterButton(menu);

        CreateValueControl( "editor_count", &spawn->m_count, y+=buttonPitch, 1, 0, 1000 );
        CreateValueControl( "editor_period", &spawn->m_period, y+=buttonPitch, 1, 0, 1000 );
        CreateValueControl( "editor_spawnlimit", &spawn->m_spawnLimit, y+=buttonPitch, 1, 0, 1000 );
    }
    else if( building->m_type == Building::TypeSpawnPopulationLock )
    {
        SpawnPopulationLock *lock = (SpawnPopulationLock *) building;

        CreateValueControl( "editor_searchradius", &lock->m_searchRadius, y+=buttonPitch, 1.0, 0.0, 10000.0 );
        CreateValueControl( "editor_maxpopulation", &lock->m_maxPopulation, y+=buttonPitch, 1, 0, 10000 );
    }
    else if( building->m_type == Building::TypeSpawnLink ||
             building->m_type == Building::TypeSpawnPointMaster ||
             building->m_type == Building::TypeSpawnPoint )
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
        button->SetShortProperties( "editor_clearlinks", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_clearlinks"));
        button->m_buildingId = building->m_id.GetUniqueId();
        RegisterButton( button );
    }
    else if( building->m_type == Building::TypeScriptTrigger )
    {
        ScriptTrigger *trigger = (ScriptTrigger *) building;

        CreateValueControl( "editor_range", &trigger->m_range, y+=buttonPitch, 0.5, 0.0, 1000.0 );
        CreateValueControl( "editor_script", trigger->m_scriptFilename, y+=buttonPitch, 0,0, sizeof(trigger->m_scriptFilename) );

        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( "editor_entitytype", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_entitytype") );
        menu->AddOption( "editor_always", SCRIPTRIGGER_RUNALWAYS );
        menu->AddOption( "editor_never", SCRIPTRIGGER_RUNNEVER );
        menu->AddOption( "editor_cameraenter", SCRIPTRIGGER_RUNCAMENTER );
        menu->AddOption( "editor_cameraview", SCRIPTRIGGER_RUNCAMVIEW );
        for( int i = 0; i < Entity::NumEntityTypes; ++i)
        {
            menu->AddOption( Entity::GetTypeName(i), i );
        }
        menu->RegisterInt( &trigger->m_entityType);
        RegisterButton( menu );
    }
    else if( building->m_type == Building::TypeStaticShape )
    {
        StaticShape *staticShape = (StaticShape *) building;

        CreateValueControl( "editor_scale", &staticShape->m_scale, y+=buttonPitch, 0.1, 0.0, 100.0 );
        CreateValueControl( "editor_shape", staticShape->m_shapeName, y+=buttonPitch, 0,0, sizeof(staticShape->m_shapeName) );
    }
    else if( building->m_type == Building::TypeIncubator ||
             building->m_type == Building::TypeCloneLab)
    {
        CreateValueControl( "editor_spirits", &((Incubator *) building)->m_numStartingSpirits, y+=buttonPitch, 1, 0, 1000 );
    }
    else if( building->m_type == Building::TypeEscapeRocket )
    {
        EscapeRocket *rocket = (EscapeRocket *) building;

        CreateValueControl( "editor_fuel", &rocket->m_fuel, y+=buttonPitch, 0.1, 0.0, 100.0 );
        CreateValueControl( "editor_passengers", &rocket->m_passengers, y+=buttonPitch, 1, 0, 100 );
        CreateValueControl( "editor_spawnport", &rocket->m_spawnBuildingId, y+=buttonPitch, 1, 0, 9999 );
    }
    else if( building->m_type == Building::TypeDynamicHub )
    {
        DynamicHub *hub = (DynamicHub *)building;

        CreateValueControl( "editor_shape", hub->m_shapeName, y+=buttonPitch, 0,0, sizeof(hub->m_shapeName) );
        CreateValueControl( "editor_requiredscore", &hub->m_requiredScore, y+=buttonPitch, 1, 0, 100000 );
        CreateValueControl( "editor_minlinks", &hub->m_minActiveLinks, y+=buttonPitch, 1, 0, 100 );
    }
    else if( building->m_type == Building::TypeDynamicNode )
    {
        DynamicNode *node = (DynamicNode *)building;

        CreateValueControl( "editor_shape", node->m_shapeName, y+=buttonPitch, 0,0, sizeof(node->m_shapeName) );
        CreateValueControl( "editor_pointspersec", &node->m_scoreValue, y+=buttonPitch, 1, 0, 1000 );
    }
	else if( building->m_type == Building::TypeFenceSwitch )
	{
		FenceSwitch *fs = (FenceSwitch *)building;

		CreateValueControl( "editor_switchonce", &fs->m_lockable, y+=buttonPitch, 0, 1, 0 );
        CreateValueControl( "editor_script", fs->m_script, y+=buttonPitch, 0, 0, sizeof(fs->m_script) );
	}
    else if( building->m_type == Building::TypeMultiwiniaZone )
    {
        MultiwiniaZone *zone = (MultiwiniaZone *) building;
        CreateValueControl( "editor_size", &zone->m_size, y+=buttonPitch, 1.0, 0.0, 1000.0 );
    }
    else if( building->m_type == Building::TypeChessBase )
    {
        ChessBase *chessBase = (ChessBase *) building;
        CreateValueControl( "editor_width", &chessBase->m_width, y+=buttonPitch, 1.0, 0.0, 99999.9 );
        CreateValueControl( "editor_depth", &chessBase->m_depth, y+=buttonPitch, 1.0, 0.0, 99999.9 );
    }
    else if( building->m_type == Building::TypeAITarget )
    {
        AITarget *target = (AITarget *)building;
        CreateValueControl( "editor_priority", &target->m_priorityModifier, y+=buttonPitch, 0.05, -1.0, 1.0 );
        CreateValueControl( "editor_checkcliffs", &target->m_checkCliffs, y+=buttonPitch, 1, 0, 1 );
    }
    else if( building->m_type == Building::TypeRadarDish )
    {
        class ClearRadarLinksButton: public DarwiniaButton
        {
        public:
            int m_buildingId;
            void MouseUp()
            {
                RadarDish *building = (RadarDish *) g_app->m_location->GetBuilding( m_buildingId );
                building->ClearLinks();
            }
        };

        RadarDish *radar = (RadarDish *)building;

        ClearRadarLinksButton *button = new ClearRadarLinksButton();
        button->SetShortProperties( "editor_clearlinks", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_clearlinks") );
        button->m_buildingId = building->m_id.GetUniqueId();
        RegisterButton( button );

        CreateValueControl( "editor_forceconnection", &radar->m_forceConnection, y+=buttonPitch, 1, 0, 1 );
        CreateValueControl( "editor_autoconnect", &radar->m_autoConnectAtStart, y+=buttonPitch, 1, 0, 1 );
    }
    else if( building->m_type == Building::TypePortal )
    {
        class ClearLinksButton: public DarwiniaButton
        {
        public:
            int m_buildingId;
            void MouseUp()
            {
                Portal *building = (Portal *) g_app->m_location->GetBuilding( m_buildingId );
//                building->ClearLinks();
            }
        };

        ClearLinksButton *button = new ClearLinksButton();
        button->SetShortProperties( "editor_clearlinks", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_clearlinks") );
        button->m_buildingId = building->m_id.GetUniqueId();
        RegisterButton( button );

        Portal *portal = (Portal *)building;
//        CreateValueControl( "editor_master", &portal->m_masterPortal, y+=buttonPitch, 1, 0, 1 );
    }
    else if( building->m_type == Building::TypePulseBomb )
    {
        PulseBomb *pulsebomb = (PulseBomb *)building;
        CreateValueControl( "editor_timer", &pulsebomb->m_startTime, y+=buttonPitch, 60, 300, 1800 );
    }
    else if( building->m_type == Building::TypeRestrictionZone )
    {
        RestrictionZone *rz = (RestrictionZone *)building;
        CreateValueControl( "editor_size" , &rz->m_size, y+=buttonPitch, 1.0, 0.0, 10000.0 );
        CreateValueControl( "editor_blockpowerups", &rz->m_blockPowerups, y+=buttonPitch, 1.0f, 0.0f, 1.0f );
    }
    else if( building->m_type == Building::TypeGunTurret )
    {
        GunTurret *turret = ((GunTurret *)building);
        DropDownMenu *menu = new DropDownMenu(true);
        menu->SetShortProperties( "editor_turrettype", 10, y+=buttonPitch, m_w-20, -1, LANGUAGEPHRASE("editor_turrettype") );
        menu->AddOption( LANGUAGEPHRASE("buildingname_gunturret"), GunTurret::GunTurretTypeStandard );
        menu->AddOption( LANGUAGEPHRASE("buildingname_rocketturret"), GunTurret::GunTurretTypeRocket );
        menu->AddOption( LANGUAGEPHRASE("buildingname_grenadeturret"), GunTurret::GunTurretTypeMortar );
        menu->AddOption( LANGUAGEPHRASE("buildingname_flameturret"), GunTurret::GunTurretTypeFlame );
        menu->AddOption( LANGUAGEPHRASE("buildingname_subversionturret"), GunTurret::GunTurretTypeSubversion );
        menu->RegisterInt( &turret->m_state );
        RegisterButton( menu );

        CreateValueControl( "editor_triggerX", &turret->m_targetPos.x, y+=buttonPitch, 1.0, -10000.0, 10000.0 );
        CreateValueControl( "editor_triggerZ", &turret->m_targetPos.z, y+=buttonPitch, 1.0, -10000.0, 10000.0 );
        CreateValueControl( "editor_triggerrad", &turret->m_targetRadius, y+=buttonPitch, 1.0, 0.0, 1000.0 );
        CreateValueControl( "editor_force", &turret->m_targetForce, y+=buttonPitch, 1.0, 0.0, 1000.0 );
    }
    else if( building->m_type == Building::TypeJumpPad )
    {
        JumpPad *pad = (JumpPad *)building;
        CreateValueControl( "editor_pitch", &pad->m_angle, y+=buttonPitch, 0.1, -M_PI, M_PI );
        CreateValueControl( "editor_force", &pad->m_force, y+=buttonPitch, 1.0, 0.0, 1000.0 );
    }
    else if( building->m_type == Building::TypeAIObjectiveMarker )
    {
        AIObjectiveMarker *marker = (AIObjectiveMarker *)building;
        CreateValueControl( "editor_searchradius", &marker->m_scanRange, y+=buttonPitch, 1.0, 1.0, 500.0 );
        CreateValueControl( "editor_armourtarget", &marker->m_armourObjective, y+=buttonPitch, 1, 0, 1 );
        CreateValueControl( "editor_pickuponly", &marker->m_pickupOnly, y+=buttonPitch, 1, -1, 1 );
    }
}


void BuildingEditWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

	Building *building = g_app->m_location->GetBuilding(g_app->m_locationEditor->m_selectionId);
    if( !building ) return;
	AppDebugAssert(building);

    g_editorFont.SetRenderShadow(true);
    glColor4ub( 255, 255, 150, 30 );
    g_editorFont.DrawText2D( m_x + m_w - 43, m_y + 8, DEF_FONT_SIZE * 1.1, "%d", building->m_id.GetUniqueId() );
    g_editorFont.DrawText2D( m_x + m_w - 43, m_y + 8, DEF_FONT_SIZE * 1.1, "%d", building->m_id.GetUniqueId() );
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
            building->m_id.SetTeamId(255);
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
	EclRemoveWindow("editor_buildingid");
}


void BuildingsCreateWindow::Create()
{
	DarwiniaWindow::Create();
	
	int y = 25;
	int ySpacing = 18;

    DropDownMenu *menu = new DropDownMenu(true);
    menu->SetShortProperties( "editor_buildingtype", 10, 25, m_w - 20, -1, LANGUAGEPHRASE("editor_buildingtype") );
    menu->RegisterInt( &m_buildingType );
    RegisterButton( menu );
    for (int i = Building::TypeInvalid + 1; i < Building::NumBuildingTypes; ++i)
    {
        if( !Building::BuildingIsBlocked(i) )
        {
            menu->AddOption( Building::GetTypeName(i), i );
        }
    }

    NewBuildingButton *b = new NewBuildingButton();
    b->SetShortProperties( "editor_createbuilding", 10, 45, m_w - 20, -1, LANGUAGEPHRASE("editor_createbuilding") );
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
