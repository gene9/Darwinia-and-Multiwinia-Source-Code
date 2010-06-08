#ifndef _included_bridge_h
#define _included_bridge_h

#include "worldobject/teleport.h"

#define BRIDGE_TRANSPORTPERIOD          0.1f
#define BRIDGE_TRANSPORTSPEED           50.0f


class Bridge : public Teleport
{
public:
    enum
    {
        BridgeTypeEnd,
        BridgeTypeTower,
        NumBridgeTypes
    };
    int     m_bridgeType;
    int     m_nextBridgeId;
    float   m_status;                           // Construction status, 0=not started, 100=finished, < 0.0f = shutdown

protected:
    Shape       *m_shapes[NumBridgeTypes];
    ShapeMarker *m_signal;
    
    bool    m_beingOperated;

public:
    Bridge();

    void Initialise     ( Building *_template );
    void SetBridgeType  ( int _type );

    void Render         ( float predictionTime );    
    void RenderAlphas   ( float predictionTime );
    bool Advance        ();

    bool GetAvailablePosition   ( Vector3 &_pos, Vector3 &_front );                     // Finds place for engineer
    void BeginOperation         ();
    void EndOperation           ();

    bool        ReadyToSend     ();
    Vector3     GetStartPoint   ();
    Vector3     GetEndPoint     ();
    bool        GetExit         ( Vector3 &_pos, Vector3 &_front );

    bool        UpdateEntityInTransit( Entity *_entity );

    int  GetBuildingLink();                 
    void SetBuildingLink( int _buildingId );

    void Read           ( TextReader *_in, bool _dynamic );
    void Write          ( FileWriter *_out );     
};


#endif
