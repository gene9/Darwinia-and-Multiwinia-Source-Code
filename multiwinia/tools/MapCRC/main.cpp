#ifdef _WIN32
	#include <windows.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#include "unrar/crc.h"

#include "lib/universal_include.h"
#include "lib/math/filecrc.h"
#include "lib/tosser/llist.h"
#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/debug_utils.h"


/*
 *      Map CRC Program
 * 
 *      Simple program to run through all files in a directory
 *      and output their filenames + CRCs into a formatted header file
 *      ready for inclusion in Multiwinia
 *
 *
 */
/*
void AppDebugOut( const char *_msg, ... )
{
    char buf[512];
    va_list ap;
    va_start (ap, _msg);
    vsprintf(buf, _msg, ap);

    OutputDebugString(buf);
}
*/

//
// strupr() isn't in POSIX
#ifdef TARGET_OS_MACOSX
inline char * strupr(char *s) {                                       
  for (char *p = s; *p; p++)                                          
	*p = toupper(*p);                                                 
  return s;                                                           
}
#endif


void SanitiseFilename( char *filename )
{
    //
    // Strip closing ".txt"

    int len = strlen(filename);

    filename[ len - 4 ] = '\x0';        


    //
    // Remove bad chars

    for( int i = 0; i < strlen(filename); ++i )
    {
        if( filename[i] == '.' ||
            filename[i] == '-' ||
            filename[i] == ' ' )
        {
            filename[i] = '_';
        }
    }

    //
    // Uppercase

    strupr( filename );
}


int GenerateMapCRCs(char *targetDir, char *outputFile)
{
    //
    // Prepare output structure

    LList<char *> output;
    
    output.PutData( "#ifndef _included_mapcrc_h\n" );
    output.PutData( "#define _included_mapcrc_h\n" );
    output.PutData( "\n" );
	output.PutData( "\n" );
    output.PutData( "// Auto-generated IDs for each map.\n" );
    output.PutData( "// DO NOT MODIFY\n" );
    output.PutData( "\n" );
	output.PutData( "\n" );


    //
    // Parse file list

    LList<char *> *fileList = ListDirectory( targetDir, "*.txt", false );

    for( int i = 0; i < fileList->Size(); ++i )
    {
        char *thisFilename = fileList->GetData(i);        
     
        char fullFilename[512];
        sprintf( fullFilename, "%s%s", targetDir, thisFilename );
        BinaryFileReader reader(fullFilename);
        int crc = FileCRC( &reader );

        SanitiseFilename( thisFilename );

        char resultLine[1024];
        sprintf( resultLine, "#define   MAPID_%-40s 0x%08x\n", thisFilename, crc );
        output.PutData( strdup(resultLine) );

        AppDebugOut( resultLine );
    }

    output.PutData( "\n" );
	output.PutData( "\n" );
    output.PutData( "#endif\n" );


	// Compare output against existing
	FILE *input = fopen( outputFile, "rt" );
	if( input )
	{
		bool differenceFound = false;

		for( int i = 0; i < output.Size(); i++ )
		{
			char line[1024] = "";
			char *result = fgets( line, 1024, input );

			if( !result ||
				strcmp( line, output.GetData(i) ) != 0 )
			{
				differenceFound = true;
				break;
			}
		}

		char line[1024];
		if( !differenceFound && fgets(line, 1024, input ) )
		{
			differenceFound = true;
		}

		fclose(input);

		// No need to generate the file if no differences found
		if( !differenceFound )
		{
			printf("No map changes found.\n");
			return 0;
		}
	}

    //
    // Output results to file

    FILE *file = fopen( outputFile, "wt" );
    if( !file ) return -1;
   
    for( int i = 0; i < output.Size(); ++i )
    {
        fprintf( file, "%s", output[i] );
    }

    fclose( file );


    return 0;
}


#ifdef _WIN32
int WINAPI WinMain(HINSTANCE _hInstance, HINSTANCE _hPrevInstance, 
                   LPSTR _cmdLine, int _iCmdShow)
{
    // 
    // Parse command line
    char *targetDir = _cmdLine;
    char *outputFile = strchr( targetDir, ' ' );
	if( !outputFile ) return -1;
    *outputFile = '\x0';
    outputFile++;
	
	return GenerateMapCRCs(targetDir, outputFile);
}
#else
int main(int argc, char *argv[])
{
	if (argc == 3)
		return GenerateMapCRCs(argv[1], argv[2]);
	else
		return -1;
}
#endif