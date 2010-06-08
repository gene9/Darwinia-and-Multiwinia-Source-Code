#ifndef __DEMO_LIMITATIONS_H
#define __DEMO_LIMITATIONS_H

#include <vector>

class Directory;

class DemoLimitations 
{
public:

	DemoLimitations();
	void Initialise( Directory * );

	bool GameModePermitted( int _gameMode ) const;
	bool MapPermitted( int _mapCRC ) const;

public:
	int m_maxGameSize;
	int m_maxDemoPlayers;
	int m_allowDemoServers;

	int m_allowedGameModes;
	std::vector<int> m_allowedMapCRCs;

};


#endif
