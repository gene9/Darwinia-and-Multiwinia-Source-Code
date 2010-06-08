#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/persisting_debug_render.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/debug_render.h"

#include "worldobject/entity.h"
#include "worldobject/entity_leg.h"

#include "app.h"
#include "location.h"
#include "main.h"
#include "renderer.h"



EntityLeg::EntityLeg(int _legNum, Entity *_parent, 
                     char const *_shapeNameUpper, 
                     char const *_shapeNameLower, 
                     char const *_rootMarkerName)
:	m_legNum(_legNum),
	m_parent(_parent)
{
	m_shapeUpper = g_app->m_resource->GetShape(_shapeNameUpper);    
    m_shapeLower = g_app->m_resource->GetShape(_shapeNameLower);
	DarwiniaReleaseAssert(m_shapeUpper, "EntityLeg: Couldn't load leg shape %s", _shapeNameUpper);
    DarwiniaReleaseAssert(m_shapeLower, "EntityLeg: Couldn't load leg shape %s", _shapeNameLower);

	ShapeMarker *endMarker = m_shapeUpper->m_rootFragment->LookupMarker("MarkerEnd");
	Matrix34 const &endMatrix = endMarker->GetWorldMatrix(Matrix34(0));
	m_thighLen = endMatrix.pos.Mag();
	
    endMarker = m_shapeLower->m_rootFragment->LookupMarker("MarkerEnd");
    Matrix34 const &endMatrixLower = endMarker->GetWorldMatrix(Matrix34(0));
    m_shinLen = endMatrixLower.pos.Mag();

	m_rootMarker = m_parent->m_shape->m_rootFragment->LookupMarker(_rootMarkerName);
	DarwiniaReleaseAssert(m_rootMarker, "EntityLeg: Couldn't find root marker %s", _rootMarkerName);

	m_foot.m_state = EntityFoot::OnGround;
}


Vector3 EntityLeg::GetLegRootPos()
{
	Matrix34 rootMat(m_parent->m_front, g_upVector, m_parent->m_pos);
	Matrix34 const &resultMat = m_rootMarker->GetWorldMatrix(rootMat);
	
	return resultMat.pos;
}


Vector3 EntityLeg::CalcFootHomePos(float _targetHoverHeight)
{
	Vector3 rootWorldPos = GetLegRootPos();
	
	Vector3 fromCentreToRoot = rootWorldPos - m_parent->m_pos;
	fromCentreToRoot.HorizontalAndNormalise();

	Vector3 groundNormal = g_app->m_location->m_landscape.m_normalMap->GetValue(
								m_parent->m_pos.x, m_parent->m_pos.z);
	fromCentreToRoot *= groundNormal.y;

	Vector3 returnVal = rootWorldPos;
	returnVal += fromCentreToRoot * (m_idealLegSlope * _targetHoverHeight);
	returnVal.y -= _targetHoverHeight;

	return returnVal;
}


float EntityLeg::CalcFootsDesireToMove(float _targetHoverHeight)
{
	Vector3 homePos = CalcFootHomePos(_targetHoverHeight);
	Vector3 delta = m_foot.m_pos - homePos;
	Vector3 deltaHoriNorm = delta; deltaHoriNorm.HorizontalAndNormalise();
	
	float scoreDueToDirection = 1.0f;
//	if (m_parent->m_vel.Mag() > 0.1f)
//	{
//		Vector3 velHoriNorm = m_parent->m_vel; 
//		velHoriNorm.HorizontalAndNormalise();
//		scoreDueToDirection = -(deltaHoriNorm * velHoriNorm);
//	}
	float scoreDueToDist = delta.Mag();
	float score = scoreDueToDirection * scoreDueToDist;

	return score;
}


Vector3 EntityLeg::CalcDesiredFootPos(float _targetHoverHeight)
{
	Vector3 rv = CalcFootHomePos(_targetHoverHeight);
	
	float expectedRotation = 1.2f * m_parent->m_angVel.y;
	Vector3 averageExpectedVel = m_parent->m_vel;
	averageExpectedVel.RotateAroundY(expectedRotation * 0.5f);
	rv += averageExpectedVel * m_lookAheadCoef;

	return rv;
}


Vector3 EntityLeg::CalcKneePos(Vector3 const &_footPos, Vector3 const &_rootPos, Vector3 const &_centrePos)
{
	Vector3 rootToFoot(_footPos - _rootPos);
	float rootToFootLen = rootToFoot.Mag();

	Vector3 rootToFootHoriNorm(rootToFoot);
	rootToFootHoriNorm.HorizontalAndNormalise();
	Vector3 centreToRoot(_rootPos - _centrePos);
	centreToRoot.HorizontalAndNormalise();
	Vector3 axis((centreToRoot ^ g_upVector).Normalise());
	float cosTheta = (rootToFootLen * 0.4f) / m_thighLen;
	// FIXME
	// cosTheta should never be greater than one, yet sometimes it is
	if (cosTheta > 1.0f) cosTheta = 1.0f;
	float theta = -acosf(cosTheta);

	Vector3 footToKnee(-rootToFoot);
	footToKnee.SetLength(m_shinLen);
	footToKnee.RotateAround(axis * theta);

	return _footPos + footToKnee;
}


void EntityLeg::LiftFoot(float _targetHoverHeight)
{
	m_foot.m_targetPos = CalcDesiredFootPos(_targetHoverHeight);
	m_foot.m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(
							m_foot.m_targetPos.x, m_foot.m_targetPos.z);
	m_foot.m_state = EntityFoot::Swinging;
	m_foot.m_leftGroundTimeStamp = g_gameTime;
	m_foot.m_lastGroundPos = m_foot.m_pos;
}


void EntityLeg::PlantFoot()
{
	m_foot.m_pos = m_foot.m_targetPos;
	m_foot.m_state = EntityFoot::OnGround;
}


Vector3 EntityLeg::GetIdealSwingingFootPos(float _fractionComplete)
{
	float fractionIncomplete = 1.0f - _fractionComplete;
	Vector3 pos = m_foot.m_lastGroundPos * fractionIncomplete + 
				  m_foot.m_targetPos * _fractionComplete;

	if (_fractionComplete < 0.33f)
	{
		pos.y += _fractionComplete * 3.0f * m_legLift;
	}
	else if (fractionIncomplete < 0.33f)
	{
		pos.y += fractionIncomplete * 3.0f * m_legLift;
	}
	else
	{
		pos.y += m_legLift;
	}

	return pos;
}


// Returns true if the foot was planted this frame
bool EntityLeg::Advance()
{
	if (m_foot.m_state == EntityFoot::Swinging)
	{
		float fractionComplete = RampUpAndDown(m_foot.m_leftGroundTimeStamp, 
											   m_legSwingDuration, g_gameTime);
		if (fractionComplete > 1.0f)
		{
			PlantFoot();
			return true;
		}
		else
		{
			m_foot.m_pos = GetIdealSwingingFootPos(fractionComplete);
		}
	}

	return false;
}


void EntityLeg::AdvanceSpiderPounce(float _fractionComplete)
{
	Vector3 offset = m_foot.m_bodyToFoot;
	if (_fractionComplete < 0.5f)
	{
		offset *= 1.0f - _fractionComplete;
	}
	else
	{
		offset *= _fractionComplete;
	}
	m_foot.m_pos = m_parent->m_pos - offset + m_parent->m_vel * _fractionComplete * 0.05f;
	m_foot.m_lastGroundPos = m_foot.m_pos;
	m_foot.m_targetPos = m_foot.m_pos;
}


void EntityLeg::Render(float _predictionTime, Vector3 const &_predictedMovement)
{
	Vector3 predictedPos = m_parent->m_pos + _predictedMovement;
	Vector3 rootPos(_predictedMovement + GetLegRootPos());
	Vector3 footPos;

	switch (m_foot.m_state)
	{
		case EntityFoot::OnGround:
			footPos = m_foot.m_pos;
			break;
		
		case EntityFoot::Swinging:
		{
			float fractionComplete = RampUpAndDown(m_foot.m_leftGroundTimeStamp, 
												   m_legSwingDuration, g_gameTime);
			footPos = GetIdealSwingingFootPos(fractionComplete);
			break;
		}

		case EntityFoot::Pouncing:
			footPos = m_foot.m_pos + _predictedMovement;
			//footPos = predictedPos - m_foot.m_bodyToFoot;
			break;
	}

	Vector3 kneePos = CalcKneePos(footPos, rootPos, predictedPos);
	
	{
		Vector3 up((kneePos - footPos).Normalise());
		Vector3 front(up ^ g_upVector);
		Matrix34 mat(front, up, footPos);
		m_shapeLower->Render(_predictionTime, mat);

        //RenderArrow( kneePos, footPos, 1.0f );
	}

	{
		Vector3 up((rootPos - kneePos).Normalise());
		Vector3 front(up ^ g_upVector);
		Matrix34 mat(front, up, kneePos);
		m_shapeUpper->Render(_predictionTime, mat);

        //RenderArrow( rootPos, kneePos, 1.0f );
	}
}


bool EntityLeg::RenderPixelEffect(float _predictionTime, Vector3 const &_predictedMovement)
{
	Vector3 predictedPos = m_parent->m_pos + _predictedMovement;
	Vector3 rootPos(_predictedMovement + GetLegRootPos());
	Vector3 footPos;

	switch (m_foot.m_state)
	{
		case EntityFoot::OnGround:
			footPos = m_foot.m_pos;
			break;
		
		case EntityFoot::Swinging:
		{
			float fractionComplete = RampUpAndDown(m_foot.m_leftGroundTimeStamp, 
												   m_legSwingDuration, g_gameTime);
			footPos = GetIdealSwingingFootPos(fractionComplete);
			break;
		}

		case EntityFoot::Pouncing:
			footPos = m_foot.m_pos + _predictedMovement;// - m_foot.m_bodyToFoot;
			break;
	}

	Vector3 kneePos = CalcKneePos(footPos, rootPos, predictedPos);

	{
		Vector3 up((kneePos - footPos).Normalise());
		Vector3 front(up ^ Vector3(1,0,0).Normalise());
		Matrix34 mat(front, up, footPos);
		g_app->m_renderer->MarkUsedCells(m_shapeLower, mat);
	}

	{
		Vector3 up((rootPos - kneePos).Normalise());
		Vector3 front(up ^ Vector3(1,0,0).Normalise());
		Matrix34 mat(front, up, kneePos);
		g_app->m_renderer->MarkUsedCells(m_shapeUpper, mat);
	}

    return true;
}
