   
#ifndef _CLIENTTOSERVER_H
#define _CLIENTTOSERVER_H

#include "lib/tosser/directory.h"
#include "lib/tosser/llist.h"
#include "lib/vector3.h"

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"

#include "network/bandwidth.h"
#include "network/network_defines.h"

class NetLib;
class NetMutex;
class NetSocketListener;
class NetSocketSession;
class ServerToClientLetter;
class NetworkUpdate;
class IFrame;
class FTPManager;
class IQNetPlayer;
class WorkQueue;
class DemoLimitations;

#define CTOS_MAX_CONNECTION_ATTEMPTS 50

class ClientToServer
{
private:    
    char                *m_serverIp;
    int                 m_serverPort;

	FTPManager			*m_ftpManager;
	unsigned char		m_sync;	// Last generated sync value

	void				AdvanceSender();
	bool				IsLocalLetter( Directory *letter );
	int					GetNumSlicesToAdvance( bool catchUp );
	void				GenerateSyncValue();
	

public:

	NetSocketSession    *m_sendSocket; 
    NetSocketListener   *m_listener;
	WorkQueue			*m_listenThread;
	
    NetMutex            *m_inboxMutex;
    NetMutex            *m_outboxMutex;
    LList               <Directory *> m_inbox;
    LList               <Directory *> m_outbox;

    int                 m_lastValidSequenceIdFromServer;    // eg if we have 11,12,13,15,18 then this is 13
	int                 m_serverSequenceId;

	BandwidthCounter	m_receiveRate, m_sendRate;
    double              m_lastServerLetterReceivedTime;
    
    double              m_pingSentTime;
    float               m_latency;

	time_t				m_serverStartTime;
	int					m_serverRandomNumber;

    int					m_clientId;
	char				m_serverVersion[128];

    int					m_connectionState;
    double				m_retryTimer;
	double				m_retryTimerFast;
	double				m_retryTimerSlow;
    int					m_connectionAttempts;

	int					m_syncErrorSeqId;
    LList               <int> m_outOfSyncClients;
	LList				<IFrame *> m_iframeHistory;

	int					m_gameOptionVersion[GAMEOPTION_NUMGAMEOPTIONS];
	int					*m_gameOptionArrayVersion[GAMEOPTION_NUMGAMEOPTIONS];

	bool				m_bSpectator;

    UnicodeString       m_password;

    bool                m_christmasMode;

	//TODO: Check this with the IV editor beta.
	bool		m_requestCustomMap;

	struct 
	{
		int				m_colourId;
		UnicodeString	m_name;
	} m_requestedTeamDetails;

    enum
    {
        StateDisconnected,
        StateConnecting,
        StateHandshaking,
        StateConnected
    };
    
public:
    ClientToServer();
    ~ClientToServer();

    int  GetOurIP_Int			();
    char *GetOurIP_String		();

    Directory				*GetNextLetter();
    int                      GetNextLetterSeqID();
    
	void AdvanceAndCatchUp		();
	void Advance				();

    void OpenConnections        ();                                 // Close and re-open listeners

    void ReceiveLetter          ( Directory *letter, int _bandwidthUsed = 0 );
    void SendLetter             ( Directory *letter );

	void AddIFrame				( IFrame *iFrame );
	void SendIFrame				( int _seqId );

	void ProcessAllServerLetters( bool catchUp );
	bool ProcessServerLetters	( Directory *letter );
    bool ProcessServerUpdates   ( Directory *letter );
	void ProcessFTPLetter		( Directory *_letter );

	void DebugPrintInbox();
	void GetSyncFileName		( const char *_prefix, char *_syncFilename, int _bufSize );

    void ClientJoin             ( const char *_serverIp, int _serverPort );
    void ClientLeave            ();
    void RequestTeam            (int _teamType, int _desiredId);
	void RequestToRegisterAsSpectator(int _teamID);
	void SendTeamScores			( int _winningTeamID );

    void RequestSelectUnit      ( unsigned char _teamId, int _unitId, int _entityId, int _buildingId );
    void RequestAimBuilding     ( unsigned char _teamId, int _buildingId, Vector3 const &_pos );
    void RequestToggleFence     ( int _buildingId );
    
    void RequestRunProgram          ( unsigned char _teamId, unsigned char _program );
    void RequestTargetProgram       ( unsigned char _teamId, unsigned char _program, Vector3 const &_pos );
    void RequestSelectProgram       ( unsigned char _teamId, unsigned char _programId );
    void RequestTerminateProgram    ( unsigned char _teamId, unsigned char _programId );
    void RequestTerminateEntity     ( unsigned char _teamId, int _entityId );
    void RequestNextWeapon          ( unsigned char _teamId );
    void RequestPreviousWeapon      ( unsigned char _teamId );
    void RequestSelectDarwinians    ( unsigned char _teamId, Vector3 const &_pos, float _radius );
    void RequestIssueDarwinanOrders ( unsigned char _teamId, Vector3 const &_pos, bool directRoute );
    void RequestOfficerOrders       ( unsigned char _teamId, Vector3 const &_pos, bool directRoute, bool forceOnlyMove );
    void RequestArmourOrders        ( unsigned char _teamId, Vector3 const &_pos, bool deployAtWaypoint );
    void RequestPromoteDarwinian    ( unsigned char _teamId, int _entityId );
    void RequestToggleEntity        ( unsigned char _teamId, int _entityId, bool secondary=false );
	void RequestOfficerFollow		( unsigned char _teamId );
	void RequestOfficerNone			( unsigned char _teamId );

	void RequestShamanSummon	    ( unsigned char _teamId, int _shamanId, unsigned char _summonType );

    void SendSyncronisation     ();
    void SetSyncState           ( int _clientId, bool _synchronised );
    bool IsSynchronised         ( int _clientId );
	bool IsSequenceIdInQueue	( int _seqId );

    int     GetEstimatedServerSeqId();
    float   GetEstimatedLatency();

    void SendIAmAlive           ( unsigned char _teamId, TeamControls const &_teamControls );
    void SendPing               ( unsigned char _teamId );

	void RequestSetGameOptionInt( int _gameOption, int _value );
	void RequestSetGameOptionString( int _gameOption, const char *_value );
	void RequestSetGameOptionArrayElementInt( int _gameOption, int _index, int _value );
	void RequestSetGameOptionsArrayInt( int _gameOption, int *_values, int _numValues );

    void RequestToggleReady     ( unsigned char _teamId );
	void RequestSetTeamReady  ( unsigned char _teamId );
	void RequestStartGame		(); 
    void RequestPause			();

    void RequestTeamName				( const UnicodeString &_teamName, unsigned char _teamId );
    void RequestTeamColour				( int _colourId, unsigned char _teamId );
	void RequestSendViralPlayerAchievements	( unsigned char _teamId );

    void SendChatMessage        ( const UnicodeString &msg, unsigned char _teamId );

	void ReliableSetTeamName(  const UnicodeString &_name );
	void ReliableSetTeamColour( int _colourId );

	int GameOptionVersion		( int _gameOption, int _index = -1 );
	void SetGameOptionVersion	( int _gameOption, int _index, int _version );
	void ResetGameOptionVersions();

    void RequestChristmasMode();

	void KeepAlive				();

	void GetDemoLimitations		( DemoLimitations &l );

	int  GetLocalPort			();                                   // Returns local port client is listening on
    void StartIdentifying		();
    void StopIdentifying		();
	bool GetIdentity			( char *_ip, int *_port );			  // Get the public IP / Port for this client
    void GetServerIp            ( char *_ip );
    int  GetServerPort          ();
};


#endif
