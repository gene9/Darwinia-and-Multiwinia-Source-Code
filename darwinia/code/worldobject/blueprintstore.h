#ifndef _included_blueprintstore_h
#define _included_blueprintstore_h

#include "worldobject/building.h"



class BlueprintBuilding : public Building
{
public:
    int     m_buildingLink;
    float   m_infected;
    int     m_segment;

protected:
    ShapeMarker *m_marker;
    Vector3      m_vel;
    
public:
    BlueprintBuilding();

    void Initialise( Building *_template );
    bool Advance();
    bool IsInView();
    void Render( float _predictionTime );
    void RenderAlphas( float _predictionTime );

    virtual void SendBlueprint( int _segment, bool _infected );

    Matrix34 GetMarker( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );            
};


// ============================================================================

#define BLUEPRINTSTORE_NUMSEGMENTS  4

class BlueprintStore : public BlueprintBuilding
{
public:
    float m_segments[BLUEPRINTSTORE_NUMSEGMENTS];
  
public:
    BlueprintStore();

    void GetDisplay     ( Vector3 &_pos, Vector3 &_right, Vector3 &_up, float &_size );

    void Initialise     ( Building *_template );
    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );
    void SendBlueprint  ( int _segment, bool _infected );

    char *GetObjectiveCounter();

    int GetNumInfected();                           // Returns number of segments totally infected ie == 100.0f
    int GetNumClean();                              // Returns number of segments totally clean ie == 0.0f
};


// ============================================================================


class BlueprintConsole : public BlueprintBuilding
{
public:
    BlueprintConsole();
    
    void            Initialise( Building *_template );
    
    void            RecalculateOwnership();
    bool            Advance();
    void            Render( float _predictionTime );
    void            RenderPorts();
        
    void            Read   ( TextReader *_in, bool _dynamic );     
    void            Write  ( FileWriter *_out );							
};


// ============================================================================


class BlueprintRelay : public BlueprintBuilding
{
public:
    float   m_altitude;

public:
    BlueprintRelay();
    
    void Initialise( Building *_template );
    void SetDetail( int _detail );

    bool Advance();
    
    void Render         ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
};

#endif