#include "lib/universal_include.h"
#include "lib/math/vector3.h"
#include "lib/tosser/directory.h"
#include "lib/string_utils.h"
#include "lib/network_stream.h"

#include <ios>

#ifdef TARGET_OS_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
	#include "lib/unicode/unicode_string.h"
#endif

#ifdef TARGET_OS_LINUX
#include <iconv.h>
#endif

char *ReadDynamicString ( std::istream &input, int maxStringLength, const char *safeString )
{
	int size = ReadPackedInt(input);

	char *string;

	if ( size == -1 ) 
    {
		string = NULL;
	}
    else if ( size < 0 ||
              size > maxStringLength )
    {        
        string = newStr(safeString);        
    }    
	else 
    {
		string = new char [size+1];
		input.read( string, size );
		string[size] = '\x0';
	}

	return string;
}


void WriteDynamicString ( std::ostream &output, const char *string )
{
	if ( string ) 
    {
		int size = strlen(string);
		WritePackedInt(output, size);
        output.write( string, size );
	}
	else 
    {
		int size = -1;
		WritePackedInt(output, size);
	}
}

// Reads a length field + UTF-16 stream into a wchar_t
wchar_t *ReadUnicodeString( std::istream &input, int maxLength, const wchar_t *safeString )
{
	int size = ReadPackedInt(input);

	if ( size == -1 ) 
    {
		return NULL;
	}
    else if ( size < 0 ||
              size > maxLength )
    {        
        return newStr(safeString);        
    }    
	else 
    {
#ifdef TARGET_OS_MACOSX
		UniChar *unichars = new UniChar[size];
		wchar_t *wchars = new wchar_t[size+1];
		CFStringRef utf16;
		
		for(int i = 0; i < size; i++)
			ReadNetworkValue( input, unichars[i] );
		
		utf16 = CFStringCreateWithCharactersNoCopy(NULL, unichars, size, kCFAllocatorNull);
		CFStringGetBytes(utf16, CFRangeMake(0, CFStringGetLength(utf16)), kUTF32Encoding, '?', false,
						 (UInt8 *)wchars, (size+1)*sizeof(wchar_t), NULL);
		wchars[size] = '\x0';
		CFRelease(utf16);
		delete unichars;
		
		return wchars;
#elif defined( TARGET_OS_LINUX )
		size_t inBytesLeft = size * sizeof(unsigned short);
		char *utf16 = new char[inBytesLeft];
		input.read( utf16, inBytesLeft );

		char *inBuf = utf16;

		size_t bufferSize = size + 1;
		size_t outBytesLeft = bufferSize * 4;
		wchar_t *buf = new wchar_t[ bufferSize ];
		char *outBuf = (char *) buf;

		buf[0] = L'\0';

		iconv_t utf16_to_utf32 = iconv_open( "UTF32LE", "UTF16LE" );
		if( utf16_to_utf32 == (iconv_t) -1 )
		{
			perror( "Failed to open iconv" );
			delete[] utf16;
			return buf;
		}

		size_t result = iconv( utf16_to_utf32, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft );
		iconv_close( utf16_to_utf32 );

		if( result == (size_t) -1 )
		{
			perror( "Failed to convert stream to utf32le" );
			delete[] utf16;
			return buf;
		}

		delete[] utf16;		

		buf[size] = L'\x0';
		return (wchar_t *) buf;
#else
		wchar_t *string = new wchar_t [size+1];
		for(int i = 0; i < size; i++)
			ReadNetworkValue( input, string[i] );
		string[size] = '\x0';
		return string;
#endif
	}
}

// Writes a wchar_t out as length field + UTF-16 stream
void WriteUnicodeString ( std::ostream &output, const wchar_t *unicode )
{
	if ( unicode ) 
    {
#ifdef TARGET_OS_MACOSX
		// wchar_t = UTF-32, so we need to convert
		CFStringRef utf16 = CFStringCreateWithBytes(NULL, (UInt8 *)unicode, wcslen(unicode)*sizeof(wchar_t),
													kUTF32Encoding, false);
		int size = CFStringGetLength(utf16);
		WritePackedInt(output, size);
        for(int i = 0; i < size; ++ i)
        {
			WriteNetworkValue( output, CFStringGetCharacterAtIndex(utf16, i) );
        }
		
		CFRelease(utf16);
#elif defined( TARGET_OS_LINUX )
		// wchar_t is 4 byes on linux, so we need to convert
		int size = wcslen(unicode);

		size_t bufferSize = sizeof(unsigned short) * size * 2;
		size_t outBytesLeft = bufferSize;
		size_t inBytesLeft = size * sizeof(wchar_t);

		char *inBuf = (char *) unicode;
		char *buf = new char[ outBytesLeft ];
		char *outBuf = buf;

		iconv_t utf32_to_utf16 = iconv_open( "UTF16LE", "UTF32LE" );
		if( utf32_to_utf16 == (iconv_t) -1 )
		{
			perror("Failed to open iconv from UTF32LE -> UTF16LE" );
			delete[] buf;
			return;
		}
		size_t result = iconv( utf32_to_utf16, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft );
		if( result == (size_t) -1 )
		{
			perror( "Failed to convert from UTF32LE -> UTF16LE" );
			delete[] buf;
			return;
		}
		iconv_close( utf32_to_utf16 );

		int bytesConverted = bufferSize - outBytesLeft;
		WritePackedInt(output, bytesConverted / sizeof(unsigned short) );
		output.write( buf, bytesConverted );
		delete[] buf;
#else
		// assume Windows and wchar_t = UTF-16
		int size = wcslen(unicode);
		WritePackedInt(output, size);
        for(int i = 0; i < size; ++ i)
        {
			WriteNetworkValue( output, unicode[i] );
        }
#endif
	}
	else 
    {
		int size = -1;
		WritePackedInt(output, size);
	}
}


void WritePackedInt( std::ostream &output, int _value )
{
    // The first byte can be either:
    // 0 - 254 inclusive : that value           OR    
    // 255 : int follows (4 bytes)

    if( _value >= 0 && _value < 255 )
    {
        unsigned char byteValue = (unsigned char)_value;
        output.write( (char *) &byteValue, sizeof(byteValue) );
    }
    else
    {
        unsigned char code = 255;
        output.write( (char *) &code, sizeof(code) );
		WriteNetworkValue(output,_value);
    }    
}


int ReadPackedInt( std::istream &input )
{
    unsigned char code;
    input.read( (char *) &code, sizeof(code) );

    if( code == 255 )
    {
        // The next 4 bytes are a full int
        int fullValue;
		ReadNetworkValue( input, fullValue );
        return fullValue;
    }
    else
    {
        // This is the value
        return (int)code;
    }
}


void WriteVoidData( std::ostream &output, const void *data, int dataLen )
{
    WriteNetworkValue( output, dataLen );
    output.write( (const char *)data, dataLen );    
}


void *ReadVoidData( std::istream &input, int *dataLen )
{
    ReadNetworkValue( input, *dataLen );    
    void *data = new char[*dataLen];
    input.read( (char *)data, *dataLen );

    return data;
}


std::istream &ReadNetworkValue( std::istream &input, Vector3 &v )
{
	ReadNetworkValue(input, v.x);
	ReadNetworkValue(input, v.y);
	return ReadNetworkValue(input, v.z);
}


std::ostream &WriteNetworkValue( std::ostream &output, const Vector3 &v )
{
	WriteNetworkValue(output, v.x);
	WriteNetworkValue(output, v.y);
	return WriteNetworkValue(output, v.z);
}
