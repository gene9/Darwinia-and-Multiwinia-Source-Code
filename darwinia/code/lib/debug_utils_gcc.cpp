#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ucontext.h>

#define AudioInfo CarbonAudioInfo
#include <Carbon/Carbon.h>
#undef AudioInfo

#include "lib/debug_utils.h"
#include "app.h"

#ifndef NO_WINDOW_MANAGER
#include "lib/window_manager.h"
#include <SDL/SDL.h>
#endif

void DebugOut(char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
    fprintf(stderr, "%s\n", buf);
}

static void fatalSignal(int signum, struct __siginfo *siginfo, void *arg)
{
	// Wait for the CrashReporter to make a crash log
	sleep(1);	
	// And now hopefully we can save it.
	DarwiniaReleaseAssert(false, "Fatal Signal (%d) Received\n", signum);
}

static const char *getCrashLogFilename()
{
	static char *crashLogFilename = NULL;
	
	if (crashLogFilename) 
		return crashLogFilename;
		
 	char *home = getenv("HOME");
	if (home == NULL) 
		return NULL;
		
	crashLogFilename = new char[strlen(home) + 256];
	sprintf(crashLogFilename, "%s/Library/Logs/CrashReporter/Darwinia.crash.log", home);
	return crashLogFilename;
}

static void copyCrashLog(FILE *_file)
{
	// Copy the Crash Log if it exists
	const char *crashLog = getCrashLogFilename();
	if (crashLog && access(crashLog, F_OK) == 0) {
		FILE *in = fopen(crashLog, "r");
		do {
			int ch = fgetc(in);
			if (ch == EOF)
				return;
			fputc(ch, _file);
		} while (true);
		fclose(in);
		fprintf(_file, "\n");
	}
}

static void hupSignal(int signum)
{
	// Display last log messages
	void DisplayLog();
	DisplayLog();
}

void SetupMemoryAccessHandlers()
{
	struct sigaction sa;
	sa.sa_sigaction = fatalSignal;
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	
	sigemptyset(&sa.sa_mask);
	sigaction(SIGSEGV, &sa, 0);
	sigaction(SIGBUS, &sa, 0);
	
	// Delete the crash report log
	const char *crashLog = getCrashLogFilename();
	if (crashLog) 
		unlink(crashLog);

	// Set up a SIGHUP handler in case the game pauses for some reason
	sa.sa_sigaction = hupSignal;
	sa.sa_flags = 0;
	sigaction(SIGHUP, &sa, 0);
}


void DarwiniaReleaseAssert(bool _condition, char const *_fmt, ...)
{
	if (!_condition)
	{
		char buf[512];
		va_list ap;
		va_start (ap, _fmt);
		vsprintf(buf, _fmt, ap);
        fprintf(stderr, "%s\n", buf);
		GenerateBlackBox(buf);
		
		Str255 title;
		
		char descriptionBuffer[512];
		Str255 description;
		
		CopyCStringToPascal("Fatal Error", title);
		
		sprintf(descriptionBuffer, 
		  "%s\n"
		   "\n"
		  "Darwinia has unexpectedly encountered a fatal error.\n"
		  "A full description of the error can be found in the file\n"
		  "blackbox.txt in your home directory",
		  buf);
		  
		CopyCStringToPascal(descriptionBuffer, description);
		
		SInt16 buttonHit;	

#ifndef NO_WINDOW_MANAGER		
		g_windowManager->UnhideMousePointer();
		SDL_Quit();
#endif
					
		StandardAlert(kAlertStopAlert, title, description, NULL, &buttonHit);
		
		exit(-1);
	}
}


void PrintMemoryLeaks()
{
}

#ifdef TARGET_OS_MACOSX
static void PrintStackTraceMacOSX(FILE *_file)
{
	register unsigned *startStackPtr asm("r1");
	unsigned *stackPtr;
	unsigned *previousStackPtr;
	
	stackPtr = startStackPtr;
	
	// There is no useful information in this frame, so skip it.
	stackPtr = *(unsigned **)stackPtr;		

	while(stackPtr) {
		unsigned *retAddress = ((unsigned **) stackPtr)[2];		
		fprintf(_file, "retAddress = %p\n", retAddress);

		previousStackPtr = stackPtr;
		stackPtr = *(unsigned **)stackPtr;		

		// Frame pointer must be aligned on a
		// DWORD boundary.  Bail if not so.
		if ( (unsigned long) stackPtr & 3 )   
			break;                    

		if ( stackPtr <= previousStackPtr )
			break;
	}
}
#endif

static void PrintStackTrace(FILE *_file)
{
    fprintf( _file, "\n" );
    fprintf( _file, "=========================\n" );
    fprintf( _file, "====== STACKTRACE =======\n" );
    fprintf( _file, "=========================\n\n" );

	#ifdef TARGET_OS_MACOSX
	PrintStackTraceMacOSX(_file);
	#endif
}
    
void GenerateBlackBox(char *_msg)
{
	// Save a block box to the ~/Desktop
	
	char filename[512];
	char *home = getenv("HOME");
	
	if (home == NULL) {
		strcpy(filename, "/tmp/blackbox.txt");
	}
	else {
		sprintf(filename, "%s/blackbox.txt", home);
	}
	
    FILE *_file = fopen( filename, "wt" );

    fprintf( _file, "=========================\n" );
    fprintf( _file, "DARWINIA BLACK BOX REPORT\n" );
    fprintf( _file, "=========================\n\n" );

    fprintf( _file, "VERSION : %s\n", DARWINIA_VERSION_STRING );

    if( _msg ) fprintf( _file, "ERROR   : '%s'\n", _msg );

	copyCrashLog(_file);

	PrintStackTrace(_file);
	fclose(_file);
}