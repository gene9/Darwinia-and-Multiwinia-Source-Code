#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "network/bandwidth.h"


BandwidthCounter::BandwidthCounter()
:	m_since( 0 ),
	m_rate( 0 ),
	m_count( 0 )
{
}

void BandwidthCounter::Count( unsigned _numBytes )
{
	const double interval = 5.0;
	double now = GetHighResTime();
	double period = now - m_since;

	if( period > interval )
	{
		m_rate = m_count / period;
		m_count = 0;
		m_since = now;
	}

	m_count += _numBytes;
}
