#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/resource.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/hi_res_time.h"
#include "lib/math/random_number.h"

#include "worldobject/spam.h"
#include "worldobject/virii.h"

#include "explosion.h"
#include "app.h"
#include "location.h"
#include "globals.h"
#include "entity_grid.h"
#include "team.h"
#include "particle_system.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "global_world.h"
#include "main.h"
#include "camera.h"

#include "sound/soundsystem.h"

static inline double SpamReloadTime()
{
	return SPAM_RELOADTIME - (SPAM_RELOADTIME / 2.0) * g_app->m_difficultyLevel / 10.0;
}

Spam::Spam()
:   Building(),
    m_timer(0.0),
    m_damage(SPAM_DAMAGE),
    m_onGround(true),
    m_research(false),
    m_activated(true)
{
    m_type = TypeSpam;
    m_timer = syncfrand( SpamReloadTime() );
    
    m_front.RotateAroundY( frand(2.0 * M_PI) );

    SetShape( g_app->m_resource->GetShape( "researchitem.shp" ) );
}


void Spam::Initialise( Building *_template )
{
    Building::Initialise( _template );
}


void Spam::SetDetail( int _detail )
{
    if( m_onGround )
    {
        m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        m_pos.y += 20.0;
    }

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );        
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );        
}


void Spam::Damage( double _damage )
{
    bool dead = m_damage <= 0.0;
    
    int oldHealthBand = int( m_damage / 10.0 );
    Building::Damage( _damage );
    m_damage += _damage;
    int newHealthBand = int( m_damage / 10.0 );

    if( newHealthBand <= 0 || newHealthBand < oldHealthBand )
    {
        double percentDead = 1.0 - m_damage / SPAM_DAMAGE;
        percentDead = min( percentDead, 1.0 );
        percentDead = max( percentDead, 0.0 );
        Matrix34 mat( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, percentDead );
    }
    
    if( !dead && m_damage <= 0.0 )
    {
        // We just died
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_online = true;
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );
    }
}

void Spam::Destroy( double _intensity )
{
	Building::Destroy( _intensity );
	m_damage = 0.0;
}

void Spam::ListSoundEvents( LList<char *> *_list )
{
    _list->PutData( "Attack" );
    _list->PutData( "Explode" );
    _list->PutData( "CreateResearch" );
}


void Spam::Render( double _predictionTime )
{
//    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
//    Vector3 predictedFront = m_front;
//    predictedFront.RotateAroundY( _predictionTime );
//    Matrix34 mat( predictedFront, g_upVector, predictedPos );
//    m_shape->Render( _predictionTime, mat );

    Vector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX( g_gameTime * 1.0 );    
    rotateAround.RotateAroundZ( g_gameTime * 0.7 );
    rotateAround.Normalise();
    
    m_front.RotateAround( rotateAround * g_advanceTime );
    m_up.RotateAround( rotateAround * g_advanceTime );
        
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
	Matrix34 mat(m_front, m_up, predictedPos);

	m_shape->Render(0.0, mat);
}


void Spam::RenderAlphas( double _predictionTime )
{
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,100,0), 10.0, "timer %d", (int) m_timer );
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,90,0), 10.0, "Damage %d", (int) m_damage );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 0;

    double alpha = 1.0;

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Vector3 centreToMpos = m_pos - m_centrePos;

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = predictedPos + centreToMpos;
        pos.x += iv_sin(timeIndex+i) * i * 0.3;
        pos.y += iv_cos(timeIndex+i) * iv_sin(i*10) * 5;
        pos.z += iv_cos(timeIndex+i) * i * 0.3;

        double size = 5.0 + iv_sin(timeIndex+i*10) * 7.0;
        size = max( size, 2.0 );
        
        //glColor4f( 0.6, 0.2, 0.1, alpha);
        glColor4f( 0.9, 0.2, 0.2, alpha);
        
        if( m_research ) glColor4f( 0.1, 0.2, 0.8, alpha);

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    
    //
    // Starbursts

    alpha = 1.0 - m_timer / SpamReloadTime();
    alpha *= 0.3;

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    int numStars = 10;
    if( buildingDetail == 2 ) numStars = 5;
    if( buildingDetail == 3 ) numStars = 2;
    
    for( int i = 0; i < numStars; ++i )
    {
        Vector3 pos = predictedPos + centreToMpos;
        pos.x += iv_sin(timeIndex+i) * i * 0.3;
        pos.y += (iv_cos(timeIndex+i) * iv_cos(i*10) * 2);
        pos.z += iv_cos(timeIndex+i) * i * 0.3;

        double size = i * 10 * alpha;
        if( i > numStars - 2 ) size = i * 20 * alpha;
        
        //glColor4f( 1.0, 0.4, 0.2, alpha );
        glColor4f( 0.8, 0.2, 0.2, alpha);

        if( m_research ) glColor4f( 0.1, 0.2, 0.8, alpha);
        
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    glDisable( GL_TEXTURE_2D );
}


void Spam::SpawnInfection()
{
    if( m_research ) return;
    
    for( int i = 0; i < 20; ++i )
    {
        Vector3 vel = g_upVector;
        vel += Vector3( SFRAND(1.0), FRAND(2.0), SFRAND(1.0) );
        vel.SetLength( 100.0 );

        SpamInfection *infection = new SpamInfection();
        infection->m_pos = m_pos;
        infection->m_vel = vel;
        infection->m_parentId = m_id.GetUniqueId();
        int index = g_app->m_location->m_effects.PutData( infection );
        infection->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
        infection->m_id.GenerateUniqueId();
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Attack" );
}


bool Spam::Advance()
{
    if( m_damage <= 0.0 ) return true;

    if( !m_onGround )
    {
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        
        double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        if( m_pos.y <= landHeight + 20.0 )
        {
            m_onGround = true;
            m_pos.y = landHeight + 20.0;
            m_vel.x += syncsfrand(20.0);
            m_vel.z += syncsfrand(30.0);
            double speed = m_vel.Mag();
            m_vel.y = 0.0;
            m_vel.SetLength( speed );
        }

        Matrix34 mat( m_front, m_up, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );        
    }
    else if( m_onGround )
    {
        if( m_vel.Mag() > 1.0 )
        {
            m_vel *= ( 1.0 - SERVER_ADVANCE_PERIOD * 0.3 );
            m_pos += m_vel * SERVER_ADVANCE_PERIOD;
            double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
            m_pos.y = landHeight + 20.0;

            Matrix34 mat( m_front, m_up, m_pos );
            m_centrePos = m_shape->CalculateCentre( mat );        
        }
        else
        {
            m_vel.Zero();
        }

        //
        // Push from nearby SPAM

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b && b->m_type == TypeSpam )
                {
                    bool intersect = SphereSphereIntersection( m_centrePos, m_radius, b->m_centrePos, b->m_radius );
                    if( intersect )
                    {
                        Vector3 dir = ( m_pos - b->m_pos );
                        m_vel += dir * 0.25;
                    }
                }
            }
        }
    }
    
    bool readyToSpawn = m_activated;
    if( readyToSpawn )
    {
        m_timer -= SERVER_ADVANCE_PERIOD;
        if( m_timer <= 0.0 )
        {
            if( readyToSpawn )
            {
                m_timer = SpamReloadTime();
                SpawnInfection();
            }
        }
    }

    if( m_research )
    {
        g_app->m_soundSystem->StopAllSounds( m_id, "Spam Create" );
    }

    return Building::Advance();
}


void Spam::SendFromHeaven()
{
    m_onGround = false;
    m_activated = false;
}


void Spam::SetAsResearch()
{
    m_research = true;
    m_activated = false;

    g_app->m_soundSystem->TriggerBuildingEvent( this, "CreateResearch" );
}


// ============================================================================


SpamInfection::SpamInfection()
:   WorldObject(),
    m_state(StateIdle),
    m_retargetTimer(0.0),
    m_life(SPAMINFECTION_LIFE),
    m_parentId(-1),
    m_startCounter(0)
{
    m_retargetTimer = syncfrand(1.0);

    m_type = EffectSpamInfection;

    if( g_app->Multiplayer() )
    {
        m_startCounter = 1;
    }
}


bool SpamInfection::SearchForEntities()
{
    int teamId = 1;
    if( g_app->Multiplayer() )
    {
        teamId = g_app->m_location->GetMonsterTeamId();
    }

    m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z, 
                                                                SPAMINFECTION_MINSEARCHRANGE, 
                                                                SPAMINFECTION_MAXSEARCHRANGE, teamId );

    if( m_targetId.IsValid() )
    {
        m_state = StateAttackingEntity;
        return true;
    }

    return false;
}


bool SpamInfection::SearchForRandomPosition()
{
    Building *building = g_app->m_location->GetBuilding( m_parentId );
    if( building )
    {
        double distance = ( building->m_pos - m_pos ).Mag();
        if( distance > 400.0 )
        {
            m_targetPos = building->m_pos;
            return true;
        }
    }

    m_targetPos = m_pos;
    m_targetPos.x += syncsfrand(200.0);
    m_targetPos.y += syncsfrand(200.0);
    m_targetPos.z += syncsfrand(200.0);

    double landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
    m_targetPos.y = max( m_targetPos.y, landHeight );
    
    return true;
}


bool SpamInfection::SearchForSpirits()
{
    // We don't always want this thing to convert the spirits.
    // It's better if sometimes the spirits are used in eggs to create more virii
    // as it shows the reproduction system better

    // Only do this in single player though
    if( syncfrand(2.0) < 1.0 &&
        !g_app->Multiplayer() ) return false;

    int index = -1;
    double nearest = 9999.9;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            double theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= SPAMINFECTION_MAXSEARCHRANGE &&
                theDist >= SPAMINFECTION_MINSEARCHRANGE &&
                theDist < nearest &&
                s->m_state == Spirit::StateFloating &&
                s->m_pos.y > 10.0 )
            {                
                index = i;
                nearest = theDist;            
            }
        }
    }            

    if( index != -1 )
    {        
        m_spiritId = index;
        m_state = StateAttackingSpirit;
        return true;
    }    

    return false;
}


void SpamInfection::AdvanceIdle()
{
    m_retargetTimer -= SERVER_ADVANCE_PERIOD;
    if( m_retargetTimer <= 0.0 )
    {
        m_retargetTimer = 1.0;
        bool targetFound = false;

        if( m_startCounter > 0 )
        {
            m_startCounter--;
        }
        else
        {
            if( syncfrand(2.0) < 1.0 )
            {
                if( !targetFound ) targetFound = SearchForEntities();
                if( !targetFound ) targetFound = SearchForSpirits();
            }
            else
            {
                if( !targetFound ) targetFound = SearchForSpirits();
                if( !targetFound ) targetFound = SearchForEntities();
            }
        }

        if( !targetFound ) targetFound = SearchForRandomPosition();
    }

    AdvanceToTargetPosition();
}   


void SpamInfection::AdvanceAttackingEntity()
{
    //
    // Is our target alive and well?

    Entity *target = g_app->m_location->GetEntity( m_targetId );
    if( !target || target->m_dead || target->m_type == Entity::TypeHarvester )
    {
        m_state = StateIdle;
        return;
    }

    int teamId = 1;
    if( g_app->Multiplayer() )
    {
        teamId = g_app->m_location->GetMonsterTeamId();
    }

    m_targetPos = target->m_pos;
    bool arrived = AdvanceToTargetPosition();

    if( arrived )
    {
        bool validTarget = (m_targetId.GetTeamId() == 0);
        
        if( g_app->Multiplayer() )
        {
            validTarget = ( m_targetId.GetTeamId() != m_id.GetTeamId() );
        }

        if( validTarget )
        {
            // Green darwinian
            int darwinianResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian );
            if( darwinianResearch > 2 && syncfrand(10.0) < 5.0 )
            {
                g_app->m_location->SpawnEntities( target->m_pos, teamId, -1, Entity::TypeDarwinian, 1, target->m_vel, 0.0 );            
                target->m_destroyed = true;
            }
            else
            {
                target->ChangeHealth( -999 );
            }
        }
        else if( m_targetId.GetTeamId() == 2 && !g_app->Multiplayer() )
        {
            // Player
            target->ChangeHealth( -999 );
        }

        int numFlashes = 5 + AppRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0), frand(15.0), sfrand(15.0) );
            double size = i * 30;
            Vector3 pos = m_pos + Vector3(0,50,0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeFire, size );
        }
        
        m_life += 1.0;
        m_state = StateIdle;
    }
}


void SpamInfection::AdvanceAttackingSpirit()
{
    //
    // Is our spirit still alive and well?

    if( !g_app->m_location->m_spirits.ValidIndex(m_spiritId) )
    {   
        m_state = StateIdle;
        return;
    }

    int teamId = 1;
    if( g_app->Multiplayer() )
    {
        teamId = g_app->m_location->GetMonsterTeamId();
    }


    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);
    
    if( spirit->m_state != Spirit::StateFloating )
    {
        m_state = StateIdle;
        return;
    }
    
    m_targetPos = spirit->m_pos;

    bool arrived = AdvanceToTargetPosition();
    if( arrived )
    {
        bool noSpawn = false;
        int entityType = Entity::TypeVirii;
        if( syncfrand(20.0) < 1.0 ) entityType = Entity::TypeSpider;  
        if( g_app->Multiplayer() )
        {
            if( Virii::s_viriiPopulation[teamId] >= VIRII_POPULATIONLIMIT &&
                entityType == Entity::TypeVirii )
            {
                noSpawn = true;
            }
        }

        if( !noSpawn )
        {
            g_app->m_location->SpawnEntities( spirit->m_pos, teamId, -1, entityType, 1, g_zeroVector, 0.0, 200.0 );
        }
        g_app->m_location->m_spirits.RemoveData(m_spiritId);
        
        int numFlashes = 5 + AppRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0), frand(15.0), sfrand(15.0) );
            double size = i * 30;
            Vector3 pos = m_pos + Vector3(0,50,0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeFire, size );
        }
        
        m_life += 1.0;
        m_state = StateIdle;
    }
}


bool SpamInfection::AdvanceToTargetPosition()
{
    m_positionHistory.PutDataAtStart( m_pos );
    int maxLength = SPAMINFECTION_TAILLENGTH;   
    for( int i = maxLength; i < m_positionHistory.Size(); ++i )
    {
        m_positionHistory.RemoveData(i);
    }

    Vector3 targetVel = ( m_targetPos - m_pos ).SetLength( 200.0 );
    double factor = SERVER_ADVANCE_PERIOD * 0.5;
    m_vel = m_vel * (1.0-factor) + targetVel * factor;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;
    
    double distance = ( m_targetPos - m_pos ).Mag();
    return( distance < 20.0 );
}


bool SpamInfection::Advance()
{
    switch( m_state )
    {
        case StateIdle:                     AdvanceIdle();              break;
        case StateAttackingEntity:          AdvanceAttackingEntity();   break;
        case StateAttackingSpirit:          AdvanceAttackingSpirit();   break;
    }

    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0 ) return true;

    return false;
}


void SpamInfection::Render( double _time )
{
    Vector3 predictedPos = m_pos + m_vel * _time;

    //RenderSphere( predictedPos, 200.0, RGBAColour(255,0,0,255) );
    //RenderArrow( predictedPos, m_targetPos, 1.0, RGBAColour(255,255,255,255) );

    glShadeModel( GL_SMOOTH );
    glEnable( GL_BLEND );
    //glDepthMask( false );
    int maxLength = SPAMINFECTION_TAILLENGTH * (m_life / SPAMINFECTION_LIFE);    
    maxLength = max( maxLength, 2 );
    maxLength = min( maxLength, m_positionHistory.Size() );

    Vector3 camPos = g_app->m_camera->GetPos();
    int numRepeats = 4;

    glEnable( GL_TEXTURE_2D );
    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    for( int j = 0; j < numRepeats; ++j )
    {
        double size = 0.3 * numRepeats;
        for( int i = 1; i < maxLength; ++i )
        {
            double alpha = 1.0 - i / (double) maxLength;
            alpha *= 0.75;
            glColor4f( 1.0, 0.1, 0.1, alpha );
            if( g_app->Multiplayer() ) 
            {
                glColor4f( 0.0f, 0.0f, 0.0f, alpha*0.5f );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            }

            Vector3 thisPos = *m_positionHistory.GetPointer(i);
            Vector3 lastPos = *m_positionHistory.GetPointer(i-1);
            Vector3 rightAngle = (thisPos - lastPos) ^ ( camPos - thisPos );
            rightAngle.SetLength( size );
            glBegin( GL_QUADS );
                glTexCoord2f(0.2, 0.0);       glVertex3dv( (thisPos - rightAngle).GetData() );
                glTexCoord2f(0.2, 1.0);       glVertex3dv( (thisPos + rightAngle).GetData() );
                glTexCoord2f(0.8, 1.0);       glVertex3dv( (lastPos + rightAngle).GetData() );
                glTexCoord2f(0.8, 0.0);       glVertex3dv( (lastPos - rightAngle).GetData() );
            glEnd();
        }
        if( m_positionHistory.Size() > 0 )
        {
            glColor4f( 1.0, 0.0, 0.0, 1.0 );
            if( g_app->Multiplayer() ) 
            {
                glColor4f( 0.0f, 0.0f, 0.0f, 0.4f );
            }

            Vector3 lastPos = *m_positionHistory.GetPointer(0);
            Vector3 thisPos = predictedPos;
            Vector3 rightAngle = (thisPos - lastPos) ^ ( camPos - thisPos );
            rightAngle.SetLength( size );
            glBegin( GL_QUADS );
                glTexCoord2f(0.2, 0.0);       glVertex3dv( (thisPos - rightAngle).GetData() );
                glTexCoord2f(0.2, 1.0);       glVertex3dv( (thisPos + rightAngle).GetData() );
                glTexCoord2f(0.8, 1.0);       glVertex3dv( (lastPos + rightAngle).GetData() );
                glTexCoord2f(0.8, 0.0);       glVertex3dv( (lastPos - rightAngle).GetData() );
            glEnd();
        }
    }
    
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );

    //glDepthMask( true );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
    glShadeModel( GL_FLAT );
}


