#ifndef __NETWORKQUALITY_H
#define __NETWORKQUALITY_H

#include "lib/tosser/llist.h"
#include "lib/netlib/net_lib.h"
#include <string>

class NetworkQuality {
public:
	NetworkQuality( unsigned _latencyInMilliseconds, float _packetLossProbability, unsigned _bandwidth );

	NetRetCode Write( NetSocketSession *_socket, const char *_data, unsigned _len, int *_amountWritten );
	void Advance();

private:
	struct Packet {
		Packet( int );
		Packet();
		Packet( NetSocketSession *_socket, const char *_data, unsigned _len );

		NetSocketSession *m_socket;
		std::string m_data;

		double m_time;

		unsigned GetLength() const;
	};

	LList<Packet> m_queue;
	LList<Packet> m_oneSecondWindow;

	unsigned m_latency, m_bandwidth;
	float m_packetLoss;

	unsigned m_bandwidthInUse;

};

#endif // __NETWORKQUALITY_H
