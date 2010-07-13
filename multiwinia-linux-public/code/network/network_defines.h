#ifndef _included_networkdefines_h
#define _included_networkdefines_h

#include "globals.h"

/*
 *	    Put this define in place if you want the optimised network symbols
 *      to be replaced with human readable symbols 
 *      (much higher bandwidth usage)
 *
 */

/*
 *	    Basic Stuff
 */


#define     PREFS_NETWORKCLIENTPORT                 "NetworkClientPort"
#define     PREFS_NETWORKSERVERPORT                 "NetworkServerPort"
#define     PREFS_NETWORKUSEPORTFORWARDING          "NetworkUsePortForwarding"
#define     PREFS_NETWORKTRACKSYNCRAND              "NetworkTrackSynchronisation"


/*
 *      All DARWINIA network messages should 
 *      have this name.  If not then they're
 *      not for us.
 *
 */


#define     NET_DARWINIA_MESSAGE                      "ba"
#define     NET_DARWINIA_COMMAND                      "bb"



/*
 *      CLIENT TO SERVER
 *      Commands
 *
 */


#define     NET_DARWINIA_CLIENT_JOIN					"ca"
#define     NET_DARWINIA_CLIENT_LEAVE					"cb"
#define		NET_DARWINIA_REQUEST_SPECTATOR				"cba"

#define     NET_DARWINIA_REQUEST_TEAM					"cc"
#define    NET_DARWINIA_TOGGLE_READY                   "cca"
#define		NET_DARWINIA_SET_TEAM_READY					"ccb"
#define		NET_DARWINIA_ALIVE							"cd"
#define		NET_DARWINIA_SELECTUNIT						"ce"
#define		NET_DARWINIA_AIMBUILDING					"cf"
#define		NET_DARWINIA_TOGGLELASERFENCE				"cg"
#define		NET_DARWINIA_RUNPROGRAM						"ch"
#define		NET_DARWINIA_TARGETPROGRAM					"ci"
#define		NET_DARWINIA_SELECTPROGRAM					"cj"
#define		NET_DARWINIA_TERMINATE						"ck"
#define		NET_DARWINIA_SHAMANSUMMON					"cl"
#define		NET_DARWINIA_SELECTDGS  					"cm"
#define		NET_DARWINIA_ORDERDGS  					    "cn"
#define     NET_DARWINIA_PROMOTE                       "co"
#define    NET_DARWINIA_ORDERGOTO                      "coa"
#define    NET_DARWINIA_DIRECTROUTE                    "cob"
#define    NET_DARWINIA_TOGGLEENTITY                   "coc"
#define    NET_DARWINIA_FORCEONLYMOVE                  "cod"
#define    NET_DARWINIA_SECONDARYFUNCTION              "coe"

#define		NET_DARWINIA_PAUSE							"cp"
#define     NET_DARWINIA_SYNCHRONISE					"cq"
#define     NET_DARWINIA_PING							"cr"

#define     NET_DARWINIA_START_GAME						"cs"
#define		NET_DARWINIA_MAPNAME						"ct"
#define		NET_DARWINIA_GAMETYPE						"cu"
#define		NET_DARWINIA_GAMEPARAMS						"cv"
#define		NET_DARWINIA_RESEARCHLEVEL					"cw"

#define    NET_DARWINIA_NEXTWEAPON                     "cx"
#define    NET_DARWINIA_PREVWEAPON                     "cy"
#define    NET_DARWINIA_FOLLOW                         "cya"
#define    NET_DARWINIA_NONE                           "cyb"
#define    NET_DARWINIA_REQUEST_TEAMNAME               "cz"
#define    NET_DARWINIA_REQUEST_COLOUR                 "czz"

#define    NET_DARWINIA_SEND_CHATMESSAGE               "cczz"

#define    NET_DARWINIA_TEAM_SCORES                    "c3"

/*
 *	    CLIENT TO SERVER
 *      Data fields
 *
 */

#define     NET_DARWINIA_TEAMTYPE						"da"
#define		NET_DARWINIA_DESIREDTEAMID					"db"
#define     NET_DARWINIA_LASTPROCESSEDSEQID				"dc"
#define     NET_DARWINIA_SYNCVALUE						"dd"

#define     NET_DARWINIA_TEAMID							"de"

#define     NET_DARWINIA_POSITION						"df"

#define		NET_DARWINIA_ALIVE_DATA						"dg"

#define		NET_DARWINIA_UNITID							"dl"
#define		NET_DARWINIA_ENTITYID						"dm"
#define		NET_DARWINIA_BUILDINGID						"dn"
#define		NET_DARWINIA_ENTITYTYPE						"do"
#define     NET_DARWINIA_RADIUS                         "dp"

#define		NET_DARWINIA_PROGRAM						"dq"

#define		NET_DARWINIA_RANDSEED						"dr"

#define     NET_DARWINIA_TEAMNAME                       "ds"
#define     NET_DARWINIA_COLOURID                       "dt"
#define     NET_DARWINIA_UNICODEMESSAGE                 "dw"
#define		NET_DARWINIA_ENCRYPTION_PADDING_CTS			"dx"

#define     NET_DARWINIA_SERVER_PASSWORD                "dy"
#define     NET_DARWINIA_CHRISTMASMODE                  "dz"

/*
 *	    SERVER TO CLIENT
 *      Commands and data
 */

#define     NET_DARWINIA_DISCONNECT						"ea"
#define     NET_DARWINIA_CLIENTHELLO					"eb"
#define     NET_DARWINIA_CLIENTGOODBYE					"ec"
#define     NET_DARWINIA_CLIENTID						"ed"

#define     NET_DARWINIA_TEAMASSIGN						"ef"
#define     NET_DARWINIA_NETSYNCERROR					"eg"
#define     NET_DARWINIA_NETSYNCFIXED					"eh"
#define     NET_DARWINIA_SYNCERRORID					"ei"
#define     NET_DARWINIA_SYNCERRORSEQID					"ej"
#define     NET_DARWINIA_UPDATE							"ek"
#define     NET_DARWINIA_VERSION						"el"
#define     NET_DARWINIA_SYSTEMTYPE						"em"

#define     NET_DARWINIA_PREVUPDATE						"en"
#define     NET_DARWINIA_NUMEMPTYUPDATES				"eo"


#define     NET_DARWINIA_SEQID							"ep"
#define     NET_DARWINIA_LASTSEQID						"eq"
#define		NET_DARWINIA_XUID							"er"
#define		NET_DARWINIA_ASSIGN_SPECTATOR				"es"
#define		NET_DARWINIA_CLIENTISDEMO					"et"
#define		NET_DARWINIA_ENCRYPTION_PADDING_STC			"eu"
#define		NET_DARWINIA_SERVER_FULL					"ev"
#define		NET_DARWINIA_MAKE_SPECTATOR					"ew"
#define     NET_DARWINIA_SPECTATORASSIGN				"ex"

#define    NET_DARWINIA_SET_PLAYERACHIEVEMENTS			"ey"
#define	   NET_DARWINIA_PLAYERACHIEVEMENTS				"ez"


/*
 *		FTP MANAGER
 *
 */

#define		NET_DARWINIA_FILESEND						"fa"
#define		NET_DARWINIA_FILENAME						"fb"
#define		NET_DARWINIA_FILESIZE						"fc"
#define		NET_DARWINIA_BLOCKNUMBER					"fd"
#define		NET_DARWINIA_BLOCKDATA						"fe"

/*
 *	    Other Data fields
 *
 */

#define     NET_DARWINIA_FROMIP							"ga"
#define     NET_DARWINIA_FROMPORT						"gb"

#define		NET_DARWINIA_FROMPLAYER						"gc"

/*
 *		LOBBY GAME OPTIONS
 */

#define     NET_DARWINIA_SETGAMEOPTIONINT				"ha"
#define     NET_DARWINIA_SETGAMEOPTIONARRAYELEMENTINT	"hb"
#define		NET_DARWINIA_SETGAMEOPTIONARRAYINT			"hc"
#define     NET_DARWINIA_SETGAMEOPTIONSTRING			"hd"
#define		NET_DARWINIA_GAMEOPTION_NAME				"he"
#define		NET_DARWINIA_GAMEOPTION_INDEX				"hf"
#define		NET_DARWINIA_GAMEOPTION_VALUE				"hg"
#define		NET_DARWINIA_GAMEOPTION_VERSION				"hi"
#define		NET_DARWINIA_GAMEOPTION_PDLC_BITMASK		"hj"
#define     NET_DARWINIA_JOIN_VIA_INVITE                "hk"


#define		GAMEOPTION_GAME_TYPE						0
#define		GAMEOPTION_GAME_PARAM						1
#define		GAMEOPTION_GAME_RESEARCH_LEVEL				2
#define		GAMEOPTION_MAPNAME							3
#define     GAMEOPTION_AIMODE                           4
#define     GAMEOPTION_COOPMODE                         5
#define     GAMEOPTION_SINGLEASSAULT                    6
#define     GAMEOPTION_BASICCRATES                      7
#define     GAMEOPTION_TEAMSSTATUS_LBOUND               8
#define     GAMEOPTION_TEAMSSTATUS_UBOUND               (GAMEOPTION_TEAMSSTATUS_LBOUND + (NUM_TEAMS - 1))
#define		GAMEOPTION_NUMGAMEOPTIONS					(GAMEOPTION_TEAMSSTATUS_UBOUND + 1)
#endif
