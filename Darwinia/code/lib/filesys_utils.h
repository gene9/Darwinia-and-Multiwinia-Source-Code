#ifndef INCLUDED_FILESYS_UTILS
#define INCLUDED_FILESYS_UTILS

#include <stdio.h>

#include "lib/llist.h"


//*****************************************************************************
// Misc directory and filename functions
//*****************************************************************************

LList <char *> *ListDirectory           (char const *_dir, char const *_filter, bool fullFilename = true);
LList <char *> *ListSubDirectoryNames   (char const *_dir);

bool DoesFileExist(char const *_fullPath);

char const *GetDirectoryPart    (char const *_fullFilePath);
char const *GetFilenamePart     (char const *_fullFilePath);
char const *GetExtensionPart    (char const *_fileFilePath);
char const *RemoveExtension     (char const *_fullFileName);

bool AreFilesIdentical          (char const *_name1, char const *_name2);

bool CreateDirectory            (char const *_directory);
void DeleteThisFile             (char const *_filename);


class EncryptedFileWriter
{
protected:
	int m_offsetIndex;
	FILE *m_out;

public:
	EncryptedFileWriter(char const *_name);
	~EncryptedFileWriter();

	void WriteLine(char *_line);	// _line gets modified
};

#endif
