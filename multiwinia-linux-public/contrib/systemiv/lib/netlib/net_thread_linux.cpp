#include "net_thread.h"

NetRetCode NetStartThread(NetThreadFunc functionPtr, void *arg)
{
	pthread_t t;

	if (pthread_create(&t,NULL,functionPtr,arg) != 0) {
		NetDebugOut("thread creation failed");
		return NetFailed;
	}
	
	// detach the thread now, because we are not interested
	// in it's return result. We want to reclaim the memory
	// as soon as the thread terminates.
	
	pthread_detach(t);

	
	return NetOk;
}

NetThreadId NetGetCurrentThreadId()
{
	return pthread_self();
}
