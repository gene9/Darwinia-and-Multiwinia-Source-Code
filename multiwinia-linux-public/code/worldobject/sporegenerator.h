
#ifndef _included_sporegenerator_h
#define _included_sporegenerator_h

#include "worldobject/entity.h"

#define SPOREGENERATOR_NUMTAILS 4


class SporeGenerator : public Entity
{
public:    
    double       m_retargetTimer;
    double       m_eggTimer;
    Vector3     m_targetPos;
    int         m_spiritId;
    
protected:    
    bool    SearchForRandomPos      ();
    bool    SearchForSpirits        ();

    bool    AdvanceToTargetPosition ();                
    bool    AdvanceIdle             ();
    bool    AdvanceEggLaying        ();
    bool    AdvancePanic            ();
    
    void    RenderTail( Vector3 const &_from, Vector3 const &_to, double _size );

protected:    
    ShapeMarker     *m_eggMarker;
    ShapeMarker     *m_tail[SPOREGENERATOR_NUMTAILS];

    enum
    {
        StateIdle,
        StateEggLaying,
        StatePanic
    };
    int m_state;

public:
    SporeGenerator();

    void Begin          ();
    bool Advance        ( Unit *_unit );
    bool ChangeHealth   ( int _amount, int _damageType );

    bool IsInView           ();
    void Render             ( double _predictionTime );
    bool RenderPixelEffect  ( double _predictionTime );

    void ListSoundEvents    ( LList<char *> *_list );
};



#endif
