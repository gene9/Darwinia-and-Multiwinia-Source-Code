#include "lib/universal_include.h"

#include "lib/netlib/net_lib.h"
#include "lib/netlib/net_mutex.h"
#include "lib/netlib/net_socket.h"
#include "lib/netlib/net_socket_listener.h"
#include "lib/netlib/net_thread.h"
#include "lib/netlib/net_udp_packet.h"

#include "lib/hi_res_time.h"
#include "lib/debug_utils.h"
#include "lib/string_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/input/input.h"
#include "lib/math/random_number.h"
#include "lib/network_stream.h"

#include <time.h>
#include <strstream>

#include "app.h"
#include "main.h"
#include "location.h"
#include "taskmanager.h"
#include "team.h"
#include "global_world.h"
#include "multiwinia.h"
#include "particle_system.h"
#include "game_menu.h"
#include "demo_limitations.h"
#include "markersystem.h"
#include "global_world.h"

#include "worldobject/factory.h"
#include "worldobject/radardish.h"
#include "worldobject/engineer.h"
#include "worldobject/laserfence.h"
#include "worldobject/shaman.h"
#include "worldobject/officer.h"   
#include "worldobject/armour.h"

#include "network/server.h"
#include "network/clienttoserver.h"
#include "network/servertoclientletter.h"
#include "network/generic.h"
#include "network/network_defines.h"
#include "network/iframe.h"
#include "network/ftp_manager.h"

#include "lib/metaserver/matchmaker.h"
#include "lib/metaserver/metaserver.h"
#include "lib/metaserver/authentication.h"
#include "lib/work_queue.h"

#include "lib/metaserver/metaserver_defines.h"

#include "UI/GameMenuWindow.h"

#include "achievement_tracker.h"

//#define NETWORKQUALITY_TESTING

#ifdef NETWORKQUALITY_TESTING
#include "network/network_quality.h"
#endif



static NetCallBackRetType ClientToServerListenCallback(NetUdpPacket *udpdata)
{
    static int s_bytesReceived = 0;
    static double s_timer = 0.0;
    static double s_interval = 5.0;

	if (udpdata)
    {
        NetIpAddress *fromAddr = &udpdata->m_clientAddress;
        char newip[16];
		IpToString(fromAddr->sin_addr, newip);

        Directory *letter = new Directory();
        bool success = letter->Read(udpdata->m_data, udpdata->m_length);

		if( success )
        {
            g_app->m_clientToServer->ReceiveLetter( letter, udpdata->m_length );
        }
        else
        {
            AppDebugOut( "Client received bogus data, discarded (1)\n" );
            delete letter;
        }

        delete udpdata;
    }

    return 0;
}

static
void ListenThread()
{
    NetRetCode retCode = g_app->m_clientToServer->m_listener->StartListening( ClientToServerListenCallback );
    AppDebugAssert( retCode == NetOk );
}

ClientToServer::ClientToServer()
:    
	m_serverIp( NULL ),
	m_sendSocket( NULL ),
	m_listener( NULL ),
	m_listenThread( NULL ),
    m_lastValidSequenceIdFromServer( -1 ),
	m_serverSequenceId( -1 ),
    m_lastServerLetterReceivedTime( 0.0 ),
    m_pingSentTime( 0.0 ),
    m_latency( 0.0 ),
    m_clientId( -1 ),
	m_connectionState( StateDisconnected ),
    m_retryTimer(0.0f),
	m_retryTimerFast(0.0f),
	m_retryTimerSlow(0.0),
    m_connectionAttempts(0),
	m_bSpectator(false),
	m_syncErrorSeqId(-1),
    m_christmasMode(false)
{
    m_inboxMutex = new NetMutex();
    m_outboxMutex = new NetMutex();

	m_ftpManager = new FTPManager( "CLIENT" );

    strcpy( m_serverVersion, "1.0" );

	memset( &m_gameOptionArrayVersion, 0, sizeof( m_gameOptionArrayVersion ) );

	int gameOptionArraySize = 64;
	m_gameOptionArrayVersion[GAMEOPTION_GAME_PARAM] = new int[gameOptionArraySize ];
	m_gameOptionArrayVersion[GAMEOPTION_GAME_RESEARCH_LEVEL] = new int[ gameOptionArraySize ];

	ResetGameOptionVersions();

	m_requestedTeamDetails.m_colourId = -1;
	m_requestedTeamDetails.m_name = "";
}

void ClientToServer::OpenConnections()
{
    if( m_listener )
    {
        m_listener->StopListening();
        //delete m_listener;
        m_listener = NULL;
    }
    
	// Don't listen if you're going to be communicating directly
	/*if( g_app->m_server )
		return;*/

    //
    // Try the requested port number

    int port = g_prefsManager->GetInt( PREFS_NETWORKCLIENTPORT, 4001 );
    m_listener = new NetSocketListener( port, "ClientToServer" );
    NetRetCode result = m_listener->Bind();

    
    //
    // If that failed, try port 0

    if( result != NetOk && port != 0 )
    {
        AppDebugOut( "Client failed to bind to port %d\n", port );
        AppDebugOut( "Client looking for any available port number...\n" );
        delete m_listener;

        m_listener = new NetSocketListener( 0, "ClientToServer" );
        result = m_listener->Bind();
    }


    //
    // If it still failed, bail out
    // (we're in trouble)

    if( result != NetOk )
    {
        AppDebugOut( "ERROR : Client failed to bind to open Listener\n" );
        delete m_listener;
        m_listener = NULL;
        return;
    }


    //
    // All ok

	if( !m_listenThread )
		m_listenThread = new WorkQueue;
	m_listenThread->Add( &ListenThread );

	AppDebugOut( "Client listening on port %d\n", GetLocalPort() );     
}

ClientToServer::~ClientToServer()
{
    while( m_outbox.Size() > 0 ) {}

	SAFE_DELETE(m_sendSocket);
	if( m_listener )
	{
		m_listener->StopListening();
		SAFE_DELETE( m_listenThread );
	}
	SAFE_DELETE(m_listener);
	SAFE_DELETE_ARRAY(m_serverIp);

	m_inbox.EmptyAndDelete();
	m_outbox.EmptyAndDelete();
	m_iframeHistory.EmptyAndDelete();

	SAFE_DELETE(m_inboxMutex);
	SAFE_DELETE(m_outboxMutex);
	SAFE_DELETE(m_ftpManager);

	for( int i = 0; i < GAMEOPTION_NUMGAMEOPTIONS; i++ )
	{
		delete [] m_gameOptionArrayVersion[i]; 
		m_gameOptionArrayVersion[i] = NULL;
	}
}


void ClientToServer::AdvanceAndCatchUp()
{
	if( g_app->m_loadingLocation )
	{
		KeepAlive();
		return;
	}

	bool catchUp;
	bool spentTooLong;

	double startTime = GetHighResTime();
	int iterations = 0;

	do
	{
		catchUp = g_lastProcessedSequenceId < m_lastValidSequenceIdFromServer - 20 && m_clientId != -1;

		Advance();
		
		ProcessAllServerLetters( catchUp );

		// If we're loading the location now, we need to do that first
		// before processing any further messages
		if( g_app->m_loadingLocation )
			return;

		// Set a hard limit of 0.3 seconds to spend on catching up 
		// so that we don't hold up the renderer too much
		// (time checked every 500 iterations)

		spentTooLong = (GetHighResTime() - startTime > 0.3);

	} while (catchUp && !spentTooLong);
}

int ClientToServer::GetLocalPort()
{
    if( !m_listener ) return -1;

    return m_listener->GetPort();
}


void ClientToServer::GetServerIp( char *_ip )
{
    if( m_serverIp )
    {
        strcpy( _ip, m_serverIp );
    }
    else
    {
        strcpy( _ip, "unknown" );
    }
}

int ClientToServer::GetServerPort()
{
    return m_serverPort;
}

void ClientToServer::StartIdentifying()
{
    MatchMaker_StartRequestingIdentity( m_listener );
}


void ClientToServer::StopIdentifying()
{
    MatchMaker_StopRequestingIdentity( m_listener );
}

bool ClientToServer::GetIdentity( char *_ip, int *_port )
{
    return MatchMaker_GetIdentity( m_listener, _ip, _port );
}

void ClientToServer::AdvanceSender()
{
    static int s_bytesSent = 0;
    static float s_timer = 0;
    static float s_interval = 5.0f;
    static int s_largest = 0;

	int bytesSentThisFrame = 0;
    m_outboxMutex->Lock();

    while (m_outbox.Size())
    {
		if (m_connectionState > StateDisconnected) 
		{
			Directory *letter = m_outbox[0];
			AppDebugAssert(letter);

			letter->CreateData( NET_DARWINIA_LASTSEQID, m_lastValidSequenceIdFromServer );

			// Send directly to locally connected server without passing through
			// the socket system
			if( g_app->m_server )
			{
				g_app->m_server->ReceiveLetter( letter, GetOurIP_String(), GetLocalPort() );
			}
			else
			{
				int letterSize = 0;
				char byteStream[16384];
				std::ostrstream o( byteStream, sizeof(byteStream) );
				letter->Write( o );
				o.str();
				letterSize = o.tellp();

				NetSocketSession *socket = m_sendSocket;
				int writtenData = 0;
				
#ifdef NETWORKQUALITY_TESTING
				static NetworkQuality q( 200, 0.25, 8000 );
				NetRetCode result = q.Write( socket, byteStream, letterSize, &writtenData );
#else
				NetRetCode result = socket->WriteData( byteStream, letterSize, &writtenData );
#endif

				if( result != NetOk )           AppDebugOut("CLIENT write data bad result %d\n", (int) result );
				if( writtenData != 0 &&
					writtenData < letterSize )  AppDebugOut("CLIENT wrote less data than requested\n" );

				int totalSize = letterSize + UDP_HEADER_SIZE;
				s_bytesSent += totalSize;

				if( totalSize > s_largest )
				{
					s_largest = totalSize;
					AppDebugOut( "CLIENT: Largest datagram sent : %d bytes\n", s_largest );
#ifdef TARGET_DEBUG
					letter->DebugPrint(0);
#endif
				}


				bytesSentThisFrame += letterSize;
				delete letter;
			}
		}
        m_outbox.RemoveData(0);
    }
    m_outboxMutex->Unlock();

	m_sendRate.Count( bytesSentThisFrame );
}


void ClientToServer::Advance()
{
    char authKey[256];
    Authentication_GetKey( authKey );

    bool demoKey = Authentication_IsDemoKey( authKey );

	Directory clientProperties;
    clientProperties.CreateData( NET_METASERVER_AUTHKEY, authKey );
    clientProperties.CreateData( NET_METASERVER_GAMENAME, APP_NAME );
    clientProperties.CreateData( NET_METASERVER_GAMEVERSION, APP_VERSION );

    if( g_app->m_originVersion ) 
    {
        clientProperties.CreateData( NET_METASERVER_ORIGINVERSION, g_app->m_originVersion );
    }

    MetaServer_SetClientProperties( &clientProperties );

    //
    // If we are disconnected burn all messages in the inbox

    if( m_connectionState == StateDisconnected )
    {
        m_inboxMutex->Lock();
        m_inbox.EmptyAndDelete();
        m_inboxMutex->Unlock();

		g_sliceNum = -1;
#ifdef TRACK_SYNC_RAND
	    FlushSyncRandLog();
#endif
    }

    //
    // Auto-retry to connect to the server until successful
	double timeNow = GetHighResTime();

    if( m_connectionState == StateConnecting && timeNow > m_retryTimer )
    {
		//
        // Attempt to hole punch
        char ourIp[256];
        int ourPort = -1;

		int usePortForwarding = g_prefsManager->GetInt( PREFS_NETWORKUSEPORTFORWARDING, 0 );

        bool identityKnown = GetIdentity( ourIp, &ourPort );

		if( usePortForwarding )
			ourPort = m_listener->GetPort();

        //bool identityKnown = GetIdentity( "142.217.199.76", &ourPort );

        if( identityKnown )
        {
            char authKey[256];
            Authentication_GetKey( authKey );

            AppAssert( m_serverIp );
            Directory ourDetails;
            
            ourDetails.CreateData( NET_METASERVER_GAMENAME, APP_NAME );
            ourDetails.CreateData( NET_METASERVER_GAMEVERSION, APP_VERSION );
            ourDetails.CreateData( NET_METASERVER_AUTHKEY, authKey );
            ourDetails.CreateData( NET_METASERVER_IP, ourIp );
            ourDetails.CreateData( NET_METASERVER_PORT, ourPort );

			AppDebugOut("Our details: %s:%d\n", ourIp, ourPort );
            MatchMaker_RequestConnection( m_listener, m_serverIp, m_serverPort, &ourDetails );
}
		else
		{
			AppDebugOut("CLIENT identity unknown.\n" );
		}

		// Attempt to connect directly
		Directory *letter = new Directory();
		letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CLIENT_JOIN );
		letter->CreateData( NET_METASERVER_GAMEVERSION, APP_VERSION );
		letter->CreateData( NET_DARWINIA_SYSTEMTYPE, APP_SYSTEM );
		// Add in authentication information
		if( g_app->m_server || demoKey  )
		{
			// If it's local, let's just send the whole key
			letter->CreateData( NET_METASERVER_AUTHKEY, authKey );
		}
		else
		{
			// Otherwise we get a hash from the metaserver
            char authKeyHash[256];
            Authentication_GetKeyHash( authKeyHash );
            int keyId = Authentication_GetKeyId(authKey);          

            letter->CreateData( NET_METASERVER_AUTHKEY, authKeyHash );
            letter->CreateData( NET_METASERVER_AUTHKEYID, keyId );
		}
        if( m_password.Length() > 0) 
        {
            letter->CreateData( NET_DARWINIA_SERVER_PASSWORD, m_password.m_unicodestring );
        }

#ifdef LOCATION_EDITOR
        if( m_requestCustomMap )
        {
            letter->CreateData( NET_DARWINIA_REQUESTMAP, true );
        }
#endif

		SendLetter( letter );

#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() != TESTBED_ON)
		{
			m_connectionAttempts++;		

			if(m_connectionAttempts >= CTOS_MAX_CONNECTION_ATTEMPTS)
			{
				AppDebugOut("CLIENT has exceeded maximum connection attempts.\n" );
				m_connectionState = StateDisconnected;
				return;

			}
		}

#endif
        m_connectionAttempts++;		

		//if(m_connectionAttempts >= CTOS_MAX_CONNECTION_ATTEMPTS)
		//{
		//	AppDebugOut("CLIENT has exceeded maximum connection attempts.\n" );
		//	m_connectionState = StateDisconnected;
		//	return;

		//}
	}

    if( m_connectionState == StateConnected &&
		g_app->m_gameMode != App::GameModeCampaign &&
		g_app->m_gameMode != App::GameModePrologue &&
		(timeNow > m_retryTimerFast || timeNow > m_retryTimer || timeNow > m_retryTimerSlow) &&
        !g_app->m_spectator )
	{
		int teamId = g_app->m_globalWorld->m_myTeamId;

		if( teamId == 255 )
		{        
			// Send Request Team messages once a second until we have a team
			if(!m_bSpectator)
			{
				RequestTeam( TeamTypeLocalPlayer, -1 );
			}
		}
		else 
		{
			// Set the team names and colours
			LobbyTeam &lobbyTeam = g_app->m_multiwinia->m_teams[ teamId ];

			if( lobbyTeam.m_teamName != m_requestedTeamDetails.m_name )
			{
				// Limit the name updates to 1 a second to reduce bandwidth
				if( timeNow > m_retryTimer )
				{
					RequestTeamName( m_requestedTeamDetails.m_name, teamId );
				}
			}

			if( lobbyTeam.m_colourId != m_requestedTeamDetails.m_colourId )
			{
                if( (!g_app->m_multiwinia->m_coopMode && g_app->m_multiwinia->m_gameType != Multiwinia::GameTypeAssault ) || 
                    (g_app->m_atMainMenu && !g_app->m_location ) )
                {
				    RequestTeamColour( m_requestedTeamDetails.m_colourId, teamId );
                }
			}

			if( timeNow > m_retryTimerSlow )
			{
				AppDebugOut("Requesting send viral achievement mask\n");
				RequestSendViralPlayerAchievements(teamId);
			}

            if( g_app->UseChristmasMode() && !m_christmasMode && g_app->m_atMainMenu )
            {
                RequestChristmasMode();
            }
		}
	}

	if( timeNow > m_retryTimer )
	{
       m_retryTimer = timeNow + 1.0;
	}

	if( timeNow > m_retryTimerFast )
       m_retryTimerFast = timeNow + 0.1;

	if( timeNow > m_retryTimerSlow )
		m_retryTimerSlow = timeNow + 60.0;



	//
	// In the event of a sync error, have any of the other clients sent us their IFrame?
	if( m_outOfSyncClients.Size() > 0 && m_syncErrorSeqId != -1)
	{
		for( int i = 0; i < NUM_TEAMS; i++ )
		{
			LobbyTeam &lobbyTeam = g_app->m_multiwinia->m_teams[i];

			if (lobbyTeam.m_clientId != m_clientId)
			{
				char filename[64];
				sprintf( filename, "IFrame %d %d", lobbyTeam.m_clientId, m_syncErrorSeqId );

				if( FTP *ftp = m_ftpManager->Retrieve( filename ) )
				{
					g_app->m_multiwinia->SaveIFrame( 
						lobbyTeam.m_clientId, 
						new IFrame( ftp->Data(), ftp->Size() ) );

					delete ftp;
				}
			}
		}
	}

	//
	// FTP Manager
	m_ftpManager->MakeSendLetters( m_outbox );

	AdvanceSender();
}


int ClientToServer::GetOurIP_Int()
{
	static int s_localIP = 0;

	if( !s_localIP ) 
	{
		s_localIP = Server::ConvertIPToInt( GetOurIP_String() );
	}

	return s_localIP;
}


char *ClientToServer::GetOurIP_String()
{
	static char result[16] = {'\0'};

    if( result[0] == '\0' )
    {
		GetLocalHostIP( result, 16 );
    }

    return result;
}


int ClientToServer::GetNextLetterSeqID()
{
    int result = -1;

    m_inboxMutex->Lock();
    if( m_inbox.Size() > 0 )
    {
        result = m_inbox[0]->GetDataInt( NET_DARWINIA_SEQID );
    }    
    m_inboxMutex->Unlock();

    return result;
}


Directory *ClientToServer::GetNextLetter()
{
    m_inboxMutex->Lock();
    Directory *letter = NULL;
    
    if( m_inbox.Size() > 0 )
    {
        letter = m_inbox[0];
        int seqId = letter->GetDataInt( NET_DARWINIA_SEQID );

		if( m_clientId == -1 && seqId != -1)
		{
			// Only process seqId = -1 messages when we don't have a client id
			letter = NULL;
		}
		else if( seqId == g_lastProcessedSequenceId+1 ||
                 seqId == -1 )
        {
            // This is the next letter, remove from list
            // (assume its now dealt with)
            m_inbox.RemoveData(0);
        }
        else if( seqId < g_lastProcessedSequenceId+1 )
        {
			AppDebugOut( "Dropping duplicate letter %d (LPSID = %d)\n", seqId, g_lastProcessedSequenceId );
            // This is a duplicate old letter, throw it away
            m_inbox.RemoveData(0);
			delete letter;
            letter = NULL;
        }
        else
        {
            // We're not ready to read this letter yet
            letter = NULL;
        }
    }

    m_inboxMutex->Unlock();

    return letter;
}


int ClientToServer::GetEstimatedServerSeqId()
{    
    float timeFactor = 0.1f;
    float timePassed = GetHighResTime() - g_gameTimer.m_estimatedServerStartTime;

    int estimatedSequenceId = timePassed / timeFactor;
    
    estimatedSequenceId = std::max( estimatedSequenceId, m_serverSequenceId );
    
    return estimatedSequenceId;
}


float ClientToServer::GetEstimatedLatency()
{
    int seqIdDifference = 
		std::max( 0, GetEstimatedServerSeqId() - g_lastProcessedSequenceId );

    float timeFactor = 0.1f;
    float estimatedLatency = seqIdDifference * timeFactor;

    return estimatedLatency;
}

void ClientToServer::DebugPrintInbox()
{
	m_inboxMutex->Lock();
    int i;
    bool inserted = false;
    for( i = 0; i < m_inbox.Size(); i++ )
    {
        Directory *thisLetter = m_inbox[i];
        int thisSeqId = thisLetter->GetDataInt(NET_DARWINIA_SEQID);

		AppDebugOut("inbox[%d] = seq id %d\n", i, thisSeqId );
    }
	m_inboxMutex->Lock();
}

void ClientToServer::ReceiveLetter( Directory *letter, int _bandwidthUsed )
{
    m_receiveRate.Count( UDP_HEADER_SIZE + _bandwidthUsed );
	
	//
    // Simulate network packet loss

#ifdef TARGET_DEBUG
    if( g_inputManager && g_inputManager->controlEvent( ControlDebugDropPacket ) )
    {    
        delete letter;
        return;
    }
#endif

    //
    // Is this part of the MatchMaker service?

    if( strcmp( letter->m_name, NET_MATCHMAKER_MESSAGE ) == 0 )
    {
        MatchMaker_ReceiveMessage( m_listener, letter );
		delete letter;
        return;
    }

    //
    // If we are disconnected, chuck this message in the bin

    if( m_connectionState == StateDisconnected )
    {
        delete letter;
        return;
    }

    //
    // Ensure the message looks valid

    if( strcmp( letter->m_name, NET_DARWINIA_MESSAGE ) != 0 ||
		!letter->HasData( NET_DARWINIA_SEQID, DIRECTORY_TYPE_INT ) )
    {
        AppDebugOut( "Client received bogus data, discarded (2)\n" );
        delete letter;
        return;
    }

    //
    // If there is a previous update included in this letter,
    // take it out and deal with it now

    Directory *prevUpdate = letter->GetDirectory( NET_DARWINIA_PREVUPDATE );
    if( prevUpdate )
    {
        letter->RemoveDirectory( NET_DARWINIA_PREVUPDATE );
        prevUpdate->SetName( NET_DARWINIA_MESSAGE );
        ReceiveLetter( prevUpdate );
    }

    //
    // Record how far behind the server we are

    if( letter->HasData(NET_DARWINIA_LASTSEQID, DIRECTORY_TYPE_INT) )
    {
        int serverSeqId = letter->GetDataInt( NET_DARWINIA_LASTSEQID );
        m_serverSequenceId = std::max( m_serverSequenceId, serverSeqId );
    }

	int seqId = letter->GetDataInt( NET_DARWINIA_SEQID );

    //
    // If this letter is just for us put it to the front of the queue
    if( seqId == -1 )
    {
        m_inboxMutex->Lock();
        m_inbox.PutDataAtStart( letter );
        m_inboxMutex->Unlock();

        // this works fine in almost all cases, except for when dealing with disconnect messages.
        // because disconnect messages are the last thing that will be received by the client from the server,
        // and because the server starts ignoring the client as soon as the disconnect is sent,
        // all new messages received have sequence id -1, which means that the last valid sequence id no longer goes up.
        // This means that GetNumSlicesToAdvance() will always return 0 almost as soon as the client has been disconnected
        // which means that no new messages ever get processed, including the disconnect message.
        // So now we have this lovely hack here to manually increase the seuqnce id when a disconnect message is receieved.
        // since disconnect messages always go to the front of the queue, this has the effect of making the client deal with that message only.
        // Because the client then disconnects from the server properly, this won't have any adverse effects
        // Gary

        if( strcmp( letter->GetDataString( NET_DARWINIA_COMMAND ), NET_DARWINIA_DISCONNECT ) == 0 )
        {
            // what a fucking awesome hack!
            m_lastValidSequenceIdFromServer++;
        }
        return;
    }

    //
    // Check for duplicates
        
    if( seqId <= m_lastValidSequenceIdFromServer )
    {
		delete letter;
        return;
    }

    //
    // Work out our start time

    double newStartTime = GetHighResTime() - (double) seqId * 0.1f;
    if( newStartTime < g_gameTimer.m_estimatedServerStartTime ) 
    {
		g_gameTimer.m_estimatedServerStartTime = newStartTime;
    }

    //
    // Do a sorted insert of the letter into the inbox

    m_inboxMutex->Lock();
    int i;
    bool inserted = false;
    for( i = m_inbox.Size()-1; i >= 0; --i )
    {
        Directory *thisLetter = m_inbox[i];
        int thisSeqId = thisLetter->GetDataInt(NET_DARWINIA_SEQID);

		if( seqId > thisSeqId )
        {
            m_inbox.PutDataAtIndex( letter, i+1 );
            inserted = true;
            break;
        }
        else if( seqId == thisSeqId )
        {
            // Throw this letter away, it's a duplicate
            delete letter;
            inserted = true;
            break;
        }
    }
    if( !inserted )
    {
        m_inbox.PutDataAtStart( letter );
    }


    //
    // Recalculate our last Known Sequence Id

    for( i = 0; i < m_inbox.Size(); ++i )
    {
        Directory *thisLetter = m_inbox[i];
        int thisSeqId = thisLetter->GetDataInt( NET_DARWINIA_SEQID );
        if( thisSeqId > m_lastValidSequenceIdFromServer+1 )
        {
            break;
        }

		//int old = m_lastValidSequenceIdFromServer ;

		m_lastValidSequenceIdFromServer = std::max( m_lastValidSequenceIdFromServer, thisSeqId );
		//if( old != m_lastValidSequenceIdFromServer )
		//{
		//	AppDebugOut("lastValidSequenceId: %d => %d (lastProcessed = %d, thisSeqId = %d). m_clientId = %d\n", 
		//		old, m_lastValidSequenceIdFromServer, g_lastProcessedSequenceId, thisSeqId, m_clientId );
		//}

        if( thisLetter->m_subDirectories.Size() == 0 &&
            thisLetter->HasData( NET_DARWINIA_NUMEMPTYUPDATES, DIRECTORY_TYPE_INT ) )
        {
            int numEmptyUpdates = thisLetter->GetDataInt( NET_DARWINIA_NUMEMPTYUPDATES );
            AppAssert( numEmptyUpdates != -1 );
            m_lastValidSequenceIdFromServer = std::max( m_lastValidSequenceIdFromServer, 
                                                   thisSeqId + numEmptyUpdates - 1 );

			//AppDebugOut("lastValidSequenceId: %d => %d (lastProcessed = %d, thisSeqId = %d, numEmpty = %d). m_clientId = %d\n", 
			//	old, m_lastValidSequenceIdFromServer, g_lastProcessedSequenceId, thisSeqId, numEmptyUpdates, m_clientId );
        }
    }

    m_inboxMutex->Unlock();
}


void ClientToServer::SendLetter( Directory *letter )
{
    if( letter )
    {
        AppAssert( letter->HasData(NET_DARWINIA_COMMAND,DIRECTORY_TYPE_STRING) );

        letter->SetName     ( NET_DARWINIA_MESSAGE );

        m_outboxMutex->Lock();
        m_outbox.PutDataAtEnd( letter );
        m_outboxMutex->Unlock();
    }
}

void ClientToServer::ClientJoin( const char *_serverIp, int _serverPort )
{
    AppDebugOut( "CLIENT : Attempting connection...\n" );

    if( m_serverIp )
    {
        delete [] m_serverIp;
    }
    m_serverIp = newStr(_serverIp);
    m_serverPort = _serverPort;

    m_lastValidSequenceIdFromServer = -1;
	m_serverSequenceId = -1;    
    
	if (!g_app->m_server)
	{
		m_sendSocket = new NetSocketSession( *m_listener, _serverIp, _serverPort );
	}

	m_connectionState = StateConnecting;
	m_retryTimer = 0.0;
	m_retryTimerFast = 0.0;
	m_connectionAttempts = 0;
}

void ClientToServer::ClientLeave()
{
	if (m_connectionState > StateConnecting) 
	{
		AppDebugOut( "CLIENT : Sending disconnect...\n" );

		for( int i = 0; i < 3; i++ )
		{
			Directory *letter = new Directory();
			letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CLIENT_LEAVE );
			SendLetter( letter );
		}

		g_app->m_networkMutex.Lock();
		AdvanceSender();
		g_app->m_networkMutex.Unlock();

		NetSleep(100);
	}

	m_clientId = -1;
    m_lastValidSequenceIdFromServer = -1;
	m_serverSequenceId = -1;
	m_connectionState = StateDisconnected;
	m_syncErrorSeqId = -1;
    m_requestedTeamDetails.m_colourId = -1;
    
	m_bSpectator = false;

	m_outOfSyncClients.Empty();
	m_iframeHistory.EmptyAndDelete();

	StopIdentifying();
	SAFE_DELETE(m_sendSocket);	

    m_inboxMutex->Lock();
    m_inbox.EmptyAndDelete();
    m_inboxMutex->Unlock();

	g_lastProcessedSequenceId = -2;

	m_ftpManager->Reset();

	g_app->m_globalWorld->m_myTeamId = 255;
    strcpy( g_app->m_requestedMap, "null" );

	AppDebugOut( "CLIENT : Disconnected\n" );
}

void ClientToServer::RequestToRegisterAsSpectator(int _teamID)
{
    AppDebugOut( "CLIENT : Requesting to become a spectator (TeamID = %d)...\n", _teamID );
	Directory *letter = new Directory();
	unsigned char cTeam = (unsigned char)_teamID;
	unsigned char cClient = (unsigned char)m_clientId;

	letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_REQUEST_SPECTATOR );
	letter->CreateData( NET_DARWINIA_TEAMID, cTeam);
	letter->CreateData( NET_DARWINIA_CLIENTID, cClient);
	g_app->m_clientToServer->SendLetter( letter );
}


void ClientToServer::SendTeamScores( int _winnerId )
{
    AppDebugOut( "CLIENT : Sending Team Scores...\n" );

    Directory *letter = new Directory();
    
    letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_TEAM_SCORES );

	int teamScores[NUM_TEAMS];
	for( int i = 0; i < NUM_TEAMS; i++ )
	{		
		teamScores[ i ] = g_app->m_multiwinia->m_teams[i].m_score;
	}

	letter->CreateData( NET_DARWINIA_TEAM_SCORES, teamScores, NUM_TEAMS );
	letter->CreateData( NET_DARWINIA_TEAMID, (unsigned char) _winnerId );
    
    SendLetter( letter );
}

void ClientToServer::RequestTeam( int _teamType, int _desiredId )
{
    AppDebugOut( "CLIENT : Requesting Team (Type = %d)...\n", _teamType );

    Directory *letter = new Directory();
    
    letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_REQUEST_TEAM );
    letter->CreateData( NET_DARWINIA_TEAMTYPE,		_teamType );
	letter->CreateData( NET_DARWINIA_DESIREDTEAMID, _desiredId );
    
    SendLetter( letter );
}

void DirectorySetVector3( Directory *_letter, const char *_field, Vector3 const &_pos )
{
	char data[256];
	std::ostrstream s(data, 256);
	WriteNetworkValue( s, _pos );
	_letter->CreateData( _field, (void *) s.str(), s.tellp() );
}

Vector3 DirectoryGetVector3( Directory *_letter, const char *_field )
{
	int len;
	const char *data = (const char *) _letter->GetDataVoid( _field, &len );
	std::istrstream s( data, len );
	Vector3 result;
	ReadNetworkValue( s, result );
	return result;
}

void ClientToServer::SendIAmAlive( unsigned char _teamId, TeamControls const &_teamControls )
{
    Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,					NET_DARWINIA_ALIVE );
	letter->CreateData( NET_DARWINIA_TEAMID,					_teamId );	

	char data[256];
	int length;

	_teamControls.Pack( data, length );
	letter->CreateData( NET_DARWINIA_ALIVE_DATA, (void *) data, length );

    SendLetter( letter );
}


void ClientToServer::SendSyncronisation()
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,				NET_DARWINIA_SYNCHRONISE );
	letter->CreateData( NET_DARWINIA_LASTPROCESSEDSEQID,	g_lastProcessedSequenceId );
	letter->CreateData( NET_DARWINIA_SYNCVALUE,				m_sync );

    SendLetter( letter );
}


void ClientToServer::SetSyncState( int _clientId, bool _synchronised )
{
    if( !_synchronised && IsSynchronised(_clientId) )
    {
        m_outOfSyncClients.PutData( _clientId );
    }

    if( _synchronised )
    {
        for( int i = 0; i < m_outOfSyncClients.Size(); ++i )
        {
            if( m_outOfSyncClients[i] == _clientId )
            {
                m_outOfSyncClients.RemoveData(i);
                --i;
            }
        }
    }
}


bool ClientToServer::IsSynchronised( int _clientId )
{
    for( int i = 0; i < m_outOfSyncClients.Size(); ++i )
    {
        if( m_outOfSyncClients[i] == _clientId )
        {
            return false;
        }
    }

    return true;
}


bool ClientToServer::IsSequenceIdInQueue( int _seqId )
{
    bool found = false;

    m_inboxMutex->Lock();
    for( int i = 0; i < m_inbox.Size(); ++i )
    {
        Directory *thisLetter = m_inbox[i];
        int thisSeqId = thisLetter->GetDataInt( NET_DARWINIA_SEQID );
        if( thisSeqId == _seqId )
        {
            found = true;
            break;
        }

        if( thisLetter->m_subDirectories.Size() == 0 &&
            thisLetter->HasData( NET_DARWINIA_NUMEMPTYUPDATES, DIRECTORY_TYPE_INT ) )
        {
            int numEmptyUpdates = thisLetter->GetDataInt( NET_DARWINIA_NUMEMPTYUPDATES );
            if( _seqId >= thisSeqId &&
                _seqId < thisSeqId + numEmptyUpdates )
            {
                found = true;
                break;
            }
        }
    }
    m_inboxMutex->Unlock();

    return found;
}

void ClientToServer::SendPing( unsigned char _teamId )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,	NET_DARWINIA_PING );
	letter->CreateData( NET_DARWINIA_TEAMID,	_teamId );

	SendLetter( letter );

    m_pingSentTime = GetHighResTime();
}


void ClientToServer::RequestPause()
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,	NET_DARWINIA_PAUSE );

	SendLetter( letter );
}

void ClientToServer::RequestTeamName( const UnicodeString &_teamName, unsigned char _teamId )
{
	AppDebugOut( "CLIENT : Requesting TeamName %s for team %d...\n", _teamName.m_charstring, _teamId );

    Directory *letter = new Directory();
    
    letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_REQUEST_TEAMNAME );
    letter->CreateData( NET_DARWINIA_TEAMNAME,      _teamName );
	letter->CreateData( NET_DARWINIA_TEAMID, _teamId );

	AppDebugOut( "Setting Team name %s at %2.4f\n", _teamName.m_charstring, GetHighResTime() );
    
    SendLetter( letter );
}

void ClientToServer::SendChatMessage( const UnicodeString &msg, unsigned char _teamId )
{
    AppDebugOut( "CLIENT: Sending chat message");

    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_SEND_CHATMESSAGE );
    letter->CreateData( NET_DARWINIA_UNICODEMESSAGE, msg.m_unicodestring );
    letter->CreateData( NET_DARWINIA_CLIENTID, _teamId );

    SendLetter( letter );
}

void ClientToServer::ResetGameOptionVersions()
{
	int gameOptionArraySize = 64;
	memset( &m_gameOptionVersion, 0, sizeof( m_gameOptionVersion ) );
	memset( m_gameOptionArrayVersion[GAMEOPTION_GAME_PARAM], 0, gameOptionArraySize * sizeof(int) );
	memset( m_gameOptionArrayVersion[GAMEOPTION_GAME_RESEARCH_LEVEL], 0, gameOptionArraySize * sizeof(int) );
}


int ClientToServer::GameOptionVersion( int _gameOption, int _index )
{
	if( _index == -1 )
		return m_gameOptionVersion[_gameOption];
	else 
		return m_gameOptionArrayVersion[_gameOption][_index];
}

void ClientToServer::SetGameOptionVersion( int _gameOption, int _index, int _version )
{
	//AppDebugOut("Setting Game Option Version %d %d to %d\n", _gameOption, _index, _version );
	if( _index == -1 )
		m_gameOptionVersion[_gameOption] = _version;
	else 
		m_gameOptionArrayVersion[_gameOption][_index] = _version;
}

void ClientToServer::ReliableSetTeamColour( int _colourId )
{
	m_requestedTeamDetails.m_colourId = _colourId;
}

void ClientToServer::ReliableSetTeamName( const UnicodeString &_name )
{
	m_requestedTeamDetails.m_name = _name;
	m_retryTimer = 0.0f;
}

void ClientToServer::RequestSendViralPlayerAchievements( unsigned char _teamId )
{
	Directory* letter = new Directory();

	unsigned char achievements = g_app->m_achievementTracker->GetViralAchievementMask();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SET_PLAYERACHIEVEMENTS );
	letter->CreateData( NET_DARWINIA_PLAYERACHIEVEMENTS,achievements );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
    
    SendLetter( letter );
}

void ClientToServer::RequestTeamColour( int _colourId, unsigned char _teamId )
{
    if( _colourId != -1 ) AppDebugOut( "CLIENT : Requesting TeamColour %d for team %d...\n", _colourId, _teamId );

    Directory *letter = new Directory();
    
    letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_REQUEST_COLOUR );
    letter->CreateData( NET_DARWINIA_COLOURID,      _colourId );
	letter->CreateData( NET_DARWINIA_TEAMID,        _teamId );
    
    SendLetter( letter );
}

void ClientToServer::RequestSetGameOptionInt( int _gameOption, int _value )
{
	Directory *letter = new Directory();
	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SETGAMEOPTIONINT );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VERSION, GameOptionVersion( _gameOption ) + 1  );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_NAME,	_gameOption );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VALUE, _value );
	SendLetter( letter );
}

void ClientToServer::RequestSetGameOptionArrayElementInt( int _gameOption, int _index, int _value )
{
	Directory *letter = new Directory();
	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SETGAMEOPTIONARRAYELEMENTINT );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VERSION, GameOptionVersion( _gameOption, _index ) + 1 );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_NAME,	_gameOption );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_INDEX,	_index );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VALUE, _value );
	SendLetter( letter );
}

void ClientToServer::RequestSetGameOptionsArrayInt( int _gameOption, int *_values, int _numValues )
{
	// TODO: Delete this method
	Directory *letter = new Directory();
	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SETGAMEOPTIONARRAYINT );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VERSION, GameOptionVersion( _gameOption ) + 1  );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_NAME,	_gameOption );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_INDEX,	_numValues);
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VALUE,  _values, _numValues );
	SendLetter( letter );
}


void ClientToServer::RequestSetGameOptionString( int _gameOption, const char *_value )
{
	Directory *letter = new Directory();
	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SETGAMEOPTIONSTRING );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VERSION, GameOptionVersion( _gameOption ) + 1 );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_NAME,	_gameOption );
	letter->CreateData( NET_DARWINIA_GAMEOPTION_VALUE, _value );
	SendLetter( letter );
}

void ClientToServer::RequestStartGame()
{
	Directory *letter = new Directory();
	
	letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_START_GAME );
	SendLetter( letter );
}



void ClientToServer::RequestSelectUnit( unsigned char _teamId, int _unitId, int _entityId, int _buildingId )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,		NET_DARWINIA_SELECTUNIT );
	letter->CreateData( NET_DARWINIA_TEAMID,		_teamId );

	/*
	 We could pack more tightly to give a reduction of 24 - 17 = 7 bytes 
	 or even 10 bytes if we introduce a packed void type (where the length is encoded as 
	 a single byte, rather than as an integer (4 bytes)
	*/

	/*
	char data[256];
	ostrstream s( data, sizeof(data ) );
	unsigned char flags = 0;
	if( _unitId != -1 ) flags |= 1;
	if( _entityId != -1) flags |= 2;
	if( _buildingId != -1) flags |= 4;
	s.put( flags );
	if( flags & 1 )
		WritePackedInt(s, _unitId );
	if( flags & 2 )
		WritePackedInt(s, _entityId );
	if( flags & 3 )
		WritePackedInt(s, _buildingId);

	letter->CreateData( NET_DARWINIA_SELECTUNITDATA, (void *) s.str(), s.tellp() );	// 4 bytes  4 bytes + 1 + max( 8 / 4) = 17
	*/

	letter->CreateData( NET_DARWINIA_UNITID,		_unitId );		// 4 bytes + 4 bytes		8
	letter->CreateData( NET_DARWINIA_ENTITYID,		_entityId );	// 4 bytes + 1-4 byte		8 
	letter->CreateData( NET_DARWINIA_BUILDINGID,	_buildingId );  // 4 bytes + 4 bytes		8

    SendLetter( letter );
}


void ClientToServer::RequestAimBuilding( unsigned char _teamId, int _buildingId, Vector3 const &_pos )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_AIMBUILDING );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_BUILDINGID,		_buildingId );
	DirectorySetVector3( letter, NET_DARWINIA_POSITION,	_pos );

    SendLetter( letter );
}


void ClientToServer::RequestToggleFence( int _buildingId )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_TOGGLELASERFENCE );
	letter->CreateData( NET_DARWINIA_BUILDINGID,		_buildingId );

    SendLetter( letter );
}


void ClientToServer::RequestRunProgram( unsigned char _teamId, unsigned char _program )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_RUNPROGRAM );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_PROGRAM,			_program );

    SendLetter( letter );
}


void ClientToServer::RequestTargetProgram( unsigned char _teamId, unsigned char _program, Vector3 const &_pos )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_TARGETPROGRAM );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_PROGRAM,			_program );
	DirectorySetVector3( letter, NET_DARWINIA_POSITION,	_pos );
	
    SendLetter( letter );    
}


void ClientToServer::RequestSelectProgram( unsigned char _teamId, unsigned char _programId )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SELECTPROGRAM );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_PROGRAM,			_programId );
	
    SendLetter( letter );    
}


void ClientToServer::RequestTerminateProgram( unsigned char _teamId, unsigned char _programId )
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_TERMINATE );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_PROGRAM,			_programId );
	letter->CreateData( NET_DARWINIA_ENTITYID,			-1 );

    SendLetter( letter );
}


void ClientToServer::RequestTerminateEntity ( unsigned char _teamId,  int _entityId )
{
    AppDebugOut( "Request terminate entity : team %d entity %d\n", (int)_teamId, _entityId );

	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_TERMINATE );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_PROGRAM,			(unsigned char)255 );
	letter->CreateData( NET_DARWINIA_ENTITYID,			_entityId );

    SendLetter( letter );
}


void ClientToServer::RequestNextWeapon( unsigned char _teamId )
{
	AppDebugOut("Requesting next\n");
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_NEXTWEAPON );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    

    SendLetter( letter );
}

void ClientToServer::RequestOfficerFollow( unsigned char _teamId )
{
	Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_FOLLOW );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    

    SendLetter( letter );
}

void ClientToServer::RequestOfficerNone( unsigned char _teamId )
{
	Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_NONE );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    

    SendLetter( letter );
}

void ClientToServer::RequestPreviousWeapon( unsigned char _teamId )
{
	AppDebugOut("Requesting previous\n");
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_PREVWEAPON );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    

    SendLetter( letter );
}


void ClientToServer::RequestShamanSummon(unsigned char _teamId, int _shamanId, unsigned char _summonType)
{
	Directory *letter = new Directory();

	letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SHAMANSUMMON );
	letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );
	letter->CreateData( NET_DARWINIA_ENTITYID,			_shamanId );
	letter->CreateData( NET_DARWINIA_ENTITYTYPE,		_summonType );

	SendLetter( letter );
}


void ClientToServer::RequestSelectDarwinians( unsigned char _teamId, Vector3 const &_pos, float _radius )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_SELECTDGS );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    
    letter->CreateData( NET_DARWINIA_RADIUS,		    _radius );

    DirectorySetVector3( letter, NET_DARWINIA_POSITION, _pos );

    SendLetter( letter );
}


void ClientToServer::RequestIssueDarwinanOrders ( unsigned char _teamId, Vector3 const &_pos, bool directRoute )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_ORDERDGS );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId ); 
    letter->CreateData( NET_DARWINIA_DIRECTROUTE,       (char)directRoute );
    
    DirectorySetVector3( letter, NET_DARWINIA_POSITION, _pos );

    SendLetter( letter );
}


void ClientToServer::RequestOfficerOrders( unsigned char _teamId, Vector3 const &_pos, bool directRoute, bool forceOnlyMove )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_ORDERGOTO );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    
    letter->CreateData( NET_DARWINIA_DIRECTROUTE,       (char)directRoute );
    letter->CreateData( NET_DARWINIA_FORCEONLYMOVE,     (char)forceOnlyMove );

    DirectorySetVector3( letter, NET_DARWINIA_POSITION,	_pos );

    SendLetter( letter );
}


void ClientToServer::RequestArmourOrders( unsigned char _teamId, Vector3 const &_pos, bool deployAtWaypoint )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,			NET_DARWINIA_ORDERGOTO );
    letter->CreateData( NET_DARWINIA_TEAMID,			_teamId );    
    letter->CreateData( NET_DARWINIA_DIRECTROUTE,       true );
    letter->CreateData( NET_DARWINIA_FORCEONLYMOVE,     (char)(!deployAtWaypoint) );

    DirectorySetVector3( letter, NET_DARWINIA_POSITION,	_pos );

    SendLetter( letter );
}


void ClientToServer::RequestPromoteDarwinian( unsigned char _teamId, int _entityId )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,   NET_DARWINIA_PROMOTE );
    letter->CreateData( NET_DARWINIA_TEAMID,    _teamId );
    letter->CreateData( NET_DARWINIA_ENTITYID,  _entityId );

    SendLetter( letter );
}


void ClientToServer::RequestToggleEntity( unsigned char _teamId, int _entityId, bool secondary )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,   NET_DARWINIA_TOGGLEENTITY );
    letter->CreateData( NET_DARWINIA_TEAMID,    _teamId );
    letter->CreateData( NET_DARWINIA_ENTITYID,  _entityId );
    letter->CreateData( NET_DARWINIA_SECONDARYFUNCTION, (char) secondary );

    SendLetter( letter );
}


void ClientToServer::RequestToggleReady( unsigned char _teamId )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,   NET_DARWINIA_TOGGLE_READY );
    letter->CreateData( NET_DARWINIA_TEAMID,   _teamId );

    SendLetter( letter );
}

void ClientToServer::RequestSetTeamReady( unsigned char _teamId )
{
    Directory *letter = new Directory();

    letter->CreateData( NET_DARWINIA_COMMAND,   NET_DARWINIA_SET_TEAM_READY );
    letter->CreateData( NET_DARWINIA_TEAMID,   _teamId );

    SendLetter( letter );
}

void ClientToServer::RequestChristmasMode()
{
    Directory *letter = new Directory();
    letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CHRISTMASMODE );
    SendLetter(letter);
}

bool ClientToServer::IsLocalLetter( Directory *letter )
{
    return letter->GetDataInt(NET_DARWINIA_CLIENTID) == m_clientId;
}


int ClientToServer::GetNumSlicesToAdvance( bool catchUp )
{
    if (catchUp)
    {
        return 20;
    }    

    int numUpdatesToProcess = m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId;
    int numSlicesPending = numUpdatesToProcess * NUM_SLICES_PER_FRAME;
    if( g_sliceNum != -1 ) numSlicesPending += ( NUM_SLICES_PER_FRAME - g_sliceNum );

    int numSlicesToAdvance = numSlicesPending * 0.3;

    if( numSlicesPending <= 10 )
    {
        float timeSinceStartOfServerAdvance = GetHighResTime() - g_gameTimer.m_lastClientAdvance;
        int numSlicesToAdvance = timeSinceStartOfServerAdvance * 100;
    }

    numSlicesToAdvance = std::max( numSlicesToAdvance, 0 );
    numSlicesToAdvance = std::min( numSlicesToAdvance, 20 );
    
    return( numSlicesToAdvance );
}


//int ClientToServer::GetNumSlicesToAdvance( bool catchUp )
//{
//	if (catchUp)
//	{
//		return NUM_SLICES_PER_FRAME;
//	}
//
//    int numUpdatesToProcess = m_lastValidSequenceIdFromServer - g_lastProcessedSequenceId;
//    int numSlicesPending = numUpdatesToProcess * NUM_SLICES_PER_FRAME;
//    if( g_sliceNum != -1 ) numSlicesPending -= g_sliceNum;
//    else if( g_sliceNum == -1 ) numSlicesPending -= 10;
//    
//    if( numSlicesPending > 20 )
//    {
//        return NUM_SLICES_PER_FRAME;
//    }
//
//    //float timeSinceStartOfAdvance = g_gameTime - g_lastServerAdvance;
//    float timeSinceStartOfServerAdvance = GetHighResTime() - g_gameTimer.m_lastClientAdvance;
//	//int numSlicesThatShouldBePending = 10 - timeSinceStartOfAdvance * 10.0f;
//
//    int numSlicesToAdvance = timeSinceStartOfServerAdvance * 100;
//    if( g_sliceNum != -1 ) numSlicesToAdvance -= g_sliceNum;
//    if( g_sliceNum == -1 ) numSlicesToAdvance -= 10;
//
//    //AppDebugAssert( numSlicesToAdvance >= 0 );
//    numSlicesToAdvance = max( numSlicesToAdvance, 0 );
//    numSlicesToAdvance = min( numSlicesToAdvance, 10 );
//    
//    return numSlicesToAdvance;
//}


void ClientToServer::GenerateSyncValue()
{
	IFrame *frame = new IFrame;
	
	frame->Generate();
	AddIFrame( frame );
	m_sync = frame->HashValue();
}

void ClientToServer::KeepAlive()
{
	static double lastKeepAlive = 0;
	double now = GetHighResTime();

	// Send a synchronisation packet to keep the server happy
	// Once every second

	if( g_lastProcessedSequenceId >= 0 && now > lastKeepAlive + 1.0 )
	{
		AppDebugOut("Sending Keep Alive\n");
		SendSyncronisation();
		AdvanceSender();
		lastKeepAlive = now;
	}
}

#ifdef TRACK_SYNC_RAND
const char *DecodeCmd( const char *_cmd )
{
	typedef std::map< std::string, std::string > StringStringMap;

	static StringStringMap s_entries;
	static bool s_inited = false;

	if( !s_inited )
	{
		s_inited = true;
				
		s_entries["ba"] = "MESSAGE";
		s_entries["bb"] = "COMMAND";
		s_entries["ca"] = "CLIENT_JOIN";
		s_entries["cb"] = "CLIENT_LEAVE";
		s_entries["cba"] = "REQUEST_SPECTATOR";
		s_entries["cc"] = "REQUEST_TEAM";
		s_entries["cca"] = "TOGGLE_READY";
		s_entries["ccb"] = "SET_TEAM_READY";
		s_entries["cd"] = "ALIVE";
		s_entries["ce"] = "SELECTUNIT";
		s_entries["cf"] = "AIMBUILDING";
		s_entries["cg"] = "TOGGLELASERFENCE";
		s_entries["ch"] = "RUNPROGRAM";
		s_entries["ci"] = "TARGETPROGRAM";
		s_entries["cj"] = "SELECTPROGRAM";
		s_entries["ck"] = "TERMINATE";
		s_entries["cl"] = "SHAMANSUMMON";
		s_entries["cm"] = "SELECTDGS";
		s_entries["cn"] = "ORDERDGS";
		s_entries["co"] = "PROMOTE";
		s_entries["coa"] = "ORDERGOTO";
		s_entries["cob"] = "DIRECTROUTE";
		s_entries["coc"] = "TOGGLEENTITY";
		s_entries["cod"] = "FORCEONLYMOVE";
		s_entries["coe"] = "SECONDARYFUNCTION";
		s_entries["cp"] = "PAUSE";
		s_entries["cq"] = "SYNCHRONISE";
		s_entries["cr"] = "PING";
		s_entries["cs"] = "START_GAME";
		s_entries["ct"] = "MAPNAME";
		s_entries["cu"] = "GAMETYPE";
		s_entries["cv"] = "GAMEPARAMS";
		s_entries["cw"] = "RESEARCHLEVEL";
		s_entries["cx"] = "NEXTWEAPON";
		s_entries["cy"] = "PREVWEAPON";
		s_entries["cya"] = "FOLLOW";
		s_entries["cyb"] = "NONE";
		s_entries["cz"] = "REQUEST_TEAMNAME";
		s_entries["czz"] = "REQUEST_COLOUR";
		s_entries["cczz"] = "SEND_CHATMESSAGE";
		s_entries["c3"] = "TEAM_SCORES";
		s_entries["da"] = "TEAMTYPE";
		s_entries["db"] = "DESIREDTEAMID";
		s_entries["dc"] = "LASTPROCESSEDSEQID";
		s_entries["dd"] = "SYNCVALUE";
		s_entries["de"] = "TEAMID";
		s_entries["df"] = "POSITION";
		s_entries["dg"] = "ALIVE_DATA";
		s_entries["dl"] = "UNITID";
		s_entries["dm"] = "ENTITYID";
		s_entries["dn"] = "BUILDINGID";
		s_entries["do"] = "ENTITYTYPE";
		s_entries["dp"] = "RADIUS";
		s_entries["dq"] = "PROGRAM";
		s_entries["dr"] = "RANDSEED";
		s_entries["ds"] = "TEAMNAME";
		s_entries["dt"] = "COLOURID";
		s_entries["dw"] = "UNICODEMESSAGE";
		s_entries["dx"] = "ENCRYPTION_PADDING_CTS";
		s_entries["dy"] = "SERVER_PASSWORD";
		s_entries["dz"] = "CHRISTMASMODE";
		s_entries["ea"] = "DISCONNECT";
		s_entries["eb"] = "CLIENTHELLO";
		s_entries["ec"] = "CLIENTGOODBYE";
		s_entries["ed"] = "CLIENTID";
		s_entries["ef"] = "TEAMASSIGN";
		s_entries["eg"] = "NETSYNCERROR";
		s_entries["eh"] = "NETSYNCFIXED";
		s_entries["ei"] = "SYNCERRORID";
		s_entries["ej"] = "SYNCERRORSEQID";
		s_entries["ek"] = "UPDATE";
		s_entries["el"] = "VERSION";
		s_entries["em"] = "SYSTEMTYPE";
		s_entries["en"] = "PREVUPDATE";
		s_entries["eo"] = "NUMEMPTYUPDATES";
		s_entries["ep"] = "SEQID";
		s_entries["eq"] = "LASTSEQID";
		s_entries["er"] = "XUID";
		s_entries["es"] = "ASSIGN_SPECTATOR";
		s_entries["et"] = "CLIENTISDEMO";
		s_entries["eu"] = "ENCRYPTION_PADDING_STC";
		s_entries["ev"] = "SERVER_FULL";
		s_entries["ew"] = "MAKE_SPECTATOR";
		s_entries["ex"] = "SPECTATORASSIGN";
		s_entries["ey"] = "SET_PLAYERACHIEVEMENTS";
		s_entries["ez"] = "PLAYERACHIEVEMENTS";
		s_entries["fa"] = "FILESEND";
		s_entries["fb"] = "FILENAME";
		s_entries["fc"] = "FILESIZE";
		s_entries["fd"] = "BLOCKNUMBER";
		s_entries["fe"] = "BLOCKDATA";
		s_entries["ga"] = "FROMIP";
		s_entries["gb"] = "FROMPORT";
		s_entries["gc"] = "FROMPLAYER";
		s_entries["ha"] = "SETGAMEOPTIONINT";
		s_entries["hb"] = "SETGAMEOPTIONARRAYELEMENTINT";
		s_entries["hc"] = "SETGAMEOPTIONARRAYINT";
		s_entries["hd"] = "SETGAMEOPTIONSTRING";
		s_entries["he"] = "GAMEOPTION_NAME";
		s_entries["hf"] = "GAMEOPTION_INDEX";
		s_entries["hg"] = "GAMEOPTION_VALUE";
		s_entries["hi"] = "GAMEOPTION_VERSION";
		s_entries["hj"] = "GAMEOPTION_PDLC_BITMASK";
		s_entries["hk"] = "JOIN_VIA_INVITE";
	}

	StringStringMap::iterator i = s_entries.find( _cmd );
	if( i == s_entries.end() )
		return _cmd;
	else
		return i->second.c_str();
}

void WriteSummary( Directory *_letter, std::ostream &_s )
{
	/*
	NET_DARWINIA_MESSAGE
		NET_DARWINIA_COMMAND
	*/
	
	if( strcmp( _letter->GetName(), NET_DARWINIA_MESSAGE ) != 0 )
	{
		_s << "Other message";
		return;
	}
	
	const char *cmd = _letter->GetDataString( NET_DARWINIA_COMMAND );
	if( strcmp( cmd, NET_DARWINIA_UPDATE ) == 0 )
	{
		DArray<Directory *> &subDirs = _letter->GetSubDirectories();
		int subDirSize = subDirs.Size();

		for( int i = 0; i < subDirSize; i++ )
		{
			if( subDirs.ValidIndex( i ) )
			{
				WriteSummary( subDirs.GetData( i ), _s );
				if( i < subDirSize - 1 )
					_s << ", ";
			}
		}
	}
	else
		_s << DecodeCmd( cmd );
}
#endif

void ClientToServer::ProcessAllServerLetters( bool catchUp )
{
	int slicesToAdvance = GetNumSlicesToAdvance( catchUp );
	Location* loc = g_app->m_location;

#ifdef TRACK_SYNC_RAND
	bool debug = g_lastProcessedSequenceId < 1000;
#endif

	// Do our heavy weight physics
	for (int i = 0; i < slicesToAdvance; ++i)
	{
		if( g_sliceNum == -1 )
		{
			// Read latest update from Server			
			Directory *letter = GetNextLetter();

			if( letter )
			{
				int seqId = letter->GetDataInt( NET_DARWINIA_SEQID );  				
								
				AppDebugAssert( seqId == -1 || seqId == g_lastProcessedSequenceId + 1 );
                m_lastServerLetterReceivedTime = GetHighResTime();

#ifdef TRACK_SYNC_RAND
				SyncRandLog( "Seq ID: %d\n", g_lastProcessedSequenceId );

				if( debug )
				{
					std::ostringstream s;
					WriteSummary( letter, s );
					SyncRandLog( "Processing Letter: %s (seqId = %d)", s.str().c_str(), seqId );
				}
#endif

				bool handled = ProcessServerLetters(letter);

#ifdef TRACK_SYNC_RAND
				if( debug )
					SyncRandLog( "Handled as Server Letter: %d", handled );
#endif

				if( seqId != -1 ) 
				{
					bool isUpdate = false;

					if( !handled )
					{						
						isUpdate = ProcessServerUpdates( letter );
					}

#ifdef TRACK_SYNC_RAND
					if( debug )
						SyncRandLog( "Handled as update: %d", isUpdate );
#endif

					g_lastProcessedSequenceId = seqId;

					if( isUpdate )
					{
						g_sliceNum = 0;
						//g_lastServerAdvance = (double) seqId * SERVER_ADVANCE_PERIOD + g_startTime;
						g_gameTimer.Tick();

						GenerateSyncValue();
						SendSyncronisation();
					}
				}

				delete letter;

				// If the letter we just processed was a game start message,
				// get out of here now (we don't want to process any more slices
				// until the location has finished loading)
				if( g_app->m_loadingLocation )
					return;
			}
		}
		
		if( g_sliceNum != -1 )
		{               
#ifdef TRACK_SYNC_RAND
			if( debug ) SyncRandLog( "g_sliceNum = %d", g_sliceNum );
#endif
			if( loc ) 
			{
                if( !g_app->GamePaused() )
                {
#ifdef TRACK_SYNC_RAND
					if( debug ) SyncRandLog( "Advance Location" );
#endif
				    loc->Advance( g_sliceNum );
				    g_app->m_particleSystem->Advance( g_sliceNum );
                }

                if( g_app->Multiplayer() && g_sliceNum == 8 )
                {
#ifdef TRACK_SYNC_RAND
					if( debug ) SyncRandLog( "Advance Multiwinia" );
#endif
                    g_app->m_multiwinia->Advance();
                }
			}
       
			if( g_sliceNum < NUM_SLICES_PER_FRAME-1 )
			{
#ifdef TRACK_SYNC_RAND
				if( debug ) SyncRandLog( "Incremement sliceNumber" );
#endif
				g_sliceNum++;
			}
			else
			{
#ifdef TRACK_SYNC_RAND
				if( debug ) SyncRandLog( "sliceNumber reset to -1" );
#endif
				g_sliceNum = -1;
			}
		}
	}
}

void ClientToServer::GetDemoLimitations( DemoLimitations &l )
{
    l.Initialise( MetaServer_RequestData( NET_METASERVER_DATA_DEMO2LIMITS ) );
}

void ClientToServer::GetSyncFileName( const char *_prefix, char *_syncFilename, int _bufSize )
{
	struct tm *current = localtime(&m_serverStartTime);
	char date[256];

	strftime( date, sizeof(date), "%Y-%m-%d--%H-%M-%S", current );		
	snprintf( _syncFilename, _bufSize, "%s%s-%s-client%d-team%d.txt", 
		g_app->GetProfileDirectory(), _prefix, date, m_clientId, g_app->m_globalWorld->m_myTeamId );

	_syncFilename[ _bufSize - 1 ] = '\0';
}


bool ClientToServer::ProcessServerLetters( Directory *letter )
{
	// Process non-game-world updates here

    if( strcmp( letter->m_name, NET_DARWINIA_MESSAGE ) != 0 ||
        !letter->HasData( NET_DARWINIA_COMMAND ) )
    { 
        AppDebugOut( "Client received bogus message, discarded (4)\n" );
        return true;
    }
		
    char *cmd = letter->GetDataString( NET_DARWINIA_COMMAND );

    if( strcmp( cmd, NET_DARWINIA_CLIENTHELLO ) == 0 )
    {
        if( IsLocalLetter( letter ) )
        {
			m_connectionState = ClientToServer::StateConnected;
            AppDebugOut( "CLIENT : Received HelloClient from Server\n" );

			if (g_app->m_gameMode == App::GameModeCampaign || g_app->m_gameMode == App::GameModePrologue)
			{
				g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
				g_app->m_clientToServer->RequestTeam( TeamTypeCPU, -1 );
				g_app->m_clientToServer->RequestTeam( TeamTypeLocalPlayer, -1 );
			}

			m_serverStartTime = letter->GetDataULLong( NET_METASERVER_STARTTIME );			
			m_serverRandomNumber = letter->GetDataInt( NET_METASERVER_RANDOM );

			// We could trigger an upsell here if the server is a full 
        }

        return true;
    }
	else if( strcmp( cmd, NET_DARWINIA_DISCONNECT ) == 0 )
	{
		// TO DO: Display a message to the user?
		int reason = letter->GetDataInt(NET_DARWINIA_DISCONNECT);

		AppDebugOut("Received the disconnect message! Showing upsell\n");
		GameMenuWindow* gmw = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
		
		ClientLeave();

		if( gmw )
		{
			AppDebugOut("Going back to main menu\n");
			if(reason == Disconnect_ServerFull)
			{
				//gmw->m_newPage = GameMenuWindow::PageJoinGame;
				gmw->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_serverfull"), GameMenuWindow::PageJoinGame );
				m_connectionState = StateDisconnected;
				AppDebugOut("------------------> Been disconnected because server is full\n");
			}
			else if(reason == Disconnect_DuplicateKey)
			{
				gmw->CreateErrorDialogue(LANGUAGEPHRASE("dialog_duplicate_key"), GameMenuWindow::PageMain);
				m_connectionState = StateDisconnected;
				AppDebugOut("------------------> Been disconnected because of duplicate key\n");
			}
            else if( reason == Disconnect_KickedByServer )
            {
				gmw->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_kickedbyserver"), GameMenuWindow::PageMain);
				m_connectionState = StateDisconnected;
				AppDebugOut("------------------> Has been kicked by the server\n");
            }
            else if( reason == Disconnect_BadPassword )
            {
				gmw->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_badpass"), GameMenuWindow::PageJoinGame);
				m_connectionState = StateDisconnected;
				AppDebugOut("------------------> Kicked because of incorrect password\n");
            }

			else
			{
				gmw->m_newPage = GameMenuWindow::PageMain;
			}

#ifdef TESTBED_ENABLED
			// We got booted out from the game so set the testbed back to main menu
			if(g_app->GetTestBedMode() == TESTBED_ON)
			{
				//g_app->TestBedLogWrite("Client got a disconnected\n");

				g_app->SetTestBedState(TESTBED_MAINMENU);
			}
#endif
		}
		return true;
	}
    else if( strcmp( cmd, NET_DARWINIA_CLIENTGOODBYE ) == 0 )
    {        
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);        
		AppDebugOut( "CLIENT : Client %d left the game\n", clientId );
		
		if( g_app->m_multiwinia )
		{
			g_app->m_multiwinia->RemoveTeams( clientId );

			if(clientId == m_clientId)
			{	
				m_bSpectator = false;
			}
		}

		//g_app->m_location->RemoveTeam( letter->m_teamId );
        SetSyncState( clientId, true );
		return true;
    }
    else if( strcmp( cmd, NET_DARWINIA_CLIENTID ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);  
		int iSpectator = letter->GetDataInt(NET_DARWINIA_MAKE_SPECTATOR);



        AppDebugOut( "CLIENT : Received ClientID of %d\n", clientId );        

        if( m_clientId != -1 )
        {
            //AppAssert( m_clientId == clientId );
            ClientLeave();
            GameMenuWindow* gmw = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
            if( gmw )
            {
                gmw->CreateErrorDialogue( LANGUAGEPHRASE("multiwinia_error_connection"), GameMenuWindow::PageMain );
            }

            return true;
        }

        m_clientId = clientId;

		// If we are a spectator then bomb out
		if(iSpectator != -1)
		{
			if(iSpectator)
			{
				GameMenuWindow* gmw = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
				
				m_connectionState = ClientToServer::StateHandshaking;

				ClientLeave();

				if( gmw )
				{

					//gmw->m_newPage = GameMenuWindow::PageJoinGame;
					gmw->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_serverfull"), GameMenuWindow::PageJoinGame );
					
					return true;

				}
			}

		}
        m_connectionState = ClientToServer::StateHandshaking;
        g_lastProcessedSequenceId = -1;

        if( letter->HasData( NET_DARWINIA_VERSION, DIRECTORY_TYPE_STRING ) )
        {
            char *serverVersion = letter->GetDataString( NET_DARWINIA_VERSION );
            strcpy( m_serverVersion, serverVersion );
            AppDebugOut( "CLIENT : Server version is %s\n", serverVersion );
        }

		int randomSeed = letter->GetDataInt( NET_DARWINIA_RANDSEED );

		AppDebugOut(" CLIENT : Setting sync rand seed to %d\n", randomSeed );

		syncrandseed( randomSeed );

#ifdef TESTBED_ENABLED
		g_app->m_iClientRandomSeed = randomSeed;
#endif


		g_app->m_multiwinia->Reset();
		ResetGameOptionVersions();
        return true;
    }
    else if( strcmp( cmd, NET_DARWINIA_TEAMASSIGN ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);        
        int teamId   = letter->GetDataInt(NET_DARWINIA_TEAMID);
        int teamType = letter->GetDataInt(NET_DARWINIA_TEAMTYPE);

		if( teamType != TeamTypeCPU && !IsLocalLetter( letter ) )
        {
			if(teamType != TeamTypeSpectator)
			{
				teamType = TeamTypeRemotePlayer;
			}
        }

		g_app->m_multiwinia->InitialiseTeam( teamId, teamType, clientId );

        if( letter->HasData( NET_DARWINIA_CLIENTISDEMO ) )
        {
            g_app->m_multiwinia->m_teams[ teamId ].m_demoPlayer = true;
            AppDebugOut( "Team %d with Client %d is DEMO PLAYER\n", teamId, clientId );
        }

        return true;
    }
    else if( strcmp( cmd, NET_DARWINIA_SERVER_FULL ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);        

		if(clientId == m_clientId)
		{

			//m_connectionState = StateDisconnected;

			GameMenuWindow* gmw = (GameMenuWindow*)EclGetWindow("multiwinia_mainmenu_title");
			
			ClientLeave();

			if( gmw )
			{
				gmw->CreateErrorDialogue(LANGUAGEPHRASE("multiwinia_error_serverfull"));
			}
		}

		return true;
	}
    else if( strcmp( cmd, NET_DARWINIA_NETSYNCERROR ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);
		int seqId = letter->GetDataInt(NET_DARWINIA_SYNCERRORSEQID);

		AppDebugOut( "CLIENT: Server informed us that client %d is out of sync at seq id %d\n", clientId, seqId );

		SetSyncState( clientId, false );

		// We only do this the first time we're told (we're told once for each client that is out of sync)
		// In a two player game, we get told twice because it's impossible to say
		// which client is out of sync
		if (m_outOfSyncClients.Size() == 1)
		{				
			SendIFrame( seqId );        
			m_syncErrorSeqId = seqId;

			char syncFilename[256];
			GetSyncFileName( "syncerror", syncFilename, sizeof(syncFilename) );

#ifdef TRACK_SYNC_RAND
			DumpSyncRandLog( syncFilename );
#endif

			g_app->m_location->DumpWorldToSyncLog( syncFilename );
		}
		
        return true;
    }
    else if( strcmp( cmd, NET_DARWINIA_NETSYNCFIXED ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);        
        AppDebugOut( "SYNCFIXED Server informed us that Client %d has repaired his Sync Error\n", clientId );
        SetSyncState( clientId, true );		
        return true;
    }
    else if( strcmp( cmd, NET_DARWINIA_SPECTATORASSIGN ) == 0 )
    {
        int clientId = letter->GetDataInt(NET_DARWINIA_CLIENTID);        
        
		g_app->m_multiwinia->RemoveTeams(clientId);

        if( clientId == m_clientId )
        {            
            AppDebugOut( "CLIENT: I am a spectator\n" );
			m_bSpectator = true;
        }
				
        return true;
    }
	else if( strcmp( cmd, NET_DARWINIA_FILESEND ) == 0 )
	{
		ProcessFTPLetter( letter );
		return true;
	}
	else 
	{
		return false;
	}
}


bool ClientToServer::ProcessServerUpdates( Directory *letter )
{
	if(!g_app)
	{
		return false;
	}

	Location* loc = g_app->m_location;
	// Process game world updates here (? Move much of this code to a method on Location?)

    if( strcmp(letter->m_name, NET_DARWINIA_MESSAGE) != 0 ||
        !letter->HasData(NET_DARWINIA_COMMAND, DIRECTORY_TYPE_STRING) )
    {
        AppDebugOut( "ClientToServer received bogus message, discarded (8)\n" );
        return false;
    }

    char *cmd = letter->GetDataString(NET_DARWINIA_COMMAND);    
    if( strcmp(cmd, NET_DARWINIA_UPDATE) != 0 )
    {
        AppDebugOut( "ClientToServer received bogus message, discarded (9)\n" );
        return false;
    }

    //
    // Special case - this may be an empty update informing us
    // of a stack of upcoming empty updates

    if( letter->m_subDirectories.Size() == 0 &&
        letter->HasData( NET_DARWINIA_NUMEMPTYUPDATES, DIRECTORY_TYPE_INT ) )
    {
        int numEmptyUpdates = letter->GetDataInt( NET_DARWINIA_NUMEMPTYUPDATES );
        AppAssert( numEmptyUpdates != -1 );
        
        int seqId = letter->GetDataInt( NET_DARWINIA_SEQID );
        
        if( numEmptyUpdates > 1 )
        {            
            Directory *letterCopy = new Directory(*letter);
            letterCopy->CreateData( NET_DARWINIA_NUMEMPTYUPDATES, numEmptyUpdates-1 );
            letterCopy->CreateData( NET_DARWINIA_SEQID, seqId+1 );
            
            m_inboxMutex->Lock();
            m_inbox.PutDataAtStart( letterCopy );
            m_inboxMutex->Unlock();
        }

        return true;
    }    

    //
    // Our updates are stored in the sub-directories
    
    for( int i = 0; i < letter->m_subDirectories.Size(); ++i )
    {
        if( letter->m_subDirectories.ValidIndex(i) )
        {
            Directory *update = letter->m_subDirectories[i];
            AppAssert( strcmp(update->m_name, NET_DARWINIA_MESSAGE) == 0 );
            AppAssert( update->HasData( NET_DARWINIA_COMMAND, DIRECTORY_TYPE_STRING ) );

            char *cmd = update->GetDataString( NET_DARWINIA_COMMAND );

			if( strcmp( cmd, NET_DARWINIA_PAUSE ) == 0 )
			{
				g_app->m_userRequestsPause = !g_app->m_userRequestsPause;

                if( g_app->m_userRequestsPause )
                {
                    g_gameTimer.RequestSpeedChange( 0.0, 0.2, -1.0 );
                }
                else
                {
                    g_gameTimer.RequestSpeedChange( 1.0, 0.2, -1.0 );
                }
			}
 			else if( strcmp( cmd, NET_DARWINIA_SET_PLAYERACHIEVEMENTS ) == 0 )
			{
				unsigned char achievementMask = 0;
				int teamId = update->HasData(NET_DARWINIA_TEAMID) ? update->GetDataUChar(NET_DARWINIA_TEAMID) : -1;
				if( update->HasData(NET_DARWINIA_PLAYERACHIEVEMENTS) )
				{
					AppDebugOut("Update has player achievement mask\n");
					achievementMask = update->GetDataUChar(NET_DARWINIA_PLAYERACHIEVEMENTS);
					AppDebugOut("Achievement mask is %d\n", achievementMask);
				}

				if( teamId >= 0 && teamId < NUM_TEAMS )
				{
					AppDebugOut("Setting achievement mask to %d for team id %d\n", achievementMask, teamId);
					g_app->m_multiwinia->m_teams[teamId].m_achievementMask = achievementMask;
				}
			}
			else if( strcmp( cmd, NET_DARWINIA_TEAM_SCORES ) == 0)
			{
				/* Ignore this */
			}
			if( strcmp( cmd, NET_DARWINIA_SETGAMEOPTIONARRAYELEMENTINT ) == 0)
			{
				AppDebugOut("Game Option Array Element Int\n" );
				int gameOption = update->GetDataInt(NET_DARWINIA_GAMEOPTION_NAME);
				int index = update->GetDataInt(NET_DARWINIA_GAMEOPTION_INDEX);
				int value = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VALUE);
				int version = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VERSION);

				if( version > GameOptionVersion( gameOption, index ) )
				{
					SetGameOptionVersion( gameOption, index, version );
					switch( gameOption )
					{
					case GAMEOPTION_GAME_PARAM:
						if( !g_app->m_multiwinia->ValidGameOption( index ) )
						{
							AppDebugOut("Warning: Invalid game param %d\n", index );
						}
						else
						{
							g_app->m_multiwinia->SetGameOption( index, value );
						}
						break;

					case GAMEOPTION_GAME_RESEARCH_LEVEL:
						if( index < 0 || index >=  GlobalResearch::NumResearchItems )
						{
							AppDebugOut("Warning: Game research %d is outside of allowed range\n", index );
						}
						else
						{
							g_app->m_globalWorld->m_research->m_researchLevel[index] = value;
						}
						break;

					default:
						AppDebugOut("Warning: Game array index option %d unknown\n", gameOption );
						break;
					}
				}					
			}
			else if( strcmp( cmd, NET_DARWINIA_SETGAMEOPTIONINT ) == 0 )
			{
				// AppDebugOut("Game Option Int\n" );
				int gameOption = update->GetDataInt(NET_DARWINIA_GAMEOPTION_NAME);
				int value = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VALUE);
				int version = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VERSION);

				if( version > GameOptionVersion( gameOption ) )
				{
					SetGameOptionVersion( gameOption, -1, version );

					if( GAMEOPTION_TEAMSSTATUS_LBOUND <= gameOption && gameOption <= GAMEOPTION_TEAMSSTATUS_UBOUND )
					{
						g_app->m_multiwinia->m_teamsStatus[gameOption - GAMEOPTION_TEAMSSTATUS_LBOUND] = value;
					}

					switch( gameOption )
					{
					case GAMEOPTION_GAME_TYPE:
						if( value < 0 || value >= Multiwinia::NumGameTypes )
						{
							AppDebugOut("Warning: Game type %d is outside of allowed range\n", value );
						}
						else
						{
							// Zero the game parameters and research values						
							int gameParams[MULTIWINIA_MAXPARAMS];
							int researchLevel[GlobalResearch::NumResearchItems];

							memset( gameParams, 0, sizeof(gameParams) );
							memset( researchLevel, 0, sizeof(researchLevel) );

							g_app->m_multiwinia->SetGameResearch( researchLevel );
							g_app->m_multiwinia->SetGameOptions( value, gameParams );
							
						}
						break;

					case GAMEOPTION_AIMODE:
						{
							if( value <0 || value > Multiwinia::NumAITypes )
							{
								AppDebugOut("Warning: AI Mode %d is outside of allowed range\n", value );
							}
							else
							{
								g_app->m_multiwinia->m_aiType = value;
							}

						}
						break;

					case GAMEOPTION_COOPMODE:
					{
						g_app->m_multiwinia->m_coopMode = value;
                        if( g_app->m_multiwinia->m_coopMode )
                        {
                            m_requestedTeamDetails.m_colourId = -1;
						    for( int i = 0; i < NUM_TEAMS; ++i )
						    {
							    g_app->m_multiwinia->SetTeamColour( i, -1 );
						    }
						    GameMenuWindow *gmw =(GameMenuWindow *)EclGetWindow("multiwinia_mainmenu_title");
                            gmw->m_previousRequestedColour = gmw->m_currentRequestedColour;
                            gmw->m_currentRequestedColour = -1;
                        }
                        else
                        {
                            GameMenuWindow *gmw =(GameMenuWindow *)EclGetWindow("multiwinia_mainmenu_title");
                            gmw->m_currentRequestedColour = gmw->m_previousRequestedColour;
                        }
					}
					break;

                    case GAMEOPTION_BASICCRATES:
                    {
                        g_app->m_multiwinia->m_basicCratesOnly = value;
                    }
                    break;

                    case GAMEOPTION_SINGLEASSAULT:
                    {
                        g_app->m_multiwinia->m_assaultSingleRound = value;
                    }
                    break;

					default:
						AppDebugOut("Warning: Game int option %d unknown\n", gameOption );
						break;
					}
				}
			}
			else if( strcmp( cmd, NET_DARWINIA_SETGAMEOPTIONARRAYINT ) == 0 )
			{
				AppDebugOut("Game Option Array  Int\n" );
				int gameOption = update->GetDataInt(NET_DARWINIA_GAMEOPTION_NAME);
				int count = update->GetDataInt(NET_DARWINIA_GAMEOPTION_INDEX);
				int version = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VERSION);
				unsigned numElems = 0;
				int *values = update->GetDataInts(NET_DARWINIA_GAMEOPTION_VALUE, &numElems);
				
				switch( gameOption )
				{
				case GAMEOPTION_GAME_PARAM:
					g_app->m_multiwinia->SetGameOptions( g_app->m_multiwinia->m_gameType, values );
					break;

				case GAMEOPTION_GAME_RESEARCH_LEVEL:
					g_app->m_multiwinia->SetGameResearch( values );
					break;

				default:
					AppDebugOut("Warning: Game array int option %d unknown\n", gameOption );
					break;
				}
			}
			else if( strcmp( cmd, NET_DARWINIA_SETGAMEOPTIONSTRING ) == 0 )
			{
				int gameOption = update->GetDataInt(NET_DARWINIA_GAMEOPTION_NAME);
				const char *value = update->GetDataString(NET_DARWINIA_GAMEOPTION_VALUE);
				int version = update->GetDataInt(NET_DARWINIA_GAMEOPTION_VERSION);

				if( version > GameOptionVersion( gameOption ) )
				{
					AppDebugOut( "Update to game string option %d (we have version %d) and client gave version %d. %s ==> %s\n",  
						gameOption, GameOptionVersion( gameOption ), version,
						g_app->m_requestedMap, value );
					SetGameOptionVersion( gameOption, -1, version );
					switch( gameOption )
					{
					case GAMEOPTION_MAPNAME:
						strcpy( g_app->m_requestedMap, value );	
                        if( g_app->m_server )
                        {
                            // Don't advertise in single player
                            if( GetServerPort() != -1 && 
								strcmp( g_app->m_requestedMap, "null" ) != 0 && strlen( g_app->m_requestedMap ) > 0 )
                            {
                                g_app->m_server->m_noAdvertise = false;
                            }
                            else
                            {
                                g_app->m_server->m_noAdvertise = true;
                            }
                        }
						break;

					default:
						AppDebugOut("Warning: Game string option %d unknown\n", gameOption );
						break;
					}
				}
				AppDebugOut( "Discarded update to game string option %d because we have version %d and client gave version %d\n",
					gameOption, GameOptionVersion( gameOption ), version );
			}
			else if( strcmp( cmd, NET_DARWINIA_START_GAME ) == 0 )
			{
				AppDebugOut("CLIENT : Received Start Game\n" );				
				// Start the game 

				// Prevent updates from being processed while the game is being loaded
				g_app->m_loadingLocation = true;	

				g_app->m_requestToggleEditing = false;
				g_app->m_requestedLocationId = 999;
				g_app->m_gameMode = App::GameModeMultiwinia;
				strcpy( g_app->m_requestedMission, "null" );

		        g_gameTimer.Reset();
				g_app->m_atMainMenu = false;
                g_app->m_hideInterface = false;

				if(g_app->m_atLobby)
					g_app->m_atLobby = false;
			}
			else if( strcmp( cmd, NET_DARWINIA_ALIVE ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );

				int length;
				char *data = (char *) update->GetDataVoid( NET_DARWINIA_ALIVE_DATA, &length );

				TeamControls teamControls;
				teamControls.Unpack( data, length );

				loc->UpdateTeam( teamId, teamControls );
			}
            else if( strcmp( cmd, NET_DARWINIA_TOGGLE_READY ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );

                if( g_app->m_atMainMenu )
                {
                    g_app->m_multiwinia->m_teams[teamId].m_ready = !g_app->m_multiwinia->m_teams[teamId].m_ready;
                }
                else
                {
                    Team *team = loc->m_teams[teamId];
                    team->m_ready = !team->m_ready;
                    AppDebugOut( "Team %d ready : %d\n", teamId, (int)team->m_ready );
                }
            }
            else if( strcmp( cmd, NET_DARWINIA_SET_TEAM_READY ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );

                Team *team = loc->m_teams[teamId];
                team->m_ready = true;

                AppDebugOut( "Team %d ready : %d\n", teamId, (int)team->m_ready );                
            }
			else if( strcmp( cmd, NET_DARWINIA_PING ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );

				if( teamId == g_app->m_globalWorld->m_myTeamId )
                {
                    m_latency = GetHighResTime() - m_pingSentTime;
                }
			}
			else if( strcmp( cmd, NET_DARWINIA_SELECTUNIT ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				int unitId = update->GetDataInt( NET_DARWINIA_UNITID );
				int entityId = update->GetDataInt( NET_DARWINIA_ENTITYID );
				int buildingId = update->GetDataInt( NET_DARWINIA_BUILDINGID );

				Team *team = loc->m_teams[ teamId ];

                team->SelectUnit( unitId, entityId, buildingId );
                team->m_taskManager->SelectTask( WorldObjectId( teamId, unitId, entityId, -1 ) );
			}
			else if( strcmp( cmd, NET_DARWINIA_AIMBUILDING ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				int buildingId = update->GetDataInt( NET_DARWINIA_BUILDINGID );
				Vector3 pos = DirectoryGetVector3( update, NET_DARWINIA_POSITION );

                Building *building = loc->GetBuilding( buildingId );
                if( building && 
                    building->m_id.GetTeamId() == teamId &&
                    building->m_type == Building::TypeRadarDish )
                {
                    RadarDish *radarDish = (RadarDish *) building;
                    radarDish->Aim( pos );
                }
			}
			else if( strcmp( cmd, NET_DARWINIA_TOGGLELASERFENCE ) == 0 )
			{
				int buildingId = update->GetDataInt( NET_DARWINIA_BUILDINGID );
				Building *building = loc->GetBuilding( buildingId );
				if (building && building->m_type == Building::TypeLaserFence)
				{
					LaserFence *laserfence = (LaserFence*)building;
					laserfence->Toggle();
				}
			}
			else if( strcmp( cmd, NET_DARWINIA_RUNPROGRAM ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				unsigned char program = update->GetDataUChar( NET_DARWINIA_PROGRAM );

                loc->m_teams[ teamId ]->m_taskManager->RunTask( program );
			}
			else if( strcmp( cmd, NET_DARWINIA_TARGETPROGRAM ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				unsigned char program = update->GetDataUChar( NET_DARWINIA_PROGRAM );
				Vector3 pos = DirectoryGetVector3( update, NET_DARWINIA_POSITION );

                loc->m_teams[teamId]->m_taskManager->TargetTask( program, pos );
			}
			else if( strcmp( cmd, NET_DARWINIA_SELECTPROGRAM ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				unsigned char program = update->GetDataUChar( NET_DARWINIA_PROGRAM );

                loc->m_teams[ teamId ]->m_taskManager->SelectTask( program );
			}
			else if( strcmp( cmd, NET_DARWINIA_TERMINATE ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				unsigned char program = update->GetDataUChar( NET_DARWINIA_PROGRAM );
				int entityId = update->GetDataInt( NET_DARWINIA_ENTITYID );

                Team *team = loc->m_teams[ teamId ];
                if( program != 255 )
                {
                    AppDebugOut( "Network update Terminate : team %d program %d\n", (int) teamId, (int) program );
                    team->m_taskManager->TerminateTask( program );
                }
                else
                {
                    AppDebugOut( "Network update Terminate : team %d entity %d\n", (int) teamId, entityId );
                    if( team->m_others.ValidIndex( entityId ) )
                    {
                        AppDebugOut( "Entity found\n" );
                        Entity *entity = team->m_others[entityId];
                        entity->ChangeHealth( -999 );
                    }
                }
			}
            else if( strcmp( cmd, NET_DARWINIA_NEXTWEAPON ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                Team *team = loc->m_teams[ teamId ];
                Entity *entity = team->GetMyEntity();
                if( entity )
                {
                    if( entity->m_type == Entity::TypeOfficer )
                    {
                        ((Officer *)entity)->SetNextMode();
                    }
                    else if( entity->m_type == Entity::TypeArmour )
                    {
                        ((Armour *)entity)->SetDirectOrders();
                    }
                }
            }
            else if( strcmp( cmd, NET_DARWINIA_PREVWEAPON ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                Team *team = loc->m_teams[ teamId ];
                Entity *entity = team->GetMyEntity();
                if( entity )
                {
                    if( entity->m_type == Entity::TypeOfficer )
                    {
                        ((Officer *)entity)->SetPreviousMode();
                    }
                    else if( entity->m_type == Entity::TypeArmour )
                    {
                        ((Armour *)entity)->SetDirectOrders();
                    }
                }
            }
			else if( strcmp( cmd, NET_DARWINIA_NONE ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                Team *team = loc->m_teams[ teamId ];
                Entity *entity = team->GetMyEntity();
                if( entity )
                {
                    if( entity->m_type == Entity::TypeOfficer )
                    {
						((Officer *)entity)->CancelOrders();
                    }
                }
            }
			else if( strcmp( cmd, NET_DARWINIA_FOLLOW ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                Team *team = loc->m_teams[ teamId ];
                Entity *entity = team->GetMyEntity();
                if( entity )
                {
                    if( entity->m_type == Entity::TypeOfficer )
                    {
                        ((Officer *)entity)->SetFollowMode();
                    }
                }
            }
			else if( strcmp( cmd, NET_DARWINIA_SHAMANSUMMON ) == 0 )
			{
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				int entityId = update->GetDataInt( NET_DARWINIA_ENTITYID );
                unsigned char entityType = update->GetDataUChar( NET_DARWINIA_ENTITYTYPE );

				WorldObjectId id;
				for( int i = 0; i < loc->m_teams[teamId]->m_specials.Size(); ++i )
				{
					id = *loc->m_teams[teamId]->m_specials.GetPointer(i);
					if( id.GetUniqueId() == entityId ) break;
				}
				Shaman *shaman = (Shaman *)loc->GetEntitySafe( id, Entity::TypeShaman );
				if( shaman ) 
				{
					shaman->SummonEntity( entityType );
				}
			}
            else if( strcmp( cmd, NET_DARWINIA_SELECTDGS ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                float radius = update->GetDataFloat( NET_DARWINIA_RADIUS );
                Vector3 pos = DirectoryGetVector3( update, NET_DARWINIA_POSITION );

                loc->SelectDarwinians( teamId, pos, radius );
            }
            else if( strcmp( cmd, NET_DARWINIA_ORDERDGS ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                bool directRoute = update->GetDataChar( NET_DARWINIA_DIRECTROUTE );
                Vector3 pos = DirectoryGetVector3( update, NET_DARWINIA_POSITION );

                loc->IssueDarwinianOrders( teamId, pos, directRoute );
            }
            else if( strcmp( cmd, NET_DARWINIA_ORDERGOTO ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                bool directRoute = update->GetDataChar( NET_DARWINIA_DIRECTROUTE );
                bool forceOnlyMove = update->GetDataChar( NET_DARWINIA_FORCEONLYMOVE );
                Vector3 pos = DirectoryGetVector3( update, NET_DARWINIA_POSITION );

                Team *team = loc->m_teams[ teamId ];
                Entity *entity = team->GetMyEntity();

                if( entity )
                {
                    if( entity->m_type == Entity::TypeOfficer )
                    {
                        Officer *officer = (Officer *)entity;
                        
                        if( forceOnlyMove )     officer->SetWaypoint( pos );
                        else                    officer->SetOrders( pos, directRoute );
                    }
                    else if( entity->m_type == Entity::TypeArmour )
                    {
                        Armour *armour = (Armour *)entity;
                        
                        if( forceOnlyMove )     armour->SetWayPoint( pos );                        
                        else                    armour->SetOrders( pos );

                        if( g_app->m_location->IsFriend( teamId, g_app->m_globalWorld->m_myTeamId ) )
                        {
                            g_app->m_markerSystem->RegisterMarker_Orders( armour->m_id );
                        }
                    }
                }
            }
            else if( strcmp( cmd, NET_DARWINIA_PROMOTE ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                int entityId = update->GetDataInt( NET_DARWINIA_ENTITYID );
                
                Team *team = loc->m_teams[ teamId ];

                if( team->m_others.ValidIndex(entityId) )
                {
                    Entity *entity = team->m_others[entityId];
                    if( entity && entity->m_type == Entity::TypeOfficer )
                    {
                        loc->DemoteOfficer( teamId, entityId );
                        team->SelectUnit(-1,-1,-1);
                    }
					else 
					{
						int officerId = loc->PromoteDarwinian( teamId, entityId );                
						team->SelectUnit( -1, officerId, -1 );
					}
				}
            }
            else if( strcmp( cmd, NET_DARWINIA_TOGGLEENTITY ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                int entityId = update->GetDataInt( NET_DARWINIA_ENTITYID );

                Team *team = loc->m_teams[ teamId ];
                
                if( team->m_others.ValidIndex(entityId) )
                {
                    Entity *entity = team->m_others[entityId];
                    if( entity )
                    {
                        if( entity->m_type == Entity::TypeArmour )
                        {
                            bool secondary = update->GetDataChar( NET_DARWINIA_SECONDARYFUNCTION );

                            Armour *armour = (Armour *)entity;

                            if( secondary ) armour->ToggleConversionAction();
                            else            armour->ToggleLoading();                            
                        }
                        else if( entity->m_type == Entity::TypeOfficer )
                        {
                            Officer *officer = (Officer *)entity;

                            if( officer->IsInFormationMode() ||
                                officer->m_orders == Officer::OrderGoto )
                            {
                                officer->CancelOrders();
                            }
                            else
                            {
                                officer->SetOrders( officer->m_pos + officer->m_front );
                                RequestSelectUnit( officer->m_id.GetTeamId(), -1, officer->m_id.GetIndex(), -1 );
                            }
                        }
                    }
                }
            }
			else if( strcmp( cmd, NET_DARWINIA_PAUSE ) == 0 )
			{
				// Handled above
			}
            else if( strcmp( cmd, NET_DARWINIA_REQUEST_TEAMNAME ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				UnicodeString &teamName = g_app->m_multiwinia->m_teams[teamId].m_teamName;

				bool showmsg = false;
				if( teamName.Length() == 0 )
				{
					showmsg = true;
				}

				teamName = update->GetDataUnicode( NET_DARWINIA_TEAMNAME );
				AppDebugOut("Team name set to %s at %2.4f\n", teamName.m_charstring, GetHighResTime() );

				if( showmsg )
				{
					if( g_app->m_atMainMenu )
					{
						GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow( "multiwinia_mainmenu_title" );
						if( gmw )
						{
							UnicodeString msg("PLAYERJOIN");
							gmw->NewChatMessage( msg, g_app->m_multiwinia->m_teams[teamId].m_clientId );
						}
					}
				}
            }
            else if( strcmp( cmd, NET_DARWINIA_REQUEST_COLOUR ) == 0 )
            {
                unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
                int colourId = update->GetDataInt( NET_DARWINIA_COLOURID );

                if( colourId < MULTIWINIA_NUM_TEAMCOLOURS )
                {
                    g_app->m_multiwinia->SetTeamColour( teamId, colourId );
                }
            }
            else if( strcmp( cmd, NET_DARWINIA_SEND_CHATMESSAGE ) == 0 )
            {
                if( update->HasData(NET_DARWINIA_UNICODEMESSAGE) && update->HasData(NET_DARWINIA_CLIENTID) )
                {
                    if( g_app->m_atMainMenu )
                    {
                        GameMenuWindow *gmw = (GameMenuWindow *)EclGetWindow("multiwinia_mainmenu_title");
                        if( gmw )
                        {
                            gmw->NewChatMessage( update->GetDataUnicode( NET_DARWINIA_UNICODEMESSAGE ), update->GetDataUChar( NET_DARWINIA_CLIENTID ) );
                        }
                    }
                    else
                    {
                        if( g_app->m_location )
                        {
                            g_app->m_location->NewChatMessage( update->GetDataUnicode( NET_DARWINIA_UNICODEMESSAGE ), update->GetDataUChar( NET_DARWINIA_CLIENTID ) );
                        }
                    }
                }
            }
			else if ( strcmp( cmd, NET_DARWINIA_ASSIGN_SPECTATOR ) == 0 )
			{
				unsigned char teamId = update->GetDataUChar( NET_DARWINIA_TEAMID );
				unsigned char clientID = update->GetDataUChar( NET_DARWINIA_CLIENTID );
				
				if( g_app->m_multiwinia )
				{
					g_app->m_multiwinia->RemoveTeams(clientID);
				}

				if(g_app->m_globalWorld->m_myTeamId == teamId)
				{
					g_app->m_spectator = true;
					g_app->m_globalWorld->m_myTeamId = 255;
				}
			}
            else if( strcmp( cmd, NET_DARWINIA_CHRISTMASMODE ) == 0 )
            {
                if( g_app->m_atMainMenu )
                {
                    m_christmasMode = true;
                }
            }
            else
            {
                AppDebugOut( "ClientToServer received bogus message, discarded (11)\n" );
            }
		}
	}
	return true;
}

void ClientToServer::AddIFrame( IFrame *iFrame )
{
	m_iframeHistory.PutDataAtEnd( iFrame );

	if( m_iframeHistory.Size() > 50 ) 
	{
		IFrame *frame = m_iframeHistory.GetData( 0 );
		m_iframeHistory.RemoveData(0);
		delete frame;
	}
}


void ClientToServer::SendIFrame( int _seqId )
{
	int size = m_iframeHistory.Size();

	for( int i = 0; i < size; i++ )
	{
		IFrame *frame = m_iframeHistory.GetData( i );

		if( frame->SequenceId() == _seqId )
		{
			char filename[256];
			sprintf(filename, "IFrame %d %d", m_clientId, _seqId );
			
			m_ftpManager->SendFile( filename, frame->Data(), frame->Length() );
			g_app->m_multiwinia->SaveIFrame( m_clientId, new IFrame(*frame) );
		}
	}
}



void ClientToServer::ProcessFTPLetter( Directory *_letter )
{
	m_ftpManager->ReceiveLetter( _letter );
}
