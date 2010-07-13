#include "lib/universal_include.h"

#include "net_socket.h"


NetSocket::NetSocket()
:	m_sockfd( -1 ),
	m_timeout( 10000 ),
	m_polltime( 100 ),
	m_port( 0 ),
	m_connected( false )
{
	memset(&m_to, 0, sizeof(m_to));
	memset(&m_from, 0, sizeof(m_from));
	memset(&m_listener, 0, sizeof(m_listener));
	memset(m_hostname, 0, MAX_HOSTNAME_LEN);
}


NetSocket::~NetSocket()
{
	if (m_sockfd != -1)
	{
		NetCloseSocket(m_sockfd);
		m_sockfd = -1;
	}
}


NetIpAddress NetSocket::GetRemoteAddress()
{
	return m_to;
}

NetIpAddress NetSocket::GetLocalAddress()
{
	return m_from;
}


NetRetCode NetSocket::CheckTimeout(unsigned int *timeout, unsigned int *timedout, int haveAllData)
{
	NetRetCode ret = NetOk;
	*timeout += m_polltime;
	if (*timeout >= m_timeout)
	{
		// If we've reached the timeout limit
		*timedout = 1;
		
		// Check if we have all the data
		switch (haveAllData)
		{
			// If we don't know if we have all the data, just return a time out
			case -1:
				ret = NetTimedout;
				break;
			
			// If we know we don't have all the data, return a 'more data' value
			case 0:
				ret = NetMoreData;
				break;
			
			// If we know we have all the data, return success
			case 1:
				ret = NetOk;
				break;
		}
	}

	return ret;
}


NetRetCode NetSocket::Connect(const char *host, unsigned short port)
{
	NetRetCode ret = NetFailed;
	
	if (host && port)
	{
		m_port = port;
		memset(m_hostname, 0, MAX_HOSTNAME_LEN);
		memcpy(m_hostname, host, MIN(strlen(host), MAX_HOSTNAME_LEN - 1));
		
		ret = Connect();
	}
	
	return ret;
}


void NetSocket::Close()
{
	if (m_sockfd != -1)
	{
		NetCloseSocket(m_sockfd);
		m_sockfd = -1;
	}
}


NetRetCode NetSocket::Connect()
{
	NetRetCode ret = NetOk;
	unsigned long timeout = 0;
	int err = 0;
	struct sockaddr_in *servaddr = &m_to;
	int sockType = SOCK_DGRAM;
	int protocol = IPPROTO_UDP;

	m_sockfd = socket(AF_INET, sockType, protocol);    
	if (!m_sockfd)
	{
		return NetFailed;
	}
	
	memset(servaddr, 0, sizeof(struct sockaddr_in));
	servaddr->sin_family = AF_INET;
	servaddr->sin_port = htons(m_port);
	
	NetHostDetails *pHostent = NetGetHostByName(m_hostname);
	if (!pHostent)
	{
		NetDebugOut("Host address resolution failed for %s\n", m_hostname);
		return NetFailed;
	}
	else 
	{
		servaddr->sin_addr.s_addr = * ((unsigned long *)pHostent->h_addr_list[0]);
	}

	// Set socket to non - blocking mode
	NetSetSocketNonBlocking(m_sockfd);
	
	// Attempt the connect until we timeout
	while (::connect(m_sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0)
	{
		NetDebugOut("Connection error: ");
		err = NetGetLastError();
		if (NetIsBlockingError(err))
		{
			timeout += 100;
			if (timeout > m_timeout)
			{
				NetDebugOut("Time out connecting to host\n");
				ret = NetTimedout;
				break;
			}
		}
		else if (NetIsConnected(err))
		{
			NetDebugOut("Already connected\n");
			break;
		}
		else
		{
			NetDebugOut("Connect to host failed: %d\n", err);
			ret = NetFailed;
			break;
		}
		NetSleep(100);
	}
	
	if (ret == 0) {
		m_connected = true;
		socklen_t size = sizeof(m_from);
		getsockname(m_sockfd, (struct sockaddr *) &m_from, &size);
	}
	
	return ret;
}


// Write method using select
NetRetCode NetSocket::WriteData(void *bufAsVoid, int bufLen, int *numActualBytes)
{
	NetRetCode ret = NetOk;
	char *buf = (char *)bufAsVoid;
	int bytesLeft = bufLen;
	int bytesSent = 0;
	int err = 0;
	unsigned int timeout = 0;
	unsigned int timedout = 0;
	int haveAllData = 0;
	
	struct timeval timeVal;
	long timeoutSeconds = (long)((m_polltime*1000) / 100000);
	long timeoutUSeconds = (long)((m_polltime*1000) % 100000);


	if (!m_connected)
	{
		err = Connect();
		if (err != 0)
			return NetRetCode(err);
	}
	
	while ((bytesLeft > 0) && (!timedout))
	{
		FD_ZERO(&m_listener);
		FD_SET(m_sockfd, &m_listener);
		timeVal.tv_sec  = timeoutSeconds;
		timeVal.tv_usec = timeoutUSeconds;
		haveAllData = -1;
		
		switch (select(m_sockfd + 1, (fd_set *)0, &m_listener, (fd_set *)0, &timeVal))
		{
			case NET_SOCKET_ERROR:
				NetDebugOut("WriteData select call failed");
				return NetFailed;
			case 0:
				if (numActualBytes)
				{
					haveAllData = ((*numActualBytes == bufLen) ? 1 : 0);
				}
				ret = CheckTimeout(&timeout, &timedout, haveAllData);
				continue;
			default:
				if (!FD_ISSET(m_sockfd, &m_listener))
				{
					return NetFailed;
				}
								
				bytesSent = send(m_sockfd, (char *)buf, bytesLeft, 0);
				err = NetGetLastError();
				if (NetIsSocketError(bytesSent) && NetIsReset(err))
				{
					bytesLeft = 0;
					shutdown(m_sockfd, NCSD_SEND);
					ret = NetClientDisconnect;
				}
				else if ((bytesSent > 0) || NetIsBlockingError(err))
				{
					bytesLeft -= bytesSent;
					buf += bytesSent;
					if (numActualBytes)
					{
						*numActualBytes += bytesSent;
						haveAllData = ((*numActualBytes == bufLen) ? 1 : 0);
					}
					ret = CheckTimeout(&timeout, &timedout, haveAllData);
				}
				else
				{
					NetDebugOut("WriteData write call failed");
					bytesLeft = 0;
					shutdown(m_sockfd, NCSD_SEND);
					ret = NetFailed;
				}
				break;
		}
	}
	
	return ret;
}
