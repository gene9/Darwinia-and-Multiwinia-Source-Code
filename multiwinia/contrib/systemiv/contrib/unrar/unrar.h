#ifndef INCLUDED_UNRAR_H
#define INCLUDED_UNRAR_H


#define MAX_FILES	16384


class MemMappedFile
{
public:
	char			*m_filename;
	unsigned char	*m_data;
	unsigned int	m_size;

	MemMappedFile(char const *_filename, unsigned int _size);
	~MemMappedFile();
};


class UncompressedArchive
{
public:
	MemMappedFile	*m_files[MAX_FILES];
	unsigned int	m_numFiles;

	// The constructor decompresses a Rar file into memory, creating a big
	// array of pointers to memory mapped files
	UncompressedArchive(char const *_filename,	// eg "yourArchive.rar"
						char const *_password);	// Set to NULL if archive is not encrypted);

	MemMappedFile *AllocNewFile(char const *_filename, unsigned int _size);
};


#endif
