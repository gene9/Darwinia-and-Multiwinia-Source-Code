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

#include "lib/debug_utils.h"

#define NetDebugOut AppDebugOut

#define MAX_HOSTNAME_LEN   	256
#define MAX_PACKET_SIZE  	1500


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


#include "net_mutex.h"
#include "net_socket.h"
#include "net_socket_listener.h"
#include "net_socket_session.h"
#include "net_thread.h"
#include "net_udp_packet.h"


class NetLib
{
public:
	NetLib();
	~NetLib();

	bool Initialise(); // Returns false on failure
};


int GetLocalHost        ();

void GetLocalHostIP     ( char *str, int len );
void IpAddressToStr     ( const in_addr &address, char *str);
int  StrToIpAddress     ( const char *str );

#endif
