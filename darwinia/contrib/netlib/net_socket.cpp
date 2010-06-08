// See header file for module description

#include "net_socket.h"

#ifdef TARGET_MSVC
#define fdopen _fdopen
#endif

NetSocket::NetSocket()
{
	m_sockfd = -1;
	m_stdiofd = (FILE *)0;
	m_timeout = 10000;
	m_polltime = 100;
	m_port = 0;
	m_ipaddr = 0;
	memset(&m_to, 0, sizeof(m_to));
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


NetRetCode NetSocket::Flush()
{
	NetRetCode ret = NetOk;
	if (!m_stdiofd)
	{
		m_stdiofd = fdopen(m_sockfd, "w");
	}
	if ((m_stdiofd ==(FILE *)0) || (fflush(m_stdiofd)))
	{
		ret = NetFailed;
	}
	return ret;
}


unsigned long NetSocket::GetIpAddr()
{
	return m_ipaddr;
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


NetRetCode NetSocket::Connect(char *host, unsigned short port)
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
	
	m_sockfd = socket(AF_INET, sockType, 0);
	if (!m_sockfd)
	{
		return NetFailed;
	}
	
	memset(servaddr, 0, sizeof(struct sockaddr_in));
	servaddr->sin_family = AF_INET;
	servaddr->sin_port = htons(m_port);
	
	// Resolve dotted IP address
	/* Removed, didn't seem to work for addresses like 127.0.0.1 and 90.0.0.3 */
	//   if (!ncValidInetAddr(ncGetInetAddr(m_hostname,&servaddr->sin_addr.s_addr)))
	//   {
	// Otherwise, resolve host name
	NetHostDetails *pHostent = NetGetHostByName(m_hostname);
	if (!pHostent)
	{
		NetDebugOut("Host address resolution failed for %s", m_hostname);
		return NetFailed;
	}
	else 
	{
		servaddr->sin_addr.s_addr = * ((unsigned long *)pHostent->h_addr_list[0]);
	}
	//   }
	
	// Stash the IP address as an unsigned long in host byte order
	m_ipaddr = servaddr->sin_addr.s_addr;
	
	// Set socket to non - blocking mode
	NetSetSocketNonBlocking(m_sockfd);
	
	// Attempt the connect until we timeout
	while (::connect(m_sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0)
	{
		NetDebugOut("Connection error");
		if (NetIsBlockingError(err))
		{
			timeout += 100;
			if (timeout > m_timeout)
			{
				NetDebugOut("Time out connecting to host");
				ret = NetTimedout;
				break;
			}
		}
		else if (NetIsConnected(err))
		{
			NetDebugOut("Already connected");
			break;
		}
		else
		{
			NetDebugOut("Connect to host failed: %d", err);
			ret = NetFailed;
			break;
		}
		NetSleep(100);
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
				
				bytesSent = sendto(m_sockfd, (char *)buf, bytesLeft, 0,
								(struct sockaddr *)&m_to, sizeof(m_to));
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
