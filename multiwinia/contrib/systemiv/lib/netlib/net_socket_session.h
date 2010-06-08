#ifndef INCLUDED_NET_SOCKET_SESSION_H
#define INCLUDED_NET_SOCKET_SESSION_H

#include "net_lib.h"
#include "net_socket_listener.h"

// Create a NetSocketSession to send data to remote hosts from a given
// socket, that may be used for listening.
//
// NetSocketSession uses the underlying socket of the NetSocketListener,
// so it'll only work while the NetSocketListener object is alive and well.

class NetSocketListener;

class NetSocketSession {
public:
	NetSocketSession(NetSocketListener &_l, const char *_host, unsigned _port);
	NetSocketSession(NetSocketListener &_l, NetIpAddress _to);
	
	NetRetCode WriteData(const void *_buf, int _bufLen, int *_numActualBytes = 0);

protected:
	NetSocketHandle		m_sockfd;
	NetIpAddress		m_to;
};

#endif // INCLUDED_NET_SOCKET_SESSION_H
