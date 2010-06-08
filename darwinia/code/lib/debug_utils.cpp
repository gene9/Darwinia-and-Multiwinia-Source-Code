#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <crtdbg.h>
#include "debug_utils.h"

#include "app.h"


void DebugOut(char *_fmt, ...)
{
    char buf[2048];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
    OutputDebugString(buf);
}


void DarwiniaReleaseAssert(bool _condition, char const *_fmt, ...)
{
	if (!_condition)
	{
		char buf[512];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);

		DWORD rc = GetLastError();
		if (rc != ERROR_SUCCESS) {
			LPVOID lpMsgBuf;

			FormatMessage(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | 
				FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				rc,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				(LPTSTR) &lpMsgBuf,
				0, NULL );
 
			sprintf( buf + strlen(buf), "\nLast error: %s (%d)", lpMsgBuf, rc );
		}
		ShowCursor(true);
		MessageBox(NULL, buf, "Fatal Error", MB_OK);
        GenerateBlackBox( buf );
#ifndef _DEBUG
		exit(-1);
#else
		_ASSERT(_condition);
#endif
	}
}


int g_newReportingThreshold = -1; // All memory allocations larger than this will be reported


void SetNewReportingThreshold(int _size)
{
	g_newReportingThreshold = _size;
}


#ifdef OVERLOADED_NEW
#undef new
void *operator new (unsigned _size, char const *_filename, int _line)
{
	void *p = malloc(_size);

	if ((signed)_size > g_newReportingThreshold)
	{
		DebugOut("%s line %d: %d bytes at 0x%0x\n", _filename, _line, _size, p);
	}
	DarwiniaDebugAssert(p);

	return p;
}


void *operator new[] (unsigned _size, char const *_filename, int _line)
{
	void *p = malloc(_size);

	if ((signed)_size > g_newReportingThreshold)
	{
		DebugOut("%s line %d: %d bytes at 0x%0x\n", _filename, _line, _size, p);
	}
	DarwiniaDebugAssert(p);

	return p;
}


void operator delete (void *p, char const *_filename, int _line)
{
	free(p);
}


void operator delete[] (void *p, char const *_filename, int _line)
{
	free(p);
}


#endif // #ifdef OVERLOADED_NEW


void PrintMemoryLeaks ()
{
#ifdef TRACK_MEMORY_LEAKS
    char filename[] = "memory.log";

    OFSTRUCT ofstruct;
    HFILE file = OpenFile ( filename,
                            &ofstruct,
                            OF_CREATE );
                     
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, (void *) file);
  
    _CrtDumpMemoryLeaks ();

    _lclose ( file );
#endif
}


unsigned *getRetAddress(unsigned *mBP)
{
#ifdef WIN32
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


void GenerateBlackBox( char *_msg )
{
    FILE *_file = fopen( "blackbox.txt", "wt" );

    fprintf( _file, "=========================\n" );
    fprintf( _file, "DARWINIA BLACK BOX REPORT\n" );
    fprintf( _file, "=========================\n\n" );

    fprintf( _file, "VERSION : %s\n", DARWINIA_VERSION_STRING );

    if( _msg ) fprintf( _file, "ERROR   : '%s'\n", _msg );


    //
    // Print preferences information

    FILE *prefsFile = fopen(App::GetPreferencesPath(), "r");
    if( prefsFile )
    {
        fprintf( _file, "\n" );
        fprintf( _file, "=========================\n" );
        fprintf( _file, "====== PREFERENCES ======\n" );
        fprintf( _file, "=========================\n\n" );

        char line[256];
	    while (fgets(line, 256, prefsFile) != NULL)
	    {
		    if (line[0] != '#' && line[0] != '\n' )				// Skip comment lines
		    {
                fprintf( _file, "%s", line );              
		    }
	    }
		fclose(prefsFile);
    }
    
    //
    // Print stack trace
	// Get our frame pointer, chain upwards

    fprintf( _file, "\n" );
    fprintf( _file, "=========================\n" );
    fprintf( _file, "====== STACKTRACE =======\n" );
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
	
    fclose( _file );
}
