#ifndef __WORK_QUEUE_H
#define __WORK_QUEUE_H

#include "lib/tosser/llist.h"
#include "lib/netlib/net_mutex.h"
#include "lib/runnable.h"

class WorkQueue
{
public:
	WorkQueue();
	~WorkQueue();

	template <class T>
	void Add( void (T::*_func)(), T *_this )
	{
		NetLockMutex lock( m_lock );
		m_queue.PutDataAtEnd( new Method<T>( _func, _this ) );
		m_event.Signal();
	}

	void Add( void (*_func)() )
	{
		NetLockMutex lock( m_lock );
		m_queue.PutDataAtEnd( new Function( _func ) );
		m_event.Signal();
	}

	void Wait();

	bool Empty() const;

private:

	static void RunThread( WorkQueue *_this );
	void Run();
	

	mutable NetMutex m_lock;
	NetEvent m_event, m_quit, m_finished, m_doneJob;

	LList<Runnable *> m_queue;
};



#endif