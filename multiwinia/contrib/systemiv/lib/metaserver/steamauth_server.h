#ifndef _included_steamauthentication_h
#define _included_steamauthentication_h


/*
 *  Authentication module for Steam Servers
 *
 *      This module does NOT require Steam to be running - 
 *      It can be run from any steam or non-steam app.
 *
 */

#include <steam/steam_api.h>

enum
{
    SteamAuthResult_Failed = -1,
    SteamAuthResult_Demo = 0,
    SteamAuthResult_Accepted = 1
};


typedef void SteamAuthCallback( CSteamID _steamId, 
                                char *_fromIp, int _fromPort, 
                                int _result, int _resultMessage );


/*
 *  Basic setup stuff
 *
 */


bool        Authentication_SteamServer_Init                 ( SteamAuthCallback *_callback );
bool        Authentication_SteamServer_GetSteamId           ( char *_result, int _resultSize );                                                   
bool        Authentication_SteamServer_GetEncryptionKey     ( void *_data, int *_dataSize, int _maxSize );                      




/*
 *  Call these functions to begin the process
 *  of Authenticating a single steam user.
 *  The SteamAuthCallback will receive the results when they come in.
 *
 */

bool        Authentication_SteamServer_IsAuthenticating     ( CSteamID _steamId );
bool        Authentication_SteamServer_AuthClient           ( CSteamID _steamId,
                                                              void *_steam3key, int _steam3KeyLen,
                                                              char *_fromIp, int _fromPort );



/*
 *  Some useful helper functions
 *
 */

char        *Authentication_SteamServer_GetResultString     ( int _result );
char        *Authentication_SteamServer_GetResultStringLanguagePhrase     ( int _result );

void        ConvertSteamIdToString                          ( CSteamID _steamId, char *_result, int _resultSize );
void        ConvertSteamIdToString                          ( unsigned int _steamId, char *_result, int _resultSize );

CSteamID        ConvertStringToSteamId                      ( char *_string );
unsigned int    ConvertStringToSteam2Id                     ( char *_string );


#endif
