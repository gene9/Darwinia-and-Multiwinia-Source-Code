#ifndef FILE_WRITER_H
#define FILE_WRITER_H

#include <stdio.h>


class FileWriter
{
protected:
	int		m_offsetIndex;
	FILE	*m_file;
	bool	m_encrypt;

public:
	FileWriter(char const *_filename, bool _encrypt);
	~FileWriter();

	int printf(char const *fmt, ...);
};


#endif
