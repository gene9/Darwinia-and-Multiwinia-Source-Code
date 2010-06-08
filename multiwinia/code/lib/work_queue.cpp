#include "lib/universal_include.h"
#include "work_queue.h"

WorkQueue::WorkQueue()
{
	NetStartThread( (NetThreadFunc) &WorkQueue::RunThread, this );
}

WorkQueue::~WorkQueue()
{
	AppDebugOut("Waiting for work queue to finish work.\n");
	
	m_quit.Signal();
	m_event.Signal();

	m_finished.Wait( -1 );
	AppDebugOut("Done.\n");
}

bool WorkQueue::Empty() const
{
	NetLockMutex lock( m_lock );
	return m_queue.Size() == 0;
}

void WorkQueue::Wait()
{
	while( !Empty() )
	{
		m_doneJob.Wait( -1 );
	}
}

void StandardiseCPUPrecision(); // TODO: put this in a header somewhere

void WorkQueue::RunThread( WorkQueue *_this )
{
	// the FPU precision flags are maintained on a per-thread basis by the OS, so we
	// need to do this for every thread we create that does simulation work.
	StandardiseCPUPrecision();
	
	_this->Run();
}

void WorkQueue::Run()
{
	while( true )
	{
		while( Empty() )
		{
			m_event.Wait( -1 );
			if( m_quit.Wait( 0 ) )
			{
				m_finished.Signal();
				return;
			}
		}

		Runnable *item = NULL;
		{
			NetLockMutex lock( m_lock );
			item = m_queue.GetData( 0 );
		}

		// AppDebugOut("Work Queue: running item: %p\n", item );
		item->Run();

		{
			NetLockMutex lock( m_lock );
			m_queue.RemoveData( 0 );
		}

		delete item;
		m_doneJob.Signal();
	}
}
