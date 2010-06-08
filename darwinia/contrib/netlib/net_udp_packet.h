#ifndef INCLUDED_NET_UDPDATA_H
#define INCLUDED_NET_UDPDATA_H


// ****************************************************************************
//  An object containing a single UDP datagram
// ****************************************************************************

#include "net_lib.h"


class NetUdpPacket
{
public:
	NetUdpPacket(int sockfd, NetIpAddress *clientaddr, char *buf, int len);
	
	int 			m_sockfd;
	int 			m_length;
	NetIpAddress	m_clientAddress;
	char 			m_data[MAX_PACKET_SIZE];
};


#endif
