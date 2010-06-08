#ifndef INCLUDED_NET_UDPDATA_H
#define INCLUDED_NET_UDPDATA_H


// ****************************************************************************
//  An object containing a single UDP datagram
// ****************************************************************************

#include "net_lib.h"


class NetUdpPacket
{
public:
	NetUdpPacket();
	
	void IpAddressToStr(char *_ipAddress);
	int  GetPort();
	
	int 			m_length;
	NetIpAddress	m_clientAddress;
	char 			m_data[MAX_PACKET_SIZE];
};


#endif
