
#ifndef _included_generator_h
#define _included_generator_h

#include "worldobject/building.h"


class FileWriter;


// ****************************************************************************
// Class PowerBuilding
// ****************************************************************************

class PowerBuilding : public Building
{
protected:
    int             m_powerLink;
    ShapeMarker     *m_powerLocation;

    LList           <float> m_surges;

public:
    PowerBuilding();

    void Initialise     ( Building *_template );
    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    bool IsInView       ();
    Vector3 GetPowerLocation();
    virtual void TriggerSurge ( float _initValue );

    void ListSoundEvents( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );

    int  GetBuildingLink();
    void SetBuildingLink( int _buildingId );
};



// ****************************************************************************
// Class Generator
// ****************************************************************************

class Generator : public PowerBuilding
{
protected:
    ShapeMarker *m_counter;

    float   m_timerSync;
    int     m_numThisSecond;
    bool    m_enabled;

public:
    float   m_throughput;

public:
    Generator();

    void TriggerSurge ( float _initValue );

    void ReprogramComplete();

    char *GetObjectiveCounter();

    void ListSoundEvents( LList<char *> *_list );

    bool Advance();
    void Render( float _predictionTime );
};



// ****************************************************************************
// Class Pylon
// ****************************************************************************

class Pylon : public PowerBuilding
{
public:
    Pylon();
    bool Advance();
};


// ****************************************************************************
// Class PylonStart
// ****************************************************************************

class PylonStart : public PowerBuilding
{
public:
    int m_reqBuildingId;

public:
    PylonStart();

    void Initialise     ( Building *_template );
    bool Advance        ();
    void RenderAlphas   ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );
    void Write  ( FileWriter *_out );
};


// ****************************************************************************
// Class PylonEnd
// ****************************************************************************

class PylonEnd : public PowerBuilding
{
public:
    PylonEnd();

    void TriggerSurge   ( float _initValue );
    void RenderAlphas   ( float _predictionTime );
};


// ****************************************************************************
// Class SolarPanel
// ****************************************************************************

#define SOLARPANEL_NUMGLOWS 4
#define SOLARPANEL_NUMSTATUSMARKERS 5

class SolarPanel : public PowerBuilding
{
protected:
    ShapeMarker *m_glowMarker   [SOLARPANEL_NUMGLOWS];
    ShapeMarker *m_statusMarkers[SOLARPANEL_NUMSTATUSMARKERS];

    bool m_operating;

public:
    SolarPanel();

    void Initialise     ( Building *_template );
    bool Advance        ();

    void Render         ( float _predictionTime );
    void RenderPorts    ();
    void RenderAlphas   ( float _predictionTime );

    void ListSoundEvents( LList<char *> *_list );
};

#endif
