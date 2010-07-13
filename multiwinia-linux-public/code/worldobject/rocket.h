#ifndef _included_rocket_h
#define _included_rocket_h

#include "worldobject/building.h"
//#include "lib/environment.h"

class AITarget;

class FuelBuilding : public Building
{
protected:
    ShapeMarker     *m_fuelMarker;
    static Shape    *s_fuelPipe;
    
public:
    int             m_fuelLink;
    double           m_currentLevel;
    
public:
    FuelBuilding();

    void Initialise( Building *_template );

    virtual void ProvideFuel( double _level );

    Vector3 GetFuelPosition();

    FuelBuilding *GetLinkedBuilding();

    bool Advance();

    bool IsInView       ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void Read   ( TextReader *_in, bool _dynamic );     
    void Write  ( TextWriter *_out );							
    
    int  GetBuildingLink();                             
    void SetBuildingLink( int _buildingId );

	void Destroy( double _intensity );
};


// ============================================================================


class FuelGenerator : public FuelBuilding
{
protected:
    Shape           *m_pump;
    ShapeMarker     *m_pumpTip;
    double           m_pumpMovement;
    double           m_previousPumpPos;
    
    Vector3         GetPumpPos();
    
public:
    double   m_surges;

public:
    FuelGenerator();
    
    void ProvideSurge   ();

    bool Advance        ();
    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void ListSoundEvents( LList<char *> *_list );

    void GetObjectiveCounter( UnicodeString& _dest );
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
    AITarget    *m_aiTarget;

public:
    FuelStation();

    void Render( double _predictionTime );
    void RenderAlphas( double _predictionTime );

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
    double           m_shadowTimer;
    double           m_cameraShake;

    Vector3         m_vel;
    bool            m_coloured;
    double           m_attackTimer;
	//Environment     m_environment; // reflected environment

    bool            m_attackWarning;    // set to true when the under attack warning has been issued, false when damage = 0
    bool            m_refueledWarning;
    bool            m_countdownWarning;

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
    double           m_fuel;
    int             m_pipeCount;
    int             m_passengers;
    double           m_countdown;
    double           m_damage;
    float           m_fuelBeforeExplosion;
    int             m_spawnBuildingId;
    bool            m_spawnCompleted;
    double           m_refuelRate;
    
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
    ~EscapeRocket();

    void Initialise     ( Building *_template );
    bool Advance        ();   

    void ProvideFuel    ( double _level );
    bool SafeToLaunch   ();
    bool BoardRocket    ( WorldObjectId _id );   
    void Damage         ( double _damage );

    bool IsSpectacle    ();
    bool IsInView       ();

    void Render         ( double _predictionTime );
    void RenderAlphas   ( double _predictionTime );

    void Read           ( TextReader *_in, bool _dynamic );     
    void Write          ( TextWriter *_out );	
        
    void GetObjectiveCounter( UnicodeString& _dest );
    
    bool DoesSphereHit          (Vector3 const &_pos, double _radius);
    bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                double _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    void ListSoundEvents( LList<char *> *_list );
    
    static int GetStateId( char *_state );
};

void ConvertFragmentColours ( ShapeFragment *_frag, RGBAColour _col );

#endif