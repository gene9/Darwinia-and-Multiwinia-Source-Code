#include "lib/universal_include.h"

#include <float.h>

#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/persisting_debug_render.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/vector2.h"
#include "lib/text_renderer.h"

#include "worldobject/entity_leg.h"
#include "worldobject/spider.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "entity_grid.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "camera.h"


//#define FOOT_MOVE_THRESHOLD	        5.0f	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
//#define FOOT_EMERGENCY_THRESHOLD	12.0f
//#define FOOT_DAMAGE_RADIUS			20.0f	// Size of region damaged by foot falls
//#define FOOT_DAMAGE_STRENGTH		10.0f
//
//#define HOVER_HEIGHT				3.0f
//
//#define TURN_RATE					0.18f
//#define NORMAL_SPEED				300.0f
//#define ATTACK_SPEED				90.0f
//
//#define MAX_PATH_HEIGHT				100.0f
//
//#define ATTACK_SEARCH_MAX_RADIUS	250.0f
//#define ATTACK_SEARCH_MIN_RADIUS	100.0f
//#define ATTACK_PREAMBLE				5.0f	// Minimum time between choosing a target and starting to charge (most of this time will normally be spent turning)

#define FOOT_MOVE_THRESHOLD	        5.0f	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
#define FOOT_EMERGENCY_THRESHOLD	12.0f
#define FOOT_DAMAGE_RADIUS			20.0f	// Size of region damaged by foot falls
#define FOOT_DAMAGE_STRENGTH		10.0f

#define HOVER_HEIGHT				3.0f

#define TURN_RATE					0.1f
#define NORMAL_SPEED				300.0f
#define ATTACK_SPEED				90.0f

#define MAX_PATH_HEIGHT				100.0f

#define ATTACK_SEARCH_MAX_RADIUS	250.0f
#define ATTACK_SEARCH_MIN_RADIUS	100.0f
#define ATTACK_PREAMBLE				5.0f	// Minimum time between choosing a target and starting to charge (most of this time will normally be spent turning)

#define SPIRIT_MAXSEARCHRANGE       100.0f
#define SPIRIT_MINSEARCHRANGE       30.0f


//*****************************************************************************
// Class Spider
//*****************************************************************************

Spider::Spider()
:	m_state(StateIdle),
	m_nextLegMoveTime(-1.0f),
	m_speed(0.0f),
	m_targetHoverHeight(HOVER_HEIGHT),
	m_up(g_upVector),
    m_retargetTimer(0.0f),
    m_spiritId(-1),
    m_eggLay(NULL)
{
	m_stats[StatHealth] = 200;

	m_shape = g_app->m_resource->GetShape("spider.shp");
    m_eggLay = m_shape->m_rootFragment->LookupMarker( "MarkerEggLay" );

	m_parameters[0].m_legLift = 3.0f;
	m_parameters[0].m_idealLegSlope = 2.6f;
	m_parameters[0].m_legSwingDuration = 0.4f;
	m_parameters[0].m_delayBetweenLifts = 0.11f;
	m_parameters[0].m_lookAheadCoef = 0.75f;
	m_parameters[0].m_idealSpeed = 10.0f;

	m_parameters[1].m_legLift = 3.0f;
	m_parameters[1].m_idealLegSlope = 2.6f;
	m_parameters[1].m_legSwingDuration = 0.25f;
	m_parameters[1].m_delayBetweenLifts = 0.06f;
	m_parameters[1].m_lookAheadCoef = 0.3f;
	m_parameters[1].m_idealSpeed = 30.0f;

	m_parameters[2].m_legLift = 3.0f;
	m_parameters[2].m_idealLegSlope = 2.6f;
	m_parameters[2].m_legSwingDuration = 0.25f;
	m_parameters[2].m_delayBetweenLifts = 0.04f;
	m_parameters[2].m_lookAheadCoef = 0.2f;
	m_parameters[2].m_idealSpeed = 50.0f;

	// Initialise legs
	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		char markerName[] = "MarkerLegX";
		markerName[strlen(markerName) - 1] = '0' + i;
		m_legs[i] = new EntityLeg(i, this, "spider_leg_upper.shp", "spider_leg_lower.shp", markerName);
		m_legs[i]->m_legLift = m_parameters[2].m_legLift;
		m_legs[i]->m_idealLegSlope = m_parameters[2].m_idealLegSlope;
		m_legs[i]->m_legSwingDuration = m_parameters[2].m_legSwingDuration;
		m_legs[i]->m_lookAheadCoef = m_parameters[2].m_lookAheadCoef;
	}
	m_delayBetweenLifts = m_parameters[2].m_delayBetweenLifts;
}


Spider::~Spider()
{
	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		delete m_legs[i];
	}
}


void Spider::Begin()
{
	Entity::Begin();

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		m_legs[i]->LiftFoot(m_targetHoverHeight);
		m_legs[i]->PlantFoot();
	}

	m_targetPos = m_pos;
}


void Spider::ChangeHealth(int _amount)
{
	bool dead = m_dead;

	if (!m_dead && _amount < -1)
	{
		Entity::ChangeHealth(_amount);

        float fractionDead = 1.0f - (float) m_stats[StatHealth] / (float) EntityBlueprint::GetStat( TypeSpider, StatHealth );
        fractionDead = max( fractionDead, 0.0f );
        fractionDead = min( fractionDead, 1.0f );
		Matrix34 transform( m_front, m_up, m_pos );
		g_explosionManager.AddExplosion( m_shape, transform, fractionDead );
	}
}


int Spider::CalcWhichFootToMove()
{
	if (g_gameTime < m_nextLegMoveTime) return -1;

	float bestScore = 0.0f;
	float bestFoot = -1;

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		if (m_legs[i]->m_foot.m_state == EntityFoot::OnGround)
		{
			float score = m_legs[i]->CalcFootsDesireToMove(m_targetHoverHeight);
			if (score > FOOT_EMERGENCY_THRESHOLD)
			{
				m_legs[i]->LiftFoot(m_targetHoverHeight);
			}
			else if (score > bestScore)
			{
				bestScore = score;
				bestFoot = i;
			}
		}
	}

	if (bestScore > FOOT_MOVE_THRESHOLD)
	{
		m_nextLegMoveTime = g_gameTime + m_delayBetweenLifts;
		return bestFoot;
	}

	return -1;
}


void Spider::StompFoot(Vector3 const &_pos)
{
    for( int p = 0; p < 3; ++p )
    {
        Vector3 vel( syncsfrand( 20.0f ),
                     5.0f + syncfrand( 5.0f ),
                     syncsfrand( 20.0f ) );

        g_app->m_particleSystem->CreateParticle( _pos, vel,
			Particle::TypeMuzzleFlash, 10.0f );
    }


    //
    // Damage everyone nearby

    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( _pos.x, _pos.z,
						FOOT_DAMAGE_RADIUS, &numFound, m_id.GetTeamId() );
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        WorldObject *obj = g_app->m_location->GetEntity( id );
        Entity *entity = (Entity *) obj;

        float distance = (entity->m_pos - _pos).Mag();
        float fraction = (FOOT_DAMAGE_RADIUS - distance) / FOOT_DAMAGE_RADIUS;
        fraction *= (1.0f + syncfrand(0.3f));

        entity->ChangeHealth( FOOT_DAMAGE_STRENGTH * fraction * -1.0f );

        Vector3 push( entity->m_pos - _pos );
        push.Normalise();
        push *= fraction;

        if( entity->m_onGround )
        {
            push.y = fraction * FOOT_DAMAGE_STRENGTH * 0.3f;
        }
        else
        {
            push.y = -1;
        }
        entity->m_vel += push;
        entity->m_onGround = false;
    }
}


void Spider::UpdateLegsPouncing()
{
	float fractionComplete = (g_gameTime - m_pounceStartTime) * 2.0f;
	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		m_legs[i]->AdvanceSpiderPounce(fractionComplete);
	}
}


void Spider::UpdateLegs()
{
	{
		int bestFootToMove = CalcWhichFootToMove();
		if (bestFootToMove != -1)
		{
			m_legs[bestFootToMove]->LiftFoot(m_targetHoverHeight);
		}
		bestFootToMove = CalcWhichFootToMove();
		if (bestFootToMove != -1)
		{
			m_legs[bestFootToMove]->LiftFoot(m_targetHoverHeight);
		}
	}

	// Work out which set of parameters to use
	int stage = 0;
	for (int i = 1; i < 3; ++i)
	{
		float diffBest = fabs(m_vel.Mag() - m_parameters[stage].m_idealSpeed);
		float diffCurrent = fabs(m_vel.Mag() - m_parameters[i].m_idealSpeed);
		if (diffCurrent < diffBest)
		{
			stage = i;
		}
	}

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		// Pass the current parameters through to the legs
		m_legs[i]->m_legLift = m_parameters[stage].m_legLift;
		m_legs[i]->m_idealLegSlope = m_parameters[stage].m_idealLegSlope * 2.5f;
		m_legs[i]->m_legSwingDuration = m_parameters[stage].m_legSwingDuration;
		m_legs[i]->m_lookAheadCoef = m_parameters[stage].m_lookAheadCoef;

		bool footPlanted = m_legs[i]->Advance();
//		if (m_attacking && footPlanted)
//		{
//			Vector3 const &legPos = m_legs[i]->m_foot.m_pos;
//			StompFoot(legPos);
//		}

        if( footPlanted ) g_app->m_soundSystem->TriggerEntityEvent( this, "FootFall" );
	}

	m_delayBetweenLifts = m_parameters[stage].m_delayBetweenLifts;
}


// Tests that the line from m_pos to _dest doesn't go above a certain height
float Spider::IsPathOK(Vector3 const &_dest)
{
	Vector3 toDest(_dest - m_pos);
	float distToDest = toDest.Mag();
	float const sampleSeperation = 8.0f;
	toDest.SetLength(sampleSeperation);

	Vector3 pos(m_pos);
	for (float i = 0.0f; i < distToDest; i += sampleSeperation)
	{
		float height = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
		if (height > MAX_PATH_HEIGHT || height < 0.1f /*Sea level*/)
		{
			float rv = i / distToDest;
			return rv;
		}
		pos += toDest;
	}

	return 1.0f;
}


void Spider::DetectCollisions()
{
	Vector3 pos(m_pos);
	pos += m_vel;
	int numFound;
	WorldObjectId *neighbours = g_app->m_location->m_entityGrid->GetNeighbours(
								pos.x, pos.z, 22.0f, &numFound);

    Vector3 escapeVector;
    bool collisionDetected = false;

    for (int i = 0; i < numFound; ++i)
	{
		Entity *ent = g_app->m_location->GetEntity(neighbours[i]);
		if (ent->m_type == Entity::TypeSpider &&
            ent->m_id != m_id )
		{
		    Entity *entity = g_app->m_location->GetEntity(neighbours[darwiniaRandom() % numFound]);
		    DarwiniaDebugAssert(entity);
		    Vector3 toNeighbour = m_pos - entity->m_pos;
		    toNeighbour.y = 0.0f;
		    toNeighbour.Normalise();
		    escapeVector += toNeighbour;
            collisionDetected = true;
		}
	}

    if( collisionDetected )
    {
        m_targetPos = m_pos + escapeVector * 40.0f;
    }
}


bool Spider::FaceTarget()
{
	Vector3 toTarget = m_targetPos - m_pos;
	toTarget.y = 0.0f;
	float toTargetMag = toTarget.Mag();
	Vector3 toTargetNormalised = toTarget / toTargetMag;

	float dotProd = toTargetNormalised * m_front;
	if (dotProd < 0.999f)
	{
		Vector3 rotation = m_front ^ toTargetNormalised;
		if( dotProd < 0.9f ) rotation.SetLength(TURN_RATE);
        else                 rotation.SetLength(TURN_RATE/2.0f);
		m_front.RotateAround(rotation);
		m_front.Normalise();
        m_vel.Zero();

        return false;
	}

    return true;
}


bool Spider::AdvanceToTarget()
{
	Vector3 toTarget = m_targetPos - m_pos;
	toTarget.y = 0.0f;
	float toTargetMag = toTarget.Mag();


	//
	// Have we reached our target

	if (toTargetMag < 5.0f)
	{
		return true;
	}

    bool facingTarget = FaceTarget();

	Vector3 toTargetNormalised = toTarget / toTargetMag;
	float dotProd = toTargetNormalised * m_front;

    if( dotProd > 0.5f)
    {
        m_vel = m_front;
        if( toTargetMag < 30.0f )
        {
            m_vel.SetLength( toTargetMag );
        }
        else
        {
            m_vel.SetLength( 30.0f );
        }
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
	}

    DetectCollisions();

	return false;
}


bool Spider::AdvanceIdle()
{
    //
    // Time to do something new?

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0f )
    {
        m_retargetTimer = 5.0f;
        bool foundNewTarget = false;
        if( !foundNewTarget ) foundNewTarget = SearchForEnemies();
        if( !foundNewTarget ) foundNewTarget = SearchForSpirits();
        if( !foundNewTarget ) foundNewTarget = SearchForRandomPos();

        if( foundNewTarget ) return false;
    }


    //
    // Just wander around a bit

	bool arrived = AdvanceToTarget();
    if( arrived ) m_vel.Zero();

	return false;
}


bool Spider::AdvanceAttack()
{
    m_targetPos = m_pounceTarget;
    bool facingTarget = FaceTarget();

    if( facingTarget )
    {
        float distance = ( m_pounceTarget - m_pos ).Mag();

        if( distance > 150.0f )
        {
            AdvanceToTarget();
        }
        else
        {
            float force = sqrtf(distance) * 24.0f;
            //if( force > 130.0f ) force = 130.0f;

            Vector3 up = ( m_pounceTarget - m_pos ).Normalise();
            up.y = 0.5f;
            up.Normalise();
            m_vel = up * force;

            m_onGround = false;
            m_retargetTimer = 3.0f;
            m_state = StatePouncing;
			m_pounceStartTime = g_gameTime;

			Vector3 forwards = (m_pounceTarget - m_pos);
			for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
			{
				m_legs[i]->m_foot.m_state = EntityFoot::Pouncing;
				m_legs[i]->m_foot.m_bodyToFoot = m_pos - m_legs[i]->m_foot.m_pos;
				m_legs[i]->m_foot.m_lastGroundPos = m_legs[i]->m_foot.m_pos;
				m_legs[i]->m_foot.m_targetPos = m_legs[i]->m_foot.m_pos + forwards;
			}

            g_app->m_soundSystem->TriggerEntityEvent( this, "Attack" );
            g_app->m_soundSystem->TriggerEntityEvent( this, "Pounce" );
        }
    }

    return false;
}


bool Spider::AdvancePouncing()
{
    m_vel.y -= 40.0f;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( m_pos.y < landHeight + 1.0f )
    {
        m_pos.y = landHeight + 1.0f;
        m_vel.Zero();
        m_onGround = true;
        m_state = StateIdle;
        m_retargetTimer = 6.0f;

		for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
		{
			m_legs[i]->m_foot.m_state = EntityFoot::OnGround;
		}

        g_app->m_soundSystem->TriggerEntityEvent( this, "PounceLand" );

        // Squash people
        float squashRange = 40.0f;
        float damage = 100.0f;
        int numFound;
        WorldObjectId *enemies = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, squashRange, &numFound, m_id.GetTeamId() );
        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = enemies[i];
            Entity *entity = g_app->m_location->GetEntity( id );

            float distance = (entity->m_pos - m_pos).Mag();
            float fraction = (squashRange - distance) / squashRange;
            fraction *= (1.0f + syncfrand(0.3f));

            if( distance < 20.0f ) entity->ChangeHealth( fraction * -damage );

            Vector3 push( m_front );
            push.y = push.Mag() * 4.0f;

            float pushLength = fraction * 30.0f;
            pushLength = min( 20.0f, pushLength );
            push.SetLength( pushLength );

            entity->m_vel += push;
            entity->m_onGround = false;
        }
    }

    return false;
}


bool Spider::SearchForRandomPos()
{
	for (int i = 0; i < 10; ++i)
	{
		m_targetPos = m_spawnPoint;
		m_targetPos.x += syncsfrand(m_roamRange);
		m_targetPos.z += syncsfrand(m_roamRange);

		if (IsPathOK(m_targetPos) > 0.99f)
		{
			break;
		}
	}

	float height = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
	m_targetPos.y = height;
	return true;
}


bool Spider::SearchForEnemies()
{
    float maxRange = ATTACK_SEARCH_MAX_RADIUS;
    float minRange = ATTACK_SEARCH_MIN_RADIUS;

    WorldObjectId targetId = g_app->m_location->m_entityGrid->GetBestEnemy(
							    m_pos.x, m_pos.z, minRange, maxRange, m_id.GetTeamId());

    Entity *entity = g_app->m_location->GetEntity( targetId );

    if( entity && !entity->m_dead )
    {
        m_pounceTarget = entity->m_pos;
        m_state = StateAttack;
        return true;
    }
    else
    {
        return false;
    }
}


bool Spider::SearchForSpirits()
{
	START_PROFILE(g_app->m_profiler, "SearchSpirits");
    Spirit *found = NULL;
    int foundIndex = -1;
    float nearest = 9999.9f;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            if( s->NumNearbyEggs() < 3 && s->m_pos.y > 10 )
            {
                float theDist = ( s->m_pos - m_pos ).Mag();

                if( theDist <= SPIRIT_MAXSEARCHRANGE &&
                    theDist >= SPIRIT_MINSEARCHRANGE &&
                    theDist < nearest &&
                    s->m_state == Spirit::StateFloating )
                {
                    found = s;
                    foundIndex = i;
                    nearest = theDist;
                }
            }
        }
    }

    if( found )
    {
        m_spiritId = foundIndex;
        Vector3 usToThem = ( found->m_pos - m_pos ).Normalise() * 45.0f;
        m_targetPos = found->m_pos + usToThem;
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        m_state = StateEggLaying;
    }

	END_PROFILE(g_app->m_profiler, "SearchSpirits");
    return found;
}


bool Spider::AdvanceEggLaying()
{
    //
    // Time to look around for enemies

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0f )
    {
        m_retargetTimer = 5.0f;
        bool foundNewTarget = false;
        if( !foundNewTarget ) foundNewTarget = SearchForEnemies();
        if( foundNewTarget ) return false;
    }


    //
    // Advance to where we think the egg should go

    bool arrived = AdvanceToTarget();


    //
    // Lay an egg if we are in place

    if( arrived )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        Matrix34 eggLayMat = m_eggLay->GetWorldMatrix(mat);

        g_app->m_location->SpawnEntities( eggLayMat.pos, m_id.GetTeamId(), -1, TypeEgg, 1, g_zeroVector, 0.0f );

        g_app->m_soundSystem->TriggerEntityEvent( this, "LayEgg" );

        m_spiritId = -1;
        m_state = StateIdle;
    }

    return false;
}


bool Spider::Advance(Unit *_unit)
{
    if( !m_dead )
    {
        //
        // Do our action

	    switch (m_state)
	    {
		    case StateIdle:		    AdvanceIdle();		    break;
            case StateEggLaying:    AdvanceEggLaying();     break;
            case StateAttack:       AdvanceAttack();        break;
            case StatePouncing:     AdvancePouncing();      break;
	    }


	    //
	    // Adjust body height

        if( m_state == StatePouncing )
		{
	        UpdateLegsPouncing();
		}
		else
	    {
		    float targetHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
		    targetHeight += m_targetHoverHeight;
		    float factor1 = 1.0f * SERVER_ADVANCE_PERIOD;
		    float factor2 = 1.0f - factor1;
		    m_pos.y = factor1 * targetHeight + factor2 * m_pos.y;
	        UpdateLegs();
	    }
    }

    return Entity::Advance(_unit);
}


void Spider::Render(float _predictionTime)
{
    if( m_dead ) return;

    glDisable( GL_TEXTURE_2D );


	//RenderArrow(m_pos, m_targetPos, 1.0f);

    if( m_state == StateAttack )
    {
        //RenderArrow(m_pos, m_pounceTarget, 1.0f, RGBAColour(255,0,0) );
    }

	g_app->m_renderer->SetObjectLighting();


	//
	// Render body

	Vector3 predictedMovement = _predictionTime * m_vel;
	Vector3 predictedPos = m_pos + predictedMovement;
//	predictedPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(predictedPos.x, predictedPos.z) +
//					 m_targetHoverHeight;

	Vector3 up = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
	Vector3 right = m_up ^ m_front;
	Vector3 front = right ^ up;

	Matrix34 mat(front, up, predictedPos);

    if( m_renderDamaged )
    {
        float timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        if( frand() > 0.5f ) mat.r *= ( 1.0f + sinf(timeIndex) * 0.5f );
        else                 mat.u *= ( 1.0f + sinf(timeIndex) * 0.5f );
        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    }

	m_shape->Render(_predictionTime, mat);

    glDisable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	//
	// Render Legs

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		m_legs[i]->Render(_predictionTime, predictedMovement);
	}

	g_app->m_renderer->UnsetObjectLighting();
}


bool Spider::RenderPixelEffect(float _predictionTime)
{
	Render(_predictionTime);

	Vector3 predictedMovement = _predictionTime * m_vel;
	Vector3 predictedPos = m_pos + predictedMovement;
	Vector3 up = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
	Vector3 right = m_up ^ m_front;
	Vector3 front = right ^ up;

	Matrix34 mat(front, up, predictedPos);
	g_app->m_renderer->MarkUsedCells(m_shape, mat);

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		m_legs[i]->RenderPixelEffect(_predictionTime, predictedMovement);
	}

    return true;
}


bool Spider::IsInView()
{
    return g_app->m_camera->SphereInViewFrustum( m_pos+m_centrePos, m_radius );
}


void Spider::ListSoundEvents(LList<char *> *_list)
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Pounce" );
    _list->PutData( "PounceLand" );
    _list->PutData( "FootFall" );
    _list->PutData( "LayEgg" );
}

