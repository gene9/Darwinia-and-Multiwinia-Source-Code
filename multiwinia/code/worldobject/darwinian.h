
#ifndef _included_darwinian_h
#define _included_darwinian_h

#include "worldobject/entity.h"


#define DARWINIAN_SEARCHRANGE_OFFICERS          100.0f
#define DARWINIAN_SEARCHRANGE_OFFICERFOLLOW     150.0f
#define DARWINIAN_SEARCHRANGE_ARMOUR            150.0f
#define DARWINIAN_SEARCHRANGE_SPIRITS           50.0f
#define DARWINIAN_SEARCHRANGE_THREATS           90.0f
#define DARWINIAN_SEARCHRANGE_GRENADES          60.0f 
#define DARWINIAN_SEARCHRANGE_TURRETS           50.0f
#define DARWINIAN_SEARCHRANGE_PORTS             100.0f 
#define DARWINIAN_SEARCHRANGE_CARRYABLE         150.0f
#define DARWINIAN_SEARCHRANGE_CRATES            150.0f

#define DARWINIAN_UNDERCONTROL_MAXPUSHRANGE     100.0f
#define DARWINIAN_FEARRANGE                     200.0f

#define DARWINIAN_INFECTIONRANGE                30.0f

class Officer;



class Darwinian : public Entity
{
public:
    enum
    {
        StateIdle,
        StateApproachingPort,
        StateOperatingPort,
        StateApproachingArmour,
        StateInsideArmour,
        StateWorshipSpirit,
        StateUnderControl,
        StateFollowingOrders,
        StateFollowingOfficer,
        StateCombat,
        StateCapturedByAnt,
        StateBoardingRocket,
        StateWatchingSpectacle,
        StateAttackingBuilding,                        // Eg attacking rocket in demo2
        StateOnFire,
        StateInWater,
        StateApproachingCarriableBuilding,
        StateCarryingBuilding,
        StateWatchingGodDish,
		StateFollowingShaman,
        StateBeingSacraficed,
        StateOpeningCrate,
        StateInVortex,
        StateBeingAbducted,
        StateRaisingFlag,
        //StateUsingJumpPad
    };

    enum
    {
        InfectionStateClean,
        InfectionStateInfected,
        InfectionStateImmune
    };

    int         m_state;
    bool        m_promoted;
    Vector3     m_wayPoint;

public:
    double          m_retargetTimer;
    WorldObjectId   m_threatId;    
    WorldObjectId   m_armourId;
    WorldObjectId   m_officerId;
	WorldObjectId   m_shamanId;
    int             m_spiritId;
    int             m_buildingId;
    int             m_portId;
    WorldObjectId   m_boxKiteId;
    WorldObjectId   m_spaceshipId;

    double      m_threatRange;
    bool        m_scared;

    int         m_controllerId;                         // Used only when Under
    bool        m_teleportRequired;                     //
    bool        m_underPlayerControl;
    bool        m_followingPlayerOrders;                // Our current order was given directly and should be obeyed to the exclusion of all else
    bool        m_manuallyApproachingArmour;
    double      m_idleTimer;                            

    Vector3     m_orders;                               // Used when receiving orders
    int         m_ordersBuildingId;                     // from an officer
    bool        m_ordersSet;                            //
              
    double      m_grenadeTimer;
    double      m_officerTimer;

    int         m_shadowBuildingId;         // This building causes us to cast a shadow
    Vector3     m_avoidObstruction;         // Used to nagivate around big obstructions, eg water

    double      m_sacraficeTimer;           // How long the sacrafice 'animation' has been running
    int         m_targetPortal;
    bool        m_burning;

    int         m_boosterTaskId;
    int         m_hotFeetTaskId;
    int         m_subversionTaskId;
    int         m_rageTaskId;

    int         m_infectionState;                 // This Darwinian is infect with the plague
    double      m_infectionTimer;

    double      m_paralyzeTimer;
    bool        m_insane;

    int         m_previousTeamId;       // used when a darwinian has been hit by a subversion shot to track the colour flashing
    double      m_colourTimer;

    double      m_fireDamage;

    bool        m_futurewinian;
	bool		m_startedThreatSound;

    int         m_wallHits;
    int         m_closestAITarget;
	
	float		m_scaleFactor;		// DEF: Optimization.

protected:
    bool        SearchForNewTask();

    bool        SearchForRandomPosition();
    bool        SearchForThreats();
    bool        SearchForSpirits();
    bool        SearchForPorts();
    bool        SearchForOfficers();
    bool        SearchForArmour();    
    bool        SearchForCarryableBuilding();
	bool		SearchForShamans();
    bool        SearchForCrates();
    bool        SearchForFlags();       // blitzkrieg flags

    bool        BeginVictoryDance();

    bool        AdvanceIdle             ();
    bool        AdvanceApproachingPort  ();
    bool        AdvanceOperatingPort    ();
    bool        AdvanceApproachingArmour();
    bool        AdvanceInsideArmour     ();
    bool        AdvanceWorshipSpirit    ();
    bool        AdvanceUnderControl     ();
    bool        AdvanceFollowingOrders  ();
    bool        AdvanceFollowingOfficer ();
    bool        AdvanceCombat           ();
    bool        AdvanceCapturedByAnt    ();
    bool        AdvanceWatchingSpectacle();
    bool        AdvanceBoardingRocket   ();
    bool        AdvanceAttackingBuilding();
    bool        AdvanceOnFire           ();  
    bool        AdvanceInWater          ();
    bool        AdvanceWatchingGodDish  ();
    bool        AdvanceApproachingCarriableBuilding();
    bool        AdvanceCarryingBuilding ();
    bool        AdvanceToTargetPosition	();     
	bool		AdvanceFollowingShaman	();
    bool        AdvanceSacraficing      ();
    bool        AdvanceInfection        ();
    bool        AdvanceOpeningCrate     ();
    bool        AdvanceInVortex         ();
    bool        AdvanceBeingAbducted    ();
    bool        AdvanceRaisingFlag      ();
    //bool        AdvanceJumpPadLaunch    ();

    void        AdvanceHotFeet();
    
    void        OrbitPortal();

    int             FindNearestUsefulBuilding ( double &distance );
    WorldObjectId   FindNearestUsefulOfficer  ( double &distance );

    bool HasRage();

public:
    Darwinian();

    void Begin                  ();
    bool Advance                ( Unit *_unit );
    bool ChangeHealth           ( int _amount, int _damageType = DamageTypeUnresistable );
    bool IsInView               ();
    void Render                 ( double _predictionTime, double _highDetail );   

    void FireDamage             ( int _amount, int _teamId );

    Vector3 PushFromObstructions( Vector3 const &pos, bool killem = true ); 

    void GiveOrders             ( Vector3 const &_targetPos, int routeId=-1 );
    void TakeControl            ( int _controllerId );
    void LoseControl            ();
    void AntCapture             ( WorldObjectId _antId );
    void WatchSpectacle         ( int _buildingId );
    void CastShadow             ( int _buildingId );
    void BoardRocket            ( int _buildingId );
    void AttackBuilding         ( int _buildingId );
    void SetFire                ();
    bool IsOnFire               ();
    void Abduct                 ( WorldObjectId _shipId );
    void FollowRoute            ();

    bool IsInvincible           ( bool _againstExplosions = false); // returns true if the darwinian has booster, is being sacraficed, inside armour etc

    void Sacrafice              ( WorldObjectId _shamanId, int _targetPortalId );
    void SetBooster             ( int _taskId );
    void Infect                 ();
    void GiveMindControl        ( int _shots );
    void GiveHotFeet            ( int _taskId );
    void GiveRage               ( int _taskId );

    void FireSubversion         ( Vector3 const &_pos ); 
    
    Officer *IsInFormation      ();

    bool IsSelectable           ();

    char *LogState              ( char *_message = NULL );

    void ListSoundEvents        ( LList<char *> *_list );
};



class BoxKite : public WorldObject
{
public:
    Shape       *m_shape;
    Vector3     m_front;
    Vector3     m_up;
    
    enum
    {
        StateHeld,
        StateReleased
    };
    int         m_state;

    double      m_birthTime;
    double      m_deathTime;
    double      m_brightness;
    double      m_size;
    
public:
    BoxKite();
    bool Advance    ();
    void Release    ();
    void Render     ( double _predictionTime );

    virtual char *LogState( char *_message = NULL );
};

#endif
