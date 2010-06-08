
#ifndef _included_generator_h
#define _included_generator_h

#include "worldobject/building.h"


class TextWriter;


struct PowerSurge
{
    int     m_teamId;
    double   m_percent;
};


// ****************************************************************************
// Class PowerBuilding
// ****************************************************************************

class PowerBuilding : public Building
{
protected:
    int             m_powerLink;
    ShapeMarker     *m_powerLocation;

    LList           <PowerSurge *> m_surges;

public:
    PowerBuilding();
	~PowerBuilding();
    
    void Initialise     ( Building *_template );    
    bool Advance        ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    bool IsInView       ();
    Vector3 GetPowerLocation();
    
    virtual void TriggerSurge ( double _initValue, int teamId=255 );

    void ListSoundEvents( LList<char *> *_list );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
    
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

    double   m_timerSync;
    int     m_numThisSecond;
    bool    m_enabled;

public:
    double   m_throughput;

public:
    Generator();

    void TriggerSurge ( double _initValue, int teamId=255 );

    void ReprogramComplete();

    void GetObjectiveCounter( UnicodeString& _dest );

    void ListSoundEvents( LList<char *> *_list );

    bool Advance();
    void Render( double _predictionTime );
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
    void RenderAlphas   ( double _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
};


// ****************************************************************************
// Class PylonEnd
// ****************************************************************************

class PylonEnd : public PowerBuilding
{    
public:
    PylonEnd();
        
    void TriggerSurge   ( double _initValue, int teamId=255 );
    void RenderAlphas   ( double _predictionTime );
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
    int  m_startingTeam;

public:
    SolarPanel();

    void Initialise     ( Building *_template );
    bool Advance        ();           

    void RecalculateOwnership();
    
    void Render         ( double _predictionTime );
    void RenderPorts    ();
    void RenderAlphas   ( double _predictionTime );

    void ListSoundEvents( LList<char *> *_list );

	bool IsOperating	();
};



// ****************************************************************************
// Class 
// ****************************************************************************

class PowerSplitter : public PowerBuilding
{    
public:
    LList<int> m_links;

public:
    PowerSplitter();

    void Initialise     ( Building *_template );

    void TriggerSurge   ( double _initValue, int teamId=255 );
    
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void SetBuildingLink( int _buildingId );            

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							

};



#endif
