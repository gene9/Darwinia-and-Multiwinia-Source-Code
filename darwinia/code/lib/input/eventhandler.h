#ifndef INCLUDED_EVENTHANDLER_H
#define INCLUDED_EVENTHANDLER_H


class EventHandler {

public:
	virtual bool WindowHasFocus() = 0;

};


extern EventHandler * g_eventHandler;


#endif // INCLUDED_EVENTHANDLER_H
