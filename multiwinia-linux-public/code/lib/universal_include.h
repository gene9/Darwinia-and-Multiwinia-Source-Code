#ifndef INCLUDED_UNIVERSAL_INCLUDE_H
#define INCLUDED_UNIVERSAL_INCLUDE_H

#define DARWINIA_VERSION "dev"			// Development
#define DARWINIA_EXE_VERSION 6,0,0,0				// Gold Master
#define STR_DARWINIA_EXE_VERSION "6, 0, 0, 0\0"	// Gold Master

#define APP_NAME        "Multiwinia"
#define APP_NAME_W      L"Multiwinia"

#define APP_VERSION     DARWINIA_VERSION

#ifdef TARGET_MSVC
    #define APP_SYSTEM "PC"
#endif
#ifdef TARGET_OS_MACOSX
	#ifdef __ppc__
		#define APP_SYSTEM "Mac (PPC)"
	#else
		#define APP_SYSTEM "Mac (Intel)"
	#endif
#endif
#ifdef TARGET_OS_LINUX
    #define APP_SYSTEM "Linux"
#endif


// === PICK ONE OF THESE TARGETS ===
// === NOTE: These targets ALL refer to Multiwinia

//#define    TARGET_DEBUG				1
//#define    TARGET_PC_HARDWARECOMPAT	1
//#define   TARGET_MULTIWINIA_DEMOONLY	1
//#define   TARGET_BETATEST_GROUP_ALL	1
//#define   TARGET_BETATEST_GROUP_A		1
//#define   TARGET_BETATEST_GROUP_B		1
//#define   TARGET_BETATEST_GROUP_C		1
//#define   TARGET_PC_PREVIEW			1
#define   TARGET_PC_FULLGAME          1
//#define	TARGET_ASSAULT_STRESSTEST	1

#if TARGET_DEBUG + \
	TARGET_PC_HARDWARECOMPAT + \
	TARGET_MULTIWINIA_DEMOONLY + \
	TARGET_BETATEST_GROUP_ALL + \
	TARGET_BETATEST_GROUP_A + \
	TARGET_BETATEST_GROUP_B + \
	TARGET_BETATEST_GROUP_C + \
	TARGET_PC_PREVIEW + \
    TARGET_PC_FULLGAME + \
	TARGET_ASSAULT_STRESSTEST \
	!= 1
#error One and only one target should be defined.
#endif

// === PICK ONE OF THESE TARGETS ===


#define    DEBUG_RENDER_ENABLED
#define    FLOAT_NUMERICS

#define    USE_CRASHREPORTING

//#define    USE_SOUND_DSPEFFECTS
//#define    USE_SOUND_HW3D
//#define    USE_SOUND_MEMORY

//#define    USE_LOADERS

#ifdef TARGET_MULTIWINIA_DEMOONLY
    //#define DEMOBUILD
    #define DARWINIA_GAMETYPE "multiwinia-demo"
    #define MULTIWINIA_DEMOONLY
    #define DUMP_DEBUG_LOG
    #define INCLUDEGAMEMODE_KOTH
    #define INCLUDEGAMEMODE_CTS
    #define INCLUDE_CRATES_BASIC
    #define MULTIPLAYER_DISABLED
#endif

#ifdef TARGET_PC_FULLGAME
    #define     DARWINIA_GAMETYPE "multiwinia"
    #define     DUMP_DEBUG_LOG
    //#define     LOCATION_EDITOR
    #define     LAN_PLAY_ENABLED
    #define     WAN_PLAY_ENABLED			
	#define		NETWORK_STATS_ENABLED
    #define     INCLUDEGAMEMODE_DOMINATION
    #define     INCLUDEGAMEMODE_KOTH
    #define     INCLUDEGAMEMODE_CTS
    #define     INCLUDEGAMEMODE_ROCKET
    #define     INCLUDEGAMEMODE_ASSAULT
    #define     INCLUDEGAMEMODE_BLITZ
	//#define	    INCLUDEGAMEMODE_TANKBATTLE
    #define     INCLUDE_CRATES_BASIC
    #define     INCLUDE_CRATES_ADVANCED
    #define     INCLUDE_TUTORIAL
	#define		AUTHENTICATION_LEVEL	1

    //#define     CHRISTMAS_DEMO

    #define     BLOCK_OLD_DARWINIA_OBJECTS
#endif


#ifdef TARGET_PC_PREVIEW
    #define     DARWINIA_GAMETYPE "pc-preview"
    //#define     LAN_PLAY_ENABLED
    //#define     WAN_PLAY_ENABLED
    #define     MULTIPLAYER_DISABLED
    #define     INCLUDEGAMEMODE_KOTH
    #define     INCLUDEGAMEMODE_CTS
    #define     INCLUDE_CRATES_BASIC
    #define     INCLUDE_TUTORIAL
    #define     PERMITTED_MAPS {MAPID_MP_KOTH_2P_1, MAPID_MP_KOTH_4P_1, MAPID_MP_CTS_2P_1, 0}
#endif

#ifdef TARGET_DEBUG
    #define DARWINIA_GAMETYPE "debug"
    //#define LOCATION_EDITOR
    #define SOUND_EDITOR
    #define CHEATMENU_ENABLED
    #define GESTURE_EDITOR
    #define NETWORK_STATS_ENABLED
	#define AVI_GENERATOR
	//#define TRACK_MEMORY_LEAKS
    //#define DUMP_DEBUG_LOG
    
    #define TRACK_SYNC_RAND

    #define    LAN_PLAY_ENABLED
    #define    WAN_PLAY_ENABLED	
    #define    WAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE
    #define    INCLUDEGAMEMODE_DOMINATION
    #define    INCLUDEGAMEMODE_KOTH
    #define    INCLUDEGAMEMODE_CTS
    #define    INCLUDEGAMEMODE_ROCKET
    #define    INCLUDEGAMEMODE_ASSAULT
    #define    INCLUDEGAMEMODE_BLITZ
	#define	   INCLUDEGAMEMODE_TANKBATTLE
    #define    INCLUDEGAMEMODE_PROLOGUE
    #define    INCLUDEGAMEMODE_CAMPAIGN
    #define    INCLUDE_CRATES_BASIC
    #define    INCLUDE_CRATES_ADVANCED
    #define    INCLUDE_TUTORIAL
	#define	   AUTHENTICATION_LEVEL	1	
	#define	   INCLUDE_CRATE_HELP_WINDOW

    #define    BLOCK_OLD_DARWINIA_OBJECTS

    //#define	   TESTBED_ENABLED
	//#define		SPECTATOR_ONLY
//#define    RENDER_CURSOR_3D
	//#define DUMP_DEBUG_LOG
#endif

#ifdef TARGET_PC_HARDWARECOMPAT
    #define DARWINIA_GAMETYPE "hwcompat"
    #define DUMP_DEBUG_LOG

    //#define    LAN_PLAY_ENABLED
    #define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE
    //#define    WAN_PLAY_ENABLED
    #define    WAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

    // Must ends with 0
    #define PERMITTED_MAPS {MAPID_MP_KOTH_2P_1, MAPID_MP_BENCHMARK_1, MAPID_MP_KOTH_4P_1, 0}

    //#define    INCLUDEGAMEMODE_DOMINATION
    #define    INCLUDEGAMEMODE_KOTH
    //#define    INCLUDEGAMEMODE_CTS
    //#define    INCLUDEGAMEMODE_ROCKET
    //#define    INCLUDEGAMEMODE_ASSAULT
    //#define    INCLUDEGAMEMODE_BLITZ
    //#define    INCLUDEGAMEMODE_PROLOGUE
    //#define    INCLUDEGAMEMODE_CAMPAIGN
	//#define	 INCLUDEGAMEMODE_TANKBATTLE

    #define    INCLUDE_CRATES_BASIC
    //#define    INCLUDE_CRATES_ADVANCED
    #define     MULTIPLAYER_DISABLED
	#define		SPECTATOR_ONLY
	#define		HIDE_INVALID_GAMETYPES
#endif

#ifdef TARGET_ASSAULT_STRESSTEST
    #define DARWINIA_GAMETYPE "assault-stress"
    #define DUMP_DEBUG_LOG

    //#define    LAN_PLAY_ENABLED
    #define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE
    //#define    WAN_PLAY_ENABLED
    #define    WAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

    // Must ends with 0
    //#define PERMITTED_MAPS {MAPID_MP_KOTH_2P_1, MAPID_MP_BENCHMARK_1, MAPID_MP_KOTH_4P_1, 0}
	#define PERMITTED_MAPS {MAPID_MP_ASSAULT_STRESS, 0 }

    //#define    INCLUDEGAMEMODE_DOMINATION
    //#define    INCLUDEGAMEMODE_KOTH
    //#define    INCLUDEGAMEMODE_CTS
    //#define    INCLUDEGAMEMODE_ROCKET
    #define    INCLUDEGAMEMODE_ASSAULT
    //#define    INCLUDEGAMEMODE_BLITZ
    //#define    INCLUDEGAMEMODE_PROLOGUE
    //#define    INCLUDEGAMEMODE_CAMPAIGN
	//#define	 INCLUDEGAMEMODE_TANKBATTLE

    #define    INCLUDE_CRATES_BASIC
    //#define    INCLUDE_CRATES_ADVANCED
    #define     MULTIPLAYER_DISABLED
	#define		SPECTATOR_ONLY
	#define		HIDE_INVALID_GAMETYPES
#endif

#ifdef TARGET_BETATEST_GROUP_ALL
    #define DARWINIA_GAMETYPE "beta-group-all"
    #define TARGET_BETATEST_GROUP

    //#define TRACK_SYNC_RAND

    // Must ends with 0
    //#define PERMITTED_MAPS {MAPID_MP_KOTH_2P_1, MAPID_MP_KOTH_3P_1, MAPID_MP_ASSAULT_2P_3, MAPID_MP_ASSAULT_3P_2, MAPID_MP_ASSAULT_3P_3, MAPID_MP_ASSAULT_4P_2, MAPID_MP_ROCKETRIOT_2P_3, MAPID_MP_ROCKETRIOT_3P_2, MAPID_MP_ROCKETRIOT_4P_1, MAPID_MP_CTS_2P_2, MAPID_MP_CTS_2P_4, MAPID_MP_CTS_3P_1, MAPID_MP_CTS_4P_1, 0}


    #define     DARWINIA_GAMETYPE "beta-group-all"
    #define     DUMP_DEBUG_LOG
    //#define     LOCATION_EDITOR
    #define     LAN_PLAY_ENABLED
    #define     WAN_PLAY_ENABLED			
	#define		NETWORK_STATS_ENABLED
    #define     INCLUDEGAMEMODE_DOMINATION
    #define     INCLUDEGAMEMODE_KOTH
    #define     INCLUDEGAMEMODE_CTS
    #define     INCLUDEGAMEMODE_ROCKET
    #define     INCLUDEGAMEMODE_ASSAULT
    #define     INCLUDEGAMEMODE_BLITZ
	//#define	    INCLUDEGAMEMODE_TANKBATTLE
    #define     INCLUDE_CRATES_BASIC
    #define     INCLUDE_CRATES_ADVANCED
    #define     INCLUDE_TUTORIAL
	#define		AUTHENTICATION_LEVEL	1

    #define     BLOCK_OLD_DARWINIA_OBJECTS
    //#define LOCATION_EDITOR
#endif

#ifdef TARGET_BETATEST_GROUP_A
    #define DARWINIA_GAMETYPE "beta-group-a"
    #define TARGET_BETATEST_GROUP

    // Must ends with 0
    #define PERMITTED_MAPS {MAPID_MP_CTS_2P_1, MAPID_MP_CTS_3P_1, MAPID_MP_CTS_4P_3, 0}

    //#define    INCLUDEGAMEMODE_DOMINATION
    //#define    INCLUDEGAMEMODE_KOTH
    #define    INCLUDEGAMEMODE_CTS
    //#define    INCLUDEGAMEMODE_ROCKET
    //#define    INCLUDEGAMEMODE_ASSAULT
    //#define    INCLUDEGAMEMODE_BLITZ
    //#define    INCLUDEGAMEMODE_PROLOGUE
    //#define    INCLUDEGAMEMODE_CAMPAIGN
	//#define	 INCLUDEGAMEMODE_TANKBATTLE

    //#define    INCLUDE_TUTORIAL

	#define    WAN_PLAY_ENABLED
	#define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

	#define    HIDE_INVALID_GAMETYPES

	#define    INCLUDE_CRATES_ADVANCED
#endif

#ifdef TARGET_BETATEST_GROUP_B
    #define DARWINIA_GAMETYPE "beta-group-b"
    #define TARGET_BETATEST_GROUP

    // Must ends with 0
    #define PERMITTED_MAPS {MAPID_MP_BLITZKRIEG_2P_2, MAPID_MP_BLITZKRIEG_3P_2, MAPID_MP_BLITZKRIEG_4P_2, 0}

    //#define    INCLUDEGAMEMODE_DOMINATION
    //#define    INCLUDEGAMEMODE_KOTH
    //#define    INCLUDEGAMEMODE_CTS
    //#define    INCLUDEGAMEMODE_ROCKET
    //#define    INCLUDEGAMEMODE_ASSAULT
    #define    INCLUDEGAMEMODE_BLITZ
    //#define    INCLUDEGAMEMODE_PROLOGUE
    //#define    INCLUDEGAMEMODE_CAMPAIGN
	//#define	 INCLUDEGAMEMODE_TANKBATTLE

    //#define    INCLUDE_TUTORIAL

	#define    WAN_PLAY_ENABLED
	#define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

	#define    HIDE_INVALID_GAMETYPES
#endif

#ifdef TARGET_BETATEST_GROUP_C
    #define DARWINIA_GAMETYPE "beta-group-c"
    #define TARGET_BETATEST_GROUP

    // Must ends with 0
    #define PERMITTED_MAPS {MAPID_MP_ROCKETRIOT_2P_4, MAPID_MP_ROCKETRIOT_2P_3, MAPID_MP_ROCKETRIOT_3P_1, 0}

    //#define    INCLUDEGAMEMODE_DOMINATION
    //#define    INCLUDEGAMEMODE_KOTH
    //#define    INCLUDEGAMEMODE_CTS
    #define    INCLUDEGAMEMODE_ROCKET
    //#define    INCLUDEGAMEMODE_ASSAULT
    //#define    INCLUDEGAMEMODE_BLITZ
    //#define    INCLUDEGAMEMODE_PROLOGUE
    //#define    INCLUDEGAMEMODE_CAMPAIGN
	//#define	 INCLUDEGAMEMODE_TANKBATTLE

    //#define    INCLUDE_TUTORIAL

	#define    WAN_PLAY_ENABLED
	#define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

	#define    HIDE_INVALID_GAMETYPES
#endif

#ifdef TARGET_BETATEST_GROUP
    #ifndef DARWINIA_GAMETYPE
        #define DARWINIA_GAMETYPE "beta-group"
    #endif

    #define DUMP_DEBUG_LOG

    //#define    LAN_PLAY_ENABLED
    #define    LAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE
    //#define    WAN_PLAY_ENABLED
    #define    WAN_PLAY_IF_NOT_ENABLED_NO_MESSAGE

	// Beta now allows all maps
    //#ifndef PERMITTED_MAPS
    //    // Must ends with 0
    //    #define PERMITTED_MAPS {0}
    //#endif

	#define	   AUTHENTICATION_LEVEL	1
    #define    INCLUDE_CRATES_BASIC
    //#define    INCLUDE_CRATES_ADVANCED

    //#define BENCHMARK_AND_FTP
#endif

#define DARWINIA_VERSION_STRING DARWINIA_PLATFORM "-" DARWINIA_GAMETYPE "-" DARWINIA_VERSION


#include <stdio.h>
#include <math.h>

#define TEXTURE_EXTENSION "bmp"
double iv_trunc_low_nibble(double x);

// Macros to disambiguate overloaded functions in the C++ version of <math.h>
double iv_atof(const char *str);
#ifdef __cplusplus
	#define iv_sin(x)	 iv_trunc_low_nibble(sin(double(x)))
	#define iv_cos(x)	 iv_trunc_low_nibble(cos(double(x)))
	#define iv_tan(x)	 iv_trunc_low_nibble(tan(double(x)))
	#define iv_sqrt(x)	 iv_trunc_low_nibble(sqrt(double(x)))
	#define iv_pow(x, y) iv_trunc_low_nibble(pow(double(x), double(y)))
	#define iv_exp(x)	 iv_trunc_low_nibble(exp(double(x)))
	#define iv_log(x)	 iv_trunc_low_nibble(log(double(x)))
	#define iv_asin(x)	 iv_trunc_low_nibble(asin(double(x)))
	#define iv_acos(x)	 iv_trunc_low_nibble(acos(double(x)))
	#define iv_atan(x)	 iv_trunc_low_nibble(atan(double(x)))
#endif

inline double iv_abs( double x ) { if( x >= 0.0 ) return x;	else return -x; }

#if defined(TARGET_MSVC)
	#pragma warning( disable : 4244 4305 4800 4018 4996 )
	#define for if (0) ; else for
	
	// Defines that will enable you to double click on a #pragma message
	// in the Visual Studio output window.
	#define MESSAGE_LINENUMBERTOSTRING(linenumber)	#linenumber
	#define MESSAGE_LINENUMBER(linenumber)			MESSAGE_LINENUMBERTOSTRING(linenumber)
	#define MESSAGE(x) message (__FILE__ "(" MESSAGE_LINENUMBER(__LINE__) "): "x) 
	
	#include <crtdbg.h>
	#define snprintf _snprintf

	// Visual studio 2005 insists that we use the underscored versions
	#include <string.h>
	#define stricmp _stricmp
	#define strupr _strupr
    #define strnicmp _strnicmp
	#define strlwr _strlwr
	#define strdup _strdup

	#include <stdlib.h>
	#define itoa _itoa

    #define DARWINIA_PLATFORM "win32"

	#define WIN32_LEAN_AND_MEAN
	#define _WIN32_WINDOWS 0x0500	// for IsDebuggerPresent
	#define _WIN32_WINNT 0x500  // for TryEnterCriticalSection

	#ifdef TRACK_MEMORY_LEAKS
		#include "lib/memory_leak.h"
	#endif

	// Use STL min() and max(), to avoid problems with macro expansion
	#define NOMINMAX
	#include <algorithm>
	using std::min;
	using std::max;

	#include "windows.h"

	#define HAVE_DSOUND
#endif

#ifdef TARGET_MINGW
	#define WIN32_LEAN_AND_MEAN
	#include "windows.h"
	#define HAVE_INET_NTOA
	#define HAVE_DSOUND
#endif

#ifdef TARGET_OS_LINUX                                                
	#include <ctype.h>                                                    
	#include <string.h>  

	#define stricmp strcasecmp                                            
	#define strnicmp strncasecmp                                          
	#define __stdcall                                                     
	template<class T> inline T min (T a, T b) { return (a < b) ? a : b; };
	template<class T> inline T max (T a, T b) { return (a > b) ? a : b; };
	inline char * strlwr(char *s) {                                       
	  char *p = s;                                                        
	  for (char *p = s; *p; p++)                                          
		*p = tolower(*p);                                                 
	  return s;                                                           
	}                                                                     
	inline char * strupr(char *s) {                                       
	  char *p = s;                                                        
	  for (char *p = s; *p; p++)                                          
		*p = toupper(*p);                                                 
	  return s;                                                           
	}                                                                     
	#include <unistd.h>                                                   
	//#define Sleep(n) sleep(n/1000.f)                                                   
	inline void Sleep(double n) { sleep(n/1000.0); }

    #define DARWINIA_PLATFORM "linux"
	#define _snprintf snprintf

	#undef AVI_GENERATOR
	#undef SOUND_EDITOR
	
	//We want OpenAL support
	#define HAVE_OPENAL
	
	// The use of INVOKE_CALLBACK_FROM_SOUND_THREAD greatly reduces
	// crackling (but is somewhat unstable if the audio device is misconfigured)
	#define INVOKE_CALLBACK_FROM_SOUND_THREAD
	//Otherwise we crash.
	#define SINGLE_THREADED_PROFILER
	
	// There are issues with OPENGL_DEBUG_ASSERTS on dual-screen setups
	// with a horizontal resolution of > ~2500 px.
	//#define OPENGL_DEBUG_ASSERTS
#endif

#ifdef TARGET_OS_MACOSX
    #define DARWINIA_PLATFORM "macosx"

	#include <unistd.h>                                                   
	#define Sleep(t) ( sleep( (t) / 1000.0 ) )

	#include <ctype.h>                                                    
	#define stricmp strcasecmp                                            
	#define strnicmp strncasecmp
	
	typedef uint32_t HRESULT;
	
	#include <algorithm>
	using std::min;
	using std::max;

	inline char * strlwr(char *s) {                                       
	  for (char *p = s; *p; p++)                                          
		*p = tolower(*p);                                                 
	  return s;                                                           
	}                                                                     
	inline char * strupr(char *s) {                                       
	  for (char *p = s; *p; p++)                                          
		*p = toupper(*p);                                                 
	  return s;                                                           
	}       
	
	#define __stdcall
	#define HAVE_INET_NTOA
	#define _snprintf snprintf
	#undef SOUND_EDITOR

	/* acosf and asinf don't link for some reason */
	#define acosf acos
	#define asinf asin
	
	#undef AVI_GENERATOR
	#undef SOUND_EDITOR
	#define INVOKE_CALLBACK_FROM_SOUND_THREAD
	#define SINGLE_THREADED_PROFILER
	#define OPENGL_DEBUG_ASSERTS
#endif // TARGET_OS_MACOSX

#ifdef TARGET_OS_MACOSX
	#define GL_GLEXT_LEGACY
	#include <OpenGL/gl.h>
	#include <OpenGL/glu.h>
	#include "lib/safegl.h"
#else
#ifdef USE_DIRECT3D
	#include <d3d9.h>
	#include <d3dx9.h>
	#include "lib/opengl_directx.h"
	#include "lib/safegl.h"
#else
	#include <GL/gl.h>
	#include <GL/glu.h>
#endif // USE_DIRECTX
#endif // !TARGET_OS_MACOSX

#ifdef TESTBED_ENABLED
#define GRACE_TIME 20.0f
#else
#define GRACE_TIME 3.0f
#endif

#define SAFE_FREE(x)         {free(x);x=NULL;}
#define SAFE_DELETE(x)       {delete x;x=NULL;}
#define SAFE_DELETE_ARRAY(x) {delete[] x;x=NULL;}
#define SAFE_RELEASE(x)      {if(x){(x)->Release();x=NULL;}}

#define GL_DEBUG()			do{ while(int glError = glGetError()){	printf("glError: %d LINE: %d\n", glError, __LINE__); } while(0)

#endif
