#ifndef __MAPDETAILSBUTTON__
#define __MAPDETAILSBUTTON__

class MapDetailsButton : public DarwiniaButton
{
public:
    void Render( int realX, int realY, bool highlighted, bool clicked )
    {
        GameMenuWindow *main = (GameMenuWindow *)EclGetWindow( /*"GameMenu"*/"multiwinia_mainmenu_title" );
        if( !main || main->m_gameType < 0 || main->m_gameType >= MAX_GAME_TYPES ) return;
        DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[main->m_gameType];
        int levelId = main->m_requestedMapId;
        if( !maps.ValidIndex(levelId) ) return;
        
        int playTime = g_app->m_multiwinia->GetGameOption( "TimeLimit" );
        /*if( playTime == 0 )
        {
            int id = GetTimeOption();

            if( id != -1 )
            {
                MultiwiniaGameBlueprint *blueprint = Multiwinia::s_gameBlueprints[main->m_gameType];
                playTime = blueprint->m_params[id]->m_
            }
        }*/

        int gap = m_fontSize * 1.05f;

        if( playTime > 0)
        {
            g_gameFont.DrawText2D( realX, realY += gap, m_fontSize, LANGUAGEPHRASE("multiwinia_playtime") );
            g_gameFont.DrawText2D( realX, realY += gap, m_fontSize, "%d mins", playTime );
        }

        realY += gap;

        if( maps[levelId]->m_difficulty )
        {
            //char diffString[256];
            //sprintf( diffString, "multiwinia_complexity_%s", maps[m_levelId]->m_difficulty );
            //g_gameFont.DrawText2D( realX + 360.0f, realY + 10.0f, m_fontSize, LANGUAGEPHRASE(diffString) );
            g_gameFont.DrawText2D( realX, realY+=gap, m_fontSize, LANGUAGEPHRASE("multiwinia_difficulty") );
            char imageName[512];
            
            if( strcmp( maps[levelId]->m_difficulty, "basic" ) == 0 )         sprintf( imageName, "icons/menu_basic.bmp" );            
            else if( strcmp( maps[levelId]->m_difficulty, "intermediate" ) == 0 )   sprintf( imageName, "icons/menu_intermediate.bmp" );                            
            else if( strcmp( maps[levelId]->m_difficulty, "advanced" ) == 0 ) sprintf( imageName, "icons/menu_advanced.bmp" );
            

            if( g_app->m_resource->DoesTextureExist( imageName ) )
            {        
                int texId = g_app->m_resource->GetTexture( imageName );

                glEnable( GL_TEXTURE_2D );
                glBindTexture( GL_TEXTURE_2D, texId );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

                int x = realX - 2;
                int y = realY += gap;
                int w = m_fontSize * 2.0f;
                int h = m_fontSize;

                glBegin( GL_QUADS );
                    glTexCoord2i(0,1);      glVertex2f( x, y );
                    glTexCoord2i(1,1);      glVertex2f( x+w, y );
                    glTexCoord2i(1,0);      glVertex2f( x+w, y + h );
                    glTexCoord2i(0,0);      glVertex2f( x, y + h );
                glEnd();

                glDisable( GL_TEXTURE_2D );
            }     
        }

        realY += gap;

        if( maps[levelId]->m_numPlayers > 0 )
        {
            //g_gameFont.DrawText2D( realX + 400.0f, realY + 10.0f, m_fontSize, "%d Player", numPlayers );
            char imageName[512];
            sprintf( imageName, "icons/menu_%dplayer.bmp", maps[levelId]->m_numPlayers );
            g_gameFont.DrawText2D( realX, realY+=gap, m_fontSize, LANGUAGEPHRASE("multiwinia_players") );

            if( g_app->m_resource->DoesTextureExist( imageName ) )
            {        
                int texId = g_app->m_resource->GetTexture( imageName );

                glEnable( GL_TEXTURE_2D );
                glBindTexture( GL_TEXTURE_2D, texId );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE );
                glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

                int x = realX - 2;
                int w = 130.0f;
                int h = 32.0f;
                int y = realY += gap;

                w *= 0.7f;
                h *= 0.7f;

                glBegin( GL_QUADS );
                    glTexCoord2i(0,1);      glVertex2f( x, y );
                    glTexCoord2i(1,1);      glVertex2f( x+w, y );
                    glTexCoord2i(1,0);      glVertex2f( x+w, y + h );
                    glTexCoord2i(0,0);      glVertex2f( x, y + h );
                glEnd();

                glDisable( GL_TEXTURE_2D );
            }     
        }
    }
};

#endif
