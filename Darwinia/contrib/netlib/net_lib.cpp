// See header file for description of this library

#include <stdarg.h>
#include <stdio.h>

#include "net_lib.h"


void NetDebugOut(char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
#ifdef WIN32
    OutputDebugStringA(buf);
#else
	fprintf(stderr, "%s\n", buf);
#endif
}



// ****************************************************************************
// Class NetLib
// ****************************************************************************

NetLib::NetLib()
{
}


NetLib::~NetLib()
{
#ifdef WIN32
    WSACleanup();
#endif
}


bool NetLib::Initialise()
{
#ifdef WIN32
    WORD versionRequested;
    WSADATA wsaData;
    
    versionRequested = MAKEWORD(2, 2);
 
    if (WSAStartup(versionRequested, &wsaData))
    {
        NetDebugOut("WinSock startup failed");
        return false;
    }
 
    // Confirm that the WinSock DLL supports 2.2. Note that if the DLL supports
	// versions greater than 2.2 in addition to 2.2, it will still return
    // 2.2 in wVersion since that is the version we requested.                                       
    if ((LOBYTE(wsaData.wVersion) != 2) || (HIBYTE(wsaData.wVersion) != 2))
    {
        // Tell the user that we could not find a usable WinSock DLL
        WSACleanup();
        NetDebugOut("No valid WinSock DLL found");
        return false; 
    }
#endif

    return true;
}
