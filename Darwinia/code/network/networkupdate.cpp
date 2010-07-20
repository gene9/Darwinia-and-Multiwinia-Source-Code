#include "lib/universal_include.h"

#include <string.h>

#include "lib/debug_utils.h"

#include "network/bytestream.h"
#include "network/networkupdate.h"


NetworkUpdate::NetworkUpdate()
:   m_type(Invalid),
    m_lastSequenceId(-1),
    m_radius(0.0f),
    m_teamId(255),
    m_entityType(Entity::TypeInvalid),
    m_numTroops(0),
    m_unitId(0),
    m_buildingId(-1),
    m_sync(0)
{
}

NetworkUpdate::NetworkUpdate( char *_byteStream)
{
    ReadByteStream( _byteStream );
}

int NetworkUpdate::ReadByteStream(char *_byteStream)
{
    char *byteStreamCopy = _byteStream;

	m_type = (UpdateType) READ_INT(_byteStream);
    m_lastSequenceId = READ_INT(_byteStream);

    switch(m_type)
    {
        case ClientJoin:
        case ClientLeave:
            break;

        case RequestTeam:
            m_teamType = READ_UNSIGNED_CHAR(_byteStream);
            m_entityType = READ_UNSIGNED_CHAR(_byteStream);
			m_desiredTeamId = READ_SIGNED_CHAR(_byteStream);
            break;

        case Alive:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            GetWorldPos().x = READ_FLOAT(_byteStream);
            GetWorldPos().y = READ_FLOAT(_byteStream);
            GetWorldPos().z = READ_FLOAT(_byteStream);
			{
				unsigned char flags = READ_UNSIGNED_SHORT(_byteStream);
				m_teamControls.SetFlags( flags );
			}
            m_sync = READ_UNSIGNED_CHAR(_byteStream);
            break;

        case Syncronise:
            m_lastProcessedSeqId = READ_INT(_byteStream);
            m_sync = READ_UNSIGNED_CHAR(_byteStream);
            break;

        case SelectUnit:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            m_unitId = READ_INT(_byteStream);
            m_entityId = READ_INT(_byteStream);
            m_buildingId = READ_INT(_byteStream);
            break;

        case CreateUnit:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            m_entityType = READ_UNSIGNED_CHAR(_byteStream);
            m_numTroops = READ_INT(_byteStream);
            m_buildingId = READ_INT(_byteStream);
            GetWorldPos().x = READ_FLOAT(_byteStream);
            GetWorldPos().y = READ_FLOAT(_byteStream);
            GetWorldPos().z = READ_FLOAT(_byteStream);
            break;

        case AimBuilding:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            m_buildingId = READ_INT(_byteStream);
            GetWorldPos().x = READ_FLOAT(_byteStream);
            GetWorldPos().y = READ_FLOAT(_byteStream);
            GetWorldPos().z = READ_FLOAT(_byteStream);
            break;

        case ToggleLaserFence:
            m_buildingId = READ_INT(_byteStream);
            break;

        case RunProgram:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            m_program = READ_UNSIGNED_CHAR(_byteStream);
            break;

        case TargetProgram:
            m_teamId = READ_UNSIGNED_CHAR(_byteStream);
            m_program = READ_UNSIGNED_CHAR(_byteStream);
            GetWorldPos().x = READ_FLOAT(_byteStream);
            GetWorldPos().y = READ_FLOAT(_byteStream);
            GetWorldPos().z = READ_FLOAT(_byteStream);
            break;

        case Invalid:
            DarwiniaDebugAssert(false);

    };

    return (_byteStream - byteStreamCopy);
}

void NetworkUpdate::SetType( UpdateType _type )
{
    m_type = _type;
}

void NetworkUpdate::SetClientIp( char *ip )
{
    strcpy( m_clientIp, ip );
}

void NetworkUpdate::SetTeamType( unsigned char _teamType )
{
    m_teamType = _teamType;
}

void NetworkUpdate::SetDesiredTeamId( signed char _desiredTeamId )
{
	m_desiredTeamId = _desiredTeamId;
}

void NetworkUpdate::SetWorldPos ( Vector3 const &_pos )
{
	// Shared with m_teamControls
	GetWorldPos() = _pos;
}

void NetworkUpdate::SetTeamControls ( TeamControls const &_teamControls )
{
	m_teamControls = _teamControls;
}

void NetworkUpdate::SetTeamId ( unsigned char _teamId )
{
    m_teamId = _teamId;
}

void NetworkUpdate::SetEntityType ( unsigned char _type )
{
    m_entityType = _type;
}

void NetworkUpdate::SetNumTroops ( int _numTroops )
{
    m_numTroops = _numTroops;
}

void NetworkUpdate::SetRadius ( float _radius )
{
    m_radius = _radius;
}

void NetworkUpdate::SetDirection( Vector3 _dir )
{
    m_direction = _dir;
}

void NetworkUpdate::SetLastSequenceId( int _lastSequenceId )
{
    m_lastSequenceId = _lastSequenceId;
}

void NetworkUpdate::SetUnitId( int _unitId )
{
    m_unitId = _unitId;
}

void NetworkUpdate::SetEntityId ( int _entityId )
{
    m_entityId = _entityId;
}

void NetworkUpdate::SetBuildingID ( int _buildingId )
{
    m_buildingId = _buildingId;
}

void NetworkUpdate::SetYaw(float _yaw)
{
	m_yaw = _yaw;
}

void NetworkUpdate::SetDive(float _dive)
{
	m_dive = _dive;
}

void NetworkUpdate::SetPower(float _power)
{
	m_power = _power;
}

void NetworkUpdate::SetProgram( unsigned char _prog )
{
    m_program = _prog;
}

void NetworkUpdate::SetLastProcessedId( int _lastProcessedId )
{
    m_lastProcessedSeqId = _lastProcessedId;
}

void NetworkUpdate::SetSync( unsigned char _sync )
{
    m_sync = _sync;
}

char *NetworkUpdate::GetByteStream(int *_linearSize)
{
    char *byteStream = m_byteStream;

    WRITE_INT(byteStream, m_type);
    WRITE_INT(byteStream, m_lastSequenceId);

    switch( m_type )
    {
        case ClientJoin:
        case ClientLeave:
            break;

        case RequestTeam:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamType);
            WRITE_UNSIGNED_CHAR(byteStream, m_entityType);
			WRITE_SIGNED_CHAR(byteStream, m_desiredTeamId);
            break;

        case Alive:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId);
            WRITE_FLOAT( byteStream, GetWorldPos().x );
            WRITE_FLOAT( byteStream, GetWorldPos().y );
            WRITE_FLOAT( byteStream, GetWorldPos().z );
			WRITE_UNSIGNED_SHORT( byteStream, m_teamControls.GetFlags() );
            WRITE_UNSIGNED_CHAR( byteStream, m_sync );
            break;

        case Syncronise:
            WRITE_INT(byteStream, m_lastProcessedSeqId );
            WRITE_UNSIGNED_CHAR(byteStream, m_sync );
            break;

        case SelectUnit:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId );
            WRITE_INT(byteStream, m_unitId );
            WRITE_INT(byteStream, m_entityId );
            WRITE_INT(byteStream, m_buildingId);
            break;

        case CreateUnit:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId );
            WRITE_UNSIGNED_CHAR(byteStream, m_entityType );
            WRITE_INT(byteStream, m_numTroops );
            WRITE_INT(byteStream, m_buildingId );
            WRITE_FLOAT( byteStream, GetWorldPos().x );
            WRITE_FLOAT( byteStream, GetWorldPos().y );
            WRITE_FLOAT( byteStream, GetWorldPos().z );
            break;

        case AimBuilding:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId );
            WRITE_INT(byteStream, m_buildingId );
            WRITE_FLOAT( byteStream, GetWorldPos().x );
            WRITE_FLOAT( byteStream, GetWorldPos().y );
            WRITE_FLOAT( byteStream, GetWorldPos().z );
            break;

        case ToggleLaserFence:
            WRITE_INT(byteStream, m_buildingId);
            break;

        case RunProgram:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId );
            WRITE_UNSIGNED_CHAR(byteStream, m_program );
            break;

        case TargetProgram:
            WRITE_UNSIGNED_CHAR(byteStream, m_teamId );
            WRITE_UNSIGNED_CHAR(byteStream, m_program );
            WRITE_FLOAT( byteStream, GetWorldPos().x );
            WRITE_FLOAT( byteStream, GetWorldPos().y );
            WRITE_FLOAT( byteStream, GetWorldPos().z );
            break;

        case Invalid:
            DarwiniaDebugAssert(false);

    }

    *_linearSize = byteStream - m_byteStream;
	DarwiniaDebugAssert( *_linearSize < NETWORKUPDATE_BYTESTREAMSIZE );
	return m_byteStream;

}


//void NetworkUpdate::SendToDebugStream(FILE *_out, int _seqNum)
//{
//    _ostr << _seqNum << ": ";
//
//    switch (m_type)
//    {
//    case Invalid:
//        _ostr << "Invalid: ";
//        break;
//    case ClientJoin:
//        _ostr << "Client Join: ";
//        break;
//    case RequestTeam:
//        _ostr << "Request Team: ";
//        break;
//    case Alive:
//        _ostr << "Alive: ";
//        break;
//    case SelectUnit:
//        _ostr << "SelectUnit:" ;
//        break;
//    }
//    _ostr << "\n";
//}


// *** Operator =
NetworkUpdate const &NetworkUpdate::operator = (NetworkUpdate const &n)
{
    memcpy( this, &n, sizeof(NetworkUpdate) );
    return *this;
}
