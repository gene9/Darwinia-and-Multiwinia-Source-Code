#ifndef __WAITINGBUTTON__
#define __WAITINGBUTTON__

class WaitingButton : public DarwiniaButton
{
public:
	char *m_string;
	WaitingButton(char *_string)
	:	DarwiniaButton(),
		m_string(strdup(_string))
	{
	}

	~WaitingButton()
	{
		delete m_string;
		m_string = NULL;
	}

	void Render( int realX, int realY, bool highlighted, bool clicked )
	{
        int originalY = realY;

        for( int x = 0; x < 2; ++x )
        {
            realY = originalY;

            if( x == 0 )
            {
                g_titleFont.SetRenderOutline(true);
                g_editorFont.SetRenderOutline(true);
                glColor4f(1.0f,1.0f,1.0f,0.0f);
            }
            else
            {
                g_titleFont.SetRenderOutline(false);
                g_editorFont.SetRenderOutline(false);
                glColor4f(1.0f,1.0f,1.0f,1.0f);
            }

		    UnicodeString string;
            
            switch( g_app->m_clientToServer->m_connectionState )
            {
                case ClientToServer::StateConnecting:
                {
                    string = LANGUAGEPHRASE(m_string);
		            int dots = (int)(GetHighResTime()*2) % 4;
		            for( int i = 0; i < dots; ++i )
		            {
			            string = string + UnicodeString(".");
		            }
                    break;
                }

                case ClientToServer::StateHandshaking:
                case ClientToServer::StateConnected:
                    string = LANGUAGEPHRASE("multiwinia_menu_connected");
                    break;
            }

		    g_titleFont.DrawText2D( realX, realY, m_fontSize, string );

            realY += m_fontSize*1.5f;

            UnicodeString attemptsString;

            if( g_app->m_clientToServer->m_connectionState == ClientToServer::StateConnecting )
            {
                char attempts[256];
                sprintf( attempts, "%d", g_app->m_clientToServer->m_connectionAttempts );
                attemptsString = LANGUAGEPHRASE("multiwinia_menu_attempts");
                attemptsString.ReplaceStringFlag( L'T', UnicodeString(attempts) );
            }
            else
            {
                int serverSeqId = g_app->m_clientToServer->m_serverSequenceId;
                int latestSeqId = g_lastProcessedSequenceId;
                float percentageDone = 100.0f * (float)latestSeqId / (float)serverSeqId;
                char percent[256];
                sprintf( percent, "%2.0f", percentageDone  );
                attemptsString = LANGUAGEPHRASE("multiwinia_menu_resyncing");
                attemptsString.ReplaceStringFlag( L'T', UnicodeString(percent) );
            }
            
            g_titleFont.DrawText2D( realX, realY, m_fontSize*0.6f, attemptsString );


            //
            // Extra data on the server being joined can be found in our parent

            GameMenuWindow *parent = (GameMenuWindow *)m_parent;
            Directory *server = parent->m_joinServer;

            if( server )
            {
                realY += m_fontSize*2.0f;

                int gameMode = server->GetDataChar( NET_METASERVER_GAMEMODE );
                int mapCRC = server->GetDataInt( NET_METASERVER_MAPCRC );
                int mapIndex = g_app->m_gameMenu->GetMapIndex( gameMode, mapCRC );

                //char *serverIp = server->GetDataString( NET_METASERVER_IP );
                //int serverPort = server->GetDataInt( NET_METASERVER_PORT );

                char serverIp[256];
                g_app->m_clientToServer->GetServerIp( serverIp );  
				int serverPort = g_app->m_clientToServer->GetServerPort();
                int max_teams = server->GetDataChar( NET_METASERVER_MAXTEAMS );
                int num_teams = server->GetDataChar( NET_METASERVER_NUMTEAMS );

                const char *mapName = g_app->m_gameMenu->GetMapName( gameMode, mapCRC );
                if( !mapName ) mapName = "Unknown Map";

                char gameModeName[512];
                GameTypeToCaption( gameMode, gameModeName );
                
                UnicodeString gameType = LANGUAGEPHRASE(gameModeName);            
                UnicodeString mapNameU = LANGUAGEPHRASE(mapName);
                UnicodeString hostName = UnicodeString(server->GetDataString( NET_METASERVER_SERVERNAME ));

                float dataX = realX + 200;
                float fontSize = m_fontSize * 0.5f;
                float gapSize = m_fontSize * 0.7f;

                g_editorFont.DrawText2D( realX, realY += gapSize, fontSize, LANGUAGEPHRASE("multiwinia_join_host" ) );
                g_editorFont.DrawText2D( dataX, realY, fontSize, hostName );

                g_editorFont.DrawText2D( realX, realY += gapSize, fontSize, LANGUAGEPHRASE("multiwinia_join_address") );
                g_editorFont.DrawText2D( dataX, realY, fontSize, "%s, port %d", serverIp, serverPort );
                
                g_editorFont.DrawText2D( realX, realY += gapSize, fontSize, LANGUAGEPHRASE("multiwinia_gametype") );
                g_editorFont.DrawText2D( dataX, realY, fontSize, gameType );

                g_editorFont.DrawText2D( realX, realY += gapSize, fontSize, LANGUAGEPHRASE("multiwinia_join_map") );
                g_editorFont.DrawText2D( dataX, realY, fontSize, mapNameU );   

                g_editorFont.DrawText2D( realX, realY += gapSize, fontSize, LANGUAGEPHRASE("multiwinia_join_players") );
                g_editorFont.DrawText2D( dataX, realY, fontSize, "%d/%d", num_teams, max_teams );   


            }
        }
	}
};

#endif
