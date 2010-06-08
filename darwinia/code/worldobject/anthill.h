
#ifndef _included_anthill_h
#define _included_anthill_h

#include "worldobject/building.h"

#define ANTHILL_SEARCHRANGE     400.0f 


class FileWriter;


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

    float   m_objectiveTimer;
    float   m_spawnTimer;
    float   m_eggConvertTimer;
    int     m_health;
    int     m_unitId;
    int     m_populationLock;
    bool    m_renderDamaged;
    
protected:
    bool SearchingArea          ( Vector3  _pos );
    bool TargettedEntity        ( WorldObjectId _id );

    bool SearchForSpirits       ( Vector3 &_pos );
    bool SearchForDarwinians    ( Vector3 &_pos, WorldObjectId &_id );
    bool SearchForEnemies       ( Vector3 &_pos, WorldObjectId &_id );

    bool SearchForScoutArea     ( Vector3 &_pos );

    bool PopulationLocked();

public:
    int m_numAntsInside;
    int m_numSpiritsInside;
    
public:
    AntHill();

    void Initialise( Building *_template );

    bool Advance();
    void Render ( float _predictionTime );
    void Damage ( float _damage );
	void Destroy( float _intensity );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );		
    
    void ListSoundEvents( LList<char *> *_list );
};



#endif
