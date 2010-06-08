// ****************************************************************************
//  A platform independent mutex implementation
// ****************************************************************************

#ifndef INCLUDED_NET_MUTEX_H
#define INCLUDED_NET_MUTEX_H


#include "net_lib.h"


class NetMutex
{
public:
	NetMutex();
	~NetMutex();
	
	bool			TryLock();
	void			Lock();
	void			Unlock();
	
	int 			IsLocked() { return m_locked; }
	
protected:
	int 			m_locked;
	NetMutexHandle 	m_mutex;
};

class NetLockMutex
{
public:
	NetLockMutex( NetMutex &_mutex )
		: m_mutex( _mutex )
	{
		m_mutex.Lock();
	}

	~NetLockMutex()
	{
		m_mutex.Unlock();
	}

private:

	NetLockMutex &operator =( const NetLockMutex & );
	NetMutex &m_mutex;
};


class NetEvent
{
public:
	NetEvent();
	~NetEvent();

	void Signal();
	bool Wait( double _timeToWait /* milliseconds */ );

private:
	NetEventHandle m_event;
};

#ifndef TARGET_OS_MACOSX  // FIXME: sem_timedwait() doesn't exist on OS X
class NetSemaphore
{
public:
	NetSemaphore( int _initialValue );
	~NetSemaphore();

	void Signal();
	bool Wait( double _timeToWait /* milliseconds */ );

private:
	NetSemaphoreHandle m_semaphore;
};
#endif

#endif
