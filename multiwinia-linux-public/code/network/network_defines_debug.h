#ifndef _included_networkdefinesdebug_h
#define _included_networkdefinesdebug_h


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


#define     NET_DARWINIA_MESSAGE                      "DARWINIA"
#define     NET_DARWINIA_COMMAND                      "CMD"



/*
 *      CLIENT TO SERVER
 *      Commands
 *
 */


#define     NET_DARWINIA_CLIENT_JOIN					"ClientJoin"
#define     NET_DARWINIA_CLIENT_LEAVE					"ClientLeave"

#define     NET_DARWINIA_REQUEST_TEAM					"RequestTeam"
#define		NET_DARWINIA_ALIVE							"Alive"
#define		NET_DARWINIA_PAUSE							"Pause"
#define		NET_DARWINIA_SELECTUNIT						"SelectUnit"
#define		NET_DARWINIA_AIMBUILDING					"AimBuilding"
#define		NET_DARWINIA_TOGGLELASERFENCE				"ToggleLaserFence"
#define		NET_DARWINIA_RUNPROGRAM						"RunProgram"
#define		NET_DARWINIA_TARGETPROGRAM					"TargetProgram"
#define		NET_DARWINIA_SELECTPROGRAM					"SelectProgram"
#define		NET_DARWINIA_TERMINATE						"Terminate"
#define		NET_DARWINIA_SHAMANSUMMON					"ShamanSummon"
#define		NET_DARWINIA_SELECTDGS  					"SelectDGs"
#define		NET_DARWINIA_ORDERDGS  					    "OrderDGs"
#define    NET_DARWINIA_PROMOTE                        "Promote"

#define		NET_DARWINIA_PAUSE							"Pause"
#define     NET_DARWINIA_SYNCHRONISE					"Sync"
#define     NET_DARWINIA_PING							"Sync"

#define     NET_DARWINIA_START_GAME						"StartGame"
#define		NET_DARWINIA_MAPNAME						"MapName"
#define		NET_DARWINIA_GAMETYPE						"GameType"
#define		NET_DARWINIA_GAMEPARAMS						"GameParams"
#define		NET_DARWINIA_RESEARCHLEVEL					"ResearchLevel"


/*
 *	    CLIENT TO SERVER
 *      Data fields
 *
 */

#define     NET_DARWINIA_TEAMTYPE						"TeamType"
#define		NET_DARWINIA_DESIREDTEAMID					"DesiredTeamId"
#define     NET_DARWINIA_LASTPROCESSEDSEQID				"LastProcSeqId"
#define     NET_DARWINIA_SYNCVALUE						"SyncValue"

#define     NET_DARWINIA_TEAMID							"TeamId"

#define     NET_DARWINIA_POSITION						"Position"
#define		NET_DARWINIA_X								"X"
#define		NET_DARWINIA_Y								"Y"
#define     NET_DARWINIA_Z								"Z"	
#define     NET_DARWINIA_ALIVE_FLAGS					"Flags"
#define     NET_DARWINIA_ALIVE_DIRECTUNITMOVEDX			"DUMoveDX"
#define		NET_DARWINIA_ALIVE_DIRECTUNITMOVEDY			"DUMoveDY"
#define     NET_DARWINIA_ALIVE_DIRECTUNITFIREDX			"DUFireDX"
#define     NET_DARWINIA_ALIVE_DIRECTUNITFIREDY			"DUFireDY"

#define		NET_DARWINIA_UNITID							"UnitID"
#define		NET_DARWINIA_ENTITYID						"EntityID"
#define		NET_DARWINIA_BUILDINGID						"BuildingID"
#define		NET_DARWINIA_ENTITYTYPE						"EntityType"
#define    NET_DARWINIA_RADIUS                         "Radius"

#define		NET_DARWINIA_PROGRAM						"Program"

#define		NET_DARWINIA_RANDSEED						"RandSeed"

/*
 *	    SERVER TO CLIENT
 *      Commands and data
 */

#define     NET_DARWINIA_DISCONNECT						"Disconnect"
#define     NET_DARWINIA_CLIENTHELLO					"ClientHello"
#define     NET_DARWINIA_CLIENTGOODBYE					"ClientGoodbye"
#define     NET_DARWINIA_CLIENTID						"ClientID"

#define     NET_DARWINIA_TEAMASSIGN						"TeamAssign"
#define     NET_DARWINIA_NETSYNCERROR					"SyncError"
#define     NET_DARWINIA_NETSYNCFIXED					"SyncFixed"
#define     NET_DARWINIA_SYNCERRORID					"ID"
#define     NET_DARWINIA_SYNCERRORSEQID					"SyncSeqID"
#define     NET_DARWINIA_UPDATE							"Update"
#define     NET_DARWINIA_VERSION						"Version"
#define     NET_DARWINIA_SYSTEMTYPE						"SystemType"

#define     NET_DARWINIA_PREVUPDATE						"PrevUpdate"
#define     NET_DARWINIA_NUMEMPTYUPDATES				"NumEmptyUpdates"

//
#define     NET_DARWINIA_SEQID							"SeqId"
#define     NET_DARWINIA_LASTSEQID						"LastSeqId"


/*
 *		FTP MANAGER
 *
 */

#define		NET_DARWINIA_FILESEND						"FTP"
#define		NET_DARWINIA_FILENAME						"Name"
#define		NET_DARWINIA_FILESIZE						"Size"
#define		NET_DARWINIA_BLOCKNUMBER					"Block"
#define		NET_DARWINIA_BLOCKDATA						"Data"

/*
 *	    Other Data fields
 *
 */

#define     NET_DARWINIA_FROMIP							"FromIP"
#define     NET_DARWINIA_FROMPORT						"FromPort"

#define		NET_DARWINIA_FROMPLAYER						"FromPlayer"

#endif
