#ifndef _included_unicodeTextFileWriter_h
#define _included_unicodeTextFileWriter_h

/*
 *	A thin wrapper for ordinary text file IO
 *  with the added ability to encrypt the data
 *  as it is sent to the file
 *
 */

#include <stdio.h>

#include "lib/filesys/text_file_writer.h"
#include "lib/unicode/unicode_string.h"

class UnicodeTextFileWriter : public TextFileWriter
{
public:
	UnicodeTextFileWriter  (char *_filename, bool _encrypt=false, bool _assertOnFail=true);

	operator bool() const;

	int printf  (wchar_t *fmt, ...);
	int printf	(UnicodeString _str);
};


#endif
