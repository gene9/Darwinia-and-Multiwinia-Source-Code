#include "lib/universal_include.h"

#include <string.h>

#include "lib/debug_utils.h"

#include "network/bytestream.h"
#include "network/servertoclientletter.h"


static char s_byteStream[SERVERTOCLIENTLETTER_BYTESTREAMSIZE];



// *** Constructor
ServerToClientLetter::ServerToClientLetter()
:   m_clientId(-1),
    m_type(Invalid),
    m_sequenceId(0),
    m_teamId(0),
    m_teamType(0),
    m_ip(0)
{
}


// *** Constructor
ServerToClientLetter::ServerToClientLetter( ServerToClientLetter &copyMe )
:   m_clientId( copyMe.m_clientId ),
    m_type( copyMe.m_type ),    
    m_sequenceId( copyMe.m_sequenceId ),
    m_teamId(copyMe.m_teamId),
    m_teamType(copyMe.m_teamType),
    m_ip(copyMe.m_ip)    
{
    //memcpy( m_updates, copyMe.m_updates, MAX_SERVER_TO_CLIENT_UPDATES*sizeof(NetworkUpdate) );

    for( int i = 0; i < copyMe.m_updates.Size(); ++i )
    {
        NetworkUpdate *copyUpdate = copyMe.m_updates[i];
        NetworkUpdate *newUpdate = new NetworkUpdate();
        memcpy( newUpdate, copyUpdate, sizeof(NetworkUpdate) );
        m_updates.PutData( newUpdate );
    }   
}


// *** Constructor
ServerToClientLetter::ServerToClientLetter(char *_byteStream, int _len)
:   m_clientId(-1),
    m_type(Invalid),
    m_sequenceId(0),
    m_teamId(0),
    m_teamType(0),
    m_ip(0)
{
	m_type = (LetterType)READ_INT(_byteStream);
    m_sequenceId = READ_INT(_byteStream);

    switch(m_type)
    {
    case HelloClient:
    case GoodbyeClient:
        m_ip         = READ_INT(_byteStream);
        break;

    case TeamAssign:
        m_teamId    = READ_UNSIGNED_CHAR(_byteStream);
        m_teamType  = READ_UNSIGNED_CHAR(_byteStream);
        m_ip        = READ_INT(_byteStream);
        break;

    case Update:
        int numUpdates = READ_INT(_byteStream);
		DarwiniaDebugAssert(numUpdates >= 0);

        for( int i = 0; i < numUpdates; ++i )
        {
            NetworkUpdate *update = new NetworkUpdate();
            _byteStream += update->ReadByteStream( _byteStream );
            m_updates.PutData( update );
        }
        break;
    }

}


// *** SetSequenceId
void ServerToClientLetter::SetSequenceId(int _id)
{
    m_sequenceId = _id;
}


// *** SetTeamType
void ServerToClientLetter::SetTeamType(int teamType)
{
    m_teamType = teamType;
}


// *** SetTeamId
void ServerToClientLetter::SetTeamId(int teamId)
{
    m_teamId = teamId;
}

// *** SetSequenceId
void ServerToClientLetter::SetClientId(int _id)
{
    m_clientId = _id;
}


// *** SetType
void ServerToClientLetter::SetType( LetterType _type )
{
    m_type = _type;
}


// *** GetClientId
int ServerToClientLetter::GetClientId()
{
    return m_clientId;
}


// *** GetSequenceId
int ServerToClientLetter::GetSequenceId()
{
    return m_sequenceId;
}

// *** SetIp
void ServerToClientLetter::SetIp(int ip)
{
    m_ip = ip;
}

// *** AddUpdate
void ServerToClientLetter::AddUpdate ( NetworkUpdate *_update )
{    
    // Make sure we COPY the update
    NetworkUpdate *update = new NetworkUpdate();
    memcpy( update, _update, sizeof(NetworkUpdate) );
    m_updates.PutData( update );                   
}

// *** GetByteStream
char *ServerToClientLetter::GetByteStream(int *_linearSize)
{
	char *byteStream = s_byteStream;

    WRITE_INT(byteStream, m_type);
    WRITE_INT(byteStream, m_sequenceId);
    
    switch (m_type)
    {
    case HelloClient:
    case GoodbyeClient:
        WRITE_INT( byteStream, m_ip );
        break;

    case TeamAssign:
        WRITE_UNSIGNED_CHAR( byteStream, m_teamId );
        WRITE_UNSIGNED_CHAR( byteStream, m_teamType );
        WRITE_INT( byteStream, m_ip );
        break;

    case Update:
        int numUpdates = m_updates.Size();
		DarwiniaDebugAssert(numUpdates >= 0);
        WRITE_INT(byteStream, numUpdates);

        for( int i = 0; i < numUpdates; ++i )
        {
            NetworkUpdate *update = m_updates[i];
            int updateSize;
            char *updateBytes = update->GetByteStream( &updateSize );
            memcpy( byteStream, updateBytes, updateSize );
            byteStream += updateSize;
        }
        break;
    }

	*_linearSize = byteStream - s_byteStream;
	DarwiniaDebugAssert( *_linearSize < SERVERTOCLIENTLETTER_BYTESTREAMSIZE );

	return s_byteStream;
}

