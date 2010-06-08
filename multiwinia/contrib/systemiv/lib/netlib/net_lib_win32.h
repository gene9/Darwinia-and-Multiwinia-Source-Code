#ifndef INCLUDED_NET_LIB_WIN32_H
#define INCLUDED_NET_LIB_WIN32_H


#define NOMINMAX

#include <winsock2.h>
#include <windows.h>
#include <Ws2tcpip.h>
#include <wspiapi.h>

class NetUdpPacket;


typedef unsigned long (WINAPI *NetCallBack)(NetUdpPacket *data);
typedef unsigned long (WINAPI *NetThreadFunc)(void *ptr);


// Define portable names for Win32 functions
#define NetGetLastError() 			WSAGetLastError()
#define NetSleep(a) 				Sleep(a)
#define NetCloseSocket 				closesocket
#define NetGetHostByName 			gethostbyname // Should eventually be getaddrinfo (?)
#define NetSetSocketNonBlocking(a) 	{ unsigned long val = 1; ioctlsocket(a, FIONBIO, &val); }

// Define portable names for Win32 types
#define NetSocketLenType 			int
#define NetSocketHandle 			SOCKET
#define NetHostDetails				HOSTENT
#define NetThreadHandle 			HANDLE
#define NetThreadId					DWORD
#define NetCallBackRetType			unsigned long WINAPI
#define NetPollObject 				struct fd_set
#define NetMutexHandle 				CRITICAL_SECTION
#define NetEventHandle				HANDLE
#define NetSemaphoreHandle			HANDLE

// Define portable names for Win32 constants
#define NET_SOCKET_ERROR 			SOCKET_ERROR
#define NCSD_SEND 					SD_SEND
#define NCSD_READ 					SD_RECEIVE

// Define portable ways to test various conditions
#define NetIsAddrInUse 				(WSAGetLastError() == WSAEADDRINUSE)
#define NetIsSocketError(a) 		(a == SOCKET_ERROR)
#define NetIsBlockingError(a) 		((a == WSAEALREADY) || (a == WSAEWOULDBLOCK) || (a == WSAEINVAL))
#define NetIsConnected(a) 			(a == WSAEISCONN)
#define NetIsReset(a) 				((a == WSAECONNRESET) || (a == WSAESHUTDOWN))


#endif
