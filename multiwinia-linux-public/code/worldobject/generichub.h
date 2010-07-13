
#ifndef _included_dynamicbuilding_h
#define _included_dynamicbuilding_h

#include "worldobject/building.h"


class TextWriter;


// ****************************************************************************
// Class DynamicBase
// ****************************************************************************

class DynamicBase : public Building
{
protected:
    int             m_buildingLink;

public:
    char            m_shapeName[256];

public:
    DynamicBase();
    
    void Initialise     ( Building *_template );
    bool Advance        ();
    void Render         ( double _predictionTime );

    void ListSoundEvents( LList<char *> *_list );

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );

    Vector3 GetPowerLocation();

    void SetShapeName   ( char *_shapeName );
};



// ****************************************************************************
// Class DynamicHub
// ****************************************************************************

class DynamicHub : public DynamicBase
{
protected:
    bool    m_enabled;          // set to true once all connected nodes are active
    bool    m_reprogrammed;     // set to true if a connected Control Tower is reprogrammed, or no tower is connected

    int     m_numLinks;         // the number of Nodes linked to this Hub
    int     m_activeLinks;      // the number of active nodes linked to this hub

public:
    int     m_currentScore;     // the current number of 'points' this hub has recieved from connected Nodes
    int     m_requiredScore;    // the required number of 'points' before this Hub will activate
    int     m_minActiveLinks;   // if scores mode is being used, this is the mimimum number of buildings required in addition to the score requirement

public:
    DynamicHub();

    void Initialise     ( Building *_template );

    void ReprogramComplete  ();
    void GetObjectiveCounter( UnicodeString& _dest );

    void ListSoundEvents    ( LList<char *> *_list );

    bool Advance            ();
    void Render             ( double _predictionTime );

    void ActivateLink       ();
    void DeactivateLink     ();

    bool ChangeScore        ( int _points );

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );

    int  PointsPerHub    ();     // the number of points each node supplies if there is a minimum active node limit
};


// ****************************************************************************
// Class DynamicNode
// ****************************************************************************

class DynamicNode : public DynamicBase
{
protected:    
    bool m_operating;

public:
    int m_scoreValue;       // the number of points that will be added to the connected hubs score every second, if this Node is active
    double m_scoreTimer;
    int m_scoreSupplied;     // the number of points this node has already given the hub
    
public:
    DynamicNode();


    void Initialise     ( Building *_template );
    bool Advance        ();
        
    void Render         ( double _predictionTime );
    void RenderPorts    ();
    void RenderAlphas   ( double _predictionTime );

    void ListSoundEvents( LList<char *> *_list );

    void ReprogramComplete();

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );
};

#endif
