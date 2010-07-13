#ifndef _included_aiobjective_h
#define _included_aiobjective_h

#include "worldobject/building.h"


/*
An AI Objective is used by the AI to track progress through assault levels
They follow the following rules:
*   An AIObjective may have 1 or more AIObjectiveMarkers
    AIObjectiveMarkers mark positions that the AI will send Darwinians to accomplish that part of the objective
    The position of the AIObjective itself doesnt matter
    Once all the AIObjectiveMarkers connected to an AIObjective have been captured by a single team (or group in coop games), the objective will be complete, and the next one will be activate
    The AIObjective should be linked to the next AIObjective building
    if an AIObjective isnt linked to by another AIObjective, it will become the starting objective for the AI. If there is more than one in this state, one will be selected randomly, allowing for multiple paths across a map
    The only AIObjective that shouldnt be linked to another is the final one, which should have objective markers on each of the Control Stations attached to the PulseBomb for that map
Gary
*/
class AIObjective : public Building
{
public:
    int         m_nextObjective;    // the objective which becomes the target once this one has been captured
    LList<int>  m_objectiveMarkers;

    bool        m_active;
    bool        m_defenseObjective;
    double      m_timer;

    //int         m_armourObjective;      // if set to true, the markers attached to this objective become Armour targets - if there are any objectives with this flag on a level, armour will only drop at those objectives
    int         m_armourMarker;
    int         m_wallTarget;           

    static bool s_objectivesInitialised;

public:
    AIObjective();
    void Initialise( Building *_template) ;

    bool Advance();

    void AdvanceStandard();
    void AdvanceDefensive();

    void RenderAlphas( double _predictionTime );

    void RegisterObjectiveMarker( int _markerId, bool _armourMarker = false );

    void SetBuildingLink( int _buildingId );
    int  GetBuildingLink();

    void Read(TextReader *_in, bool _dynamic);
    void Write( TextWriter *_out );

    static void InitialiseObjectives();

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
};

class AIObjectiveMarker : public Building
{
public:
    double   m_scanRange;
    double   m_timer;

    bool    m_registered;
    bool    m_pickupAvailable;
    bool    m_defenseMarker;

    int     m_objectiveId;
    int     m_armourObjective;
    int     m_pickupOnly;
    int     m_objectiveBuildingId;
    int     m_defaultTeam;

public:
    AIObjectiveMarker();
    void Initialise( Building *_template) ;

    bool Advance();
    void AdvanceStandard();
    void AdvanceDefensive();
    void AdvanceRocketRiot();

    void RenderAlphas( double _predictionTime );

    void SetBuildingLink( int _buildingId );
    int  GetBuildingLink();

    void Read(TextReader *_in, bool _dynamic);
    void Write( TextWriter *_out );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);
};

#endif