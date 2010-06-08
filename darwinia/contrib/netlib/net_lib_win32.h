#ifndef INCLUDED_NET_LIB_WIN32_H
#define INCLUDED_NET_LIB_WIN32_H


#include <winsock2.h>
#include <windows.h>


class NetUdpPacket;


typedef unsigned long (WINAPI *NetCallBack)(NetUdpPacket *data);
typedef unsigned long (WINAPI *NetThreadFunc)(void *ptr);


// Define portable names for Win32 functions
#define NetGetLastError() 			WSAGetLastError()
#define NetSleep(a) 				Sleep(a)
#define NetCloseSocket 				closesocket
#define NetGetHostByName 			gethostbyname // Should eventually be getaddrinfo (?)
#define NetSetSocketNonBlocking(a) 	ioctlsocket(a, FIONBIO, (unsigned long *)0x01)

// Define portable names for Win32 types
#define NetSocketLenType 			int
#define NetSocketHandle 			SOCKET
#define NetHostDetails				HOSTENT
#define NetThreadHandle 			HANDLE
#define NetCallBackRetType			unsigned long WINAPI
#define NetPollObject 				struct fd_set
#define NetMutexHandle 				CRITICAL_SECTION

// Define portable names for Win32 constants
#define NET_SOCKET_ERROR 			SOCKET_ERROR
#define NCSD_SEND 					SD_SEND
#define NCSD_READ 					SD_RECEIVE
#define NET_RECEIVE_FLAG 			0

// Define portable ways to test various conditions
#define NetIsAddrInUse 				(WSAGetLastError() == WSAEADDRINUSE)
#define NetIsSocketError(a) 		(a == SOCKET_ERROR)
#define NetIsBlockingError(a) 		((a == WSAEALREADY) || (a == WSAEWOULDBLOCK) || (a == WSAEINVAL))
#define NetIsConnected(a) 			(a == WSAEISCONN)
#define NetIsReset(a) 				((a == WSAECONNRESET) || (a == WSAESHUTDOWN))


#endif
