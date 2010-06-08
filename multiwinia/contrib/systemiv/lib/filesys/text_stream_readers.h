#ifndef INCLUDE_TEXT_STREAM_READERS_H
#define INCLUDE_TEXT_STREAM_READERS_H

#include <stdio.h>
#include "lib/unicode/unicode_string.h"
#include <string>


//*****************************************************************************
// Class TextReader
//*****************************************************************************

// This is an ABSTRACT BASE class. If you want to actually tokenise some text,
// I recommend either TextFileReader or TextDataReader.

#define NO_COPY(Class)					\
private:								\
	Class &operator = (const Class &);	\
	Class( const Class &);				\


class TextReader
{
protected:
	char			m_seperatorChars[16];
	std::string		m_filename;
	int			    m_offsetIndex;
	int			    m_fileEncrypted;	    // -1 means we don't know yet. 0 means no. 1 means yes.
	bool			m_isunicode;

	void		    DoubleMaxLineLen();
	virtual void	CleanLine();			// Decrypt, strip comments and scan for conflict markers

	UnicodeString	m_unicodeline;
	UnicodeString	m_restOfUnicodeLine;
	UnicodeString	m_unicodeToken;

public:
	int				m_tokenIndex;
	char			*m_line;
	unsigned int	m_maxLineLen;		    // Doesn't include '\0' - m_line points to an array one byte larger than this
	unsigned int	m_lineNum;

	TextReader		    ();
	virtual ~TextReader ();

	operator bool() const;

    virtual bool IsOpen			() const = 0;
	virtual bool ReadLine		() = 0;	// Returns false on EOF, true otherwise
	virtual bool TokenAvailable	();
	bool		 IsUnicode		();
	char *GetNextToken			();
    char *GetRestOfLine			();

	virtual const UnicodeString&	GetNextUnicodeToken		();
    virtual const UnicodeString&	GetRestOfUnicodeLine	();

	char const	*GetFilename	();

	virtual	void SetSeperatorChars		(char const *_seperatorChars);
	virtual	void SetDefaultSeperatorChars();

	NO_COPY( TextReader )
};


//*****************************************************************************
// Class TextFileReader
//*****************************************************************************

class TextFileReader: public TextReader
{
protected:
	FILE *m_file;

public:
	TextFileReader				(char const *_filename);
	~TextFileReader				();

    bool IsOpen                 () const;
	bool ReadLine				();

	NO_COPY( TextFileReader )
};


//*****************************************************************************
// Class TextDataReader
//*****************************************************************************

class TextDataReader: public TextReader
{
protected:
	char const			*m_data;
	unsigned int		m_dataSize;
	unsigned int		m_offset;

public:
	TextDataReader				(char const *_data, unsigned int _dataSize, char const *_filename);

    bool IsOpen                 () const;
	bool ReadLine				();

	NO_COPY( TextDataReader )
};


#endif
