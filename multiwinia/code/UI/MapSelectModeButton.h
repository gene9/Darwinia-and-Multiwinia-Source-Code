#ifndef __MAPSELECTMODEBUTTON__
#define __MAPSELECTMODEBUTTON__

class MapSelectModeButton : public GameMenuButton
{
public:
    MapSelectModeButton()
    : GameMenuButton( "MapSelect" )
    {
    }

    void MouseUp()
    {
        return;
        GameMenuButton::MouseUp();

		GameMenuWindow *parent = (GameMenuWindow *)m_parent;
		bool canChangeMap = true;

		// We don't want to be able to change the map without checking other players
        if( 0 <= parent->m_gameType && parent->m_gameType < MAX_GAME_TYPES &&
            g_app->m_gameMenu->m_maps[parent->m_gameType].ValidIndex(parent->m_requestedMapId ) )
        {
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( i < g_app->m_gameMenu->m_maps[parent->m_gameType][parent->m_requestedMapId]->m_numPlayers  )
                {
					AppDebugOut("At the moment we can %schange map. Index %d in team search is %s\n", 
						canChangeMap ? "" : "not ",
						i,
						g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused ? "unused" : 
						g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeCPU ? "CPU" : 
						g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeLocalPlayer ? "local player" : 
						g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeRemotePlayer ? "remote player" : 
						"unknown"
					);
                    canChangeMap &= g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused;
                }
            }

			parent->UpdateGameOptions( true );
        }

        if( g_app->m_server && canChangeMap )
        {
            ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageMapSelect;
        }
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        //GameMenuButton::Render( realX, realY, highlighted, clicked );
        GameMenuWindow *parent = ((GameMenuWindow *) m_parent);
        
        RenderHighlightBeam( realX, realY + m_h/2, m_w, m_h );

        if( 0 <= parent->m_gameType && parent->m_gameType < MAX_GAME_TYPES &&
            g_app->m_gameMenu->m_maps[parent->m_gameType].ValidIndex( parent->m_requestedMapId ) )
        {
            MapData *md = g_app->m_gameMenu->m_maps[parent->m_gameType][parent->m_requestedMapId];
            const char *tempName = GetMapNameId( md->m_fileName );
			char name[256];
			strcpy(name,tempName);
            if( !name )
            {
                strcpy(name,md->m_fileName);
            }

            if( !g_app->m_server )
            {
                highlighted = false;
                clicked = false;
            }


            int h = g_app->m_renderer->ScreenH();
            float buttonH = h * 0.03f;
            float fontSize = m_fontSize;

			float texY = realY + m_h/2.0f - fontSize/2.0f + 7.0f;

            g_editorFont.SetRenderOutline(true);
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            g_editorFont.DrawText2DCentre(realX + m_w/2, texY, fontSize, LANGUAGEPHRASE(name) );

            g_editorFont.SetRenderOutline(false);
           /* if( highlighted )
            {
                glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
            }
            else*/
            {
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            }
            g_editorFont.DrawText2DCentre(realX + m_w/2, texY, fontSize, LANGUAGEPHRASE(name) );
        }

    }
};

#endif
