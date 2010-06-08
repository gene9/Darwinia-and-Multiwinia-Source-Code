#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#ifdef WIN32
#include <crtdbg.h>
#include <windows.h>
#elif TARGET_OS_MACOSX
#include <CoreFoundation/CoreFoundation.h>
#include <sys/stat.h>
#endif

#include "lib/debug_utils.h"
#include "lib/string_utils.h"
#include "lib/filesys/filesys_utils.h"
#ifndef NO_DEBUG_OUTPUT
#  include "lib/netlib/net_lib.h"
#endif // NO_DEBUG_OUTPUT

#include "lib/math/vector3.h"
#include "lib/math/matrix34.h"

#ifdef _WIN32
#pragma warning( disable : 4996 )
#endif

static char *s_AppDebugOutRedirect = NULL;
static char *s_AppDebugLogPathPrefix = NULL;

//
// Returns a new string containing the full path name for the log file _filename, inside the
// appropriate directory.
//
char *AppDebugLogPath(const char *_filename)
{
#if TARGET_OS_MACOSX
	// Put the log file in ~/Library/Logs/<application name>/ if possible, falling back to /tmp
	CFStringRef appName, dir, path;
	char *utf8Path;
	int maxUTF8PathLen;
	
	appName = (CFStringRef)CFDictionaryGetValue(CFBundleGetInfoDictionary(CFBundleGetMainBundle()),
												kCFBundleNameKey);
	if (appName && getenv("HOME"))
	{
		char *utf8Dir;
		int maxUTF8DirLen;
		struct stat dirInfo;
		
		dir = CFStringCreateWithFormat(NULL, NULL, CFSTR("%s/Library/Logs/%@"), getenv("HOME"), appName);
		maxUTF8DirLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(dir), kCFStringEncodingUTF8);
		utf8Dir = new char[maxUTF8DirLen + 1];
		CFStringGetCString(dir, utf8Dir, maxUTF8DirLen, kCFStringEncodingUTF8);
		
		// Make the directory in case it doesn't exist
		CreateDirectoryRecursively(utf8Dir);
			
		delete utf8Dir;
	}
	else
		dir = CFSTR("/tmp");
		
	path = CFStringCreateWithFormat(NULL, NULL, CFSTR("%@/%s"), dir, _filename);
	maxUTF8PathLen = CFStringGetMaximumSizeForEncoding(CFStringGetLength(path), kCFStringEncodingUTF8);
	utf8Path = new char[maxUTF8PathLen + 1];
	CFStringGetCString(path, utf8Path, maxUTF8PathLen, kCFStringEncodingUTF8);
	
	CFRelease(dir);
	CFRelease(path);
	return utf8Path;
	
#elif TARGET_OS_LINUX
	// Just throw it in /tmp
	char *path = new char[strlen(_filename) + sizeof("/tmp/") + 1];
	
	strcpy(path, "/tmp/");
	strcat(path, _filename);
	return path;
#else
	if( s_AppDebugLogPathPrefix )
	{
		char *path = new char[strlen(_filename) + strlen(s_AppDebugLogPathPrefix) + 1];

		strcpy(path, s_AppDebugLogPathPrefix);
		strcat(path, _filename);
		return path;
	}
	return newStr( _filename );
#endif
}

void AppDebugLogPathPrefix(const char *_filename)
{
	if ( s_AppDebugLogPathPrefix )
	{
		delete [] s_AppDebugLogPathPrefix;
	}
	s_AppDebugLogPathPrefix = newStr( _filename );
}


#ifndef WIN32
inline
void OutputDebugString( const char *s )
{
	fputs( s, stderr );
}
#endif


void AppDebugOutRedirect(const char *_filename)
{
	// re-using same log file is a no-op
    if ( !s_AppDebugOutRedirect || strcmp(s_AppDebugOutRedirect, AppDebugLogPath(_filename)) != 0 )
	{
		s_AppDebugOutRedirect = AppDebugLogPath(_filename);

		// Check we have write access, and clear the file

		FILE *file = fopen(s_AppDebugOutRedirect, "w" );

		if( !file )
		{
			delete s_AppDebugOutRedirect;
			s_AppDebugOutRedirect = NULL;

			AppDebugOut( "Failed to open %s for writing\n", _filename );
		}
		else
		{
			fclose( file );
		}
	}
}

#ifndef NO_DEBUG_OUTPUT

static NetMutex s_appDebugOutMutex;

void AppDebugOut( const char *_msg, ... )
{
    char buf[10240];
    va_list ap;
    va_start (ap, _msg);
    vsprintf(buf, _msg, ap);
	
	if( s_AppDebugOutRedirect )
    {
		NetLockMutex lock( s_appDebugOutMutex );

        FILE *file = fopen(s_AppDebugOutRedirect, "a" );
        if( !file )
        {
			// recursion without termination is not good for stack :)
			// I changed order so that AppDebugOut goes last
			delete s_AppDebugOutRedirect;
			s_AppDebugOutRedirect = NULL;
			OutputDebugString(buf);
            AppDebugOut( "Failed to open %s for writing\n", s_AppDebugOutRedirect );
        }
        else
        {
            fprintf( file, "%s", buf );
            fclose( file );
        }
    }
    else
    {
        OutputDebugString(buf);
    }
}

void AppDebugOutData(const void *_data, int _dataLen)
{
    for( int i = 0; i < _dataLen; ++i )
    {
        if( i % 16 == 0 )
        {
            AppDebugOut( "\n" );
        }
        AppDebugOut( "%02x ", ((const unsigned char *)_data)[i] );
    }    
    AppDebugOut( "\n\n" );
}

void AppDebugOutData(const int *_data, int _dataLen)
{
    for( int i = 0; i < _dataLen; ++i )
    {
        if( i % 16 == 0 )
        {
            AppDebugOut( "\n" );
        }
        AppDebugOut( "%d ", _data[i] );
    }    
    AppDebugOut( "\n\n" );
}
#endif // NO_DEBUG_OUTPUT

#if !defined( NO_DEBUG_ASSERT ) && !defined( AppDebugAssert )
void AppDebugAssert(bool _condition)
{
    if(!_condition)
    {
#ifdef WIN32
		ShowCursor(true);
		_ASSERT(_condition); 
#else
		abort();
#endif  
    }
}
#endif // NO_DEBUG_ASSERT


void AppReleaseAssertFailed(char const *_msg, ...)
{
	char buf[512];
	va_list ap;
	va_start (ap, _msg);
	vsprintf(buf, _msg, ap);

#if defined(WIN32)
	ShowCursor(true);
	MessageBox(NULL, buf, "Fatal Error", MB_OK);
#else
	fputs(buf, stderr);
#endif

#ifndef _DEBUG
	AppGenerateBlackBox("blackbox.txt",buf);
	throw;
	//exit(-1);
#else
#ifdef WIN32
	_ASSERT(false);
#else
	abort();
#endif
#endif // _DEBUG
}


void AppReleaseExit(char const *_msg, ...)
{
	char buf[512];
	va_list ap;
	va_start (ap, _msg);
	vsprintf(buf, _msg, ap);

	AppDebugOut("%s\n", buf );
#if defined(WIN32)
	ShowCursor(true);
	MessageBox(NULL, buf, "Exit", MB_OK);
#else
	fputs(buf, stderr);
#endif

#ifndef _DEBUG
	AppGenerateBlackBox("blackbox.txt",buf);
	exit(-1);
#else
#ifdef WIN32
	_ASSERT(false);
#else
	abort();
#endif
#endif // _DEBUG
}


unsigned *getRetAddress(unsigned *mBP)
{
#if defined(WIN32)
	unsigned *retAddr;

	__asm {
		mov eax, [mBP]
		mov eax, ss:[eax+4];
		mov [retAddr], eax
	}

	return retAddr;
#else
	unsigned **p = (unsigned **) mBP;
	return p[1];
#endif
}


void AppGenerateBlackBox( const char *_filename, const char *_msg )
{
	char *path = AppDebugLogPath( _filename );
    FILE *_file = fopen( path, "wt" );
    if( _file )
    {
        fprintf( _file, "=========================\n" );
        fprintf( _file, "=   BLACK BOX REPORT    =\n" );
        fprintf( _file, "=========================\n\n" );

        fprintf( _file, "%s %s built %s\n", APP_NAME, APP_VERSION, __DATE__  );

        time_t timet = time(NULL);
        tm *thetime = localtime(&timet);
        fprintf( _file, "Date %d:%d, %d/%d/%d\n\n", thetime->tm_hour, thetime->tm_min, thetime->tm_mday, thetime->tm_mon+1, thetime->tm_year+1900 );

        if( _msg ) fprintf( _file, "ERROR : '%s'\n", _msg );

		// For MacOSX, suggest Smart Crash Reports: http://smartcrashreports.com/
		
#if !defined(TARGET_OS_MACOSX)
        //
        // Print stack trace
	    // Get our frame pointer, chain upwards

        fprintf( _file, "\n" );
        fprintf( _file, "=========================\n" );
        fprintf( _file, "=      STACKTRACE       =\n" );
        fprintf( _file, "=========================\n\n" );

	    unsigned *framePtr;
        unsigned *previousFramePtr = NULL;

    
#ifdef WIN32
	    __asm { 
		    mov [framePtr], ebp
	    }
#else
	    asm (
	        "movl %%ebp, %0;"
	        :"=r"(framePtr)
	        );
#endif
	    while(framePtr) {
		                    
		    fprintf(_file, "retAddress = %p\n", getRetAddress(framePtr));
		    framePtr = *(unsigned **)framePtr;

	        // Frame pointer must be aligned on a
	        // DWORD boundary.  Bail if not so.
#ifdef _WIN32
#pragma warning( suppress : 4311 ) // warning cast pointer to unsigned long
#endif
			
	        if ( (unsigned long) framePtr & 3 )   
		    break;                    

            if ( framePtr <= previousFramePtr )
                break;

            // Can two DWORDs be read from the supposed frame address?          
#ifdef WIN32
            if ( IsBadWritePtr(framePtr, sizeof(PVOID)*2) ||
                 IsBadReadPtr(framePtr, sizeof(PVOID)*2) )
                break;
#endif

            previousFramePtr = framePtr;
    
        }
#endif // TARGET_OS_MACOSX
	    
        fclose( _file );
    }
	delete path;
}

// Requires 17 bytes (with NUL)
char *HashDouble( double value, char *buffer )
{
    union
    {
        double doublePart;
        unsigned long long intPart;
    } v;

    v.doublePart = value;
    unsigned long long i = v.intPart;

    for( int j = 0; j < 16; ++j )
    {
        unsigned int val = i & 0xF;
        if( val <= 9 )
        {
            buffer[j] = '0' + val;
        }
        else
        {
            buffer[j] = 'a' + (val - 10);
        }
        i >>= 4;
    }
    buffer[16] = '\0';

    return buffer;
};

// Requires 53 bytes (with NUL)
char *HashVector3( const Vector3 &value, char *buffer )
{
	char *s = buffer;

	*s++ = '(';
	HashDouble( value.x, s ); s+=16;
	*s++ = ',';
	HashDouble( value.y, s ); s+=16;
	*s++ = ',';
	HashDouble( value.z, s ); s+=16;
	*s++ = ')';
	*s++ = '\0';

	return buffer;
}

// Requires 214 bytes (with NUL)
char *HashMatrix34( const Matrix34 &value, char *buffer )
{
	char *s = buffer;

	*s++ = '(';
	HashVector3( value.r, s ); s += 16*3+4;
	*s++ = ',';
	HashVector3( value.u, s ); s += 16*3+4;
	*s++ = ',';
	HashVector3( value.f, s ); s += 16*3+4;
	*s++ = ',';
	HashVector3( value.pos, s ); s += 16*3+4;
	*s++ = ')';
	*s++ = '\0';

	return buffer;
}