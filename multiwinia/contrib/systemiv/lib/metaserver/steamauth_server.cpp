
#include "lib/universal_include.h"

#include <steam/steam_api.h>
#include <steam/steam_gameserver.h>
#include <steam/isteamgameserver.h>
#include <strstream>

#ifdef WIN32
#include <windows.h>
#pragma warning( disable: 4355 ) // disables ' 'this' : used in base member initializer list'
#endif

#include "lib/math/random_number.h"
#include "lib/tosser/hash_table.h"
#include "lib/debug_utils.h"
#include "lib/netlib/net_thread.h"

#include "steamauth_server.h"

using namespace std;


class SteamAuthInProgress
{
public:
    CSteamID    m_steamId;
    char        m_fromIp[256];
    int         m_fromPort;
    double      m_timeStarted;
};

HashTable<SteamAuthInProgress *> s_inProgress;                          // Indexed on Steam2 ID



class CallbackHandler
{
public:
    STEAM_GAMESERVER_CALLBACK( CallbackHandler, OnGSClientApprove, GSClientApprove_t, m_CallbackGSClientApprove );
    STEAM_GAMESERVER_CALLBACK( CallbackHandler, OnGSClientDeny, GSClientDeny_t, m_CallbackGSClientDeny );
//    STEAM_GAMESERVER_CALLBACK( CallbackHandler, OnGSClientSteam2Deny, GSClientSteam2Deny_t, m_CallbackGSClientSteam2Deny );
//    STEAM_GAMESERVER_CALLBACK( CallbackHandler, OnGSClientSteam2Accept, GSClientSteam2Accept_t, m_CallbackGSClientSteam2Accept );

public:
    CallbackHandler();
  
    void HandleAuthResult( unsigned int _steamId, bool success, int _resultMessage );
};


static  SteamAuthCallback   *s_callback=NULL;
static  CallbackHandler     s_callbackHandler;

static  NetMutex            s_steamMutex;
static  bool                s_steamLoggedOn;


// ============================================================================


CallbackHandler::CallbackHandler() 
:   m_CallbackGSClientApprove( this, &CallbackHandler::OnGSClientApprove ),
    m_CallbackGSClientDeny( this, &CallbackHandler::OnGSClientDeny )
//    m_CallbackGSClientSteam2Deny( this, &CallbackHandler::OnGSClientSteam2Deny ),
//    m_CallbackGSClientSteam2Accept( this, &CallbackHandler::OnGSClientSteam2Accept )
{
}


//void CallbackHandler::OnGSClientSteam2Deny( GSClientSteam2Deny_t *pGSClientSteam2Deny )
//{
//    HandleAuthResult( pGSClientSteam2Deny->m_UserID, false, pGSClientSteam2Deny->m_eSteamError );
//}
//
//
//void CallbackHandler::OnGSClientSteam2Accept( GSClientSteam2Accept_t *pGSClientSteam2Accept )
//{
//    printf( "Steam2Accept for SteamID %d\n", 
//                pGSClientSteam2Accept->m_UserID );
//}


void CallbackHandler::OnGSClientDeny( GSClientDeny_t *pGSClientDeny )
{
    HandleAuthResult( pGSClientDeny->m_SteamID.GetAccountID(), false, pGSClientDeny->m_eDenyReason );
}


void CallbackHandler::OnGSClientApprove( GSClientApprove_t *pGSClientApprove )
{
    SteamGameServer()->SendUserDisconnect( pGSClientApprove->m_SteamID );

    HandleAuthResult( pGSClientApprove->m_SteamID.GetAccountID(), true, -1 );
}


void CallbackHandler::HandleAuthResult( unsigned int _steamId, bool success, int _resultMessage )
{
    //
    // Find the In Progress structure for this steamID
    // And remove it from the list

    char key[256];
    ConvertSteamIdToString(_steamId, key, sizeof(key) );
    int index = s_inProgress.GetIndex( key );

    if( index == -1 )
    {
        AppDebugOut( "Received Steam authentication for unknown client : SteamID %s\n", key );
        return;
    }

    SteamAuthInProgress *inProgress = s_inProgress.GetData(index);
    s_inProgress.RemoveData(index);


    //
    // Call the user specified callback with the results

    if( s_callback )
    {
        int result = ( success ? SteamAuthResult_Accepted :
                       _resultMessage == k_EDenyNoLicense ? SteamAuthResult_Demo :
                       SteamAuthResult_Failed );

        s_callback( inProgress->m_steamId, 
                    inProgress->m_fromIp, 
                    inProgress->m_fromPort, 
                    result, 
                    _resultMessage );

        delete inProgress;
    }
}


static NetCallBackRetType SteamAPICallbacksThread( void *unused )
{
    AppDebugOut( "Steam API callbacks thread running\n" );

    while( true )
    {
        s_steamMutex.Lock();

        SteamAPI_RunCallbacks();  

        SteamGameServer_RunCallbacks();

        s_steamLoggedOn = (SteamGameServer() && SteamGameServer()->BLoggedOn());

        s_steamMutex.Unlock();

        NetSleep(100);
    }
}


bool Authentication_SteamServer_Init( SteamAuthCallback *_callback )
{
    s_callback = _callback;

    //
    // Initialise Steam game server
        
    int ourIp = GetLocalHost();
    int localPort = 0;
    int gamePort = 0;
    int specPort = 0;
    int queryPort = 0;

    s_steamMutex.Lock();
    bool result = SteamGameServer_Init( ourIp, localPort, gamePort, specPort, queryPort, eServerModeAuthentication, "defcon-steam3", APP_VERSION );
    s_steamMutex.Unlock();

    if( !result )   
    {
        AppDebugOut( "Failed to initialise Steam Game Server\n" );
        return false;
    }


    //
    // Start callbacks thread

    NetStartThread( SteamAPICallbacksThread );


    //
    // Log on to steam service

    int tries = 0;
    bool success = true;

    s_steamMutex.Lock();

    while( !SteamGameServer() ||
           !SteamGameServer()->BLoggedOn() )
    {                
		s_steamMutex.Unlock();
        Sleep(100);
		s_steamMutex.Lock();

        tries++;
        if( tries > 30 ) 
        {   
            success = false;
            break;
        }
    }

    s_steamMutex.Unlock();

    if( !success )
    {
        AppDebugOut( "Failed to connect to Steam Game Server\n" );
        s_steamLoggedOn = false;
    }
    else
    {
        s_steamLoggedOn = true;
    }

    return s_steamLoggedOn;
}

bool Authentication_SteamServer_GetEncryptionKey( void *_data, int *_dataSize, int _maxSize  )
{
    /*if( !s_steamLoggedOn )
    {
        return false;
    }

    memset(_data, 0, _maxSize);
    unsigned int length = 0;

    s_steamMutex.Lock();
    bool result = SteamGameServer()->GSGetSteam2GetEncryptionKeyToSendToNewClient( _data, &length, _maxSize );
    s_steamMutex.Unlock();

    if( result )
    {
        *_dataSize = length;
        return true;
    }*/

    return false;
}


bool Authentication_SteamServer_GetSteamId( char *_result, int _resultSize )
{
    if( !s_steamLoggedOn )
    {
        return false;
    }

    s_steamMutex.Lock();
    CSteamID steamId = SteamGameServer()->GetSteamID();
    s_steamMutex.Unlock();

    ConvertSteamIdToString(steamId, _result, _resultSize);

    return true;
}


void ConvertSteamIdToString( CSteamID _steamId, char *_result, int _resultSize )
{
    uint64 steamId64 = _steamId.ConvertToUint64();

    ostrstream str(_result, _resultSize);
    str << steamId64;
    str << '\0';
}


void ConvertSteamIdToString( unsigned int _steamId, char *_result, int _resultSize )
{
    sprintf( _result, "%d", _steamId );
}


CSteamID ConvertStringToSteamId( char *_string )
{
    uint64 userId64;
    istrstream input(_string);
    input >> userId64;

    CSteamID result;
    result.SetFromUint64( userId64 );

    return result;
}


unsigned int ConvertStringToSteam2Id( char *_string )
{
    unsigned int result;

    sscanf( _string, "%d", &result );

    return result;
}


bool Authentication_SteamServer_IsAuthenticating( CSteamID _steamId )
{
    char key[256];
    ConvertSteamIdToString(_steamId.GetAccountID(), key, sizeof(key) );

    int existingIndex = s_inProgress.GetIndex( key );
    if( existingIndex != -1 )
    {
        return true;
    }

    return false;
}


bool Authentication_SteamServer_AuthClient( CSteamID _steamId,
                                           void *_steam3key, int _steam3KeyLen,
                                           char *_fromIp, int _fromPort )
{
    if( !s_steamLoggedOn )
    {
        return false;
    }


    //
    // Look to see if this authentication is already in progress

    char key[256];
    ConvertSteamIdToString(_steamId.GetAccountID(), key, sizeof(key) );
    int existingIndex = s_inProgress.GetIndex( key );
    if( existingIndex != -1 )
    {
        return true;
    }


    //
    // Log the start of the authentication

    SteamAuthInProgress *progress = new SteamAuthInProgress();
    progress->m_steamId = _steamId;
    strcpy( progress->m_fromIp, _fromIp );
    progress->m_fromPort = _fromPort;
    progress->m_timeStarted = GetHighResTime();
    
    s_inProgress.PutData( key, progress );


    //
    // Request the auth of steam

    int ipServer = GetLocalHost();
    int portServer = 0;
    uint32 userId = _steamId.GetAccountID();

    int ipClient = StrToIpAddress(_fromIp);
        
    s_steamMutex.Lock();
    //bool result = SteamGameServer()->GSSendSteam2UserConnect( userId, _steam2Key, _steam2KeyLen, ipServer, portServer, _steam3key, _steam3KeyLen );
    bool result = SteamGameServer()->SendUserConnectAndAuthenticate( ipClient, _steam3key, _steam3KeyLen, &_steamId );
    s_steamMutex.Unlock();


    return result;
}


char *Authentication_SteamServer_GetResultString( int _result )
{
    switch ( _result )
    {
        case k_EDenySteamValidationStalled:
            return "Error verifying STEAM UserID Ticket(server was unable to contact the authentication server).\n";
            // This error means you couldn't connect to the Steam2 servers to do this first part of the user authentication

        case k_EDenyNotLoggedOn:
            return "Invalid Steam Logon";
            // The user isn't properly connected to Steam, clients need to fix that

        case k_EDenyLoggedInElseWhere:
            return "This account has been logged into in another location";
            // This account is being used in multiple places at once, clients need to log back in at their final location

        case k_EDenyNoLicense:
            return "This Steam account does not own this game. \nPlease login to the correct Steam account.";
            // The client doesn't own this game


            //-----------------------------------------------
            // GS specific responses that you shouldn't get
            //-----------------------------------------------
        case k_EDenyInvalidVersion:
            return "Client version incompatible with server. \nPlease exit and restart";

        case k_EDenyCheater:
            return "This account is banned";

        case k_EDenyUnknownText:
            return "Client dropped by server";

        case k_EDenyIncompatibleAnticheat:
            return "You are running an external tool that is incompatible with Secure servers.";

        case k_EDenyMemoryCorruption:
            return "Memory corruption detected.";

        case k_EDenyIncompatibleSoftware:
            return "You are running software that is not compatible with Secure servers.";

        case k_EDenyGeneric:
        default:
            return "Client dropped by server";
    }

    return "Unknown";
}


char *Authentication_SteamServer_GetResultStringLanguagePhrase( int _result )
{
    switch ( _result )
    {
        case k_EDenySteamValidationStalled:
            return "dialog_steam_error_edenysteamvalidationstalled";
            // This error means you couldn't connect to the Steam2 servers to do this first part of the user authentication

        case k_EDenyNotLoggedOn:
            return "dialog_steam_error_edenynotloggedon";
            // The user isn't properly connected to Steam, clients need to fix that

        case k_EDenyLoggedInElseWhere:
            return "dialog_steam_error_edenyloggedinelsewhere";
            // This account is being used in multiple places at once, clients need to log back in at their final location

        case k_EDenyNoLicense:
            return "dialog_steam_error_edenynolicense";
            // The client doesn't own this game


            //-----------------------------------------------
            // GS specific responses that you shouldn't get
            //-----------------------------------------------
        case k_EDenyInvalidVersion:
            return "dialog_steam_error_edenyinvalidversion";

        case k_EDenyCheater:
            return "dialog_steam_error_edenycheater";

        case k_EDenyUnknownText:
            return "dialog_steam_error_edenyunknowntext";

        case k_EDenyIncompatibleAnticheat:
            return "dialog_steam_error_edenyincompatibleanticheat";

        case k_EDenyMemoryCorruption:
            return "dialog_steam_error_edenymemorycorruption";

        case k_EDenyIncompatibleSoftware:
            return "dialog_steam_error_edenyincompatiblesoftware";

        case k_EDenyGeneric:
        default:
            return "dialog_steam_error_edenygeneric";
    }

    return "dialog_steam_error_unknown";
}
