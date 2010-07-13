#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

class Vector3;

#ifdef NO_DEBUG_OUTPUT
#define AppDebugOut(...)		{}
#define AppDebugOutData(x,y)	{}
#else
void    AppDebugOut            (const char *_msg, ...);                                     // Outputs to Devstudio output pane                                    
void    AppDebugOutData        (const void *_data, int _dataLen);                           // Dumps the data as a hex table
#endif

#ifdef NO_DEBUG_ASSERT
#	define AppDebugAssert(x)       {}
#else
	void    AppDebugAssert         (bool _condition);                           // Does nothing in Release builds
#endif

void    AppReleaseAssertFailed (char const *_fmt, ...);                    // Same as DebugAssert in Debug builds
void    AppGenerateBlackBox    (const char *_filename, const char *_msg );

#ifdef TARGET_MSVC
#define AppReleaseAssert(cond, ...) \
	do { \
		if (!(cond)) \
			AppReleaseAssertFailed( __VA_ARGS__ ); \
	} while (false)
#else
// GCC
#define AppReleaseAssert(cond, fmt, args...) \
	do { \
		if (!(cond)) \
			AppReleaseAssertFailed( fmt, ## args ); \
	} while (false)
#endif

#define AppAssert(x)        AppReleaseAssert((x),          \
                            "Assertion failed : '%s'\n\n"  \
                            "%s\nline number %d",          \
                            #x, __FILE__, __LINE__ )

#ifdef TARGET_MSVC
#define AppAssertArgs(x, fmt, ...) AppReleaseAssert((x),          \
                                   "Assertion failed : '%s'\n\n"  \
                                   "%s\nline number %d\n" fmt,    \
                                   #x, __FILE__, __LINE__, __VA_ARGS__ )
#else
// GCC
#define AppAssertArgs(x, fmt, args...) AppReleaseAssert((x),          \
                                       "Assertion failed : '%s'\n\n"  \
                                       "%s\nline number %d\n" fmt,    \
                                       #x, __FILE__, __LINE__, ## args )
#endif

#define AppAbort(x)         AppReleaseAssert(false,        \
                            "Abort : '%s'\n\n"             \
                            "%s\nline number %d",          \
                            x, __FILE__, __LINE__ )


void    AppReleaseExit(char const *_fmt, ...);                    // Same as DebugAssert in Debug builds


void    AppDebugOutRedirect     (const char *_filename);

void    AppDebugLogPathPrefix   (const char *_filename);

char *HashDouble( double value, char *buffer );
char *HashVector3( const Vector3 &value, char *buffer );



#endif

