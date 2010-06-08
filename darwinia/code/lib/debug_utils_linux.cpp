#include "lib/universal_include.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/ucontext.h>
#include <unistd.h>
#include <errno.h>

#include "app.h"
#include "lib/debug_utils.h"

#ifndef NO_WINDOW_MANAGER
#include "lib/window_manager.h"
#include <SDL/SDL.h>
#endif

static void GenerateBlackBox( char *_msg, unsigned *_framePtr );

void DebugOut(char *_fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start (ap, _fmt);
    vsprintf(buf, _fmt, ap);
    fprintf(stderr, "%s\n", buf);
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

		register unsigned *framePtr asm("ebp");
        GenerateBlackBox( buf, framePtr );

#ifndef NO_WINDOW_MANAGER		
		g_windowManager->UncaptureMouse();
		g_windowManager->UnhideMousePointer();
		SDL_Quit();
#endif

		exit(-1);
	}
}


void PrintMemoryLeaks()
{
}

static char *s_pathToProgram = 0;

void SetupPathToProgram(const char *program)
{
	// binreloc gives us the full path
	s_pathToProgram = strcpy(new char[strlen(program)+1], program);
}

static void gdbStackTrace(FILE *_output)
{
	if (s_pathToProgram == NULL) {
		fprintf(stderr, "Could not invoke gdb stack trace because path to executable not set\n");
		return;
	}

	fflush(_output);

	const char *commandfile = "/tmp/darwinia.gdb";
	const char *stacktracefile = "/tmp/darwinia.stacktrace";

	FILE *scriptfp;

	if ((scriptfp = fopen(commandfile, "w")) == NULL) {
		fprintf(stderr, "Couldn't write to %s: %s", commandfile, strerror(errno));
		fprintf(_output, "\n\nCouldn't write to %s: %s\n\n", commandfile, strerror(errno));
		return;
	}
	
	fprintf(scriptfp,
			"file %s\nattach %d\ninfo threads\nthread apply all bt\nquit\n",
			s_pathToProgram, getpid());
	fclose(scriptfp);
	
	char line[1024];
	sprintf(line, "gdb < %s > %s", commandfile, stacktracefile);

	unlink(stacktracefile);

	// Darwinia will be paused while gdb does the stack trace.
	system(line);

	FILE *fp = fopen(stacktracefile, "r");
	if (!fp) {
		unlink(commandfile);
#ifdef _DEBUG
		fprintf(stderr, "Failed to invoke gdb for backtrace");
#endif
		fprintf(_output, "\n\nFailed to invoke gdb for backtrace\n\n");
		return;
	}
	fprintf(_output, "\n\ngdb stack trace:\n\n");
	while (fgets(line, sizeof(line), fp)) {
#ifdef _DEBUG
		fputs(line, stderr);
#endif
		fputs(line, _output);
	}
	fclose(fp);
	unlink(stacktracefile);
	unlink(commandfile);
}

static void PrintStackTrace(FILE *_file, unsigned *startStackPtr)
{
    fprintf( _file, "\n" );
    fprintf( _file, "=========================\n" );
    fprintf( _file, "====== STACKTRACE =======\n" );
    fprintf( _file, "=========================\n\n" );
	
	unsigned *stackPtr = startStackPtr;
	unsigned *previousStackPtr;

	// There is no useful information in this frame, so skip it.
	// stackPtr = *(unsigned **)stackPtr;		

	while(stackPtr) {
		unsigned *retAddress = ((unsigned **) stackPtr)[1];		
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

	gdbStackTrace(_file);
}

static void fatalSignal(int signum, siginfo_t *siginfo, void *arg)
{
	static char msg[64];
	sprintf(msg, "Got a fatal signal: %d\n", signum);
	ucontext_t *u = (ucontext_t *) arg;

	GenerateBlackBox(msg, (unsigned *) u->uc_mcontext.gregs[REG_EBP]);

	fprintf(stderr, 
			"\n\nDarwinia has unexpectedly encountered a fatal error.\n"
			"A full description of the error can be found in the file\n"
			"blackbox.txt in the current working directory\n\n");

#ifndef NO_WINDOW_MANAGER		
	g_windowManager->UncaptureMouse();
	g_windowManager->UnhideMousePointer();
	SDL_Quit();
#endif
}


void SetupMemoryAccessHandlers()
{
	struct sigaction sa;

	sa.sa_sigaction = fatalSignal;
	sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
	sigemptyset(&sa.sa_mask);
	
	sigaction(SIGSEGV, &sa, 0);
	sigaction(SIGBUS, &sa, 0);
}

static void GenerateBlackBox(char *_msg, unsigned *_framePtr)
{
	extern char * g_origWorkingDir;

	const char *blackboxBasename = "blackbox.txt";
	char *blackboxFilename = new char [
		strlen(blackboxBasename) 
		+ (g_origWorkingDir ? strlen(g_origWorkingDir) + 1 : 0) 
		+ 1];

	sprintf(blackboxFilename, "%s%s%s", 
			(g_origWorkingDir ? g_origWorkingDir : ""),
			(g_origWorkingDir ? "/" : ""),
			blackboxBasename);

    FILE *_file = fopen( blackboxFilename, "wt" );

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

	fflush(_file);
	PrintStackTrace(_file, _framePtr);
    fclose( _file );
}
