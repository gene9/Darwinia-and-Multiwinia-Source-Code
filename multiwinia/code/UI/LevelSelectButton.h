#ifndef __LEVELSELECTBUTTON__
#define __LEVELSELECTBUTTON__

class LevelSelectButton : public LevelSelectButtonBase
{
public:
    int m_levelIndex;
    int m_type;

    void MouseUp()
    {
		GameMenuWindow* gmw = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
				

		g_app->m_renderer->InitialiseMenuTransition(1.0f,1);
        GameMenuWindow *main = (GameMenuWindow *) m_parent;

		if( main->m_gameType < 0 || main->m_gameType >= MAX_GAME_TYPES || !g_app->m_gameMenu->m_maps[main->m_gameType].ValidIndex( m_levelIndex ) )
		{
			AppDebugOut( "Game Menu: Failed to select level due to invalid game type or map id (Game Type = %d, Map ID = %d)\n",
			             (int) main->m_gameType, m_levelIndex );
			return;
		}

		g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuSelect", SoundSourceBlueprint::TypeMultiwiniaInterface );

		MapData *mapData = g_app->m_gameMenu->m_maps[main->m_gameType][m_levelIndex];
		/* Removed due to game menu lag, tested further in game start up
		mapData->CalculeMapId();
		if( !g_app->IsMapPermitted( main->m_gameType, mapData->m_mapId ) )
		{
			return;
		}
		*/

		if( IS_DEMO && main->DemoRestrictedMap( m_levelIndex ) )
		{
            main->m_newPage = GameMenuWindow::PageBuyMeNow;
            return;
		}
        
        if( main->m_settingUpCustomGame )
        {
            main->m_customMapId = m_levelIndex;
            main->m_newPage = GameMenuWindow::PageJoinGame; 
            return;
        }

		main->m_requestedMapId = m_levelIndex;   
        main->m_localSelectedLevel = m_levelIndex;
        main->m_newPage = GameMenuWindow::PageGameSetup;

        //MapData *mapData = g_app->m_gameMenu->m_maps[main->m_gameType][m_levelIndex];
        if( main->m_coopMode && !mapData->m_coop )
        {
            main->m_coopMode = false;
        }

        if( mapData->m_forceCoop ) main->m_coopMode = true;

        int id = GetTimeOption();

        if( id != -1 )
        {
            int playTime = mapData->m_playTime;
            if( playTime != 0 )
            {
                main->m_params[id] = playTime;
            }
            else
            {
                MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[main->m_gameType];
                main->m_params[id] = blueprint->m_params[id]->m_default;
            }
        }

    }

    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *main = (GameMenuWindow *) m_parent;

        if( !m_mouseHighlightMode )
        {
            highlighted = false;
        }

        if( main->m_buttonOrder[main->m_currentButton] == this )
        {
            highlighted = true;
        }

        RenderBackground( realX, realY, highlighted, clicked );
     
        if( highlighted )
        {
            main->m_highlightedLevel = m_levelIndex;
        }

        if( main->m_requestedMapId == m_levelIndex )
        {
            clicked = true;
        }

		int gameType;
        if( main->m_settingUpCustomGame ) 
		{
			gameType = main->m_customGameType;
		}
		else
		{
	        gameType = main->m_gameType;
		}

		if( gameType < 0 || gameType >= MAX_GAME_TYPES || !g_app->m_gameMenu->m_maps[gameType].ValidIndex( m_levelIndex ) )
		{
			return;
		}

        DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[gameType];
		MapData *mapData = maps[m_levelIndex];

        char levelName[256];

        if( mapData->m_customMap )
        {
            if( mapData->m_levelName ) 
            {
                strcpy( levelName, mapData->m_levelName );
            }
            else
            {
                strcpy( levelName, mapData->m_fileName );
            }

        }
        else
        {
            const char *tempLevelName = GetMapNameId( maps[m_levelIndex]->m_fileName );
            strcpy(levelName, tempLevelName);
            if( !levelName )
            {
                //levelName = maps[m_levelId]->m_fileName;
                strcpy(levelName, maps[m_levelIndex]->m_fileName);
            }
        }

        UnicodeString name = LANGUAGEPHRASE(levelName);

        bool demoRestricted = IS_DEMO && main->DemoRestrictedMap( m_levelIndex );
		RenderMapName( name, realX, realY, highlighted, clicked, demoRestricted );

        int playTime = mapData->m_playTime;
        if( playTime == 0 )
        {
            int id = GetTimeOption();

            if( id != -1 )
            {
                MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[gameType];
                playTime = blueprint->m_params[id]->m_default;
            }
        }

        float nameX, timeX, playersX, coopX, diffX;
        GetColumnPositions( realX, m_w, nameX, timeX, playersX, coopX, diffX );

        float fontY = realY + m_h/2.0f - m_fontSize/2.0f + 7.0f;

        char playTimeCaption[256];
        sprintf( playTimeCaption, "0:%02d", playTime );
        if( playTime == 0 ) sprintf( playTimeCaption, "-" );

        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );
        g_titleFont.SetRenderOutline(true);
        g_titleFont.DrawText2D( timeX, fontY, m_fontSize, playTimeCaption );

        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
        if( demoRestricted ) glColor4f( 0.3f, 0.3f, 0.3f, 1.0f );
        g_titleFont.SetRenderOutline(false);
        g_titleFont.DrawText2D( timeX, fontY, m_fontSize, playTimeCaption );


        float scale = 1.0f;

        /*if( mapData->m_difficulty )
        {
            char imageName[512];
            
            if( strcmp( mapData->m_difficulty, "basic" ) == 0 )                 sprintf( imageName, "icons/menu_basic.bmp");            
            else if( strcmp( mapData->m_difficulty, "intermediate" ) == 0 )     sprintf( imageName, "icons/menu_intermediate.bmp");                            
            else if( strcmp( mapData->m_difficulty, "advanced" ) == 0 )         sprintf( imageName, "icons/menu_advanced.bmp");
            
            if( g_app->m_resource->DoesTextureExist( imageName ) )
            {        
                int texId = g_app->m_resource->GetTexture( imageName );

                glEnable( GL_TEXTURE_2D );
                glBindTexture( GL_TEXTURE_2D, texId );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

				if( demoRestricted )
                {
                    glColor4f( 0.2f, 0.2f, 0.2f, 0.7f );
                }

                int x = diffX;
                int y = realY;

                int w = 48 * scale;
                int h = 24 * scale;

                glBegin( GL_QUADS );
                    glTexCoord2i(1,1);      glVertex2f( x, y );
                    glTexCoord2i(0,1);      glVertex2f( x+w, y );
                    glTexCoord2i(0,0);      glVertex2f( x+w, y + h );
                    glTexCoord2i(1,0);      glVertex2f( x, y + h );
                glEnd();

                glDisable( GL_TEXTURE_2D );
            }     
        }*/

        int numPlayers = mapData->m_numPlayers;

		if( numPlayers > 0 )
        {
            int x = playersX;

            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/darwinian.bmp" ) );
            glEnable        ( GL_TEXTURE_2D );
            glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            
            float h = m_h * 0.9f;
            float w = h;
            float y = m_y + m_h/2.0f;
            x = playersX + w * 0.55f * 5.0f;

            for( int i = 3; i >= 0; --i )
            {
                x -= w * 0.55f;

                if( i < numPlayers )
                {
                    Vector3 pos1( -w/2.0f, 0, -h/2.0f );
                    Vector3 pos2( +w/2.0f, 0, -h/2.0f );
                    Vector3 pos3( +w/2.0f, 0, +h/2.0f );
                    Vector3 pos4( -w/2.0f, 0, +h/2.0f );

                    pos1.RotateAroundY( -0.125f * M_PI );
                    pos2.RotateAroundY( -0.125f * M_PI );
                    pos3.RotateAroundY( -0.125f * M_PI );
                    pos4.RotateAroundY( -0.125f * M_PI );

                    pos1 += Vector3( x, 0, y );
                    pos2 += Vector3( x, 0, y );
                    pos3 += Vector3( x, 0, y );
                    pos4 += Vector3( x, 0, y );

                    RGBAColour teamCol = g_app->m_multiwinia->GetColour(i);
                    if( mapData->m_forceCoop ) 
                    {
                        int index = int(i/2);
                        if( numPlayers == 3 && i == 1 ) index = 1;
                        teamCol = g_app->m_multiwinia->GetColour( index );
                    }

                    if( demoRestricted ) teamCol.Set( 75, 75, 75, 255 );
                    
                    glColor4ubv( teamCol.GetData());

                    glBegin( GL_QUADS );
                        glTexCoord2i(0,1);      glVertex2f( pos1.x, pos1.z );
                        glTexCoord2i(1,1);      glVertex2f( pos2.x, pos2.z );
                        glTexCoord2i(1,0);      glVertex2f( pos3.x, pos3.z );
                        glTexCoord2i(0,0);      glVertex2f( pos4.x, pos4.z );
                    glEnd();
                }
            }

            glDisable( GL_TEXTURE_2D );            

        }

    //    if( mapData->m_coop || mapData->m_forceCoop )
    //    {
    //        //g_gameFont.DrawText2D( realX + 400.0f, realY + 10.0f, m_fontSize, "%d Player", numPlayers );
    //        char imageName[512];
    //        sprintf( imageName, "icons/menu_coop.bmp" );
    //        if( mapData->m_forceCoop ) sprintf( imageName, "icons/menu_forcecoop.bmp" );

    //        if( g_app->m_resource->DoesTextureExist( imageName ) )
    //        {        
    //            int texId = g_app->m_resource->GetTexture( imageName );

    //            glEnable( GL_TEXTURE_2D );
    //            glBindTexture( GL_TEXTURE_2D, texId );
    //            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
				//if( IS_DEMO && main->DemoRestrictedMap( mapData->m_mapId ) )
    //            {
    //                glColor4f( 0.2f, 0.2f, 0.2f, 0.7f );
    //            }

    //            int x = coopX;
    //            int w = 32.0f * scale;
    //            int h = 32.0f * scale;
    //            int y = realY + 2;

    //            w *= 0.7f;
    //            h *= 0.7f;

    //            glBegin( GL_QUADS );
    //                glTexCoord2i(0,1);      glVertex2f( x, y );
    //                glTexCoord2i(1,1);      glVertex2f( x+w, y );
    //                glTexCoord2i(1,0);      glVertex2f( x+w, y + h );
    //                glTexCoord2i(0,0);      glVertex2f( x, y + h );
    //            glEnd();

    //            glDisable( GL_TEXTURE_2D );
    //        }     
    //    }

		if( highlighted && GameMenuButton::s_previousHighlightButton != this )
		{
			g_app->m_soundSystem->TriggerOtherEvent( NULL, "MenuRollOver", SoundSourceBlueprint::TypeMultiwiniaInterface );
			GameMenuButton::s_previousHighlightButton = this;
		}
    }
};

#endif
