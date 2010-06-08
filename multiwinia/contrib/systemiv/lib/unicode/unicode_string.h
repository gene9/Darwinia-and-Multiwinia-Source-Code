#ifndef INCLUDE_UNICODESTRING__H
#define INCLUDE_UNICODESTRING__H

#include <stdio.h>
#include <wchar.h>

#ifdef TARGET_OS_MACOSX
	typedef wchar_t WCHAR;
	#if __BIG_ENDIAN__
		#define kUTF32Encoding kCFStringEncodingUTF32BE
	#elif __LITTLE_ENDIAN__
		#define kUTF32Encoding kCFStringEncodingUTF32LE
	#endif
#endif

//*****************************************************************************
// Class UnicodeString
//*****************************************************************************

// An abstraction of wchar so that we can do more things easily!
class UnicodeString
{
protected:
	int				m_linelength;			// Length of line

	void			Init(int line_length, const wchar_t* start_data, int copy_length); 
	
	bool operator >				(const UnicodeString &_other) const;
	bool operator <=			(const UnicodeString &_other) const;
	bool operator >=			(const UnicodeString &_other) const;

public:
	bool			m_isBigEndian;
	wchar_t			*m_unicodestring;		// This string as wide characters
	char			*m_charstring;			// Backwards compatibility!

	// Constructors/Destructor
	UnicodeString		();
	UnicodeString		(int line_length);
	UnicodeString		(int line_length, wchar_t* start_data, int copy_length);
	UnicodeString		(char const *carray);
	UnicodeString		(wchar_t const *warray);
	UnicodeString		(const UnicodeString &_source );
	~UnicodeString		();

	// Operator overloads
	operator const char *		() const;			// So we can nicely convert non-ANSI chars to ?s.
	operator wchar_t *			();					// So we can be used to directly call functions, like wcslen
	bool operator ==			(const UnicodeString &_other) const;
	bool operator !=			(const UnicodeString &_other) const;
	bool operator <				(const UnicodeString &_other) const;
	UnicodeString& operator =	(const UnicodeString &_other);
	UnicodeString operator +	(const UnicodeString &_other) const;

	// Methods
	int				Length					() const;			// Get the maximum length of the string
	void			CopyUnicodeToCharArray	();
	void			ReplaceStringFlag		(wchar_t flag, const UnicodeString &_string);
	wchar_t*		StrStr					(const UnicodeString &_other);
	int				StrCmp					(const UnicodeString &_rhs, bool _caseSensitive = true) const;
	void			StrUpr					();
	void			StrLwr					();
	int				WcsLen					() const;
	int				FirstPositionOf			(wchar_t _character);
	int				LastPositionOf			(wchar_t _character);
	int				LastHanCharacter		();
	const char*		GetCharArray			();
	void			Resize					(int _newSize);
	void			Truncate				(int _maxLength);
	void			Trim					();
	int				StrOffsetCmp			(const UnicodeString &_rhs, int _offset, bool _caseSensitive) const;

	int				swprintf( const wchar_t *_fmt, ... );
};

#endif
