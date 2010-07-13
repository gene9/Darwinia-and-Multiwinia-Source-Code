#ifndef __GAMETYPEBUTTON__
#define __GAMETYPEBUTTON__

class GameTypeButton : public GameMenuButton
{
public:
    int m_gameType;

    GameTypeButton( const UnicodeString &_iconName, int _gameType )
    :   GameMenuButton( _iconName ),
        m_gameType(_gameType)
    {
    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
		if( !g_app->m_globalWorld )
			return;

        GameMenuWindow *parent = (GameMenuWindow *)m_parent;
        if( g_app->m_globalWorld->m_myTeamId != 255 || 
            g_app->m_spectator ||
            parent->m_settingUpCustomGame )
        {
            GameMenuButton::Render( realX, realY, highlighted, clicked );

            float fontLarge, fontMed, fontSmall;
            GetFontSizes( fontLarge, fontMed, fontSmall );

            if( parent->m_buttonOrder[ parent->m_currentButton ] == this )
            {
                highlighted = true;
            }
            else
            {
                highlighted = false;
            }
			
            char *difficulty = MultiwiniaGameBlueprint::GetDifficulty(m_gameType);

            int w = m_w * 0.1f;
            int h = w * 0.5f;

            g_titleFont.SetRenderOutline(true);
            glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
            //g_titleFont.DrawText2DRight( realX + m_w, realY + m_h - (14.0f * scale), fontSmall, LANGUAGEPHRASE(difficulty) );

            char imageName[256];
			if( strstr( difficulty, "advanced" ) )
            {
                glColor4f( 1.0f, 0.2f, 0.2f, 1.0f );
                sprintf( imageName, "icons/menu_advanced.bmp");
            }
            else if( strstr( difficulty, "intermediate" ) )
            {
                glColor4f( 1.0f, 1.0f, 0.2f, 1.0f );
                sprintf( imageName, "icons/menu_intermediate.bmp");                            
            }
			else
            {
                glColor4f( 0.6f, 1.0f, 0.4f, 1.0f );
                sprintf( imageName, "icons/menu_basic.bmp");            
            }

            g_titleFont.SetRenderOutline(false);
            //g_titleFont.DrawText2DRight( realX + m_w, realY + m_h - (14.0f * scale), fontSmall, LANGUAGEPHRASE(difficulty) );

            int x = realX + m_w - w;
            int y = realY + m_h * 0.5f - h * 0.5f;

            glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture(imageName) );
            glEnable( GL_TEXTURE_2D );

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            glBegin( GL_QUADS );
                glTexCoord2i(1,1);      glVertex2f( x, y );
                glTexCoord2i(0,1);      glVertex2f( x+w, y );
                glTexCoord2i(0,0);      glVertex2f( x+w, y + h );
                glTexCoord2i(1,0);      glVertex2f( x, y + h );
            glEnd();

            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glDisable( GL_TEXTURE_2D );

            if( highlighted )
            {
                ((GameMenuWindow *) m_parent)-> m_highlightedGameType = m_gameType;
            }
        }
    }

    void MouseUp()
    {
		GameMenuWindow *main = (GameMenuWindow *)EclGetWindow( /*"GameMenu"*/"multiwinia_mainmenu_title" );

		if( main && IS_DEMO && !g_app->IsGameModePermitted(m_gameType) )
		{
            GameMenuButton::MouseUp();
            main->m_newPage = GameMenuWindow::PageBuyMeNow;
            return;
		}

		if( m_inactive )
		{
			return;
		}

        GameMenuButton::MouseUp();

        if( g_app->m_globalWorld->m_myTeamId != 255 || 
            g_app->m_spectator ||
            main->m_settingUpCustomGame )
        {
            /*if( IS_DEMO )
            {
                if( g_app->m_gameMenu->m_numDemoMaps[m_gameType] == 0 ) return;
            }*/

            main->m_newPage = GameMenuWindow::PageMapSelect;
            if( main->m_settingUpCustomGame )
            {
                main->m_customGameType = m_gameType;
            }
            else
            {
                main->m_gameType = m_gameType;
                main->m_uiGameType = -1;
                main->m_clickGameType = m_gameType;
            }

            
            MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[m_gameType];
		    for( int i = 0; i < blueprint->m_params.Size(); ++i )
		    {
			    MultiwiniaGameParameter *param = blueprint->m_params[i];   
			    main->m_params[i] = param->m_default;
		    }
        }
    }
};

#endif
