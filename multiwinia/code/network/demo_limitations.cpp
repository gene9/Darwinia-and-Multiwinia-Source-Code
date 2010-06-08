#include "lib/universal_include.h"
#include "demo_limitations.h"
#include "lib/tosser/directory.h"
#include "lib/metaserver/metaserver_defines.h"
#include "multiwinia.h"
#include "mapcrc.h"

#include <algorithm>

DemoLimitations::DemoLimitations()
{
	Initialise( NULL );
}

void DemoLimitations::Initialise( Directory *_dir )
{
	Directory *demoLims = _dir;

    if( demoLims &&
		demoLims->HasData( NET_METASERVER_MAXDEMOGAMESIZE, DIRECTORY_TYPE_INT ) &&
		demoLims->HasData( NET_METASERVER_MAXDEMOPLAYERS, DIRECTORY_TYPE_INT ) &&
		demoLims->HasData( NET_METASERVER_ALLOWDEMOSERVERS, DIRECTORY_TYPE_BOOL ) &&
		demoLims->HasData( NET_METASERVER_DEMOMODES, DIRECTORY_TYPE_INT ) &&
		demoLims->HasData( NET_METASERVER_DEMOMAPS, DIRECTORY_TYPE_INTS )
		)
	{
        m_maxGameSize = demoLims->GetDataInt( NET_METASERVER_MAXDEMOGAMESIZE );
        m_maxDemoPlayers = demoLims->GetDataInt( NET_METASERVER_MAXDEMOPLAYERS );
        m_allowDemoServers = demoLims->GetDataBool( NET_METASERVER_ALLOWDEMOSERVERS );

		m_allowedGameModes = demoLims->GetDataInt( NET_METASERVER_DEMOMODES );
		demoLims->GetDataInts( NET_METASERVER_DEMOMAPS, m_allowedMapCRCs );		
	}
	else
	{
        m_maxGameSize = 4;
        m_maxDemoPlayers = 4;
        m_allowDemoServers = true;
		m_allowedGameModes = (1 << Multiwinia::GameTypeKingOfTheHill) | 
                             (1 << Multiwinia::GameTypeCaptureTheStatue) |
                             (1 << Multiwinia::GameTypeRocketRiot);
		m_allowedMapCRCs.resize(5);
		m_allowedMapCRCs[0] = MAPID_MP_KOTH_2P_1;
		m_allowedMapCRCs[1] = MAPID_MP_KOTH_4P_1;
		m_allowedMapCRCs[2] = MAPID_MP_CTS_2P_1;
		m_allowedMapCRCs[3] = MAPID_MP_KOTH_3P_1;
        m_allowedMapCRCs[4] = MAPID_MP_ROCKETRIOT_3P_1;
	}
}

bool DemoLimitations::GameModePermitted( int _gameMode ) const
{
	return (m_allowedGameModes & (1 << _gameMode)) != 0;
}

bool DemoLimitations::MapPermitted( int _mapCRC ) const
{
	return std::find( m_allowedMapCRCs.begin(), m_allowedMapCRCs.end(), _mapCRC ) != m_allowedMapCRCs.end();
}