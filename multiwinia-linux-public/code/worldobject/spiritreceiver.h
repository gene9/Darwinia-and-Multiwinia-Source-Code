
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

    LList           <double> m_spirits;

public:
    ReceiverBuilding();
    
    void Initialise     ( Building *_template );    
    bool Advance        ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    bool            IsInView            ();
    virtual Vector3 GetSpiritLocation   ();
    virtual void    TriggerSpirit       ( double _initValue );

    void ListSoundEvents( LList<char *> *_list );

    static SpiritProcessor *GetSpiritProcessor();
    
    static void BeginRenderUnprocessedSpirits();
    static void RenderUnprocessedSpirit( Vector3 const &_pos, double _life=1.0 ); // gl friendly
	static void RenderUnprocessedSpirit_basic( Vector3 const &_pos, double _life=1.0 ); // dx friendly
	static void RenderUnprocessedSpirit_detail( Vector3 const &_pos, double _life=1.0 ); // dx friendly
    static void EndRenderUnprocessedSpirits();
    
    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            
};



// ****************************************************************************
// Class SpiritProcessor
// ****************************************************************************

class SpiritProcessor : public ReceiverBuilding
{
protected:
    double   m_timerSync;
    int     m_numThisSecond;
    double   m_spawnSync;
    double   m_throughput;

public:
    LList   <UnprocessedSpirit *> m_floatingSpirits;

public:
    SpiritProcessor();

    void TriggerSpirit ( double _initValue );

    void GetObjectiveCounter( UnicodeString & _dest);

    void Initialise( Building *_building );
    bool Advance();
    bool IsInView();
    void Render( double _predictionTime );
    void RenderAlphas( double _predictionTime );
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
    void Render         ( double _predictionTime );
    void RenderPorts    ();
    void RenderAlphas   ( double _predictionTime );

};


// ****************************************************************************
// Class UnprocessedSpirit
// ****************************************************************************

class UnprocessedSpirit : public WorldObject
{
protected:
    double       m_timeSync;
    double       m_positionOffset;                       // Used to make them double around a bit
    double       m_xaxisRate;
    double       m_yaxisRate;
    double       m_zaxisRate;

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
    double GetLife();                        // Returns 0.0-1.0 (0.0=dead, 1.0=alive)

    void Render( double _predictionTime );
};

#endif
