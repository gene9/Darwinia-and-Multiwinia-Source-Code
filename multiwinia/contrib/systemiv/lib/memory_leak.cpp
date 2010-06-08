#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
using namespace std;

#include "lib/tosser/btree.h"
#include "lib/tosser/llist.h"
#include "lib/filesys/filesys_utils.h"



char *LowerCaseString ( const char *thestring )
{

	char *thecopy = new char [strlen(thestring)+1];
	strcpy ( thecopy, thestring );

	for ( char *p = thecopy; *p != '\x0'; ++p )
		if ( *p >= 'A' && *p <= 'Z' )
			*p += 'a' - 'A';

	return thecopy;

}

void ParseMemoryLeakFile ( char *_inputFilename, char *_outputFilename )
{

    //
    // Start up
    //

    BTree <int> combined;
    BTree <int> frequency;
    int unrecognised = 0;

    //
    // Open the file and start parsing
    //

    ifstream memoryfile ( _inputFilename );

    while ( !memoryfile.eof () ) 
    {
        char thisline [256];
        memoryfile.getline ( thisline, 256 );

        if ( !strncmp ( thisline, " Data:", 6 ) == 0 &&         // This line is a data line - useless to us
              strchr ( thisline, ':' ) ) {                      // This line does not have a source file location - useless to us

            // Get the size

            char *lastcomma = strrchr ( thisline, ',' );
            char *ssize = lastcomma+2;
            int size;
            char unused [32];
            sscanf ( ssize, "%d %s", &size, unused );

            // Get the source file name

            char *sourcelocation = thisline;
            char *colon = strrchr ( thisline, ':' );
            *(colon-1) = '\x0';
            char *lowercasesourcelocation = LowerCaseString ( sourcelocation );
            
            // Put the result into our BTree

            BTree <int> *btree = combined.LookupTree ( lowercasesourcelocation );
            if ( btree ) ((int) btree->data) += size;
            else combined.PutData ( lowercasesourcelocation, size );
            
            BTree <int> *freq = frequency.LookupTree ( lowercasesourcelocation );
            if ( freq ) ((int) freq->data) ++;
            else frequency.PutData ( lowercasesourcelocation, 1 );

            delete lowercasesourcelocation;
        }
        else 
        {
            char *lastcomma = strrchr ( thisline, ',' );
            
            if ( lastcomma ) 
            {

                char *ssize = lastcomma+2;
                int size;
                char unused [32];
                if( sscanf ( ssize, "%d %s", &size, unused ) == 2 )
	                unrecognised += size;
            }
        }
    }

    memoryfile.close ();

    
    //
    // Sort the results into a list
    //

    DArray <int> *sizes = combined.ConvertToDArray ();
    DArray <char *> *sources = combined.ConvertIndexToDArray ();
    LList <char *> sorted;
    int totalsize = 0;

    for ( int i = 0; i < sources->Size (); ++i )
    {
        char *newsource = sources->GetData (i);
        int newsize = sizes->GetData (i);
        totalsize += newsize;
        bool inserted = false;

        for ( int j = 0; j < sorted.Size (); ++j ) {

            char *existingsource = sorted.GetData (j);
            int existingsize = combined.GetData ( existingsource );

            if ( newsize <= existingsize ) {

                sorted.PutDataAtIndex ( newsource, j );
                inserted = true;
                break;

            }

        }

        if ( !inserted ) sorted.PutDataAtEnd ( newsource );
    }


    //
    // Open the output file
    //

    FILE *output = fopen( _outputFilename, "wt" );

    //
    // Print out our sorted list
    // 

    fprintf ( output, "Total recognised memory leaks   : %d Kbytes\n", int(totalsize/1024)  );
    fprintf ( output, "Total unrecognised memory leaks : %d Kbytes\n\n", int(unrecognised/1024) );
    
    for ( int k = sorted.Size () - 1; k >= 0; --k ) 
    {

        char *source = sorted.GetData (k);
        int size = combined.GetData ( source );
        int freq = frequency.GetData ( source );

        if( size > 2048 )
        {
            fprintf ( output, "%-95s (%d Kbytes in %d leaks)\n", source, int(size/1024), freq );
        }
        else
        {
            fprintf ( output, "%-95s (%d  bytes in %d leaks)\n", source, size, freq );
        }
    }


    //
    // Clear up

    fclose( output );

    delete sources;
    delete sizes;
}


void AppPrintMemoryLeaks( char *_filename )
{
    //
    // Print all raw memory leak data to a temporary file

    char tmpFilename[512];
    sprintf( tmpFilename, "%s.tmp", _filename );

    
    HANDLE file = 
		CreateFile( tmpFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
                     
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, file);
  
    _CrtDumpMemoryLeaks ();

    CloseHandle ( file );


    //
    // Parse the temp file into a sensible format

    ParseMemoryLeakFile( tmpFilename, _filename );



    //
    // Delete the temporary file

    //DeleteThisFile( tmpFilename );
   
}
