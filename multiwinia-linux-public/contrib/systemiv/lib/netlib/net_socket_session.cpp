#include "lib/universal_include.h"

#include "net_lib.h"
#include "net_socket_session.h"
#include "net_socket_listener.h"


NetSocketSession::NetSocketSession(NetSocketListener &_l, const char *_host, unsigned _port)
	: m_sockfd(_l.GetBoundSocketHandle())
{
	memset(&m_to, 0, sizeof(m_to));

#if defined(WIN32) && _MSC_VER <= 1200
	// Non thread-safe code 
	NetHostDetails *pHostent = NetGetHostByName(_host);
	if (!pHostent)
	{
		NetDebugOut("Host address resolution failed for %s\n", _host);
		m_sockfd = (NetSocketHandle) -1;		
		return;
	}
	else 
	{
		m_to.sin_addr.s_addr = * ((unsigned long *)pHostent->h_addr_list[0]);
	}
#else
	// This is the preferred thread-safe variant, but it seems to require MSVC > 6.0
	// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/winsock/winsock/getaddrinfo_2.asp

	// Attempt to resolve _host (getaddrinfo is threadsafe)
	struct addrinfo hints, *ai;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
	
	int error = getaddrinfo(_host, NULL, &hints, &ai);
	
	if (error != 0) 
	{
		NetDebugOut("Host address resolution failed for %s (%s)\n", _host, gai_strerror(error));
		m_sockfd = (NetSocketHandle)-1;
		return;
	}
	
    if( ai == NULL )
    {
        NetDebugOut("Error creating socket to %s\n", _host );
        m_sockfd = (NetSocketHandle)-1;
        return;
    }

	// Success, fill in the rest
	memcpy(&m_to, ai->ai_addr, sizeof(m_to));
	
	freeaddrinfo(ai);
#endif	
	m_to.sin_family = AF_INET;
	m_to.sin_port = htons(_port);
}

NetSocketSession::NetSocketSession(NetSocketListener &_l, NetIpAddress _to )
	: m_sockfd(_l.GetBoundSocketHandle()),
	  m_to(_to)
{
}

	
NetRetCode NetSocketSession::WriteData(const void *_buf, int _bufLen, int *_numActualBytes)
{
	int bytesSent = -1;

	if (int(m_sockfd) < 0)
	{
		NetDebugOut("NetSocketSession::WriteData invalid socket\n");
		return NetFailed;
	}

	bytesSent = sendto(m_sockfd, (const char *)_buf, _bufLen, 0, (struct sockaddr *)&m_to, sizeof(m_to));

	if (bytesSent < 0)
	{		
#if 0
		int errorCode = NetGetLastError(); 
		char ip[32];
		IpAddressToStr( m_to.sin_addr, ip );
		NetDebugOut("NetSocketSession::WriteData to %s:%d failed (%d : %s)\n", 
					ip,
					ntohs(m_to.sin_port),
					errorCode,
					strerror(errorCode));
#endif
		return NetFailed;
	}

	if (_numActualBytes != NULL)
	{
		*_numActualBytes = bytesSent;
	}

	return NetOk;
}

