#ifndef _included_textTextFileWriter_h
#define _included_textTextFileWriter_h

/*
 *	A thin wrapper for ordinary text file IO
 *  with the added ability to encrypt the data
 *  as it is sent to the file
 *
 */

#include <stdio.h>
#include <sstream>

class TextWriter
{
public:
    TextWriter() 
    {};
    virtual ~TextWriter() {};

    virtual operator bool() const;    
    virtual int printf( char *fmt, ... ) = 0;
};

class TextFileWriter : public TextWriter
{
protected:
	int		m_offsetIndex;
	bool	m_encrypt;
	bool	m_canWrite;
	bool	m_assertOnFail;
	FILE	*m_file;

public:
	TextFileWriter  (char *_filename, bool _encrypt, bool _assertOnFail = true);
	~TextFileWriter ();

	virtual operator bool() const;

	virtual int printf  (char *fmt, ...);
};

class TextMemoryWriter : public TextWriter 
{
private:
    std::ostringstream  m_stringStream;
public:
    TextMemoryWriter();

    virtual operator bool() const;
    virtual int printf( char *_fmt, ... );

    std::ostringstream *GetStream();
};


#endif
