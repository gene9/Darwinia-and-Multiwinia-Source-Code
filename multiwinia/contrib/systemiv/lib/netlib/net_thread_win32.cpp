#include "lib/universal_include.h"

#include "net_thread.h"


NetRetCode NetStartThread(NetThreadFunc functionPtr, void *arg )
{
	NetRetCode retVal = NetOk;
	DWORD dwID = 0;
	
	if (CreateThread(NULL, NULL, functionPtr, arg, NULL, &dwID) == NULL)
	{
		NetDebugOut("Thread creation failed");
		retVal = NetFailed;
	}

	return retVal;
}

NetThreadId NetGetCurrentThreadId()
{
	return GetCurrentThreadId();
}
