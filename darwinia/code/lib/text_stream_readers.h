#ifndef INCLUDE_TEXT_STREAM_READERS_H
#define INCLUDE_TEXT_STREAM_READERS_H

#include <stdio.h>
#include <string>


//*****************************************************************************
// Class TextReader
//*****************************************************************************

// This is an ABSTRACT BASE class. If you want to actually tokenise some text,
// I recommend either TextFileReader or TextDataReader.
class TextReader
{
protected:
	char			m_seperatorChars[16];
	char			m_filename[256];

	int			m_offsetIndex;
	int			m_fileEncrypted;	// -1 means we don't know yet. 0 means no. 1 means yes.

	void		DoubleMaxLineLen();
	void		CleanLine();			// Decrypt, strip comments and scan for conflict markers

public:
	int				m_tokenIndex;
	char			*m_line;
	unsigned int	m_maxLineLen;		// Doesn't include '\0' - m_line points to an array one byte larger than this
	unsigned int	m_lineNum;

	TextReader		();
	virtual ~TextReader();

    virtual bool IsOpen         () = 0;
	virtual bool ReadLine		() = 0;	// Returns false on EOF, true otherwise
	bool		 TokenAvailable	();
	char		 *GetNextToken	();
    char		 *GetRestOfLine ();

	char const	*GetFilename	();

	void SetSeperatorChars		(char const *_seperatorChars);
	void SetDefaultSeperatorChars();
};


//*****************************************************************************
// Class TextFileReader
//*****************************************************************************

class TextFileReader: public TextReader
{
protected:
	FILE			*m_file;

public:
	TextFileReader				(char const *_filename);
	TextFileReader				(std::string const &_filename);
	~TextFileReader				();

    bool IsOpen                 ();
	bool ReadLine				();
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

    bool IsOpen                 ();
	bool ReadLine				();
};


#endif
