#include "lib/universal_include.h"

#include "net_lib.h"
#include "net_mutex.h"
#include "net_socket.h"
#include "net_socket_listener.h"
#include "net_thread.h"
#include "net_udp_packet.h"

#include "lib/debug_utils.h"
#include "lib/profiler.h"
#include "lib/preferences.h"

#include "app.h"
#include "globals.h"
#include "main.h"
#include "team.h"

#include "network/generic.h"
#include "network/server.h"
#include "network/servertoclient.h"
#include "network/servertoclientletter.h"
#include "network/clienttoserver.h"


// ****************************************************************************
// Class ServerTeam
// ****************************************************************************

ServerTeam::ServerTeam(int _clientId)
:    m_clientId(_clientId)
{
}


// ****************************************************************************
// Class Server
// ****************************************************************************

// ***ListenCallback
static NetCallBackRetType ListenCallback(NetUdpPacket *udpdata)
{
    if (udpdata)
    {
        NetIpAddress *fromAddr = &udpdata->m_clientAddress;
        char newip[16];
		IpToString(fromAddr->sin_addr, newip);

        if ( g_app->m_server )
        {
            NetworkUpdate *letter = new NetworkUpdate(udpdata->m_data);
            g_app->m_server->ReceiveLetter( letter, newip );
//            SET_PROFILE(g_app->m_profiler,  "#Server Receive", (double) udpdata->getLength() );
        }
        
        delete udpdata;
    }

    return 0;
}


Server::Server()
:   m_netLib(NULL),
    m_sequenceId(0),
    m_inboxMutex(NULL),
    m_outboxMutex(NULL)
{
    m_sync.SetSize( 0 );
}

Server::~Server()
{
    m_history.EmptyAndDelete();
    m_clients.EmptyAndDelete();
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
    NetSocketListener *m_listener = new NetSocketListener( 4000 );
    m_listener->StartListening( ListenCallback );
    return 0;
}


void Server::Initialise()
{
    m_inboxMutex = new NetMutex();
    m_outboxMutex = new NetMutex();
    
    if( !g_app->m_bypassNetworking )
    {
        m_netLib = new NetLib();
        m_netLib->Initialise();

        NetStartThread( ListenThread );
    }
}


int Server::GetClientId ( char *_ip )
{
    for ( int i = 0; i < m_clients.Size(); ++i ) 
    {
        if ( m_clients.ValidIndex(i) )
        {
            char *thisIP = m_clients[i]->GetIP();
            if ( strcmp ( thisIP, _ip ) == 0 )
            {
                return i;
            }
        }
    }

    return -1;
}


int Server::ConvertIPToInt( const char *_ip )
{
	DarwiniaReleaseAssert(strlen(_ip) < 17, "IP address too long");
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


// ***RegisterNewClient
void Server::RegisterNewClient ( char *_ip )
{
    DarwiniaDebugAssert(GetClientId(_ip) == -1);
    ServerToClient *sToC = new ServerToClient(_ip);
    m_clients.PutData(sToC);

    
    //
    // Tell all clients about it

    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->SetType( ServerToClientLetter::HelloClient );
    letter->SetIp( ConvertIPToInt( _ip ) );
    SendLetter( letter );
}


void Server::RemoveClient( char *_ip )
{
    int clientId = GetClientId(_ip);
    ServerToClient *sToC = m_clients[clientId];
    m_clients.MarkNotUsed( clientId );
    delete sToC;


    //
    // Tell all clients about it

    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->SetType( ServerToClientLetter::GoodbyeClient );
    letter->SetIp( ConvertIPToInt( _ip ) );
    SendLetter( letter );

}


// *** RegisterNewTeam
void Server::RegisterNewTeam ( char *_ip, int _teamType, int _desiredTeamId )
{               
    int clientId = GetClientId(_ip);
    DarwiniaDebugAssert( clientId != -1 );                           // Client not properly connected

    if( _desiredTeamId != -1 )                              // Specified Team ID - An AI
    {                                                       
        if( !m_teams.ValidIndex(_desiredTeamId) )
        {
            ServerTeam *team = new ServerTeam(clientId);
			if (m_teams.Size() <= _desiredTeamId)
			{
				m_teams.SetSize(_desiredTeamId+1);
			}
			m_teams.PutData(team, _desiredTeamId);
        }
    }
    else
    {
        DarwiniaDebugAssert( m_teams.NumUsed() < NUM_TEAMS );
        ServerTeam *team = new ServerTeam(clientId);
        int teamId = m_teams.PutData( team );

        ServerToClientLetter *letter = new ServerToClientLetter();
        letter->SetType( ServerToClientLetter::TeamAssign );
        letter->SetTeamId(teamId);
        letter->SetIp( ConvertIPToInt( _ip ) );
        letter->SetTeamType( _teamType );
        SendLetter( letter );
    }

/*
    DarwiniaDebugAssert(GetClientId(_ip) != -1);

    if (m_teams.NumUsed() < NUM_TEAMS)
    {
        int clientId = GetClientId(_ip);
        ServerTeam *team = new ServerTeam(clientId);
        int teamId = -1;

        if (_desiredTeamId == -1)
		{
			teamId = m_teams.PutData( team );
		}
		else
		{
			DarwiniaDebugAssert(!m_teams.ValidIndex(_desiredTeamId));
			teamId = _desiredTeamId;
			if (m_teams.Size() <= _desiredTeamId)
			{
				m_teams.SetSize(_desiredTeamId+1);
			}
			m_teams.PutData(team, _desiredTeamId);
		}

        //
        // Send a TeamAssign letter to all Clients

        if( _teamType != Team::TeamTypeAI )
        {
            ServerToClientLetter *letter = new ServerToClientLetter();
            letter->SetType( ServerToClientLetter::TeamAssign );
            letter->SetTeamId(teamId);
            letter->SetIp( ConvertIPToInt( _ip ) );
            letter->SetTeamType( _teamType );
            letter->SetAIType( _aiType );
            
            SendLetter( letter );
        }                
    }*/

}

NetworkUpdate *Server::GetNextLetter()
{
    m_inboxMutex->Lock();
    NetworkUpdate *letter = NULL;

    if( m_inbox.Size() > 0 )
    {
        letter = m_inbox[0];
        m_inbox.RemoveData(0);
    }

    m_inboxMutex->Unlock();
    return letter;
}

void Server::ReceiveLetter( NetworkUpdate *update, char *fromIP )
{
    update->SetClientIp( fromIP );

    m_inboxMutex->Lock();
    m_inbox.PutDataAtEnd(update);
    m_inboxMutex->Unlock();
}

void Server::SendLetter( ServerToClientLetter *letter )
{

    //
    // Assign a sequence id

    letter->SetSequenceId(m_sequenceId);
    m_sequenceId++;

    m_history.PutDataAtEnd( letter );
}


void Server::AdvanceSender()
{
    int bytesSentThisFrame = 0;
    m_outboxMutex->Lock();

    while (m_outbox.Size())
    {
        ServerToClientLetter *letter = m_outbox[0];
        DarwiniaDebugAssert(letter);

        if (m_clients.ValidIndex(letter->GetClientId()))
        {                               
            if( g_app->m_bypassNetworking )
            {
                g_app->m_clientToServer->ReceiveLetter( letter );
            }
            else
            {                
    			int linearSize = 0;
                ServerToClient *client = m_clients[letter->GetClientId()];
                NetSocket *socket = client->GetSocket();
                char *linearisedLetter = letter->GetByteStream(&linearSize);                
				socket->WriteData( linearisedLetter, linearSize );
                bytesSentThisFrame += linearSize;
                delete letter;
            }
        }

		// The letter has now been sent so we can take it off the outbox list
        m_outbox.RemoveData(0);
    }

    m_outboxMutex->Unlock();

    if( bytesSentThisFrame > 0 )
    {
//        SET_PROFILE(g_app->m_profiler,  "#Server Send", (double) bytesSentThisFrame );
    }
}


void Server::Advance()
{
    START_PROFILE(g_app->m_profiler, "Advance Server");


    //
    // Compile all incoming messages into a ServerToClientLetter

    ServerToClientLetter *letter = new ServerToClientLetter();
    letter->SetType( ServerToClientLetter::Update );
    
    NetworkUpdate *incoming = GetNextLetter();

    while( incoming )
    {
        if( incoming->m_type == NetworkUpdate::ClientJoin )
        {
            if( GetClientId( incoming->m_clientIp ) == -1 )
            {
                DebugOut ( "SERVER: New Client connected from %s\n", incoming->m_clientIp );
                RegisterNewClient( incoming->m_clientIp );
            }
        }
        else if ( incoming->m_type == NetworkUpdate::ClientLeave )
        {
            if( GetClientId( incoming->m_clientIp ) != -1 )
            {
                DebugOut ( "SERVER: Client at %s disconnected gracefully\n", incoming->m_clientIp );
                RemoveClient( incoming->m_clientIp );                
            }
        }
        else if ( incoming->m_type == NetworkUpdate::RequestTeam )
        {
            if( GetClientId( incoming->m_clientIp ) != -1 )
            {
                DebugOut ( "SERVER: New team request from %s\n", incoming->m_clientIp );
                RegisterNewTeam(incoming->m_clientIp, incoming->m_teamType, incoming->m_desiredTeamId);
            }
        }
        else if( incoming->m_type == NetworkUpdate::Syncronise )
        {
            int sequenceId = incoming->m_lastProcessedSeqId;
            unsigned char sync = incoming->m_sync;
            
            if( sequenceId != 0 && !m_sync.ValidIndex(sequenceId-1) )
            {
                // This incoming packet has a sequence ID that is too high
                // Most likely it was sent from a previous client connected to a previous server
                // Then that server shut down, and this one started up 
                // Then this server received the packet intended for the old server
                // So we simply discard it   
                //DebugOut( "Sync %d discarded as bogus\n", sequenceId );
            }
            else
            {
                if( m_sync.Size() <= sequenceId )
                {                
                    m_sync.SetSize( m_sync.Size() + 1000 );
                }

                if( m_sync.ValidIndex(sequenceId) )
                {
                    unsigned char lastKnownSync = m_sync[sequenceId];
                    DarwiniaDebugAssert( lastKnownSync == sync );   
                    //DebugOut( "Sync %02d verified as %03d\n", sequenceId, sync );
                }
                else
                {
                    m_sync.PutData( sync, sequenceId );
                    //DebugOut( "Sync %02d set to %03d\n", sequenceId, sync );
                }
            }
        }
        else if( incoming->m_teamId != 255 )
        {            
            letter->AddUpdate( incoming );
        }
        
        int clientId = GetClientId( incoming->m_clientIp );
        if( clientId != -1 )
        {
            ServerToClient *sToc = m_clients[ clientId ];
            if( incoming->m_lastSequenceId > sToc->m_lastKnownSequenceId )
            {
                sToc->m_lastKnownSequenceId = incoming->m_lastSequenceId;
            }
        }

        delete incoming;
        incoming = GetNextLetter();
    }

    SendLetter( letter );


    //
    // Update all clients depending on their state

    int maxUpdates = 25;                                            // Sensible to cap re-transmissions like this
    if( g_prefsManager->GetInt( "RecordDemo" ) == 2 )
    {
        maxUpdates = 1;
    }

    for( int i = 0; i < m_clients.Size(); ++i )
    {
        if( m_clients.ValidIndex(i) )
        {
            ServerToClient *s2c = m_clients[i];
            int sendFrom = s2c->m_lastKnownSequenceId + 1;
            int sendTo   = m_history.Size();
            if( sendTo - sendFrom > maxUpdates ) sendTo = sendFrom + maxUpdates;                
            
            for( int l = sendFrom; l < sendTo; ++l )
            {
                if( m_history.ValidIndex(l) )
                {
                    ServerToClientLetter *theLetter = m_history[l];
                    ServerToClientLetter *letterCopy = new ServerToClientLetter( *theLetter );
                    letterCopy->SetClientId(i);
            
                    m_outboxMutex->Lock();
                    m_outbox.PutDataAtEnd( letterCopy );
                    m_outboxMutex->Unlock();
                }
            }
        }
    }

	AdvanceSender();

    END_PROFILE(g_app->m_profiler, "Advance Server");
}

void Server::LoadHistory( char *_filename )
{
    FILE *file = fopen( _filename, "rb" );

    int historySize;
    fread( &historySize, sizeof(historySize), 1, file );

    for( int i = 0; i < historySize; ++i )
    {
        int linearSize;
        fread( &linearSize, sizeof(linearSize), 1, file );

        char byteStream[SERVERTOCLIENTLETTER_BYTESTREAMSIZE];
        fread( byteStream, linearSize, 1, file );

        ServerToClientLetter *letter = new ServerToClientLetter( byteStream, linearSize );
        m_history.PutData( letter );

        if( letter->GetSequenceId() > m_sequenceId )
        {
            m_sequenceId = letter->GetSequenceId() + 1;
        }
    }

    fclose( file );
}

void Server::SaveHistory( char *_filename )
{
    FILE *file = fopen( _filename, "wb" );

    int historySize = m_history.Size();
    fwrite( &historySize, sizeof(historySize), 1, file );

    for( int i = 0; i < historySize; ++i )
    {
        ServerToClientLetter *letter = m_history[i];
        int linearSize;
        char *byteStream = letter->GetByteStream( &linearSize );
        
        fwrite( &linearSize, sizeof(linearSize), 1, file );
        fwrite( byteStream, linearSize, 1, file );
    }

    fclose( file );
}
