#ifndef __GAMETYPEINFOBUTTON__
#define __GAMETYPEINFOBUTTON__

#include "ScrollingTextButton.h"

class GameTypeInfoButton : public ScrollingTextButton
{
public:
    GameTypeInfoButton()
    :   ScrollingTextButton()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *main = (GameMenuWindow *)m_parent;
        int gameType = main->m_highlightedGameType;
        char stringId[256];
        if( Multiwinia::s_gameBlueprints.ValidIndex(gameType) )
        {
            sprintf( stringId, "multiwinia_help_%s", Multiwinia::s_gameBlueprints[gameType]->m_name );
        }
        else if( gameType == GAMETYPE_PROLOGUE )
        {
            sprintf( stringId, "multiwinia_help_prologue") ;
        }
        else if( gameType == GAMETYPE_CAMPAIGN )
        {
            sprintf( stringId, "multiwinia_help_campaign") ;
        }

        if( ISLANGUAGEPHRASE(stringId) )
        {
            m_string = LANGUAGEPHRASE(stringId);
            ClearWrapped();
            ScrollingTextButton::Render( realX, realY, highlighted, clicked );
        }
    }
};


#endif
