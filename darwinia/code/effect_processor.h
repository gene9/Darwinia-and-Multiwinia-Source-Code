#ifndef _INCLUDED_EFFECT_PROCESSOR
#define _INCLUDED_EFFECT_PROCESSOR

#ifdef SOUND_EDITOR

#include "lib/llist.h"


// This module provides a mechanism to define volume envelopes and frequency 
// changes that can be used to drive the Pokey chip. The idea is that the
// registers of the Pokey will be updated many times a second (say 60). In
// the old days this was done by an Interrupt Service Routine. 
//   ____________
//  /            \
// /              \
// 0 1          2 3
// A typical volume envelope might look like this. It contains four 
// VolumePoints. The m_time variables might be 0, 30, 100, 130 respectively.
// Interpolation is used to generate volume levels for times that are not
// specified directly - ie time index 15 would produce half volume.


// ***************
// Class TimeValue
// ***************

class TimeValue
{
public:
	enum 
	{
		TypeNone = -1,
		TypeFreq = 0,
		TypeCtrl,
		TypeVol,
		TypeNumParametersInAChannel,
		TypeTime
	};

	// if this is a volume value then 0 = mute, 255 = full volume (note the pokey will only use the top 4 bits)
	// if it is a freq value then 0 = high freq, 255 = low freq (this value is used as a clock divider)
	unsigned char m_value;	
	unsigned short m_time;	// How many interrupts to occur before this point becomes active.

	TimeValue(unsigned char _val, unsigned short _time) : m_value(_val), m_time(_time) {}
};


// *************
// Class Channel
// *************

class Channel
{
public:
	unsigned m_duration;
	unsigned m_currentTime;
	bool	 m_looping;

	LList<TimeValue *> m_points[TimeValue::TypeNumParametersInAChannel];

    // For the example above, if m_currentTime==15, this equals 0
	unsigned m_currentPoint[TimeValue::TypeNumParametersInAChannel]; 

public:
	Channel();
	~Channel();

	bool Advance();

	unsigned GetDuration();

	unsigned char GetVolume(); // Returns an interpolated value between volume points
	unsigned char GetFreq();   // Returns an interpolated value between frequency points	
	unsigned char GetCtrl();   // Returns the most recent ctrl value
	unsigned char GetValue(int _type);

	void CreateTimeValue(int _type, unsigned char _value, unsigned short _time);
	void DeleteTimeValue(int _type, int _index);
	void SetTimeValue(int _type, unsigned char _value, unsigned short _time, int _index);
};


// *********************
// Class EffectProcessor
// *********************

class EffectProcessor
{
public:
	Channel *m_channel;
	bool m_active;

public:
	EffectProcessor();
	~EffectProcessor();
	
	void Advance(); // Called as if by an interrupt, say every 60th of a second
};

#endif // SOUND_EDITOR

#endif
