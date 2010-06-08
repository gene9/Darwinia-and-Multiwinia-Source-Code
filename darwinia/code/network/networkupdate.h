
/*
 * NETWORK UPDATE
 *
 * Represents a single event that occured and must be transmitted
 * Could be Client->Server or Server->Client
 *
 */


#ifndef _included_networkupdate_h
#define _included_networkupdate_h

#define NETWORKUPDATE_BYTESTREAMSIZE        42

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"

#include "team.h"

class NetworkUpdate
{
public:
    enum UpdateType
    {
        Invalid,                            // Unused
        ClientJoin,                         // New Client connects
        ClientLeave,                        // Client disconnects
        RequestTeam,                        // Client requests a new team, either virtual player or real
        Alive,                              // Updates Avatar position
        SelectUnit,                         // A player selected a unit
        CreateUnit,                         // A player issued instructions to a factory
        AimBuilding,                        // A player is aiming something like a radar dish
        ToggleLaserFence,                   // A laser fence is toggled
        RunProgram,                         // A player wishes to run a program
        TargetProgram,                      // A player targets a running program
        Pause,
        Syncronise                          // Performs a check to make sure we're in sync
    };

    UpdateType      m_type;
    char            m_clientIp[16];
    unsigned char   m_teamType;
	signed char		m_desiredTeamId;		// Used by the server host to force AI players to have a specific ID. -1 if we don't care
    int             m_unitId;
    int             m_entityId;
    int             m_buildingId;
    unsigned char   m_entityType;
    int             m_numTroops;
    unsigned char   m_teamId;
    float           m_radius;
    Vector3         m_direction;
	float			m_yaw, m_dive;
	float			m_power;
    unsigned char   m_program;
	TeamControls	m_teamControls;

    int             m_lastProcessedSeqId;       // Used for sync checks
    unsigned char   m_sync;

    int m_lastSequenceId;

    char m_byteStream[NETWORKUPDATE_BYTESTREAMSIZE];

public:
    NetworkUpdate();
    NetworkUpdate( char *_byteStream );

    void SetType            ( UpdateType _type );
    void SetClientIp        ( char *ip );
    void SetTeamType        ( unsigned char _teamType );
	void SetDesiredTeamId   ( signed char _desiredTeamId );
    void SetWorldPos        ( Vector3 const &_pos );
	void SetTeamControls	( TeamControls const &_teamControls );
    void SetTeamId          ( unsigned char _teamId );
    void SetEntityType      ( unsigned char _type );
    void SetNumTroops       ( int _numTroops );
    void SetRadius          ( float _radius );
    void SetDirection       ( Vector3 _dir );
    void SetUnitId          ( int _unitId );
    void SetEntityId        ( int _entityId );
    void SetBuildingID      ( int _buildingId );
	void SetYaw			    ( float _yaw );
	void SetDive		    ( float _dive );
	void SetPower		    ( float _power );
    void SetProgram         ( unsigned char _prog );
    void SetLastProcessedId ( int _lastProcessedId );
    void SetSync            ( unsigned char _sync );

    void SetLastSequenceId( int _lastSequenceId );

	const Vector3 &			GetWorldPos() const;
	Vector3 &				GetWorldPos();

    int ReadByteStream(char *_byteStream);                          // Returns number of bytes read
	char *GetByteStream(int *_linearSize);

//    void SendToDebugStream(FILE *_out, int _seqNum);

    NetworkUpdate const &operator = (NetworkUpdate const &n);
};

// Inlines
#include "networkupdate.inc"

#endif
