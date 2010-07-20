// SendEmail.cpp  Version 1.0
//
// Author:  Hans Dietrich
//          hdietrich2@hotmail.com
//
// This software is released into the public domain.
// You are free to use it in any way you like, except
// that you may not sell this source code.
//
// This software is provided "as is" with no expressed
// or implied warranty.  I accept no liability for any
// damage or loss of business that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

// Notes on compiling:  this does not use MFC.  Set precompiled header
//                      option to "Not using precompiled headers".
//
// This code assumes that a default mail client has been set up.


#include <windows.h>
#include <crtdbg.h>
#include <io.h>
#include "strcvt.h"
#include "XTrace.h"
#include "GetFilePart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#  include <winsock.h>
#else
#  include <unistd.h>
#  include <errno.h>
#  include <netdb.h>
#  include <sys/types.h>
#  include <netinet/in.h>
#  include <sys/socket.h>
#endif

#pragma warning(disable:4127)	// for _ASSERTE

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

#define PORT 4537
#define HOST "crashreports.introversion.co.uk"

#define MAX_SEND 2048

BOOL SendEmail(HWND    hWnd,			// parent window, must not be NULL
			   LPCTSTR lpszAppMagicPrefix, // may not be NULL
			   LPCTSTR lpszAppVersion,	// may not be NULL
			   LPCTSTR lpszCrashID,		// may be NULL
			   LPCTSTR lpszSubject,		// may be NULL
			   LPCTSTR lpszMessage,		// may be NULL
			   LPCTSTR lpszAttachment)	// may be NULL
{
	TRACE(_T("in SendEmail\n"));
	printf( "in SendEmail\n" );


	// ===== SETUP THE WINSOCK LIBRARY =====

    WSADATA wsaData;   // if this doesn't work
    //WSAData wsaData; // then try this instead

    if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        exit(1);
    }


	// ===== SETUP THE SOCKET =====

    int sockfd;
    struct hostent *he;
    struct sockaddr_in their_addr; // connector's address information 

    if ((he = gethostbyname(HOST)) == NULL) {  // get the host info 
        //herror("gethostbyname");
        exit(1);
    }

	if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    their_addr.sin_family = AF_INET;    // host byte order 
    their_addr.sin_port = htons(PORT);  // short, network byte order 
    their_addr.sin_addr = *((struct in_addr *)he->h_addr);
    memset(&(their_addr.sin_zero), '\0', 8);  // zero the rest of the struct

	if (connect(sockfd, (struct sockaddr *)&their_addr,
                                          sizeof(struct sockaddr)) == -1) {
        perror("connect");
        exit(1);
    }


	// ===== SETUP FINISHED, READY TO SEND =====

	// some extra precautions are required to use MAPISendMail as it
	// tends to enable the parent window in between dialogs (after
	// the login dialog, but before the send note dialog).

	::SetCapture(hWnd);
	::SetFocus(NULL);
	::EnableWindow(hWnd, FALSE);

	char sendbuf[MAX_SEND];

	// Make the magic happen
	sprintf( sendbuf, "MAGIC %s%s\n", lpszAppMagicPrefix, lpszAppVersion );
	if ( send( sockfd, sendbuf, strlen( sendbuf ), 0 ) <= 0 ) {
      perror( "magic" );
	  exit(1);
	}

	// Send the crash ID
	if (NULL != lpszCrashID && ('\0' != lpszCrashID[0])) {
		sprintf( sendbuf, "IDENT %s\n", lpszCrashID );
		if ( send( sockfd, sendbuf, strlen( sendbuf ), 0 ) <= 0 ) {
	      perror( "ident" );
		  exit(1);
		}
	}

	// Send the user message
	if (NULL != lpszMessage && ('\0' != lpszMessage[0])) {
		sprintf( sendbuf, "MESSG %d\n", strlen(lpszMessage) );
		if ( send( sockfd, sendbuf, strlen( sendbuf ), 0 ) <= 0 ) {
	      perror( "messg head" );
		  exit(1);
		}
		if ( send( sockfd, lpszMessage, strlen( lpszMessage ), 0 ) <= 0 ) {
	      perror( "messg body" );
		  exit(1);
		}
	}

	//FILE * debugFile = fopen( "send_debug.txt", "wb" );

	// Send the crash report zip
	if ((NULL != lpszAttachment) && ('\0' != lpszAttachment[0])) {
		FILE * zipFile = fopen( lpszAttachment, "rb" );
		if (NULL != zipFile) {
			// Get file size
			long size;
			fseek ( zipFile, 0, SEEK_END );
			size = ftell( zipFile );
			rewind( zipFile );
			//fprintf( debugFile, "Zip size: %d, Pointer after rewind: %d\n", size, ftell( zipFile ) );

			// Send header
			sprintf( sendbuf, "CRASH %d\n", size );
			if ( send( sockfd, sendbuf, strlen( sendbuf ), 0 ) <= 0 ) {
				perror( "crash head" );
				exit(1);
			}
			//fprintf( debugFile, "Sent header: %s", sendbuf );

			// Send file contents
			int len; int sent;
			//long totalsent = 0;
			do {
				len = fread( sendbuf, 1, MAX_SEND, zipFile );
				if (len > 0) {
					if ( (sent = send( sockfd, sendbuf, len, 0 )) <= 0 ) {
						perror( "crash body" );
						//fprintf( debugFile, "Error: crash body send: %d read, %d error.\n", len, sent );
						exit(1);
					//} else {
					//	totalsent += sent;
					}
				}
			} while (len > 0);
			//fprintf( debugFile, "Total bytes sent: %d\n", totalsent );

			/*
			while( MAX_SEND == (len = fread( sendbuf, 1, MAX_SEND, zipFile )) ) {
				if ( send( sockfd, sendbuf, len, 0 ) <= 0 ) {
					perror( "crash body" );
					exit(1);
				}
			}
			if (len > 0) {
				if ( send( sockfd, sendbuf, len, 0 ) <= 0 ) {
					perror( "crash body" );
					exit(1);
				}
			}
			*/

			// Check for an error
			if (ferror( zipFile )) {
				perror( "crash read" );
				exit(1);
			}

			fclose( zipFile );
			//fclose( debugFile );

		}
	}

	// This just throws exceptions
	shutdown( sockfd, 1 );
	//close(sockfd);

	// ===== SEND COMPLETE, CLEAN UP =====

	// after returning from the MAPISendMail call, the window must be
	// re-enabled and focus returned to the frame to undo the workaround
	// done before the MAPI call.

	::ReleaseCapture();
	::EnableWindow(hWnd, TRUE);
	::SetActiveWindow(NULL);
	::SetActiveWindow(hWnd);
	::SetFocus(hWnd);

	BOOL bRet = TRUE;

	return bRet;
}
