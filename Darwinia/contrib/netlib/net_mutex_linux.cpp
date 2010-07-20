// See net_mutex.h for module description

#include "net_mutex.h"


NetMutex::NetMutex()
{
   pthread_mutex_init(&m_mutex,(pthread_mutexattr_t *)0);
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


void NetMutex::Unlock()
{
   pthread_mutex_unlock(&m_mutex);
   m_locked = 0;
}
