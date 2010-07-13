#ifndef _included_weapons_h
#define _included_weapons_h


#include "worldobject/entity.h"
#include "worldobject/worldobject.h"

class Shape;


// ****************************************************************************
//  Class ThrowableWeapon
// ****************************************************************************

class ThrowableWeapon : public WorldObject
{    
protected:
    Shape   *m_shape;
    double   m_birthTime;
    double   m_force;
    int     m_numFlashes;

    Vector3 m_front;
    Vector3 m_up;
            
    RGBAColour m_colour;
    
    void TriggerSoundEvent( char *_event );

public:
    ThrowableWeapon( int _type, Vector3 const &_startPos, Vector3 const &_front, double _force );

    void Initialise ();
    bool Advance    ();
    void Render     ( double _predictionTime );

    void RemoveForce();
    void ResetForce();

	static double GetMaxForce( int _researchLevel );
	static double GetApproxMaxRange( double _maxForce );
};


// ****************************************************************************
//  Class Grenade
// ****************************************************************************

class Grenade : public ThrowableWeapon
{
public:
    double   m_life;
    double   m_power;

    bool    m_explodeOnImpact;  // this grenade will not time out, but will explode as soon as it hits the ground
    bool    m_collected;        // the grenade has been sucked up by a harvester
    
public:
    Grenade( Vector3 const &_startPos, Vector3 const &_front, double _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************

class AirStrikeMarker : public ThrowableWeapon
{
public:
    WorldObjectId    m_airstrikeUnit;
    bool             m_naplamBombers;
    
public:
    AirStrikeMarker( Vector3 const &_startPos, Vector3 const &_front, double _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

class ControllerGrenade : public ThrowableWeapon
{
public:
    int m_squadTaskId;

public:
    ControllerGrenade( Vector3 const &_startPos, Vector3 const &_front, double _force );
    bool Advance    ();
};


// ****************************************************************************
//  Class Rocket
// ****************************************************************************

class Rocket : public WorldObject
{
public:
    unsigned char m_fromTeamId;
    
    Shape   *m_shape;
    double    m_timer;
    double    m_lifeTime;
    int     m_fromBuildingId;
	bool	m_firework;

public:
    Vector3 m_target;

    Rocket() {}
    Rocket(Vector3 _startPos, Vector3 _targetPos);

    void Initialise ();
    bool Advance    ();
    void Render     ( double predictionTime );

    void Explode();
};



// ****************************************************************************
//  Class Laser
// ****************************************************************************

class Laser : public WorldObject
{
public:
    unsigned char m_fromTeamId;
    bool     m_mindControl;

protected:
    double   m_life;
    bool    m_harmless;                 // becomes true after hitting someone
    bool    m_bounced;
    
public:   
    Laser() {}    

    virtual void Initialise(double _lifeTime);
    virtual bool Advance();
    virtual void Render( double predictionTime );
};


// ****************************************************************************
//  Class TurretShell
// ****************************************************************************

class TurretShell : public WorldObject
{
protected:
    double  m_age;
    double  m_life;
    bool    m_stopRendering;

public:
    int     m_fromBuildingId;

public:
    TurretShell(double _life);

    bool Advance();
    void Render( double predictionTime );

    void ExplodeShape( float fraction );
};


// ****************************************************************************
//  Class Shockwave
// ****************************************************************************

class Shockwave : public WorldObject
{
public:
    Shape   *m_shape;
    int     m_teamId;
    double   m_size;
    double   m_life;
    int     m_fromBuildingId;

public:
    Shockwave( int _teamId, double _size );
    
    bool Advance();
    void Render( double predictionTime );
};



// ****************************************************************************
//  Class MuzzleFlash
// ****************************************************************************

class MuzzleFlash : public WorldObject
{
public:
    Vector3 m_front;
    double   m_size;
    double   m_life;

public:
    MuzzleFlash();
    MuzzleFlash( Vector3 const &_pos, Vector3 const &_front, double _size, double _life );
    
    bool Advance();
    void Render( double _predictionTime );
};



// ****************************************************************************
//  Class Missile
// ****************************************************************************

class Missile : public WorldObject
{
protected:
    double           m_life;
    LList           <Vector3> m_history;
    Shape           *m_shape;
    ShapeMarker     *m_booster;
    MuzzleFlash     m_fire;
    
public:
    WorldObjectId   m_tankId;                   // Who fired me
    Vector3         m_front;
    Vector3         m_up;
    Vector3         m_target;
    
public:
    Missile();
    
    bool Advance();
    bool AdvanceToTargetPosition( Vector3 const &_pos );
    void Explode();
    void Render( double _predictionTime );
};

class Fireball : public WorldObject
{
public:
    double   m_life;
    double   m_radius;
    bool    m_createdParticles;

    int     m_fromBuildingId;

    LList<int>  m_damaged;  // maintains a list of damaged darwinians to make sure it doesnt cause damage more than once

public:
    Fireball();
    bool Advance();
    bool Damaged( int _darwinianId );
};

class PulseWave : public WorldObject
{
public:

    static PulseWave *s_pulseWave;

    double m_radius;
    double m_birthTime;

public:
    PulseWave();
    ~PulseWave();
    bool Advance();
    void Render( double _predictionTime );
};

class LandMine : public WorldObject
{
public:
    Shape   *m_shape;
    Vector3 m_front;
    Vector3 m_up;

public:
    LandMine();
    bool Advance();
    void Render( double _predictionTime );
};

class FlameGrenade : public Grenade
{
public:
    FlameGrenade( Vector3 const &_startPos, Vector3 const &_front, double _force );
    bool Advance    ();

};


class TurretEmptyShell : public WorldObject
{
public:
    Shape   *m_shape;
    Vector3 m_front;
    Vector3 m_up;
    double   m_force;

public:
    TurretEmptyShell( Vector3 const &pos, Vector3 const &front, Vector3 const &vel );

    bool Advance();
    void Render( double _predictionTime );
};

#endif
