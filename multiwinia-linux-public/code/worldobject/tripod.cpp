#include "lib/universal_include.h"

#include <float.h>

#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/persisting_debug_render.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/vector2.h"
#include "lib/math/random_number.h"

#include "worldobject/entity_leg.h"
#include "worldobject/tripod.h"

#include "app.h"
#include "entity_grid.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "renderer.h"


#define FOOT_MOVE_THRESHOLD	1.0	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
#define IDEAL_LEG_SLOPE		0.2
#define LEG_LIFT			3.0
#define LEG_SWING_DURATION	0.5
#define LOOK_AHEAD_COEF		1.2

#define ATTACK_HOVER_HEIGHT		18.0
#define STATIONARY_HOVER_HEIGHT 30.0
#define HOVER_LOWER_WITH_SPEED_FACTOR	0.2

#define NAVIGATION_SEARCH_RADIUS	1000.0
#define ATTACK_SEARCH_RADIUS		200.0

#define ATTACK_DURATION				7.0


//*****************************************************************************
// Class TripodNavData
//*****************************************************************************

TripodNavData::TripodNavData()
{
	m_directions[0].x = 0.5;	m_directions[0].y = 0.86603;
	m_directions[1].x = 1.0;	m_directions[1].y = 0.0;
	m_directions[2].x = 0.5;	m_directions[2].y = -0.86603;
	m_directions[3].x = -0.5;	m_directions[3].y = -0.86603;
	m_directions[4].x = -1.0;	m_directions[4].y = 0.0;
	m_directions[5].x = -0.5;	m_directions[5].y = 0.86603;
}


//*****************************************************************************
// Class Tripod
//*****************************************************************************

Tripod::Tripod()
:	m_mode(ModeWalking),
	m_nextLegToMove(0),
	m_speed(0.0),
	m_targetHoverHeight(27.0),
	m_bodyVel(0,0,0),
	m_up(g_upVector)
{
	//m_shape = g_app->m_resource->GetShape("tripod.shp");
	m_modeStartTime = 0.0;
	
	// Initialise legs
	/*for (int i = 0; i < 3; ++i)
	{
		char markerName[] = "MarkerLegX";
		markerName[strlen(markerName) - 1] = '0' + i;
		m_legs[i] = new EntityLeg(i, this, "tripod_leg_part.shp", "tripod_leg_part.shp", markerName);
		m_legs[i]->m_legLift = LEG_LIFT;
		m_legs[i]->m_idealLegSlope = IDEAL_LEG_SLOPE;
		m_legs[i]->m_legSwingDuration = LEG_SWING_DURATION;
		m_legs[i]->m_lookAheadCoef = LOOK_AHEAD_COEF;
	}*/
}


Tripod::~Tripod()
{
	for (int i = 0; i < 3; ++i)
	{
		delete m_legs[i];
	}
}


void Tripod::Begin()
{
    SetShape( "tripod.shp" );
    // Initialise legs
	for (int i = 0; i < 3; ++i)
	{
		char markerName[] = "MarkerLegX";
		markerName[strlen(markerName) - 1] = '0' + i;
		m_legs[i] = new EntityLeg(i, this, "tripod_leg_part.shp", "tripod_leg_part.shp", markerName);
		m_legs[i]->m_legLift = LEG_LIFT;
		m_legs[i]->m_idealLegSlope = IDEAL_LEG_SLOPE;
		m_legs[i]->m_legSwingDuration = LEG_SWING_DURATION;
		m_legs[i]->m_lookAheadCoef = LOOK_AHEAD_COEF;
	}

	Entity::Begin();
	
	for (int i = 0; i < 3; ++i)
	{
		m_legs[i]->LiftFoot(m_targetHoverHeight);
		m_legs[i]->PlantFoot();
	}

	m_navData.m_targetPos = m_pos;
	m_navData.m_dir = -1;
}


bool Tripod::ChangeHealth(int _amount, int _damageType)
{
    if( _damageType == DamageTypeLaser ) return false;

	if (m_mode == ModeAttacking)
	{
	    bool dead = m_dead;
	
		Entity::ChangeHealth(_amount);

		if (m_dead && !dead)
		{
			// We just died
			Matrix34 transform( m_front, m_up, m_pos );
			g_explosionManager.AddExplosion( m_shape, transform ); 
		}
        return true;
	}

    return false;
}


int Tripod::CalcWhichFootToMove()
{
	bool allOnGround = m_legs[0]->m_foot.m_state == EntityFoot::OnGround &&
					   m_legs[1]->m_foot.m_state == EntityFoot::OnGround &&
					   m_legs[2]->m_foot.m_state == EntityFoot::OnGround;

	if (!allOnGround) return -1;

	double bestScore = 0.0;
	double bestFoot = -1;

	for (int i = 0; i < 3; ++i)
	{
		double score = m_legs[i]->CalcFootsDesireToMove(m_targetHoverHeight);
		if (score > bestScore) 
		{
			bestScore = score;
			bestFoot = i;
		}
	}

	if (bestScore > FOOT_MOVE_THRESHOLD)
	{
		return bestFoot;
	}

	return -1;
}


void Tripod::DoFallForTwoLegs()
{
	// Calc point between feet and point under body
	Vector3 footToFoot;
	Vector3 pointBetweenFeet;
	Vector3 pointUnderBody;
	if (!m_legs[0]->m_foot.m_state == EntityFoot::OnGround)
	{
		footToFoot = (m_legs[1]->m_foot.m_pos - m_legs[2]->m_foot.m_pos);
		RayRayDist(m_legs[1]->m_foot.m_pos, footToFoot, m_pos, g_upVector, &pointBetweenFeet, &pointUnderBody);
	}
	else if (!m_legs[1]->m_foot.m_state == EntityFoot::OnGround)
	{
		footToFoot = (m_legs[2]->m_foot.m_pos - m_legs[0]->m_foot.m_pos);
		RayRayDist(m_legs[0]->m_foot.m_pos, footToFoot, m_pos, g_upVector, &pointBetweenFeet, &pointUnderBody);
	}
	else
	{
		footToFoot = (m_legs[0]->m_foot.m_pos - m_legs[1]->m_foot.m_pos);
		RayRayDist(m_legs[0]->m_foot.m_pos, footToFoot, m_pos, g_upVector, &pointBetweenFeet, &pointUnderBody);
	}

	// Calc moment due to centre of gravity and the contact point on the ground not being vertically aligned
	Vector3 lever(pointBetweenFeet - pointUnderBody);
	double momentPerUnitMass = lever.Mag() * GRAVITY;

	// Calc force due to moment
	Vector3 feetToBody(m_pos - pointBetweenFeet);
	double feetToBodyLen = feetToBody.Mag();
	Vector3 footToFootDir(footToFoot);
	footToFootDir.Normalise();
	Vector3 forcePerUnitMass(m_up ^ footToFootDir);
	forcePerUnitMass.SetLength(momentPerUnitMass / feetToBodyLen);

	// Calc change in body vel due to force
	m_bodyVel += forcePerUnitMass * SERVER_ADVANCE_PERIOD;

	// Calc change in body angle due to change in position (it is rotating around pointBetweenFeet)
	double rotation = m_bodyVel.Mag() / feetToBodyLen; // Using small angle approximation (tan theta ~= theta)
	Vector3 axis(footToFootDir * -rotation * 0.1);
	m_up.RotateAround(axis);
//		m_front.RotateAround(axis);
	
	m_pos += m_bodyVel * SERVER_ADVANCE_PERIOD;
	Vector3 newFeetToBody(m_pos - pointBetweenFeet);
	newFeetToBody.SetLength(feetToBodyLen);
	m_pos = pointBetweenFeet + newFeetToBody;
}


WorldObjectId Tripod::FindEntityToAttack()
{
	START_PROFILE( "FindEntityToA");

	WorldObjectId id;

	int numFound;	
	g_app->m_location->m_entityGrid->GetEnemies(s_neighbours, m_pos.x, m_pos.z, 
                                                ATTACK_SEARCH_RADIUS, &numFound, m_id.GetTeamId());

	int nearest = -1;
	double nearestDistSqrd = FLT_MAX;
	for (int i = 0; i < numFound; ++i)
	{
		Entity *entity = g_app->m_location->GetEntity(s_neighbours[i]);
		double deltaX = entity->m_pos.x - m_pos.x;
		double deltaY = entity->m_pos.z - m_pos.z;
		double distSqrd = deltaX * deltaX + deltaY * deltaY;
		if (distSqrd < nearestDistSqrd)
		{
			nearestDistSqrd = distSqrd;
			nearest = i;
		}
	}

	if (nearest != -1)
	{
		id = s_neighbours[nearest];
	}

	END_PROFILE( "FindEntityToA");

	return id;
}


Vector2 Tripod::ChooseDestination()
{
	START_PROFILE( "ChooseDest");

	int numFound;
	g_app->m_location->m_entityGrid->GetEnemies(s_neighbours, m_pos.x, m_pos.z, 
																	NAVIGATION_SEARCH_RADIUS, &numFound, m_id.GetTeamId());
	Vector2 pos;

	if (numFound == 0)
	{
		pos = m_pos;
		pos.x += syncsfrand(300.0);
		pos.y += syncsfrand(300.0);
	}
	else
	{
		int i = syncrand() % numFound;
		Entity *targetEnemy = g_app->m_location->GetEntity(s_neighbours[i]);
		pos = targetEnemy->m_pos;
		pos.x += syncsfrand(100.0);
		pos.y += syncsfrand(100.0);
	}

	END_PROFILE( "ChooseDest");

	return pos;
}


void Tripod::DoNavigation()
{
	START_PROFILE( "DoNav");

	// If m_dir is -1 that means we aren't trying to go anywhere. We need to
	// wait until we come to rest before we choose a new direction to travel in
	if (m_navData.m_dir == -1)
	{
		double speed = m_vel.Mag();
		if (speed > 0.5)
		{
			END_PROFILE( "DoNav");
			return;
		}
	}

	// Have we reached our destination?
	Vector2 pos(m_pos);
	Vector2 delta(m_navData.m_targetPos - pos);
	double deltaMag = delta.Mag();
	if (deltaMag < 1.0)
	{
		m_navData.m_targetPos = ChooseDestination();

		delta = (m_navData.m_targetPos - pos);
		deltaMag = delta.Mag();
		m_navData.m_dir = -1;
	}

	// Are we currently going in a direction?
	if (m_navData.m_dir == -1)
	{
		// Choose the best direction
		Vector2 deltaNorm(delta / deltaMag);
		double largestDot = m_navData.m_directions[0] * deltaNorm;
		unsigned int best = 0;
		for (unsigned int i = 1; i < 6; i++)
		{
			double result = m_navData.m_directions[i] * deltaNorm;
			if (result > largestDot)
			{
				largestDot = result;
				best = i;
			}
		}
		m_navData.m_dir = best;
	}

	// Should we changed direction?
	double cross = m_navData.m_directions[m_navData.m_dir] ^ delta;
	double theta = iv_asin(cross / deltaMag);
	if (theta < -M_PI / 6.0)
	{
		m_navData.m_dir++;
		m_navData.m_dir = m_navData.m_dir % 6;
	}
	else if (theta > M_PI / 6.0)
	{
		m_navData.m_dir--;
		m_navData.m_dir = m_navData.m_dir % 6;
	}

	// Apply some acceleration to move us in our chosen direction
	m_vel.x += m_navData.m_directions[m_navData.m_dir].x * 1.0;
	m_vel.z += m_navData.m_directions[m_navData.m_dir].y * 1.0;

	END_PROFILE( "DoNav");
}


Vector3 Tripod::CalcAttackUpVector()
{
	Vector3 toEnemy = m_attackTarget - m_pos;
	toEnemy.Normalise();
	Vector3 toEnemyHorizontal(toEnemy);
	toEnemyHorizontal.HorizontalAndNormalise();
	Vector3 up = g_upVector - toEnemyHorizontal;
	up.Normalise();

	return up;
}


void Tripod::AdvanceWalk()
{
	START_PROFILE( "AdvanceWalk");

	// Consider mode switch
	{
		double timeSinceAttack = g_gameTime - m_modeStartTime;
		if (timeSinceAttack > 10.0 + ATTACK_DURATION)
		{
			WorldObjectId targetId = FindEntityToAttack();
			if (targetId.IsValid())
			{
				Entity *target = g_app->m_location->GetEntity(targetId);
				m_attackTarget = target->m_pos;
				m_mode = ModePreAttack;
				m_modeStartTime = g_gameTime;
				END_PROFILE( "AdvanceWalk");
				return;
			}
		}
	}


	// Consider choosing a different nav dest
	if (syncrand() % 40 == 1)
	{
		m_navData.m_targetPos = ChooseDestination();				
	}


	// Hover
	m_targetHoverHeight = STATIONARY_HOVER_HEIGHT - fabsf(m_speed) * HOVER_LOWER_WITH_SPEED_FACTOR;


	// Navigate
	DoNavigation();
	

	// Fall
	{
		bool allOnGround = m_legs[0]->m_foot.m_state == EntityFoot::OnGround &&
						   m_legs[1]->m_foot.m_state == EntityFoot::OnGround &&
						   m_legs[2]->m_foot.m_state == EntityFoot::OnGround;
		if (!allOnGround)
		{
			DoFallForTwoLegs();
		}
		else
		{
			m_bodyVel.Zero();
			m_front.HorizontalAndNormalise();
			m_up = g_upVector;
		}
	}

	END_PROFILE( "AdvanceWalk");
}


void Tripod::AdvancePreAttack()
{
	START_PROFILE( "AdvancePreAttack");

	// Exit if we haven't come to a stop yet
	if (m_vel.Mag() > 0.05)
	{
		END_PROFILE( "AdvancePreAttack");
		return;
	}
	
	m_targetHoverHeight = ATTACK_HOVER_HEIGHT;

	// See if we have achieved a full crouch yet
	double height = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
	if (height < ATTACK_HOVER_HEIGHT + 0.5)
	{
		m_mode = ModeAttacking;
		m_modeStartTime = g_gameTime;
	}

	// Blend into attack orientation
	Vector3 desiredUp = CalcAttackUpVector();
	double factor1 = 0.8 * SERVER_ADVANCE_PERIOD;
	double factor2 = 1.0 - factor1;
	m_up = factor1 * desiredUp + factor2 * m_up;
	Vector3 right = m_up ^ m_front;
	m_front = right ^ m_up;
	m_front.Normalise();

	END_PROFILE( "AdvancePreAttack");
}


void Tripod::AdvanceAttack()
{
	START_PROFILE( "AdvanceAttack");


	// Consider mode switch
	double timeInAttack = g_gameTime - m_modeStartTime;
	if (timeInAttack > ATTACK_DURATION)
	{
		m_mode = ModePostAttack;
		m_modeStartTime = g_gameTime;
		END_PROFILE( "AdvanceAttack");
		return;
	}


//	m_up = CalcAttackUpVector();
	Vector3 right = m_up ^ g_upVector;
//	m_front = m_up ^ right;
//	m_front.Normalise();

	if (timeInAttack > 1.0)
	{
		int t2 = (int)(timeInAttack * 10.0);
		if (t2 & 1)
		{
			Vector3 toEnemy = m_attackTarget - m_pos;
			toEnemy.Normalise();
			double const speed = 80.0;
			right.SetLength(4.0);
			Vector3 pos = m_pos + right;
			toEnemy.x += syncsfrand(0.1);
			toEnemy.z += syncsfrand(0.1);
			g_app->m_location->FireLaser(pos + toEnemy * 5.0, toEnemy * speed, m_id.GetTeamId());
			pos = m_pos - right;
			g_app->m_location->FireLaser(pos + toEnemy * 5.0, toEnemy * speed, m_id.GetTeamId());
		}
	}

	END_PROFILE( "AdvanceAttack");
}


void Tripod::AdvancePostAttack()
{
	m_targetHoverHeight = STATIONARY_HOVER_HEIGHT;

	// Check if we have achieved full walking height yet
	double height = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
	if (height > STATIONARY_HOVER_HEIGHT - 0.5)
	{
		m_mode = ModeWalking;
		m_modeStartTime = g_gameTime;
	}

	// Blend out of attack orientation
	double factor1 = 0.8 * SERVER_ADVANCE_PERIOD;
	double factor2 = 1.0 - factor1;
	m_up = factor1 * g_upVector + factor2 * m_up;
	Vector3 right = m_up ^ m_front;
	m_front = right ^ m_up;
	m_front.Normalise();
}


bool Tripod::Advance(Unit *_unit)
{
	//
	// Advance current mode

	switch (m_mode)
	{
		case ModeWalking:		AdvanceWalk();			break;
		case ModePreAttack:		AdvancePreAttack();		break;
		case ModeAttacking:		AdvanceAttack();		break;
		case ModePostAttack:	AdvancePostAttack();	break;
	}


	// 
	// Rotation

	m_front.RotateAroundY(g_advanceTime * m_angVel.y);
	m_angVel.y *= 1.0 - SERVER_ADVANCE_PERIOD * 0.5;


	//
	// Move

	double factor1 = SERVER_ADVANCE_PERIOD * 1.65;
	double factor2 = 1.0 - factor1;
	m_speed *= 1.0 - SERVER_ADVANCE_PERIOD * 0.5;
	Vector3 frontHoriNorm = m_front;
	frontHoriNorm.HorizontalAndNormalise();
	m_vel = m_front * m_speed * factor1 + m_vel * factor2;
	m_pos += m_vel * SERVER_ADVANCE_PERIOD;


	//
	// Adjust body height

	{
		double targetHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
		targetHeight += m_targetHoverHeight;
		double factor1 = 1.0 * SERVER_ADVANCE_PERIOD;
		double factor2 = 1.0 - factor1;
		m_pos.y = factor1 * targetHeight + factor2 * m_pos.y;
	}


	//
	// Update legs

	int bestFootToMove = CalcWhichFootToMove();
	if (bestFootToMove != -1)
	{
		m_legs[bestFootToMove]->LiftFoot(m_targetHoverHeight);
	}

	for (int i = 0; i < 3; ++i)
	{
		m_legs[i]->Advance();
	}

	
	// 
	// Detect death

	if (m_pos.y < g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z))
	{
		Matrix34 mat(m_front, m_up, m_pos);
		g_explosionManager.AddExplosion(m_shape, mat);
		return true;
	}
	
	return m_dead;
}


void Tripod::Render(double _predictionTime)
{
    glDisable( GL_TEXTURE_2D );
    
	g_app->m_renderer->SetObjectLighting();


	// 
	// Render body

	Vector3 predictedMovement = _predictionTime * m_vel;
	Vector3 predictedPos = m_pos + predictedMovement;
//	predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z) +
//					 m_targetHoverHeight;

	Matrix34 mat(m_front, m_up, predictedPos);
	m_shape->Render(_predictionTime, mat);


	//
	// Render Legs

	for (int i = 0; i < 3; ++i)
	{
		m_legs[i]->Render(_predictionTime, predictedMovement);
	}

//	m_s->Render();

	g_app->m_renderer->UnsetObjectLighting();
}
