#ifndef SERVER_TO_CLIENT_LETTER_H
#define SERVER_TO_CLIENT_LETTER_H

#include "lib/vector3.h"
#include "worldobject/worldobject.h"
#include "lib/tosser/directory.h"

#define SERVERTOCLIENTLETTER_BYTESTREAMSIZE	1024


// ****************************************************************************
//  Class ServerToClientLetter
// ****************************************************************************

class ServerToClientLetter
{
public:
    int             m_receiverId;
    bool            m_clientDisconnected;
    Directory       *m_data;

    ServerToClientLetter()
    :   m_receiverId(-1),
        m_data(NULL),
        m_clientDisconnected(false)
    {}

    ~ServerToClientLetter()
    {
        delete m_data;        
    }

	void AddUpdate( const Directory *_update );
};

#endif
