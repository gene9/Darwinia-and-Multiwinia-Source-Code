#ifndef _included_matchmaker_h
#define _included_matchmaker_h

class NetSocketListener;
class Directory;


/*
 *	Part of the MetaServer functions
 *
 *  Provides methods for clients to connect to each other,
 *  regardless of what sort of NAT they might be behind.
 *
 *  Uses UDP hole punching via our MetaServer.
 *
 */




void    MatchMaker_LocateService                ( const char *_matchMakerIp, int _port );



void	MatchMaker_RequestConnection			( NetSocketListener *_listener, const char *_targetIp, int _targetPort, Directory *_myDetails );
void    MatchMaker_StartRequestingIdentity      ( NetSocketListener *_listener );                   // Identifies the public IP and port of this listener
void    MatchMaker_StopRequestingIdentity       ( NetSocketListener *_listener );
bool    MatchMaker_IsRequestingIdentity         ( NetSocketListener *_listener );



bool    MatchMaker_IsIdentityKnown              ( NetSocketListener *_listener );
bool    MatchMaker_GetIdentity                  ( NetSocketListener *_listener, 
                                                  char *_ip, int *_port );                          // ip and port are written into



void    MatchMaker_RequestConnection            ( char *_targetIp, int _targetPort,                 // Relays the request via our matchmaker
                                                  Directory *_myDetails );


bool    MatchMaker_ReceiveMessage               ( NetSocketListener *_listener,                     // Whoever owns the listener should pass replies in to here
                                                  Directory *_message );                            // Also pass in the listener than picked up the reply




#endif
