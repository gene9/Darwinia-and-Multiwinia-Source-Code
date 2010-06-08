// ****************************************************************************
// A UDP socket listener implementation. This class blocks until data is
// received, so you probably want to put it in its own thread.
// ****************************************************************************

#ifndef INCLUDED_NET_SOCKET_LISTENER_H
#define INCLUDED_NET_SOCKET_LISTENER_H


#include "net_lib.h"


class NetSocketListener
{
protected:
	NetSocketHandle 	m_sockfd;
	int					m_listening;
	unsigned short	 	m_port;

public:
	NetSocketListener(unsigned short port);
	~NetSocketListener();
	
	NetRetCode	StartListening(NetCallBack fnptr);
	
	// Stops the listener after next (non-blocking) accept call
	void		StopListening();
};


#endif
