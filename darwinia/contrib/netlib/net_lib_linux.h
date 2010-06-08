#ifndef INCLUDED_NET_LIB_LINUX_H
#define INCLUDED_NET_LIB_LINUX_H

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <arpa/inet.h>
#include <errno.h>
#ifdef HAVE_POLL
  #include <poll.h>
  // The following defines should be in <bits/poll.h>
  #ifndef POLLRDNORM
  #define POLLRDNORM 0x040
  #endif
  #ifndef POLLWRNORM
  #define POLLWRNORM 0x100
  #endif
  #ifndef POLLERR
  #define POLLERR    0x008
  #endif
#else
	#define USE_SELECT_READ_WRITE 1
  #endif
#endif
#include <fcntl.h>
#include <map.h>
#include <vector.h>
#include <iterator.h>
#include <pthread.h>


typedef void *(*NetCallBack)(void *ptr);
typedef void *(*NetThreadFunc)(void *ptr);


// Define portable names for Linux functions
#define NetCloseSocket					::close
#define NetSleep(a)						usleep(a*1000)
#define NetSetSocketNonBlocking(a)		fcntl(m_sockfd, F_SETFL, fcntl(a,F_GETFD) | O_NONBLOCK)

// Define portable names for Linux types
#define NetSocketLenType				socklen_t
#define NetSocketHandle					int
#define NetHostDetails					struct hostent
#define NetCallBackRetType				void *
#define NetThreadHandle					pthread_t
#define NetMutexHandle					pthread_mutex_t

// Define portable names for Linux constants
#define NET_SOCKET_ERROR 				-1
#define NCSD_SEND 						1
#define NCSD_READ 						0
#define NET_RECEIVE_FLAG				MSG_NOSIGNAL

#ifdef HAVE_POLL
  #define NetPollObject					pollfd
#else
  #define NetPollObject					fd_set
#endif

#ifdef HAVE_GETHOSTBYNAME
  #define NetGetHostByName gethostbyname
#endif

#ifndef ncRecvPtr
  #define ncRecvPtr void *
#endif

#ifndef ncAcceptLenType
  #define ncAcceptLenType(a) a
#endif

// Define portable ways to test various conditions
#define NetIsAddrInUse					(errno != EADDRINUSE)
#define NetIsSocketError(a)				(a == -1)
#define NetIsBlockingError(a)			((a == EALREADY) || (a == EINPROGRESS) || (a == EAGAIN))
#define NetIsConnected(a)				0
#define NetIsReset(a)					((a == EPIPE) || (a == ECONNRESET))


#endif
