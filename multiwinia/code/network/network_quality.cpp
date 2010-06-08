#include "lib/universal_include.h"
#include "network_quality.h"
#include "lib/math/random_number.h"
#include "lib/hi_res_time.h"
#include "net_lib.h"


NetworkQuality::NetworkQuality( unsigned _latencyInMilliseconds, float _packetLossProbability, unsigned _bandwidth )
:	m_latency( _latencyInMilliseconds ),
	m_packetLoss( _packetLossProbability ),
	m_bandwidth( _bandwidth ),
	m_bandwidthInUse( 0 )
{
}

NetRetCode NetworkQuality::Write( NetSocketSession *_socket, const char *_data, unsigned _len, int *_amountWritten )
{
	Advance();

	if( _amountWritten )
		*_amountWritten = _len;

	if( m_bandwidthInUse > m_bandwidth )
		return NetOk;

	if( AppRandom() % 1000 > m_packetLoss * 1000 )
	{
		Packet p( _socket, _data, _len );

		m_queue.PutDataAtEnd( p );
		m_oneSecondWindow.PutDataAtEnd( p );
		m_bandwidthInUse += _len;

	}

	return NetOk;
}

void NetworkQuality::Advance()
{
	double now = GetHighResTime();

	double oneSecondAgo = now - 1.0;
	int numPackets = m_oneSecondWindow.Size();
	for( int i = 0; i < numPackets; i++ )
	{
		const Packet &p = m_oneSecondWindow[i];
		if( p.m_time < oneSecondAgo )
		{
			m_bandwidthInUse -= p.GetLength();
			m_oneSecondWindow.RemoveData( i );
			i--;
			numPackets--;
		}
	}

	unsigned numQueued = m_queue.Size();
	for( int i = 0; i < numQueued; i++ )
	{
		const Packet &p = m_queue[i];
		if( now >= p.m_time + m_latency / 1000.0 )
		{
			// Send it!
			p.m_socket->WriteData( p.m_data.data(), p.m_data.size() );
				
			m_queue.RemoveData( i );
			i--;
			numQueued--;
		}
	}

}


NetworkQuality::Packet::Packet( NetSocketSession *_socket, const char *_data, unsigned _len )
:	m_data( _data, _len ),
	m_time( GetHighResTime() ),
	m_socket( _socket )
{
}

NetworkQuality::Packet::Packet()
:	m_time( 0.0 ),
	m_socket( NULL )
{
}

NetworkQuality::Packet::Packet( int )
:	m_time( 0.0 ),
	m_socket( NULL )
{
}

unsigned NetworkQuality::Packet::GetLength() const
{
	return m_data.size();
}
