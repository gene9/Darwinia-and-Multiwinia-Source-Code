
/*
 *  MetaServer Access Library
 *
 *      Used by Servers to advertise their existence 
 *      with a WAN MetaServer, or by LAN broadcast
 *
 *      Used by Clients to request lists of all known servers
 *
 *
 *  These #defines control what is enabled:
 *      LAN_PLAY_ENABLED
 *      WAN_PLAY_ENABLED
 *
 */


#ifndef _included_metaserver_h
#define _included_metaserver_h

#include "lib/tosser/llist.h"

class Directory;
class NetSocket;
class NetSocketSession;



/*
 *  Basic stuff
 */

void    MetaServer_Initialise                   ();
bool    MetaServer_SendDirectory                ( Directory *_directory, NetSocket *_socket );
bool    MetaServer_SendDirectory                ( Directory *_directory, NetSocketSession *_socket );
bool    MetaServer_SendToMetaServer             ( Directory *_directory );

void    MetaServer_Connect                      ( const char *_metaServerIp, int _port );
void    MetaServer_Connect                      ( const char *_metaServerIp, int _metaServerPort, int _port );
void    MetaServer_Disconnect                   ();
bool    MetaServer_IsConnected                  ();


/*
 *  Servers should use these functions to register their existence
 *  with a WAN Meta Server, or over LAN broadcast
 */

void    MetaServer_SetServerProperties          ( Directory *_properties );

void    MetaServer_StartRegisteringOverWAN      ();
void    MetaServer_StartRegisteringOverLAN      ();

void    MetaServer_StopRegisteringOverWAN       ();
void    MetaServer_StopRegisteringOverLAN       ();

bool    MetaServer_IsRegisteringOverWAN         ();
bool    MetaServer_IsRegisteringOverLAN         ();



/*
 *  Client commands for requesting Server lists
 *  from a LAN broadcast, or a WAN MetaServer
 */


void        MetaServer_SetClientProperties          ( Directory *_properties );
Directory   *MetaServer_GetClientProperties         ();                                         // A copy is created and returned

void        MetaServer_RequestServerListWAN         ( int _gameType );
bool        MetaServer_HasReceivedListWAN           ();

void        MetaServer_RequestServerDetails         ( char *_ip, int _port );



/*
 *  Client commands for retrieving bits of Data from a MetaServer
 *  Eg TheTime, Latest Version, MOTD etc.
 *  Automatically sends requests when needed
 *
 */


Directory       *MetaServer_RequestData             ( const char *_dataType, const char *_lang = NULL );

int             MetaServer_GetServerTTL             ();


/*
 *  Functions to maintain a list of known servers
 *  Removing old servers when they don't register for a while
 */

void    MetaServer_ClearServerList              ();
void    MetaServer_DelistServers                ( char *_authKey );
void    MetaServer_UpdateServerList             ( char *_ip, int _port, bool _lanOrWan, Directory *_server );
void	MetaServer_DelistServer					( const char *_ip, int _port, Directory *_server );

void    MetaServer_SetAuthenticationStatus      ( char *_ip, int _port, int _status );
void    MetaServer_PurgeServerList              ();
int     MetaServer_GetNumServers                ( bool wanServers=true, bool lanServers=true );
void    MetaServer_SetServerTTL                 ( int _seconds );


Directory *MetaServer_GetServer                 ( char *_ip, int _port,
                                                  bool _authRequired );                     // A copy is created and returned

LList   <Directory *> *MetaServer_GetServerList ( bool authRequired=false,
                                                  bool wanServers=true,
                                                  bool lanServers=true,
                                                  int maxListSize=-1 );                     // A copy is created and returned


/*
 *  Misc stuff
 *
 */

int     MetaServer_BytesSent();                             // Reports bytes sent since last call

void    MetaServer_GraduallySortList();                     // Performs one iteration of a bubble-sort (into joinability order)                                                            


#endif
