#include "lib/universal_include.h"
#include "lib/debug_utils.h"
#include "unicode_string.h"
#include <stdarg.h>
#include "lib/string_utils.h"
#include <string.h>

#ifdef TARGET_OS_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
#endif

#define MAX_UNICODE_LINE_LENGTH = 65536

#ifdef TARGET_OS_MACOSX
// convenience functions for converting between wchar_t and system-native CFStrings
static CFStringRef WideCharsToCFString(const wchar_t *_wideChars)
{
	return CFStringCreateWithBytes(NULL, (UInt8 *)_wideChars,
								   wcslen(_wideChars)*sizeof(wchar_t),
								   kUTF32Encoding, false);
}

static wchar_t *CFStringToWideChars(CFStringRef _cfString)
{
	int cfLength;
	int wcharsLength;
	wchar_t *wideChars;
	
	cfLength = CFStringGetLength(_cfString);
	wcharsLength = CFStringGetMaximumSizeForEncoding(cfLength, kUTF32Encoding) / sizeof(wchar_t);
	wideChars = new wchar_t[ wcharsLength + 1 ];
	CFStringGetBytes(_cfString, CFRangeMake(0, cfLength), kUTF32Encoding,
					 0, false, (UInt8 *)wideChars, wcharsLength*sizeof(wchar_t), NULL);
	wideChars[wcharsLength] = L'\0';
	
	return wideChars;
}
#endif

UnicodeString::UnicodeString()
{
	m_unicodestring = new wchar_t[1];
	m_unicodestring[0] = L'\0';
	
	m_charstring = new char[1];
	m_charstring[0] = '\0';

	m_linelength = 1;
#if __BIG_ENDIAN__
	m_isBigEndian = true;
#else
	m_isBigEndian = false;
#endif
}

UnicodeString::UnicodeString(int line_length)
{
	m_unicodestring = new wchar_t[line_length];
	m_unicodestring[0] = L'\0';
	m_charstring = new char[line_length];
	m_charstring[0] = '\0';
	m_linelength = line_length;
#if __BIG_ENDIAN__
	m_isBigEndian = true;
#else
	m_isBigEndian = false;
#endif
}

UnicodeString::UnicodeString(int line_length, wchar_t* start_data, int copy_length)
:	m_unicodestring(NULL),
	m_charstring(NULL)
{
	Init(line_length, start_data, copy_length);
}

UnicodeString::UnicodeString(wchar_t const *warray)
:
m_unicodestring(NULL),
m_charstring(NULL)
{
	int length = wcslen(warray)+1;
	Init(length, warray, length);
}

UnicodeString::UnicodeString(char const *carray)
{
	m_linelength = strlen(carray)+1;			// + 1 null terminator (which already exists in the char*).
	m_unicodestring = new wchar_t[m_linelength];
	m_charstring = new char[m_linelength];

	int len = m_linelength;
	for (int i = 0; i < len; i++)
	{
		m_unicodestring[i] = (unsigned char) carray[i];
	}

#ifdef __BIG_ENDIAN__
	m_isBigEndian = true;
#else
	m_isBigEndian = false;
#endif

	CopyUnicodeToCharArray();
}

UnicodeString::UnicodeString( const UnicodeString &_source )
{
	m_isBigEndian = _source.m_isBigEndian;
	m_linelength = _source.m_linelength;

	m_unicodestring = new wchar_t[m_linelength];
	memcpy(m_unicodestring, _source.m_unicodestring, m_linelength*sizeof(wchar_t));

	m_charstring = new char[m_linelength];

	CopyUnicodeToCharArray();
}

UnicodeString::~UnicodeString()
{
	delete[] m_unicodestring;
	m_unicodestring = NULL;

	delete[] m_charstring;
	m_charstring = NULL;
}

void UnicodeString::Init(int line_length, const wchar_t* start_data, int copy_length)
{
	delete [] m_unicodestring;
	m_unicodestring = new wchar_t[line_length];
	memcpy(m_unicodestring,start_data,copy_length*sizeof(wchar_t));
	if (copy_length < line_length)
	{
		m_unicodestring[copy_length] = L'\0';
	}

	delete [] m_charstring;
	m_charstring = new char[line_length];

	m_linelength = line_length;
#ifdef __BIG_ENDIAN__
	m_isBigEndian = true;
#else
	m_isBigEndian = false;
#endif

	CopyUnicodeToCharArray();
}

UnicodeString::operator const char *() const
{
	return m_charstring;
}

UnicodeString::operator wchar_t *()
{
	return m_unicodestring;
}

UnicodeString UnicodeString::operator + (const UnicodeString &_other) const
{
	int this_total_length = wcslen(m_unicodestring);
	int	that_total_length = wcslen(_other.m_unicodestring);

	UnicodeString newUstring(this_total_length+that_total_length+1);
	memcpy(newUstring.m_unicodestring,m_unicodestring,this_total_length*sizeof(wchar_t));
	memcpy(newUstring.m_unicodestring + this_total_length,_other.m_unicodestring,that_total_length*sizeof(wchar_t));
	newUstring.m_unicodestring[this_total_length+that_total_length] = L'\0';
	newUstring.CopyUnicodeToCharArray();

	return newUstring;
}

UnicodeString& UnicodeString::operator = (const UnicodeString &_other)
{
	// No op for self assign
	if( this == &_other )
		return *this;

	m_linelength = _other.m_linelength;
	m_isBigEndian = _other.m_isBigEndian;

	delete[] m_unicodestring;

	m_unicodestring = new wchar_t[m_linelength];

	delete [] m_charstring;

	m_charstring = new char[m_linelength];

	memcpy(m_unicodestring, _other.m_unicodestring,m_linelength*sizeof(wchar_t));
	CopyUnicodeToCharArray();

	return *this;
}

bool UnicodeString::operator == (const UnicodeString &_other) const
{
	return wcscmp(m_unicodestring, _other.m_unicodestring) == 0;
}

bool UnicodeString::operator != (const UnicodeString &_other) const 
{
	return !(*this == _other);
}

bool UnicodeString::operator < (const UnicodeString &_other) const 
{
	return wcscmp(m_unicodestring, _other.m_unicodestring) < 0;
}


static int CountOccurrences( wchar_t flag, const UnicodeString &_hay )
{
	int length = wcslen(_hay.m_unicodestring)-1;
	int occurrences = 0;

	for( int i = 0; i < length; i++ )
	{
		if( _hay.m_unicodestring[i] == L'*' && _hay.m_unicodestring[i + 1] == flag )
		{
			occurrences++;
			i++;
		}
	}

	return occurrences;
}

void UnicodeString::ReplaceStringFlag(wchar_t flag, const UnicodeString &_string)
{
	int numOccurrences = CountOccurrences( flag, *this );
	int length = Length();
	int stringLength = _string.Length();
	int newLength = length + stringLength * numOccurrences;

	wchar_t* final = new wchar_t[newLength + 1];
	int bufferPosition = 0;

	for (int i = 0; i <= length; i++)
	{
		if( m_unicodestring[i] == '*' &&
            i+1 <= length )
        {
            if( m_unicodestring[i+1] == flag )
            {
				memcpy( final + bufferPosition, _string.m_unicodestring, sizeof(wchar_t) * stringLength );
				// Skip the flag character
                i++;
				bufferPosition += stringLength;
            }
            else
            {
                final[bufferPosition] = m_unicodestring[i];
                bufferPosition++;
            }
        }
		else
        {
            final[bufferPosition] = m_unicodestring[i];
            bufferPosition++;
        }
	}

	final[newLength] = L'\0';

	delete[] m_unicodestring;
	m_unicodestring = final;
	m_linelength = wcslen(m_unicodestring)+1;

	CopyUnicodeToCharArray();
}

wchar_t* UnicodeString::StrStr(const UnicodeString &_other)
{
	return wcsstr(m_unicodestring,_other.m_unicodestring);
}

void UnicodeString::StrUpr()
{
#ifdef TARGET_OS_MACOSX
	// Work around lack of a _wcsupr() function (which can't be reliably
	// done in-place anyway)
	CFStringRef cfString = WideCharsToCFString(m_unicodestring);
	CFMutableStringRef uppercase = CFStringCreateMutableCopy(NULL, 0, cfString);
	wchar_t *uppercaseChars;
	
	CFStringUppercase(uppercase, CFLocaleGetSystem());
	uppercaseChars = CFStringToWideChars(uppercase);
	wcsncpy(m_unicodestring, uppercaseChars, m_linelength);
	m_unicodestring[min((size_t)m_linelength, wcslen(uppercaseChars))] = L'\0';
	delete uppercaseChars;
	CFRelease(uppercase);
	CFRelease(cfString);
#elif defined( TARGET_MSVC )
	_wcsupr(m_unicodestring);
#else
	StrToUpper( m_unicodestring );
#endif
	CopyUnicodeToCharArray();
}


void UnicodeString::StrLwr()
{
#ifdef TARGET_OS_MACOSX
	CFStringRef cfString = WideCharsToCFString(m_unicodestring);
	CFMutableStringRef lowercase = CFStringCreateMutableCopy(NULL, 0, cfString);
	wchar_t *lowercaseChars;
	
	CFStringLowercase(lowercase, CFLocaleGetSystem());
	lowercaseChars = CFStringToWideChars(lowercase);
	wcsncpy(m_unicodestring, lowercaseChars, m_linelength);
	m_unicodestring[min((size_t)m_linelength, wcslen(lowercaseChars))] = L'\0';
	delete lowercaseChars;
	CFRelease(lowercase);
	CFRelease(cfString);
#elif defined( TARGET_MSVC )
	_wcslwr(m_unicodestring);
#else
	StrToLower( m_unicodestring );
#endif
	CopyUnicodeToCharArray();
}

int UnicodeString::StrCmp(const UnicodeString &_rhs, bool _caseSensitive) const
{	
	if (_caseSensitive)
		return wcscmp(m_unicodestring,_rhs.m_unicodestring);
	else
	{
#ifdef TARGET_OS_MACOSX
		// work around lack of a wcsicmp() function on OS X
		CFStringRef thisStr, rhsStr;
		CFComparisonResult result;
		
		thisStr = WideCharsToCFString(m_unicodestring);
		rhsStr = WideCharsToCFString(_rhs.m_unicodestring);
		result = CFStringCompare(thisStr, rhsStr, kCFCompareCaseInsensitive);
		
		CFRelease(thisStr);
		CFRelease(rhsStr);
		return (int)result;
#elif defined( TARGET_MSVC )
		return _wcsicmp(m_unicodestring,_rhs.m_unicodestring);
#else
		return wcscasecmp( m_unicodestring,_rhs.m_unicodestring );
#endif
	}
}

int UnicodeString::FirstPositionOf(wchar_t _character)
{
	wchar_t* w = wcschr(m_unicodestring, _character);
	if (w)
	{
		return w-m_unicodestring;
	}
	else
	{
		return 0;
	}
}

int UnicodeString::LastHanCharacter()
{
	int pos = -1;
	for( int i = WcsLen(); i >= 0; i-- )
	{
		if( m_unicodestring[i] >= 0x3000 && m_unicodestring[i] <= 0xd7ff )
		{
			pos = i;
			break;
		}
	}
	return pos;
}

int UnicodeString::LastPositionOf(wchar_t _character)
{
	wchar_t* w = wcsrchr(m_unicodestring, _character);
	if (w)
	{
		return w-m_unicodestring;
	}
	else
	{
		return 0;
	}
}

void UnicodeString::CopyUnicodeToCharArray()
{
	delete [] m_charstring;
	m_charstring = new char[m_linelength];

	int len = m_linelength;
	for (int i = 0; i < len; i++)
	{
		m_charstring[i] = (m_unicodestring[i] > 255 ? '?' : (char)m_unicodestring[i]);
	}
	
	m_charstring[ m_linelength - 1 ] = '\0';
}

int UnicodeString::Length() const
{
	return m_linelength-1;
}

int UnicodeString::WcsLen() const
{
	return wcslen(m_unicodestring);
}

const char* UnicodeString::GetCharArray()
{
	return m_charstring;
}

void UnicodeString::Truncate(int _maxLength)
{
	if( _maxLength < m_linelength ) Resize(_maxLength);
}

void UnicodeString::Resize(int _newSize)
{
	wchar_t *newUnicodeStr = new wchar_t[_newSize];
	memcpy(newUnicodeStr, m_unicodestring,sizeof(wchar_t)*(_newSize > m_linelength ? m_linelength : _newSize));
	delete [] m_unicodestring;
	m_unicodestring = newUnicodeStr;
	m_linelength = _newSize;
	m_unicodestring[_newSize-1] = L'\0';

	char *newCharStr = new char[_newSize];
	delete [] m_charstring;
	m_charstring = newCharStr;
	CopyUnicodeToCharArray();
}

void UnicodeString::Trim()
{
	while( m_unicodestring[0] != L'\0' && m_unicodestring[0] == L' ' )
	{
		m_unicodestring = &m_unicodestring[1];
	}

	int i = WcsLen()-1;
	while( i >= 0 && m_unicodestring[i] == L' ' )
	{
		i--;
	}

	m_unicodestring[i+1] = L'\0';
}

int	UnicodeString::swprintf( const wchar_t *_fmt, ... )
{
	va_list ap;
	va_start( ap, _fmt );
	int result;

#ifdef TARGET_MSVC
	result = wvsprintfW( m_unicodestring, _fmt, ap );
#else
	result = vswprintf( m_unicodestring, Length(), _fmt, ap );
#endif

	CopyUnicodeToCharArray();
	return result;
}

// Replacement for wcsncmp() and _wcsnicmp(), which are non-portable
int	UnicodeString::StrOffsetCmp(const UnicodeString &_rhs, int _offset, bool _caseSensitive) const
{
	int len = _rhs.WcsLen();
	
#ifdef TARGET_OS_MACOSX
	CFStringRef thisStr, rhsStr;
	CFComparisonResult result;
	
	thisStr = WideCharsToCFString(m_unicodestring+_offset);
	rhsStr = WideCharsToCFString(_rhs.m_unicodestring);
	// the range here may not correspond to UTF-32 chars in all cases
	result = CFStringCompareWithOptions(thisStr, rhsStr, CFRangeMake(0, len),
										_caseSensitive ? kCFCompareCaseInsensitive : 0);
	
	CFRelease(thisStr);
	CFRelease(rhsStr);
	return (int)result;
#elif defined( TARGET_MSVC )
	if (_caseSensitive)
		return wcsncmp(m_unicodestring+_offset, _rhs.m_unicodestring, len);
	else
		return _wcsnicmp(m_unicodestring+_offset, _rhs.m_unicodestring, len);
#else
	if( _caseSensitive )
		return wcsncmp( m_unicodestring, _rhs.m_unicodestring, len );		
	else
		return wcsncasecmp( m_unicodestring + _offset, _rhs.m_unicodestring, len );

#endif
}
