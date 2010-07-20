#ifndef _included_cave_h
#define _included_cave_h

#include "worldobject/building.h"

class ShapeMarker;


class Cave : public Building
{
protected:
    int             m_troopType;
    int             m_unitId;
    float           m_spawnTimer;
    bool            m_dead;

    ShapeMarker     *m_spawnPoint;

public:
    Cave();

    bool Advance();
    void Damage( float _damage );
};


#endif
