#include "lib/universal_include.h"

#include <string.h>

#include "unrar/crc.h"

#include "lib/filesys/binary_stream_readers.h"


int ReadFileData( BinaryReader *reader, int maxSize, char *buffer )
{
    int bufferIndex = 0;
    
    while( reader->IsOpen() &&
        !reader->m_eof )
    {
        buffer[bufferIndex] = reader->ReadU8();
        bufferIndex++;

        if( bufferIndex > maxSize ) break;
    }

    return bufferIndex;
}


int FileCRC( BinaryReader *reader )
{
    //
    // Buffer to read file into

    #define MAXSIZE 100000
    static char s_buffer[MAXSIZE];
    memset( s_buffer, 0, MAXSIZE );

    //
    // Read file into buffer

    int filesize = ReadFileData(reader, MAXSIZE, s_buffer );


    //
    // CRC buffer

    int crc = CRC(0, s_buffer, filesize );
    
    return crc;
}


