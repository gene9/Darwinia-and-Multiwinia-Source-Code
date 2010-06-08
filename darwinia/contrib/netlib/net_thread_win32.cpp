// See net_thread.h for description of module

#include "net_thread.h"


NetRetCode NetStartThread(NetThreadFunc functionPtr)
{
	NetRetCode retVal = NetOk;
	DWORD dwID = 0;
	
	if (CreateThread(NULL, NULL, functionPtr, NULL, NULL, &dwID) == NULL)
	{
		NetDebugOut("Thread creation failed");
		retVal = NetFailed;
	}

	return retVal;
}
