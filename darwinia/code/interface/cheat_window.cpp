#include "lib/universal_include.h"
#include "lib/math_utils.h"
#include "lib/resource.h"

#include "interface/cheat_window.h"
#include "interface/debugmenu.h"

#include "app.h"
#include "global_world.h"
#include "globals.h"
#include "location.h"
#include "unit.h"
#include "team.h"
#include "camera.h"
#include "renderer.h"
#include "taskmanager.h"



#ifdef CHEATMENU_ENABLED

class KillAllEnemiesButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( g_app->m_location )
        {
            int myTeamId = g_app->m_globalWorld->m_myTeamId;
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( !g_app->m_location->IsFriend( myTeamId, t ) )
                {
                    Team *team = &g_app->m_location->m_teams[t];
                    
                    // Kill all UNITS
                    for( int u = 0; u < team->m_units.Size(); ++u )
                    {
                        if( team->m_units.ValidIndex(u) )
                        {
                            Unit *unit = team->m_units[u];
                            for( int e = 0; e < unit->m_entities.Size(); ++e )
                            {
                                if( unit->m_entities.ValidIndex(e) )
                                {
                                    Entity *entity = unit->m_entities[e];
                                    entity->ChangeHealth( entity->m_stats[Entity::StatHealth] * -2.0f );
                                }
                            }
                        }
                    }

                    // Kill all ENTITIES
                    for( int o = 0; o < team->m_others.Size(); ++o )
                    {
                        if( team->m_others.ValidIndex(o) )
                        {
                            Entity *entity = team->m_others[o];
                            entity->ChangeHealth( entity->m_stats[Entity::StatHealth] * -2.0f );
                        }
                    }
                }
            }

            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *building = g_app->m_location->m_buildings[i];
                    if( building->m_type == Building::TypeAntHill ||
                        building->m_type == Building::TypeTriffid )
                    {
                        building->Damage( -999 );
                    }
                }
            }
        }
    }
};


class SpawnDarwiniansButton : public DarwiniaButton
{
public:
    int m_teamId;

    void MouseUp()
    {
        if( g_app->m_location )
        {
	        Vector3 rayStart;
	        Vector3 rayDir;
	        g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
									     g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
            Vector3 _pos;
            g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

            g_app->m_location->SpawnEntities( _pos, m_teamId, -1, Entity::TypeDarwinian, 20, g_zeroVector, 30 );
        }        
    }
};


class SpawnTankButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( g_app->m_location )
        {
	        Vector3 rayStart;
	        Vector3 rayDir;
	        g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
									     g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
            Vector3 _pos;
            g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

            g_app->m_location->SpawnEntities( _pos, 2, -1, Entity::TypeArmour, 1, g_zeroVector, 0 );
        }        
    }
};


class SpawnViriiButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( g_app->m_location )
        {
	        Vector3 rayStart;
	        Vector3 rayDir;
	        g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
									     g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
            Vector3 _pos;
            g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

            g_app->m_location->SpawnEntities( _pos, 1, -1, Entity::TypeVirii, 20, g_zeroVector, 0, 1000.0f );
        }        
    }
};


class SpawnSpiritButton : public DarwiniaButton
{
    void MouseUp()
    {
        if( g_app->m_location )
        {
	        Vector3 rayStart;
	        Vector3 rayDir;
	        g_app->m_camera->GetClickRay(g_app->m_renderer->ScreenW()/2, 
									     g_app->m_renderer->ScreenH()/2, &rayStart, &rayDir);
            Vector3 _pos;
            g_app->m_location->m_landscape.RayHit( rayStart, rayDir, &_pos );

            for( int i = 0; i < 10; ++i )
            {
                Vector3 spiritPos = _pos + Vector3( syncsfrand(20.0f), 0.0f, syncsfrand(20.0f) );
                g_app->m_location->SpawnSpirit( spiritPos, g_zeroVector, 2, WorldObjectId() );
            }
        }
    }
};


class AllowArbitraryPlacementButton : public DarwiniaButton
{
    void MouseUp()
    {
        g_app->m_taskManager->m_verifyTargetting = false;
    }
};


class EnableGeneratorAndMineButton : public DarwiniaButton
{
    void MouseUp()
    {
        int generatorLocationId = g_app->m_globalWorld->GetLocationId( "generator" );
        int mineLocationId = g_app->m_globalWorld->GetLocationId( "mine" );

        for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
        {
            if( g_app->m_globalWorld->m_buildings.ValidIndex(i) )
            {
                GlobalBuilding *gb = g_app->m_globalWorld->m_buildings[i];
                if( gb && gb->m_locationId == generatorLocationId &&
                    gb->m_type == Building::TypeGenerator )
                {
                    gb->m_online = true;
                }

                if( gb && gb->m_locationId == mineLocationId &&
                    gb->m_type == Building::TypeRefinery )
                {
                    gb->m_online = true;
                }
            }
        }

        g_app->m_globalWorld->EvaluateEvents();
    }
};


class EnableReceiverAndBufferButton : public DarwiniaButton
{
    void MouseUp()
    {
        int receiverLocationId = g_app->m_globalWorld->GetLocationId( "receiver" );
        int bufferLocationId = g_app->m_globalWorld->GetLocationId( "pattern_buffer" );

        for( int i = 0; i < g_app->m_globalWorld->m_buildings.Size(); ++i )
        {
            if( g_app->m_globalWorld->m_buildings.ValidIndex(i) )
            {
                GlobalBuilding *gb = g_app->m_globalWorld->m_buildings[i];
                if( gb && gb->m_locationId == receiverLocationId &&
                    gb->m_type == Building::TypeSpiritProcessor )
                {
                    gb->m_online = true;
                }

                if( gb && gb->m_locationId == bufferLocationId &&
                    gb->m_type == Building::TypeBlueprintStore )
                {
                    gb->m_online = true;
                }
            }
        }

        g_app->m_globalWorld->EvaluateEvents();        
    }
};


class OpenAllLocationsButton : public DarwiniaButton
{
    void MouseUp()
    {
        for( int i = 0; i < g_app->m_globalWorld->m_locations.Size(); ++i )
        {
            GlobalLocation *loc = g_app->m_globalWorld->m_locations[i];
            loc->m_available = true;
        }
    }
};


class ClearResourcesButton : public DarwiniaButton
{
    void MouseUp()
    {
        g_app->m_resource->FlushOpenGlState();
		g_app->m_resource->RegenerateOpenGlState();
    }
};


class GiveAllResearchButton : public DarwiniaButton
{
    void MouseUp()
    {
        for( int i = 0; i < GlobalResearch::NumResearchItems; ++i )
        {
            g_app->m_globalWorld->m_research->AddResearch( i );
            //g_app->m_globalWorld->m_research->SetCurrentProgress( i, 400 );
            g_app->m_globalWorld->m_research->EvaluateLevel( i );
        }
    }
};


class SpawnPortsButton : public DarwiniaButton
{
    void MouseUp()
    {
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                for( int p = 0; p < building->GetNumPorts(); ++p )
                {
                    Vector3 portPos, portFront;
                    building->GetPortPosition( p, portPos, portFront );

                    g_app->m_location->SpawnEntities( portPos, 0, -1, Entity::TypeDarwinian, 3, g_zeroVector, 30.0f );
                }
            }
        }
    }
};

#ifdef PROFILER_ENABLED
class ProfilerCreateButton : public DarwiniaButton
{
    void MouseUp()
    {
        DebugKeyBindings::ProfileButton();
    }
};
#endif // PROFILER_ENABLED

#ifdef SOUND_EDITOR
class SoundEditorCreateButton : public DarwiniaButton
{
    void MouseUp()
    {
        DebugKeyBindings::SoundEditorButton();
    }
};


class SoundStatsCreateButton : public DarwiniaButton
{
    void MouseUp()
    {
        DebugKeyBindings::SoundStatsButton();
    }
};
#endif // SOUND_EDITOR

void CheatWindow::Create()
{
    DarwiniaWindow::Create();
    
    int y = 25;

    KillAllEnemiesButton *killAllEnemies = new KillAllEnemiesButton();
    killAllEnemies->SetShortProperties( "Kill All Enemies", 10, y, m_w - 20 );
    RegisterButton( killAllEnemies );

    SpawnDarwiniansButton *spawnDarwiniansGreen = new SpawnDarwiniansButton();
    spawnDarwiniansGreen->SetShortProperties( "Spawn Green", 10, y += 20, (m_w - 25)/2 );
    spawnDarwiniansGreen->m_teamId = 0;
    RegisterButton( spawnDarwiniansGreen );

    SpawnDarwiniansButton *spawnDarwiniansRed = new SpawnDarwiniansButton();
    spawnDarwiniansRed->SetShortProperties( "Spawn Red", spawnDarwiniansGreen->m_x+spawnDarwiniansGreen->m_w+5, y, (m_w - 25)/2 );
    spawnDarwiniansRed->m_teamId = 1;
    RegisterButton( spawnDarwiniansRed );

    SpawnTankButton *spawnTankButton = new SpawnTankButton();
    spawnTankButton->SetShortProperties( "Spawn Armour", 10, y += 20, m_w - 20 );
    RegisterButton( spawnTankButton );

    SpawnViriiButton *spawnVirii = new SpawnViriiButton();
    spawnVirii->SetShortProperties( "Spawn Virii", 10, y += 20, m_w - 20 );
    RegisterButton( spawnVirii );

    SpawnSpiritButton *spawnSpirits = new SpawnSpiritButton();
    spawnSpirits->SetShortProperties( "Spawn Spirits", 10, y += 20, m_w - 20 );
    RegisterButton( spawnSpirits );
    
    AllowArbitraryPlacementButton *allowPlacement = new AllowArbitraryPlacementButton();
    allowPlacement->SetShortProperties( "Allow Arbitrary Placement", 10, y += 20, m_w - 20 );
    RegisterButton( allowPlacement );
    
    EnableGeneratorAndMineButton *enable = new EnableGeneratorAndMineButton();
    enable->SetShortProperties( "Enable Generator and Mine", 10, y += 20, m_w - 20 );
    RegisterButton( enable );

    EnableReceiverAndBufferButton *receiver = new EnableReceiverAndBufferButton();
    receiver->SetShortProperties( "Enable Receiver and Buffer", 10, y += 20, m_w - 20 );
    RegisterButton( receiver );

    OpenAllLocationsButton *openAllLocations = new OpenAllLocationsButton();
    openAllLocations->SetShortProperties( "Open All Locations", 10, y += 20, m_w - 20 );
    RegisterButton( openAllLocations );

    GiveAllResearchButton *allResearch = new GiveAllResearchButton();
    allResearch->SetShortProperties( "Give all research", 10, y += 20, m_w - 20 );
    RegisterButton( allResearch );

    ClearResourcesButton *clearResources = new ClearResourcesButton();
    clearResources->SetShortProperties( "Clear Resources", 10, y += 20, m_w - 20 );
    RegisterButton( clearResources );

    SpawnPortsButton *spawnPorts = new SpawnPortsButton();
    spawnPorts->SetShortProperties( "Spawn Ports", 10, y+=20, m_w - 20 );
    RegisterButton( spawnPorts );

    y+=20;
    
#ifdef PROFILER_ENABLED
    ProfilerCreateButton *profiler = new ProfilerCreateButton();
    profiler->SetShortProperties( "Profiler", 10, y+=20, m_w-20);
    RegisterButton( profiler );
#endif // PROFILER_ENABLED
	
#ifdef SOUND_EDITOR
    SoundEditorCreateButton *soundEditor = new SoundEditorCreateButton();
    soundEditor->SetShortProperties( "Sound Editor", 10, y+=20, m_w-20);
    RegisterButton( soundEditor );

    SoundStatsCreateButton *soundStats = new SoundStatsCreateButton();
    soundStats->SetShortProperties( "Sound Stats", 10, y+=20, m_w-20 );
    RegisterButton( soundStats );
#endif // SOUND_EDITOR
}


#else   // CHEATMENU_ENABLED

void CheatWindow::Create()
{
}

#endif  // CHEATMENU_ENABLED

CheatWindow::CheatWindow( char *_name )
:   DarwiniaWindow( _name )
{
}

