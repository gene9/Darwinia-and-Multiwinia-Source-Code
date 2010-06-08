#ifndef __REQUESTAIBUTTON__
#define __REQUESTAIBUTTON__

class RequestAIButton : public GameMenuButton 
{
public:
	RequestAIButton()
	: GameMenuButton( "Add AI Player")
	{
	}

    void MouseUp()
    {
        GameMenuButton::MouseUp();
		g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
    }
};

#endif
