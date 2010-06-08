#ifndef __GAMESERVERBUTTON__
#define __GAMESERVERBUTTON__

#include "interface/scrollbar.h"
#include "game_menu.h"
#include "lib/text_renderer.h"
#include "achievement_tracker.h"

class GameServerButton : public GameMenuButton 
{
public:
	GameServerButton( int _serverNumber )
		: m_serverNumber( _serverNumber ),
		GameMenuButton("" )
	{
		sprintf( m_name, "server%d", _serverNumber );
	}

	bool IsServerFull( Directory *server  )
	{
	    m_maxPlayers = server->GetDataChar(NET_METASERVER_MAXTEAMS);
	    m_numPlayers = server->GetDataChar(NET_METASERVER_NUMTEAMS);
        m_numHumanPlayers = server->GetDataChar(NET_METASERVER_NUMHUMANTEAMS);

        bool isRunning = server->GetDataUChar( NET_METASERVER_GAMEINPROGRESS );

        if( m_numHumanPlayers == '?' || !isRunning ) m_numHumanPlayers = m_numPlayers;

	    return m_numPlayers == m_maxPlayers;
	}
    
    bool IsLanGame( Directory *server )
    {
        return( server->HasData( NET_METASERVER_LANGAME ) );
    }

    bool IsDemoRestricted( Directory *server )
    {
        return ( server->HasData( NET_METASERVER_DEMORESTRICTED ) );
    }

    void MouseUp()
    {
        GameMenuButton::MouseUp();

		Directory *server = GetServer();
		if( !server )
			return;

		GameMenuWindow *parent = (GameMenuWindow *) m_parent;
		int gameMode = server->GetDataChar( NET_METASERVER_GAMEMODE );
		int mapCRC = server->GetDataInt( NET_METASERVER_MAPCRC );
		int mapIndex = g_app->m_gameMenu->GetMapIndex( gameMode, mapCRC );
        char *theirVersion = server->GetDataString( NET_METASERVER_GAMEVERSION );

		if( !IsProtocolMatch( server ) )
		{            
            parent->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_incompatible"), GameMenuWindow::PageJoinGame );
		}
        else if( server->HasData( NET_METASERVER_DEMORESTRICTED ) )
        {
            parent->m_newPage = GameMenuWindow::PageBuyMeNow;
        }
#ifdef TESTBED_ENABLED
		else if( IsServerFull( server ) && !g_app->GetTestBedMode() == TESTBED_ON )
#else
		else if( IsServerFull( server ))
#endif
		{
			parent->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_serverfull"), GameMenuWindow::PageJoinGame );
		}
		else if( mapIndex == -1 )
		{
			parent->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_missingmap"), GameMenuWindow::PageJoinGame );
		}
        else if( server->HasData( NET_METASERVER_HASPASSWORD ) )
        {
            parent->m_serverIndex = GetServerIndex();
            parent->m_newPage = GameMenuWindow::PageEnterPassword;
        }
		else
		{
#ifdef LOCATION_EDITOR
            if( mapIndex == -1 )
            {
                g_app->m_clientToServer->m_requestCustomMap = true;
            }
            else
            {
                g_app->m_clientToServer->m_requestCustomMap = false;
            }
#endif
			parent->JoinServer( GetServerIndex() );
		}
    }

	int GetServerIndex()
	{
		GameMenuWindow *parent = (GameMenuWindow *) m_parent;

		return m_serverNumber + parent->m_scrollBar->m_currentValue;
	}

	Directory *GetServer()
	{
		int index = GetServerIndex();
		
		GameMenuWindow *parent = (GameMenuWindow *) m_parent;

		if( !parent->m_serverList->ValidIndex( index ) )
			return NULL;

		return parent->m_serverList->GetData( index );
	}

    void Render(int realX, int realY, bool highlighted, bool clicked)
    {
		m_inactive = false;
		m_caption = UnicodeString();

        GameMenuWindow *parent = (GameMenuWindow *) m_parent;

		Directory *server = GetServer();

		if( !server )
		{
    	    m_inactive = true;
			return;
		}

		bool isFull = IsServerFull( server );
		bool isProtocolMatch = IsProtocolMatch( server );
        bool isLan = IsLanGame( server );
        bool demoRestricted = IsDemoRestricted( server );

		if( !m_mouseHighlightMode )
		{
			highlighted = false;
		}

		if( parent->m_buttonOrder[parent->m_currentButton] == this )
		{
			highlighted = true;
		}

		if( highlighted )
		{
			//RenderHighlightBeam( realX, realY+m_h/2.0f, m_w, m_h );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

			if( !isProtocolMatch )
			{
				glColor4f( 0.3f, 0.0f, 0.0f, 0.5f );
			}
			else if( !isFull )
			{
				glColor4f( 0.75f, 0.75f, 1.0f, 0.5f );
			}
			else
			{
				glColor4f( 0.3f, 0.3f, 0.3f, 0.5f );
			}

			glBegin( GL_QUADS);
				glVertex2f( realX, realY );
				glVertex2f( realX + m_w, realY );
				glVertex2f( realX + m_w, realY + m_h );
				glVertex2f( realX, realY + m_h );
			glEnd();
		}

		UpdateButtonHighlight();

        int gameMode = server->GetDataChar( NET_METASERVER_GAMEMODE );
        int mapCRC = server->GetDataInt( NET_METASERVER_MAPCRC );
        int mapIndex = g_app->m_gameMenu->GetMapIndex( gameMode, mapCRC );
        const char *mapName = g_app->m_gameMenu->GetMapName( gameMode, mapCRC );

        char mapNameString[512];
        
        if( mapName )
        {
            sprintf( mapNameString, LANGUAGEPHRASE(mapName) );        
        }
        else
        {
#ifdef LOCATION_EDITOR
            if( server->HasData( NET_METASERVER_CUSTOMMAPNAME ) )
            {
                sprintf( mapNameString, server->GetDataString( NET_METASERVER_CUSTOMMAPNAME ) );
            }
            else
#endif
            {
                sprintf( mapNameString, LANGUAGEPHRASE("multiwinia_error_unknownmap") );
            }
        }

        if( demoRestricted )
        {
            sprintf( mapNameString, LANGUAGEPHRASE("authentication_demorestricted") );
        }

		if( !isProtocolMatch )
        {
            char *version = server->GetDataString( NET_METASERVER_GAMEVERSION );
            sprintf( mapNameString, "INCOMPATIBLE : %s", version );
        }

        if( isLan && mapName )
        {
//            sprintf( mapNameString, "[LAN] %s", LANGUAGEPHRASE(mapName) );
        }

		char gameModeName[512];
		GameTypeToShortCaption( gameMode, gameModeName );
		m_gameType = LANGUAGEPHRASE(gameModeName);
		m_mapName = LANGUAGEPHRASE(mapName);
		m_maxPlayers = server->GetDataChar(NET_METASERVER_MAXTEAMS);
		m_numPlayers = server->GetDataChar(NET_METASERVER_NUMTEAMS);
		m_hostName = UnicodeString(server->GetDataString( NET_METASERVER_SERVERNAME ));

        parent->m_highlightedGameType = gameMode;
        parent->m_highlightedLevel = mapIndex;

		RGBAColour col( 255*0.6f, 255*1.0f, 255*0.4f, 255*1.0f );

		if( highlighted )
		{
			if(!isProtocolMatch)
			{
				col.Set( 255*0.3f, 255*0.0f, 255*0.0f, 255*1.0f );
			}
			else if(!isFull)
			{
				col.Set( 255*1.0f, 255*0.3f, 255*0.3f, 255*1.0f );
			}
			else if( mapIndex == -1 )
			{
				col.Set( 100,100,100 );
			}
			else
			{
				col.Set( 255*0.3f, 255*0.3f, 255*0.3f, 255*1.0f );
			}
		}
		else {
			if( !isProtocolMatch )
			{
				col.Set( 255*0.5f, 255*0.0f, 255*0.0f, 255*1.0f );
			}
            else if( isLan )
            {
                col.Set( 150, 150, 255, 255 );
            }
            else if( demoRestricted )
            {
                col.Set( 255*1.0f, 255*1.0f, 255*0.4f, 255*1.0f );
            }
			else if( !isFull )
			{
				col.Set( 255*0.6f, 255*1.0f, 255*0.4f, 255*1.0f );
			}
			else if( mapIndex == -1 )
			{
				col.Set( 100,100,100 );
			}
			else
			{
				col.Set( 255*0.5f, 255*0.5f, 255*0.5f, 255*1.0f );
			}
		}

		int x = realX;
		int y = realY + m_h/2.0f - m_fontSize/2.0f + 7;// - m_fontSize / 4.0f;
		x += m_w * 0.05f;
		g_editorFont.SetRenderOutline(true);

        if( server->HasData( NET_METASERVER_HASPASSWORD ) )
        {
            glEnable        ( GL_TEXTURE_2D );
            glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/locked.bmp" ) );
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

            glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

            int lockX = realX + m_w * 0.01f;
            int lockY = realY + m_h * 0.1f;
            
            glBegin( GL_QUADS );
                glTexCoord2i(0,1);      glVertex2f( lockX, lockY );
                glTexCoord2i(1,1);      glVertex2f( lockX + m_h * 0.8f, lockY );
                glTexCoord2i(1,0);      glVertex2f( lockX + m_h * 0.8f, lockY + m_h * 0.8f );
                glTexCoord2i(0,0);      glVertex2f( lockX, lockY + m_h * 0.8f );
            glEnd();

            glDisable       ( GL_TEXTURE_2D );
        }

		if( !isProtocolMatch )
		{
			glColor4f(0.2f,0.0f,0.0f,0.0f);
		}
		else if( !isFull )
		{
			glColor4f(1.0f,1.0f,1.0f,0.0f);
		}
		else
		{
			glColor4f(0.2f,0.2f,0.2f,0.0f);
		}

		g_editorFont.DrawText2D( x, y, m_fontSize, m_hostName );
		g_editorFont.SetRenderOutline(false);
		glColor4ubv(col.GetData());
		g_editorFont.DrawText2D( x, y, m_fontSize, m_hostName );

		x += m_w * 0.25f;
		g_editorFont.SetRenderOutline(true);
		if( !isProtocolMatch )
		{
			glColor4f(0.2f,0.0f,0.0f,0.0f);
		}
		else if( !isFull )
		{
			glColor4f(1.0f,1.0f,1.0f,0.0f);
		}
		else
		{
			glColor4f(0.2f,0.2f,0.2f,0.0f);
		}
		g_editorFont.DrawText2D( x, y, m_fontSize, m_gameType );
		g_editorFont.SetRenderOutline(false);
		glColor4ubv(col.GetData());
		g_editorFont.DrawText2D( x, y, m_fontSize, m_gameType );

		x += m_w * 0.1f;
		g_editorFont.SetRenderOutline(true);
		if( !isProtocolMatch )
		{
			glColor4f(0.2f,0.0f,0.0f,0.0f);
		}
		else if( !isFull )
		{
			glColor4f(1.0f,1.0f,1.0f,0.0f);
		}
		else
		{
			glColor4f(0.2f,0.2f,0.2f,0.0f);
		}
		g_editorFont.DrawText2D( x, y, m_fontSize, mapNameString );
		g_editorFont.SetRenderOutline(false);
		glColor4ubv(col.GetData());
		g_editorFont.DrawText2D( x, y, m_fontSize, mapNameString );

		x += m_w * 0.5f;
		g_editorFont.SetRenderOutline(true);

		if( !isProtocolMatch )
		{
			glColor4f(0.2f,0.0f,0.0f,0.0f);
		}
		else if( !isFull )
		{
			glColor4f(1.0f,1.0f,1.0f,0.0f);
		}
		else
		{
			glColor4f(0.2f,0.2f,0.2f,0.0f);
		}
		g_editorFont.DrawText2D( x, y, m_fontSize, "(%d/%d)", m_numPlayers, m_maxPlayers );
		g_editorFont.SetRenderOutline(false);
		glColor4ubv(col.GetData());
		g_editorFont.DrawText2D( x, y, m_fontSize, "(%d/%d)", m_numPlayers, m_maxPlayers );

        float xPos = x - 80;
        float yPos = m_y + 4;
        float height = m_h - 8;
        float width = height * 0.66f;

        for( int i = 0; i < m_maxPlayers; ++i )
        {
            glColor4ubv( col.GetData() );

            if( i < m_numPlayers ) 
            {
                if( i < m_numHumanPlayers ) glColor4ubv( col.GetData() );
                else                        glColor4ubv( (col * 0.6f).GetData() );

                glBegin( GL_QUADS );
                    glVertex2f( xPos, yPos );
                    glVertex2f( xPos+width, yPos );
                    glVertex2f( xPos+width, yPos+height );
                    glVertex2f( xPos, yPos+height );
                glEnd();
            }

            glColor4ubv( col.GetData() );

            glBlendFunc( GL_SRC_ALPHA, GL_ONE );
            glBegin( GL_LINE_LOOP );
                glVertex2f( xPos, yPos );
                glVertex2f( xPos+width, yPos );
                glVertex2f( xPos+width, yPos+height );
                glVertex2f( xPos, yPos+height );
            glEnd();
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            xPos += width + 2;
        }
    }

	bool SpecialScroll( int direction )
	{
		GameMenuWindow *parent = (GameMenuWindow *)m_parent;
		int realNum = GetServerIndex();
		if( direction == 1 &&
			m_serverNumber == 0 &&
			parent->m_serverList->ValidIndex( realNum + SERVERS_ON_SCREEN)) 
		{
			if( parent->m_scrollBar->m_currentValue + 1 <  parent->m_scrollBar->m_numRows )
			{
				 parent->m_scrollBar->m_currentValue++;
				 return true;
			}
		}
		else if( direction == -1 &&
			m_serverNumber + 1 == SERVERS_ON_SCREEN )
		{
			if( parent->m_scrollBar->m_currentValue > 0 )
			{
				parent->m_scrollBar->m_currentValue--;
				return true;
			}

		}

		return false;
	}

	int m_serverNumber;
	UnicodeString m_hostName;
	UnicodeString m_mapName;
	UnicodeString m_gameType;
	int m_numPlayers;
	int m_maxPlayers;
    int m_numHumanPlayers;
		
};

#endif
