#ifndef INCLUDED_NET_LIB_LINUX_H
#define INCLUDED_NET_LIB_LINUX_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>

class NetUdpPacket;

typedef void * (*NetCallBack)(NetUdpPacket *data);
typedef void * (*NetThreadFunc)(void *ptr);

// Define portable names for Linux functions
#define NetGetLastError() 			errno
#define NetSleep(a) 				usleep(a*1000)
#define NetCloseSocket 				::close
#define NetGetHostByName 			gethostbyname // Should eventually be getaddrinfo (?)
#define NetSetSocketNonBlocking(a) 	fcntl(m_sockfd, F_SETFL, fcntl(a,F_GETFD) | O_NONBLOCK)

// Define portable names for Linux types
#define NetSocketLenType				socklen_t
#define NetSocketHandle					int
#define NetHostDetails					struct hostent
#define NetThreadHandle					pthread_t
#define NetCallBackRetType				void *
#define NetPollObject 				    fd_set
#define NetMutexHandle					pthread_mutex_t
#define NetSemaphoreHandle				sem_t
#define NetThreadId					    pthread_t


struct NetEventHandle {
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool signalled;
};

// Define portable names for Linux constants
#define NET_SOCKET_ERROR 				-1
#define NCSD_SEND 						1
#define NCSD_READ 						0


// Define portable ways to test various conditions
#define NetIsAddrInUse					(errno != EADDRINUSE)
#define NetIsSocketError(a)				(a == -1)
#define NetIsBlockingError(a)			((a == EALREADY) || (a == EINPROGRESS) || (a == EAGAIN))
#define NetIsConnected(a)				0
#define NetIsReset(a)					((a == EPIPE) || (a == ECONNRESET))

#endif
