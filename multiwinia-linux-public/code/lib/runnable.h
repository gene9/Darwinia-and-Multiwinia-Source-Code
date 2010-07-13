#ifndef __RUNNABLE_H
#define __RUNNABLE_H

#include "lib/netlib/net_lib.h"

class Runnable
{
public:
	virtual ~Runnable();	
	virtual void Run() const = 0;
};

class NoOp : public Runnable
{
public:
	void Run() const;
};

class Function : public Runnable
{
public:
	Function( void (*_func)() )
		: m_func( _func )
	{
	}

	void Run() const
	{
		m_func();
	}

private:

	void (*m_func)();
};

template <class T> 
struct Method : public Runnable
{
	Method( void (T::*_func)(), T *_this )
	:	m_func( _func ),
		m_this( _this )
	{
	}

	void Run() const
	{
		(m_this->*m_func)();
	}

private:

	void (T::*m_func)();
	T * m_this;
};


template <class T, class A1> 
struct Method1 : public Runnable
{
	Method1( void (T::*_func)( A1 ), T *_this, A1 _arg )
	:	m_func( _func ),
		m_this( _this ),
		m_arg( _arg )
	{
	}

	void Run() const
	{
		(m_this->*m_func)( m_arg );
	}

private:

	void (T::*m_func)( A1 );
	T * m_this;
	A1 m_arg;
};

template <class T, class A1>
inline
Method1<T, A1> *method1( void (T::*_func)( A1 ), T *_this, A1 _arg )
{
	return new Method1< T, A1 >( _func, _this, _arg );
}


class Job
{
public:
	
	virtual ~Job();
	void Process();
	void Wait();

protected:
	virtual void Run() = 0;

private:

	NetEvent			m_event;
};


class RunnableJob : public Job
{
public:
	RunnableJob( Runnable *_r );

protected:
	void Run();

private:
	Runnable *m_r;
};

class DelayedJob {
public:
	DelayedJob( Runnable *_runnable, int _counter );

	~DelayedJob();


	bool ReadyToRun(); // decrements counter
	void Run();

private: 
	Runnable *m_runnable;
	int m_counter;

};



#endif
