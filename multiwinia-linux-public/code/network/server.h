#ifndef SERVER_H
#define SERVER_H

#include "lib/tosser/llist.h"
#include "lib/tosser/darray.h"
#include "lib/unicode/unicode_string.h"
#include "network/bandwidth.h"

class NetMutex;
class NetSocketListener;
class ServerToClient;
class ServerToClientLetter;
class Directory;
class IQNetPlayer;

#define UDP_HEADER_SIZE     32           // 12 bytes for UDP header, 20 bytes for IP header

enum            // Disconnection types
{
    Disconnect_ClientLeave,
    Disconnect_ClientDrop,
    Disconnect_ClientNeedsPDLC,
	Disconnect_ServerFull,
	Disconnect_InvalidKey,
	Disconnect_DuplicateKey,
	Disconnect_BadPassword,
	Disconnect_DemoFull,
	Disconnect_KeyAuthFailed,
    Disconnect_KickedByServer
};

class ServerTeam
{
public:
    int m_clientId;
	int m_teamType;

    ServerTeam(int _clientId, int _teamType);
};


class Server
{
private:  

	time_t			m_startTimeActual;
    double			m_startTime;
	double			m_nextServerAdvanceTime;
	unsigned		m_random;
	int				m_syncRandSeed;

	bool			m_gameStartMsgReceived;

    LList           <ServerToClientLetter *> m_history;

public:

#ifdef TESTBED_ENABLED
	bool				m_bRandSeedOverride;
	int					m_iRandSeed;
#endif

    int             m_sequenceId;
    int             m_nextClientId;

    DArray          <ServerToClient *> m_clients;
	DArray			<ServerToClient *> m_disconnectedClients;
    DArray          <ServerTeam *> m_teams;

    NetMutex         *m_inboxMutex;
    NetMutex         *m_outboxMutex;
    LList           <Directory *> m_inbox;
    LList           <ServerToClientLetter *> m_outbox;

	// To hwlp with client rejection
	int				m_iGameType;
	int				m_iMapID;
	int				m_iClientCount;

	bool			m_noAdvertise;

    BandwidthCounter m_sendRate, m_receiveRate;

    UnicodeString   m_serverPassword;

	NetSocketListener *m_listener;

private:
	int CountEmptyMessages	( int _startingSeqId );
    void AuthenticateClients();
    void AuthenticateClient ( int _clientId );

public:
    Server();
    ~Server();

    void Initialise			();
    
    Directory *GetNextLetter();

    void ReceiveLetter      ( Directory *update, const char *fromIP, int _fromPort, int _bandwidthUsed = 0 );

    void SendLetter         ( ServerToClientLetter *letter );

	ServerToClient          *GetClient( int _id );
	int  GetClientId        ( const char *_ip, int _port );
	bool IsDisconnectedClient ( const char *_ip, int _port );
	ServerToClient* GetDisconnectedClient( const char *_ip, int _port );
	bool RemoveDisconnectedClient ( const char *_ip, int _port );
	ServerToClient* GetDisconnectedClient( int _id );
    void SendClientId       ( int _clientId );

    int  RegisterNewClient  ( Directory *_client, int _clientId = -1 );
    void RemoveClient       ( int _clientId, int _reason );
    void RegisterNewTeam    ( int _clientId, int _teamType, int _desiredTeamId );
	void RegisterSpectator	( int _clientId );
	int GetNumSpectators	();

    void HandleSyncMessage	( Directory *_message, int _clientId );
	bool HandleDisconnectedClient( Directory *_message );

    void NotifyNetSyncError ( int _clientId, int _sequenceId );
    void NotifyNetSyncFixed ( int _clientId );

	void SkipToTime( double _timeNow );
	void AdvanceIfNecessary( double _timeNow );

	bool CheckPlayerCanJoin ( Directory* _incoming );

	void AdvanceSender		();
    void Advance			();
	void UpdateClients		();

    void LoadHistory        ( const char *_filename );
    void SaveHistory        ( const char *_filename );

    static int   ConvertIPToInt( const char *_ip );
    static char *ConvertIntToIP( const int _ip );

    void Advertise          ();

	inline unsigned int GetRandom() { return m_random; }

    void KickClient( int _clientId );
    UnicodeString &GetPassword();
    void SetPassword( UnicodeString &_password );

    bool GetIdentity        ( char *_ip, int *_port );

	int GetLocalPort();

public:
	void StartListening();

};


#endif



