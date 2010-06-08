#ifndef __LISTENER_H
#define __LISTENER_H

#include "lib/tosser/llist.h"

class Listener {
public:
	virtual ~Listener();
	virtual void Notify() = 0;
};

class EventListener : public Listener {
public:
	EventListener();
	void Notify();
	operator bool() const;

private:
	bool m_triggered;
};


class ListenerList : public Listener {
public:
	~ListenerList();

	void Add( Listener *_e );
	void Remove( Listener *_e );

	void Notify();

private:
	LList< Listener *> m_listeners;
};


#endif // __LISTENER_H