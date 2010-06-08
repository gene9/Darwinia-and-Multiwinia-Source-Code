#ifndef _included_rocket_h
#define _included_rocket_h

#include "worldobject/building.h"



class FuelBuilding : public Building
{
protected:
    ShapeMarker     *m_fuelMarker;
    static Shape    *s_fuelPipe;
    
public:
    int             m_fuelLink;
    float           m_currentLevel;
    
public:
    FuelBuilding();

    void Initialise( Building *_template );

    virtual void ProvideFuel( float _level );

    Vector3 GetFuelPosition();

    FuelBuilding *GetLinkedBuilding();

    bool Advance();

    bool IsInView       ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( FileWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );

	void Destroy( float _intensity );
};


// ============================================================================


class FuelGenerator : public FuelBuilding
{
protected:
    Shape           *m_pump;
    ShapeMarker     *m_pumpTip;
    float           m_pumpMovement;
    float           m_previousPumpPos;
    
    Vector3         GetPumpPos();
    
public:
    float   m_surges;

public:
    FuelGenerator();
    
    void ProvideSurge   ();

    bool Advance        ();
    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    void ListSoundEvents( LList<char *> *_list );

    char *GetObjectiveCounter();
};


// ============================================================================


class FuelPipe : public FuelBuilding
{    
public:
    FuelPipe();

    bool Advance();

    void ListSoundEvents( LList<char *> *_list );
};


// ============================================================================


class FuelStation : public FuelBuilding
{
protected:
    ShapeMarker *m_entrance;

public:
    FuelStation();

    void Render( float _predictionTime );
    void RenderAlphas( float _predictionTime );

    Vector3 GetEntrance();

    bool Advance();
    bool IsLoading();
    bool BoardRocket( WorldObjectId id );

    void ListSoundEvents( LList<char *> *_list );

    bool PerformDepthSort( Vector3 &_centrePos ); 
};


// ============================================================================


class EscapeRocket : public FuelBuilding
{
protected:
    ShapeMarker     *m_booster;   
    ShapeMarker     *m_window[3];
    Shape           *m_rocketLowRes;
    float           m_shadowTimer;
    float           m_cameraShake;

    Vector3         m_vel;

public:
    enum
    {
        StateRefueling,
        StateLoading,
        StateIgnition,
        StateReady,
        StateCountdown,
        StateExploding,
        StateFlight,
        NumStates
    };
    
    int             m_state;
    float           m_fuel;
    int             m_pipeCount;
    int             m_passengers;
    float           m_countdown;
    float           m_damage;
    int             m_spawnBuildingId;
    bool            m_spawnCompleted;
    
protected:
    void Refuel();    
    void SetupSpectacle();
    void SetupAttackers();

    void AdvanceRefueling();
    void AdvanceLoading();
    void AdvanceIgnition();
    void AdvanceReady();
    void AdvanceCountdown();
    void AdvanceFlight();
    void AdvanceExploding();

    void SetupSounds();

public:
    EscapeRocket();

    void Initialise     ( Building *_template );
    bool Advance        ();
    
    void ProvideFuel    ( float _level );
    bool SafeToLaunch   ();
    bool BoardRocket    ( WorldObjectId _id );   
    void Damage         ( float _damage );

    bool IsSpectacle    ();
    bool IsInView       ();

    void Render         ( float _predictionTime );
    void RenderAlphas   ( float _predictionTime );

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( FileWriter *_out );	
        
    char *GetObjectiveCounter();
    
    void ListSoundEvents( LList<char *> *_list );
    
    static int GetStateId( char *_state );
};

#endif