#include "lib/universal_include.h"
#include "lib/input/xbox_eventhandler.h"

bool g_windowHasFocus = true;

XboxEventHandler::XboxEventHandler()
{
}

bool XboxEventHandler::WindowHasFocus() 
{
	return g_windowHasFocus;
}

XboxEventHandler *getXboxEventHandler()
{
	EventHandler *handler = g_eventHandler;
	return dynamic_cast<XboxEventHandler *>( handler );
}
