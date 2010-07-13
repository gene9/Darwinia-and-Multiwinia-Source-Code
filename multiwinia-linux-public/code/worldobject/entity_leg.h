 #ifndef INCLUDED_ENTITY_LEG_H
#define INCLUDED_ENTITY_LEG_H


#include "lib/vector3.h"

class Entity;
class ShapeMarker;
class Shape;


//*****************************************************************************
// Class EntityFoot
//*****************************************************************************

class EntityFoot
{
public:
	enum FootState
	{
		OnGround,
		Swinging,
		Pouncing
	};

	Vector3			m_pos;
	Vector3			m_targetPos;
	FootState		m_state;
	double			m_leftGroundTimeStamp;
	Vector3			m_lastGroundPos;
	Vector3			m_bodyToFoot;			// Used whilst pouncing
};


//*****************************************************************************
// Class EntityLeg
//*****************************************************************************

class EntityLeg
{
public:
	double			m_legLift;				// How high to lift the foot (in metres)
	double			m_idealLegSlope;		// 0.0 means vertical 1.0 means 45 degrees.
	double			m_legSwingDuration;
	double			m_lookAheadCoef;		// Amount of velocity to add on to foot home pos to calc foot target pos

	int				m_legNum;
	EntityFoot		m_foot;
	ShapeMarker		*m_rootMarker;
	Shape			*m_shapeUpper;
    Shape           *m_shapeLower;
	double			m_thighLen;
	double			m_shinLen;
	Entity			*m_parent;

protected:
	Vector3 GetLegRootPos();
	Vector3 CalcFootHomePos(double _targetHoverHeight);
	Vector3 CalcDesiredFootPos(double _targetHoverHeight);
	Vector3 CalcKneePos(Vector3 const &_footPos, Vector3 const &_rootPos, Vector3 const &_centrePos);
	Vector3	GetIdealSwingingFootPos(double _fractionComplete);

public:
	EntityLeg(int _legNum, Entity *_parent, 
                char const *_shapeNameUpper,
                char const *_shapeNameLower,
                char const *_rootMarkerName);
	
	double	CalcFootsDesireToMove(double _targetHoverHeight);
	void	LiftFoot(double _targetHoverHeight);
	void	PlantFoot();

	bool	Advance(); // Returns true if the foot was planted this frame
	void	AdvanceSpiderPounce(double _fractionComplete);
	void	Render(double _predictionTime, Vector3 const &_predictedMovement);
	bool	RenderPixelEffect(double _predictionTime, Vector3 const &_predictedMovement);
};


#endif
