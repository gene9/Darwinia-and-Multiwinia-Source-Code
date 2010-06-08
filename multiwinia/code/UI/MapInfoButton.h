#ifndef __MAPINFOBUTTON__
#define __MAPINFOBUTTON__

class MapInfoButton : public ScrollingTextButton
{
public:
    MapInfoButton()
    :   ScrollingTextButton()
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        int gameType = ((GameMenuWindow *) m_parent)->m_gameType;        
        int mapId = ((GameMenuWindow *) m_parent)->m_highlightedLevel;

        if( gameType == -1 ) gameType = ((GameMenuWindow *) m_parent)->m_highlightedGameType;

        if( gameType >= 0 && gameType < Multiwinia::NumGameTypes )
        {
            if( !g_app->m_gameMenu->m_maps[gameType].ValidIndex(mapId) )
            {
                mapId = ((GameMenuWindow *) m_parent)->m_requestedMapId;
            }
            if( g_app->m_gameMenu->m_maps[gameType].ValidIndex(mapId) )
            {
                MapData *md = g_app->m_gameMenu->m_maps[gameType][mapId];

                if( md->m_customMap )
                {
                    if( md->m_description && strlen(md->m_description) > 0 )
                    {
                        m_string = UnicodeString( md->m_description );
                        ClearWrapped();
                        ScrollingTextButton::Render( realX, realY, highlighted, clicked );
                    }
                }
                else
                {
                    char *description = GetMapDescriptionId( md->m_fileName );
                    if( description )
                    {
                        m_string = LANGUAGEPHRASE(description);
                        ClearWrapped();
                        ScrollingTextButton::Render( realX, realY, highlighted, clicked );
                    }
                }
            }
        }
    }
};


#endif
