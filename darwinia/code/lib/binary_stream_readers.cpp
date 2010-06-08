#include "lib/universal_include.h"

#ifdef WIN32
#include <io.h>
#endif

#include <stdio.h>
#include <string.h>

#include "lib/debug_utils.h"
#include "lib/binary_stream_readers.h"


// ****************************************************************************
// BinaryReader
// ****************************************************************************

BinaryReader::BinaryReader()
:	m_eof(false)
{
	m_filename[0] = '\0';
}


BinaryReader::~BinaryReader()
{
}



// ****************************************************************************
// BinaryFileReader
// ****************************************************************************

BinaryFileReader::BinaryFileReader(char const *_filename)
:	BinaryReader()
{
	if (_filename)
	{
		strncpy(m_filename, _filename, sizeof(m_filename) - 1);
		m_file = fopen(_filename, "rb");
		DarwiniaDebugAssert(m_file);
	}
}


BinaryFileReader::~BinaryFileReader()
{
	fclose(m_file);
}


bool BinaryFileReader::IsOpen()
{
	if (m_file) return true;
	return false;
}


char *BinaryFileReader::GetFileType()
{
	char *extension = strrchr(m_filename, '.');
	if (extension)
	{
		return extension + 1;
	}
	
	return &m_filename[strlen(m_filename)];
}


signed char BinaryFileReader::ReadS8()
{
	int c = fgetc(m_file);
	if (c == EOF)
	{
		m_eof = true;
	}
	return c;
}


unsigned char BinaryFileReader::ReadU8()
{
	int c = fgetc(m_file);
	if (c == EOF)
	{
		m_eof = true;
	}
	return c;
}


short BinaryFileReader::ReadS16()
{
	int b1 = fgetc(m_file);
	int b2 = fgetc(m_file);
	
	if (b1 == EOF || b2 == EOF)
	{
		m_eof = true;
	}

	return ((b2 << 8) | b1);
}


int BinaryFileReader::ReadS32()
{
	int b1 = fgetc(m_file);
	int b2 = fgetc(m_file);
	int b3 = fgetc(m_file);
	int b4 = fgetc(m_file);

	if (b1 == EOF || b2 == EOF || b3 == EOF || b4 == EOF)
	{
		m_eof = true;
	}

	return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}


unsigned int BinaryFileReader::ReadBytes(unsigned int _count, unsigned char *_buffer)
{
	int bytesRead = fread(_buffer, 1, _count, m_file);
	if (bytesRead < _count)
	{
		m_eof = true;
	}

	return bytesRead;
}


int BinaryFileReader::Seek(int _offset, int _origin)
{
	return fseek(m_file, _offset, _origin);
}


int BinaryFileReader::Tell()
{
	return ftell(m_file);
}


// ****************************************************************************
// BinaryDataReader
// ****************************************************************************

BinaryDataReader::BinaryDataReader(unsigned char const *_data, unsigned int _dataSize, 
								   char const *_filename)
:	BinaryReader(),
	m_data(_data),
	m_dataSize(_dataSize),
	m_offset(0)
{
	strncpy(m_filename, _filename, sizeof(m_filename) - 1);
}


BinaryDataReader::~BinaryDataReader()
{
}


bool BinaryDataReader::IsOpen()
{
	return true;
}


char *BinaryDataReader::GetFileType()
{
	char *extension = strrchr(m_filename, '.');
	if (extension)
	{
		return extension + 1;
	}
	
	return &m_filename[strlen(m_filename)];
}


signed char BinaryDataReader::ReadS8()
{
	if (m_offset >= m_dataSize)
	{
		m_eof = true;
		return EOF;
	}

	return m_data[m_offset++];
}


unsigned char BinaryDataReader::ReadU8()
{
	if (m_offset >= m_dataSize)
	{
		m_eof = true;
		return EOF;
	}

	return m_data[m_offset++];
}


short BinaryDataReader::ReadS16()
{
	if (m_offset >= m_dataSize - 1)
	{
		m_eof = true;
		return 0;
	}

	int b1 = m_data[m_offset++];
	int b2 = m_data[m_offset++];
	
	return ((b2 << 8) | b1);
}


int BinaryDataReader::ReadS32()
{
	if (m_offset >= m_dataSize - 3)
	{
		m_eof = true;
		return 0;
	}

	int b1 = m_data[m_offset++];
	int b2 = m_data[m_offset++];
	int b3 = m_data[m_offset++];
	int b4 = m_data[m_offset++];

	if (b1 == EOF || b2 == EOF || b3 == EOF || b4 == EOF)
	{
		m_eof = true;
	}

	return ((b4 << 24) | (b3 << 16) | (b2 << 8) | b1);
}


unsigned int BinaryDataReader::ReadBytes(unsigned int _count, unsigned char *_buffer)
{
	if (m_eof) 
	{
		return 0;
	}

	for (unsigned int i = 0; i < _count; ++i)
	{
		_buffer[i] = ReadU8();
		if (m_offset >= m_dataSize)
		{
			m_eof = true;
			return i + 1;
		}
	}

	return _count;
}


int BinaryDataReader::Seek(int _offset, int _origin)
{
	switch (_origin)
	{
	case SEEK_CUR:
		m_offset += _offset;
		break;
	case SEEK_END:
		m_offset = m_dataSize - _offset;	// It isn't clear from the VC++ docs whether there should be a -1 here
		break;
	case SEEK_SET:
		m_offset = _offset;
		break;
	}

	if (m_offset >= m_dataSize)
	{
		m_eof = true;
	}
	else
	{
		m_eof = false;
	}

	return 0;
}


int BinaryDataReader::Tell()
{
	return m_offset;
}

int BinaryReader::GetSize()
{
	int now = Tell();
	Seek(0,SEEK_END);
	int size = Tell();
	Seek(now,SEEK_SET);
	return size;
}
