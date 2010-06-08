#include "lib/universal_include.h"

#include <string.h>

#include "net_lib.h"
#include "net_socket.h"
#include "net_socket_session.h"

#include "app.h"
#include "lib/debug_utils.h"
#include "network/servertoclient.h"
#include "network/ftp_manager.h"
#include "lib/tosser/directory.h"


ServerToClient::ServerToClient( 
	char *_ip, int _port, NetSocketListener *_listener 
	)
:   
	m_socket(NULL),
	m_port(_port),
	m_basicAuthCheck(0),
	m_authKeyId(0),
	m_clientId(-1),
	m_disconnected(-1),
	m_ftpManager(NULL),
    m_lastKnownSequenceId(-1),
    m_lastMessageReceived(GetHighResTime()),
    m_lastSentSequenceId(-1),
    m_caughtUp(false),
    m_syncErrorSeqId(-1),
	m_lastAlive( NULL ),
	m_lastSelectUnit( NULL ),
	m_spectator(false),
	m_lastTeamColour( NULL )
{   
	strcpy(	m_authKey, "" );
    strcpy ( m_ip, _ip );
    m_socket = new NetSocketSession( *_listener, _ip, _port  );
}


ServerToClient::~ServerToClient()
{
	delete m_ftpManager;
	delete m_lastAlive;
	delete m_lastSelectUnit;
	delete m_lastTeamColour;
	delete m_socket;
	m_socket = NULL;

	m_ftpManager = NULL;
	m_lastAlive = NULL;
	m_lastSelectUnit = NULL;
	m_lastTeamColour = NULL;
}

char *ServerToClient::GetIP()
{
    return m_ip;
}


NetSocketSession *ServerToClient::GetSocket()
{
    return m_socket;
}

FTPManager *ServerToClient::GetFTPManager ()
{
	if (!m_ftpManager) 
	{
		char name[64];
		sprintf(name, "SERVER:%d", m_clientId);
		m_ftpManager = new FTPManager( name );
	}

	return m_ftpManager;
}
