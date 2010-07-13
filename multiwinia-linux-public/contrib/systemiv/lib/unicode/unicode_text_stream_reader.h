#ifndef INCLUDE_UNICODE_TEXT_STREAM_READERS_H
#define INCLUDE_UNICODE_TEXT_STREAM_READERS_H

#include <stdio.h>
#include <wchar.h>

#include "lib/filesys/text_stream_readers.h"
#include "unicode_string.h"

//*****************************************************************************
// Class UnicodeTextReader
//*****************************************************************************

class UnicodeTextReader: public TextReader
{
protected:
	UnicodeString	m_unicodeline;
	UnicodeString	m_restOfUnicodeLine;
	UnicodeString	m_unicodeToken;
	WCHAR			m_seperatorWchars[16];

	virtual void		DoubleMaxLineLen();
	virtual wchar_t*	ReadUnicodeLine	(wchar_t *ws, size_t n);
	virtual WCHAR		ReadUnicodeChar	();

public:
	UnicodeTextReader					();
	~UnicodeTextReader					();

	virtual bool IsOpen					() const;
	virtual bool ReadLine				();
	virtual bool TokenAvailable			();

	const UnicodeString&	GetNextUnicodeToken		();
    const UnicodeString&	GetRestOfUnicodeLine	();

	void		SetSeperatorChars		(char const *_seperatorChars);
	void		SetDefaultSeperatorChars();

	void		CleanLine				();

	NO_COPY(UnicodeTextReader)
};


//*****************************************************************************
// Class UnicodeTextFileReader
//*****************************************************************************

class UnicodeTextFileReader: public UnicodeTextReader
{
protected:
	FILE			*m_file;
	wchar_t*		ReadUnicodeLine	(wchar_t *ws, size_t n);
	WCHAR			ReadUnicodeChar	();

public:
	UnicodeTextFileReader			(char const *_filename);
	~UnicodeTextFileReader			();

    bool IsOpen						() const;
	bool ReadLine					();

	NO_COPY(UnicodeTextFileReader)
};

//*****************************************************************************
// Class UnicodeTextDataReader
//*****************************************************************************

class UnicodeTextDataReader: public UnicodeTextReader
{
protected:
	char const			*m_data;
	unsigned int		m_dataSize;
	unsigned int		m_offset;
	wchar_t*	ReadUnicodeLine	(wchar_t *ws, size_t n);
	WCHAR		ReadUnicodeChar	();

public:
	UnicodeTextDataReader		(char const *_data, unsigned int _dataSize, char const *_filename);

    bool IsOpen                 () const;
	bool ReadLine				();

	NO_COPY(UnicodeTextDataReader)
};

#endif
