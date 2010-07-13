#ifndef __PROLOGUEBUTTON__
#define __PROLOGUEBUTTON__

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "../tutorial.h"
#endif
#include "app.h"
#include "loading_screen.h"

class PrologueButton : public GameMenuButton
{
public:
    PrologueButton( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

	static void LoadPrologue()
	{
		if( g_app->m_server )
		{
			g_app->ShutdownCurrentGame();
		}

		strcpy( g_app->m_gameDataFile, "game_demo2.txt" );
		g_app->m_gameMode = App::GameModePrologue;
		g_app->m_globalWorld->m_loadingNewProfile = false;
		g_app->LoadProfile();

		g_app->m_requestedLocationId = g_app->m_globalWorld->GetLocationId("launchpad");
		GlobalLocation *gloc = g_app->m_globalWorld->GetLocation(g_app->m_requestedLocationId);

		if( gloc == NULL )
		{
			// Soft message better than hard crash
			AppDebugOut("Failed to find launchpad level in the global world.\n");
			return;
		}

		AppReleaseAssert( gloc != NULL, "Unable to find launchpad level in the global world." );

		strcpy(g_app->m_requestedMap, gloc->m_mapFilename);
		strcpy(g_app->m_requestedMission, gloc->m_missionFilename);

#ifdef USE_SEPULVEDA_HELP_TUTORIAL
		g_app->m_tutorial = new Demo2Tutorial();
#endif
	}
    void MouseUp()
    {
		AppDebugOut("Prologue Button Pressed. Time = %f\n", GetHighResTime() );
        GameMenuButton::MouseUp();

		g_app->m_loadingLocation = true;
		g_loadingScreen->m_workQueue->Add( &LoadPrologue );
		g_loadingScreen->Render();

        g_app->m_atMainMenu = false;
        g_app->m_gameMode = App::GameModePrologue;
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuButton::Render( realX, realY, highlighted, clicked );
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        if( parent->m_buttonOrder[ parent->m_currentButton ] == this )
        {
            highlighted = true;
        }
        if( highlighted )
        {
            GameMenuWindow *parent = (GameMenuWindow *)m_parent;
            parent->m_highlightedGameType = GAMETYPE_PROLOGUE;
        }
		/*
        if( IS_DEMO )
        {
            float fontLarge, fontMed, fontSmall;
            GetFontSizes( fontLarge, fontMed, fontSmall );

            float texY = realY + m_h/2 - m_fontSize/4;
            char mapNums[512];
            sprintf( mapNums, "(1 / 1 Playable)" ); // LOCALISE ME
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2DRight( realX + m_w, texY, fontSmall, mapNums );
            glColor4f( 0.5f, 1.0f, 0.3f, 1.0f );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2DRight( realX + m_w, texY, fontSmall, mapNums );
        }
		*/
    }
};

#endif
