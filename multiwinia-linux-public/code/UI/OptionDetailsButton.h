#ifndef __OPTIONDETAILSBUTTON__
#define __OPTIONDETAILSBUTTON__

class OptionDetailsButton : public ScrollingTextButton
{
public:
    OptionDetailsButton()
    :   ScrollingTextButton()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        if( parent->m_buttonOrder.ValidIndex( parent->m_currentButton ) )
        {
            EclButton *b = parent->m_buttonOrder[ parent->m_currentButton ];
            char stringId[512];
            sprintf( stringId, "%s_description_%d", b->m_name, parent->m_gameType.Get() );
            if( g_app->m_langTable->DoesPhraseExist( stringId ) )
            {
                m_string = LANGUAGEPHRASE(stringId);
                ClearWrapped();
            }
            else
            {
                sprintf( stringId, "%s_description", b->m_name );
                if( g_app->m_langTable->DoesPhraseExist( stringId ) )
                {
                    m_string = LANGUAGEPHRASE(stringId);
                    ClearWrapped();
                }
            }
            ScrollingTextButton::Render( realX, realY, highlighted, clicked );

        }
    }
};


#endif
