// See header file for module description

#include "net_lib.h"
#include "net_socket.h"
#include "net_socket_listener.h"
#include "net_thread.h"
#include "net_udp_packet.h"


NetSocketListener::NetSocketListener(unsigned short port)
{
	m_port = port;
	m_listening = 0;
}


NetSocketListener::~NetSocketListener()
{
	NetCloseSocket(m_sockfd);
}


NetRetCode NetSocketListener::StartListening(NetCallBack functionPointer)
{
	NetRetCode ret = NetOk;
	int bindAttempts = 0;
	NetIpAddress servaddr;
	NetIpAddress clientaddr;

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&clientaddr, 0, sizeof(clientaddr));
	
	if (!(m_sockfd = socket(AF_INET, SOCK_DGRAM, 0)))
	{
		return NetFailed;
	}
	
	NetSocketHandle client = 0;
	unsigned int addrlen = sizeof(clientaddr);
	
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(m_port);
	
	// Make sure incoming arguments make sense
	if (functionPointer == (NetCallBack)NULL)
	{
		return NetBadArgs;
	}
	
	// Signal that we should be listening
	m_listening = 1;
	
	// Bind socket to port
	while ((bind(m_sockfd, (sockaddr *)&servaddr, sizeof(servaddr)) == -1) &&
		   m_listening)
	{
		if ((bindAttempts++ == 10) || (!NetIsAddrInUse))
		{
			return NetFailed;
		}
		else
		{
			NetDebugOut("Cannot bind to port");
			NetSleep(bindAttempts * 1000);
		}
	}
	
	int datasize = 0;
	char buf[MAX_PACKET_SIZE];
	NetSocketLenType cliLen = sizeof(clientaddr);
	
	while (m_listening)
	{
		memset(buf, 0, MAX_PACKET_SIZE * sizeof(char));
		datasize = recvfrom(m_sockfd, buf, MAX_PACKET_SIZE, 0,
							(struct sockaddr *)&clientaddr, &cliLen);
		
		// Call function pointer with datagram data (type is NetUdpPacket) -
		// the function pointed to must free NetUdpPacket passed in
		(*functionPointer)(new NetUdpPacket(client, &clientaddr, buf, datasize));
		if (!m_listening) break;
	}
	
	return ret;
}

   
void NetSocketListener::StopListening()
{
	m_listening = 0;
	NetSleep(250);
	if (m_sockfd)
	{
		shutdown(m_sockfd, 0);
	}
}
