// See net_mutex.h for module description

#include "net_mutex.h"
#include <sys/time.h>

NetMutex::NetMutex()
{
   pthread_mutexattr_t attributes;
   
   // Make it a recursive mutex, since that's the default on Windows
   pthread_mutexattr_init(&attributes);
   pthread_mutexattr_settype(&attributes, PTHREAD_MUTEX_RECURSIVE);
   pthread_mutex_init(&m_mutex, &attributes);
   pthread_mutexattr_destroy(&attributes);
   m_locked = 0;
}


NetMutex::~NetMutex()
{
   pthread_mutex_destroy(&m_mutex);
   m_locked = 0;
}


void NetMutex::Lock()
{
   pthread_mutex_lock(&m_mutex);
   m_locked = 1;
}

bool NetMutex::TryLock()
{
	bool success = ( pthread_mutex_trylock(&m_mutex) == 0 );
	if (success)
		m_locked = 1;
		
	return success;
}

void NetMutex::Unlock()
{
   m_locked = 0;
   pthread_mutex_unlock(&m_mutex);
}
// --- NetEvent

NetEvent::NetEvent()
{
	pthread_mutex_init( &m_event.mutex, NULL );
	pthread_cond_init( &m_event.cond, NULL );
	m_event.signalled = false;
}

NetEvent::~NetEvent()
{
	pthread_mutex_destroy( &m_event.mutex );
	pthread_cond_destroy( &m_event.cond );
}

void NetEvent::Signal()
{
	pthread_mutex_lock( &m_event.mutex );
	m_event.signalled = true;
	pthread_cond_signal( &m_event.cond );	
	pthread_mutex_unlock( &m_event.mutex );
}

bool NetEvent::Wait( double _timeToWait /* milliseconds */ )
{
	bool success = true;
	
	pthread_mutex_lock( &m_event.mutex );
	if( !m_event.signalled )
	{
		if (_timeToWait >= 0)
		{
			struct timespec t;
			struct timeval	now;

			gettimeofday( &now, NULL );

			now.tv_usec += (unsigned long) ( _timeToWait * 1000.0 );
			if( now.tv_usec > 1000000 )
			{
				now.tv_sec += now.tv_usec / 1000000;
				now.tv_usec = now.tv_usec % 1000000;
			}

			t.tv_sec = now.tv_sec;
			t.tv_nsec = now.tv_usec * 1000L;

			if( pthread_cond_timedwait( &m_event.cond, &m_event.mutex, &t ) != 0 )
				success = false;
		}
		else // support the -1 = infinite wait idiom
		{
			if( pthread_cond_wait( &m_event.cond, &m_event.mutex ) != 0 )
				success = false;
		}
	}
	m_event.signalled = false;
	pthread_mutex_unlock( &m_event.mutex );

	return success;
}

// --- NetSemaphore

#ifndef TARGET_OS_MACOSX // FIXME: sem_timedwait() doesn't exist on OS X
NetSemaphore::NetSemaphore( int _initialValue )
{
	sem_init( &m_semaphore, 0, _initialValue );	
}

NetSemaphore::~NetSemaphore()
{
	sem_destroy( &m_semaphore );
}

void NetSemaphore::Signal()
{
	sem_post( &m_semaphore );
}

bool NetSemaphore::Wait( double _timeToWait /* milliseconds */ )
{
	if( _timeToWait < 0.0 )
		return sem_wait( &m_semaphore ) == 0;
	else if( _timeToWait == 0.0 )
	{
		return sem_trywait( &m_semaphore ) == 0;
	}
	else 
	{
		struct timespec t;
		struct timeval	now;

		gettimeofday( &now, NULL );

		now.tv_usec += (unsigned long) ( _timeToWait * 1000.0 );
		if( now.tv_usec > 1000000 )
		{
			now.tv_sec += now.tv_usec / 1000000;
			now.tv_usec = now.tv_usec % 1000000;
		}

		t.tv_sec = now.tv_sec;
		t.tv_nsec = now.tv_usec * 1000L;

		return sem_timedwait( &m_semaphore, &t ) == 0;
	}
}
#endif
