#include "lib/universal_include.h"

#include <string.h>		// For memcpy

#include "net_udp_packet.h"

NetUdpPacket::NetUdpPacket()
	: m_length(0)
{
	memset(m_data, 0, MAX_PACKET_SIZE * sizeof(char));
	memset(&m_clientAddress, 0, sizeof(m_clientAddress));
}

void NetUdpPacket::IpAddressToStr(char *_ipAddress)
{
	::IpAddressToStr(m_clientAddress.sin_addr, _ipAddress);
}

int NetUdpPacket::GetPort()
{
	return ntohs(m_clientAddress.sin_port);
}

