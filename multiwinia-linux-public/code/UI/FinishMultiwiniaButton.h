#ifndef __FINISHMULTIWINIABUTTON__
#define __FINISHMULTIWINIABUTTON__

extern const char *s_pageNames[GameMenuWindow::NumPages];

#include "location.h"

class FinishMultiwiniaButton : public BackPageButton
{
public:
    FinishMultiwiniaButton( int _page )
    :   BackPageButton( _page )
    {
    }

    void MouseUp()
    {
        if( !g_app->m_server )
        {
            bool lagged = GetHighResTime() > g_app->m_clientToServer->m_lastServerLetterReceivedTime + 5.0f;
            int teamId = g_app->m_globalWorld->m_myTeamId;
            if( teamId != 255 && !lagged )
            {
                if( g_app->m_multiwinia->m_teams[teamId].m_ready )
                {
                    ((GameMenuWindow *)m_parent)->m_disconnectFromGame = true;
                    g_app->m_clientToServer->RequestToggleReady(teamId);
                    return;
                }
            }
        }
        
		AppDebugOut("FinishMultiwiniaButton pressed. m_pageId = %s\n", s_pageNames[m_pageId] );
		g_app->ShutdownCurrentGame();
		g_app->m_atLobby = false;
        ((GameMenuWindow *)m_parent)->m_requestedMapId = -1;
		BackPageButton::MouseUp();

        ((GameMenuWindow *)m_parent)->m_chatMessages.EmptyAndDelete();
        ((GameMenuWindow *)m_parent)->m_serverPassword.Set( UnicodeString() );
    }
};

class FinishSinglePlayerButton : public BackPageButton
{
public:
    FinishSinglePlayerButton( int _page )
    :   BackPageButton( _page )
    {
    }

    void MouseUp()
    {
        g_app->m_multiwinia->m_coopMode = false;
        g_app->m_multiwinia->m_basicCratesOnly = false;
        g_app->m_clientToServer->ReliableSetTeamColour(-1);
        for( int i = 0; i < NUM_TEAMS; ++i )
        {
            g_app->m_multiwinia->m_teams[i].m_colourId = -1;
        }
        BackPageButton::MouseUp();

        ((GameMenuWindow *)m_parent)->m_chatMessages.EmptyAndDelete();
        ((GameMenuWindow *)m_parent)->m_serverPassword.Set( UnicodeString() );
    }
};

class SaveFiltersButton : public BackPageButton
{
public:
    SaveFiltersButton( int _page )
    :   BackPageButton( _page )
    {
    }

    void MouseUp()
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        g_prefsManager->SetInt( "FilterShowDemos", (int)parent->m_showDemosFilter );
        g_prefsManager->SetInt( "FilterShowIncompatible", (int)parent->m_showIncompatibleGamesFilter );
        g_prefsManager->SetInt( "FilterShowFullGames", (int)parent->m_showFullGamesFilter );
        g_prefsManager->SetInt( "FilterShowCustomMaps", (int)parent->m_showCustomMapsFilter );
        g_prefsManager->SetInt( "FilterShowPassworded", (int)parent->m_showPasswordedFilter );
        g_prefsManager->Save();

        BackPageButton::MouseUp();
    }

};

#endif
