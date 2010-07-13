
#ifndef _included_multiwiniazone_h
#define _included_multiwiniazone_h

#include "worldobject/building.h"
#include "lib/rgb_colour.h"

class AITarget;
class Flag;

#define BLITZKRIEG_FLAGCAPTURE_RANGE 20.0f

class MultiwiniaZone : public Building
{
public:
    double      m_size;
    double      m_life;                         // -1 = live forever.  > 0 = life remaining until delete

    double      m_recountTimer;

    int         m_totalCount;
    int         m_teamCount[NUM_TEAMS];

    Flag        *m_blitzkriegFlag;
    AITarget    *m_aiTarget;
    
    LList<int>  m_blitzkriegLinks;
    int         m_numStandardLinks;
    
    double      m_blitzkriegOwnership;
    int         m_blitzkriegCaptureTeam;
    int         m_blitzkriegUpOrDown;
    bool        m_blitzkriegLocked;

    double      m_blitzkriegUnderAttackMessageTimer;

    static int  s_blitzkriegBaseZone[NUM_TEAMS];

protected:
    void Advance_KingOfTheHill();
    void Advance_CaptureTheStatue();
    void Advance_Blitzkrieg();

    void RenderBlitzkrieg();

    static double GetBlitzkriegPriority( int _zoneId, int _teamId );

public:
    MultiwiniaZone();
    ~MultiwiniaZone();

    void Initialise     ( Building *_template );
    void SetDetail      ( int _detail );

    bool Advance        ();
    void Render         ( double predictionTime );        
    void RenderAlphas   ( double predictionTime );
    void RenderZoneEdge ( double startAngle, double totalAngle, RGBAColour colour, bool animated );

    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                 double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);

    void ListSoundEvents        ( LList<char *> *_list );

    int  GetBuildingLink();                             // Allows a building to link to another
    void SetBuildingLink( int _buildingId );            // eg control towers

    bool    IsBlitzkriegBaseZone();
    bool    IsBlitzkriegZoneLocked();
    int     GetBaseTeamId();

    void Read       ( TextReader *_in, bool _dynamic );     
    void Write      ( TextWriter *_out );	

    static int  GetNumZones( int _teamId );
};


#endif
