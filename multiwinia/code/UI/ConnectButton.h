#ifndef _included_connectbutton_h
#define _included_connectbutton_h

class ServerConnectButton : public GameMenuButton
{
public:
    ServerConnectButton()
    :   GameMenuButton( UnicodeString() )
    {
    }

    void MouseUp()
    {
        if( m_inactive ) return;

        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        parent->JoinServer( parent->m_serverIndex );
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        if( parent->m_serverPassword.Get().Length() == 0 )
        {
            m_inactive = true;
        }
        else
        {
            m_inactive = false;
        }
        GameMenuButton::Render( realX, realY, highlighted, clicked );
    }
};

#endif