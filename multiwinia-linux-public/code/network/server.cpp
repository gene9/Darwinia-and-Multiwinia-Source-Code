#include "lib/universal_include.h"

#include <time.h>
#include <fstream>

#include "lib/netlib/net_lib.h"
#include "lib/netlib/net_mutex.h"
#include "lib/netlib/net_socket.h"
#include "lib/netlib/net_socket_listener.h"
#include "lib/netlib/net_thread.h"
#include "lib/netlib/net_udp_packet.h"

#include "lib/debug_utils.h"
#include "lib/hi_res_time.h"
#include "lib/profiler.h"
#include "lib/preferences.h"

#include "lib/vector3.h"
#include "lib/hash.h"

#include "app.h"
#include "globals.h"
#include "main.h"
#include "team.h"
#include "multiwinia.h"

#include "network/generic.h"
#include "network/server.h"
#include "network/servertoclient.h"
#include "network/servertoclientletter.h"
#include "network/clienttoserver.h"
#include "network/network_defines.h"
#include "network/ftp_manager.h"

#include "lib/metaserver/metaserver_defines.h"
#include "game_menu.h"

#include "lib/math/random_number.h"
#include "achievement_tracker.h"

#include "sound/soundsystem.h"

#include "UI/MapData.h"

#include "lib/metaserver/metaserver.h"
#include "lib/metaserver/authentication.h"
#include "lib/metaserver/matchmaker.h"

// ****************************************************************************
// Class ServerTeam
// ****************************************************************************

ServerTeam::ServerTeam(int _clientId, int _teamType)
:   m_clientId( _clientId ),
	m_teamType( _teamType )
{
}


// ****************************************************************************
// Class Server
// ****************************************************************************

// ***ListenCallback
static NetCallBackRetType ServerListenCallback(NetUdpPacket *udpdata)
{
    if( !udpdata )
		return 0;

	Server *server = g_app->m_server;

	if( !server )
	{
		delete udpdata;
		return 0;
	}

    NetIpAddress *fromAddr = &udpdata->m_clientAddress;
    char newip[16];
	IpToString(fromAddr->sin_addr, newip);
    int newPort = udpdata->GetPort();	

    Directory *letter = new Directory();
    bool success = letter->Read( udpdata->m_data, udpdata->m_length );
    if( success )
    {
        server->ReceiveLetter( letter, newip, newPort, udpdata->m_length );
    }
    else
    {
        AppDebugOut( "Server received bogus letter, discarded (10)\n" );
        delete letter;
    }

	delete udpdata;
    return 0;
}

Server::Server()
:   m_sequenceId(0),
	m_nextClientId(0),
    m_inboxMutex(NULL),
    m_outboxMutex(NULL),
	m_listener(NULL),
	m_nextServerAdvanceTime(0),
	m_syncRandSeed( 0 ),
	m_random( 0 ),
	m_startTime( -1.0 ),
	m_noAdvertise(false),
	m_gameStartMsgReceived(false)
{
}


Server::~Server()
{
	AppDebugOut("Shutting down server\n" );
	MetaServer_StopRegisteringOverLAN();
    MetaServer_StopRegisteringOverWAN();
    MatchMaker_StopRequestingIdentity( m_listener );
	
	m_listener->StopListening();
	delete m_listener;

    m_history.EmptyAndDelete();
    m_clients.EmptyAndDelete();
	m_disconnectedClients.EmptyAndDelete();
    m_teams.EmptyAndDelete();

    m_inboxMutex->Lock();    
    m_inbox.EmptyAndDelete();
    m_inboxMutex->Unlock();

    m_outboxMutex->Lock();
    m_outbox.EmptyAndDelete();
    m_outboxMutex->Unlock();
}

static NetCallBackRetType ListenThread(void *ptr)
{
	Server *server = (Server *) ptr;
	server->StartListening();
    return 0;
}

void Server::StartListening()
{
	m_listener->StartListening( ServerListenCallback );
}

static unsigned MakeRandom()
{
#ifdef _WIN32
	unsigned threadId = GetCurrentThreadId();
	double theTime = GetHighResTime();
	return  threadId ^  *(unsigned *) &theTime;
#elif TARGET_OS_MACOSX
	return (unsigned)pthread_self(); // what's good for the goose...
#endif
}

static int GenSyncRandSeed( double _startTime, unsigned _random )
{
	unsigned int hash[5];
	hash_context c;
	
	hash_initial( &c );
	Hash( c, _random );
	Hash( c, _startTime );
	hash_final( &c, hash );

	return (int) hash[0];
}

void Server::Initialise()
{
	m_iClientCount = 0;

    m_inboxMutex = new NetMutex();
    m_outboxMutex = new NetMutex();

	int ourPort = g_prefsManager->GetInt( PREFS_NETWORKSERVERPORT, 4000 );	

	m_listener = new NetSocketListener( ourPort, "Server");
	NetRetCode result = m_listener->Bind();

	if( result != NetOk )
	{
		// TODO PORT FORWARDING: 		
		// We need to indicate to the user that his chosen port is unavailable instead of 
		// just dynamically allocating one
	}

    NetStartThread( ListenThread, this );

	m_startTimeActual = time(NULL);
	m_startTime = GetHighResTime();
	m_random = MakeRandom();
	m_syncRandSeed = GenSyncRandSeed( m_startTime, m_random );
	m_gameStartMsgReceived = false;
	
	m_nextServerAdvanceTime = m_startTime + 0.1;
}

// *** GetClientId

ServerToClient *Server::GetClient( int _id )
{
    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *stc = m_clients[i];
            if( stc->m_clientId == _id )
            {
                return stc;
            }
        }
    }

    return NULL;
}


bool Server::CheckPlayerCanJoin( Directory* _incoming )
{
	return true;
}

int Server::GetClientId( const char *_ip, int _port )
{
    for ( int i = 0; i < m_clients.Size(); ++i ) 
    {
        if ( m_clients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_clients[i];
            
            if( strcmp ( sToC->m_ip, _ip ) == 0 &&
                sToC->m_port == _port )
            {
                return m_clients[i]->m_clientId;
            }
        }
    }

    return -1;
}

int Server::ConvertIPToInt( const char *_ip )
{
	AppReleaseAssert(strlen(_ip) < 17, "IP address too long");
    char ipCopy[17];
	strcpy(ipCopy, _ip);
    int ipLen = strlen(ipCopy);

    for( int i = 0; i < ipLen; ++i )
    {
        if( ipCopy[i] == '.' )
        {
            ipCopy[i] = '\n';
        }
    }

    int part1, part2, part3, part4;
    sscanf( ipCopy, "%d %d %d %d", &part1, &part2, &part3, &part4 );

    int result = (( part4 & 0xff ) << 24) +
                 (( part3 & 0xff ) << 16) +
                 (( part2 & 0xff ) << 8) +
                  ( part1 & 0xff );
    return result;
}


char *Server::ConvertIntToIP ( const int _ip )
{
    static char result[16];
    sprintf( result, "%d.%d.%d.%d", (_ip & 0x000000ff),
                                    (_ip & 0x0000ff00) >> 8,
                                    (_ip & 0x00ff0000) >> 16,
                                    (_ip & 0xff000000) >> 24 );

    return result;
}


int Server::RegisterNewClient ( Directory *_client, int _clientId )
{
    char *ip = _client->GetDataString( NET_DARWINIA_FROMIP );
    int port = _client->GetDataInt( NET_DARWINIA_FROMPORT );

    AppAssert( GetClientId( ip, port ) == -1 );

    ServerToClient *sToC = new ServerToClient( ip, port, m_listener );

    strcpy( sToC->m_authKey, _client->GetDataString( NET_METASERVER_AUTHKEY ) );
    sToC->m_authKeyId = _client->GetDataInt( NET_METASERVER_AUTHKEYID );

	if( _clientId == -1 )
    {
        sToC->m_clientId = m_nextClientId;
        m_nextClientId++;
    }
    else
    {
        sToC->m_clientId = _clientId;
    }

    if( _client->HasData( NET_DARWINIA_SERVER_PASSWORD ) )
    {
        sToC->m_password =  _client->GetDataUnicode( NET_DARWINIA_SERVER_PASSWORD );
    }

    m_clients.PutData(sToC);

    //
    // Try to authenticate this person

	Authentication_RequestStatus( sToC->m_authKey, METASERVER_GAMETYPE_MULTIWINIA, sToC->m_authKeyId, ip );
    
    //
    // Tell all clients about it
	Directory *data = new Directory;
	data->SetName( NET_DARWINIA_MESSAGE );
	data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CLIENTHELLO );
	data->CreateData( NET_DARWINIA_CLIENTID, sToC->m_clientId );

	// Tell about the start time and random number 
	data->CreateData( NET_METASERVER_STARTTIME, (unsigned long long ) m_startTimeActual );
	data->CreateData( NET_METASERVER_RANDOM, (int) m_random );

    if( Authentication_IsDemoKey( sToC->m_authKey ) )
    {
        data->CreateData( NET_DARWINIA_CLIENTISDEMO, true );
    }

    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->m_data = data;
    SendLetter( letter );

    //
    // Return assigned clientID
	return sToC->m_clientId;
}


void Server::SendClientId( int _clientId )
{
    if( _clientId != -1 )
    {
        ServerToClient *sToC = GetClient(_clientId);
        AppAssert(sToC);

        Directory *data = new Directory;
        data->SetName( NET_DARWINIA_MESSAGE );
        data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CLIENTID );
        data->CreateData( NET_DARWINIA_CLIENTID, _clientId );
        data->CreateData( NET_DARWINIA_SEQID, -1 );
        data->CreateData( NET_DARWINIA_VERSION, APP_VERSION );

#ifdef TESTBED_ENABLED
		if(m_bRandSeedOverride)
		{
			if(m_iRandSeed != 0)
			{
				data->CreateData( NET_DARWINIA_RANDSEED, m_iRandSeed );
			}
			else
			{
				data->CreateData( NET_DARWINIA_RANDSEED, m_syncRandSeed );
			}
		}
		else
		{
			data->CreateData( NET_DARWINIA_RANDSEED, m_syncRandSeed );
		}
#else
		data->CreateData( NET_DARWINIA_RANDSEED, m_syncRandSeed );
#endif

		// At this point I am going to decide to make the player a spectator or not based on the fill status of the server

		if(g_app->GetMaxNumberofPlayers())
		{
			int iTotalPlayers = g_app->GetMaxNumberofPlayers();

			// We want to reject the player if the server is full
			if(iTotalPlayers != 0)
			{
				if(m_clients.NumUsed() > iTotalPlayers)
				{
					AppDebugOut("Clients Used: %d, total players %d, making spectator\n",
						m_clients.NumUsed(), iTotalPlayers);
					data->CreateData( NET_DARWINIA_MAKE_SPECTATOR, 1 );

				}
				else
				{
					data->CreateData( NET_DARWINIA_MAKE_SPECTATOR, 0 );
				}
			}
		}
        
        ServerToClientLetter *letter = new ServerToClientLetter();
		letter->m_data = data;
        letter->m_receiverId = _clientId;

        m_outboxMutex->Lock();
        m_outbox.PutDataAtStart( letter );
        m_outboxMutex->Unlock();

		char fromStr[64];
		sprintf( fromStr, "%s:%d", sToC->m_ip, sToC->m_port );

		AppDebugOut( "SERVER: Client at %s requested ID.  Sent ID %d (syncrand %d).\n", fromStr, _clientId, m_syncRandSeed );
    }
}


void Server::RemoveClient( int _clientId, int _reason )
{
    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_clients[i];
            if( sToC->m_clientId == _clientId )
			{
				static const char *stringReasons[] = {
					"client left",
					"client dropped",
					"client needed pdlc",
					"server full",
					"invalid key",
					"duplicate key",
					"bad password",
					"demo full",
                    "key authentication failed",
                    "kicked by server"
					};
    
				const char *stringReason = stringReasons[_reason];

				char fromStr[64];
				sprintf( fromStr, "%s:%d", sToC->m_ip, sToC->m_port );
				
				AppDebugOut("SERVER: Client at %s disconnected (%s)\n", 
                                            fromStr, 
											stringReason );
				//
				// Tell all clients about it

				ServerToClientLetter *letter = new ServerToClientLetter();

				letter->m_data = new Directory();
				letter->m_data->SetName( NET_DARWINIA_MESSAGE );
				letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_CLIENTGOODBYE );
				letter->m_data->CreateData( NET_DARWINIA_CLIENTID, sToC->m_clientId );
				letter->m_data->CreateData( NET_DARWINIA_DISCONNECT, _reason );

				SendLetter( letter );

				//
                // Tell the client specifically
                // (Unless he's telling us he left)

                if( _reason != Disconnect_ClientLeave )
                {
					AppDebugOut("The client did not leave, so sending them a message\n");
                    letter = new ServerToClientLetter();
                    letter->m_data = new Directory();
                    letter->m_data->SetName( NET_DARWINIA_MESSAGE );
                    letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_DISCONNECT );
                    letter->m_data->CreateData( NET_DARWINIA_DISCONNECT, _reason );
                    letter->m_receiverId = sToC->m_clientId;
                    letter->m_data->CreateData( NET_DARWINIA_SEQID, -1);

                    m_outbox.PutDataAtEnd( letter );                
                }

                AdvanceSender();

				m_clients.RemoveData( i );
				m_disconnectedClients.PutData(sToC);
				sToC->m_disconnected = _reason;

				//
				// Remove his teams if we're in the lobby

				if( g_app->m_multiwinia->GameInLobby() )
				{
					for( int j = 0; j < m_teams.Size(); j++ )
					{
						if( m_teams.ValidIndex( j ) )
						{
							ServerTeam *st = m_teams[j];
							if( st->m_clientId == _clientId )
							{
								AppDebugOut("Removing server team %d for client %d\n", j, _clientId );
								m_teams.RemoveData( j );
								delete st;
								j--;
							}
						}
					}
				}

				break;
			}
		}
	}
}


void Server::RegisterNewTeam ( int _clientId, int _teamType, int _desiredTeamId )
{               
	
	ServerToClient *sToC = GetClient(_clientId);
    AppAssert( sToC );

	// Ignore team requests from spectators
	if( sToC->m_spectator )
		return;
 
	/*
	// TODO: Remove desiredTeamId code
    if( _desiredTeamId != -1 )                              // Specified Team ID - An AI
    {                                                       
        if( !m_teams.ValidIndex(_desiredTeamId) )
        {
            ServerTeam *team = new ServerTeam( _clientId, _teamType );
			
			if (m_teams.Size() <= _desiredTeamId)
			{
				m_teams.SetSize(_desiredTeamId+1);
			}

			m_teams.PutData(team, _desiredTeamId);
        }
    }
    else
	*/
    {
		//
		// If we already have a local team do something different

		if( _teamType == TeamTypeLocalPlayer )
		{
			for( int i = 0; i < m_teams.Size(); ++i )
			{
				if( m_teams.ValidIndex(i) )
				{
					ServerTeam *team = m_teams[i];
					if( team->m_clientId == _clientId &&
						team->m_teamType == TeamTypeLocalPlayer )
					{
						AppDebugOut( "SERVER: Failed to create new team - this client already has a Team\n" );
						return;
					}
				}
			}

			// Single player
			if(g_app->m_clientToServer->GetServerPort() == -1)
			{
				int nbPlayerTeams = 0;
				for( int i = 0; i < m_teams.Size(); ++i )
				{
					if( m_teams.ValidIndex(i) )
					{
						ServerTeam *team = m_teams[i];
						if( team->m_teamType == TeamTypeLocalPlayer ||
						    team->m_teamType == TeamTypeRemotePlayer )
						{
							nbPlayerTeams++;
						}
					}
				}

				if(nbPlayerTeams > 0)
				{
					// Remove the client
					RemoveClient(_clientId,Disconnect_ServerFull);
					return;
				}
			}

			if(m_teams.NumUsed() >= g_app->GetMaxNumberofPlayers() && (g_app->GetMaxNumberofPlayers() != 0))
			{
				// Remove the client
				RemoveClient(_clientId,Disconnect_ServerFull);
				return;
			}

		}
		else if( _teamType == TeamTypeCPU )
		{
			// If the server is full then reject
			if(m_teams.NumUsed() >= g_app->GetMaxNumberofPlayers())
			{
				return;
			}
		}
		else
		{
			// Check if this player is already here
			for( int i = 0; i < m_teams.Size(); ++i )
			{
				if( m_teams.ValidIndex(i) )
				{
					ServerTeam *team = m_teams[i];

					if( team->m_clientId == _clientId )
					{
						AppDebugOut( "SERVER: Failed to create new team - this client already has a Team\n" );
						return;
					}
				}
			}

			// If single player or the server is full then reject
			if(g_app->m_clientToServer->GetServerPort() == -1 || 
			   (m_teams.NumUsed() >= g_app->GetMaxNumberofPlayers() && (g_app->GetMaxNumberofPlayers() != 0)))
			{
				// Remove the client
				RemoveClient(_clientId,Disconnect_ServerFull);
				return;
			}

		}

        AppDebugAssert( m_teams.NumUsed() < NUM_TEAMS );
        ServerTeam *team = new ServerTeam( _clientId, _teamType );
        int teamId = m_teams.PutData( team );

        ServerToClientLetter *letter = new ServerToClientLetter();

		letter->m_data = new Directory();
		letter->m_data->SetName( NET_DARWINIA_MESSAGE );
		letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_TEAMASSIGN );
		letter->m_data->CreateData( NET_DARWINIA_TEAMID, teamId );
		letter->m_data->CreateData( NET_DARWINIA_TEAMTYPE, _teamType );
		letter->m_data->CreateData( NET_DARWINIA_CLIENTID, _clientId );
        
        if( m_clients.ValidIndex(_clientId) )
        {
            ServerToClient *client = m_clients[ _clientId ];
            if( Authentication_IsDemoKey( client->m_authKey ) )
            {            
                letter->m_data->CreateData( NET_DARWINIA_CLIENTISDEMO, true );
            }
        }

        SendLetter( letter );

    }
}

Directory *Server::GetNextLetter()
{
    m_inboxMutex->Lock();
    Directory *letter = NULL;

    if( m_inbox.Size() > 0 )
    {
        letter = m_inbox[0];
        m_inbox.RemoveData(0);
    }

    m_inboxMutex->Unlock();
    return letter;
}


void Server::ReceiveLetter( Directory *update, const char *fromIP, int _fromPort, int _bandwidthUsed )
{
	m_receiveRate.Count( _bandwidthUsed );

    update->CreateData( NET_DARWINIA_FROMIP, fromIP );
    update->CreateData( NET_DARWINIA_FROMPORT, _fromPort ); 	

    if( strcmp( update->m_name, NET_MATCHMAKER_MESSAGE ) == 0 )
    {
        MatchMaker_ReceiveMessage( m_listener, update );
		delete update;
    }
	else {
		m_inboxMutex->Lock();
		m_inbox.PutDataAtEnd(update);
		m_inboxMutex->Unlock();
	}
}


void Server::SendLetter( ServerToClientLetter *letter )
{
    if( letter )
    {
		//
		// Assign a sequence id

        letter->m_data->CreateData( NET_DARWINIA_SEQID, m_sequenceId);
        m_sequenceId++;

        m_history.PutDataAtEnd( letter );
    }   
}

// To handle waking from sleep for instance
void Server::SkipToTime( double _timeNow )
{	
	m_nextServerAdvanceTime = _timeNow;
}

void Server::AdvanceIfNecessary( double _timeNow )
{
	if( _timeNow > m_nextServerAdvanceTime )
    {
		// AppDebugOut("%f, Advancing server\n", _timeNow );
        g_app->m_server->Advance();

#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() == TESTBED_ON)
		{
			if(g_app->GetTestBedType() == TESTBED_SERVER)
			{
				if(g_app->m_server->m_sequenceId > g_lastProcessedSequenceId + 30)
				{
					m_nextServerAdvanceTime += 0.1f;
				}
				else
				{
					// Advance time 10 times faster than normal mode
					m_nextServerAdvanceTime += 0.1f;
				}
			}
			else
			{
				// Advance time 10 times faster than normal mode
				m_nextServerAdvanceTime += 0.1f;
			}


		}
		else
		{
			
			m_nextServerAdvanceTime += 0.1f;
		}
#else
	m_nextServerAdvanceTime += 0.1f;
#endif   
	}
}        		



void Server::AdvanceSender()
{
    static int s_largest = 0;

    m_outboxMutex->Lock();

    while (m_outbox.Size())
    {
        ServerToClientLetter *letter = m_outbox[0];
        AppDebugAssert(letter);

		letter->m_data->CreateData( NET_DARWINIA_LASTSEQID, m_sequenceId );

		// Send directly to locally connected client without passing through
		// the socket system
		if( g_app->m_clientToServer->m_clientId == letter->m_receiverId )
        {
            g_app->m_clientToServer->ReceiveLetter( letter->m_data );
			letter->m_data = NULL;
			delete letter;
        }
        else
        {                
			int linearSize = 0;
            ServerToClient *client = GetClient(letter->m_receiverId);

			if( client == NULL )
			{
				AppDebugOut("Client is null. Checking disconnected client list\n");
				client = GetDisconnectedClient( letter->m_receiverId );
				AppDebugOut("Client was %sin disconnected client list\n", client == NULL ? "not " : "");
			}

			if( client != NULL )
			{

				int seqId = letter->m_data->GetDataInt(NET_DARWINIA_SEQID);
				const char *cmd = "";

				Directory *firstSubDir = letter->m_data->GetDirectory(NET_DARWINIA_MESSAGE);
				if (firstSubDir)
					cmd = firstSubDir->GetDataString(NET_DARWINIA_COMMAND);

				// AppDebugOut("Sending letter %d to %s:%d [%s]\n", seqId, client->GetIP(), client->m_port, cmd);
				// letter->m_data->DebugPrint(0);

				char *linearisedLetter = letter->m_data->Write(linearSize);  

				NetSocketSession *socket = client->GetSocket();
				socket->WriteData( linearisedLetter, linearSize );

				unsigned totalSize = linearSize + UDP_HEADER_SIZE;
				m_sendRate.Count( totalSize );

				double now = GetHighResTime();
				static double lastPrintTime = now;

				if( linearSize > s_largest )
				{
					s_largest = totalSize;
					AppDebugOut( "SERVER: Largest datagram sent : %d bytes\n", s_largest );
#ifdef TARGET_DEBUG
					//letter->m_data->DebugPrint( 0 );
#endif
				}			
				else if( now > lastPrintTime + 5 )
				{
					lastPrintTime = now;
#ifdef TARGET_DEBUG
					AppDebugOut( "server: sending datagram of %d bytes\n", totalSize );
					//letter->m_data->DebugPrint( 0 );
#endif
	
                }

				delete letter;
				delete [] linearisedLetter;
			}
        }

		// The letter has now been sent so we can take it off the outbox list
        m_outbox.RemoveData(0);
    }

    m_outboxMutex->Unlock();
}


void Server::NotifyNetSyncError( int _clientId, int _sequenceId )
{
    char syncErrorId[256];
    time_t theTimeT = time(NULL);
    tm *theTime = localtime(&theTimeT);
    sprintf( syncErrorId, "%02d-%02d-%02d %d.%02d.%d %d", 
                            1900+theTime->tm_year, 
                            theTime->tm_mon+1,
                            theTime->tm_mday,
                            theTime->tm_hour,
                            theTime->tm_min,
                            theTime->tm_sec,
							_sequenceId );


    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->m_data = new Directory();
    letter->m_data->SetName( NET_DARWINIA_MESSAGE );
    letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_NETSYNCERROR );
    letter->m_data->CreateData( NET_DARWINIA_CLIENTID, _clientId );
    letter->m_data->CreateData( NET_DARWINIA_SYNCERRORID, syncErrorId );
	letter->m_data->CreateData( NET_DARWINIA_SYNCERRORSEQID, _sequenceId );
    SendLetter( letter );
    
    AppDebugOut( "SYNCERROR Server: Notified all clients they are out of sync\n" );
    // AppDebugOut( "SYNCERROR Server: Notified client %d he is out of sync\n", _clientId );
}


void Server::NotifyNetSyncFixed ( int _clientId )
{
    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->m_data = new Directory();
    letter->m_data->SetName( NET_DARWINIA_MESSAGE );
    letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_NETSYNCFIXED );
    letter->m_data->CreateData( NET_DARWINIA_CLIENTID, _clientId );
    SendLetter( letter );

    AppDebugOut( "SYNCFIXED Server: Notified client %d his Sync Error is now corrected\n", _clientId );
}

static bool SaveLast( const char *_incomingCmd, const char *_cmd, Directory *& _last, Directory *& _incoming )
{
	if( strcmp( _incomingCmd, _cmd ) == 0 )
	{
		delete _last;
		_last = _incoming;
		_incoming = NULL;
		return true;
	}
	return false;
}

static void AddLast( ServerToClientLetter *_letter, Directory *& _msg)
{
	if( _msg )
	{
		_letter->AddUpdate( _msg );
		delete _msg;
		_msg = NULL;
	}
}

static bool SecondMessage( const char *_incomingCmd, const char *_cmd, bool &_messageAlreadyReceived, Directory *& _incoming )
{
	if( strcmp( _incomingCmd, _cmd ) == 0 )
	{
		if( _messageAlreadyReceived )
		{
			delete _incoming;
			_incoming = NULL;
			return true;
		}
		else
			_messageAlreadyReceived = true;
	}
	return false;
}


void Server::Advance()
{
    START_PROFILE( "Advance Server");

	//
	// Compile all incoming messages into a ServerToClientLetter

	ServerToClientLetter *letter = new ServerToClientLetter();

    letter->m_data = new Directory();
    letter->m_data->SetName( NET_DARWINIA_MESSAGE );
    letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_UPDATE );

	while( true )
	{
        Directory *incoming = GetNextLetter();

        if( !incoming ) break;

        //
        // Sanity check the message

        if( strcmp(incoming->m_name, NET_DARWINIA_MESSAGE) != 0 ||
            !incoming->HasData( NET_DARWINIA_COMMAND, DIRECTORY_TYPE_STRING ) )
        {
            AppDebugOut( "Server received bogus message, discarded (12)\n" );
            delete incoming;
            continue;
        }

        char *cmd       = incoming->GetDataString(NET_DARWINIA_COMMAND);
        int lastSeqId   = incoming->GetDataInt(NET_DARWINIA_LASTSEQID);

		char fromStr[64];

		char *fromIp    = incoming->GetDataString(NET_DARWINIA_FROMIP);
        int fromPort    = incoming->GetDataInt(NET_DARWINIA_FROMPORT);
		int clientId	= GetClientId( fromIp, fromPort );

		sprintf( fromStr, "%s:%d", fromIp, fromPort );
		bool isDisconnectedClient = IsDisconnectedClient(fromIp, fromPort);
		if( isDisconnectedClient && !strcmp(cmd, NET_DARWINIA_CLIENT_JOIN) == 0 )
		{
			AppDebugOut("Disconnected client message from %s\n", fromStr);
			bool handled = HandleDisconnectedClient( incoming ); 
			AppDebugOut("We %s disconnected client\n", handled ? "handled" : "did not handle");
		}
		else if( strcmp(cmd, NET_DARWINIA_CLIENT_JOIN) == 0 )
		{
			bool rejectJoin = false;

			if( clientId == -1 )
			{
				RemoveDisconnectedClient( fromIp, fromPort );

				AppDebugOut ( "SERVER: New Client connected from %s\n", fromStr );
				clientId = RegisterNewClient( incoming );

				SendClientId( clientId );
			}
		}
        else if ( strcmp(cmd, NET_DARWINIA_CLIENT_LEAVE) == 0 )
        {
            if( clientId != -1 )
            {
				AppDebugOut ( "SERVER: Client at %s disconnected gracefully\n", fromStr );
                RemoveClient( clientId, Disconnect_ClientLeave );                
            }
        }
        else if ( strcmp(cmd, NET_DARWINIA_REQUEST_TEAM ) == 0 )
        {
            if( clientId != -1 /* && !g_app->m_gameRunning */ )
			{


				AppDebugOut ( "SERVER: New team request from %s\n", fromStr );
				int teamType = incoming->GetDataInt(NET_DARWINIA_TEAMTYPE);
				int desiredTeamId = incoming->GetDataInt(NET_DARWINIA_DESIREDTEAMID);
				RegisterNewTeam( clientId, teamType, desiredTeamId );

           }
        }
		else if ( strcmp( cmd, NET_DARWINIA_REQUEST_SPECTATOR ) == 0 )
		{
			unsigned char teamId = incoming->GetDataUChar( NET_DARWINIA_TEAMID );
			unsigned char clientId = incoming->GetDataUChar( NET_DARWINIA_CLIENTID );

			// BYRON TODO: Explain rationale here
			for(int i = 0; i < m_teams.Size(); i++)
			{
				if(m_teams.ValidIndex(i))
				{
					ServerTeam* pTeam = m_teams[i];

					if(pTeam->m_clientId == (int)clientId)
					{
						m_teams.RemoveData(i);
						break;
					}
				}
			}

			// If single player then reject
			if(g_app->m_clientToServer->GetServerPort() == -1)
			{
				// Remove the client
				RemoveClient(clientId,Disconnect_ServerFull);
				return;
			}

			// BYRON TODO: Move this into ClientToServer::RequestSpectator method
			// Send a assign letter
			Directory *letter = new Directory();
			letter->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_ASSIGN_SPECTATOR );
			letter->CreateData( NET_DARWINIA_TEAMID, teamId);
			letter->CreateData( NET_DARWINIA_CLIENTID, clientId);
			g_app->m_clientToServer->SendLetter( letter );
		}
        else if( strcmp(cmd, NET_DARWINIA_SYNCHRONISE) == 0 )
        {
            if( clientId != -1 )
            {
                HandleSyncMessage( incoming, clientId );
            }
        }
		else if( strcmp(cmd, NET_DARWINIA_FILESEND) == 0 )
		{
			if( clientId != -1)
			{
				ServerToClient *sToC = GetClient( clientId );
				
				FTPManager * ftpManager = sToC->GetFTPManager();
				ftpManager->ReceiveLetter( incoming );
			}
		}
		else 
        {
			// TO DO: See Defcon for special case on removing AI team

			int teamId = incoming->GetDataUChar(NET_DARWINIA_TEAMID);

			if( teamId != 255 &&  clientId != -1 )
			{
				ServerToClient *sToC = GetClient( clientId );

				// We only keep the last ALIVE message from the clients. This prevents a 
				// growing snowball of ALIVE messages if the server starts to fall behind 
				// the update rate for whatever reason.
				//
				// Similary, we only keep the last NET_DARWINIA_SELECTUNIT as these can be 
				// sent multiple times within a session.
					
				if( !SaveLast( cmd, NET_DARWINIA_ALIVE, sToC->m_lastAlive, incoming ) &&
					!SaveLast( cmd, NET_DARWINIA_SELECTUNIT, sToC->m_lastSelectUnit, incoming ) &&
					!SaveLast( cmd, NET_DARWINIA_REQUEST_COLOUR, sToC->m_lastTeamColour, incoming ) &&
					!SecondMessage( cmd, NET_DARWINIA_START_GAME, m_gameStartMsgReceived, incoming ) )
				{
					letter->AddUpdate( incoming );
				}
			}
        }
    
		if( clientId != -1 )
		{
			ServerToClient *sToc = GetClient( clientId ); 

			if (sToc) 
			{
				// It could be that the client has disconnected, in which 
				// case he won't be listed in our clients list anymore.

				sToc->m_lastMessageReceived = GetHighResTime();
				if( lastSeqId > sToc->m_lastKnownSequenceId )
				{
					sToc->m_lastKnownSequenceId = lastSeqId;
				}
			}
		}

		delete incoming;
		incoming = NULL;
	}

	// Add in Alive messages
	int clientsSize = m_clients.Size();
    for( int i = 0; i < clientsSize; ++i )
    {
        if( m_clients.ValidIndex(i) )
		{
			ServerToClient *sToC = m_clients[i];

			AddLast( letter, sToC->m_lastAlive );
			AddLast( letter, sToC->m_lastSelectUnit );
			AddLast( letter, sToC->m_lastTeamColour );
		}
	}

	SendLetter( letter );


	//
    // Update all clients by sending the next updates to them
    
    START_PROFILE( "UpdateClients" );
    UpdateClients();
    END_PROFILE( "UpdateClients" );

    //
    // Authenticate all connected clients
    // Check for duplicate key usage
    // Kick clients who are using dodgy keys

    START_PROFILE( "Authentication" );
    AuthenticateClients();
    END_PROFILE("Authentication");

    START_PROFILE(  "SEND" );
	AdvanceSender();
    END_PROFILE(  "SEND" );

    //
    // Advertise our existence

    START_PROFILE( "Advertise" );
	if( !m_noAdvertise )
	{
		Advertise();
	}
    END_PROFILE( "Advertise" );

    END_PROFILE( "Advance Server");
}


int Server::CountEmptyMessages( int _startingSeqId )
{
    int result = 0;

    for( int i = _startingSeqId; i < m_history.Size(); ++i )
    {
        if( m_history.ValidIndex(i) )
        {
            ServerToClientLetter *theLetter = m_history[i];
            char *cmd = theLetter->m_data->GetDataString( NET_DARWINIA_COMMAND );
            if( strcmp(cmd, NET_DARWINIA_UPDATE ) == 0 &&
                       theLetter->m_data->m_subDirectories.NumUsed() == 0 )
            {
                result++;
            }
            else
            {
                break;
            }
        }       
    }
    
    return result;
}

void Server::AuthenticateClients ()
{
    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *s2c = m_clients[i];
            
            // Do a basic check
            if( s2c->m_basicAuthCheck == 0 )
            {
                AuthenticateClient( s2c->m_clientId );
            }

			
			// BYRON TODO - re-enabled this check. I had to remove it because it was stopping me from testing
            // Do a key check via the metaserver
            int keyResult = Authentication_GetStatus( s2c->m_authKey );
            if( keyResult < 0 )
			{
                s2c->m_basicAuthCheck = -3;
            }


		            
            // If its bad kick them now
            if( s2c->m_basicAuthCheck < 0 )
            {
#if AUTHENTICATION_LEVEL == 1
                int kickReason = ( s2c->m_basicAuthCheck == -1 ? Disconnect_InvalidKey :
                                   s2c->m_basicAuthCheck == -2 ? Disconnect_DuplicateKey :
                                   s2c->m_basicAuthCheck == -3 ? Disconnect_KeyAuthFailed :
                                   s2c->m_basicAuthCheck == -4 ? Disconnect_BadPassword :
                                   s2c->m_basicAuthCheck == -5 ? Disconnect_ServerFull :
                                   s2c->m_basicAuthCheck == -6 ? Disconnect_DemoFull : 
                                                                 Disconnect_InvalidKey );

                if( g_app->m_clientToServer->GetServerPort() != -1 )
                {
                    // dont do this in single player
                    RemoveClient( s2c->m_clientId, kickReason );
                }
#endif				
            }
        }
    }
}

void Server::AuthenticateClient ( int _clientId )
{
    ServerToClient *client = GetClient(_clientId);
    AppAssert(client);

	//
    // Basic key check first

	int authResult = Authentication_SimpleKeyCheck( client->m_authKey, METASERVER_GAMETYPE_MULTIWINIA );
    if( authResult < 0 )
    {
        client->m_basicAuthCheck = -1;
        AppDebugOut( "Client %d failed basic key check\n", _clientId );
        return;
    }
    

    //
    // Duplicates key check 

#ifndef TESTBED_ENABLED
#ifndef TARGET_DEBUG

    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *s2c = m_clients[i];
            bool duplicate = false;

            if( s2c->m_clientId != _clientId )
            {
                if( strcmp( s2c->m_authKey, client->m_authKey ) == 0 )
                {
                    duplicate = true;
                }

                bool isHash = Authentication_IsHashKey(s2c->m_authKey);
                if( !isHash )
                {
                    char ourHash[256];
                    int hashToken = Authentication_GetHashToken();
                    Authentication_GetKeyHash( s2c->m_authKey, ourHash, hashToken );

                    if( strcmp( ourHash, client->m_authKey ) == 0 )
                    {
                        duplicate = true;
                    }
                }

                if( duplicate )
                {
                    client->m_basicAuthCheck = -2;
                    AppDebugOut( "Client %d is using a duplicate key\n", _clientId );
                    return;
                }
            }
        }
    }
#endif
#endif


#ifdef TESTBED_ENABLED
	if(g_app->GetTestBedMode() == TESTBED_ON)
	{
		if(g_app->GetTestBedType() == TESTBED_CLIENT)
		{
			RegisterSpectator( _clientId );
		}
		else
		{
			if(_clientId != g_app->m_clientToServer->m_clientId && g_app->m_clientToServer->m_clientId != -1)
			{
				RegisterSpectator( _clientId );
				g_app->m_iClientCount++;
			}
		}
	}
	else
	{
		int numTeams = m_teams.NumUsed();
		int maxTeams = g_app->GetMaxNumberofPlayers();
		int numSpectators = GetNumSpectators();
		int maxSpectators = 4;
		
		if(maxTeams != 0 && numTeams != 0)
		{
			if( numTeams >= maxTeams)
			{
				//if( numSpectators >= maxSpectators )
				//{
					AppDebugOut( "Teams are already full, client %d cannot join\n", client->m_clientId );        
					client->m_basicAuthCheck = -5;
					return;
				//}
				//else
				//{
				//	AppDebugOut( "Teams are already full, client %d joins as spectator\n", _clientId );
				//	RegisterSpectator( _clientId );
				//}
			}

		}

	}

#else
	int numTeams = m_teams.NumUsed();
	int maxTeams = g_app->GetMaxNumberofPlayers();
	int numSpectators = GetNumSpectators();
	int maxSpectators = 4;
	
	if(maxTeams != 0 && numTeams != 0)
	{
		if( numTeams >= maxTeams )
		{
			//if( numSpectators >= maxSpectators )
			//{
				AppDebugOut( "Teams are already full, client %d cannot join\n", client->m_clientId );        
				client->m_basicAuthCheck = -5;
				return;
			//}
			//else
			//{
			//	AppDebugOut( "Teams are already full, client %d joins as spectator\n", _clientId );
			//	RegisterSpectator( _clientId );
			//}
		}

	}

    if( m_serverPassword.Length() > 0 )
    {
        if( client->m_password != m_serverPassword )
        {
            client->m_basicAuthCheck = -4;
            AppDebugOut("Client has failed the password auth check\n");
            return;
        }
    }

#endif


	// Defcon also checked:
	//
    // Are there too many teams already?
	//
    // Are there too many DEMO teams?
    // Or is this an unusual game mode?
    // Only affects this client if they are themselves a demo
    //
    // If we are running a MOD and the client does not support MODs
    // (ie < v1.2) we must disconnect them now.
    //
    // Check for a server password
    //
    // Everything looks good

	client->m_basicAuthCheck = 1;
}

void Server::UpdateClients()
{  
	// LEANDER : PC build disconnected clients garbage collection 
	int clientsSize = m_clients.Size();
    for( int i = 0; i < clientsSize; ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *s2c = m_clients[i];

            double timeNow = GetHighResTime();
            double timeSinceLastMessage = timeNow - s2c->m_lastMessageReceived;


#ifndef _DEBUG
            float maxTimeout = 20.0;

            #ifdef TESTBED
            maxTimeout = 60.0;
            #endif

            if( timeSinceLastMessage > maxTimeout )
            {
                // This person just isn't responding anymore
                // So disconnect them
                RemoveClient( s2c->m_clientId , Disconnect_ClientDrop );
                continue;
            }
#endif

            s2c->m_lastSentSequenceId = max( s2c->m_lastSentSequenceId, s2c->m_lastKnownSequenceId );
            
            int sendFrom = s2c->m_lastSentSequenceId + 1;            
            int sendTo = m_history.Size();
            if( sendTo - sendFrom > 3 ) sendTo = sendFrom + 3;
                       
            int fallenBehindThreshold = 10;

			// TODO Stage5 (Basic Lobby, the update rate changes from 1 Hz to 10Hz)
            // if( !g_app->m_gameRunning ) fallenBehindThreshold = 4;

            if( s2c->m_lastKnownSequenceId < m_history.Size() - fallenBehindThreshold )
            {
                // This client appears to have lost some packets and fallen behind, so rewind a bit
                s2c->m_caughtUp = false;                                
            }
            else if( s2c->m_lastKnownSequenceId > m_history.Size() - fallenBehindThreshold/2 - 1 )
            {
                s2c->m_caughtUp = true;
            }

            if( !s2c->m_caughtUp )
            {
                if( s2c->m_lastKnownSequenceId < s2c->m_lastSentSequenceId - fallenBehindThreshold )
                {
                    sendFrom = s2c->m_lastKnownSequenceId + 1;
                }

                sendTo = sendFrom + 3;  
				// TODO Stage5 (Basic Lobby, the update rate changes from 1 Hz to 10Hz)
                // if( !g_app->m_gameRunning ) sendTo = sendFrom + 50;
            }


            //
            // Special case - do run-length style encoding if there are lots of empty messages
            
            int numEmptyMessages = CountEmptyMessages( sendFrom );
            if( numEmptyMessages > 5 && timeSinceLastMessage < 5.0 )
            {
                ServerToClientLetter *letter = new ServerToClientLetter();
                letter->m_receiverId = s2c->m_clientId;
                letter->m_data = new Directory();
                letter->m_data->SetName( NET_DARWINIA_MESSAGE );
                letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_UPDATE );
                letter->m_data->CreateData( NET_DARWINIA_NUMEMPTYUPDATES, numEmptyMessages );
                letter->m_data->CreateData( NET_DARWINIA_SEQID, sendFrom );

                m_outboxMutex->Lock();
                m_outbox.PutDataAtEnd( letter );
                m_outboxMutex->Unlock();

                s2c->m_lastSentSequenceId = sendFrom + numEmptyMessages - 1;
            }
            else
            {
                for( int l = sendFrom; l < sendTo; ++l )
                {
                    if( m_history.ValidIndex(l) )
                    {
                        ServerToClientLetter *theLetter = m_history[l];
                        if( theLetter )
                        {
                            ServerToClientLetter *letterCopy = new ServerToClientLetter();
                            letterCopy->m_data = new Directory(*theLetter->m_data);
                            letterCopy->m_receiverId = s2c->m_clientId;

                            // To help combat packet loss, re-send messages that have definately been sent
                            // (ie < lastSentSequenceId) but havent yet been acknowledged (>lastKnownSequenceId)

                            int historyIndex = s2c->m_lastKnownSequenceId + (l-sendFrom) + 1;

                            if( !s2c->m_caughtUp )
                            {
                                // This client is having problems.  Assume he may have lost
                                // some messages, even though he told us he received them.
                                historyIndex -= 9;
                            }

                            if( historyIndex < l && m_history.ValidIndex(historyIndex) )
                            {
                                Directory *prevData = new Directory(*m_history[historyIndex]->m_data);
                                prevData->SetName( NET_DARWINIA_PREVUPDATE );
                                letterCopy->m_data->AddDirectory( prevData );
                            }

                            m_outboxMutex->Lock();
                            m_outbox.PutDataAtEnd( letterCopy );
                            m_outboxMutex->Unlock();

                            s2c->m_lastSentSequenceId = l;
                        }
                    }
                }
            }
		
			//
			// FTPManager
			FTPManager *ftpManager = s2c->m_ftpManager;
			if (ftpManager)
			{
				LList<Directory *> ftpMessages;
				
				ftpManager->MakeSendLetters( ftpMessages );

				int size = ftpMessages.Size();

				m_outboxMutex->Lock();
				for( int i = 0; i < size; i++ )
				{
					ServerToClientLetter *s = new ServerToClientLetter;
					s->m_data = ftpMessages.GetData(i);
					s->m_receiverId = s2c->m_clientId;

					m_outbox.PutDataAtEnd( s );
				}
				m_outboxMutex->Unlock();
			}

			//
			// Check to see if any clients have sent us IFrame from a sync error
			// If so, send that IFrame to the other clients
			
			if( ftpManager && s2c->m_syncErrorSeqId != -1 )
			{
				char filename[256];
				sprintf( filename, "IFrame %d %d", s2c->m_clientId, s2c->m_syncErrorSeqId );

				FTP *ftp = ftpManager->Retrieve( filename );

				if( ftp )
				{
					// Send the IFrame to the other clients
					for( int j = 0; j < clientsSize; j++ )
					{
						if ( j != i && m_clients.ValidIndex( j ) )
						{
							ServerToClient *other = m_clients[j];
							FTPManager *otherFTP = other->GetFTPManager();
							otherFTP->SendFile( filename, ftp->Data(), ftp->Size() );
						}
					}

					delete ftp;
				}
			}
        }
    }
}

void Server::HandleSyncMessage( Directory *incoming, int _clientId )
{
    int sequenceId      = incoming->GetDataInt( NET_DARWINIA_LASTPROCESSEDSEQID );
    unsigned char sync  = incoming->GetDataUChar( NET_DARWINIA_SYNCVALUE );

	if( !m_history.ValidIndex(sequenceId) )
	{
		AppDebugOut("Invalid SequenceID received! SeqID is %d, clientID is %d, sync char is %d\n", sequenceId, _clientId, sync);
        AppDebugAssert( m_history.ValidIndex(sequenceId) );
        return;
	}

    ServerToClient *sToc = GetClient( _clientId );

    //
    // Log the incoming sync value

    if( sToc )
    {
        sToc->m_sync.PutData( sync, sequenceId );
    }

    //
    // Provisional test - is the frame out of sync?

    int numResults = 0;
    bool provisionalResult = true;
    
    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *thisClient = m_clients[i];
            if( thisClient->m_sync.ValidIndex(sequenceId) )
            {
                ++numResults;
            
                unsigned char thisSync = thisClient->m_sync[sequenceId];
                if( thisSync != sync )
                {
                    provisionalResult = false;
                }
            }
        }
    }


    //
    // If something went wrong then figure out who is to blame
    // We must have all Sync bytes before we can do this
    // Look for the client with the sync byte that doesn't match anybody else

    bool allResponses = ( numResults == m_clients.NumUsed() );	
	
    if( allResponses && provisionalResult == false )
    {
        for( int i = 0; i < m_clients.Size(); ++i )
        {
            if( m_clients.ValidIndex(i) )
            {
                ServerToClient *clientA = m_clients[i];
                if( clientA->m_syncErrorSeqId == -1 )
                {
                    unsigned char clientASync = clientA->m_sync[sequenceId];
                    int numSame = 0;
                    for( int j = 0; j < m_clients.Size(); ++j )
                    {
                        if( i != j &&
                            m_clients.ValidIndex(j) )
                        {
                            ServerToClient *clientB = m_clients[j];
                            if( clientB->m_syncErrorSeqId == -1 ||
                                clientB->m_syncErrorSeqId > sequenceId )
                            {
                                unsigned char clientBSync = clientB->m_sync[sequenceId];
                                if( clientASync == clientBSync )
                                {
                                    ++numSame;
                                }
                            }
                        }
                    }

                    if( numSame == 0 )
                    {
                        clientA->m_syncErrorSeqId = sequenceId;
                        NotifyNetSyncError(i, sequenceId);
                    }
                }
            }
        }
    }

    //
    // If everything was Ok in that frame but some clients believe they are out of sync,
    // there is a good chance they are now in Sync.  So notify them of the good news.
    // Note: This happens when Out-Of-Sync clients disconnect and then rejoin.
    // They will receive the "out of sync" message, and then (hopefully) the same sync error will not occur.
    // They then receive the all-clear, and can continue.

    if( allResponses && provisionalResult == true )
    {
        for( int i = 0; i < m_clients.Size(); ++i )
        {
            if( m_clients.ValidIndex(i) )
            {
                ServerToClient *thisClient = m_clients[i];
                if( thisClient->m_syncErrorSeqId != -1 &&
                    thisClient->m_syncErrorSeqId <= sequenceId )
                {
					// #### Johnny
					// We don't want this for now. If we have a sync error, we want to 
					// stay with it.

                    // thisClient->m_syncErrorSeqId = -1;
                    // NotifyNetSyncFixed( i );
                }
            }
        }
    }
}

void Server::LoadHistory( const char *_filename )
{
	std::ifstream input( _filename, std::ios::in | std::ios::binary );

	m_history.EmptyAndDelete();

    int historySize;
	input.read( (char *) &historySize, sizeof(historySize) );

    for( int i = 0; i < historySize; ++i )
    {
        ServerToClientLetter *letter = new ServerToClientLetter;
		letter->m_data = new Directory;
		letter->m_data->Read( input );

		m_history.PutData( letter );

		int seqId = letter->m_data->GetDataInt( NET_DARWINIA_SEQID );

        if( seqId > m_sequenceId )
        {
            m_sequenceId = seqId + 1;
        }
    }

	input.close();
}

void Server::SaveHistory( const char *_filename )
{
	std::ofstream output( _filename, std::ios::binary | std::ios::out );

    int historySize = m_history.Size();
	output.write( (const char *) &historySize, sizeof(historySize) );

	for( int i = 0; i < historySize; ++i )
    {
		m_history[i]->m_data->Write( output );
    }

    output.close();
}


bool Server::IsDisconnectedClient( const char *_ip, int _port )
{
    for( int i = 0; i < m_disconnectedClients.Size(); ++i )
    {
        if( m_disconnectedClients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_disconnectedClients[i];
            if( strcmp ( sToC->m_ip, _ip ) == 0 &&
                sToC->m_port == _port )
            {
                return true;
            }
        }
    }

    return false;
}

bool Server::HandleDisconnectedClient( Directory *_message )
{
	char *fromIp    = _message->GetDataString(NET_DARWINIA_FROMIP);
    int fromPort    = _message->GetDataInt(NET_DARWINIA_FROMPORT);
	ServerToClient *sToC = GetDisconnectedClient( fromIp, fromPort );

	if( !sToC )
	{
		AppDebugOut("Could not find this disconnected client's ServerToClient\n");
		return false;
	}

    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->m_data = new Directory();
    letter->m_data->SetName( NET_DARWINIA_MESSAGE );
    letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_DISCONNECT );
    letter->m_data->CreateData( NET_DARWINIA_DISCONNECT, sToC->m_disconnected );
    letter->m_receiverId = sToC->m_clientId;
    letter->m_data->CreateData( NET_DARWINIA_SEQID, -1);
    letter->m_clientDisconnected = true;

    m_outbox.PutDataAtEnd( letter );

    sToC->m_lastMessageReceived = GetHighResTime();

	char fromStr[64];
	sprintf( fromStr, "%s:%d", sToC->m_ip, sToC->m_port );

    AppDebugOut( "Re-sent disconnect message to client %s\n", fromStr );

    return true;
}

ServerToClient *Server::GetDisconnectedClient( int _id )
{
    for( int i = 0; i < m_disconnectedClients.Size(); ++i )
    {
        if( m_disconnectedClients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_disconnectedClients[i];
            if( sToC->m_clientId == _id )
            {
                return sToC;
            }
        }
    }

    return NULL;
}

ServerToClient *Server::GetDisconnectedClient( const char *_ip, int _port )
{
    for( int i = 0; i < m_disconnectedClients.Size(); ++i )
    {
        if( m_disconnectedClients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_disconnectedClients[i];
            if( strcmp ( sToC->m_ip, _ip ) == 0 &&
                sToC->m_port == _port )
            {
                return sToC;
            }
        }
    }

    return NULL;
}

bool Server::RemoveDisconnectedClient( const char *_ip, int _port )
{
    for( int i = m_disconnectedClients.Size() - 1; i >= 0; --i )
    {
        if( m_disconnectedClients.ValidIndex(i) )
        {
            ServerToClient *sToC = m_disconnectedClients[i];
            if( strcmp ( sToC->m_ip, _ip ) == 0 &&
                sToC->m_port == _port )
            {
                m_disconnectedClients.RemoveData(i);
                delete sToC;
            }
        }
    }

    return NULL;
}

void Server::Advertise()
{
    if( strcmp( g_app->m_requestedMap, "null" ) == 0 || 
        strlen( g_app->m_requestedMap ) == 0 ) 
    {
        // no map, dont advertise
        return;
    }

	// WAN / LAN advertisement is a game property.
	bool advertiseOnWan = true, advertiseOnLan = true;

	int maxTeams = 4; // maxTeams is a game property.
    int currentTeams    = m_teams.NumUsed();

	//
    // Update our information

    if( advertiseOnWan || advertiseOnLan )
    {
		// Multiwinia is a game property

		const char *defaultServerName = g_prefsManager->GetString( "PlayerName", "NewServer" );//getenv("USERNAME");
		if( !defaultServerName )
			defaultServerName = "Multiwinia";

		const char *serverName = g_prefsManager->GetString( "ServerName", defaultServerName );

        Directory serverProperties;

        //
        // Basic variables

#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() == TESTBED_ON)
		{
			// We have to use char * for servernames here
			serverProperties.CreateData( NET_METASERVER_SERVERNAME,     g_app->GetTestBedServerName().m_charstring );
		}
		else
		{
			serverProperties.CreateData( NET_METASERVER_SERVERNAME,     serverName );	
		}
#else

        serverProperties.CreateData( NET_METASERVER_SERVERNAME,     serverName );
#endif
        serverProperties.CreateData( NET_METASERVER_GAMENAME,       APP_NAME );
        serverProperties.CreateData( NET_METASERVER_GAMEVERSION,    APP_VERSION );
		
		serverProperties.CreateData( NET_METASERVER_STARTTIME,		(unsigned long long) m_startTimeActual );
		serverProperties.CreateData( NET_METASERVER_RANDOM,			(int) m_random );
		
        char localIp[256];
        GetLocalHostIP( localIp, 256 );
        int localPort = GetLocalPort();

        serverProperties.CreateData( NET_METASERVER_LOCALIP,        localIp );
        serverProperties.CreateData( NET_METASERVER_LOCALPORT,      localPort );	

		//
        // Game properties

		int gameType = g_app->m_multiwinia->m_gameType;
		int maxTeams = 1;
        int mapCRC = 0;

        MapData *mapData = NULL;

		// Determine how many players can join this game
		if( 0 <= gameType && gameType < MAX_GAME_TYPES && g_app->m_requestedMap )
		{
			DArray<MapData *> &maps = g_app->m_gameMenu->m_maps[gameType];			

			for( int i = 0; i < maps.Size(); i++)
			{
				if( maps.ValidIndex( i ) )
				{
					MapData *m = maps[i];
					if( stricmp( g_app->m_requestedMap, m->m_fileName ) == 0 )
					{
                        mapData = m;
						maxTeams = m->m_numPlayers;
                        mapCRC = m->m_mapId;
						break;
					}
				}
			}
		}		

#ifdef TESTBED_ENABLED
		if(g_app->GetTestBedMode() == TESTBED_ON)
		{
			if(g_app->GetTestBedType() == TESTBED_SERVER)
			{
				currentTeams = g_app->m_iClientCount + 1;
			}

		}
#endif

        int currentHumanTeams = m_clients.NumUsed();

        serverProperties.CreateData( NET_METASERVER_NUMTEAMS,       (unsigned char) currentTeams );
        serverProperties.CreateData( NET_METASERVER_MAXTEAMS,       (unsigned char) maxTeams );
        serverProperties.CreateData( NET_METASERVER_NUMHUMANTEAMS,  (unsigned char) currentHumanTeams );
		serverProperties.CreateData( NET_METASERVER_NUMSPECTATORS,  (unsigned char) 0 );
		serverProperties.CreateData( NET_METASERVER_MAXSPECTATORS,	(unsigned char) 0 );
		serverProperties.CreateData( NET_METASERVER_GAMEINPROGRESS, (unsigned char) g_app->m_multiwinia->GameRunning() );
		serverProperties.CreateData( NET_METASERVER_GAMEMODE,		(unsigned char) gameType );

        if( m_serverPassword.Length() > 0 )
        {
            serverProperties.CreateData( NET_METASERVER_HASPASSWORD, (unsigned char ) 1 );
        }

        serverProperties.CreateData( NET_METASERVER_MAPCRC,         mapCRC );
#ifdef LOCATION_EDITOR
        if( mapData->m_customMap )
        {
            serverProperties.CreateData( NET_METASERVER_CUSTOMMAPNAME, mapData->m_levelName );
        }
#endif

	    char authKey[256];
	    Authentication_GetKey( authKey );
		serverProperties.CreateData( NET_METASERVER_AUTHKEY, authKey );


		if( advertiseOnWan )
		{
			// TODO: If port portforwarding use localPort, otherwise use the identity 
			// given by the matchmaker.

			char publicIP[256];
			int publicPort = -1;
			bool identityKnown = GetIdentity( publicIP, &publicPort );
			int usePortForwarding = g_prefsManager->GetInt( PREFS_NETWORKUSEPORTFORWARDING, 0 );

			if( usePortForwarding )
			{
				serverProperties.CreateData( NET_METASERVER_PORT, localPort );
			}
			else if( identityKnown )
			{
				serverProperties.CreateData( NET_METASERVER_PORT, publicPort );
			}
		}
		else {
			// LAN advertisements
			serverProperties.CreateData( NET_METASERVER_PORT, localPort );
		}

		//AppDebugOut("Advertising server properties\n" );
		//serverProperties.DebugPrint( 0 );

		MetaServer_SetServerProperties( &serverProperties );
	}

	// Advertising

	char authKey[256];
	Authentication_GetKey( authKey );
	int authResult = Authentication_SimpleKeyCheck( authKey, METASERVER_GAMETYPE_MULTIWINIA );

	bool singlePlayer = g_app->m_clientToServer->GetServerPort() == -1;
	
	bool permittedToAdvertise = 
		g_app->m_multiwinia->m_gameType != -1 &&
		authResult >= 0 &&
        !singlePlayer;

	bool shouldAdvertiseOnLan = permittedToAdvertise && advertiseOnLan;
	bool shouldAdvertiseOnWan = permittedToAdvertise && advertiseOnWan;

	// LAN Advertising
	if( shouldAdvertiseOnLan && !MetaServer_IsRegisteringOverLAN() )
	{
		MetaServer_StartRegisteringOverLAN();
	}
	else if( MetaServer_IsRegisteringOverLAN() && !shouldAdvertiseOnLan )
	{
		MetaServer_StopRegisteringOverLAN();
	}

	// WAN Advertising
	if( shouldAdvertiseOnWan && !MetaServer_IsRegisteringOverWAN() )
	{
        MatchMaker_StartRequestingIdentity( m_listener );
        MetaServer_StartRegisteringOverWAN();
	}
	else if( MetaServer_IsRegisteringOverWAN() && !shouldAdvertiseOnWan )
	{
        MetaServer_StopRegisteringOverWAN();
        MatchMaker_StopRequestingIdentity( m_listener );
	}
}

bool Server::GetIdentity( char *_ip, int *_port )
{
    return MatchMaker_GetIdentity( m_listener, _ip, _port );
}

int Server::GetLocalPort()
{
    if( !m_listener ) return -1;

    return m_listener->GetPort();
}

// ServerToClientLetter

void ServerToClientLetter::AddUpdate( const Directory *_update )
{
	Directory *copy = new Directory( *_update );
	copy->RemoveData( NET_DARWINIA_FROMIP );
	copy->RemoveData( NET_DARWINIA_FROMPORT );
	copy->RemoveData( NET_DARWINIA_LASTSEQID );

	m_data->AddDirectory( copy );
}

void Server::RegisterSpectator( int _clientId )
{
    ServerToClient *sToC = GetClient(_clientId);
    AppAssert(sToC);

    if( !sToC->m_spectator )
    {
        sToC->m_spectator = true;


        //
        // If he has any (non-AI) teams remove them now

        //if( !g_app->m_gameRunning )
        //{
            for( int t = 0; t < m_teams.Size(); ++t )
            {
                if( m_teams.ValidIndex(t) )
                {
                    ServerTeam *team = m_teams[t];
                    if( team->m_clientId == _clientId )
                    {
                        m_teams.RemoveData(t);
                        delete team;
                    }
                }
            }
        //}


        //
        // Send a letter to all Clients

        ServerToClientLetter *letter = new ServerToClientLetter();
        letter->m_data = new Directory();
        letter->m_data->SetName( NET_DARWINIA_MESSAGE );
        letter->m_data->CreateData( NET_DARWINIA_COMMAND, NET_DARWINIA_SPECTATORASSIGN );
        letter->m_data->CreateData( NET_DARWINIA_CLIENTID, _clientId );
		//letter->m_data->CreateData( NET_DARWINIA_RANDSEED, m_syncRandSeed );
        SendLetter( letter );
    }

}

int Server::GetNumSpectators()
{
    int count = 0;

    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *client = m_clients[i];
            if( client->m_spectator )
            {
                ++count;
            }
        }
    }
    
    return count;
}

void Server::KickClient(int _clientId)
{
    AppDebugOut("Server kicking client id %d\n", _clientId );
    RemoveClient( _clientId, Disconnect_KickedByServer );
}

UnicodeString &Server::GetPassword()
{
    return m_serverPassword;
}

void Server::SetPassword(UnicodeString &_password)
{
    m_serverPassword = _password;
}
