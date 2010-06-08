#include "lib/universal_include.h"
#include "lib/runnable.h"

Runnable::~Runnable()
{
}

Job::~Job()
{
}

void NoOp::Run() const
{
}

void Job::Process()
{
	Run();
	m_event.Signal();
}

void Job::Wait()
{
	m_event.Wait( -1 );
}

RunnableJob::RunnableJob( Runnable *_r )
:	m_r( _r )
{
}

void RunnableJob::Run() 
{
	m_r->Run();
}

DelayedJob::DelayedJob( Runnable *_runnable, int _counter )
: m_runnable(_runnable), m_counter(_counter)
{
}

DelayedJob::~DelayedJob()
{ 
	delete m_runnable;
	m_runnable = NULL;
};

bool DelayedJob::ReadyToRun() // decrements counter
{
	if( m_counter > 0 )
	{
		m_counter--;
		return false;
	}
	else
	{
		return true;
	}
}

void DelayedJob::Run()
{
	m_runnable->Run();
}