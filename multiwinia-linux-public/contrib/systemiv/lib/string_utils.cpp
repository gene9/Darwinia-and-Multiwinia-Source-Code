#include "lib/universal_include.h"

#include "string_utils.h"

#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <wctype.h>
#include <algorithm>

#ifdef TARGET_MSVC
#pragma warning( disable : 4996 )
#endif

void StringReplace( const char *target, const char *string, char *subject )
{
	// ISSUE: This function has the following problems:
	//		   1. It writes into a temporary space (final) which is of fixed length 1024.
	//		   2. It copies that result into 'subject', which not have sufficient space.
	
    char final[1024];
    memset( final, 0, sizeof(final) );
    int bufferPosition = 0;

	size_t subjectLen = strlen( subject );
	size_t targetLen = strlen( target );
	size_t stringLen = strlen( string );

    for( size_t i = 0; i <= subjectLen; ++i )
    {
        if( subject[i] == target[0] && i + targetLen <= subjectLen)
        {
            bool match = true;
            for( size_t j = 0; j < targetLen; ++j )
            {
                if( subject[i + j] != target[j] )
                {
                    match = false;
                }
            }

            if( match )
            {
                for( size_t j = 0; j < stringLen; ++j )
                {                        
                    final[bufferPosition] = string[j];
                    bufferPosition++;
                }
            }
            else
            {
                final[bufferPosition] = subject[i];
                bufferPosition++;
            }

        }
        else
        {
            final[bufferPosition] = subject[i];
            bufferPosition++;
        }
    }
    strcpy( subject, final );
}


void SafetyString( char *string )
{
    size_t strlength = strlen(string);

    for( size_t i = 0; i < strlength; ++i )
    {
        if( string[i] == '%' ) 
        {
            string[i] = ' ';
        }
    }
}


void StripTrailingWhitespace ( char *string )
{
    for( int i = int(strlen(string))-1; i >= 0; --i )
    {
        if( string[i] == ' ' ||
            string[i] == '\n' ||
            string[i] == '\r' )
        {
            string[i] = '\x0';
        }
        else
        {
            break;
        }
    }
}

void StrToLower(char *_string)
{
	while (*_string != '\0')
	{
		*_string = tolower(*_string);
		_string++;
	}
}

void StrToLower		( std::string &_string )
{
	std::transform( _string.begin(), _string.end(), _string.begin(), tolower );
}

void    StrToUpper      ( wchar_t *_string )
{
	while (*_string != '\0')
	{
		*_string = towupper(*_string);
		_string++;
	}
}

void    StrToLower      ( wchar_t *_string )
{
	while (*_string != '\0')
	{
		*_string = towlower(*_string);
		_string++;
	}
}


#ifdef TRACK_MEMORY_LEAKS
#undef newStr
#endif

// Ordinary
char *newStr( const char *s)
{
    if ( !s )
        return NULL;
	char *d = new char [strlen(s) + 1];
	strcpy(d, s);
	return d;
}	

wchar_t *newStr( const wchar_t *s)
{
    if ( !s )
        return NULL;
	wchar_t *d = new wchar_t [wcslen(s) + 1];
	wcscpy(d, s);
	return d;
}	


#ifdef TRACK_MEMORY_LEAKS
#undef new

// Tracks the parent
char *newStr( const char *_filename, int _line, const char *s)
{
    if ( !s )
        return NULL;
	char *d = new (_NORMAL_BLOCK, _filename, _line) char [strlen(s) + 1];
	strcpy(d, s);
	return d;
}	

// Tracks the parent
wchar_t *newStr( const char *_filename, int _line, const wchar_t *s)
{
    if ( !s )
        return NULL;
	wchar_t *d = new (_NORMAL_BLOCK, _filename, _line) wchar_t [wcslen(s) + 1];
	wcscpy(d, s);
	return d;
}	
#endif

std::string Join( std::vector< std::string > _strings, const std::string &_separator )
{
	std::ostringstream s;

	int secondLast = _strings.size() - 1;
	for( int i = 0; i < secondLast; i++ )
	{
		s << _strings[i] << _separator;		
	}
	if( secondLast >= 0 )
		s << _strings[secondLast];

	return s.str();
}

