#ifndef _CLIENTTOSERVER_H
#define _CLIENTTOSERVER_H

#include "lib/llist.h"
#include "lib/vector3.h"

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"


class NetLib;
class NetSocket;
class NetMutex;
class NetSocketListener;
class ServerToClientLetter;
class NetworkUpdate;


class ClientToServer
{
private:    
	NetLib				*m_netLib;

	void AdvanceSender	();

public:
    NetSocket           *m_sendSocket; 
    NetSocketListener   *m_receiveSocket;

    NetMutex            *m_inboxMutex;
    NetMutex            *m_outboxMutex;
    LList               <ServerToClientLetter *> m_inbox;
    LList               <NetworkUpdate *> m_outbox;

    int                 m_lastValidSequenceIdFromServer;    // eg if we have 11,12,13,15,18 then this is 13

public:
    ClientToServer();
    ~ClientToServer();

    int  GetOurIP_Int			();
    char *GetOurIP_String		();

    ServerToClientLetter    *GetNextLetter();
    int                      GetNextLetterSeqID();
    
	void Advance				();

    void ReceiveLetter          ( ServerToClientLetter *letter );
    void SendLetter             ( NetworkUpdate *letter );
    void ProcessServerUpdates   ( ServerToClientLetter *letter );

    void ClientJoin             ();
    void ClientLeave            ();
    void RequestTeam            (int _teamType, int _desiredId);

    void RequestSelectUnit      ( unsigned char _teamId, int _unitId, int _entityId, int _buildingId );
    void RequestCreateUnit      ( unsigned char _teamId, unsigned char _troopType, int _numToCreate, int _buildingId );
    void RequestCreateUnit      ( unsigned char _teamId, unsigned char _troopType, int _numToCreate, Vector3 const &_pos );
    void RequestAimBuilding     ( unsigned char _teamId, int _buildingId, Vector3 const &_pos );
    void RequestToggleFence     ( int _buildingId );
    void RequestRunProgram      ( unsigned char _teamId, unsigned char _program );
    void RequestTargetProgram   ( unsigned char _teamId, unsigned char _program, Vector3 const &_pos );

    void SendSyncronisation     ( int _lastProcessedId, unsigned char _sync );
    void SendIAmAlive           ( unsigned char _teamId, TeamControls const &_teamControls );

    void RequestPause();
};


#endif
