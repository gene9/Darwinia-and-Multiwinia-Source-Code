#include "lib/universal_include.h"

#include <float.h>

#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/persisting_debug_render.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/vector2.h"

#include "worldobject/entity_leg.h"
#include "worldobject/tripod.h"

#include "app.h"
#include "entity_grid.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "renderer.h"


#define FOOT_MOVE_THRESHOLD	1.0f	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
#define IDEAL_LEG_SLOPE		0.2f
#define LEG_LIFT			3.0f
#define LEG_SWING_DURATION	0.5f
#define LOOK_AHEAD_COEF		1.2f

#define ATTACK_HOVER_HEIGHT		18.0f
#define STATIONARY_HOVER_HEIGHT 30.0f
#define HOVER_LOWER_WITH_SPEED_FACTOR	0.2f

#define NAVIGATION_SEARCH_RADIUS	1000.0f
#define ATTACK_SEARCH_RADIUS		200.0f

#define ATTACK_DURATION				7.0f


//*****************************************************************************
// Class TripodNavData
//*****************************************************************************

TripodNavData::TripodNavData()
{
	m_directions[0].x = 0.5f;	m_directions[0].y = 0.86603f;
	m_directions[1].x = 1.0f;	m_directions[1].y = 0.0f;
	m_directions[2].x = 0.5f;	m_directions[2].y = -0.86603f;
	m_directions[3].x = -0.5f;	m_directions[3].y = -0.86603f;
	m_directions[4].x = -1.0f;	m_directions[4].y = 0.0f;
	m_directions[5].x = -0.5f;	m_directions[5].y = 0.86603f;
}


//*****************************************************************************
// Class Tripod
//*****************************************************************************

Tripod::Tripod()
:	m_mode(ModeWalking),
	m_nextLegToMove(0),
	m_speed(0.0f),
	m_targetHoverHeight(27.0f),
	m_bodyVel(0,0,0),
	m_up(g_upVector)
{
	m_shape = g_app->m_resource->GetShape("tripod.shp");
	m_modeStartTime = 0.0f;

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
	Entity::Begin();

	for (int i = 0; i < 3; ++i)
	{
		m_legs[i]->LiftFoot(m_targetHoverHeight);
		m_legs[i]->PlantFoot();
	}

	m_navData.m_targetPos = m_pos;
	m_navData.m_dir = -1;
}


void Tripod::ChangeHealth(int _amount)
{
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
	}
}


int Tripod::CalcWhichFootToMove()
{
	bool allOnGround = m_legs[0]->m_foot.m_state == EntityFoot::OnGround &&
					   m_legs[1]->m_foot.m_state == EntityFoot::OnGround &&
					   m_legs[2]->m_foot.m_state == EntityFoot::OnGround;

	if (!allOnGround) return -1;

	float bestScore = 0.0f;
	float bestFoot = -1;

	for (int i = 0; i < 3; ++i)
	{
		float score = m_legs[i]->CalcFootsDesireToMove(m_targetHoverHeight);
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
	float momentPerUnitMass = lever.Mag() * GRAVITY;

	// Calc force due to moment
	Vector3 feetToBody(m_pos - pointBetweenFeet);
	float feetToBodyLen = feetToBody.Mag();
	Vector3 footToFootDir(footToFoot);
	footToFootDir.Normalise();
	Vector3 forcePerUnitMass(m_up ^ footToFootDir);
	forcePerUnitMass.SetLength(momentPerUnitMass / feetToBodyLen);

	// Calc change in body vel due to force
	m_bodyVel += forcePerUnitMass * SERVER_ADVANCE_PERIOD;

	// Calc change in body angle due to change in position (it is rotating around pointBetweenFeet)
	float rotation = m_bodyVel.Mag() / feetToBodyLen; // Using small angle approximation (tan theta ~= theta)
	Vector3 axis(footToFootDir * -rotation * 0.1f);
	m_up.RotateAround(axis);
//		m_front.RotateAround(axis);

	m_pos += m_bodyVel * SERVER_ADVANCE_PERIOD;
	Vector3 newFeetToBody(m_pos - pointBetweenFeet);
	newFeetToBody.SetLength(feetToBodyLen);
	m_pos = pointBetweenFeet + newFeetToBody;
}


WorldObjectId Tripod::FindEntityToAttack()
{
	START_PROFILE(g_app->m_profiler, "FindEntityToA");

	WorldObjectId id;

	int numFound;
	WorldObjectId *enemies = g_app->m_location->m_entityGrid->GetEnemies(m_pos.x, m_pos.z,
																	ATTACK_SEARCH_RADIUS, &numFound, m_id.GetTeamId());

	int nearest = -1;
	float nearestDistSqrd = FLT_MAX;
	for (int i = 0; i < numFound; ++i)
	{
		Entity *entity = g_app->m_location->GetEntity(enemies[i]);
		float deltaX = entity->m_pos.x - m_pos.x;
		float deltaY = entity->m_pos.z - m_pos.z;
		float distSqrd = deltaX * deltaX + deltaY * deltaY;
		if (distSqrd < nearestDistSqrd)
		{
			nearestDistSqrd = distSqrd;
			nearest = i;
		}
	}

	if (nearest != -1)
	{
		id = enemies[nearest];
	}

	END_PROFILE(g_app->m_profiler, "FindEntityToA");

	return id;
}


Vector2 Tripod::ChooseDestination()
{
	START_PROFILE(g_app->m_profiler, "ChooseDest");

	int numFound;
	WorldObjectId *enemies = g_app->m_location->m_entityGrid->GetEnemies(m_pos.x, m_pos.z,
																	NAVIGATION_SEARCH_RADIUS, &numFound, m_id.GetTeamId());
	Vector2 pos;

	if (numFound == 0)
	{
		pos = m_pos;
		pos.x += syncsfrand(300.0f);
		pos.y += syncsfrand(300.0f);
	}
	else
	{
		int i = syncrand() % numFound;
		Entity *targetEnemy = g_app->m_location->GetEntity(enemies[i]);
		pos = targetEnemy->m_pos;
		pos.x += syncsfrand(100.0f);
		pos.y += syncsfrand(100.0f);
	}

	END_PROFILE(g_app->m_profiler, "ChooseDest");

	return pos;
}


void Tripod::DoNavigation()
{
	START_PROFILE(g_app->m_profiler, "DoNav");

	// If m_dir is -1 that means we aren't trying to go anywhere. We need to
	// wait until we come to rest before we choose a new direction to travel in
	if (m_navData.m_dir == -1)
	{
		float speed = m_vel.Mag();
		if (speed > 0.5f)
		{
			END_PROFILE(g_app->m_profiler, "DoNav");
			return;
		}
	}

	// Have we reached our destination?
	Vector2 pos(m_pos);
	Vector2 delta(m_navData.m_targetPos - pos);
	float deltaMag = delta.Mag();
	if (deltaMag < 1.0f)
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
		float largestDot = m_navData.m_directions[0] * deltaNorm;
		unsigned int best = 0;
		for (unsigned int i = 1; i < 6; i++)
		{
			float result = m_navData.m_directions[i] * deltaNorm;
			if (result > largestDot)
			{
				largestDot = result;
				best = i;
			}
		}
		m_navData.m_dir = best;
	}

	// Should we changed direction?
	float cross = m_navData.m_directions[m_navData.m_dir] ^ delta;
	float theta = asinf(cross / deltaMag);
	if (theta < -M_PI / 6.0f)
	{
		m_navData.m_dir++;
		m_navData.m_dir = m_navData.m_dir % 6;
	}
	else if (theta > M_PI / 6.0f)
	{
		m_navData.m_dir--;
		m_navData.m_dir = m_navData.m_dir % 6;
	}

	// Apply some acceleration to move us in our chosen direction
	m_vel.x += m_navData.m_directions[m_navData.m_dir].x * 1.0f;
	m_vel.z += m_navData.m_directions[m_navData.m_dir].y * 1.0f;

	END_PROFILE(g_app->m_profiler, "DoNav");
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
	START_PROFILE(g_app->m_profiler, "AdvanceWalk");

	// Consider mode switch
	{
		float timeSinceAttack = g_gameTime - m_modeStartTime;
		if (timeSinceAttack > 10.0f + ATTACK_DURATION)
		{
			WorldObjectId targetId = FindEntityToAttack();
			if (targetId.IsValid())
			{
				Entity *target = g_app->m_location->GetEntity(targetId);
				m_attackTarget = target->m_pos;
				m_mode = ModePreAttack;
				m_modeStartTime = g_gameTime;
				END_PROFILE(g_app->m_profiler, "AdvanceWalk");
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

	END_PROFILE(g_app->m_profiler, "AdvanceWalk");
}


void Tripod::AdvancePreAttack()
{
	START_PROFILE(g_app->m_profiler, "AdvancePreAttack");

	// Exit if we haven't come to a stop yet
	if (m_vel.Mag() > 0.05f)
	{
		END_PROFILE(g_app->m_profiler, "AdvancePreAttack");
		return;
	}

	m_targetHoverHeight = ATTACK_HOVER_HEIGHT;

	// See if we have achieved a full crouch yet
	float height = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
	if (height < ATTACK_HOVER_HEIGHT + 0.5f)
	{
		m_mode = ModeAttacking;
		m_modeStartTime = g_gameTime;
	}

	// Blend into attack orientation
	Vector3 desiredUp = CalcAttackUpVector();
	float factor1 = 0.8f * SERVER_ADVANCE_PERIOD;
	float factor2 = 1.0f - factor1;
	m_up = factor1 * desiredUp + factor2 * m_up;
	Vector3 right = m_up ^ m_front;
	m_front = right ^ m_up;
	m_front.Normalise();

	END_PROFILE(g_app->m_profiler, "AdvancePreAttack");
}


void Tripod::AdvanceAttack()
{
	START_PROFILE(g_app->m_profiler, "AdvanceAttack");


	// Consider mode switch
	float timeInAttack = g_gameTime - m_modeStartTime;
	if (timeInAttack > ATTACK_DURATION)
	{
		m_mode = ModePostAttack;
		m_modeStartTime = g_gameTime;
		END_PROFILE(g_app->m_profiler, "AdvanceAttack");
		return;
	}


//	m_up = CalcAttackUpVector();
	Vector3 right = m_up ^ g_upVector;
//	m_front = m_up ^ right;
//	m_front.Normalise();

	if (timeInAttack > 1.0f)
	{
		int t2 = (int)(timeInAttack * 10.0f);
		if (t2 & 1)
		{
			Vector3 toEnemy = m_attackTarget - m_pos;
			toEnemy.Normalise();
			float const speed = 80.0f;
			right.SetLength(4.0f);
			Vector3 pos = m_pos + right;
			toEnemy.x += syncsfrand(0.1f);
			toEnemy.z += syncsfrand(0.1f);
			g_app->m_location->FireLaser(pos + toEnemy * 5.0f, toEnemy * speed, m_id.GetTeamId());
			pos = m_pos - right;
			g_app->m_location->FireLaser(pos + toEnemy * 5.0f, toEnemy * speed, m_id.GetTeamId());
		}
	}

	END_PROFILE(g_app->m_profiler, "AdvanceAttack");
}


void Tripod::AdvancePostAttack()
{
	m_targetHoverHeight = STATIONARY_HOVER_HEIGHT;

	// Check if we have achieved full walking height yet
	float height = m_pos.y - g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
	if (height > STATIONARY_HOVER_HEIGHT - 0.5f)
	{
		m_mode = ModeWalking;
		m_modeStartTime = g_gameTime;
	}

	// Blend out of attack orientation
	float factor1 = 0.8f * SERVER_ADVANCE_PERIOD;
	float factor2 = 1.0f - factor1;
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
	m_angVel.y *= 1.0f - SERVER_ADVANCE_PERIOD * 0.5f;


	//
	// Move

	float factor1 = SERVER_ADVANCE_PERIOD * 1.65f;
	float factor2 = 1.0f - factor1;
	m_speed *= 1.0f - SERVER_ADVANCE_PERIOD * 0.5f;
	Vector3 frontHoriNorm = m_front;
	frontHoriNorm.HorizontalAndNormalise();
	m_vel = m_front * m_speed * factor1 + m_vel * factor2;
	m_pos += m_vel * SERVER_ADVANCE_PERIOD;


	//
	// Adjust body height

	{
		float targetHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
		targetHeight += m_targetHoverHeight;
		float factor1 = 1.0f * SERVER_ADVANCE_PERIOD;
		float factor2 = 1.0f - factor1;
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


void Tripod::Render(float _predictionTime)
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
