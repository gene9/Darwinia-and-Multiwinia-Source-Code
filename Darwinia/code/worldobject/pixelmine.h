
#ifndef _included_pixelmine_h
#define _included_pixelmine_h

#include "worldobject/building.h"

#define MINE_SPAWN_INTERVAL			12.0f	// How many seconds between each spawn
#define MINE_POLYGON_SEARCH_RADIUS	50.0f	// How far to search for nearby polygons
#define MINE_MAX_NEARBY_POLYGONS	12		// Stop spawning if there are this many polyons already here

class PixelMine : public Building
{
protected:
    ShapeMarker     *m_door;
	
	float m_nextSpawn;
public:

	float m_spawnDelay;

    PixelMine();

    void GetExitPoint( Vector3 &_pos, Vector3 &_front );
    void Initialise ( Building *_template );
    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

	bool Advance();
};


#endif