#include "lib/universal_include.h"

#include "lib/debug_utils.h"

#include "effect_processor.h"

#ifdef SOUND_EDITOR

// ****************************************************************************
// Class Channel
// ****************************************************************************

// *** Constructor
Channel::Channel()
:	m_duration(0),
	m_currentTime(0),
	m_looping(true)
{
	for (int i = 0; i < TimeValue::TypeNumParametersInAChannel; ++i)
	{
		m_currentPoint[i] = 0;
	}
}


// *** Destructor
Channel::~Channel()
{
	for (int i = 0; i < TimeValue::TypeNumParametersInAChannel; ++i)
	{
		m_points[i].EmptyAndDelete();
	}
}


// *** Advance
// Returns false if we have reached the end 
bool Channel::Advance()
{
	m_currentTime++;
	if (m_currentTime >= GetDuration())
	{
		m_currentTime = 0;
		for (int i = 0; i < TimeValue::TypeNumParametersInAChannel; ++i)
		{
			m_currentPoint[i] = 0;
		}
		return m_looping;
	}

	return true;
}


// *** GetDuration
unsigned Channel::GetDuration()
{
	return m_duration;
}


unsigned char Channel::GetValue(int _type)
{
	if (_type == TimeValue::TypeCtrl)
	{
		TimeValue *p1 = m_points[_type].GetData(m_currentPoint[_type]);
		TimeValue *p2 = m_points[_type].GetData(m_currentPoint[_type] + 1);
		if (p2 && p2->m_time <= m_currentTime)
		{
			p1 = p2;
			m_currentPoint[_type]++;
		}

		return p1->m_value;
	}
	else
	{
		TimeValue *p1 = m_points[_type].GetData(m_currentPoint[_type]);
		TimeValue *p2 = m_points[_type].GetData(m_currentPoint[_type] + 1);
		if (p2 && p2->m_time <= m_currentTime)
		{
			m_currentPoint[_type]++;
			p1 = p2;
			p2 = m_points[_type].GetData(m_currentPoint[_type] + 1);
		}

		if (p2)
		{
			float deltaTime = p2->m_time - p1->m_time;
			float deltaVolume = p2->m_value - p1->m_value;
			float slope = deltaVolume / deltaTime;
			float interpTime = m_currentTime - p1->m_time;
			float returnVal = p1->m_value + slope * interpTime;
			return (unsigned char)returnVal;
		}

		return p1->m_value;
	}
}


// *** GetVolume
unsigned char Channel::GetVolume()
{
	return GetValue(TimeValue::TypeVol);	
//	TimeValue *p1 = m_volumePoints.GetData(m_currentVolumePoint);
//	TimeValue *p2 = m_volumePoints.GetData(m_currentVolumePoint + 1);
//	if (p2 && p2->m_time <= m_currentTime)
//	{
//		m_currentVolumePoint++;
//		p1 = p2;
//		p2 = m_volumePoints.GetData(m_currentVolumePoint + 1);
//	}
//
//	if (p2)
//	{
//		float deltaTime = p2->m_time - p1->m_time;
//		float deltaVolume = p2->m_value - p1->m_value;
//		float slope = deltaVolume / deltaTime;
//		float interpTime = m_currentTime - p1->m_time;
//		float returnVal = p1->m_value + slope * interpTime;
//		return (unsigned char)returnVal;
//	}
//
//	return p1->m_value;
}


// *** GetFreq
unsigned char Channel::GetFreq()
{
	return GetValue(TimeValue::TypeFreq);	
//	TimeValue *p1 = m_freqPoints.GetData(m_currentFreqPoint);
//	TimeValue *p2 = m_freqPoints.GetData(m_currentFreqPoint + 1);
//	if (p2 && p2->m_time <= m_currentTime)
//	{
//		m_currentFreqPoint++;
//		p1 = p2;
//		p2 = m_freqPoints.GetData(m_currentFreqPoint + 1);
//	}
//
//	if (p2)
//	{
//		float deltaTime = p2->m_time - p1->m_time;
//		float deltaFreq = p2->m_value - p1->m_value;
//		float slope = deltaFreq / deltaTime;
//		float interpTime = m_currentTime - p1->m_time;
//		float returnVal = p1->m_value + slope * interpTime;
//		return (unsigned char)returnVal;
//	}
//	
//	return p1->m_value;
}


// *** GetCtrl
unsigned char Channel::GetCtrl()
{
	return GetValue(TimeValue::TypeCtrl);	
//	TimeValue *p1 = m_ctrlPoints.GetData(m_currentCtrlPoint);
//	TimeValue *p2 = m_ctrlPoints.GetData(m_currentCtrlPoint + 1);
//	if (p2 && p2->m_time <= m_currentTime)
//	{
//		p1 = p2;
//		m_currentCtrlPoint++;
//	}
//
//	return p1->m_value;
}


// *** CreateTimeValue
// Inserts a new TimeValue into the correct place in a sorted list of 
// TimeValues. Will overwrite an existing TimeValue if there is one at 
// the specified time index already.
void Channel::CreateTimeValue(int _type, unsigned char _freq, unsigned short _time)
{
	if (_time > m_duration)
	{
		m_duration = _time;
	}
	
	LList<TimeValue*> *points = &m_points[_type];

	unsigned int numPoints = points->Size();
	for (unsigned int i = 0; i < numPoints; ++i)
	{
		TimeValue *p = points->GetData(i);
		if (p->m_time == _time)
		{
			p->m_value = _freq;
			return;
		}
		else if (p->m_time > _time)
		{
			TimeValue *newP = new TimeValue(_freq, _time);
			points->PutDataAtIndex(newP, i);
			return;
		}
	}

	TimeValue *newP = new TimeValue(_freq, _time);
	points->PutDataAtEnd(newP);
}


// *** DeleteTimeValue
void Channel::DeleteTimeValue(int _type, int _index)
{
	if (_index <= 0) return;

	LList<TimeValue*> *points = &m_points[_type];

	if (_index >= points->Size())
	{
		return;
	}
	
	points->RemoveData(_index);
}


// *** SetTimeValue
void Channel::SetTimeValue(int _type, unsigned char _value, unsigned short _time, int _index)
{
	LList<TimeValue*> *points = &m_points[_type];

	TimeValue *t = points->GetData(_index);
	DarwiniaDebugAssert(t);
	
	points->RemoveData(_index);
	t->m_time = _time;
	t->m_value = _value;

	// Find out where the new time value should go in the sorted list
	bool insertAtEnd = true;
	for (int i = 0; i < points->Size(); ++i)
	{
		TimeValue *localT = points->GetData(i);
		if (localT->m_time > _time)
		{
			points->PutDataAtIndex(t, i);
			insertAtEnd = false;
			break;
		}
	}

	if (insertAtEnd)
	{
		points->PutDataAtEnd(t);

		// Re-evaluate m_duration
		m_duration = _time;
	}
}



// ****************************************************************************
// Class EffectProcessor
// ****************************************************************************

// *** Constructor
EffectProcessor::EffectProcessor()
:	m_active(true)
{
	m_channel = new Channel();
	m_channel->CreateTimeValue(TimeValue::TypeVol, 255, 0);
	m_channel->CreateTimeValue(TimeValue::TypeVol, 255, 60);

	m_channel->CreateTimeValue(TimeValue::TypeFreq, 10, 0);
	m_channel->CreateTimeValue(TimeValue::TypeFreq, 40, 15);
	m_channel->CreateTimeValue(TimeValue::TypeFreq, 10, 45);
	m_channel->CreateTimeValue(TimeValue::TypeFreq, 40, 60);

	m_channel->CreateTimeValue(TimeValue::TypeCtrl,  1, 0);
	m_channel->CreateTimeValue(TimeValue::TypeCtrl,  2, 10);
	m_channel->CreateTimeValue(TimeValue::TypeCtrl,  7, 50);
}


// *** Destructor
EffectProcessor::~EffectProcessor()
{
	delete m_channel;
}


// *** Advance
void EffectProcessor::Advance()
{
	if (m_active)
	{
		m_active = m_channel->Advance();

		int const currentChannel = 0;
		int const currentChip = 0;
		int const gain = 4;

		// Update frequency register
		unsigned char freq = m_channel->GetFreq();
		Update_pokey_sound(0xd200 + currentChannel * 2, freq, currentChip, gain);

		// Update volume and control register
		unsigned char vol = m_channel->GetVolume() >> 4;
		unsigned char ctrl = m_channel->GetCtrl() << 5;
		Update_pokey_sound(0xd201 + currentChannel * 2, ctrl | vol, currentChip, gain);
	}
}

#endif // SOUND_EDITOR
