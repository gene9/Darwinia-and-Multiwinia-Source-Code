#ifndef _included_metaserverdefines_h
#define _included_metaserverdefines_h


#define     PREFS_NETWORKMETASERVER             "NetworkMetaServerPort"
#define     PORT_METASERVER_LISTEN              5008
#define     PORT_METASERVER_CLIENT_LISTEN       5009

#define     NET_METASERVER_MESSAGE              "METASERVER"
#define     NET_METASERVER_COMMAND              "CMD"


#define     SECONDS                             * 1000
#define     PERIOD_SERVER_REGISTER_LAN          2 SECONDS
#define     PERIOD_SERVER_REGISTER_WAN          10 SECONDS
#define     PERIOD_SERVER_DISCARD               20 SECONDS
#define     PERIOD_AUTHENTICATION_RETRY         2 SECONDS



/*
 *	MetaServer command messages
 *
 */

#define     NET_METASERVER_REGISTER             "Register"
#define     NET_METASERVER_REQUEST_LIST         "Request"
#define     NET_METASERVER_LISTRESULT           "List"
#define     NET_METASERVER_REQUEST_AUTH         "ReqAuth"
#define     NET_METASERVER_AUTHRESULT           "AuthResult"
#define     NET_METASERVER_REQUEST_DATA         "ReqData"



/*
 *	MatchMaker commands and data
 */


#define     NET_MATCHMAKER_MESSAGE              "MATCHMAKER"
#define     NET_MATCHMAKER_REQUEST_IDENTITY     "ReqIdentity"
#define     NET_MATCHMAKER_IDENTIFY             "Identify"
#define     NET_MATCHMAKER_REQUEST_CONNECT      "ReqConnect"
#define     NET_MATCHMAKER_RECEIVED_CONNECT     "ReceiveCon"

#define     NET_MATCHMAKER_UNIQUEID             "UniqueId"
#define     NET_MATCHMAKER_YOURIP               "YourIp"
#define     NET_MATCHMAKER_YOURPORT             "YourPort"
#define     NET_MATCHMAKER_TARGETIP             "TargetIP"
#define     NET_MATCHMAKER_TARGETPORT           "TargetPort"

#define     PERIOD_MATCHMAKER_REQUESTID             10 SECONDS
#define     PERIOD_MATCHMAKER_REQUESTCONNECTION     3 SECONDS


/*
 *	MetaServer data fields
 *
 */

#define     NET_METASERVER_IP                   "IP"            
#define     NET_METASERVER_PORT                 "Port"
#define     NET_METASERVER_FROMIP               "FromIP"                                        // Where the UDP data came from (IP)
#define     NET_METASERVER_FROMPORT             "FromPort"                                      // Where the UDP data came from (Port)
#define     NET_METASERVER_LOCALIP              "LocalIP"                                       // Your IP on your local LAN
#define     NET_METASERVER_LOCALPORT            "LocalPort"                                     // Your port on the local system

#define     NET_METASERVER_SERVERNAME           "Name"
#define     NET_METASERVER_GAMENAME             "Game"
#define     NET_METASERVER_GAMEVERSION          "Version"
#define     NET_METASERVER_NUMTEAMS             "NumTeams"
#define     NET_METASERVER_NUMDEMOTEAMS         "NumDemoTeams"
#define     NET_METASERVER_MAXTEAMS             "MaxTeams"
#define     NET_METASERVER_NUMSPECTATORS        "NumSpectators"
#define     NET_METASERVER_MAXSPECTATORS        "MaxSpectators"
#define     NET_METASERVER_PROTOCOL             "Protocol"
#define     NET_METASERVER_PROTOCOLMATCH        "ProtocolMatch"
#define     NET_METASERVER_AUTHKEY              "AuthKey"
#define     NET_METASERVER_AUTHKEYID            "AuthKeyId"
#define     NET_METASERVER_HASHTOKEN            "HashToken"
#define     NET_METASERVER_PASSWORD             "Password"
#define     NET_METASERVER_RECEIVETIME          "ReceiveTime"

#define     NET_METASERVER_GAMEINPROGRESS       "GameInProgress"
#define     NET_METASERVER_GAMEMODE             "GameMode"
#define     NET_METASERVER_SCOREMODE            "ScoreMode"
#define     NET_METASERVER_GAMETIME             "GameTime"
#define     NET_METASERVER_DEMOSERVER           "DemoServer"
#define     NET_METASERVER_DEMORESTRICTED       "DemoRestr"
#define     NET_METASERVER_PLAYERNAME           "P"


/*
 *  MetaServer data request types
 *
 */

#define     NET_METASERVER_DATATYPE             "DataType"
#define     NET_METASERVER_DATA_TIME            "Time"
#define     NET_METASERVER_DATA_MOTD            "MOTD"
#define     NET_METASERVER_DATA_DEMOLIMITS      "DemoLimits"
#define     NET_METASERVER_DATA_LATESTVERSION   "LatestVersion"
#define     NET_METASERVER_DATA_UPDATEURL       "UpdateURL"
#define     NET_METASERVER_DATA_STRESSSERVERS   "StressServers"
#define     NET_METASERVER_DATA_STRESSCLIENTS   "StressClients"

#define     NET_METASERVER_DATATIMETOLIVE       "DataTTL"
#define     NET_METASERVER_TIME_HOURS           "TimeHours"
#define     NET_METASERVER_TIME_MINUTES         "TimeMinutes"
#define     NET_METASERVER_MAXDEMOGAMESIZE      "MaxDemoGameSize"
#define     NET_METASERVER_MAXDEMOPLAYERS       "MaxDemoPlayers"
#define     NET_METASERVER_ALLOWDEMOSERVERS     "AllowDemoServers"



/*
 *  MetaServer Steam data
 *
 */

#define     NET_METASERVER_STEAM_AUTHENTICATE          "SteamAuthenticate"

#define     NET_METASERVER_STEAM_ENCRYPTIONDETAILS     "SteamEncDetails"
#define     NET_METASERVER_STEAM_ENCRYPTIONKEY         "SteamEncKey"
#define     NET_METASERVER_STEAM_USERID                "SteamUserId"
#define     NET_METASERVER_STEAM2KEY                   "Steam2Key"
#define     NET_METASERVER_STEAM3KEY                   "Steam3Key"
#define     NET_METASERVER_STEAM_AUTHRESULT            "SteamAuthResult"
#define     NET_METASERVER_STEAM_AUTHRESULTMESSAGE     "SteamAuthResultMessage"


#endif
