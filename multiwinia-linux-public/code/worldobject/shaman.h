
#ifndef _included_shaman_h
#define _included_shaman_h

#include "worldobject/entity.h"

struct ShamanTargetArea
{
    Vector3 m_pos;
    double   m_radius;
};

class Shaman : public Entity
{
protected:
	Vector3 m_wayPoint;
	int		m_teleportId;

	bool	m_sacraficeDarwinians;
	double	m_sacraficeTimer;
	double	m_toggleTimer;

    int     m_summonType;

    int     m_mode;
    double   m_lastModeChange;

public:

    enum
    {
        ModeNone,
        ModeCreateDarwinians,
        ModeFollow,
        //ModeSacrafice, 
        //ModeTeleport,
        NumModes
    };

	//LList<int>	m_eggQueue;

    int     m_sacrafices;
    int     m_targetPortal;

    bool    m_vunerable;
    bool    m_paralyzed;        // the final summon is being cast on him, so he can no longer act

    bool    m_renderTeleportTargets;

public:
	Shaman();
	~Shaman();

	void Begin();

	bool Advance					( Unit *_unit );
	bool AdvanceToTargetPosition	();
    void AdvanceCreateDarwinians    ();

	void Render						( double predictionTime );
    void RenderTeleportTargets      ();

    bool ChangeHealth               ( int _amount, int _damageType = DamageTypeUnresistable );

	void SetWaypoint				( Vector3 const &_wayPoint );

	void BeginSummoning				( int _type );
	void SummonEntity				( int _type );

	void SacraficeDarwinians		();
	bool IsSacraficing				();
    void GrabSouls                  (); // Grab any nearby free souls

    bool CallingDarwinians          ();

	int GetSummonType				();
    int GetNearestPortal            ();
    void Paralyze                   ();

    void ChangeMode                 ( Vector3 _mousePos );
    void TeleportToPos              ( Vector3 _mousePos );
    bool ValidTeleportPos           ( Vector3 _pos );
    LList<ShamanTargetArea> *GetTargetArea();

    bool IsSelectable               ();
	
	bool		    CanSummonEntity( int _entityType );
    bool            CanCapturePortal();

    static bool     IsSummonable ( int _entityType );
	static double	GetSummonTime( int _entityType );
	static int		GetDefaultSpawnNumber( int _entityType );
	static int		GetSoulRequirement( int _entityType );
	static int		GetEggType( int _entityType );
	static int      GetTypeFromEgg( int _eggSpawnType );
};

#endif