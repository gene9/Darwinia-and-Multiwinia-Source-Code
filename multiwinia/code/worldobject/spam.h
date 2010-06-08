
#ifndef _included_spam_h
#define _included_spam_h

#include "worldobject/building.h"

#define SPAM_RELOADTIME             60.0 
#define SPAM_DAMAGE                 100.0 


class Spam : public Building
{
protected:
    double   m_timer;
    double   m_damage;
            
    bool    m_research;
    bool    m_onGround;
    bool    m_activated;
    
public:
    Spam();

    void Initialise( Building *_template );
    void SetDetail( int _detail );
    void Damage( double _damage );
	void Destroy( double _intensity );

    void Render( double _predictionTime );
    void RenderAlphas( double _predictionTime );

    bool Advance();

    void ListSoundEvents( LList<char *> *_list );

    void SetAsResearch();
    void SendFromHeaven();
    void SpawnInfection();
};



// ****************************************************************************
//  Class SpamInfection
// ****************************************************************************

#define SPAMINFECTION_MINSEARCHRANGE    100.0 
#define SPAMINFECTION_MAXSEARCHRANGE    200.0 
#define SPAMINFECTION_LIFE              10.0
#define SPAMINFECTION_TAILLENGTH        30

class SpamInfection : public WorldObject
{
protected:
    enum
    {
        StateIdle,
        StateAttackingEntity,
        StateAttackingSpirit
    };

    int             m_state;
    double           m_retargetTimer;
    WorldObjectId   m_targetId;
    int             m_spiritId;
    Vector3         m_targetPos;
    double           m_life;
    int             m_startCounter;
    
    LList           <Vector3> m_positionHistory;
    
protected:
    void AdvanceIdle();
    void AdvanceAttackingEntity();
    void AdvanceAttackingSpirit();
    bool AdvanceToTargetPosition();

    bool SearchForEntities();
    bool SearchForRandomPosition();
    bool SearchForSpirits();

public:
     int m_parentId;                     // id of Spam that spawned me

public:
    SpamInfection();

    bool Advance	();
    void Render	    ( double _time );
};


#endif
