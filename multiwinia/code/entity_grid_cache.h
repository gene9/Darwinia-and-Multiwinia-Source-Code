#ifndef _included_entitygridcache_h
#define _included_entitygridcache_h

class WorldObjectId;


class EntityGridCacheElement
{   
public:
    float           m_timeIndex;
    float           m_worldX;
    float           m_worldZ;
    float           m_rangeSqd;
    float           m_halfRangeSqd;
    unsigned char   m_teamId;
    int             m_numFound;
    std::vector<WorldObjectId> m_ids;

public:
    EntityGridCacheElement();
};


// ============================================================================

#define ENTITYGRIDCACHE_MAXAGE     1.0f


class EntityGridCache
{
public:
    FastDArray<EntityGridCacheElement *>  m_cache;

protected:
    int     FindCacheHit( double worldX, double worldZ, double range, unsigned char teamId );
    
public:
    EntityGridCache();
	~EntityGridCache();

    std::vector<WorldObjectId> *GetEnemies(double _worldX, double _worldZ, double _range,
                                           int *_numFound, unsigned char _myTeam);

	void Render();
    void Advance();
};



#endif
