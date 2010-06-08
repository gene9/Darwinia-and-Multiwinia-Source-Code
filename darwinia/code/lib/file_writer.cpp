#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/file_writer.h"


static unsigned int s_offsets[] = {
	31, 7, 9, 1, 
	11, 2, 5, 5, 
	3, 17, 40, 12,
	35, 22, 27, 2
}; 


FileWriter::FileWriter(char const *_filename, bool _encrypt)
:	m_offsetIndex(0),
	m_encrypt(_encrypt)
{
	m_file = fopen(_filename, "w");
	
	DarwiniaReleaseAssert(m_file, "Couldn't create file %s", _filename);

	if (_encrypt)
	{
		fprintf(m_file, "redshirt2");
	}
}


FileWriter::~FileWriter()
{
	fclose(m_file);
}


int FileWriter::printf(char const *_fmt, ...)
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

	return fprintf(m_file, "%s", buf);
}
