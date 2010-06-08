// ****************************************************************************
// A UDP socket listener implementation. This class blocks until data is
// received, so you probably want to put it in its own thread.
// ****************************************************************************

#ifndef INCLUDED_NET_SOCKET_LISTENER_H
#define INCLUDED_NET_SOCKET_LISTENER_H


#include "net_lib.h"

#include <string>


class NetMutex;

class NetSocketListener
{
public:
	NetSocketListener(unsigned short port, const std::string &_name, const std::string &_interface = "");
	~NetSocketListener();

	NetRetCode	StartListening(NetCallBack fnptr);
	
	// Asynchronously stops the listener after next accept call
	void		StopListening();

	// Called by NetSocketSession to get the socket
	NetSocketHandle		GetBoundSocketHandle();

	NetRetCode	Bind();
	
    NetRetCode  EnableBroadcast();

	int			GetPort() const;

	NetIpAddress	GetListenAddress() const;
	
protected:
	void AppDebugOutNetSocketError( int _errorCode );

	NetSocketHandle 	m_sockfd;
	bool				m_binding;
	bool				m_listening;
	unsigned short	 	m_port;
	std::string			m_name;
	std::string			m_interface;
	NetMutex           *m_mutex;
};

#endif
