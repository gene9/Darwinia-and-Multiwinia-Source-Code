#ifndef __STRING_UTILS_H
#define __STRING_UTILS_H

#include <string>

void	StrToLower		( char *_string );											// Lowercase the string
void	StrToLower		( std::string &_string );

void    StrToLower      ( wchar_t *_string );
void    StrToUpper      ( wchar_t *_string );

#include <sstream>

template< class T > 
inline
std::string ToString( const T &x )
{
	std::ostringstream s;
	s << x;
	return s.str();
}

#include <vector>

std::string Join( std::vector< std::string > _strings, const std::string &_separator );

#ifdef TRACK_MEMORY_LEAKS
char    *newStr         ( const char *_file, int _line, const char *s);             // Make a copy of s, use delete[] to reclaim storage
wchar_t *newStr         ( const char *_file, int _line, const wchar_t *s);
#define newStr( x )		newStr( __FILE__, __LINE__, x )
#else
char    *newStr         ( const char *s );											// Make a copy of s, use delete[] to reclaim storage
wchar_t	*newStr			( const wchar_t *s );
#endif

void    StringReplace   ( const char *target, const char *string, char *subject );  // replace target with string in subject

void    SafetyString    ( char *string );                                           // Removes dangerous characters like %, replaces with safe characters

void    StripTrailingWhitespace ( char *string );                                   // Removes trailing /n, /r, space 

#ifdef TARGET_OS_LINUX
#ifndef stricmp
#define stricmp strcasecmp
#endif
#endif // TARGET_OS_LINUX

#endif // __STRING_UTILS_H
