
#ifndef _included_anthill_h
#define _included_anthill_h

#include "worldobject/building.h"

#define ANTHILL_SEARCHRANGE     400.0 


class TextWriter;


struct AntObjective
{
    Vector3         m_pos;
    WorldObjectId   m_targetId;
    int             m_numToSend;
};


class AntHill : public Building
{
protected:
    LList<AntObjective *> m_objectives;

    double   m_objectiveTimer;
    double   m_spawnTimer;
    double   m_eggConvertTimer;
    int     m_health;
    int     m_unitId;
    int     m_populationLock;
    bool    m_renderDamaged;
    int     m_antInsideCheck;

protected:
    bool SearchingArea          ( Vector3  _pos );
    bool TargettedEntity        ( WorldObjectId _id );

    bool SearchForSpirits       ( Vector3 &_pos );
    bool SearchForDarwinians    ( Vector3 &_pos, WorldObjectId &_id );
    bool SearchForEnemies       ( Vector3 &_pos, WorldObjectId &_id );

    bool SearchForScoutArea     ( Vector3 &_pos );

    bool PopulationLocked();

    void SpawnAnt               ( Vector3 &waypoint, WorldObjectId targetId );

public:
    int m_numAntsInside;
    int m_numSpiritsInside;
    
public:
    AntHill();
	~AntHill();

    void Initialise( Building *_template );

    bool Advance();
    void Render ( double _predictionTime );
    void Damage ( double _damage );
	void Destroy( double _intensity );

    void Burn   ();

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );		
    
    void ListSoundEvents( LList<char *> *_list );
};



#endif
