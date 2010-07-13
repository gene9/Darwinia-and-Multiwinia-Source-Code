#ifndef _included_steamauthclient_h
#define _included_steamauthclient_h


/*
 *  Authentication module for Steam Clients
 *
 *      This module DOES require Steam to be running
 *      It should only be used from within a Steam app
 *
 *      Basic operation is you call Initiate, which creates a thread 
 *      to do the work async with everything else.  It performs a full
 *      Steam authentication with the MetaServer, and the end product
 *      is either a new key or a status message such as "banned"
 *
 *      This module shuts itself down when its done.
 *
 */

class Directory;



enum SteamAuthProgress
{
    SteamAuth_NotStarted=0,
    SteamAuth_Started,
    SteamAuth_StartingSteam,
    SteamAuth_StartingSteamAPI,
    SteamAuth_LoggingOnToSteam,
    SteamAuth_GettingSteamId,
//    SteamAuth_GettingEncryptionDetails,
//    SteamAuth_GettingSteam2Auth,
    SteamAuth_GettingSteam3Auth,
    SteamAuth_RequestAuthentication,
    SteamAuth_Success,
    SteamAuth_SuccessDemo,
    SteamAuth_Failed
};



void        SteamAuthentication_Initiate();
int         SteamAuthentication_GetProgress();
char       *SteamAuthentication_GetProgressName();
char       *SteamAuthentication_GetProgressNameLanguagePhrase();

int         SteamAuthentication_GetErrorCode();
char       *SteamAuthentication_GetErrorString();
char       *SteamAuthentication_GetErrorStringLanguagePhrase();

void        SteamAuthentication_ReceiveMessage( Directory *_message );


#endif
