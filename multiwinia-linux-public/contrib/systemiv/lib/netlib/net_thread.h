// ****************************************************************************
//  A platform independent thread implementation
// ****************************************************************************

#ifndef INCLUDED_NET_THREAD_H
#define INCLUDED_NET_THREAD_H

#include "net_lib.h"


NetThreadId NetGetCurrentThreadId();
NetRetCode NetStartThread(NetThreadFunc functionPointer, void *arg=NULL );
	

#endif
