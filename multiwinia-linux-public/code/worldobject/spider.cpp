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
#include "lib/math/random_number.h"

#include "worldobject/entity_leg.h"
#include "worldobject/spider.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "deform.h"
#include "entity_grid.h"
#include "explosion.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "camera.h"


//#define FOOT_MOVE_THRESHOLD	        5.0	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
//#define FOOT_EMERGENCY_THRESHOLD	12.0
//#define FOOT_DAMAGE_RADIUS			20.0	// Size of region damaged by foot falls
//#define FOOT_DAMAGE_STRENGTH		10.0
//
//#define HOVER_HEIGHT				3.0
//
//#define TURN_RATE					0.18
//#define NORMAL_SPEED				300.0
//#define ATTACK_SPEED				90.0
//
//#define MAX_PATH_HEIGHT				100.0
//
//#define ATTACK_SEARCH_MAX_RADIUS	250.0
//#define ATTACK_SEARCH_MIN_RADIUS	100.0
//#define ATTACK_PREAMBLE				5.0	// Minimum time between choosing a target and starting to charge (most of this time will normally be spent turning)

#define FOOT_MOVE_THRESHOLD	        5.0	// Lower means feet are lifted when less distant from their ideal pos, and thus smaller steps are taken
#define FOOT_EMERGENCY_THRESHOLD	12.0
#define FOOT_DAMAGE_RADIUS			20.0	// Size of region damaged by foot falls
#define FOOT_DAMAGE_STRENGTH		10.0

#define HOVER_HEIGHT				3.0

#define TURN_RATE					0.1
#define NORMAL_SPEED				300.0
#define ATTACK_SPEED				90.0

#define MAX_PATH_HEIGHT				100.0

#define ATTACK_SEARCH_MAX_RADIUS	250.0
#define ATTACK_SEARCH_MIN_RADIUS	100.0
#define ATTACK_PREAMBLE				5.0	// Minimum time between choosing a target and starting to charge (most of this time will normally be spent turning)

#define SPIRIT_MAXSEARCHRANGE       100.0
#define SPIRIT_MINSEARCHRANGE       30.0


//*****************************************************************************
// Class Spider
//*****************************************************************************

Spider::Spider()
:	Entity(),
    m_state(StateIdle),
	m_nextLegMoveTime(-1.0),
	m_speed(0.0),
	m_targetHoverHeight(HOVER_HEIGHT),
	m_up(g_upVector),
    m_retargetTimer(0.0),
    m_spiritId(-1),
    m_eggLay(NULL)
{
	//m_shape = g_app->mresource->GetShape("spider.shp");
    //const char eggLayName[] = "MarkerEggLay";
    //m_eggLay = m_shape->m_rootFragment->LookupMarker( eggLayName );
    //AppReleaseAssert( m_eggLay, "Spider: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", eggLayName, m_shape->m_name );

	m_parameters[0].m_legLift = 3.0;
	m_parameters[0].m_idealLegSlope = 2.6;
	m_parameters[0].m_legSwingDuration = 0.4;
	m_parameters[0].m_delayBetweenLifts = 0.11;
	m_parameters[0].m_lookAheadCoef = 0.75;
	m_parameters[0].m_idealSpeed = 10.0;

	m_parameters[1].m_legLift = 3.0;
	m_parameters[1].m_idealLegSlope = 2.6;
	m_parameters[1].m_legSwingDuration = 0.25;
	m_parameters[1].m_delayBetweenLifts = 0.06;
	m_parameters[1].m_lookAheadCoef = 0.3;
	m_parameters[1].m_idealSpeed = 30.0;

	m_parameters[2].m_legLift = 3.0;
	m_parameters[2].m_idealLegSlope = 2.6;
	m_parameters[2].m_legSwingDuration = 0.25;
	m_parameters[2].m_delayBetweenLifts = 0.04;
	m_parameters[2].m_lookAheadCoef = 0.2;
	m_parameters[2].m_idealSpeed = 50.0;
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

    SetShape("spider.shp", true);
    const char eggLayName[] = "MarkerEggLay";
    m_eggLay = m_shape->m_rootFragment->LookupMarker( eggLayName );
    AppReleaseAssert( m_eggLay, "Spider: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", eggLayName, m_shape->m_name );

    
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
	
	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		m_legs[i]->LiftFoot(m_targetHoverHeight);
		m_legs[i]->PlantFoot();
	}

	m_targetPos = m_pos;
}


bool Spider::ChangeHealth(int _amount, int _damageType)
{
    if( _damageType == DamageTypeLaser ) return false;
	bool dead = m_dead;

	if (!m_dead && _amount < -1)
	{
		Entity::ChangeHealth(_amount);

        double fractionDead = 1.0 - (double) m_stats[StatHealth] / (double) EntityBlueprint::GetStat( TypeSpider, StatHealth );
        fractionDead = max( fractionDead, 0.0 );
        fractionDead = min( fractionDead, 1.0 );
		Matrix34 transform( m_front, m_up, m_pos );
		g_explosionManager.AddExplosion( m_shape, transform, fractionDead );
	}

    return true;
}


int Spider::CalcWhichFootToMove()
{
	if (GetNetworkTime() < m_nextLegMoveTime) return -1;
        
	double bestScore = 0.0;
	double bestFoot = -1;

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		if (m_legs[i]->m_foot.m_state == EntityFoot::OnGround)
		{
			double score = m_legs[i]->CalcFootsDesireToMove(m_targetHoverHeight);
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
		m_nextLegMoveTime = GetNetworkTime() + m_delayBetweenLifts;
		return bestFoot;
	}

	return -1;
}


void Spider::StompFoot(Vector3 const &_pos)
{
    //
    // Damage everyone nearby

    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, _pos.x, _pos.z, 
						FOOT_DAMAGE_RADIUS, &numFound, m_id.GetTeamId() );
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        WorldObject *obj = g_app->m_location->GetEntity( id );
        Entity *entity = (Entity *) obj;

        double distance = (entity->m_pos - _pos).Mag();
        double fraction = (FOOT_DAMAGE_RADIUS - distance) / FOOT_DAMAGE_RADIUS;
        fraction *= (1.0 + syncfrand(0.3));

        entity->ChangeHealth( FOOT_DAMAGE_STRENGTH * fraction * -1.0 );

        Vector3 push( entity->m_pos - _pos );
        push.Normalise();
        push *= fraction;

        if( entity->m_onGround )
        {
            push.y = fraction * FOOT_DAMAGE_STRENGTH * 0.3;
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
	double fractionComplete = (GetNetworkTime() - m_pounceStartTime) * 2.0;
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
		double diffBest = iv_abs(m_vel.Mag() - m_parameters[stage].m_idealSpeed);
		double diffCurrent = iv_abs(m_vel.Mag() - m_parameters[i].m_idealSpeed);
		if (diffCurrent < diffBest)
		{
			stage = i;
		}
	}

	for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
	{
		// Pass the current parameters through to the legs
		m_legs[i]->m_legLift = m_parameters[stage].m_legLift;
		m_legs[i]->m_idealLegSlope = m_parameters[stage].m_idealLegSlope * 2.5;
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
double Spider::IsPathOK(Vector3 const &_dest)
{
	Vector3 toDest(_dest - m_pos);
	double distToDest = toDest.Mag();
	double const sampleSeperation = 8.0;
	toDest.SetLength(sampleSeperation);
	
	Vector3 pos(m_pos);
	for (double i = 0.0; i < distToDest; i += sampleSeperation)
	{
		double height = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
		if (height > MAX_PATH_HEIGHT || height < 0.1 /*Sea level*/)
		{
			double rv = i / distToDest;
			return rv;
		}
		pos += toDest;
	}

	return 1.0;
}


void Spider::DetectCollisions()
{
	Vector3 pos(m_pos);
	pos += m_vel;
	int numFound;
	g_app->m_location->m_entityGrid->GetNeighbours(s_neighbours,
								pos.x, pos.z, 22.0, &numFound);

    Vector3 escapeVector;
    bool collisionDetected = false;

    for (int i = 0; i < numFound; ++i)
	{
		Entity *ent = g_app->m_location->GetEntity(s_neighbours[i]);
		if (ent->m_type == Entity::TypeSpider &&
            ent->m_id != m_id )
		{
            int randomNeighbourIndex = syncrand() % numFound;
		    Entity *entity = g_app->m_location->GetEntity(s_neighbours[randomNeighbourIndex]);
		    AppDebugAssert(entity);
		    Vector3 toNeighbour = m_pos - entity->m_pos;
		    toNeighbour.y = 0.0;
		    toNeighbour.Normalise();
		    escapeVector += toNeighbour;			
            collisionDetected = true;
		}
	}

    if( collisionDetected )
    {
        m_targetPos = m_pos + escapeVector * 40.0;
    }
}


bool Spider::FaceTarget()
{
	Vector3 toTarget = m_targetPos - m_pos;
	toTarget.y = 0.0;
	double toTargetMag = toTarget.Mag();
	Vector3 toTargetNormalised = toTarget / toTargetMag;

	double dotProd = toTargetNormalised * m_front;
	if (dotProd < 0.999)
	{
		Vector3 rotation = m_front ^ toTargetNormalised;
		if( dotProd < 0.9 ) rotation.SetLength(TURN_RATE);
        else                 rotation.SetLength(TURN_RATE/2.0);
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
	toTarget.y = 0.0;
	double toTargetMag = toTarget.Mag();


	//
	// Have we reached our target

	if (toTargetMag < 5.0)
	{
		return true;
	}
	
    bool facingTarget = FaceTarget();

	Vector3 toTargetNormalised = toTarget / toTargetMag;
	double dotProd = toTargetNormalised * m_front;
    
    if( dotProd > 0.5)
    {
        m_vel = m_front;
        if( toTargetMag < 30.0 )
        {
            m_vel.SetLength( toTargetMag );
        }
        else
        {
            m_vel.SetLength( 30.0 );
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
    if( m_retargetTimer <= 0.0 )
    {
        m_retargetTimer = 5.0;
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
        double distance = ( m_pounceTarget - m_pos ).Mag();

        if( distance > 150.0 )
        {
            AdvanceToTarget();
        }
        else
        {
            double force = iv_sqrt(distance) * 24.0;
            //if( force > 130.0 ) force = 130.0;

            Vector3 up = ( m_pounceTarget - m_pos ).Normalise();
            up.y = 0.5;
            up.Normalise();
            m_vel = up * force;

            m_onGround = false;
            m_retargetTimer = 3.0;
            m_state = StatePouncing;
			m_pounceStartTime = GetNetworkTime();

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
#ifdef USE_DIRECT3D
	Vector3 oldPos = m_pos;
#endif

	m_vel.y -= 40.0;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
    if( m_pos.y < landHeight + 1.0 )
    {
        m_pos.y = landHeight + 1.0;
        m_vel.Zero();
        m_onGround = true;
        m_state = StateIdle;
        m_retargetTimer = 6.0;

		for (int i = 0; i < SPIDER_NUM_LEGS; ++i)
		{
			m_legs[i]->m_foot.m_state = EntityFoot::OnGround;
		}

        g_app->m_soundSystem->TriggerEntityEvent( this, "PounceLand" );

        // Squash people
        double squashRange = 40.0;
        double damage = 100.0;
        int numFound;
        g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, squashRange, &numFound, m_id.GetTeamId() );

		//char buf1[32];
		//SyncRandLog( "Spider squashRange = [%s], numFound = %d", HashDouble( squashRange, buf1 ), numFound );

        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = s_neighbours[i];
            Entity *entity = g_app->m_location->GetEntity( id );

            double distance = (entity->m_pos - m_pos).Mag();
            double fraction = (squashRange - distance) / squashRange;
			
			//SyncRandLog( "Spider Push: fraction0 = [%s]", HashDouble( fraction, buf1 ) );

			// Ordering of operations important here for error control
			volatile double randomFactor = 1.0;
			randomFactor += syncfrand(0.3);

            fraction *= randomFactor;

			//SyncRandLog( "Spider Push: fraction1 = [%s]", HashDouble( fraction, buf1 ) );


            if( distance < 20.0 ) entity->ChangeHealth( fraction * -damage );

            Vector3 push( m_front );
            push.y = push.Mag() * 5.0;

            double pushLength = fraction * 30.0;
            pushLength = min( 20.0, pushLength );

			//char buf2[32], buf3[64], buf4[32];
			//SyncRandLog( "Spider Push: distance = [%s], fraction = [%s], push0 = [%s], pushLength = [%s]", 
			//	HashDouble( distance, buf1 ),
			//	HashDouble( fraction, buf2 ),
			//	HashVector3( push, buf3 ),
			//	HashDouble( pushLength, buf4 ) );
				
            push.SetLength( pushLength );

			//SyncRandLog( "Spider Push: push1 = [%s]", HashVector3( push, buf3 ) );

            entity->m_vel += push;
            entity->m_onGround = false;        
        }
    }

#ifdef USE_DIRECT3D
	if(g_deformEffect)
	{
		Vector3 newPos = m_pos;
		g_deformEffect->AddTearingPath(oldPos,newPos,0.25);
	}
#endif

	return false;
}


bool Spider::SearchForRandomPos()
{
	for (int i = 0; i < 10; ++i)
	{
		m_targetPos = m_spawnPoint;
		m_targetPos.x += syncsfrand(m_roamRange);
		m_targetPos.z += syncsfrand(m_roamRange);

		if (IsPathOK(m_targetPos) > 0.99)
		{
			break;
		}
	}

	double height = g_app->m_location->m_landscape.m_heightMap->GetValue(m_targetPos.x, m_targetPos.z);
	m_targetPos.y = height;
	return true;
}


bool Spider::SearchForEnemies()
{
    double maxRange = ATTACK_SEARCH_MAX_RADIUS;
    double minRange = ATTACK_SEARCH_MIN_RADIUS;

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
	START_PROFILE( "SearchSpirits");
    Spirit *found = NULL;
    int foundIndex = -1;
    double nearest = 9999.9;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            if( s->NumNearbyEggs() < 3 && s->m_pos.y > 10 )
            {
                double theDist = ( s->m_pos - m_pos ).Mag();

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
        Vector3 usToThem = ( found->m_pos - m_pos ).Normalise() * 45.0;
        m_targetPos = found->m_pos + usToThem;
        m_targetPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
        m_state = StateEggLaying;
    }    

	END_PROFILE( "SearchSpirits");
    return found;
}


bool Spider::AdvanceEggLaying()
{
    //
    // Time to look around for enemies

    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0 )
    {
        m_retargetTimer = 5.0;
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
        
        g_app->m_location->SpawnEntities( eggLayMat.pos, m_id.GetTeamId(), -1, TypeEgg, 1, g_zeroVector, 0.0 );

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
		    double targetHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
		    targetHeight += m_targetHoverHeight;
		    double factor1 = 1.0 * SERVER_ADVANCE_PERIOD;
		    double factor2 = 1.0 - factor1;
		    m_pos.y = factor1 * targetHeight + factor2 * m_pos.y;
	        UpdateLegs();
	    }
    }
 
    return Entity::Advance(_unit);
}


void Spider::Render(double _predictionTime)
{
    if( m_dead ) return;

    glDisable( GL_TEXTURE_2D );


	//RenderArrow(m_pos, m_targetPos, 1.0);
    
    if( m_state == StateAttack )
    {
        //RenderArrow(m_pos, m_pounceTarget, 1.0, RGBAColour(255,0,0) );
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
        double timeIndex = g_gameTime + m_id.GetUniqueId() * 10;
        if( frand() > 0.5 ) mat.r *= ( 1.0 + iv_sin(timeIndex) * 0.5 );
        else                 mat.u *= ( 1.0 + iv_sin(timeIndex) * 0.5 );    
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


bool Spider::RenderPixelEffect(double _predictionTime)
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

