#include "lib/universal_include.h"
#include "lib/debug_utils.h"

#include "text_file_writer.h"
#include "filesys_utils.h"

#include <stdarg.h> 
#include <sstream>

static unsigned int s_offsets[] = {
	31, 7, 9, 1, 
	11, 2, 5, 5, 
	3, 17, 40, 12,
	35, 22, 27, 2
}; 

// ===== TEXT WRITER BASE CLASS
TextWriter::operator bool() const
{
    return false;
}
// =====


// TextFileWriter ==================================================================

TextFileWriter::TextFileWriter(char *_filename, bool _encrypt, bool _assertOnFail)
:	TextWriter(),
    m_offsetIndex(0),
	m_encrypt(_encrypt),
	m_canWrite(false),
	m_assertOnFail(_assertOnFail)
{
	std::string filename = FindCaseInsensitive( _filename );
	m_file = fopen( filename.c_str(), "w" );

	if( m_file )
	{
		m_canWrite = true;
	}
	else if( m_assertOnFail )
	{
		AppReleaseAssert(m_file, "Couldn't create file %s", _filename);
	}
	else
	{
		AppDebugOut("Couldn't create file %s\n", _filename);
	}

	if (m_canWrite && _encrypt)
	{
		fprintf( m_file, "redshirt2" );
	}
}


TextFileWriter::~TextFileWriter()
{
	if( !m_canWrite )
	{
		return;
	}

	fclose(m_file);
}

TextFileWriter::operator bool() const
{
	return m_canWrite;
}

int TextFileWriter::printf(char *_fmt, ...)
{
	char buf[10240];
    va_list ap;
    va_start (ap, _fmt);

    int len = vsprintf(buf, _fmt, ap);

	if (m_encrypt)
	{
		for (int i = 0; i < len; ++i)
		{
			if (buf[i] > 32) {
				m_offsetIndex++;
				m_offsetIndex %= 16;
				int j = buf[i] + s_offsets[m_offsetIndex];
				if (j >= 128) j -= 95;
				buf[i] = j;
			}
		}
	}

	if( !m_canWrite )
	{
		//AppDebugOut("Can't write at the moment - %s\n", buf);
		return -1;
	}

	return fputs( buf, m_file );
}

// TextMemoryWriter ===============================================================

TextMemoryWriter::TextMemoryWriter()
:   TextWriter()
{    
}

TextMemoryWriter::operator bool() const
{
    return m_stringStream.good();
}

int TextMemoryWriter::printf(char *_fmt, ...)
{
    char buf[10240];
    va_list ap;
    va_start (ap, _fmt);

    int len = vsprintf(buf, _fmt, ap);

    m_stringStream.write( buf, len );

    return 1;
}

std::ostringstream *TextMemoryWriter::GetStream()
{
    return &m_stringStream;
}