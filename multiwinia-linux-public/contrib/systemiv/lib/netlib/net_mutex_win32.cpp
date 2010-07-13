#include "lib/universal_include.h"

#include "net_mutex.h"

// NetMutex

NetMutex::NetMutex()
{
	InitializeCriticalSection(&m_mutex);
	m_locked = 0;
}


NetMutex::~NetMutex()
{
	DeleteCriticalSection(&m_mutex);
	m_locked = 0;
}


bool NetMutex::TryLock()
{
	bool entered = TryEnterCriticalSection(&m_mutex);
	if( entered )
		m_locked = 1;
	
	return entered;
}

void NetMutex::Lock()
{
	EnterCriticalSection(&m_mutex);
	m_locked = 1;
}


void NetMutex::Unlock()
{
	LeaveCriticalSection(&m_mutex);
	m_locked = false;
}


// NetEvent

NetEvent::NetEvent()
{
	m_event = CreateEvent( NULL, false, false, NULL );
}

NetEvent::~NetEvent()
{
	CloseHandle( m_event );
}

void NetEvent::Signal()
{
	SetEvent( m_event );
}

bool NetEvent::Wait( double _timeToWait /* milliseconds */ )
{
	DWORD waitTime;

	if( _timeToWait < 0.0 )
		waitTime = INFINITE;
	else
		waitTime = _timeToWait;

	return WaitForSingleObject( m_event, waitTime ) == WAIT_OBJECT_0;
}

// --- NetSemaphore

NetSemaphore::NetSemaphore( int _initialValue )
{
	m_semaphore = CreateSemaphore( NULL, _initialValue, MAXLONG, NULL );
}

NetSemaphore::~NetSemaphore()
{
	CloseHandle( m_semaphore );
}

void NetSemaphore::Signal()
{
	ReleaseSemaphore( m_semaphore, 1, NULL );
}

bool NetSemaphore::Wait( double _timeToWait /* milliseconds */ )
{
	DWORD waitTime;

	if( _timeToWait < 0.0 )
		waitTime = INFINITE;
	else
		waitTime = _timeToWait;

	return WaitForSingleObject( m_semaphore, waitTime ) == WAIT_OBJECT_0;
}

