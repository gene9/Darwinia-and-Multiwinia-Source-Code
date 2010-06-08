#ifndef __ROLLINGDEMOBUTTON__
#define __ROLLINGDEMOBUTTON__

class RollingDemoButton : public GameMenuButton
{
public:
    RollingDemoButton( const UnicodeString &caption )
        :   GameMenuButton( caption )
    {
    }

    void MouseUp()
    {
        GameMenuWindow *main = (GameMenuWindow *)m_parent;
        if( main->m_quickStart ) return;

        GameMenuButton::MouseUp();

        g_app->m_soundSystem->WaitIsInitialized();

        main->m_gameType = Multiwinia::GameTypeKingOfTheHill;
        g_app->m_multiwinia->m_gameType = Multiwinia::GameTypeKingOfTheHill;
        main->m_requestedMapId = 0;			
        main->m_currentRequestedColour = 0;

        MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[Multiwinia::GameTypeKingOfTheHill];
        for( int i = 0; i < blueprint->m_params.Size(); ++i )
        {
            MultiwiniaGameParameter *param = blueprint->m_params[i];   
            main->m_params[i] = param->m_default;
        }
        
        GameMenuWindow *window = (GameMenuWindow *)m_parent;

        g_app->m_renderer->StartFadeOut();

        g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
        g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
        g_app->m_clientToServer->RequestStartGame();        
    }
};

#endif
