#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/profiler.h"
#include "lib/vector2.h"
#include "lib/shape.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/math/random_number.h"

#include "worldobject/weapons.h"
#include "worldobject/insertion_squad.h"
#include "worldobject/airstrike.h"
#include "worldobject/darwinian.h"
#include "worldobject/armour.h"
#include "worldobject/tank.h"
#include "worldobject/gunturret.h"

#include "multiwinia.h"
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
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "helpsystem.h"
#endif
#include "taskmanager.h"

#include "sound/soundsystem.h"
#include "sound/sound_library_3d.h"


static std::vector<WorldObjectId> s_neighbours;


// ****************************************************************************
//  Class ThrowableWeapon
// ****************************************************************************

ThrowableWeapon::ThrowableWeapon( int _type, Vector3 const &_startPos, Vector3 const &_front, double _force )
:   m_shape(NULL),
    m_force(1.0),
    m_numFlashes(0)
{
    m_shape = g_app->m_resource->GetShape( "throwable.shp" );
    m_pos = _startPos;
    m_vel = _front * _force;

    m_up = _front;
    m_front = g_upVector;
    Vector3 right = m_front ^ m_up;
    m_front = right ^ m_up;
    
    m_birthTime = GetNetworkTime();

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
        case EffectFlameGrenade:
            g_app->m_soundSystem->TriggerOtherEvent( this, _event, SoundSourceBlueprint::TypeAirstrikeBomb );
            break;
    }
}


bool ThrowableWeapon::Advance()
{
    if( !g_app->m_editing )
    {
	    if (m_type != EffectThrowableAirstrikeMarker && 
            syncfrand(1) > 0.1 / g_gameTimer.GetGameSpeed() )
	    {
            Vector3 vel( m_vel / 4.0 );
            vel.x += syncsfrand(5.0);
            vel.y += syncsfrand(5.0);
            vel.z += syncsfrand(5.0);
		    g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeMissileTrail, 60.0);
	    }
    }

    if( m_force > 0.1 )
    {
        m_vel.y -= 9.8 * g_gameTimer.GetGameSpeed();
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 1.0 )
        {
            BounceOffLandscape();
            m_force *= 0.75;
            m_vel *= m_force;
            if( m_pos.y < landHeight + 1.0 ) m_pos.y = landHeight + 1.0;               
            TriggerSoundEvent( "Bounce" );
        }
    }

    return false;
}


void ThrowableWeapon::Render( double _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;
    
    Vector3 right = m_up ^ m_front;
    m_up.RotateAround( right * m_force * m_force * g_gameTimer.GetServerAdvancePeriod() * 1.0f );
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


    int numFlashes = int( GetNetworkTime() - m_birthTime );
    if( numFlashes > m_numFlashes )
    {
        TriggerSoundEvent( "Flash" );
        m_numFlashes = numFlashes;
    }

    double flashAlpha = 1.0 - (( GetNetworkTime() - m_birthTime ) - numFlashes);    
    if( flashAlpha < 0.2 )
    {
        double distToThrowable = (g_app->m_camera->GetPos() - predictedPos).Mag();

        double size = 1000.0 / iv_sqrt(distToThrowable);    
        glColor4ub      ( m_colour.r, m_colour.g, m_colour.b, 200 );   
        glEnable        (GL_TEXTURE_2D);
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDisable       (GL_CULL_FACE);
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();
        size *= 0.4;
        glColor4f       ( 1.0, 1.0, 1.0, 0.7 );   
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glBegin( GL_QUADS );
            glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        glEnd();
        glDisable       (GL_TEXTURE_2D);
        glEnable        (GL_CULL_FACE);
    }
}

double ThrowableWeapon::GetMaxForce( int _researchLevel )
{
	switch( _researchLevel )
    {        
        case 0 :    return 50.0;
        case 1 :    return 80.0;
        case 2 :    return 110.0;
        case 3 :    return 140.0;
        case 4 :    return 170.0;
		default:	return 0.0;
    }
}

double ThrowableWeapon::GetApproxMaxRange( double _maxForce )
{
	return _maxForce + ( _maxForce * 0.75 ) + ( _maxForce * 0.75 * 0.75 );
}

void ThrowableWeapon::RemoveForce()
{
    m_force = 0.0;
}

void ThrowableWeapon::ResetForce()
{
    m_force = 1.0;
}

// ****************************************************************************
//  Class Grenade
// ****************************************************************************

Grenade::Grenade( Vector3 const &_startPos, Vector3 const &_front, double _force )
:   ThrowableWeapon( EffectThrowableGrenade, _startPos, _front, _force ),    
    m_life(3.0),
    m_power(25.0),
    m_collected(false),
    m_explodeOnImpact(false)
{
    m_colour.Set( 255, 100, 100 );
}


bool Grenade::Advance()
{
    if( m_collected ) return false;

    m_life -= SERVER_ADVANCE_PERIOD;

    bool explode = m_life <= 0.0;

    if( m_explodeOnImpact )
    {
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 2.0f) explode = true;
    }

    if( explode )
    {
        if( !g_app->m_editing )
        {
            TriggerSoundEvent( "Explode" );
            g_app->m_location->Bang( m_pos, m_power/2.0, m_power*2.0, m_id.GetTeamId());            
        }
        return true;
    }
    
    return ThrowableWeapon::Advance();
}


// ****************************************************************************
//  Class AirStrikeMarker
// ****************************************************************************


AirStrikeMarker::AirStrikeMarker( Vector3 const &_startPos, Vector3 const &_front, double _force )
:   ThrowableWeapon( EffectThrowableAirstrikeMarker, _startPos, _front, _force )
{
    m_naplamBombers = false;
    m_colour.Set( 255, 100, 100 ); 
}


bool AirStrikeMarker::Advance()
{
    //AppDebugOut("Updating AirStrike Marker %d\n", m_id.GetUniqueId() );
    ThrowableWeapon::Advance();


    //
    // Dump loads of smoke for the bombers to see

    Vector3 vel( m_vel / 4.0 );
    vel.x += syncsfrand(5.0);
    vel.y += syncsfrand(5.0);
    vel.z += syncsfrand(5.0);
	g_app->m_particleSystem->CreateParticle(m_pos, vel, Particle::TypeFire, 100.0);


    //
    // If its time, summon the actual airstrike

    if( GetNetworkTime() > m_birthTime + 2.0f &&
        m_force <= 0.1f )
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
           // AppDebugOut("Creating AirStrike Unit at %2.2f\n", GetNetworkTime() );
            // Summon an air strike now
            int teamId = m_id.GetTeamId();
            int unitId;
            Team *team = g_app->m_location->m_teams[teamId];

            int airStikeResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeAirStrike );            
            AirstrikeUnit *unit = (AirstrikeUnit *) team->NewUnit( Entity::TypeSpaceInvader, airStikeResearch, &unitId, m_pos );            
            unit->m_effectId = m_id.GetIndex();
            unit->m_napalm = m_naplamBombers;
            m_airstrikeUnit.Set( teamId, unitId, -1, -1 );
        }
    }

    return false;
}


// ****************************************************************************
//  Class ControllerGrenade
// ****************************************************************************

ControllerGrenade::ControllerGrenade( Vector3 const &_startPos, Vector3 const &_front, double _force )
:   ThrowableWeapon( EffectThrowableControllerGrenade, _startPos, _front, _force )
{
    m_squadTaskId = -1;
    m_colour.Set( 100, 100, 255 );
}


bool ControllerGrenade::Advance()
{
    ThrowableWeapon::Advance();

    if( GetNetworkTime() > m_birthTime + 3.0 )
    {             
        g_app->m_soundSystem->TriggerOtherEvent( this, "ExplodeController", SoundSourceBlueprint::TypeGrenade );

        //
        // Particle shower

        int numFlashes = 5 + AppRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0), frand(35.0), sfrand(15.0) );
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeControlFlash, 100.0 );
        }

        for( int i = 0; i < 50; ++i )
        {
            Vector3 thisVector = Vector3(50,0,0);
            thisVector.RotateAroundY( 2.0 * M_PI * (double)i / 50.0 );
            
            g_app->m_particleSystem->CreateParticle( m_pos + thisVector, thisVector/10, Particle::TypeControlFlash, 100.0 );
        }


        //
        // Zap all darwinians in range on our team

        int numFound;
        g_app->m_location->m_entityGrid->GetFriends( s_neighbours, m_pos.x, m_pos.z, 50.0f, &numFound, m_id.GetTeamId() );
        for( int i = 0; i < numFound; ++i )
        {
            WorldObjectId id = s_neighbours[i];
            Entity *entity = g_app->m_location->GetEntity( id );
            if( entity && entity->m_type == Entity::TypeDarwinian )
            {
                Darwinian *darwinian = (Darwinian *) entity;
                darwinian->TakeControl( m_squadTaskId );
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
    m_fromTeamId(255),
    m_lifeTime(0.0f),
    m_fromBuildingId(-1),
	m_firework(false)
{
    m_pos = _startPos + Vector3(0,2,0);
    m_vel = ( _targetPos - m_pos ).Normalise() * 50.0;
    
    m_shape = g_app->m_resource->GetShape( "throwable.shp" );
   
    m_timer = GetNetworkTime();
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

    if( m_vel.Mag() < 150.0 )
    {
        m_vel *= 1.2;
    }
    
	// 
	// Curl for a firework
	if( m_firework )
	{
		Vector3 toBeAt = m_pos + (m_vel * SERVER_ADVANCE_PERIOD);
		double xDiff = toBeAt.x-m_pos.x;
		double zDiff = toBeAt.z-m_pos.z;
		m_vel.x += xDiff < 0 ? max(xDiff*-xDiff,-16.0) : min(xDiff*xDiff,16.0);
		m_vel.z += zDiff < 0 ? max(zDiff*-zDiff,-16.0) : min(zDiff*zDiff,16.0);
	}

    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    
    //
    // Create smoke trail

	if (syncfrand(1) > 0.2)
	{
        Vector3 vel( m_vel / 10.0 );
        vel.x += syncsfrand(4.0);
        vel.y += syncsfrand(4.0);
        vel.z += syncsfrand(4.0);
        g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeFire);
	}
    

    //
    // Have we run out of steam?

    int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeRocket );
    double maxLife = 0.0;
    switch( rocketResearch )
    {
        case 0 :        maxLife = 0.25;        break;
        case 1 :        maxLife = 0.5;         break;
        case 2 :        maxLife = 0.75;        break;
        case 3 :        maxLife = 1.0;         break;
        case 4 :        maxLife = 1.5;         break;
    }

    if( m_lifeTime != 0.0 ) maxLife = m_lifeTime;

    if( GetNetworkTime() > m_timer + maxLife )
    {
        Explode();

        return true;
    }


    //
    // Have we hit the ground?

    if (g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z) >= m_pos.y)
    {
        Explode();

        return true;
    }


    //
    // Have we hit any buildings?

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building->DoesSphereHit( m_pos, 3.0 ) )
        {
            Explode();

            return true;
        }
    }

    return false;
}

void Rocket::Explode()
{
    int numKills = g_app->m_location->Bang( m_pos, 15.0f, 25.0f, m_id.GetTeamId(), m_fromBuildingId );  
	
	if( m_firework )
    {
        int numCores = 15.0f * 25.0f * 0.01f;
        numCores += frand(numCores);
	    RGBAColour colour(sfrand(255.0f), sfrand(255.0f), sfrand(255.0f), 1.0f-frand());
        for( int p = 0; p < numCores; ++p )
        {
            Vector3 vel( sfrand( 30.0f ), 
                         10.0f + frand( 10.0f ), 
                         sfrand( 30.0f ) );
            float size = 120.0f + frand(120.0f);
            g_app->m_particleSystem->CreateParticle( m_pos + g_upVector * 15.0f * 0.3f, vel, 
			    Particle::TypeExplosionCore, size, colour );
        }
    }

    g_app->m_soundSystem->StopAllSounds( m_id, "Rocket Create" );
    g_app->m_soundSystem->TriggerOtherEvent( this, "Explode", SoundSourceBlueprint::TypeRocket );

    GunTurret *g = (GunTurret *)g_app->m_location->GetBuilding( m_fromBuildingId );
    if( g && g->m_type == Building::TypeGunTurret )
    {
        g->RegisterKill( 255, numKills );
    }
}


void Rocket::Render( double predictionTime )
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
        Vector3 vel( m_vel / -20.0 );
        vel.x += sfrand(8.0);
        vel.y += sfrand(8.0);
        vel.z += sfrand(8.0);
        Vector3 pos = predictedPos - m_vel * (0.05+frand(0.05));
        pos.x += sfrand(8.0);
        pos.y += sfrand(8.0);
        pos.z += sfrand(8.0);
        double size = 50.0 + frand(50.0);
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
    }   
}



// ****************************************************************************
//  Class Laser
// ****************************************************************************

void Laser::Initialise(double _lifeTime)
{
    m_life = _lifeTime;
    m_harmless = false;
    m_bounced = false;

    g_app->m_soundSystem->TriggerOtherEvent( this, "Create", SoundSourceBlueprint::TypeLaser );
}


bool Laser::Advance()
{
    if( m_fromTeamId == -1 || m_fromTeamId == 255 ) return true;
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0 )
    {
        return true;
    }

    Vector3 oldPos = m_pos;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    //
    // Detect collisions with landscape / buildings / people

    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

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
            double distanceTravelled = (hitPoint - oldPos).Mag();
            double distanceTotal = (m_vel * SERVER_ADVANCE_PERIOD).Mag();
        
            Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(hitPoint.x, hitPoint.z);
            Vector3 incomingVel = m_vel * -1.0;
            double dotProd = normal * incomingVel;        
            m_vel = 2.0 * dotProd * normal - incomingVel;
        
            m_pos = hitPoint;
            Vector3 distanceRemaining = m_vel;
            distanceRemaining.SetLength( distanceTotal - distanceTravelled );

            m_pos += distanceRemaining;

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
                Vector3 vel( -m_vel/15.0 );
                vel.x += sfrand(10.0);
                vel.y += sfrand(10.0);
                vel.z += sfrand(10.0);
                g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeRocketTrail );
                g_app->m_soundSystem->TriggerOtherEvent( this, "HitBuilding", SoundSourceBlueprint::TypeLaser );
                return true;
            }
        }


        //
        // Detect collisions with entities
        
        if( !m_harmless )
        {
		    Vector3 halfDelta = m_vel * (SERVER_ADVANCE_PERIOD * 0.5);
		    Vector3 rayStart = m_pos - halfDelta;
		    Vector3 rayEnd = m_pos + halfDelta;
            int numFound;
            double maxRadius = halfDelta.Mag() * 2.0;
            g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, m_pos.x, m_pos.z, maxRadius, &numFound, m_fromTeamId );
            for (int i = 0; i < numFound; ++i)
            {
                WorldObjectId id = s_neighbours[i];
                Entity *entity = g_app->m_location->GetEntity(id);

				double verticalHit = g_app->IsSinglePlayer() ||
									iv_abs( entity->m_pos.y - m_pos.y ) < 5.0;
                
				// Note by chris :
				// Single player doesnt check lasers are vertically hitting the target,
				// which allows squaddies to be inaccurate and still fun.
				// In multiplayer darwinians shoot accurately enough to use vertical checking
				
			    if( verticalHit &&
                    PointSegDist2D(Vector2(entity->m_pos), Vector2(rayStart), Vector2(rayEnd)) < 5.0 )
                {       
                    // note - we shouldnt need any of these entity type checks with the new typed damage system, Gary
                    /*if( entity->m_type == Entity::TypeSpider ||
                        entity->m_type == Entity::TypeSporeGenerator ||
                        entity->m_type == Entity::TypeEngineer ||
                        entity->m_type == Entity::TypeTriffidEgg ||
                        entity->m_type == Entity::TypeSoulDestroyer ||
                        entity->m_type == Entity::TypeArmour ||
                        entity->m_type == Entity::TypeHarvester ||
                        entity->m_type == Entity::TypeTripod || 
                        entity->m_type == Entity::TypeTentacle ||
                        entity->m_type == Entity::TypeTank ||
                        entity->m_type == Entity::TypeSpaceShip ||
                        entity->m_type == Entity::TypeSpaceInvader ||
                        (entity->m_type == Entity::TypeDarwinian && ((Darwinian *)entity)->IsInvincible() ) ) // boosted Darwinians are immune to lasers
                    {
						//entity->ChangeHealth(-1);
					}
					else*/
					{
                        //g_app->m_soundSystem->TriggerOtherEvent( this, "HitEntity", SoundSourceBlueprint::TypeLaser );

                        if( m_mindControl && entity->m_type == Entity::TypeDarwinian )
                        {
                            WorldObjectId id = g_app->m_location->SpawnEntities( entity->m_pos, m_fromTeamId, -1, Entity::TypeDarwinian, 1, entity->m_vel, 0.0 );            
                            entity->m_destroyed = true;

                            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( id, Entity::TypeDarwinian );
                            if( d )
                            {
                                //d->GiveMindControl( m_mindControl );
                                d->m_previousTeamId = entity->m_id.GetTeamId();
                                d->m_colourTimer = 4.0;
                                /*for( int i = 0; i < 10; i++ )
                                {
                                    Vector3 vel;
                                    vel.x = sfrand(15.0);
                                    vel.y = frand(20.0);
                                    vel.z = sfrand(15.0);
                                    double size = 10.0 + frand(15.0);
                                    g_app->m_particleSystem->CreateParticle( d->m_pos, vel, Particle::TypeControlFlash, size, g_app->m_location->m_teams[d->m_id.GetTeamId()]->m_colour );
                                }*/
                            }
                        }
                        else
                        {
						    double damage = 20.0 + syncfrand(20.0);
						    bool dead = entity->m_dead;
                            bool success = entity->ChangeHealth( (int)-damage, Entity::DamageTypeLaser );
                            if( !dead && entity->m_dead ) 
                            {
								if( entity->m_type == Entity::TypeDarwinian || g_app->IsSinglePlayer() )
                                {
                                    g_app->m_location->m_teams[m_fromTeamId]->m_teamKills++;
                                }
                                g_app->m_soundSystem->TriggerOtherEvent( entity->m_pos, "KilledEntity", SoundSourceBlueprint::TypeLaser, m_id.GetTeamId() );
                            }
                            else if( !dead && !entity->m_dead )
                            {
                                if( entity->m_type == Entity::TypeDarwinian )
                                {
                                    // It's just a flesh wound
                                    g_app->m_soundSystem->TriggerOtherEvent( entity->m_pos, "InjuredEntity", SoundSourceBlueprint::TypeLaser, m_id.GetTeamId() );
                                }
                            }
                            if( success )
                            {
                                Vector3 push( m_vel );
                                push.SetLength(syncfrand(10.0));
                                push.y = 7.0 + syncfrand(5.0);
                                
                                if( entity->m_type != Entity::TypeInsertionSquadie &&
                                    entity->m_type != Entity::TypeVirii &&
                                    entity->m_type != Entity::TypeShaman )
                                {
                                    entity->m_front = push;
                                    entity->m_front.Normalise();
                                }
                                if( entity->m_type != Entity::TypeVirii &&
                                    entity->m_type != Entity::TypeShaman )
                                {
                                    entity->m_vel += push;
                                }
                                //entity->m_onGround = false;
                            }
                        }
                        m_harmless = true;                        
                    }
                    return false;
                }
            }
        }
    }

    return false;
}


void Laser::Render( double predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * predictionTime;
    
    //
    // No richochet occurred recently
	Vector3 lengthVector = m_vel;
	lengthVector.SetLength(10.0);
    Vector3 fromPos = predictedPos;
    Vector3 toPos = predictedPos + lengthVector;

    Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0;
    Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
    double   camDistSqd      = camToMidPoint.MagSquared();
    Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
    
    rightAngle *= 0.8;

    if( m_mindControl )
    {
        glEnd();
        glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/subversion.bmp"));
        glBegin( GL_QUADS );
        rightAngle *= 2.0;
    }

    if( m_fromTeamId == -1 || m_fromTeamId == 255 ) return;

    RGBAColour const &colour = g_app->m_location->m_teams[ m_fromTeamId ]->m_colour;
    int alpha = 255;
    glColor4ub( colour.r, colour.g, colour.b, alpha );

    if( g_app->m_location->GetMonsterTeamId() == m_fromTeamId )
    {
        glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
    }

    //glBegin( GL_QUADS );
        for( int i = 0; i < 5; ++i )
        {
            glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
            glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     
        }
    //glEnd();

    if( !g_app->m_multiwinia->GameOver() &&
        !g_app->GamePaused() )
    {
        if( camDistSqd < 200.0 )
        {
            g_app->m_camera->CreateCameraShake( 0.5 );
        }
    }

    if( m_mindControl )
    {
        glEnd();
        glBindTexture	(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/laser.bmp", false));
        glBegin( GL_QUADS );
    }
}

// ****************************************************************************
// Class Shockwave
// ****************************************************************************

Shockwave::Shockwave( int _teamId, double _size )
:   m_teamId(_teamId),
    m_size(_size),
    m_life(_size),
    m_shape(NULL),
    m_fromBuildingId(-1)
{    
//    m_shape = g_app->m_resource->GetShape( "shockwave.shp" );
    m_type = EffectShockwave;
}


bool Shockwave::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0 )
    {
        return true;
    }

    double radius = 35.0 + 40.0 * (m_size - m_life);
    int numFound;
	int numKilled = 0;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, radius, &numFound );
    
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        WorldObject *wobj = g_app->m_location->GetEntity( id );
        Entity *ent = (Entity *) wobj;

        bool dead = ent->m_dead;

        double distance = ( ent->m_pos - m_pos ).Mag();
        if( iv_abs(distance - radius) < 10.0 )
        {
            double fraction = (m_life / m_size);
            bool success = false;

            if( ent->m_type == Entity::TypeDarwinian )
			{
				if( syncfrand(4) < 0.1 )
				{
					((Darwinian *)ent)->SetFire();
				}
				else
				{
                    success = ent->ChangeHealth( -40 * fraction, Entity::DamageTypeExplosive );
				}
			}
			else
			{
				success = ent->ChangeHealth( -40 * fraction, Entity::DamageTypeExplosive );
			}

            if( !dead && ent->m_dead )
            {
                if( ent->m_id.GetTeamId() != m_id.GetTeamId() )
                {
                    if( (ent->m_type == Entity::TypeDarwinian &&
                        m_teamId >= 0 && m_teamId < NUM_TEAMS) || 
					    g_app->IsSinglePlayer() )
                    {
                        g_app->m_location->m_teams[m_teamId]->m_teamKills++;
                    }

                    GunTurret *g = (GunTurret *)g_app->m_location->GetBuilding( m_fromBuildingId );
                    if( g && g->m_type == Building::TypeGunTurret )
                    {
                        g->RegisterKill( ent->m_id.GetTeamId(), 1 );
                    }
                }
            }


            if( success && ent->m_onGround )
            {
                if( ent->m_type != Entity::TypeInsertionSquadie &&
                    ent->m_type != Entity::TypeOfficer &&
                    ent->m_type != Entity::TypeShaman ) 
                {
                    Vector3 push( ent->m_pos - m_pos );
                    push.Normalise();
                    push.y = 3.0;
                    push.SetLength( 25.0 * fraction );

                    ent->m_vel += push;
                    ent->m_onGround = false;
                }
            }
        }
        
    }
   
    return false;    
}

void Shockwave::Render( double predictionTime )
{
    glShadeModel    ( GL_SMOOTH );
    glDisable       ( GL_DEPTH_TEST );

    glColor4f( 1.0, 1.0, 0.0, 1.0 );
    int numSteps = 50.0;
    double predictedLife = m_life - predictionTime;
    double radius = 35.0 + 40.0 * (m_size - predictedLife);
    double alpha = 0.6 * predictedLife / m_size;

    glBegin( GL_TRIANGLE_FAN );
    glColor4f( 1.0, 1.0, 0.0, 0.0 );
    glVertex3dv( (m_pos+Vector3(0,5,0)).GetData() );
    glColor4f( 1.0, 0.5, 0.5, alpha );

    double angle = 0.0;
    for( int i = 0; i <= numSteps; ++i )
    {
        double xDiff = radius * iv_sin(angle);
        double zDiff = radius * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        glVertex3dv( pos.GetData() );
        angle += 2.0 * M_PI / (double) numSteps;
    }
    
    glEnd();

    glShadeModel( GL_FLAT );
    glEnable    ( GL_DEPTH_TEST );

    //  
    // Render screen flash if we are new

    if( m_size - predictedLife < 0.1 &&
        !g_app->m_multiwinia->GameOver() &&
        !g_app->GamePaused() )
    {
        if( g_app->m_camera->PosInViewFrustum( m_pos ) )
        {
            double distance = ( g_app->m_camera->GetPos() - m_pos ).Mag();
            double distanceFactor = 1.0 - ( distance / 500.0 );
            if( distanceFactor < 0.0 ) distanceFactor = 0.0;
            
            double alpha = 1.0 - ( m_size - predictedLife ) / 0.1;
            alpha *= distanceFactor;
            
            glColor4f(1.0,1.0,1.0,alpha);
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

    if( m_size - predictedLife < 1.0 )
    {
        double distToBang = (g_app->m_camera->GetPos() - m_pos).Mag();
        Vector3 predictedPos = m_pos;

        double size = (m_size * 500.0) / iv_sqrt(distToBang); 
        double alpha = 1.0 - ( m_size - predictedLife ) / 1.0;

        glColor4f       (1.0,0.5,0.2,alpha);
        glEnable        (GL_TEXTURE_2D);
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
        glDisable       (GL_CULL_FACE);

        glBegin( GL_QUADS );

        for( int i = 0; i < 5; ++i )
        {
            size *= 1.2;

            glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        }

        glEnd();

        size = (m_size * 1000.0) / iv_sqrt(distToBang); 
        alpha = 1.0 - ( m_size - predictedLife ) / 1.0;

        glColor4f       (1.0,0.5,0.2,alpha);
        glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
        glBegin( GL_QUADS );

        for( int i = 0; i < 5; ++i )
        {
            size *= 1.2;

            glTexCoord2i( 0, 0 );       glVertex3dv( (predictedPos - g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 1, 0 );       glVertex3dv( (predictedPos + g_app->m_camera->GetRight()*size).GetData() );
            glTexCoord2i( 1, 1 );       glVertex3dv( (predictedPos + g_app->m_camera->GetUp()*size).GetData() );
            glTexCoord2i( 0, 1 );       glVertex3dv( (predictedPos - g_app->m_camera->GetRight()*size).GetData() );
        }

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
    m_size(1.0),
    m_life(1.0)
{
    m_type = EffectMuzzleFlash;
}


MuzzleFlash::MuzzleFlash( Vector3 const &_pos, Vector3 const &_front, double _size, double _life )
:   WorldObject(),
    m_front(_front),
    m_size(_size),
    m_life(_life)
{
    m_pos = _pos;
    m_type = EffectMuzzleFlash;
}


bool MuzzleFlash::Advance()
{
    if( m_life <= 0.0 )
    {
        return true;
    }

    m_pos += m_front * SERVER_ADVANCE_PERIOD * 10.0;
    m_life -= SERVER_ADVANCE_PERIOD * 10.0;
        
    return false;
}


void MuzzleFlash::Render( double _predictionTime )
{
    double predictedLife = m_life - _predictionTime * 10.0;
    //double predictedLife = m_life;
    Vector3 predictedPos = m_pos + m_front * _predictionTime * 10.0;
    Vector3 right = m_front ^ g_upVector;
    
    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    Vector3 fromPos = predictedPos;
    Vector3 toPos = predictedPos + m_front * m_size;

    Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0;
    Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
    Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
    Vector3 toPosToFromPos  = toPos - fromPos;

    rightAngle *= m_size * 0.5;
    
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/muzzleflash.bmp" ) );
    glDepthMask     ( false );

    if( m_size < 50 )
    {
        glDisable       ( GL_DEPTH_TEST );
    }

    float alpha = predictedLife * 0.75f;
    alpha = min( 1.0f, alpha );
    alpha = max( 0.0f, alpha );
    glColor4f       ( 1.0f, 1.0f, 1.0f, alpha );
 
    glBegin( GL_QUADS );
        for( int i = 0; i < 5; ++i )
        {
            rightAngle *= 0.8;
            toPosToFromPos *= 0.8;
            glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (fromPos + toPosToFromPos + rightAngle).GetData() );                
            glTexCoord2i(1,0);      glVertex3dv( (fromPos + toPosToFromPos - rightAngle).GetData() );                     
        }
    glEnd();

    glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );    
}


// ****************************************************************************
// Class Missile
// ****************************************************************************

Missile::Missile()
:   WorldObject()
{
    m_shape = g_app->m_resource->GetShape( "missile.shp" );
    const char boosterName[] = "MarkerBooster";
    m_booster = m_shape->m_rootFragment->LookupMarker( boosterName );
    AppReleaseAssert( m_booster, "Missile: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", boosterName, m_shape->m_name );

    int rocketResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeRocket );
    m_life = 5.0 + rocketResearch * 5.0;
}


bool Missile::AdvanceToTargetPosition( Vector3 const &_pos )
{
    double amountToTurn = SERVER_ADVANCE_PERIOD * 2.0;
    Vector3 targetDir = (_pos - m_pos).Normalise();

    // Look ahead to see if we're about to hit the ground
    Vector3 forwardPos = m_pos + targetDir * 100.0;
    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(forwardPos.x, forwardPos.z);
    if( forwardPos.y <= landHeight && (forwardPos - _pos).Mag() > 100.0 )
    {
        targetDir = g_upVector;
    }

    Vector3 actualDir = m_front * (1.0 - amountToTurn) + targetDir * amountToTurn;
    actualDir.Normalise();
    double speed = 200.0;
    
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
    
    return ( m_pos - _pos ).Mag() < 20.0;
}


bool Missile::Advance()
{
    bool dead = false;
    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life < 0.0 )
    {
        Explode();
        return true;
    }

    Tank *tank = (Tank *) g_app->m_location->GetEntitySafe( m_tankId, Entity::TypeTank );
    if( tank )
    {
        m_target = tank->GetMissileTarget();
    }

    bool arrived = AdvanceToTargetPosition( m_target );
    if( arrived )
    {
        Explode();
        return true;
    }
    
    m_history.PutData( m_pos );

    //
    // Create smoke trail

    Vector3 vel( m_vel / -20.0 );
    vel.x += syncsfrand(2.0);
    vel.y += syncsfrand(2.0);
    vel.z += syncsfrand(2.0);
    double size = 50.0 + syncfrand(150.0);
    double backPos = syncfrand(3.0);

    Matrix34 mat( m_front, m_up, m_pos );
    Vector3 boosterPos = m_booster->GetWorldMatrix(mat).pos;
    g_app->m_particleSystem->CreateParticle(boosterPos - m_vel*SERVER_ADVANCE_PERIOD*2.0, vel, Particle::TypeMissileTrail, size );
    g_app->m_particleSystem->CreateParticle(boosterPos - m_vel*SERVER_ADVANCE_PERIOD*1.5, vel, Particle::TypeMissileTrail, size );

    return false;
}


void Missile::Explode()
{
    g_app->m_location->Bang( m_pos, 20.0, 100.0 );
}


void Missile::Render( double _predictionTime )
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
    m_fire.m_size = 30.0 + frand(20.0);
    m_fire.m_front = -m_front;
    m_fire.Render( _predictionTime );
    
    for( int i = 0; i < 5; ++i )
    {
        Vector3 vel( m_vel / -10.0 );
        vel.x += sfrand(8.0);
        vel.y += sfrand(8.0);
        vel.z += sfrand(8.0);
        Vector3 pos = predictedPos - m_vel * (0.1+frand(0.1));
        pos.x += sfrand(8.0);
        pos.y += sfrand(8.0);
        pos.z += sfrand(8.0);
        double size = 200.0 + frand(200.0);
        g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeMissileFire, size );
    }   
}


// ****************************************************************************
// Class TurretShell
// ****************************************************************************

TurretShell::TurretShell(double _life)
:   WorldObject(),
    m_life(_life),
    m_stopRendering(false),
    m_fromBuildingId(-1),
    m_age(0.0f)
{
    m_type = EffectGunTurretShell;
}


void TurretShell::ExplodeShape( float fraction )
{
    Vector3 midPoint = m_pos - m_vel * SERVER_ADVANCE_PERIOD * 0.25f;

    Vector3 predictedFront = m_vel;
    predictedFront.Normalise();
 
    Vector3 right = predictedFront ^ g_upVector;
    Vector3 up = right ^ predictedFront;

    Shape *shape = g_app->m_resource->GetShape( "turretshell.shp" );

    Matrix34 shellMat( predictedFront, up, midPoint );

    g_explosionManager.AddExplosion( shape, shellMat, fraction );
}


bool TurretShell::Advance()
{
    //
    // Update our position

    Vector3 oldPos = m_pos;

    m_life -= SERVER_ADVANCE_PERIOD;
    m_age += SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    if( m_life <= 0.0 )
    {
        ExplodeShape( 0.2f ); 
        
        return true;
    }

    
    if ( m_pos.x < 0 || m_pos.x > g_app->m_location->m_landscape.GetWorldSizeX() ||
         m_pos.z < 0 || m_pos.z > g_app->m_location->m_landscape.GetWorldSizeZ()  )
    {
        // Outside of world
        return true;
    }
     

    //
    // Did we hit anyone?

    Vector3 centrePos = ( m_pos + oldPos ) / 2.0;
    double radius = ( m_pos - oldPos ).Mag() / 1.0;
    int numFound;
    g_app->m_location->m_entityGrid->GetEnemies( s_neighbours, centrePos.x, centrePos.z, radius, &numFound, m_id.GetTeamId() );
    
    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = s_neighbours[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        if( entity )
        {            
            Vector3 rayDir = m_vel;
            rayDir.Normalise();
            if( entity->RayHit( oldPos, rayDir ) )
            {
                bool dead = entity->m_dead;

                bool success = entity->ChangeHealth( -15, Entity::DamageTypeTurretShell );

                if( !dead && entity->m_dead && m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS ) 
                {
                    g_app->m_soundSystem->TriggerOtherEvent( entity->m_pos, "KilledEntity", SoundSourceBlueprint::TypeLaser, m_id.GetTeamId() );

                    int numKills = 1;
                    if( entity->m_type == Entity::TypeArmour )
                    {
                        numKills += ((Armour *)entity)->GetNumPassengers();
                    }
                    else if( entity->m_type == Entity::TypeDarwinian )
                    {
                        // no points for killing a burning darwinian, as this will have been attributed to something else
                        if( ((Darwinian *)entity)->IsOnFire() ) numKills = 0;
                    }
                        
                    g_app->m_location->m_teams[m_id.GetTeamId()]->m_teamKills += numKills;
                    
                    Building *building = g_app->m_location->GetBuilding( m_fromBuildingId );
                    if( building && building->m_type == Building::TypeGunTurret )
                    {
                        ((GunTurret *)building)->RegisterKill( entity->m_id.GetTeamId(), numKills );
                    }
                }

                if( success && entity->m_onGround )
                {
                    Vector3 push( entity->m_pos - m_pos );
                    push.Normalise();
                    push += Vector3( SFRAND(3.0), SFRAND(3.0), SFRAND(3.0) );
                    push.SetLength( 20.0 );
                    entity->m_vel += push;
                    entity->m_onGround = false;   
                }
            }
        }
    }

    Vector3 midPoint = (m_pos + oldPos) / 2.0f;

    //
    // Did we hit any buildings?
    
    {
        Vector3 rayDir = m_vel;
        rayDir.Normalise();
        

        LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

        for( int b = 0; b < buildings->Size(); ++b )
        {
            int buildingId = buildings->GetData(b);
            Building *building = g_app->m_location->GetBuilding( buildingId );
            if( building )
            {

                bool allowedHit = ( building->m_type == Building::TypeTree ||
                                    building->m_type == Building::TypeAntHill ||
                                    building->m_type == Building::TypeTriffid );

                Vector3 hitpos;

                if( building->DoesRayHit( oldPos, rayDir, (m_pos - oldPos).Mag(), &hitpos ) )
                {
                    for( int p = 0; p < 3; ++p )
                    {
                        Vector3 vel = ( m_pos - building->m_centrePos ).Normalise();
                        vel *= 50.0f;
                        vel.x += sfrand(10.0f);
                        vel.y += frand(10.0f);
                        vel.z += sfrand(10.0f);
                        vel *= 0.5f;
                        float size = 10.0f + frand(10.0f);
                        g_app->m_particleSystem->CreateParticle( hitpos, vel, Particle::TypeMissileTrail, size );        
                    }
                    if(allowedHit)
                    {
                        building->Damage( -2 );
                    }

                    ExplodeShape( 0.1f ); 
                    return true;
                }
            }
        }
    }


    //
    // Did we hit the landscape?

    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    double midLandHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(midPoint.x, midPoint.z);

    if (m_pos.y <= landHeight ||
        midPoint.y <= midLandHeight )
    {
        Vector3 groundPos;
        g_app->m_location->m_landscape.RayHit( oldPos, (m_pos - oldPos).Normalise(), &groundPos );

        Vector3 vel = g_app->m_location->m_landscape.m_normalMap->GetValue(groundPos.x, groundPos.z);
        vel.x += sfrand(0.5f);
        vel.y += frand(1.0f);
        vel.z += sfrand(0.5f);
        vel *= 10.0f;
        float size = 15.0f + frand(20.0f);
        g_app->m_particleSystem->CreateParticle( groundPos, vel, Particle::TypeRocketTrail, size );        
        
        m_pos = groundPos;
        ExplodeShape( 0.2f ); 

        return true;
    }


    return false;
}


void TurretShell::Render( double predictionTime )
{
    float predictionAdder = 0.5f - m_age;
    predictionAdder = max( predictionAdder, 0.0f );

    predictionTime += SERVER_ADVANCE_PERIOD * predictionAdder;
    
    //Vector3 predictedVel = m_vel;
    //predictedVel.y -= 30.0 * predictionTime;
    //Vector3 predictedPos = m_pos + predictedVel * predictionTime;
    //RenderSphere( predictedPos, 1.0 );    

    Vector3 midPoint = m_pos - m_vel * SERVER_ADVANCE_PERIOD * 0.25;
    
    Vector3 predictedPos = midPoint + m_vel * predictionTime;

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

    //
    // Are we controlling the turret that fired this shell?
    // If so render differently

    bool controllingTurret = false;

    Building *building = g_app->m_location->GetBuilding( m_fromBuildingId );
    if( building && 
        building->m_type == Building::TypeGunTurret )
    {
        Team *myTeam = g_app->m_location->GetMyTeam();
        if( myTeam &&
            myTeam->m_currentBuildingId == m_fromBuildingId )
        {
            controllingTurret = true;
        }
    }
        

    float lineLen = 0.05f;
    float alpha = 0.5f;

    if( controllingTurret )
    {
        lineLen *= 2;
        alpha = 1.0f;
    }

    glLineWidth( 2.0f );
    glShadeModel( GL_SMOOTH );
    glBegin( GL_LINES );
        glColor4f( 1.0f, 1.0f, 1.0f, alpha );    glVertex3dv( predictedPos.GetData() );
        glColor4f( 1.0f, 1.0f, 1.0f, 0.0f );     glVertex3dv( (predictedPos - m_vel * lineLen).GetData() );
    glEnd();
    glShadeModel( GL_FLAT );
    glLineWidth( 1.0 );
}

Fireball::Fireball()
:   WorldObject(),
    m_life(1.0),
    m_radius(3.0),
    m_createdParticles(false),
    m_fromBuildingId(-1)
{
	m_type = EffectFireball;
}

bool Fireball::Advance()
{
    if( !m_createdParticles )
    {
        m_createdParticles = true;
        for( int i = 0; i < 80; ++i )
        {
            Vector3 pos = m_pos;
            pos += m_vel;
            pos += Vector3( sfrand(20.0), sfrand(20.0), sfrand(20.0) );
            Vector3 vel = (pos - m_pos).SetLength( m_vel.Mag() );

            int particleType = Particle::TypeFireball;

            double size = 50 + sfrand(25.0);

            g_app->m_particleSystem->CreateParticle( m_pos + frand(0.2) * vel, vel, particleType, size );
        }
    }

    m_life -= SERVER_ADVANCE_PERIOD;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

	m_radius = 20.0 * std::min(1.0, iv_abs(1.0 - m_life));

	//char buf1[32], buf2[64], buf3[64], buf4[32];
	//SyncRandLog( "Fireball m_life = %s, m_vel = %s, m_pos = %s, m_radius = %s", 
	//	HashDouble(m_life, buf1), HashVector3(m_vel, buf2), HashVector3(m_pos, buf3), HashDouble(m_radius, buf4) );

    int numFound;
    g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, m_radius, &numFound );

	//SyncRandLog( "Fireball numFound = %d\n", numFound );

    bool registerKills = false;
    GunTurret *building = (GunTurret *)g_app->m_location->GetBuilding( m_fromBuildingId );
    if( building && building->m_type == Building::TypeGunTurret )
    {
        registerKills = true;
    }

    for( int i = 0; i < numFound; ++i )
    {
        Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
        if( d && !Damaged( d->m_id.GetUniqueId() ) )
        {
            double distance = (m_pos - d->m_pos).Mag();
            distance = max( 1.0, distance );
            double damageAmount = 30.0 * ( m_radius / distance ) * max(1.0, m_life);

            int oldState = d->m_state;
            d->FireDamage( damageAmount, m_id.GetTeamId() );
            m_damaged.PutData(d->m_id.GetUniqueId());
            if( oldState != Darwinian::StateOnFire &&
                d->m_state == Darwinian::StateOnFire && 
                registerKills )
            {
                building->RegisterKill( d->m_id.GetTeamId(), 1 );
            }
        }
    }

    LList<int> *buildings = g_app->m_location->m_obstructionGrid->GetBuildings( m_pos.x, m_pos.z );

    for( int b = 0; b < buildings->Size(); ++b )
    {
        int buildingId = buildings->GetData(b);
        Building *building = g_app->m_location->GetBuilding( buildingId );
        if( building && building->m_type == Building::TypeTree )
        {
            building->Damage(-100.0);
        }
    }

    // make some particles to create a cool fireball effect

   /* for( int i = 0; i < 5; ++i )
    {
        Vector3 pos = m_pos;
        pos += Vector3( sfrand(5.0), sfrand(5.0), sfrand(5.0) );

        int particleType = Particle::TypeFire;
        if( i > 3 ) particleType = Particle::TypeMissileTrail;

        double size = 50 + sfrand(25.0);

        g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, particleType, size );
    }*/

    bool groundHit = false;
    if( m_pos.y <= g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z ) )
    {
        groundHit = true;
        /*// we hit the ground, so explode a bit
        int numCores = 10;
        numCores += frand(numCores);
        for( int p = 0; p < numCores; ++p )
        {
            Vector3 vel( sfrand( 30.0 ), 
                         10.0 + frand( 10.0 ), 
                         sfrand( 30.0 ) );
            double size = 120.0 + frand(120.0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeFire, size );
        }

        int numDebris = 5;
        for( int p = 0; p < numDebris; ++p )
        {
            double angle = 2.0 * M_PI * p/(double)numDebris;

            Vector3 vel(1,0,0);
            vel.RotateAroundY( angle );
            vel.y = 2.0 + frand(2.0);
            vel.SetLength( 20.0 + frand(30.0) );

            double size = 20.0 + frand(20.0);

            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeExplosionDebris, size );
        }*/
    }

    return ( m_life <= 0.0 || groundHit );
}

bool Fireball::Damaged( int _darwinianId )
{
    for( int i = 0; i < m_damaged.Size(); ++i )
    {
        if( m_damaged[i] == _darwinianId ) return true;
    }

    return false;
}

// =================================================================

PulseWave *PulseWave::s_pulseWave = NULL;

PulseWave::PulseWave()
:   WorldObject(),
    m_radius(0.0)
{
    m_type = EffectPulseWave;
    m_birthTime = g_gameTime;

    s_pulseWave = this;
}

PulseWave::~PulseWave()
{
    s_pulseWave = NULL;
}

bool PulseWave::Advance()
{
    m_radius += m_radius * 0.03;
    return (m_radius >= g_app->m_location->m_landscape.GetWorldSizeX());
}

void PulseWave::Render( double _predictionTime )
{
    if( g_gameTime - m_birthTime < 0.3 &&
        !g_app->m_multiwinia->GameOver() )
    {
        double distance = ( g_app->m_camera->GetPos() - m_pos ).Mag();
        double distanceFactor = 1.0 - ( distance / 500.0 );
        if( distanceFactor < 0.0 ) distanceFactor = 0.0;
        
        double alpha = 1.0;
        
        glColor4f(1.0,1.0,1.0,alpha);
        g_app->m_renderer->SetupMatricesFor2D(true);
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

    double startAngle = (int)(g_gameTime) % int(2.0 * M_PI);
    startAngle += g_gameTime - startAngle;
    double totalAngle = 2.0 * M_PI;
    int numSteps = totalAngle * 3;
    RGBAColour colour;
    if( m_id.GetTeamId() >= 0 && m_id.GetTeamId() < NUM_TEAMS )
    {
        colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
    }

    glDisable( GL_CULL_FACE );
    glDepthMask(false);
    glEnable( GL_BLEND );

    //
    // Basic outline on ground

    double angle = startAngle;
    
    double timeFactor = g_gameTime;
    timeFactor *= 2.0;
    
    glBegin( GL_QUAD_STRIP );
    for( int i = 0; i <= numSteps; ++i )
    {        
        double colMod = 0.25 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.75);
        double alpha = 0.2 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.8);
        //if( m_radius > g_app->m_location->m_landscape.GetWorldSizeX() * 0.75 )
        {
            alpha *= 1.0 - ( m_radius / g_app->m_location->m_landscape.GetWorldSizeX() );
        }

        glColor4ub( colour.r * colMod, colour.g * colMod, colour.b * colMod, alpha*colour.a );

        double xDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_sin(angle);
        double zDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        pos.y -= 20;
        glVertex3dv( pos.GetData() );
        pos.y += 200;
        glVertex3dv( pos.GetData() );
        angle += totalAngle / (double) numSteps;
    }
    glEnd();


    //
    // Small glow tube

    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser-long.bmp" ) );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glDisable( GL_DEPTH_TEST );

    angle = startAngle;

    glBegin( GL_QUAD_STRIP );
    for( int i = 0; i <= numSteps; ++i )
    {
        double alpha = 0.5 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.5);
        //if( m_radius > g_app->m_location->m_landscape.GetWorldSizeX())
        {
            alpha *= 1.0 - ( m_radius / g_app->m_location->m_landscape.GetWorldSizeX() );
        }

        glColor4ub( colour.r, colour.g, colour.b, alpha*colour.a );

        double xDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_sin(angle);
        double zDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        pos.y -= 20;        
        glTexCoord2f(i + g_gameTime,0);       glVertex3dv( pos.GetData() );
        pos.y += 400;
        glTexCoord2f(i + g_gameTime,1);       glVertex3dv( pos.GetData() );
        angle += totalAngle / (double) numSteps;
    }
    glEnd();

    glDisable( GL_TEXTURE_2D );

    glEnable( GL_DEPTH_TEST );


    //
    // Glow tube running up to sky

    angle = startAngle;

    glBegin( GL_QUAD_STRIP );
    for( int i = 0; i <= numSteps; ++i )
    {
        double colMod = 0.25 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.75);
        double alpha = 0.2 + (iv_abs(iv_sin(angle * 2 + timeFactor)) * 0.7);
        //if( m_radius > g_app->m_location->m_landscape.GetWorldSizeX() * 0.75 )
        {
            alpha *= 1.0 - ( m_radius / g_app->m_location->m_landscape.GetWorldSizeX() );
        }
        alpha *= 0.1;

        glColor4ub( colour.r * colMod, colour.g * colMod, colour.b * colMod, alpha*colour.a );

        double xDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_sin(angle);
        double zDiff = (m_radius + (m_radius * 0.3 * _predictionTime)) * iv_cos(angle);
        Vector3 pos = m_pos + Vector3(xDiff,5,zDiff);
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos.x, pos.z);
        if( pos.y < 2 ) pos.y = 2;
        glVertex3dv( pos.GetData() );
        pos.y += 1000;
        glVertex3dv( pos.GetData() );
        angle += totalAngle / (double) numSteps;
    }
    glEnd();

    //glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnable( GL_CULL_FACE );
    glShadeModel( GL_FLAT );
    glDepthMask( true );
}


// ===========================================
// LandMine
// ===========================================

LandMine::LandMine()
:   WorldObject()
{
    m_shape = g_app->m_resource->GetShape( "flying_egg.shp" );
}

bool LandMine::Advance()
{
    int num = g_app->m_location->m_entityGrid->GetNumEnemies( m_pos.x, m_pos.z, 15.0, m_id.GetTeamId() );
    if( num > 0 )
    {
        g_app->m_location->Bang( m_pos, 20.0, 20.0, m_id.GetTeamId() );

        Matrix34 transform( m_front, m_up, m_pos );
        g_explosionManager.AddExplosion( m_shape, transform, 1.0 );
        return true;
    }

    return false;
}

void LandMine::Render(double _predictionTime)
{
    Matrix34 mat(m_front, m_up, m_pos);
	m_shape->Render(_predictionTime, mat);

    /*double signalSize = 6.0;
    Vector3 camR = g_app->m_camera->GetRight();
    Vector3 camU = g_app->m_camera->GetUp();

    if( m_id.GetTeamId() == 255 )
    {
        glColor3f( 0.5, 0.5, 0.5 );
    }
    else
    {
        glColor3ubv( g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour.GetData() );
    }

    Vector3 lightPos = m_pos;
    lightPos += m_up * 10.0;
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );

    for( int i = 0; i < 10; ++i )
    {
        double size = signalSize * (double) i / 10.0;
        glBegin( GL_QUADS );
            glTexCoord2f    ( 0.0, 0.0 );             glVertex3dv     ( (lightPos - camR * size - camU * size).GetData() );
            glTexCoord2f    ( 1.0, 0.0 );             glVertex3dv     ( (lightPos + camR * size - camU * size).GetData() );
            glTexCoord2f    ( 1.0, 1.0 );             glVertex3dv     ( (lightPos + camR * size + camU * size).GetData() );
            glTexCoord2f    ( 0.0, 1.0 );             glVertex3dv     ( (lightPos - camR * size + camU * size).GetData() );
        glEnd();
    }

    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );

    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glDisable       ( GL_TEXTURE_2D ); */
}


FlameGrenade::FlameGrenade( Vector3 const &_startPos, Vector3 const &_front, double _force )
:   Grenade( _startPos, _front, _force )
{
    m_colour.Set( 255, 100, 100 );
    m_type = EffectFlameGrenade;
}


bool FlameGrenade::Advance()
{
    m_life -= SERVER_ADVANCE_PERIOD;

    bool explode = m_life <= 0.0f;

    if( explode )
    {
        if( !g_app->m_editing )
		{
            TriggerSoundEvent( "Explode" );

            //g_app->m_location->Bang( m_pos, m_power/2.0f, m_power*2.0f, m_id.GetTeamId());
            int numFound;
            bool include[NUM_TEAMS];
            memset( include, true, sizeof(include) );
            g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, m_power * 1.25, &numFound, include );
            for( int i = 0; i < numFound; ++i )
            {
                Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
                if( d )
                {
                    d->SetFire();
                }
            }

            for( int i = 0; i < 80; ++i )
            {
                Vector3 vel( sfrand(50.0f), sfrand(20.0f), sfrand(50.0f) );
                int particleType = Particle::TypeFireball;
                if( i > 50 ) 
                {
                    particleType = Particle::TypeFire;
                    vel = Vector3( sfrand(50.0f), 0.0f, sfrand(50.0f) );
                }

                if( i > 70 )
                {
                    particleType = Particle::TypeMissileTrail;
                }

                float size = 150.0f + frand(100.0f);

                g_app->m_particleSystem->CreateParticle( m_pos + frand(0.2f) * vel, vel, particleType, size );
            }

			// Set flammable buildings on fire
			double maxBuildingRange = m_power * 2.0;
			for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
			{
				if( g_app->m_location->m_buildings.ValidIndex(i) )
				{	    
    				Building *building = g_app->m_location->m_buildings[i];
					if( building->m_type != Building::TypeTree &&
						building->m_type != Building::TypeTriffid )
						continue;

    				double dist = (m_pos - building->m_pos).Mag();
		            
					if( dist < maxBuildingRange )
					{
						//double fraction = (_range*3.0 - dist) / _range*3.0;
						double fraction = 1.0 - dist / maxBuildingRange;
						fraction = max( 0.0, fraction );
						fraction = min( 1.0, fraction );
						building->Damage( m_power * -4.0 );
					}
				}
			}
        }
        return true;
    }
    
    return ThrowableWeapon::Advance();
}


TurretEmptyShell::TurretEmptyShell( Vector3 const &pos, Vector3 const &front, Vector3 const &vel )
{
    m_pos = pos;
    m_front = front;
    m_vel = vel;
    m_force = 1.0f;
    m_up = front ^ vel;
    m_up.Normalise();

    m_type = EffectTurretEmptyShell;
    m_shape = g_app->m_resource->GetShape( "emptyshell.shp" );
}


bool TurretEmptyShell::Advance()
{
    if( m_force > 0.2 )
    {
        m_vel.y -= 9.8 * g_gameTimer.GetGameSpeed();
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        Vector3 axis = m_front;
        axis += Vector3( fmod( m_id.GetUniqueId() * 0.1, 1.0 ), 
                         fmod( m_id.GetUniqueId() * 0.3, 1.0 ),
                         fmod( m_id.GetUniqueId() * 0.7, 1.0 ) );

        m_up.RotateAround( axis * SERVER_ADVANCE_PERIOD * m_force );
        m_front.RotateAround( axis * SERVER_ADVANCE_PERIOD * m_force );

        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z );
        if( m_pos.y < landHeight + 1.0f )
        {
            BounceOffLandscape();
            m_force *= 0.5;
            m_vel *= m_force;
            if( m_pos.y < landHeight + 1.0f ) m_pos.y = landHeight + 1.0f;               
        }
    }    
    else
    {
        return true;
    }

    return false;
}


void TurretEmptyShell::Render( double _predictionTime )
{
    _predictionTime -= SERVER_ADVANCE_PERIOD;

    Vector3 right = m_up ^ m_front;
    //m_up.RotateAround( right * m_force * m_force * g_gameTimer.GetServerAdvancePeriod() * 1.0f );
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
    glDepthMask     (true);
    
    m_shape->Render( _predictionTime, transform );

    glDepthMask     (false);
    glEnable        (GL_BLEND);
    glDisable       (GL_COLOR_MATERIAL);
    g_app->m_renderer->UnsetObjectLighting();
}

