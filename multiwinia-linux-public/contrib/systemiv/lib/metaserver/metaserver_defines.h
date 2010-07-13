#ifndef _included_metaserverdefines_h
#define _included_metaserverdefines_h


#define		METASERVER_GAMETYPE_DEFCON			2
#define		METASERVER_GAMETYPE_MULTIWINIA		4


#define     PREFS_NETWORKMETASERVER             "NetworkMetaServerPort"
#define     PORT_METASERVER_LISTEN              5113
#define     PORT_DEFCON_METASERVER_LISTEN       5008
#define     PORT_METASERVER_CLIENT_LISTEN       5009

#define     NET_METASERVER_MESSAGE              "m"
#define     NET_METASERVER_COMMAND              "c"


#define     SECONDS                             * 1000
#define     PERIOD_SERVER_REGISTER_LAN          5 SECONDS
#define     PERIOD_AUTHENTICATION_RETRY         2 SECONDS



/*
 *	MetaServer command messages
 *
 */

#define     NET_METASERVER_REGISTER             "ma"
#define     NET_METASERVER_REQUEST_LIST         "mb"
#define     NET_METASERVER_LISTRESULT           "mc"
#define     NET_METASERVER_REQUEST_AUTH         "md"
#define     NET_METASERVER_AUTHRESULT           "me"
#define     NET_METASERVER_REQUEST_DATA         "mf"
#define     NET_METASERVER_LISTRESULT_ENVELOPE  "mg"
#define     NET_METASERVER_REQUESTSERVERDETAILS "mh"
#define     NET_METASERVER_UNREGISTER           "mi"

/*
 *	MatchMaker commands and data
 */


#define     NET_MATCHMAKER_MESSAGE              "match"
#define     NET_MATCHMAKER_REQUEST_IDENTITY     "aa"
#define     NET_MATCHMAKER_IDENTIFY             "ab"
#define     NET_MATCHMAKER_REQUEST_CONNECT      "ac"
#define     NET_MATCHMAKER_RECEIVED_CONNECT     "ad"

#define     NET_MATCHMAKER_UNIQUEID             "ae"
#define     NET_MATCHMAKER_YOURIP               "af"
#define     NET_MATCHMAKER_YOURPORT             "ag"
#define     NET_MATCHMAKER_TARGETIP             "ah"
#define     NET_MATCHMAKER_TARGETPORT           "ai"

#define     PERIOD_MATCHMAKER_REQUESTID             10 SECONDS
#define     PERIOD_MATCHMAKER_REQUESTCONNECTION     3 SECONDS


/*
 *	MetaServer data fields
 *
 */

#define     NET_METASERVER_IP                   "da"            
#define     NET_METASERVER_PORT                 "db"
#define     NET_METASERVER_FROMIP               "dc"                                       // Where the UDP data came from (IP)
#define     NET_METASERVER_FROMPORT             "dd"                                       // Where the UDP data came from (Port)
#define     NET_METASERVER_LOCALIP              "de"                                       // Your IP on your local LAN
#define     NET_METASERVER_LOCALPORT            "df"                                       // Your port on the local system

#define     NET_METASERVER_SERVERNAME           "dg"
#define     NET_METASERVER_GAMENAME             "dh"
#define     NET_METASERVER_GAMEVERSION          "di"
#define     NET_METASERVER_NUMTEAMS             "dj"
#define     NET_METASERVER_NUMDEMOTEAMS         "dk"
#define     NET_METASERVER_MAXTEAMS             "dl"
#define     NET_METASERVER_NUMSPECTATORS        "dm"
#define     NET_METASERVER_MAXSPECTATORS        "dn"
#define     NET_METASERVER_PROTOCOL             "do"
#define     NET_METASERVER_PROTOCOLMATCH        "dp"
#define     NET_METASERVER_AUTHKEY              "dq"
#define     NET_METASERVER_AUTHKEYID            "dr"
#define     NET_METASERVER_HASHTOKEN            "ds"
#define     NET_METASERVER_PASSWORD             "dt"
#define     NET_METASERVER_RECEIVETIME          "du"

#define     NET_METASERVER_GAMEINPROGRESS       "dv"
#define     NET_METASERVER_GAMEMODE             "dw"
#define     NET_METASERVER_SCOREMODE            "dx"
#define     NET_METASERVER_GAMETIME             "dy"
#define     NET_METASERVER_DEMOSERVER           "dz"
#define		 NET_METASERVER_DELISTED		     "dA"
#define		 NET_METASERVER_RANDOM				 "dB"
#define		 NET_METASERVER_STARTTIME			 "dC"
#define		 NET_METASERVER_SEARCHRESULT		 "dD"
#define     NET_METASERVER_MAPCRC               "dE"
#define     NET_METASERVER_MAPINDEX             "dF"
#define     NET_METASERVER_MASTERPLAYER         "dG"
#define		NET_METASERVER_ORIGINVERSION		"dH"
#define     NET_METASERVER_HASPASSWORD          "dI"
#define     NET_METASERVER_CUSTOMMAPNAME        "dJ"

#define     NET_METASERVER_DEMORESTRICTED       "demo"
#define     NET_METASERVER_LANGAME              "lan"

#define     NET_METASERVER_PLAYERNAME           "pn"
#define     NET_METASERVER_PLAYERSCORE          "ps"
#define     NET_METASERVER_PLAYERALLIANCE       "pa"

#define     NET_METASERVER_NUMHUMANTEAMS        "ea"
#define     NET_METASERVER_MODPATH              "eb"

#define		NET_METASERVER_LANGUAGE				"ec"

/*
 *  MetaServer data request types
 *
 */

#define     NET_METASERVER_DATATYPE             "DataType"
#define     NET_METASERVER_DATA_TIME            "Time"
#define     NET_METASERVER_DATA_MOTD            "MOTD"
#define     NET_METASERVER_DATA_DEMOLIMITS      "DemoLimits"
#define     NET_METASERVER_DATA_DEMO2LIMITS     "Demo2Limits"
#define     NET_METASERVER_DATA_KEYLIMITS       "KeyLimits"
#define     NET_METASERVER_DATA_LATESTVERSION   "LatestVersion"
#define     NET_METASERVER_DATA_UPDATEURL       "UpdateURL"
#define     NET_METASERVER_DATA_STRESSSERVERS   "StressServers"
#define     NET_METASERVER_DATA_STRESSCLIENTS   "StressClients"
#define     NET_METASERVER_DATA_SERVERTTL       "ServerTTL"

#define     NET_METASERVER_DATATIMETOLIVE       "DataTTL"
#define     NET_METASERVER_DATAEXPIRED          "DataExpired"
#define     NET_METASERVER_TIME_HOURS           "TimeHours"
#define     NET_METASERVER_TIME_MINUTES         "TimeMinutes"
#define     NET_METASERVER_MAXDEMOGAMESIZE      "MaxDemoGameSize"
#define     NET_METASERVER_MAXDEMOPLAYERS       "MaxDemoPlayers"
#define     NET_METASERVER_ALLOWDEMOSERVERS     "AllowDemoServers"
#define     NET_METASERVER_ALLOWDEMOLANPLAY     "AllowDemoLanPlay"

#define		NET_METASERVER_DEMOMODES            "DMo"
#define		NET_METASERVER_DEMOMAPS				"DMa"


/*
 *  MetaServer Steam data
 *
 */

#define     NET_METASERVER_STEAM_AUTHENTICATE          "sa"

#define     NET_METASERVER_STEAM_ENCRYPTIONDETAILS     "sb"
#define     NET_METASERVER_STEAM_ENCRYPTIONKEY         "sc"
#define     NET_METASERVER_STEAM_USERID                "sd"
#define     NET_METASERVER_STEAM2KEY                   "se"
#define     NET_METASERVER_STEAM3KEY                   "sf"
#define     NET_METASERVER_STEAM_AUTHRESULT            "sg"
#define     NET_METASERVER_STEAM_AUTHRESULTMESSAGE     "sh"


#endif
