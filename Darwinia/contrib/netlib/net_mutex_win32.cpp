// See net_mutex.h for module description

#include "net_mutex.h"


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
