/* Represents one connection to one client. Server is expected to have a list of these. */


#ifndef _SERVERTOCLIENT_H
#define _SERVERTOCLIENT_H


class NetSocket;


class ServerToClient
{
private:
    char		m_ip[16];
    NetSocket	*m_socket;

public:
    ServerToClient( char *_ip );

    char        *GetIP ();
    NetSocket   *GetSocket ();

    int         m_lastKnownSequenceId;
};



#endif

