#include "lib/universal_include.h"

#ifdef BENCHMARK_AND_FTP

#include <time.h>
#ifdef _WIN32
	#include <shellapi.h>
#endif

#include "lib/filesys/filesys_utils.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/hi_res_time.h"
#include "lib/metaserver/authentication.h"
#include "lib/preferences.h"
#include "lib/preference_names.h"

#include "benchmark.h"
#include "app.h"
#include "renderer.h"
#include "multiwinia.h"

#include "network/clienttoserver.h"
#include "network/server.h"


BenchMark::BenchMark()
:   m_timer(0.0f),
    m_recording(false)
{
}

BenchMark::~BenchMark()
{
	m_dxDiag.EmptyAndDelete();
}

void BenchMark::RequestDXDiag()
{
    int result = -1;
#ifdef TARGET_MSVC
    __try
#endif
    {
        char fullFileName[512];
        snprintf( fullFileName, sizeof(fullFileName), "/t %sdxdiag.txt", g_app->GetProfileDirectory() );
        fullFileName[ sizeof(fullFileName) - 1 ] = '\0';
        result = (int)ShellExecute( NULL, "open", "dxdiag.exe", fullFileName, NULL, SW_SHOWNORMAL );
    }
#ifdef TARGET_MSVC
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        AppDebugOut( "Requesting DXDiag...caught exception, continue\n" );
    }
#endif
    AppDebugOut( "Requesting DXDiag...result code %d\n", result );
}


bool BenchMark::BuildDxDiag()
{
    char fullFileName[512];
    snprintf( fullFileName, sizeof(fullFileName), "%sdxdiag.txt", g_app->GetProfileDirectory() );
    fullFileName[ sizeof(fullFileName) - 1 ] = '\0';
    TextFileReader reader( fullFileName );
    if( !reader.IsOpen() ) return false;

    m_dxDiag.EmptyAndDelete();

    int sectionId = 0;                      // 1 = System Information
                                            // 2 = Display Devices
                                            // 3 = Sound Devices

    LList<char *> permittedNames;

	char *line = NULL;


    while( reader.IsOpen() && reader.ReadLine() && (line = reader.GetRestOfLine()) )
    {
        //
        // Is this line a header?
        // If so, populate our permittedNames list

        if( strstr( line, "System Information" ) )
        {
            sectionId = 1;

            permittedNames.Empty();
            permittedNames.PutData( "Operating System" );
            permittedNames.PutData( "Language" );
            permittedNames.PutData( "System Manufacturer" );
            permittedNames.PutData( "System Model" );
            permittedNames.PutData( "BIOS" ); 
            permittedNames.PutData( "Processor" );
            permittedNames.PutData( "Memory" );
            permittedNames.PutData( "DirectX Version" );
        }
        else if( strstr( line, "Display Devices" ) )
        {
            sectionId = 2;

            permittedNames.Empty();
            permittedNames.PutData( "Card name" );
            permittedNames.PutData( "Chip type" );
            permittedNames.PutData( "Display Memory" );            
            permittedNames.PutData( "Driver Version" );
            permittedNames.PutData( "Driver Date/Size" );    
        }
        else if( strstr( line, "Sound Devices" ) )
        {
            sectionId = 3;

            permittedNames.Empty();

            permittedNames.PutData( "Description" );
            permittedNames.PutData( "Driver Version" );
            permittedNames.PutData( "Date and Size" );
        }
        else
        {
            //
            // Is this line something we are interested in?

            bool permittedLine = false;
            char identifier[256];
            strcpy( identifier, line );
            char *colon = strchr( identifier, ':' );
            char *restOfLine = NULL;
            if( colon )
            {
                *colon = '\x0';
                restOfLine = colon+1;
                for( int i = 0; i < permittedNames.Size(); ++i )
                {
                    if( stricmp( identifier, permittedNames[i] ) == 0 )
                    {
                        permittedLine = true;
                        permittedNames.RemoveData(i);
                        break;
                    }
                }
                
                // Strip commas
                char *currentChar = restOfLine;
                while( true )
                {
                    if( *currentChar == ',' ) *currentChar = ' ';
                    if( *currentChar == '\x0' ) break;
                    ++currentChar;
                }

                if( permittedLine )
                {                
                    char *type = (  sectionId == 1 ? "SYS" :
                                    sectionId == 2 ? "GFX" :
                                    sectionId == 3 ? "SND" : "??" );

                    char fullLine[1024];
                    sprintf( fullLine, "%s %0-20s,%s", type, identifier, restOfLine );
                    m_dxDiag.PutData( strdup(fullLine) );                
                }
            }
        }
    }


    return true;
}


void BenchMark::Begin()
{
    m_recording = false;

    char benchmarksDir[512];
    snprintf( benchmarksDir, sizeof(benchmarksDir), "%sbenchmarks", g_app->GetProfileDirectory() );
    benchmarksDir[ sizeof(benchmarksDir) - 1 ] = '\0';

    CreateDirectory( benchmarksDir );

    //
    // Do we have dxdiag output?

    bool dxDiagDone = BuildDxDiag();
    if( !dxDiagDone ) return;

    //
    // Don't bother in single player

    if( g_app->m_gameMode != App::GameModeMultiwinia )
    {
        return;
    }


    //
    // Build our filename

    char authKey[256] = "-";

    Authentication_GetKey( authKey );

    snprintf( m_filename, sizeof(m_filename), "%s/game %s %s.txt", benchmarksDir, authKey, BuildTimeIndex() );
    m_filename[ sizeof(m_filename) - 1 ] = '\0';

    // Be sure to remove any \n or \r from the filename (might be in authkey)

    if( DoesFileExist( m_filename ) )
    {
        return;
    }

    m_recording = true;


    //
    // Parse system specs from dxdiag.txt

    FILE *file = fopen( m_filename, "a" );

    if( file )
    {        
        char *gameName = Multiwinia::s_gameBlueprints[ g_app->m_multiwinia->m_gameType ]->GetName();
        char *mapName = g_app->m_requestedMap;

        int screenW = g_prefsManager->GetInt(SCREEN_WIDTH_PREFS_NAME);
        int screenH = g_prefsManager->GetInt(SCREEN_HEIGHT_PREFS_NAME);
        int windowed = g_prefsManager->GetInt( SCREEN_WINDOWED_PREFS_NAME);
        int colours = g_prefsManager->GetInt( SCREEN_COLOUR_DEPTH_PREFS_NAME);
        int zDepth = g_prefsManager->GetInt( SCREEN_Z_DEPTH_PREFS_NAME );
        int refresh = 0;
#ifdef HAVE_REFRESH_RATES
        refresh = g_prefsManager->GetInt( SCREEN_REFRESH_PREFS_NAME );
#endif
    
        char currentScreenRes[256];
        sprintf( currentScreenRes, "%d x %d %s, %d bit Colour, %d hz, %d zDepth", screenW, screenH, (windowed ? "windowed" : "fullscreen" ), colours, refresh, zDepth );

        fprintf( file, "USR AuthKey             , %s\n", authKey );
        fprintf( file, "USR Build               , %s\n", DARWINIA_VERSION_STRING );
        fprintf( file, "USR Game                , %s\n", gameName );
        fprintf( file, "USR Map                 , %s\n", mapName );
        fprintf( file, "USR Screen              , %s\n", currentScreenRes );

        for( int i = 0; i < m_dxDiag.Size(); ++i )
        {
            fprintf( file, "%s", m_dxDiag[i] );
        }
        
        fprintf( file, "\n" );

        fclose(file);
    }

    AppDebugOut( "Benchmark started at %s\n", BuildTimeIndex() );
} 


char *BenchMark::BuildTimeIndex()
{
    static char s_timeIndex[256];

    time_t theTimeT = time(NULL);
    tm *theTime = localtime(&theTimeT);

    sprintf( s_timeIndex, "%02d-%02d-%02d %02d.%02d.%02d", 
                            1900+theTime->tm_year, 
                            theTime->tm_mon+1,
                            theTime->tm_mday,
                            theTime->tm_hour,
                            theTime->tm_min,
                            theTime->tm_sec );

    return s_timeIndex;
}


char *BenchMark::BuildBenchmark()
{
    int framesRendered = g_app->m_renderer->m_fps;
    int bandwidthIn = g_app->m_clientToServer->m_receiveRate;
    int bandwidthOut = g_app->m_clientToServer->m_sendRate;
    if( g_app->m_server )
    {
        bandwidthIn = g_app->m_server->m_receiveRate;
        bandwidthOut = g_app->m_server->m_sendRate;
    }

    int latency = g_app->m_clientToServer->GetEstimatedLatency() * 1000.0f;
    int entityCount = Entity::s_entityPopulation;

    static char s_benchmark[256];
    sprintf( s_benchmark, "%s,%5d,%6d,%6d,%5d,%5d", 
                                BuildTimeIndex(), 
                                framesRendered,
                                bandwidthIn,
                                bandwidthOut,
                                latency,
                                entityCount );

    return s_benchmark;
}


void BenchMark::Advance()
{
    if( !m_recording ) 
    {
        // Try and start now
        Begin();
        if( !m_recording ) return;
    }

    double timeNow = GetHighResTime();
    if( timeNow >= m_timer )
    {
        m_timer = timeNow + 1.0f;
        
        FILE *file = fopen( m_filename, "a" );
        if( file )
        {
            fprintf( file, "%s\n", BuildBenchmark() );
            fclose( file );
        }
    }
}


void BenchMark::End()
{
    if( !m_recording ) return;

    FILE *file = fopen( m_filename, "a" );
    if( file )
    {
        fprintf( file, "@\n" );
        fclose( file );
    }

    AppDebugOut( "Benchmark stopped\n" );
}


void BenchMark::Upload()
{
    if( DoesFileExist( m_filename ) )
    {
        char args[1024];
        sprintf( args, "-t 60 -u multiwinia_stats -p multiwinia_stats ftp.introversion.co.uk . \"%s\"", m_filename );

        int result = (int)ShellExecute( NULL, "open", "ncftpput.exe", args, NULL, SW_HIDE );
        
        AppDebugOut( "Uploading benchmark...result code %d\n", result );
        //AppDebugOut( "CMD: ncftpput.exe %s\n", args );
    }
}

#endif // BENCHMARK_AND_FTP
