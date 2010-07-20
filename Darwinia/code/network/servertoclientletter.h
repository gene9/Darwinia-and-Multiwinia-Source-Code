#ifndef SERVER_TO_CLIENT_LETTER_H
#define SERVER_TO_CLIENT_LETTER_H

#include "lib/vector3.h"
#include "worldobject/worldobject.h"
#include "network/networkupdate.h"


#define SERVERTOCLIENTLETTER_BYTESTREAMSIZE	1024


// ****************************************************************************
//  Class ServerToClientLetter
// ****************************************************************************

class ServerToClientLetter
{
public:
    enum LetterType
    {
        Invalid,
        HelloClient,
        GoodbyeClient,
        TeamAssign,
        Update
    };

    LetterType m_type;                      // If you add any new data here, remember to update the copy constructor
    unsigned char m_teamId;
    unsigned char m_teamType;
    int m_ip;                               // This tells you specifically which client gets the HelloClient or TeamAssign

    LList<NetworkUpdate *> m_updates;

private:
    int m_clientId;                 // An index into the server's DArray of ServerToClient objects
    int m_sequenceId;

public:
    ServerToClientLetter();
    ServerToClientLetter( ServerToClientLetter &copyMe );
	ServerToClientLetter(char *_byteStream, int _len);

    void SetClientId    (int _id);
    void SetType        ( LetterType _type );
    void SetSequenceId  (int seqId);
    void SetTeamId      (int teamId);
    void SetTeamType    (int teamType);
    void SetIp          (int ip);

    int GetClientId();
    int GetSequenceId();

    void AddUpdate              ( NetworkUpdate *_update );

	// Writes all the current data into a sequential byte stream suitable to
	// be stuffed into a UDP packet. Sets linearSize to be the stream length.
	// Do NOT DELETE the returned pointer - it is part of this object.
	char *GetByteStream(int *_linearSize);
};



#endif
