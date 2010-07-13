#ifndef __IFRAME_H
#define __IFRAME_H

#include "lib/tosser/llist.h"

class Vector3;
class SyncDiff;
class RGBAColour;

class IFrame 
{
public:
	IFrame();
	IFrame( IFrame const &_copyMe );
	IFrame( const char *_data, int _length );
	~IFrame();

	void			Generate(); 
	unsigned char	HashValue() const;
	int				SequenceId() const;

	const char *	Data() const;
	int				Length() const;

	void			CalculateDiff( const IFrame &_other, const RGBAColour &_otherColour, LList<SyncDiff *> &_differences );

private:
	void			CalculateHashValue();

	char			*m_data;
	int				m_sequenceId;
	int				m_length;
	unsigned char	m_hashValue;
};

void DumpSyncDiffs( LList<SyncDiff *> &_differences );

#endif