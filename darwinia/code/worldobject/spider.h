#ifndef INCLUDED_SPIDER_H
#define INCLUDED_SPIDER_H


#include "lib/vector2.h"

#include "worldobject/entity.h"

class Unit;
class EntityLeg;


#define SPIDER_NUM_LEGS	8


class SpiderParameters
{
public:
	float 		m_legLift;
	float 		m_idealLegSlope;
	float 		m_legSwingDuration;
	float 		m_delayBetweenLifts;		// Time between successive foot lifts
	float		m_lookAheadCoef;			// Amount of velocity to add on to foot home pos to calc foot target pos
	float 		m_idealSpeed;				// Speed at which this set of parameters is appropriate
};



//*****************************************************************************
// Class Spider
//*****************************************************************************

class Spider: public Entity
{
public:
	enum
	{
		StateIdle,
        StateEggLaying,
        StateAttack,
        StatePouncing
	};

	int				m_state;

protected:
	SpiderParameters m_parameters[3];
    ShapeMarker     *m_eggLay;

	EntityLeg		*m_legs[SPIDER_NUM_LEGS];
	float			m_nextLegMoveTime;			// Actually just the next opportunity for a leg to move - there is no guarantee that a leg will move then
	float			m_delayBetweenLifts;

	float			m_speed;
	float			m_targetHoverHeight;
	Vector3			m_targetPos;
	Vector3			m_up;
    
	float			m_pounceStartTime;

	int		CalcWhichFootToMove();
	void	StompFoot(Vector3 const &_pos);
	void	UpdateLegsPouncing();
	void	UpdateLegs();
	float	IsPathOK(Vector3 const &_dest);		// Returns amount of path that can be followed
	void	DetectCollisions();

protected:          // AI stuff
    
    float           m_retargetTimer;
    Vector3         m_pounceTarget;
    int             m_spiritId;
    
	bool	SearchForRandomPos();
    bool    SearchForEnemies();
    bool    SearchForSpirits();

	bool	AdvanceIdle();
    bool    AdvanceEggLaying();
    bool    AdvanceAttack();
    bool    AdvancePouncing();

    bool    FaceTarget();
	bool	AdvanceToTarget();

public:
	Spider();
	~Spider();

	void    Begin               ();
	bool    Advance             (Unit *_unit);
    void	ChangeHealth        (int _amount);
	void    Render              (float _predictionTime);
	bool    RenderPixelEffect   (float _predictionTime);

    bool    IsInView            ();
    void    ListSoundEvents     (LList<char *> *_list);
};


#endif
