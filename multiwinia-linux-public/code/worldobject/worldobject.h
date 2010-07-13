#ifndef WORLDOBJECT_H
#define WORLDOBJECT_H

#include "lib/rgb_colour.h"
#include "lib/vector3.h"


// ****************************************************************************
//  Class WorldObjectId
// ****************************************************************************

#define UNIT_BUILDINGS      -100
#define UNIT_SPIRITS        -101
#define UNIT_LASERS         -102
#define UNIT_EFFECTS        -103


class WorldObjectId
{
protected:
    unsigned char   m_teamId;
    int             m_unitId;
    int             m_index;
    int             m_uniqueId;

protected:
    static int      s_nextUniqueId;

public:
    WorldObjectId();
    WorldObjectId( unsigned char _teamId, int _unitId, int _index, int _uniqueId );
    void			Set( unsigned char _teamId, int _unitId, int _index, int _uniqueId );
    
    void            SetInvalid();       
    
	void			SetTeamId   (unsigned char _teamId) { m_teamId = _teamId; }
	void			SetUnitId   (int _unitId)			{ m_unitId = _unitId; }
	void			SetIndex    (int _index)			{ m_index = _index; }
	void			SetUniqueId (int _uniqueId)		    { m_uniqueId = _uniqueId; }
    void            GenerateUniqueId                    ();                                 

	unsigned char   GetTeamId   () const				{ return m_teamId; }
    int             GetUnitId   () const				{ return m_unitId; }
    int             GetIndex    () const				{ return m_index; }
    int             GetUniqueId () const				{ return m_uniqueId; }
    
	bool            IsValid() const					    { return( m_teamId != 255 ||
														          m_unitId != -1 ||
														          m_index != -1 ); }    

	bool operator != (WorldObjectId const &w) const;		
	bool operator == (WorldObjectId const &w) const;		
    WorldObjectId const &operator = (WorldObjectId const &w);

	static void		ResetUniqueIdSeed();
};


// ****************************************************************************
//  Class WorldObject
// ****************************************************************************

class WorldObject
{
public:
    enum                                            // These enums apply if our unitID is UNIT_EFFECTS
    {
        TypeInvalid,
        EffectThrowableGrenade,
        EffectThrowableAirstrikeMarker,
        EffectThrowableAirstrikeBomb,
        EffectThrowableControllerGrenade,
        EffectGunTurretTarget,                            
        EffectGunTurretShell,
        EffectSpamInfection,                              
        EffectBoxKite,
        EffectSnow,
        EffectRocket,
        EffectShockwave,
        EffectMuzzleFlash,
        EffectOfficerOrders,
        EffectOfficerOrderTrail,
        EffectZombie,
        EffectRandomiser,
        EffectDustBall,
        EffectLightning,
        EffectPulseWave,
        EffectFlameGrenade,
        EffectTurretEmptyShell,
		EffectFireball
    };  

public:
    WorldObjectId   m_id;
    int             m_type;
    Vector3         m_pos;
    Vector3         m_vel;
    bool            m_onGround;
    bool            m_enabled;

    WorldObject						();
    virtual ~WorldObject            ();
	void BounceOffLandscape			();

    virtual bool Advance			();
    virtual void Render				( double _time );
	virtual bool RenderPixelEffect	( double predictionTime );               // Return true if you did anything

    virtual char *LogState( char *_message = NULL );
};


// ****************************************************************************
//  Class Light
// ****************************************************************************

class Light
{
public:
    double m_colour[4];	// Forth element seems irrelevant but OpenGL insists we specify it
    double m_front[4];	// Forth element must be 0.0 to signify an infinitely distance light

    Light();
	void SetColour(double colour[4]);
	void SetFront(double front[4]);
	void SetFront(Vector3 front);
	void Normalise();
};


#endif
