#ifndef _included_nuke_h
#define _included_nuke_h

#include "worldobject/entity.h"

class Nuke : public Entity
{
protected:

    double               m_totalDistance;

    LList<Vector3 *>    m_history;
    double               m_historyTimer;

    bool                m_exploded;
    bool                m_launched;
    bool                m_armed;

    double               m_casualtyMessageTimer;
    int                 m_numDeaths;

    Vector3             m_subPos;
    Vector3             m_subVel;
    double               m_subTimer;

    double               m_launchTimer;

public:
    Vector3 m_targetPos;
    bool    m_renderMarker;

protected:

    void RenderHistory( double _predictionTime );
    void RenderDeaths();
    void RenderSub( double _predictionTime );
    void RenderGroundMarker();

public:
    Nuke();
    ~Nuke();

    void Begin          ();

    bool Advance        ( Unit *_unit );
    bool AdvanceToTarget();
    bool AdvanceLaunched();
    bool AdvanceSub();

    void Render         ( double _predictionTime );

    void Launch         ();

    void ListSoundEvents( LList<char *> *_list );

};


#endif
