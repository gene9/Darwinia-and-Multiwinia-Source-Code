/* Represents one connection to one client. Server is expected to have a list of these. */


#ifndef _SERVERTOCLIENT_H
#define _SERVERTOCLIENT_H


class NetSocketSession;
class NetSocketListener;
class FTPManager;
class IQNetPlayer;
class Directory;

#include "lib/tosser/darray.h"

class ServerToClient
{
public:
	int					m_port;
    NetSocketSession	*m_socket;
    char				m_ip[16];
	int				    m_basicAuthCheck;
    char                m_authKey[256];
	int					m_authKeyId;

	int					m_clientId;
	FTPManager			*m_ftpManager;
	bool				m_spectator;

    int					m_lastKnownSequenceId;
    int					m_lastSentSequenceId;
    bool				m_caughtUp;
    double				m_lastMessageReceived;

    int					m_syncErrorSeqId;                   // SeqID where we got out of sync, or -1 if in sync
	int					m_disconnected;

    DArray<unsigned char>	m_sync;							// Syncronisation values for each sequenceId


	Directory			*m_lastAlive;						// Last alive message sent from this client
	Directory			*m_lastSelectUnit;					// Last select unit message sent from this client
	Directory			*m_lastTeamColour;					// Last request team colour message

    UnicodeString       m_password;                         // Password supplied by the client

public:

    ServerToClient( char *_ip, int _port, NetSocketListener *_listener );

	~ServerToClient();

    char				*GetIP ();
    NetSocketSession	*GetSocket ();

	FTPManager			*GetFTPManager ();
    
};



#endif



