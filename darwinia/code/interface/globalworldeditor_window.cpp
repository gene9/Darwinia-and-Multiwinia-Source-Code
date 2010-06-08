#include "lib/universal_include.h"
#include "lib/resource.h"
#include "lib/language_table.h"

#include "interface/input_field.h"
#include "interface/globalworldeditor_window.h"
#include "interface/message_dialog.h"

#include "app.h"
#include "global_world.h"
#include "level_file.h"


static char s_locationName[256] = "NewLevel";


class SetModeButton : public DarwiniaButton
{
public:
    int m_mode;
    void MouseUp()
    {
        g_app->m_globalWorld->m_editorMode = m_mode;
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        if( g_app->m_globalWorld->m_editorMode == m_mode )
        {
            DarwiniaButton::Render( realX, realY, true, clicked );
        }
        else
        {
            DarwiniaButton::Render( realX, realY, highlighted, clicked );
        }
    }   
};


class NewLocationButton : public DarwiniaButton
{
    void MouseUp()
    {
        //
        // If a MOD hasn't been set, don't allow this to happen
        // as it will try to save into darwinia/data/levels, which is clearly wrong
        // for the end user (but allow it for us)

#ifndef TARGET_DEBUG
        if( !g_app->m_resource->IsModLoaded() ) 
        {
            EclRegisterWindow( new MessageDialog( LANGUAGEPHRASE( "dialog_newlocationfail1" ),
                                                  LANGUAGEPHRASE( "dialog_newlocationfail2" ) ), 
                                                  m_parent );
            return;
        }
#endif

        //
        // Create the map and mission files
        
        LevelFile levelFile;
        sprintf( levelFile.m_mapFilename, "map_%s.txt", s_locationName );
        sprintf( levelFile.m_missionFilename, "mission_%s.txt", s_locationName );
        strlwr( levelFile.m_mapFilename );
        strlwr( levelFile.m_missionFilename );

        levelFile.Save();       
        
        //
        // Create new global location

        GlobalLocation *loc = new GlobalLocation();
        sprintf( loc->m_mapFilename, "map_%s.txt", s_locationName );
        sprintf( loc->m_missionFilename, "mission_%s.txt", s_locationName );
        strlwr( loc->m_mapFilename );
        strlwr( loc->m_missionFilename );
        strcpy( loc->m_name, s_locationName );
        loc->m_available = true;
        loc->m_pos.Set( -96.25, -274.02, 75.16 );
        g_app->m_globalWorld->AddLocation( loc );

        //
        // Save game.txt and locations.txt

        g_app->m_globalWorld->SaveGame( "game.txt" );
        g_app->m_globalWorld->SaveLocations( "locations.txt" );
    }
};


class SaveLocationsButton : public DarwiniaButton
{
    void MouseUp()
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

        //
        // It's ok to save

        g_app->m_globalWorld->SaveLocations( "locations.txt" );
    }
};


GlobalWorldEditorWindow::GlobalWorldEditorWindow()
:   DarwiniaWindow( LANGUAGEPHRASE("editor_globalworldeditor") )
{
    SetPosition( 20, 20 );
    SetSize( 200, 100 );
}


void GlobalWorldEditorWindow::Create()
{
    DarwiniaWindow::Create();

    int y = 20;
    int h = 18;

    SetModeButton *setEdit = new SetModeButton();
    setEdit->m_mode = 0;
    setEdit->SetShortProperties( LANGUAGEPHRASE("editor_editlocations"), 10, y += h, m_w-20 );
    RegisterButton( setEdit );

    SetModeButton *setMove = new SetModeButton();
    setMove->m_mode = 1;
    setMove->SetShortProperties( LANGUAGEPHRASE("editor_movelocations"), 10, y += h, m_w-20 );
    RegisterButton( setMove );

    y += h;

    NewLocationButton *newLoc = new NewLocationButton();
    newLoc->SetShortProperties( LANGUAGEPHRASE("editor_createnewlocation"), 10, y += h, m_w - 20 );
    RegisterButton( newLoc );

    CreateValueControl( LANGUAGEPHRASE("dialog_name"), InputField::TypeString, s_locationName, y +=h, 0, 0, 0, NULL, 10, m_w-20 );

    y += h;
    
    SaveLocationsButton *save = new SaveLocationsButton();
    save->SetShortProperties( LANGUAGEPHRASE("editor_savelocations"), 10, y += h, m_w - 20 );
    RegisterButton( save );
}


void GlobalWorldEditorWindow::Update()
{
    if( !g_app->m_editing ||
        g_app->m_location )
    {
        EclRemoveWindow( m_name );
    }
}