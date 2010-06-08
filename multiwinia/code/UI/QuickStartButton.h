#ifndef __QUICKSTARTBUTTON__
#define __QUICKSTARTBUTTON__

class QuickStartButton : public GameMenuButton
{
public:
    QuickStartButton()
    :   GameMenuButton("")
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        if( g_app->m_globalWorld->m_myTeamId == 255) return;

        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }

    void MouseUp()
    {
        GameMenuWindow *main = (GameMenuWindow *)m_parent;
        if( main->m_quickStart ) return;

        GameMenuButton::MouseUp();

        //if( !g_app->m_server )
        {
            	// TODO Remove me

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

            main->m_quickStart = true;

            g_app->m_renderer->StartFadeOut();
        }
    }
};

#endif
