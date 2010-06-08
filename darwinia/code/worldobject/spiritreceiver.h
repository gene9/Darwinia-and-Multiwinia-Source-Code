
#ifndef _included_spiritreceiver_h
#define _included_spiritreceiver_h

#include "worldobject/building.h"

class SpiritProcessor;
class UnprocessedSpirit;


// ****************************************************************************
// Class ReceiverBuilding
// ****************************************************************************

class ReceiverBuilding : public Building
{
protected:
    int             m_spiritLink;
    ShapeMarker     *m_spiritLocation;

    LList           <float> m_spirits;

public:
    ReceiverBuilding();
    
    void Initialise     ( Building *_template );    
    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    bool            IsInView            ();
    virtual Vector3 GetSpiritLocation   ();
    virtual void    TriggerSpirit       ( float _initValue );

    void ListSoundEvents( LList<char *> *_list );

    static SpiritProcessor *GetSpiritProcessor();
    
    static void BeginRenderUnprocessedSpirits();
    static void RenderUnprocessedSpirit( Vector3 const &_pos, float _life=1.0f ); // gl friendly
	static void RenderUnprocessedSpirit_basic( Vector3 const &_pos, float _life=1.0f ); // dx friendly
	static void RenderUnprocessedSpirit_detail( Vector3 const &_pos, float _life=1.0f ); // dx friendly
    static void EndRenderUnprocessedSpirits();
    
    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            
};



// ****************************************************************************
// Class SpiritProcessor
// ****************************************************************************

class SpiritProcessor : public ReceiverBuilding
{
protected:
    float   m_timerSync;
    int     m_numThisSecond;
    float   m_spawnSync;
    float   m_throughput;

public:
    LList   <UnprocessedSpirit *> m_floatingSpirits;

public:
    SpiritProcessor();

    void TriggerSpirit ( float _initValue );

    char *GetObjectiveCounter();

    void Initialise( Building *_building );
    bool Advance();
    bool IsInView();
    void Render( float _predictionTime );
    void RenderAlphas( float _predictionTime );
};



// ****************************************************************************
// Class ReceiverLink
// ****************************************************************************

class ReceiverLink : public ReceiverBuilding
{
public:
    ReceiverLink();
    bool Advance();
};


// ****************************************************************************
// Class ReceiverSpiritSpawner
// ****************************************************************************

class ReceiverSpiritSpawner : public ReceiverBuilding
{
public:
    ReceiverSpiritSpawner();
    bool Advance();
};


// ****************************************************************************
// Class SpiritReceiver
// ****************************************************************************

#define SPIRITRECEIVER_NUMSTATUSMARKERS 5

class SpiritReceiver : public ReceiverBuilding
{    
protected:
    ShapeMarker *m_headMarker;
    Shape       *m_headShape;
    ShapeMarker *m_spiritLink;
    ShapeMarker *m_statusMarkers[SPIRITRECEIVER_NUMSTATUSMARKERS];

public:
    SpiritReceiver();

    Vector3 GetSpiritLocation();

    void Initialise     ( Building *_template );
    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderPorts    ();
    void RenderAlphas   ( float _predictionTime );

};


// ****************************************************************************
// Class UnprocessedSpirit
// ****************************************************************************

class UnprocessedSpirit : public WorldObject
{
protected:
    float       m_timeSync;
    float       m_positionOffset;                       // Used to make them float around a bit
    float       m_xaxisRate;
    float       m_yaxisRate;
    float       m_zaxisRate;

public:
    Vector3     m_hover;
    enum
    {
        StateUnprocessedFalling,
        StateUnprocessedFloating,
        StateUnprocessedDeath
    };
    int         m_state;
    
public:
    UnprocessedSpirit();

    bool Advance();
    float GetLife();                        // Returns 0.0f-1.0f (0.0f=dead, 1.0f=alive)
};

#endif
