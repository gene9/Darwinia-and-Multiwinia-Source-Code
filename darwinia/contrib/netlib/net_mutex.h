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
	
	void			Lock();
	void			Unlock();
	
	int 			IsLocked() { return m_locked; }
	
protected:
	int 			m_locked;
	NetMutexHandle 	m_mutex;
};


#endif
