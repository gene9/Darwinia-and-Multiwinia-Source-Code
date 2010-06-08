#include "lib/universal_include.h"

#include "unicode_text_stream_reader.h"

#include "lib/debug_utils.h"
#include "lib/filesys/filesys_utils.h"

#define DEFAULT_SEPERATOR_CHARS " \f\t\n\r:,"
#define DEFAULT_SEPERATOR_WCHARS L" \f\t\n\r:,"
#define INITIAL_LINE_LEN 128

//*****************************************************************************
// Class UnicodeTextReader
//*****************************************************************************

static unsigned int s_offsets[] = {
	31, 7, 9, 1, 
	11, 2, 5, 5, 
	3, 17, 40, 12,
	35, 22, 27, 2
}; 

UnicodeTextReader::UnicodeTextReader()
:	TextReader(),
	m_unicodeline(UnicodeString(INITIAL_LINE_LEN+1)),
	m_restOfUnicodeLine(UnicodeString())
{
	wcscpy(m_seperatorWchars, DEFAULT_SEPERATOR_WCHARS);
	m_maxLineLen = INITIAL_LINE_LEN;
	delete [] m_line; // JK: Release memory allocated in TextReader's constructor
	m_line = NULL;
	m_isunicode = true;
}

UnicodeTextReader::~UnicodeTextReader()
{
	m_line = NULL;
}

void UnicodeTextReader::DoubleMaxLineLen()
{
	AppReleaseAssert(m_maxLineLen < 65536, "Text file contains line with more than 65536 characters");
	// Increase unicode size
	/*bool bigEndian = m_unicodeline.m_isBigEndian;
	m_unicodeline = UnicodeString(m_maxLineLen * 2 + 1, m_unicodeline.m_unicodestring, m_maxLineLen+1);
	m_unicodeline.m_isBigEndian = bigEndian;
	*/
	m_unicodeline.Resize(m_maxLineLen * 2 + 1);

	// Get the new char* line from the UnicodeString
	m_line = const_cast<char *>((const char *) m_unicodeline);

	m_maxLineLen *= 2;
}

bool UnicodeTextReader::IsOpen() const
{
	return false;
}

bool UnicodeTextReader::ReadLine()
{
	m_unicodeline = UnicodeString();
	return false;
}

bool UnicodeTextReader::TokenAvailable()
{
	unsigned int i = m_tokenIndex;
	
	while (m_unicodeline.m_unicodestring[i] != L'\0')
	{
		if (wcschr(m_seperatorWchars, m_unicodeline.m_unicodestring[i]) == NULL)
		{
			return true;
		}

		++i;
	}

	return false;
}

const UnicodeString& UnicodeTextReader::GetNextUnicodeToken()
{
	// Make sure there is more input on the line
	if (m_unicodeline.m_unicodestring[m_tokenIndex] == L'\0')
	{
		m_unicodeToken = UnicodeString();
		return m_unicodeToken;
	}

	// Skip over initial separator characters
	int m_tokenStart = m_tokenIndex;
	while(m_unicodeline.m_unicodestring[m_tokenStart] != L'\0' &&
		wcschr(m_seperatorWchars, m_unicodeline.m_unicodestring[m_tokenStart]) != NULL)
	{
		m_tokenStart++;
	}

	// Make sure that we haven't found an empty token
	if (m_unicodeline.m_unicodestring[m_tokenStart] == L'\0')
	{
		m_unicodeToken = UnicodeString();
		return m_unicodeToken;
	}

	// Find the end of the token
	m_tokenIndex = m_tokenStart;
	while(m_unicodeline.m_unicodestring[m_tokenStart] != L'\0')
	{
		if (wcschr(m_seperatorWchars, m_unicodeline.m_unicodestring[m_tokenIndex]) != NULL)
		{
			m_unicodeline.m_unicodestring[m_tokenIndex] = L'\0';
			m_tokenIndex++;
			break;
		}
		m_tokenIndex++;
	}

	m_unicodeToken = UnicodeString(&m_unicodeline.m_unicodestring[m_tokenStart]);
	return m_unicodeToken;
}

const UnicodeString& UnicodeTextReader::GetRestOfUnicodeLine()
{
	// Make sure there is more input on the line
	if (m_unicodeline.m_unicodestring[m_tokenIndex] == L'\0')
	{
		m_restOfUnicodeLine = UnicodeString();
		return m_restOfUnicodeLine;
	}

	// Skip over initial separator characters
	int m_tokenStart = m_tokenIndex;
	while(m_unicodeline.m_unicodestring[m_tokenStart] != L'\0' &&
		  wcschr(m_seperatorWchars, m_unicodeline.m_unicodestring[m_tokenStart]) != NULL)
	{
		m_tokenStart++;
	}

	m_restOfUnicodeLine = UnicodeString(&m_unicodeline.m_unicodestring[m_tokenStart]);
	return m_restOfUnicodeLine;
}

wchar_t* UnicodeTextReader::ReadUnicodeLine( wchar_t *_ws, size_t _n )
{
	if (_n > 0)
	{
		_ws[0] = L'\0';
	}

	return _ws;
}

WCHAR UnicodeTextReader::ReadUnicodeChar ()
{
	return L'\0';
}

void UnicodeTextReader::SetSeperatorChars( char const *_seperatorChars )
{
	strncpy(m_seperatorChars, _seperatorChars, 15);
	for (int i = 0; i < strlen(_seperatorChars); i++)
	{
		m_seperatorWchars[i] = (WCHAR)_seperatorChars[i];
	}
}

void UnicodeTextReader::SetDefaultSeperatorChars()
{
	strcpy(m_seperatorChars, DEFAULT_SEPERATOR_CHARS);
	wcscpy(m_seperatorWchars, DEFAULT_SEPERATOR_WCHARS);
}

void UnicodeTextReader::CleanLine()
{
	//
	// Decryption stuff
	
	if (m_fileEncrypted != 0)
	{
		size_t len = wcslen(m_unicodeline.m_unicodestring);

		// Check if file is encrypted
		if (m_fileEncrypted == -1)
		{
			if (m_unicodeline.StrOffsetCmp(L"redshirt2", 9, true) == 0)
			{
				m_fileEncrypted = 1;
				for (size_t i = 9; i < len; ++i)
				{
					m_unicodeline.m_unicodestring[i - 9] = m_unicodeline.m_unicodestring[i]; 
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
			for (size_t i = 0; i < len; ++i)
			{
				if (m_unicodeline.m_unicodestring[i] > 32) {
					m_offsetIndex++;
					m_offsetIndex %= 16;
					int j = m_unicodeline.m_unicodestring[i] - s_offsets[m_offsetIndex];
					if (j < 33) j += 95;
					m_unicodeline.m_unicodestring[i] = j;
				}
			}
		}
	}

	//
	// Scan for comments (which we remove) and merge conflict markers (which we assert on)
	
	int numAdjacentAngleBracketsFound = 0;
	wchar_t *c = m_unicodeline;
	while (c[0] != L'\0')
	{
		if (c[0] == L'<' || c[0] == L'>')
		{
			numAdjacentAngleBracketsFound++;
			AppDebugAssert(numAdjacentAngleBracketsFound < 3);
            // Looks like you've got a merge error mate
		}
		else
		{
			numAdjacentAngleBracketsFound = 0;
			if (c[0] == L'#')
			{
				c[0] = L'\0';
				break;
			}
		}
		c++;
	}

	// Get rid of \n \r 
	wchar_t* p = wcschr(m_unicodeline.m_unicodestring,'\n');
	if( p ) *p = '\0';

	p = wcschr(m_unicodeline.m_unicodestring,'\r');
	if( p ) *p = '\0';
}

//*****************************************************************************
// Class UnicodeTextFileReader
//*****************************************************************************

// If this UnicodeTextReader returns that it is not unicode (IsUnicode() = false)
// then the opened file isn't a unicode file, please deal with it.
UnicodeTextFileReader::UnicodeTextFileReader(char const *_filename)
:	UnicodeTextReader()
{
	m_filename = FindCaseInsensitive( _filename );
	m_file = fopen(m_filename.c_str(), "rb");

	if (!m_file)
	{
		m_isunicode = false;
		return;
	}

	//
	// Read in the Byte-Order Marker.
	// If it doesn't contain this, we just fail.
	int BOM[2];
	BOM[0] = fgetc(m_file);
	BOM[1] = fgetc(m_file);

	if (BOM[0] == 0xFF && BOM[1] == 0xFE)
	{
		// Little Endian
		m_unicodeline.m_isBigEndian = false;
		m_isunicode = true;
	}
	else if (BOM[0] == 0xFE && BOM[1] == 0xFF)
	{
		// Big Endian
		m_unicodeline.m_isBigEndian = true;
		m_isunicode = true;
	}
	else
	{
		// This isn't a unicodefile, so tell someone.
		m_isunicode = false;
	}
}

UnicodeTextFileReader::~UnicodeTextFileReader()
{
	if( m_file ) fclose(m_file);
}

bool UnicodeTextFileReader::IsOpen() const
{
	return( m_isunicode && m_file && m_unicodeline.m_unicodestring );
}

// Reads a line from m_file. Removes comments that are marked with
// a hash ('#', aka the pound sign or indeed the octothorpe) character. 
// Returns 0 on EOF, 1 otherwise
bool UnicodeTextFileReader::ReadLine()
{
	bool eof = false, found = false;
	m_tokenIndex = 0;

	//
	// Read some data from the file

	// If fgets returns NULL that means we've found the EOF
	if (ReadUnicodeLine(m_unicodeline.m_unicodestring, m_maxLineLen+1) == NULL)
	{
		eof = true;
	}

	//
	// Make sure we read a whole line

	while (!found && !eof)
	{
		if (wcschr(m_unicodeline.m_unicodestring, L'\n'))
		{
			found = true;
		}

		if (!found)
		{
			unsigned int oldLineLen = m_maxLineLen;
			DoubleMaxLineLen();
			if (ReadUnicodeLine(&m_unicodeline.m_unicodestring[oldLineLen], oldLineLen+1) == NULL)
			{
				eof = true;
			}
			
		}
	}

	m_unicodeline.CopyUnicodeToCharArray();
	m_line = const_cast<char *>((const char *) m_unicodeline);
	CleanLine();	
	m_lineNum++;

	return !eof;
}

wchar_t* UnicodeTextFileReader::ReadUnicodeLine( wchar_t *_ws, size_t _n )
{
	WCHAR temp;
	int i;
	for (i = 0; i < _n-1; i++)
	{
		temp = ReadUnicodeChar();

		if (temp != NULL)
		{
 			_ws[i] = temp;
		}
		else
		{
			if (feof(m_file))
			{
				return NULL;
			}
			else if (ferror(m_file))
			{
				// TODO: error checking!
				return NULL; // Breakpointable
			}
		}

		if (_ws[i] == L'\n')
		{
			i++;
			break;
		}
	}

	_ws[i] = L'\0';

	return _ws;
}

WCHAR UnicodeTextFileReader::ReadUnicodeChar ()
{
	int first = fgetc(m_file);
	int second;
	WCHAR ret = NULL;

	if (first != EOF)
		second = fgetc(m_file);
	
	if (first != EOF && second != EOF)
	{
		if (m_unicodeline.m_isBigEndian)
		{
			ret = (first << 8) | second;
		}
		else
		{
			ret = (second << 8) | first;
		}
	}

	return ret;
}


//*****************************************************************************
// Class UnicodeTextDataReader
//*****************************************************************************

UnicodeTextDataReader::UnicodeTextDataReader(char const *_data, unsigned int _dataSize, char const *_filename)
:	UnicodeTextReader(),
	m_data(_data),
	m_dataSize(_dataSize),
	m_offset(0)
{
	m_filename = _filename;

	//
	// Read in the Byte-Order Marker.
	// If it doesn't contain this, we just fail.
	if ((unsigned char)m_data[0] == 0xFF && (unsigned char)m_data[1] == 0xFE)
	{
		// Little Endian
		m_unicodeline.m_isBigEndian = false;
		m_isunicode = true;
		m_offset = 2;
	}
	else if ((unsigned char)m_data[0] == 0xFE && (unsigned char)m_data[1] == 0xFF)
	{
		// Big Endian
		m_unicodeline.m_isBigEndian = true;
		m_isunicode = true;
		m_offset = 2;
	}
	else
	{
		// This isn't a unicodefile, so tell someone.
		m_isunicode = false;
	}
}


bool UnicodeTextDataReader::IsOpen() const
{
    return m_data != NULL;
}


// Reads a line from a block of text data. Removes comments that are marked with
// a hash ('#', aka the pound sign or indeed the octothorpe) character. 
// Returns 0 on EOF, 1 otherwise
bool UnicodeTextDataReader::ReadLine()
{
	bool eof = false;

	m_tokenIndex = 0;

	// Find the next '\n' character
	unsigned int eolOffset = m_offset;
	for (eolOffset = m_offset; eolOffset < m_dataSize; eolOffset+=2)
	{
		WCHAR character;
		if (m_unicodeline.m_isBigEndian)
		{
			character = ((wchar_t)m_data[eolOffset] << 8) | m_data[eolOffset+1];
		}
		else
		{
			character = ((wchar_t)m_data[eolOffset+1] << 8) | m_data[eolOffset];
		}

		if (character == L'\n') 
		{
			break;
		}
	}

	if (eolOffset >= m_dataSize) 
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

	// Copy from the data block into a wchar array
	wchar_t* tempArray = new wchar_t[(lineLen/2)+1];
	ReadUnicodeLine(tempArray, lineLen/2);
	tempArray[(lineLen/2)] = L'\0';
	UnicodeString tempString(tempArray);
	tempString.m_isBigEndian = m_unicodeline.m_isBigEndian;
	m_unicodeline = tempString;
	delete [] tempArray;

	CleanLine();
	m_unicodeline.CopyUnicodeToCharArray();
	m_line = const_cast<char *>((const char *) m_unicodeline);
	m_lineNum++;
	m_offset+=2;	// Ignore the newline!

	return !eof;
}

wchar_t* UnicodeTextDataReader::ReadUnicodeLine(wchar_t *_ws, size_t n)
{
	for (int i = 0; i < n; i++)
	{
		_ws[i] = ReadUnicodeChar();
	}
	return _ws;
}

WCHAR UnicodeTextDataReader::ReadUnicodeChar()
{
	unsigned char first = m_data[m_offset], second = m_data[m_offset+1];

	wchar_t ret;
	if (m_unicodeline.m_isBigEndian)
	{
		ret = ((wchar_t)first << 8) | second;
	}
	else
	{
		ret = ((wchar_t)second << 8) | first;
	}
	// Increase the m_offset by 2 (as we read two char's for every wchar)
	m_offset += 2;
	return ret;
}
