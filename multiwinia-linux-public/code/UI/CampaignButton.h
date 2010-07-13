#ifndef __CAMPAIGNBUTTON__
#define __CAMPAIGNBUTTON__

#include "main.h"

class CampaignButton : public GameMenuButton
{
public:
    CampaignButton ( char *_iconName )
    :   GameMenuButton( _iconName )
    {
    }

	static void LoadCampaign()
	{
		if( g_app->m_server )
		{
			g_app->ShutdownCurrentGame();
		}

		RequestBootloaderSequence();
		strcpy( g_app->m_gameDataFile, "game.txt" );
		g_app->m_gameMode = App::GameModeCampaign;
		g_app->LoadProfile();

		g_app->m_atMainMenu = false;
	}
    void MouseUp()
    {
        if( IS_DEMO ) return;

        GameMenuButton::MouseUp();

		g_app->m_loadingLocation = true;
		g_loadingScreen->m_workQueue->Add( &LoadCampaign );
		g_loadingScreen->Render();
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
            parent->m_highlightedGameType = GAMETYPE_CAMPAIGN;
        }
		/*
        if( IS_DEMO )
        {
            float fontLarge, fontMed, fontSmall;
            GetFontSizes( fontLarge, fontMed, fontSmall );

            float texY = realY + m_h/2 - m_fontSize/4;
            char mapNums[512];
            sprintf( mapNums, "(0 / 10 Playable)" ); // LOCALISE ME
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_titleFont.SetRenderOutline(true);
            g_titleFont.DrawText2DRight( realX + m_w, texY, fontSmall, mapNums );
            glColor4f( m_ir, m_ig, m_ib, m_ia );
            g_titleFont.SetRenderOutline(false);
            g_titleFont.DrawText2DRight( realX + m_w, texY, fontSmall, mapNums );
        }
        else
		*/
		if( !IS_DEMO )
        {
            m_inactive = false;
        }
    }
};

#endif







