// ****************************************************************************
// A UDP socket sender class
// ****************************************************************************

#ifndef INCLUDED_NET_SOCKET_H
#define INCLUDED_NET_SOCKET_H


#include <stdio.h>

#include "net_lib.h"


class NetSocket
{
protected:
	// Establishes a connection to the currently stored m_hostname and m_port.
	// Returns NetOK on success, NetTimedout on timeout and NetFailed on failure
	NetRetCode Connect();

public:
	NetSocket();
	~NetSocket();
	
	// Establishes a connection to the host/port specified
	// on object construction. Returns NetOK on success,
	// NetTimedout on timeout, and NetFailed on failure
	NetRetCode Connect(const char *host, unsigned short port);
	
	// Close the current socket connection
	void Close();
	
	// This method writes up to numBytes from buf and returns NetOK in the 
	// simplest case.  If the NetSocket::setTimeout is called with a timeout
	// that is shorter than the time it takes to write all of the requested
	// data, this method will write as many bytes as possible(up to numBytes)
	// from buf and store the actual number of bytes sent in the numActualBytes
	// argument if it is included.  If numActualBytes != numBytes and the
	// read times out, a value of NetMoreData is returned.
	//
	// Thus, the easiest way to do a blocking write is to set the timeout
	// to a large value(i.e. 60000ms which is 60 seconds).  On the other hand,
	// the easiest way to do a non - blocking read is to set the timeout to
	// a small value(i.e. 0ms, or 0 seconds).  The default timeout value
	// is ~10 seconds - thus the sockets act as blocking sockets by default.
	//  
	// If no data is write to the socket at all before the specified timeout
	// period, this method returns a value of NetTimeout.
	//	
	// Note: NetMoreData will never be returned unless the numActualBytes
	// argument is non - null when calling this method.
	//	  
	// Returns NetClientDisconnect if a client forcefully disconnects.
	NetRetCode WriteData(void *buf, int bufLen, int *numActualBytes = 0);
			
	NetIpAddress		GetRemoteAddress();
	NetIpAddress		GetLocalAddress();
	
protected:
	NetRetCode			CheckTimeout(unsigned int *timeout,	unsigned int *timedout,	int haveAllData);
	
	NetSocketHandle		m_sockfd;
	NetPollObject		m_listener;
	NetIpAddress		m_from;
	NetIpAddress		m_to;
	unsigned int		m_timeout;
	unsigned int		m_polltime;
	unsigned short		m_port;
	bool				m_connected;
	char				m_hostname[MAX_HOSTNAME_LEN];
};


#endif
