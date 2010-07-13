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
	double 		m_legLift;
	double 		m_idealLegSlope;
	double 		m_legSwingDuration;
	double 		m_delayBetweenLifts;		// Time between successive foot lifts
	double		m_lookAheadCoef;			// Amount of velocity to add on to foot home pos to calc foot target pos
	double 		m_idealSpeed;				// Speed at which this set of parameters is appropriate
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
	double			m_nextLegMoveTime;			// Actually just the next opportunity for a leg to move - there is no guarantee that a leg will move then
	double			m_delayBetweenLifts;

	double			m_speed;
	double			m_targetHoverHeight;
	Vector3			m_targetPos;
	Vector3			m_up;
    
	double			m_pounceStartTime;

	int		CalcWhichFootToMove();
	void	StompFoot(Vector3 const &_pos);
	void	UpdateLegsPouncing();
	void	UpdateLegs();
	double	IsPathOK(Vector3 const &_dest);		// Returns amount of path that can be followed
	void	DetectCollisions();

protected:          // AI stuff
    
    double           m_retargetTimer;
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
    bool	ChangeHealth        (int _amount, int _damageType);
	void    Render              (double _predictionTime);
	bool    RenderPixelEffect   (double _predictionTime);

    bool    IsInView            ();
    void    ListSoundEvents     (LList<char *> *_list);
};


#endif
