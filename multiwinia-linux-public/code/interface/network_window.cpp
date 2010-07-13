#include "lib/universal_include.h"


#include <stdio.h>

#include "lib/text_renderer.h"
#include "lib/profiler.h"
#include "lib/math/math_utils.h"

#include "network/server.h"
#include "network/clienttoserver.h"
#include "network/servertoclient.h"

#include "app.h"
#include "main.h"
#include "multiwinia.h"

#include "interface/network_window.h"
#include "renderer.h"

NetworkWindow::NetworkWindow( char *name )
:   DarwiniaWindow( name )
{
	SetSize( 600, 200 );
}

static void RectFill( float x, float y, float w, float h, RGBAColour const &col )
{
	glColor4ubv( col.GetData() );
    glBegin( GL_QUADS );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();
}

static void Rect ( float x, float y, float w, float h, RGBAColour const &col, float lineWidth = 1.0 )
{
    glLineWidth(lineWidth);
    glColor4ubv( col.GetData() );
    glBegin( GL_LINE_LOOP );
        glVertex2f( x, y );
        glVertex2f( x + w, y );
        glVertex2f( x + w, y + h );
        glVertex2f( x, y + h );
    glEnd();
}

static void Line ( float x1, float y1, float x2, float y2, RGBAColour const &col, float lineWidth )
{
    glLineWidth( lineWidth );
    glColor4ubv( col.GetData() );
    glBegin( GL_LINES );
        glVertex2f( x1, y1 );
        glVertex2f( x2, y2 );
    glEnd();
}


void NetworkWindow::Create()
{
	DarwiniaWindow::Create();

	if( g_app->m_server )
	{
		int maxClients = 6;

		int boxH = maxClients * 20 + 15;
        SetSize( m_w, 170 + boxH );
    
        InvertedBox *box = new InvertedBox();
        box->SetProperties( "invert", 10, 110, m_w-20, boxH, UnicodeString(" "), UnicodeString(" ") );
        RegisterButton( box );
	}

	m_x = 10;
    m_y = g_app->m_renderer->ScreenH() - m_h;
}

void NetworkWindow::Render( bool hasFocus )
{
    DarwiniaWindow::Render( hasFocus );

	
	TextRenderer &f = g_editorFont;
    
    //
    // Render some Networking stats

    int y = m_y + 15;
    int h = 15;

    if( g_app->m_server )
    {		
        glColor4ubv( g_colourWhite.GetData() );
		
		f.DrawText2D( m_x + 10, y+=h, 12, "Server SeqID       %d", g_app->m_server->m_sequenceId );
        f.DrawText2D( m_x + 10, y+=h, 12, "Server Send        %2.1 Kb/sec", (int) g_app->m_server->m_sendRate/1024.0 );
        f.DrawText2D( m_x + 10, y+=h, 12, "Server Receive     %2.1 Kb/sec", (int) g_app->m_server->m_receiveRate/1024.0 );

		Line( m_x + 10, y + 20, m_x + m_w - 10, y + 20, g_colourWhite, 1 );

        int clientX = m_x + 20;
        int ipX = clientX + 60;
        int seqX = ipX + 140;
        int playerX = seqX + 60;
        int lagX = playerX + 150;
        int syncX = lagX + 90;

        y+=h*2;

        f.DrawText2D( clientX, y, 14, "ID" );
        f.DrawText2D( ipX, y,	 14, "IP:Port" );
        f.DrawText2D( seqX, y,	 14, "SeqID" );
        f.DrawText2D( playerX, y, 14, "Name" );
        f.DrawText2D( lagX, y,	 14, "Status" );

        y+=h*2;

		int maxClients = 6;

        for( int i = 0; i < maxClients; ++i )
        {
			if( g_app->m_server->m_clients.ValidIndex(i) )
			{           
                ServerToClient *sToC = g_app->m_server->m_clients[i];

				char netLocation[256];
				sprintf( netLocation, "%s:%d", sToC->m_ip, sToC->m_port );

				double timeBehind = GetHighResTime() - sToC->m_lastMessageReceived;
				char caption[256];
				RGBAColour col;

				if( timeBehind > 2.0 )
                {
					col.Set( 255, 0, 0, 255 );
                    sprintf( caption, "Lost Con...%ds", (int)timeBehind );
                }
                else if( !sToC->m_caughtUp ) 
                {
					col.Set( 200, 200, 30, 255 );
                    int percent = 100 * (sToC->m_lastKnownSequenceId / (float)g_app->m_server->m_sequenceId);
                    sprintf( caption, "Synching...%d", percent );
                }                
				else
                {
                    int latency = (g_app->m_server->m_sequenceId - sToC->m_lastKnownSequenceId);
                    int latencyMs = latency * 100;
                    sprintf( caption, "Ping %dms", latencyMs );
                    unsigned red = std::min(255, latency*20);
                    unsigned green = 255 - red;
                    col.Set(red,green,0,255);                
                }

                RGBAColour normalCol(200,200,255,200);

                UnicodeString playerName;
				RGBAColour playerCol;

				for( int j = 0; j < NUM_TEAMS; ++j )
                {
					LobbyTeam &thisTeam = g_app->m_multiwinia->m_teams[j];

					if( thisTeam.m_clientId == sToC->m_clientId )
                    {
						char * teamType = Team::GetTeamType( thisTeam.m_teamType );
                        playerName = LANGUAGEPHRASE(teamType);
						playerCol = thisTeam.m_colour;
						delete [] teamType;
                        break;
                    }
                }

				glColor4ubv( normalCol.GetData() );
				f.DrawText2D( clientX, y, 12, "%d", sToC->m_clientId );
                f.DrawText2D( ipX, y, 12, "%s", netLocation );
                f.DrawText2D( seqX, y, 12, "%d", sToC->m_lastKnownSequenceId );                

				if( playerName.m_unicodestring )
                {
					glColor4ubv( playerCol.GetData() );
                    f.DrawText2DSimple( playerX, y, 12, playerName );
                }

				glColor4ubv( col.GetData() );
                f.DrawText2D( lagX, y, 12, "%s", caption );

                if( sToC->m_syncErrorSeqId != -1 )
                {
					glColor4ub( 255U, 0, 0, 255U );
                    f.DrawText2D( syncX, y , 12, "out of sync" );
                }
			}
			else 
			{
				glColor4ub( 255U, 255U, 255U, 50 );
				f.DrawText2D( clientX, y, 12, "Empty" );
			}

			y += 20;
		}
	}

    //
    // Show which packets are queued up if we're a client

	RGBAColour packetBackground(20,20,50,255);
	RGBAColour packetSequenced(0,255,0,255);
	RGBAColour packetFrame(255,255,255,100);

	if( g_app->m_clientToServer && g_lastProcessedSequenceId >= 0 )
	{					
        float yPos = m_y + m_h - 25;

		glColor4ubv( g_colourWhite.GetData() );
		f.DrawText2D( m_x + 10, yPos-25, 13, "Sequence messages in queue. Ideal is 0 or 1 filled, no gaps." );

        float xPos = m_x + 10;
        float width = (m_w - 120) / 10;
        float gap = width * 0.2;
        
        for( int i = 0; i <= 10; ++i )
        {
            if( i != 0 )
            {
                RectFill( xPos, yPos-5, width-gap, 20, packetBackground );

                if( g_app->m_clientToServer->IsSequenceIdInQueue( g_lastProcessedSequenceId+i ) )
                {
                    RectFill( xPos, yPos-5, width-gap, 20, packetSequenced );
                }

                Rect( xPos, yPos-5, width-gap, 20, packetFrame );
            }

            if( i == 0 )
            {
				glColor4ubv( g_colourWhite.GetData() );
				f.DrawText2D( xPos, yPos, 13, "SeqID %d", g_lastProcessedSequenceId );
                xPos += width * 1;
            }
            
            xPos += width;
        }
	 }

	if( g_app->m_clientToServer )
    {
		glColor4ubv( g_colourWhite.GetData() );
        if( !g_app->m_server )
        {
            f.DrawText2D( m_x + 10, y+=h, 12, "Server Known     SeqID   %d", g_app->m_clientToServer->m_serverSequenceId );
            f.DrawText2D( m_x + 10, y+=h, 12, "Server Estimated SeqID   %d", g_app->m_clientToServer->GetEstimatedServerSeqId() );
			f.DrawText2DCentre( m_x + m_w/2, y+40, 
				15, "Estimated Latency : %2.1 secs", g_app->m_clientToServer->GetEstimatedLatency() );
        }

        y = m_y + 15;
        f.DrawText2D( m_x + 250, y+=h, 12, "Client SEQID       %d", g_app->m_clientToServer->m_lastValidSequenceIdFromServer );
        f.DrawText2D( m_x + 250, y+=h, 12, "Client Send        %2.1 Kb/sec", (int) g_app->m_clientToServer->m_sendRate/1024.0 );
        f.DrawText2D( m_x + 250, y+=h, 12, "Client Receive     %2.1 Kb/sec", (int) g_app->m_clientToServer->m_receiveRate/1024.0 );
    }

}




