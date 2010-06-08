#ifndef __FANCYCOLOURBUTTON__
#define __FANCYCOLOURBUTTON__

class FancyColourButton : public GameMenuButton
{
public:
	int	m_colourIndex;
	FancyColourButton(int _colourId)
	:	GameMenuButton(""),
		m_colourIndex(_colourId)
	{
	}

	void MouseUp()
	{
		if( m_inactive ) return;

        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

		bool taken = false;
        if( parent->m_gameType == Multiwinia::GameTypeAssault )
        {
            int num = 0;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex )
                {
                    num++;
                }
            }

            if( 0 <= parent->m_gameType && parent->m_gameType < MAX_GAME_TYPES &&
                g_app->m_gameMenu->m_maps[parent->m_gameType].ValidIndex( parent->m_requestedMapId ) )
            {
                int numPlayers = g_app->m_gameMenu->m_maps[parent->m_gameType][parent->m_requestedMapId]->m_numPlayers;
                if( numPlayers == 3 )
                {
                    if( m_colourIndex == 0 && num > 0 ) taken = true;
                    else if( m_colourIndex == 1 && num == 2 ) taken = true;
                }
                else
                {
                    if( num >= numPlayers / 2 ) taken = true;
                }
            }
        }
        else if( g_app->m_multiwinia->m_coopMode )
        {
            if( !g_app->m_multiwinia->IsValidCoopColour( m_colourIndex ) )
            {
                taken = true;
            }
        }
        else
        {
		    for( int i = 0; i < NUM_TEAMS; ++i )
		    {
			    if( g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex )
			    {
				    taken = true;
			    }
		    }
        }
        if( m_colourIndex == -1 ) taken = false;

		if( !taken )
		{
			parent->m_currentRequestedColour = m_colourIndex;
            if( parent->m_gameType != Multiwinia::GameTypeAssault && !g_app->m_multiwinia->m_coopMode )
            {
                g_prefsManager->SetInt( "PlayerPreferedColour", m_colourIndex );
                g_prefsManager->Save();
            }

            g_app->m_renderer->InitialiseMenuTransition( 1.0f, -1 );
            ((GameMenuWindow *) m_parent)->m_newPage = GameMenuWindow::PageGameSetup;
		}
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
        GameMenuWindow *parent = (GameMenuWindow *)m_parent;

		bool taken = false;
        if( parent->m_gameType == Multiwinia::GameTypeAssault )
        {
            int num = 0;
            for( int i = 0; i < NUM_TEAMS; ++i )
            {
                if( g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused ) continue;

                if( g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex &&
                    g_app->m_globalWorld->m_myTeamId != i )
                {
                    num++;
                }

                if( i == g_app->m_globalWorld->m_myTeamId &&
				    g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex) 
			    {
				    clicked = true;
			    }
            }

            if( 0 <= parent->m_gameType && parent->m_gameType < MAX_GAME_TYPES &&
                g_app->m_gameMenu->m_maps[parent->m_gameType].ValidIndex( parent->m_requestedMapId ) )
            {
                int numPlayers = g_app->m_gameMenu->m_maps[parent->m_gameType][parent->m_requestedMapId]->m_numPlayers;
                if( numPlayers == 3 )
                {
                    if( m_colourIndex == 0 && num > 0 ) taken = true;
                    else if( m_colourIndex == 1 && num == 2 ) taken = true;
                }
                else
                {
                    if( num >= numPlayers / 2 ) taken = true;
                }
            }
        }
        else if( g_app->m_multiwinia->m_coopMode )
        {
            if( !g_app->m_multiwinia->IsValidCoopColour( m_colourIndex ) )
            {
                taken = true;
            }

            if(g_app->m_multiwinia->m_teams[g_app->m_globalWorld->m_myTeamId].m_colourId == m_colourIndex) 
		    {
			    clicked = true;
		    }
        }
        else
        {
		    for( int i = 0; i < NUM_TEAMS; ++i )
		    {
                if( g_app->m_multiwinia->m_teams[i].m_teamType == TeamTypeUnused ) continue;

			    if( g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex &&
				    g_app->m_globalWorld->m_myTeamId != i )
			    {
				    taken = true;
			    }

			    if( i == g_app->m_globalWorld->m_myTeamId &&
				    g_app->m_multiwinia->m_teams[i].m_colourId == m_colourIndex) 
			    {
				    clicked = true;
			    }
		    }
        }

		if( !m_mouseHighlightMode )
		{
			highlighted = false;
		}

		if( parent->m_buttonOrder[parent->m_currentButton] == this )
		{
			highlighted = true;
		}


		UpdateButtonHighlight();

		if( taken ) m_inactive = true;
		else m_inactive = false;

		if( !highlighted && !clicked )
        {
            glColor4f( 1.0f, 1.0f, 1.0f, 0.1f );
		}
		else
		{
			glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
		}

		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glBegin( GL_QUADS);
			glVertex2f( realX, realY );
			glVertex2f( realX + m_w, realY );
			glVertex2f( realX + m_w, realY + m_h );
			glVertex2f( realX, realY + m_h );
		glEnd();

		glColor4f(1.0f,1.0f,1.0f,0.0f);
		g_titleFont.SetRenderOutline(true);
		float texY = realY + (m_h/2) - (m_fontSize/2) + 7;
		g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );
		glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
		if( clicked )
		{
			glColor4f( 1.0f, 0.3f, 0.3f, 1.0f );
		}
		if( m_inactive ) glColor4f( m_ir, m_ig, m_ib, m_ia );
	    
		g_titleFont.SetRenderOutline(false);
		g_titleFont.DrawText2D( realX + m_w*0.05f, texY, m_fontSize, m_caption );

        if( m_colourIndex == -1 )
        {
            glColor4f( 0.7f, 0.7f, 0.7f, 1.0f );
        }
		else if( taken )
		{
			glColor4f( 0.5f, 0.5f, 0.5f, 1.0f );
		}
		else
		{
			glColor4ubv( g_app->m_multiwinia->GetColour(m_colourIndex).GetData());
		}


		int boxX = realX + m_w - (m_w / 4);
		int boxW = (m_w/4) * 0.95f;
		int boxH = m_h * 0.8f;
		int boxY = realY + m_h * 0.1f;

		glShadeModel( GL_SMOOTH);
        glBegin( GL_QUADS);
            glVertex2f( boxX, boxY );
            glVertex2f( boxX, boxY + boxH );
            glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );
            glVertex2f( boxX + boxW, boxY + boxH );
            glVertex2f( boxX + boxW, boxY );
        glEnd();
        glShadeModel( GL_FLAT );

		
		if( highlighted && s_previousHighlightButton != this )
		{
			g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
			s_previousHighlightButton = this;
		}
	}
};

#endif
