#ifndef _included_factory_h
#define _included_factory_h

#include "worldobject/building.h"
#include "worldobject/entity.h"
#include "worldobject/spiritstore.h"


class TextWriter;


class Factory: public Building
{
public:
    unsigned char   m_troopType;
    unsigned char   m_stats[Entity::NumStats];

	int				m_initialCapacity;		// Read from level file

    int             m_unitId;
    int             m_numToCreate;
    int             m_numCreated;
    
    double           m_timeToCreate;         // Total Time to create ALL troops
    double           m_timeSoFar;

    enum
    {
        StateUnused,
        StateCreating,
        StateRecharging
    };
    int m_state;

    SpiritStore     m_spiritStore;

public:
    Factory();

    void Initialise( Building *_template );

    void Render         ( double predictionTime );
    void RenderAlphas   ( double predictionTime );

    bool Advance();
    void AdvanceStateUnused();
    void AdvanceStateCreating();
    void AdvanceStateRecharging();

    void SetTeamId( int _teamId );

    void RequestUnit( unsigned char _troopType, int _numToCreate );

	void Read(TextReader *_in, bool _dynamic);
	void Write(TextWriter *_out);
};

#endif
