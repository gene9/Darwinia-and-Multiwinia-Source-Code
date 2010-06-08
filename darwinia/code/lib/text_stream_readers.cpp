#include "lib/universal_include.h"

#ifdef WIN32
#include <io.h>
#endif

#include <stdio.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/text_stream_readers.h"

#define DEFAULT_SEPERATOR_CHARS " \t\n\r:,"
#define INITIAL_LINE_LEN 128


//*****************************************************************************
// Class TextReader
//*****************************************************************************

static unsigned int s_offsets[] = {
	31, 7, 9, 1, 
	11, 2, 5, 5, 
	3, 17, 40, 12,
	35, 22, 27, 2
}; 


TextReader::TextReader()
:	m_maxLineLen(INITIAL_LINE_LEN),
	m_lineNum(0),
	m_offsetIndex(0),
	m_fileEncrypted(-1)
{
	m_filename[0] = '\0';
	m_line = new char[INITIAL_LINE_LEN + 1];	// Don't forget space for the null terminator
	strcpy(m_seperatorChars, DEFAULT_SEPERATOR_CHARS);
}


TextReader::~TextReader()
{
	delete [] m_line;
}


void TextReader::DoubleMaxLineLen()
{
	DarwiniaReleaseAssert(m_maxLineLen < 65536, "Text file contains line with more than 65536 characters");
	char *newLine = new char [m_maxLineLen * 2 + 1];
	memcpy(newLine, m_line, m_maxLineLen + 1);
	delete m_line;
	m_line = newLine;
	m_maxLineLen *= 2;
}


void TextReader::CleanLine()
{
	//
	// Decryption stuff
	
	if (m_fileEncrypted != 0)
	{
		int len = strlen(m_line);

		// Check if file is encrypted
		if (m_fileEncrypted == -1)
		{
			if (strnicmp(m_line, "redshirt2", 9) == 0)
			{
				m_fileEncrypted = 1;
				for (int i = 9; i < len; ++i)
				{
					m_line[i - 9] = m_line[i]; 
				}
				len -= 9;
			}
			else
			{
				m_fileEncrypted = 0;
			}
		}

		// Decrypt this line if necessary
		if (m_fileEncrypted)
		{
			for (int i = 0; i < len; ++i)
			{
				if (m_line[i] > 32) {
					m_offsetIndex++;
					m_offsetIndex %= 16;
					int j = m_line[i] - s_offsets[m_offsetIndex];
					if (j < 33) j += 95;
					m_line[i] = j;
				}
			}
		}
	}

	//
	// Scan for comments (which we remove) and merge conflict markers (which we assert on)
	
	int numAdjacentAngleBracketsFound = 0;
	char *c = m_line;
	while (c[0] != '\0')
	{
		if (c[0] == '<' || c[0] == '>')
		{
			numAdjacentAngleBracketsFound++;
			DarwiniaDebugAssert(numAdjacentAngleBracketsFound < 3);
            // Looks like you've got a merge error mate
		}
		else
		{
			numAdjacentAngleBracketsFound = 0;
			if (c[0] == '#')
			{
				c[0] = '\0';
				break;
			}
		}
		c++;
	}
}


void TextReader::SetSeperatorChars(char const *_seperatorChars)
{
	strncpy(m_seperatorChars, _seperatorChars, 15);
}


void TextReader::SetDefaultSeperatorChars()
{
	strcpy(m_seperatorChars, DEFAULT_SEPERATOR_CHARS);
}


bool TextReader::TokenAvailable()
{
	unsigned int i = m_tokenIndex;
	
	while (m_line[i] != '\0')
	{
		if (strchr(m_seperatorChars, m_line[i]) == NULL)
		{
			return true;
		}

		++i;
	}

	return false;
}


char const *TextReader::GetFilename()
{
	return m_filename;
}


// Returns the next token on the current line. Strips all separator
// characters from the start and end of the token, so "blah, wibble:7"
// would yield the tokens "blah", "wibble" and "7". If there are 
// trailing separator characters at the end of the line, then they are
// also discarded (the naive implementation would return the empty string
// for the last token)
char *TextReader::GetNextToken()
{
	// Make sure there is more input on the line
	if (m_line[m_tokenIndex] == '\0')
	{
		return NULL;
	}

	// Skip over initial separator characters
	int m_tokenStart = m_tokenIndex;
	while(m_line[m_tokenStart] != '\0' &&
		  strchr(m_seperatorChars, m_line[m_tokenStart]) != NULL)
	{
		m_tokenStart++;
	}

	// Make sure that we haven't found an empty token
	if (m_line[m_tokenStart] == '\0')
	{
		return NULL;
	}

	// Find the end of the token
	m_tokenIndex = m_tokenStart;
	while(m_line[m_tokenStart] != '\0')
	{
		if (strchr(m_seperatorChars, m_line[m_tokenIndex]) != NULL)
		{
			m_line[m_tokenIndex] = '\0';
			m_tokenIndex++;
			break;
		}
		m_tokenIndex++;
	}

	return &m_line[m_tokenStart];
}


char *TextReader::GetRestOfLine()
{
	// Make sure there is more input on the line
	if (m_line[m_tokenIndex] == '\0')
	{
		return NULL;
	}

	// Skip over initial separator characters
	int m_tokenStart = m_tokenIndex;
	while(m_line[m_tokenStart] != '\0' &&
		  strchr(m_seperatorChars, m_line[m_tokenStart]) != NULL)
	{
		m_tokenStart++;
	}

    return &m_line[m_tokenStart];
}



//*****************************************************************************
// Class TextFileReader
//*****************************************************************************

TextFileReader::TextFileReader(char const *_filename)
:	TextReader()
{
	strncpy(m_filename, _filename, sizeof(m_filename) - 1);
	m_file = fopen(_filename, "r");
}

TextFileReader::TextFileReader(std::string const &_filename)
:	TextReader()
{
	strncpy(m_filename, _filename.c_str(), sizeof(m_filename) - 1);
	m_file = fopen(_filename.c_str(), "r");
}


TextFileReader::~TextFileReader()
{
	if( m_file ) fclose(m_file);
}


bool TextFileReader::IsOpen()
{
    return( m_file && m_line );
}


// Reads a line from m_file. Removes comments that are marked with
// a hash ('#', aka the pound sign or indeed the octothorpe) character. 
// Returns 0 on EOF, 1 otherwise
bool TextFileReader::ReadLine()
{
	bool eof = false;
	m_tokenIndex = 0;

	
	//
	// Read some data from the file

	// If fgets returns NULL that means we've found the EOF
	if (fgets(m_line, m_maxLineLen + 1, m_file) == NULL)
	{
		eof = true;
	}

	
	//
	// Make sure we read a whole line

	bool found = false;
	while (!found && !eof)
	{
		if (strchr(m_line, '\n'))
		{
			found = true;
		}

		if (!found)
		{
			unsigned int oldLineLen = m_maxLineLen;
			DoubleMaxLineLen();
			if (fgets(&m_line[oldLineLen], oldLineLen + 1, m_file) == NULL)
			{
				eof = true;
			}
		}
	}

	CleanLine();	
	m_lineNum++;

	return !eof;
}



//*****************************************************************************
// Class TextDataReader
//*****************************************************************************

TextDataReader::TextDataReader(char const *_data, unsigned int _dataSize, char const *_filename)
:	TextReader(),
	m_data(_data),
	m_dataSize(_dataSize),
	m_offset(0)
{
	strncpy(m_filename, _filename, sizeof(m_filename) - 1);
}


bool TextDataReader::IsOpen()
{
    return true;
}


// Reads a line from a block of text data. Removes comments that are marked with
// a hash ('#', aka the pound sign or indeed the octothorpe) character. 
// Returns 0 on EOF, 1 otherwise
bool TextDataReader::ReadLine()
{
	bool eof = false;

	m_tokenIndex = 0;

	// Find the next '\n' character
	unsigned int eolOffset = m_offset;
	for (eolOffset = m_offset; eolOffset < m_dataSize; ++eolOffset)
	{
		if (m_data[eolOffset] == '\n') 
		{
			break;
		}
	}
	if (eolOffset == m_dataSize) 
	{
		eolOffset--;
		eof = true;
	}

	// Make sure the line buffer is big enough to accomodate our painful length
	unsigned int lineLen = eolOffset - m_offset + 1;
	while (lineLen > m_maxLineLen)
	{
		DoubleMaxLineLen();
	}

	// Copy from the data block into m_line
	memcpy(m_line, &m_data[m_offset], lineLen);
	m_line[lineLen] = '\0';

	// Move m_offset on
	m_offset += lineLen;

	CleanLine();
	m_lineNum++;

	return !eof;
}
