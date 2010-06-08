#include "lib/universal_include.h"

#include <steam/steam_api.h>

#include "lib/tosser/directory.h"
#include "lib/metaserver/metaserver.h"
#include "lib/metaserver/metaserver_defines.h"
#include "lib/metaserver/authentication.h"
#include "lib/netlib/net_thread.h"

#include "steamauth_client.h"
#include "steamauth_server.h"


static  int     s_currentStatus = SteamAuth_NotStarted;
static  int     s_numRetries = 0;

static  char    s_encryptionKey[1024];
static  int     s_encryptionKeyLen;
static  bool    s_encryptionDetailsKnown = false;

//static  char            s_steam2Key[1024];
//static  unsigned int    s_steam2KeyLen;
//static  bool            s_steam2KeyKnown = false;

static  char            s_steam3Key[2048];
static  unsigned int    s_steam3KeyLen;
static  bool            s_steam3KeyKnown = false;

static  CSteamID        s_metaServerSteamId;
static  int             s_steamAppId = 1520;

static  CSteamID        s_clientSteamId;
static  bool            s_clientSteamIdKnown = false;

static  int             s_steamErrorCode = -1;


using namespace std;


void RunNextStage()
{
    s_currentStatus++;
    s_numRetries = 0;

    AppDebugOut( "Steam Authentication : %s\n", SteamAuthentication_GetProgressName() );
}


void StartSteam()
{
    /*TSteamError steamError;
    bool result = SteamStartup( STEAM_USING_USERID, &steamError);
    if(!result)
    {
        return;
    }*/

    RunNextStage();
}


void StartSteamAPI()
{
    bool result = SteamAPI_Init();
    if( !result )
    {
        return;
    }

    RunNextStage();
}


void LogOnToSteam()
{
    if( s_numRetries > 8 &&
        SteamUser() )
    {
        RunNextStage();
    }

    if( !SteamUser() ||
        !SteamUser()->BLoggedOn() )
    {
        SteamAPI_RunCallbacks();        
        return;
    }

    RunNextStage();
}


void GetClientSteamId()
{
    AppAssert( SteamUser() );

    s_clientSteamId = SteamUser()->GetSteamID();
    s_clientSteamIdKnown = true;
    
    char steamId[256];
    ConvertSteamIdToString(s_clientSteamId, steamId, sizeof(steamId) );
    AppDebugOut( "Steam Authentication : SteamID = %s\n", steamId );

    RunNextStage();
}


void GetEncryptionDetails()
{
    Directory *dir = MetaServer_RequestData( NET_METASERVER_STEAM_ENCRYPTIONDETAILS );

    if( dir &&
        dir->HasData( NET_METASERVER_STEAM_ENCRYPTIONKEY, DIRECTORY_TYPE_VOID ) &&
        dir->HasData( NET_METASERVER_STEAM_USERID, DIRECTORY_TYPE_STRING ) )
    {     
        void *data = dir->GetDataVoid( NET_METASERVER_STEAM_ENCRYPTIONKEY, &s_encryptionKeyLen );
        memcpy( s_encryptionKey, data, s_encryptionKeyLen );
        char *userId = dir->GetDataString( NET_METASERVER_STEAM_USERID );
        
#ifdef _DEBUG        
        AppDebugOut( "Steam Authentication : MetaServer Encryption key (%d bytes)\n", s_encryptionKeyLen );
        AppDebugOutData( s_encryptionKey, s_encryptionKeyLen );
        AppDebugOut( "Steam Authentication : MetaServer SteamID = %s\n", userId );
#endif
    
        s_metaServerSteamId = ConvertStringToSteamId(userId);

        s_encryptionDetailsKnown = true;
        RunNextStage();
    }

    if( dir )
    {
        delete dir;
    }
}


//void GetSteam2Auth()
//{
//    AppAssert( s_encryptionDetailsKnown );
//
//    ESteamError eError;
//    TSteamError tError;
//    eError = SteamGetEncryptedUserIDTicket( s_encryptionKey, 
//                                            s_encryptionKeyLen, 
//                                            s_steam2Key, 
//                                            1024, 
//                                            &s_steam2KeyLen, 
//                                            &tError );
//    
//    if( eError != eSteamErrorNone )
//    {
//        return;
//    }
//
//#ifdef _DEBUG
//    AppDebugOut( "Steam Authentication : Steam2 key (%d bytes)\n", s_steam2KeyLen );
//    AppDebugOutData( s_steam2Key, s_steam2KeyLen );
//#endif
//
//    s_steam2KeyKnown = true;
//    RunNextStage();
//}


void GetSteam3Auth()
{
    //AppAssert( s_encryptionDetailsKnown );
    AppAssert( SteamUser() );

    int ipServer = GetLocalHost();
    int portServer = 0;

    int result = SteamUser()->InitiateGameConnection( s_steam3Key, 
                                                      2048, 
                                                      s_metaServerSteamId,                                                       
                                                      ipServer, portServer, 
                                                      false );

    if( result <= 0 )
    {
        return;
    }

    s_steam3KeyLen = result;
    s_steam3KeyKnown = true;

    RunNextStage();
}


void RequestAuthentication()
{
    AppAssert( s_clientSteamIdKnown );
    AppAssert( s_steam3KeyKnown );

    char steamId[256];
    ConvertSteamIdToString(s_clientSteamId, steamId, sizeof(steamId) );

    Directory message;
    message.SetName( NET_METASERVER_MESSAGE );
    message.CreateData( NET_METASERVER_COMMAND, NET_METASERVER_STEAM_AUTHENTICATE );
    message.CreateData( NET_METASERVER_STEAM_USERID, steamId );
    message.CreateData( NET_METASERVER_STEAM3KEY, s_steam3Key, s_steam3KeyLen );


    MetaServer_SendToMetaServer( &message );

#ifdef _DEBUG
    AppDebugOut( "Steam Authentication : Sent request to MetaServer\n" );
#endif

    NetSleep(1000);
}


static NetCallBackRetType SteamAuthenticationMainThread( void *unused )
{
    NetSleep(1000);

    bool completed = false;

    while( !completed )
    {        
        switch( s_currentStatus )
        {
            case SteamAuth_Started:                         RunNextStage();             break;
            case SteamAuth_StartingSteam:                   StartSteam();               break;
            case SteamAuth_StartingSteamAPI:                StartSteamAPI();            break;
            case SteamAuth_LoggingOnToSteam:                LogOnToSteam();             break;
            case SteamAuth_GettingSteamId:                  GetClientSteamId();         break;
//            case SteamAuth_GettingEncryptionDetails:        GetEncryptionDetails();     break;
//            case SteamAuth_GettingSteam2Auth:               GetSteam2Auth();            break;
            case SteamAuth_GettingSteam3Auth:               GetSteam3Auth();            break;
            case SteamAuth_RequestAuthentication:           RequestAuthentication();    break;
            
            case SteamAuth_Success:   
            case SteamAuth_SuccessDemo:
            case SteamAuth_Failed:
                completed = true;           
                break;                  
        }


        if( s_numRetries >= 10 )
        {
            AppDebugOut( "Steam Authentication FAILED\n" );
            s_currentStatus = SteamAuth_Failed;
            break;
        }

        s_numRetries++;
        NetSleep( s_numRetries * 100 );
    };


    return NetOk;
}


void SteamAuthentication_Initiate()
{
    if( s_currentStatus != SteamAuth_NotStarted )
    {
        return;
    }

    s_currentStatus = SteamAuth_Started;

    NetStartThread( SteamAuthenticationMainThread );

    AppDebugOut( "Steam Authentication Initiated\n" );
}


int SteamAuthentication_GetProgress()
{
    return s_currentStatus;
}


char *SteamAuthentication_GetProgressName()
{
    static char *s_stateNames[] = {     
                                        "Not Started",
                                        "Started",
                                        "Starting Steam",
                                        "Starting Steam API",
                                        "Logging On To Steam",
                                        "Getting Steam Id",
//                                        "Getting Encryption Details",
//                                        "Getting Steam2 Auth",
                                        "Getting Steam3 Auth",
                                        "Requesting Authentication",
                                        "Success",
                                        "Success (Demo)",
                                        "Failed"
                                    };

    AppDebugAssert( s_currentStatus >= 0 && s_currentStatus < 13 );
    return s_stateNames[ s_currentStatus ];
}


char *SteamAuthentication_GetProgressNameLanguagePhrase()
{
    static char *s_stateNames[] = {     
                                        "dialog_steam_progress_not_started",
                                        "dialog_steam_progress_started",
                                        "dialog_steam_progress_starting_steam",
                                        "dialog_steam_progress_starting_steam_api",
                                        "dialog_steam_progress_logging_on_to_steam",
                                        "dialog_steam_progress_getting_steam_id",
//                                        "dialog_steam_progress_getting_encryption_details",
//                                        "dialog_steam_progress_getting_steam2_auth",
                                        "dialog_steam_progress_getting_steam3_auth",
                                        "dialog_steam_progress_requesting_authentication",
                                        "dialog_steam_progress_success",
                                        "dialog_steam_progress_success_demo",
                                        "dialog_steam_progress_failed"
                                    };

    AppDebugAssert( s_currentStatus >= 0 && s_currentStatus < 13 );
    return s_stateNames[ s_currentStatus ];
}


void SteamAuthentication_ReceiveMessage( Directory *_message )
{
    if( !_message->HasData( NET_METASERVER_STEAM_AUTHRESULT, DIRECTORY_TYPE_INT ) )
    {
        AppDebugOut( "Received bogus Steam Auth message, discarded\n" );
        return;
    }


    int authResult = _message->GetDataInt( NET_METASERVER_STEAM_AUTHRESULT );
    
    if( authResult == SteamAuthResult_Failed )
    {
        int resultMessage = _message->GetDataInt( NET_METASERVER_STEAM_AUTHRESULTMESSAGE );
        s_steamErrorCode = resultMessage;
        AppDebugOut( "Steam Authentication FAILED\n%s\n", 
                        Authentication_SteamServer_GetResultString(resultMessage) );    
        s_currentStatus = SteamAuth_Failed;
    }
    else if( authResult == SteamAuthResult_Demo )
    {
        AppDebugOut( "Steam Authentication Accepted : DEMO USER\n" );    
        s_currentStatus = SteamAuth_SuccessDemo;

        char currentKey[512];
        Authentication_GetKey(currentKey);
        if( !Authentication_IsDemoKey(currentKey) )
        {
            Authentication_GenerateKey( currentKey, true );
            Authentication_SetKey( currentKey );
            Authentication_SaveKey( currentKey, "authkey" );
        }
    }
    else if( authResult == SteamAuthResult_Accepted )
    {
        AppDebugOut( "Steam Authentication Accepted : FULL GAME USER\n" );    
        s_currentStatus = SteamAuth_Success;

        char *key = _message->GetDataString( NET_METASERVER_AUTHKEY );
        int keyId = _message->GetDataInt( NET_METASERVER_AUTHKEYID );
        int hashToken = _message->GetDataInt( NET_METASERVER_HASHTOKEN );
        
        Authentication_SetKey( key );
        Authentication_SaveKey( key, "authkey" );
        Authentication_SetStatus( key, keyId, AuthenticationAccepted );
        Authentication_SetHashToken( hashToken );
    }

    s_numRetries = 0;
}


int SteamAuthentication_GetErrorCode()
{
    return s_steamErrorCode;
}


char *SteamAuthentication_GetErrorString()
{
    if( s_steamErrorCode == -1 )
    {
        return NULL;
    }

    return Authentication_SteamServer_GetResultString( s_steamErrorCode );
}


char *SteamAuthentication_GetErrorStringLanguagePhrase()
{
    if( s_steamErrorCode == -1 )
    {
        return NULL;
    }

    return Authentication_SteamServer_GetResultStringLanguagePhrase( s_steamErrorCode );
}

