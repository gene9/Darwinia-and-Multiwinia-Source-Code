#ifndef __BANDWIDTH_H
#define __BANDWIDTH_H

class BandwidthCounter {
public:

	BandwidthCounter();

	operator float () const;

	void Count( unsigned _numBytes );
	float Rate() const;

private:

	double m_since;
	unsigned m_count;
	float m_rate;
};

inline BandwidthCounter::operator float() const
{
	return Rate();
}

inline float BandwidthCounter::Rate() const
{
	return m_rate;
}

#endif // __BANDWIDTH_H

