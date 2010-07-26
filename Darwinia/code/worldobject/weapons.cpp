#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/vector2.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"

#include "worldobject/weapons.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/airstrike.h"
#include "worldobject/darwinian.h"
#include "worldobject/armour.h"


#include "explosion.h"
#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "obstruction_grid.h"
#include "location.h"
#include "main.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "global_world.h"
#include "helpsystem.h"
#include "taskmanager.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_3d.h"


// ****************************************************************************
//  Class ThrowableWeapon
// ****************************************************************************

ThrowableWeapon::ThrowableWeapon( int _type, Vector3 const &_startPos, Vector3 const &_front, float _force )
:   m_shape(NULL),
    m_force(1.0f),
    m_numFlashes(0)
{
    m_shape = g_app->m_resource->GetShape( "throwable.shp" );
    m_pos = _startPos;
    m_vel = _front * _force;

    m_up = _front;
    m_front = g_upVector;
    Vector3 right = m_front ^ m_up;
    m_front = right ^ m_up;

    m_birthTime = GetHighResTime();

    m_type = _type;
}


void ThrowableWeapon::Initialise ()
{
    TriggerSoundEvent( "Create" );
}


void ThrowableWeapon::TriggerSoundEvent( char *_event )
{
    switch( m_type )
    {
        case EffectThrowableGrenade:
        case EffectThrowableAirstrikeMarker:
        case EffectThrowableControllerGrenade:
            g_app->m_soundSystem->TriggerOtherEvent( this, _event, SoundSourceBlueprint::TypeGrenade );
            break;

        case EffectThrowableAirstrikeBomb:
            g_app->m_soundSystem->TriggerOtherEvent( this, _event, SoundSourceBlueprint::TypeAirstrikeBomb );
            break;
    }
}


bool ThrowableWeapon::Advance()
{
	if (m_type != EffectThrowableAirstrikeMarker && syncfrand() > 0.2f)
	{
        Vector3 vel( m_vel / 4.0f );
        vel.x += syncsfrand(5.0f);
        vel.y += syncsfrand(5.0f);
        vel.z += syncsfrand(5.0f);
		g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeRocketTrail, 40.0f);
	}

    if( m_force > 0.1f )
    {
        m_vel.y -= 9.8f;
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 1.0f )
        {
            BounceOffLandscape();
            m_force *= 0.75f;
            m_vel *= m_force;
            if( m_pos.y < landHeight + 1.0f ) m_pos.y = landHeight + 1.0f;
            TriggerSoundEvent( "Bounce" );
        }
    }

    return false;
}


void ThrowableWeapon::Render( float _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;

    Vector3 right = m_up ^ m_front;
    m_up.RotateAround( right * m_force * m_force * 0.15f );
    m_front = right ^ m_up;
    m_front.Normalise();
    m_up.Normalise();

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Matrix34 transform( m_front, m_up, predictedPos );

  	g_app->m_renderer->SetObjectLighting();
    glEnable        (GL_CULL_FACE);
    glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_COLOR_MATERIAL);
    glDisable       (GL_BLEND);

    m_shape->Render( _predictionTime, transform );

    glEnable        (GL_BLEND);
    glDisable       (GL_COLOR_MATERIAL);
    g_app->m_renderer->UnsetObjectLighting();


    int numFlashes = int( GetHighResTime() - m_birthTime );
    if( numFlashes > m_numFlashes )
    {
        TriggerSoundEvent( "Flash" );
        m_numFlashes = numFlashes;
    }

    float flashAlpha = 1.0f - (( GetHighResTime() - m_birthTime ) - numFlashes);
    if( flashAlpha < 0.2f )
    {
        float distToThrowable = (g_app->m_camera->GetPos() - predictedPos).Mag();

        float size = 1000.0f / sqrtf(distToThrowable);
        glColor4ub      ( m_colour.r, m_colour.g, m_colour.b, 200 );
        glEnable        (GL_TEXTURE_2D);
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDisable       (GL_CULL_FACE);
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();
        size *= 0.4f;
        glColor4f       ( 1.0f, 1.0f, 1.0f, 0.7f );
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );       glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 0 );       glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();
        glDisable       (GL_TEXTURE_2D);
        glEnable        (GL_CULL_FACE);
    }
}

float ThrowableWeapon::GetMaxForce( int _researchLevel )
{
	switch( _researchLevel )
    {
        case 0 :    return 50.0f;
        case 1 :    return 80.0f;
        case 2 :    return 110.0f;
        case 3 :    return 140.0f;
        case 4 :    return 170.0f;
		default:	return 0.0f;
    }
}

float ThrowableWeapon::GetApproxMaxRange( float _maxForce )
{
	return _maxForce + ( _maxForce * 0.75f ) + ( _maxForce * 0.75f * 0.75f );
}

// ****************************************************************************
//  Class Grenade
// ****************************************************************************

Grenade::Grenade( Vector3 const &_startPos, Vector3 const &_front, float _force )
:   ThrowableWeapon( EffectThrowableGrenade, _startPos, _front, _force ),
    m_life(3.0f),
    m_power(25.0f)
{
    m_colour.Set( 255, 100, 100 );
}


bool Grenade::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;

    if( m_life <= 0.0f )
    {
        TriggerSoundEvent( "Explode" );

        g_app->m_location->Bang( m_pos, m_power/2.0f, m_power*2.0f);
        return true;
    }

    return ThrowableWeapon::Advance();
}


// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************


AirStrikeMarker::AirStrikeMarker( Vector3 const &_startPos, Vector3 const &_front, float _force )
:   ThrowableWeapon( EffectThrowableAirstrikeMarker, _startPos, _front, _force )
{
    m_colour.Set( 255, 100, 100 );
}


bool AirStrikeMarker::Advance()
{
    ThrowableWeapon::Advance();


    //
    // Dump loads of smoke for the bombers to see

    Vector3 vel( m_vel / 4.0f );
    vel.x += syncsfrand(5.0f);
    vel.y += syncsfrand(5.0f);
    vel.z += syncsfrand(5.0f);
	g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeFire, 100.0f);


    //
    // If its time, summon the actual airstrike

    if( GetHighResTime() > m_birthTime + 2.0f )
    {
        if( m_airstrikeUnit.GetTeamId() != -1 &&
            m_airstrikeUnit.GetUnitId() != -1 )
        {
            // Air strike unit has been created
            Unit *unit = g_app->m_location->GetUnit( m_airstrikeUnit );
            if( !unit )
            {
                m_airstrikeUnit.SetInvalid();
                return true;
            }
            AirstrikeUnit *airStrikeUnit = (AirstrikeUnit *) unit;
            if( airStrikeUnit->m_state == AirstrikeUnit::StateLeaving )
            {
                m_airstrikeUnit.SetInvalid();
                return true;
            }
        }
        else
        {
            // Summon an air strike now
            int teamId = g_app->m_globalWorld->m_myTeamId;
            int unitId;
            Team *team = g_app->m_location->GetMyTeam();

            int airStikeResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeAirStrike );
            AirstrikeUnit *unit = (AirstrikeUnit *) team->NewUnit( Entity::TypeSpaceInvader, airStikeResearch, &unitId, m_pos );
            unit->m_effectId = m_id.GetIndex();
            m_airstrikeUnit.Set( teamId, unitId, -1, -1 );
        }
    }

    return false;
}


// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

ControllerGrenade::ControllerGrenade( Vector3 const &_startPos, Vector3 const &_front, float _force )
:   ThrowableWeapon( EffectThrowableControllerGrenade, _startPos, _front, _force )
{
    m_colour.Set( 100, 100, 255 );
}


bool ControllerGrenade::Advance()
{
    ThrowableWeapon::Advance();

    if( GetHighResTime() > m_birthTime + 3.0f )
    {
        g_app->m_soundSystem->TriggerOtherEvent( this, "ExplodeController", SoundSourceBlueprint::TypeGrenade );

        int numFlashes = 5 + darwiniaRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0f), frand(35.0f), sfrand(15.0f) );
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash, 100.0f );
        }

        Task *currentTask = g_app->m_taskManager->GetCurrentTask();
        if( currentTask && currentTask->m_type == GlobalResearch::TypeSquad )
        {
            Unit *owner = g_app->m_location->GetUnit( currentTask->m_objId );
            if( owner && owner->m_troopType == Entity::TypeInsertionSquadie )
            {
                InsertionSquad *squad = (InsertionSquad *) owner;

                int numFound;
                WorldObjectId *ids = g_app->m_location->m_entityGrid->GetFriends( m_pos.x, m_pos.z, 50.0f, &numFound, m_id.GetTeamId() );
                for( int i = 0; i < numFound; ++i )
                {
                    WorldObjectId id = ids[i];
                    Entity *entity = g_app->m_location->GetEntity( id );
                    if( entity && entity->m_type == Entity::TypeDarwinian )
                    {
                        Darwinian *darwinian = (Darwinian *) entity;
                        darwinian->TakeControl( squad->m_controllerId );
                    }
                }
            }
        }

        return true;
    }

    return false;
}


// ****************************************************************************
//  Class Rocket
// ****************************************************************************

Rocket::Rocket(Vector3 _startPos, Vector3 _targetPos)
:   m_target(_targetPos),
    m_fromTeamId(255)
{
    m_pos = _startPos + Vector3(0,2,0);
    m_vel = ( _targetPos - m_pos ).Normalise() * 50.0f;

    m_shape = g_app->m_resource->GetShape( "throwable.shp" );

    m_timer = GetHighResTime();
    m_type = EffectRocket;
}


void Rocket::Initialise()
{
    g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeRocket );
}


bool Rocket::Advance()
{
    //
    // Move a little bit

    if( m_vel.Mag() < 150.0f )
    {
        m_vel *= 1.2f;
    }

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;


    //
    // Create smoke trail

	if (syncfrand() > 0.2f)
	{
        Vector3 vel( m_vel / 10.0f );
        vel.x += syncsfrand(4.0f);
        vel.y += syncsfrand(4.0f);
        vel.z += syncsfrand(4.0f);
        g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeFire);
	}


    //
    // Have we run out of steam?

    int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeRocket );
    float maxLife = 0.0f;
    switch( rocketResearch )
    {
        case 0 :        maxLife = 0.25f;        break;
        case 1 :        maxLife = 0.5f;         break;
        case 2 :        maxLife = 0.75f;        break;
        case 3 :        maxLife = 1.0f;         break;
        case 4 :        maxLife = 1.5f;         break;
    }

    if( GetHighResTime() > m_timer + maxLife )
    {
        g_app->m_location->Bang( m_pos, 15.0f, 25.0f );
        g_app->m_soundSystem->StopAllSounds( m_id, "Rocket Create" );
        g_app->m_soundSystem->TriggerOtherEvent( this, "Explode", SoundSourceBlueprint::TypeRocket );
        return true;
    }


    //
    // Have we hit the ground?

    if (g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) >= m_pos.y)
    {
        g_app->m_location->Bang( m_pos, 15.0f, 25.0f );
        g_app->m_soundSystem->TriggerOtherEvent( this, "Explode", SoundSourceBlueprint::TypeRocket );
        return true;
    }


    //
    // Have we hit any buildings?

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building->DoesSphereHit( m_pos, 3.0f ) )
        {
            g_app->m_location->Bang( m_pos, 15.0f, 25.0f );
            g_app->m_soundSystem->TriggerOtherEvent( this, "Explode", SoundSourceBlueprint::TypeRocket );
            return true;
        }
    }

    return false;
}


void Rocket::Render( float predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;

    Vector3 front = m_vel;
    front.Normalise();
    Vector3 right = front ^ g_upVector;
    Vector3 up = right ^ front;

    Matrix34 transform( front, up, predictedPos );

  	g_app->m_renderer->SetObjectLighting();
    glEnable        (GL_CULL_FACE);
    glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_COLOR_MATERIAL);
    glDisable       (GL_BLEND);

    m_shape->Render( predictionTime, transform );

    glEnable        (GL_BLEND);
    glDisable       (GL_COLOR_MATERIAL);
    g_app->m_renderer->UnsetObjectLighting();

    for( int i = 0; i < 5; ++i )
    {
        Vector3 vel( m_vel / -20.0f );
        vel.x += sfrand(8.0f);
        vel.y += sfrand(8.0f);
        vel.z += sfrand(8.0f);
        Vector3 pos = predictedPos - m_vel * (0.05f+frand(0.05f));
        pos.x += sfrand(8.0f);
        pos.y += sfrand(8.0f);
        pos.z += sfrand(8.0f);
        float size = 50.0f + frand(50.0f);
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
    }
}



// ****************************************************************************
//  Class Laser
// ****************************************************************************

void Laser::Initialise(float _lifeTime)
{
    m_life = _lifeTime;
    m_harmless = false;
    m_bounced = false;

    g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeLaser );
}


bool Laser::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0f )
    {
        return true;
    }

    Vector3 oldPos = m_pos;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    //
    // Detect collisions with landscape / buildings / people

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if (m_pos.y <= landHeight )
    {
        if( m_bounced )
        {
            // Second richochet, so die
            return true;
        }
        else
        {
            // Richochet
            Vector3 hitPoint;
            Vector3 vel = m_vel;
            vel.Normalise();
            g_app->m_location->m_landscape.RayHit( oldPos, vel, &hitPoint );
            float distanceTravelled = (hitPoint - oldPos).Mag();
            float distanceTotal = (m_vel * SERVER_ADVANCE_PERIOD).Mag();

            Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(hitPoint.x, hitPoint.z);
            Vector3 incomingVel = m_vel * -1.0f;
            float dotProd = normal * incomingVel;
            m_vel = 2.0f * dotProd * normal - incomingVel;

            m_pos = hitPoint;
            Vector3 distanceRemaining = m_vel;
            distanceRemaining.SetLength( distanceTotal - distanceTravelled );

            m_pos += distanceRemaining;
            g_app->m_soundSystem->TriggerOtherEvent( this, "Richochet", SoundSourceBlueprint::TypeLaser );

            m_bounced = true;
        }
    }
    else if ( m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() ||
              m_pos.z < 0 || m_pos.z > g_app->m_location->m_landscape.GetWorldSizeZ() )
    {
        // Outside game world
        return true;
    }
    else
    {
        //
        // Detect collisions with buildings

        Vector3 rayDir = m_vel;
        rayDir.Normalise();
        Vector3 hitPos(0,0,0);
        Vector3 hitNorm(0,0,0);

        LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
        for( int i = 0; i < nearbyBuildings->Size(); ++i )
        {
            Building *building = g_app->m_location->GetBuilding( nearbyBuildings->GetData(i) );
            if( building->DoesRayHit( m_pos, rayDir, (m_vel * SERVER_ADVANCE_PERIOD).Mag(), &hitPos, &hitNorm ) )
            {
                Vector3 vel( -m_vel/15.0f );
                vel.x += sfrand(10.0f);
                vel.y += sfrand(10.0f);
                vel.z += sfrand(10.0f);
                g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeRocketTrail );
                g_app->m_soundSystem->TriggerOtherEvent( this, "HitBuilding", SoundSourceBlueprint::TypeLaser );
                return true;
            }
        }


        //
        // Detect collisions with entities

        if( !m_harmless )
        {
		    Vector3 halfDelta = m_vel * (SERVER_ADVANCE_PERIOD * 0.5f);
		    Vector3 rayStart = m_pos - halfDelta;
		    Vector3 rayEnd = m_pos + halfDelta;
            int numFound;
            float maxRadius = halfDelta.Mag() * 2.0f;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, maxRadius, &numFound, m_fromTeamId );
            for (int i = 0; i < numFound; ++i)
            {
                WorldObjectId id = ids[i];
                WorldObject *wobj = g_app->m_location->GetEntity(id);
			    Entity *entity = (Entity *) wobj;

			    if( PointSegDist2D(Vector2(entity->m_pos), Vector2(rayStart), Vector2(rayEnd)) < 10.0f )
                {
                    g_app->m_soundSystem->TriggerOtherEvent( this, "HitEntity", SoundSourceBlueprint::TypeLaser );
                    if( entity->m_type == Entity::TypeSpider ||
                        entity->m_type == Entity::TypeSporeGenerator ||
                        entity->m_type == Entity::TypeEngineer ||
                        entity->m_type == Entity::TypeTriffidEgg ||
                        entity->m_type == Entity::TypeSoulDestroyer ||
                        entity->m_type == Entity::TypeArmour )
                    {
						//entity->ChangeHealth(-1);
					}
					else
					{
						float damage = 20.0f + syncfrand(20.0f);
						entity->ChangeHealth( (int)-damage );
                        Vector3 push( m_vel );
                        push.SetLength(syncfrand(10.0f));
                        push.y = 7.0f + syncfrand(5.0f);
                        if( entity->m_type != Entity::TypeInsertionSquadie &&
                            entity->m_type != Entity::TypeVirii )
                        {
                            entity->m_front = push;
                            entity->m_front.Normalise();
                        }
                        if( entity->m_type != Entity::TypeVirii )
                        {
                            entity->m_vel += push;
                        }
                        //entity->m_onGround = false;
                        m_harmless = true;
                    }
                    return false;
                }
            }
        }
    }

    return false;
}


void Laser::Render( float predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;

    //
    // No richochet occurred recently
	Vector3 lengthVector = m_vel;
	lengthVector.SetLength(10.0f);
    Vector3 fromPos = predictedPos;
    Vector3 toPos = predictedPos - lengthVector;

    Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
    Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
    float   camDistSqd      = camToMidPoint.MagSquared();
    Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

    rightAngle *= 0.8f;

    RGBAColour const &colour = g_app->m_location->m_teams[ m_fromTeamId ].m_colour;
    glColor4ub( colour.r, colour.g, colour.b, 255 );

    glBegin( GL_QUADS );
        for( int i = 0; i < 5; ++i )
        {
            glTexCoord2i(0,0);      glVertex3fv( (fromPos - rightAngle).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (fromPos + rightAngle).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (toPos + rightAngle).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (toPos - rightAngle).GetData() );
        }
    glEnd();

    if( camDistSqd < 200.0f )
    {
        g_app->m_camera->CreateCameraShake( 0.5f );
    }
}


// ****************************************************************************
// Class Shockwave
// ****************************************************************************

Shockwave::Shockwave( int _teamId, float _size )
:   m_teamId(_teamId),
    m_size(_size),
    m_life(_size),
    m_shape(NULL)
{
//    m_shape = g_app->m_resource->GetShape( "shockwave.shp" );
    m_type = EffectShockwave;
}


bool Shockwave::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0f )
    {
        return true;
    }

    float radius = 35.0f + 40.0f * (m_size - m_life);
    int numFound;
    WorldObjectId *ids;
    if( m_teamId != 255 )
    {
        ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, radius, &numFound, m_teamId );
    }
    else
    {
        ids = g_app->m_location->m_entityGrid->GetNeighbours( m_pos.x, m_pos.z, radius, &numFound );
    }

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        WorldObject *wobj = g_app->m_location->GetEntity( id );
        Entity *ent = (Entity *) wobj;
        float distance = ( ent->m_pos - m_pos ).Mag();
        if( fabs(distance - radius) < 10.0f )
        {
            float fraction = (m_life / m_size);
            if( ent->m_onGround )
            {
                Vector3 push( ent->m_pos - m_pos );
                push.Normalise();
                push.y = 3.0f;
                push.SetLength( 25.0f * fraction );
                ent->m_vel += push;
                ent->m_onGround = false;
            }
			if( ent->m_type == Entity::TypeDarwinian )
			{
				if( syncfrand() < 0.1f )
				{
					((Darwinian *)ent)->SetFire();
				}
				else
				{
					ent->ChangeHealth( -40 * fraction );
				}
			}
			else
			{
				ent->ChangeHealth( -40 * fraction );
			}
        }

    }

    return false;
}

void Shockwave::Render( float predictionTime )
{
    glShadeModel    ( GL_SMOOTH );
    glDisable       ( GL_DEPTH_TEST );

    glColor4f( 1.0f, 1.0f, 0.0f, 1.0f );
    int numSteps = 50.0f;
    float predictedLife = m_life - predictionTime;
    float radius = 35.0f + 40.0f * (m_size - predictedLife);
    float alpha = 0.6f * predictedLife / m_size;

    glBegin( GL_TRIANGLE_FAN );
    glColor4f( 1.0f, 1.0f, 0.0f, 0.0f );
    glVertex3fv( (m_pos+Vector3(0,5,0)).GetData() );
    glColor4f( 1.0f, 0.5f, 0.5f, alpha );

    float angle = 0.0f;
    for( int i = 0; i <= numSteps; ++i )
    {
        float xDiff = radius * sinf(angle);
        float zDiff = radius * cosf(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        glVertex3fv( pos.GetData() );
        angle += 2.0f * M_PI / (float) numSteps;
    }

    glEnd();

    glShadeModel( GL_FLAT );
    glEnable    ( GL_DEPTH_TEST );

    //
    // Render screen flash if we are new

    if( m_size - predictedLife < 0.1f )
    {
        if( g_app->m_camera->PosInViewFrustum( m_pos ) )
        {
			float distance = ( g_app->m_camera->GetPos() - m_pos ).Mag();
            float distanceFactor = 1.0f - ( distance / 500.0f );
            if( distanceFactor < 0.0f ) distanceFactor = 0.0f;

            float alpha = 1.0f - ( m_size - predictedLife ) / 0.1f;
            alpha *= distanceFactor;
			glColor4f(1,1,1,alpha);
			g_app->m_renderer->SetupMatricesFor2D();
            int screenW = g_app->m_renderer->ScreenW();
            int screenH = g_app->m_renderer->ScreenH();
            glDisable( GL_CULL_FACE );
            glBegin( GL_QUADS );
                glVertex2i( 0, 0 );
                glVertex2i( screenW, 0 );
                glVertex2i( screenW, screenH );
                glVertex2i( 0, screenH );
            glEnd();
            glEnable( GL_CULL_FACE );
            g_app->m_renderer->SetupMatricesFor3D();

            g_app->m_camera->CreateCameraShake( alpha );
        }
	}


    //
    // Render big blast

    if( m_size - predictedLife < 1.0f )
    {
        float distToBang = (g_app->m_camera->GetPos() - m_pos).Mag();
        Vector3 predictedPos = m_pos;
        float size = (m_size * 2000.0f) / sqrtf(distToBang);
        float alpha = 1.0f - ( m_size - predictedLife ) / 1.0f;
        glColor4f       (1.0f,0.4f,0.4f,alpha);
        glEnable        (GL_TEXTURE_2D);
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDisable       (GL_CULL_FACE);
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();

        size *= 0.4f;
        glColor4f       ( 1.0f, 1.0f, 0.0f, alpha );
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );       glVertex3fv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3fv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3fv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 0 );       glVertex3fv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();
        glDisable       (GL_TEXTURE_2D);
        glEnable        (GL_CULL_FACE);
    }
}


// ****************************************************************************
// Class MuzzleFlash
// ****************************************************************************


MuzzleFlash::MuzzleFlash()
:   WorldObject(),
    m_size(1.0f),
    m_life(1.0f)
{
    m_type = EffectMuzzleFlash;
}


MuzzleFlash::MuzzleFlash( Vector3 const &_pos, Vector3 const &_front, float _size, float _life )
:   WorldObject(),
    m_front(_front),
    m_size(_size),
    m_life(_life)
{
    m_pos = _pos;
}


bool MuzzleFlash::Advance()
{
    if( m_life <= 0.0f ) return true;

    m_pos += m_front * SERVER_ADVANCE_PERIOD * 10.0f;
    m_life -= SERVER_ADVANCE_PERIOD * 10.0f;

    return false;
}


void MuzzleFlash::Render( float _predictionTime )
{
    float predictedLife = m_life - _predictionTime * 10.0f;
    //float predictedLife = m_life;
    Vector3 predictedPos = m_pos + m_front * _predictionTime * 10.0f;
    Vector3 right = m_front ^ g_upVector;

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    Vector3 fromPos = predictedPos;
    Vector3 toPos = predictedPos + m_front * m_size;

    Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
    Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
    Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
    Vector3 toPosToFromPos  = toPos - fromPos;

    rightAngle *= m_size * 0.5f;

    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/muzzleflash.bmp" ) );
    glDepthMask     ( false );

    float alpha = predictedLife;
    alpha = min( 1.0f, alpha );
    alpha = max( 0.0f, alpha );
    glColor4f       ( 1.0f, 1.0f, 1.0f, alpha );

    glBegin( GL_QUADS );
        for( int i = 0; i < 5; ++i )
        {
            rightAngle *= 0.8f;
            toPosToFromPos *= 0.8f;
            glTexCoord2i(0,0);      glVertex3fv( (fromPos - rightAngle).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (fromPos + rightAngle).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (fromPos + toPosToFromPos + rightAngle).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (fromPos + toPosToFromPos - rightAngle).GetData() );
        }
    glEnd();

    glDisable       ( GL_TEXTURE_2D );
}


// ****************************************************************************
// Class Missile
// ****************************************************************************

Missile::Missile()
:   WorldObject()
{
    m_shape = g_app->m_resource->GetShape( "missile.shp" );
    m_booster = m_shape->m_rootFragment->LookupMarker( "MarkerBooster" );

    int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeRocket );
    m_life = 5.0f + rocketResearch * 5.0f;
}


bool Missile::AdvanceToTargetPosition( Vector3 const &_pos )
{
    float amountToTurn = SERVER_ADVANCE_PERIOD * 2.0f;
    Vector3 targetDir = (_pos - m_pos).Normalise();

    // Look ahead to see if we're about to hit the ground
    Vector3 forwardPos = m_pos + targetDir * 100.0f;
    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(forwardPos.x, forwardPos.z);
    if( forwardPos.y <= landHeight && (forwardPos - _pos).Mag() > 100.0f )
    {
        targetDir = g_upVector;
    }

    Vector3 actualDir = m_front * (1.0f - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();
    float speed = 200.0f;

    Vector3 oldPos = m_pos;
    Vector3 newPos = m_pos + actualDir * speed * SERVER_ADVANCE_PERIOD;
    landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( newPos.x, newPos.z );
    if( newPos.y <= landHeight ) return true;

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;
    m_vel = ( m_pos - oldPos ) / SERVER_ADVANCE_PERIOD;
    m_front = actualDir;

    Vector3 right = m_front ^ g_upVector;
    m_up = right ^ m_front;

    return ( m_pos - _pos ).Mag() < 20.0f;
}


bool Missile::Advance()
{
    bool dead = false;
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life < 0.0f )
    {
        Explode();
        return true;
    }

//    Tank *tank = (Tank *) g_app->m_location->GetEntitySafe( m_tankId, Entity::TypeTank );
//    if( tank )
//    {
//        m_target = tank->GetMissileTarget();
//    }
//
    bool arrived = AdvanceToTargetPosition( m_target );
    if( arrived )
    {
        Explode();
        return true;
    }

    m_history.PutData( m_pos );

    //
    // Create smoke trail

    Vector3 vel( m_vel / -20.0f );
    vel.x += syncsfrand(2.0f);
    vel.y += syncsfrand(2.0f);
    vel.z += syncsfrand(2.0f);
    float size = 50.0f + syncfrand(150.0f);
    float backPos = syncfrand(3.0f);

    Matrix34 mat( m_front, m_up, m_pos );
    Vector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;
    g_app->m_particleSystem->CreateParticle(boosterPos - m_vel*SERVER_ADVANCE_PERIOD*2.0f, vel, Particle::TypeMissileTrail, size );
    g_app->m_particleSystem->CreateParticle(boosterPos - m_vel*SERVER_ADVANCE_PERIOD*1.5f, vel, Particle::TypeMissileTrail, size );

    return false;
}


void Missile::Explode()
{
    g_app->m_location->Bang( m_pos, 20.0f, 100.0f );
}


void Missile::Render( float _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Matrix34 mat( m_front, m_up, predictedPos );

    glDisable( GL_BLEND );

    g_app->m_renderer->SetObjectLighting();
    glEnable        (GL_CULL_FACE);
    glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_COLOR_MATERIAL);
    glDisable       (GL_BLEND);

    m_shape->Render( _predictionTime, mat );
    g_app->m_renderer->UnsetObjectLighting();

    glEnable        ( GL_BLEND );
    glDisable       ( GL_COLOR_MATERIAL );
    glDisable       ( GL_CULL_FACE );

    Vector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;
    m_fire.m_pos = boosterPos;
    m_fire.m_vel = m_vel;
    m_fire.m_size = 30.0f + frand(20.0f);
    m_fire.m_front = -m_front;
    m_fire.Render( _predictionTime );

    for( int i = 0; i < 5; ++i )
    {
        Vector3 vel( m_vel / -10.0f );
        vel.x += sfrand(8.0f);
        vel.y += sfrand(8.0f);
        vel.z += sfrand(8.0f);
        Vector3 pos = predictedPos - m_vel * (0.1f+frand(0.1f));
        pos.x += sfrand(8.0f);
        pos.y += sfrand(8.0f);
        pos.z += sfrand(8.0f);
        float size = 200.0f + frand(200.0f);
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
    }
}


// ****************************************************************************
// Class TurretShell
// ****************************************************************************

TurretShell::TurretShell(float _life)
:   WorldObject(),
    m_life(_life)
{
    m_type = EffectGunTurretShell;
}


bool TurretShell::Advance()
{
    //
    // Update our position

    Vector3 oldPos = m_pos;

    m_life -= SERVER_ADVANCE_PERIOD;
    //m_vel.y -= 10.0f * SERVER_ADVANCE_PERIOD;
    //m_vel.x *= ( 1.0f - SERVER_ADVANCE_PERIOD );
    //m_vel.z *= ( 1.0f - SERVER_ADVANCE_PERIOD );
    //if( m_vel.y > 0 ) m_vel.y *= ( 1.0f - SERVER_ADVANCE_PERIOD );
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    if( m_life <= 0.0f )
    {
        return true;
    }


    if ( m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() ||
         m_pos.z < 0 || m_pos.z > g_app->m_location->m_landscape.GetWorldSizeZ() ||
         m_pos.y < 0 )
    {
        // Outside of world
        return true;
    }


    //
    // Did we hit anyone?

    Vector3 centrePos = ( m_pos + oldPos ) / 2.0f;
    float radius = ( m_pos - oldPos ).Mag() / 1.0f;
    int numFound;
    WorldObjectId *ids = g_app->m_location->m_entityGrid->GetNeighbours( centrePos.x, centrePos.z, radius, &numFound );

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity )
        {
            Vector3 rayDir = m_vel;
            rayDir.Normalise();
            if( entity->RayHit( oldPos, rayDir ) )
            {
                entity->ChangeHealth( -10 );
                if( entity->m_onGround )
                {
                    Vector3 push( entity->m_pos - m_pos );
                    push.Normalise();
                    push += Vector3( syncsfrand(3.0f), syncfrand(3.0f), syncsfrand(3.0f) );
                    push.SetLength( 20.0f );
                    entity->m_vel += push;
                    entity->m_onGround = false;
                }
            }
        }
    }


    //
    // Did we hit any buildings?

    {
        Vector3 rayDir = m_vel;
        rayDir.Normalise();
        Vector3 hitPos(0,0,0);
        Vector3 hitNorm(0,0,0);

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings.GetData(i);
                if( building->DoesRayHit( m_pos, rayDir, (m_vel * SERVER_ADVANCE_PERIOD).Mag(), &hitPos, &hitNorm ) )
                {
                    for( int p = 0; p < 3; ++p )
                    {
                        Vector3 vel = ( m_pos - building->m_centrePos ).Normalise();
                        vel *= 50.0f;
                        vel.x += sfrand(10.0f);
                        vel.y += frand(10.0f);
                        vel.z += sfrand(10.0f);
                        float size = 25.0f + frand(25.0f);
                        g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeRocketTrail, size );
                    }
                    building->Damage( -2 );
                    return true;
                }
            }
        }
    }


    //
    // Did we hit the landscape?

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if (m_pos.y <= landHeight )
    {
        for( int i = 0; i < 3; ++i )
        {
            Vector3 vel = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
            vel *= 50.0f;
            vel.x += sfrand(10.0f);
            vel.y += frand(10.0f);
            vel.z += sfrand(10.0f);
            float size = 25.0f + frand(25.0f);
            g_app->m_particleSystem->CreateParticle( oldPos, vel, Particle::TypeRocketTrail, size );
        }

        return true;
    }

    return false;
}


void TurretShell::Render( float predictionTime )
{
    predictionTime+=SERVER_ADVANCE_PERIOD;

    //Vector3 predictedVel = m_vel;
    //predictedVel.y -= 30.0f * predictionTime;
    //Vector3 predictedPos = m_pos + predictedVel * predictionTime;
    //RenderSphere( predictedPos, 1.0f );


    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    Vector3 predictedFront = m_vel;
    predictedFront.Normalise();
    Vector3 right = predictedFront ^ g_upVector;
    Vector3 up = right ^ predictedFront;
    Shape *shape = g_app->m_resource->GetShape( "turretshell.shp" );


    Matrix34 shellMat( predictedFront, up, predictedPos );

    glDisable( GL_BLEND );
    g_app->m_renderer->SetObjectLighting();
    shape->Render( predictionTime, shellMat );
    g_app->m_renderer->UnsetObjectLighting();
    glEnable( GL_BLEND );
}

// ****************************************************************************
//  Class SubversionBeam
// ****************************************************************************

void SubversionBeam::Initialise(float _lifeTime)
{
    m_life = _lifeTime * 2;
    m_harmless = false;
    m_bounced = false;

    g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeLaser );
}


bool SubversionBeam::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0f )
    {
        return true;
    }

    Vector3 oldPos = m_pos;
    m_pos += (m_vel/2) * SERVER_ADVANCE_PERIOD;

    //
    // Detect collisions with landscape / buildings / people

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if (m_pos.y <= landHeight )
    {
        if( m_bounced )
        {
            // Second richochet, so die
            return true;
        }
        else
        {
            // Richochet
            Vector3 hitPoint;
            Vector3 vel = m_vel;
            vel.Normalise();
            g_app->m_location->m_landscape.RayHit( oldPos, vel, &hitPoint );
            float distanceTravelled = (hitPoint - oldPos).Mag();
            float distanceTotal = (m_vel * SERVER_ADVANCE_PERIOD).Mag();

            Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(hitPoint.x, hitPoint.z);
            Vector3 incomingVel = m_vel * -1.0f;
            float dotProd = normal * incomingVel;
            m_vel = 2.0f * dotProd * normal - incomingVel;

            m_pos = hitPoint;
            Vector3 distanceRemaining = m_vel;
            distanceRemaining.SetLength( distanceTotal - distanceTravelled );

            m_pos += distanceRemaining;
            g_app->m_soundSystem->TriggerOtherEvent( this, "Richochet", SoundSourceBlueprint::TypeLaser );

            m_bounced = true;
        }
    }
    else if ( m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() ||
              m_pos.z < 0 || m_pos.z > g_app->m_location->m_landscape.GetWorldSizeZ() )
    {
        // Outside game world
        return true;
    }
    else
    {
        //
        // Detect collisions with buildings

        Vector3 rayDir = m_vel;
        rayDir.Normalise();
        Vector3 hitPos(0,0,0);
        Vector3 hitNorm(0,0,0);

        LList<int> *nearbyBuildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );
        for( int i = 0; i < nearbyBuildings->Size(); ++i )
        {
            Building *building = g_app->m_location->GetBuilding( nearbyBuildings->GetData(i) );
            if( building->DoesRayHit( m_pos, rayDir, (m_vel * SERVER_ADVANCE_PERIOD).Mag(), &hitPos, &hitNorm ) )
            {
                Vector3 vel( -m_vel/15.0f );
                vel.x += sfrand(10.0f);
                vel.y += sfrand(10.0f);
                vel.z += sfrand(10.0f);
                g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeRocketTrail );
                g_app->m_soundSystem->TriggerOtherEvent( this, "HitBuilding", SoundSourceBlueprint::TypeLaser );
                return true;
            }
        }


        //
        // Detect collisions with entities

        if( !m_harmless )
        {
		    Vector3 halfDelta = m_vel * (SERVER_ADVANCE_PERIOD * 0.5f);
		    Vector3 rayStart = m_pos - halfDelta;
		    Vector3 rayEnd = m_pos + halfDelta;
            int numFound;
            float maxRadius = halfDelta.Mag() * 2.0f;
            WorldObjectId *ids = g_app->m_location->m_entityGrid->GetEnemies( m_pos.x, m_pos.z, maxRadius, &numFound, m_fromTeamId );
            for (int i = 0; i < numFound; ++i)
            {
                WorldObjectId id = ids[i];
                WorldObject *wobj = g_app->m_location->GetEntity(id);
			    Entity *entity = (Entity *) wobj;

			    if( PointSegDist2D(Vector2(entity->m_pos), Vector2(rayStart), Vector2(rayEnd)) < 10.0f )
                {
                    g_app->m_soundSystem->TriggerOtherEvent( this, "HitEntity", SoundSourceBlueprint::TypeLaser );
					if( entity->m_type == Entity::TypeDarwinian )
					{
                        WorldObjectId id = g_app->m_location->SpawnEntities( entity->m_pos, m_fromTeamId, -1, Entity::TypeDarwinian, 1, entity->m_vel, 0.0 );            
                        entity->m_dead = true;

						Darwinian *victim = (Darwinian *) entity;
                        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
					
						d->m_corrupted = victim->m_corrupted;
						d->m_soulless = victim->m_soulless;
						d->m_subversive = victim->m_subversive;

						d->m_colourTimer = 4;
						d->m_oldTeamId = victim->m_id.GetTeamId();

                        m_harmless = true;
                    }
                    return false;
                }
            }
        }
    }

    return false;
}


void SubversionBeam::Render( float predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;

    //
    // No richochet occurred recently
	Vector3 lengthVector = m_vel;
	lengthVector.SetLength(10.0f);
    Vector3 fromPos = predictedPos;
    Vector3 toPos = predictedPos - lengthVector;

    Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
    Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
    float   camDistSqd      = camToMidPoint.MagSquared();
    Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

    rightAngle *= 0.8f;

    glEnd();
    glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/subversion.bmp"));
    glBegin( GL_QUADS );
    rightAngle *= 2.0;
	
    if( m_fromTeamId == -1 || m_fromTeamId == 255 ) return;

	RGBAColour const &colour = g_app->m_location->m_teams[ m_fromTeamId ].m_colour;
    glColor4ub( colour.r, colour.g, colour.b, 255 );

    glBegin( GL_QUADS );
        for( int i = 0; i < 5; ++i )
        {
            glTexCoord2i(0,0);      glVertex3fv( (fromPos - rightAngle).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (fromPos + rightAngle).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (toPos + rightAngle).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (toPos - rightAngle).GetData() );
        }
    glEnd();

    if( camDistSqd < 200.0f )
    {
        g_app->m_camera->CreateCameraShake( 0.5f );
    }

    glEnd();
    glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp", false));
    glBegin( GL_QUADS );
}