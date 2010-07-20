// ****************************************************************************
//  Top level include file for NetLib
//
//  NetLib - A very thin portable UDP network library
// ****************************************************************************

#ifndef INCLUDED_NET_LIB_H
#define INCLUDED_NET_LIB_H


#ifdef __APPLE__
 #include "net_lib_apple.h"
#endif

#ifdef WIN32
 #include "net_lib_win32.h"
#endif

#if (defined __linux__) 
 #include "net_lib_linux.h"
#endif

#if (!defined MIN)
#define MIN(a,b) ((a < b) ? a : b)
#endif


void NetDebugOut(char *fmt, ...);


#define MAX_HOSTNAME_LEN   	256
#define MAX_PACKET_SIZE  	512


typedef struct sockaddr_in NetIpAddress;


enum NetRetCode
{
   NetFailed = -1,
   NetOk,
   NetTimedout,
   NetBadArgs,
   NetMoreData,
   NetClientDisconnect,
   NetNotSupported
};


class NetLib
{
public:
	NetLib();
	~NetLib();

	bool Initialise(); // Returns false on failure
};


#endif
