#include "lib/universal_include.h"
#include "listener.h"
#include "lib/debug_utils.h"

// Listener

Listener::~Listener()
{
}

// EventListener

EventListener::EventListener()
:	m_triggered( false )
{
}

void EventListener::Notify()
{
	m_triggered = true;
}

EventListener::operator bool() const
{
	return m_triggered;
}

// ListenerList

ListenerList::~ListenerList()
{
	m_listeners.Empty();
}

void ListenerList::Add( Listener *_e )
{
	m_listeners.PutData( _e );
}

void ListenerList::Remove( Listener *_e )
{
	int index = m_listeners.FindData( _e );
	if( index >= 0 )
		m_listeners.RemoveData( index );
}

void ListenerList::Notify()
{
	int numListeners = m_listeners.Size();
	for( int i = 0; i < numListeners; i++ )
		m_listeners[i]->Notify();
}

