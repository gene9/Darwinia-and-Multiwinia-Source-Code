#ifndef INCLUDED_NET_LIB_APPLE_H
#define INCLUDED_NET_LIB_APPLE_H


// Treat Mac OS X like Linux
#include <sys/types.h>
#include <sys/socket.h>   
#include <netinet/in.h>
#define USE_LINUX
#define HAVE_SOCKET
#define HAVE_SELECT
#define HAVE_GETHOSTBYNAME
#define NET_RECEIVE_FLAG 0


#endif
