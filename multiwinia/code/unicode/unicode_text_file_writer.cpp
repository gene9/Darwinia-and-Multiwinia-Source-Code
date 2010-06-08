#include "lib/universal_include.h"
#include "lib/debug_utils.h"

#include "unicode/unicode_text_file_writer.h"
#include "lib/filesys/filesys_utils.h"

#include <stdarg.h>
#ifdef TARGET_OS_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
	
	#if __BIG_ENDIAN__
		#define kUTF32Encoding kCFStringEncodingUTF32BE
	#elif __LITTLE_ENDIAN__
		#define kUTF32Encoding kCFStringEncodingUTF32LE
	#endif
#else
	#include <io.h>
#endif
#include <fcntl.h>


static unsigned int s_offsets[] = {
	31, 7, 9, 1, 
	11, 2, 5, 5, 
	3, 17, 40, 12,
	35, 22, 27, 2
}; 


UnicodeTextFileWriter::UnicodeTextFileWriter(char *_filename, bool _encrypt, bool _assertOnFail)
:	TextFileWriter(_filename, false, _assertOnFail)	// TextFileWriter constructor writes redshirt to the file is encyrpting. We'll do it ourselves
{
	if( !m_canWrite )
	{
		return;
	}

#if defined(TARGET_OS_MACOSX)
	CFStringRef str = _encrypt ? CFSTR("redshirt2") : CFSTR("");
	char buf[256];
	CFIndex len = 0;
	char BOM[2] = { 0xFF, 0xFE }; // little-endian
	
	fwrite(&BOM, 1, 2, m_file);
	CFStringGetBytes(str, CFRangeMake(0, CFStringGetLength(str)), kCFStringEncodingUTF16LE,
					 0, false, (UInt8 *)buf, sizeof(buf), &len);
	fwrite(buf, 1, len, m_file);
	
	m_encrypt = _encrypt;
#else
	_setmode(_fileno(m_file), _O_U16TEXT);
	wchar_t BOM[2];
	BOM[0] = 0xFEFF;
	BOM[1] = 0x0;

	if( fputws(BOM, m_file) == WEOF )
	{
		// Error opening file
		fclose(m_file);
		m_file = NULL;
		m_canWrite = false;
	}
	else if (_encrypt)
	{		
        fwprintf(m_file, L"redshirt2");
	}

	m_encrypt = _encrypt;
#endif
}

UnicodeTextFileWriter::operator bool() const
{
	return TextFileWriter::operator bool();
}

int UnicodeTextFileWriter::printf(UnicodeString _str)
{
	return printf(_str.m_unicodestring);
}

int UnicodeTextFileWriter::printf(wchar_t *_fmt, ...)
{
	wchar_t buf[10240];
    va_list ap;
    va_start (ap, _fmt);

    int len = vswprintf(buf, sizeof(buf)/sizeof(wchar_t), _fmt, ap);

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
		return -1;
	}

	#ifdef TARGET_OS_MACOSX
	{
		CFStringRef str;
		CFIndex len;
		short utf16buf[10240*2];
		
		str = CFStringCreateWithBytes(NULL, (const UInt8 *)buf, wcslen(buf)*sizeof(wchar_t), kUTF32Encoding, false);
		CFStringGetBytes(str, CFRangeMake(0, CFStringGetLength(str)), kCFStringEncodingUTF16LE,
						 0, true, (UInt8 *)utf16buf, sizeof(utf16buf), &len);
		CFRelease(str);
		fwrite(utf16buf, 1, len, m_file);
		return ferror(m_file) ? -1 : 0;
	}
	#else
		return fputws( buf, m_file );
	#endif
}
