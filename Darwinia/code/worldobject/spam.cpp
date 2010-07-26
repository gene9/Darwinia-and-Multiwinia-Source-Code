#include "lib/universal_include.h"
#include "lib/debug_render.h"
#include "lib/text_renderer.h"
#include "lib/resource.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/hi_res_time.h"

#include "worldobject/spam.h"
#include "entity_grid.h"

#include "explosion.h"
#include "app.h"
#include "location.h"
#include "globals.h"
#include "entity_grid.h"
#include "team.h"
#include "particle_system.h"
#include "sepulveda.h"
#include "global_world.h"
#include "main.h"
#include "camera.h"
#include "worldobject/entity.h"

#include "sound/soundsystem.h"

static inline float SpamReloadTime()
{
	return SPAM_RELOADTIME - (SPAM_RELOADTIME / 2.0) * g_app->m_difficultyLevel / 10.0;
}

Spam::Spam()
:   Building(),
    m_timer(0.0f),
    m_damage(SPAM_DAMAGE),
    m_onGround(true),
    m_research(false),
    m_activated(true)
{
    m_type = TypeSpam;
    m_timer = syncfrand( SpamReloadTime() );

    m_front.RotateAroundY( frand(2.0f * M_PI) );

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
        m_pos.y += 20.0f;
    }

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}


void Spam::Damage( float _damage )
{
    bool dead = m_damage <= 0.0f;

    int oldHealthBand = int( m_damage / 10.0f );
    Building::Damage( _damage );
    m_damage += _damage;
    int newHealthBand = int( m_damage / 10.0f );

    if( newHealthBand <= 0 || newHealthBand < oldHealthBand )
    {
        float percentDead = 1.0f - m_damage / SPAM_DAMAGE;
        percentDead = min( percentDead, 1.0f );
        percentDead = max( percentDead, 0.0f );
        Matrix34 mat( m_front, g_upVector, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, percentDead );
    }

    if( !dead && m_damage <= 0.0f )
    {
        // We just died
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb ) gb->m_online = true;
        g_app->m_soundSystem->TriggerBuildingEvent( this, "Explode" );
    }
}

void Spam::Destroy( float _intensity )
{
	Building::Destroy( _intensity );
	m_damage = 0.0f;
}

void Spam::ListSoundEvents( LList<char *> *_list )
{
    _list->PutData( "Attack" );
    _list->PutData( "Explode" );
    _list->PutData( "CreateResearch" );
}


void Spam::Render( float _predictionTime )
{
//    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
//    Vector3 predictedFront = m_front;
//    predictedFront.RotateAroundY( _predictionTime );
//    Matrix34 mat( predictedFront, g_upVector, predictedPos );
//    m_shape->Render( _predictionTime, mat );

    Vector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX( g_gameTime * 1.0f );
    rotateAround.RotateAroundZ( g_gameTime * 0.7f );
    rotateAround.Normalise();

    m_front.RotateAround( rotateAround * g_advanceTime );
    m_up.RotateAround( rotateAround * g_advanceTime );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
	Matrix34 mat(m_front, m_up, predictedPos);

	m_shape->Render(0.0f, mat);
}


void Spam::RenderAlphas( float _predictionTime )
{
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,100,0), 10.0f, "timer %d", (int) m_timer );
    //g_editorFont.DrawText3DCentre( m_pos+Vector3(0,90,0), 10.0f, "Damage %d", (int) m_damage );

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();

    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );

    float timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0f;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 0;

    float alpha = 1.0f;

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
    Vector3 centreToMpos = m_pos - m_centrePos;

    for( int i = 0; i < maxBlobs; ++i )
    {
        Vector3 pos = predictedPos + centreToMpos;
        pos.x += sinf(timeIndex+i) * i * 0.3f;
        pos.y += cosf(timeIndex+i) * sinf(i*10) * 5;
        pos.z += cosf(timeIndex+i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex+i*10) * 7.0f;
        size = max( size, 2.0f );

        //glColor4f( 0.6f, 0.2f, 0.1f, alpha);
        //glColor4f( 0.9f, 0.2f, 0.2f, alpha);

		RGBAColour spamColour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
		glColor4f(spamColour.r, spamColour.g, spamColour.b, alpha);

        if( m_research ) glColor4f( 0.1f, 0.2f, 0.8f, alpha);

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    //
    // Starbursts

    alpha = 1.0f - m_timer / SpamReloadTime();
    alpha *= 0.3f;

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    int numStars = 10;
    if( buildingDetail == 2 ) numStars = 5;
    if( buildingDetail == 3 ) numStars = 2;

    for( int i = 0; i < numStars; ++i )
    {
        Vector3 pos = predictedPos + centreToMpos;
        pos.x += sinf(timeIndex+i) * i * 0.3f;
        pos.y += (cosf(timeIndex+i) * cosf(i*10) * 2);
        pos.z += cosf(timeIndex+i) * i * 0.3f;

        float size = i * 10 * alpha;
        if( i > numStars - 2 ) size = i * 20 * alpha;

        //glColor4f( 1.0f, 0.4f, 0.2f, alpha );
		//glColor4f( 0.8f, 0.2f, 0.2f, alpha); //"the" backup code in case we can't do anything about this shit below...

		//if( g_app->m_location )
    //{
	    //for (int i = 0; i < NUM_TEAMS; ++i)
	   // {
			RGBAColour spamColour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
			glColor4f((float) spamColour.r / 255.0,(float)  spamColour.g / 255.0,(float)  spamColour.b / 255.0, alpha);
        //}
    //}

        if( m_research ) glColor4f( 0.1f, 0.2f, 0.8f, alpha);

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
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
        vel += Vector3( syncsfrand(1.0f), syncfrand(2.0f), syncsfrand(1.0f) );
        vel.SetLength( 100.0f );
		//Team spam_team = g_app->m_location->GetMyTeam();

        SpamInfection *infection = new SpamInfection();
        infection->m_pos = m_centrePos;
        infection->m_vel = vel;
        infection->m_parentId = m_id.GetUniqueId();
		//infection->m_
        int index = g_app->m_location->m_effects.PutData( infection );
        infection->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
		//infection->m_id.SetTeamId(spam_team);
        infection->m_id.GenerateUniqueId();
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Attack" );
}


bool Spam::Advance()
{
    if( m_damage <= 0.0f ) return true;

    if( !m_onGround )
    {
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;

        float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        if( m_pos.y <= landHeight + 20.0f )
        {
            m_onGround = true;
            m_pos.y = landHeight + 20.0f;
            m_vel.x += syncsfrand(20.0f);
            m_vel.z += syncsfrand(30.0f);
            float speed = m_vel.Mag();
            m_vel.y = 0.0f;
            m_vel.SetLength( speed );
        }

        Matrix34 mat( m_front, m_up, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
    }
    else if( m_onGround )
    {
        if( m_vel.Mag() > 1.0f )
        {
            m_vel *= ( 1.0f - SERVER_ADVANCE_PERIOD * 0.3f );
            m_pos += m_vel * SERVER_ADVANCE_PERIOD;
            float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
            m_pos.y = landHeight + 20.0f;

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
                        m_vel += dir * 0.25f;
                    }
                }
            }
        }
    }

    bool readyToSpawn = m_activated;
    if( readyToSpawn )
    {
        m_timer -= SERVER_ADVANCE_PERIOD;
        if( m_timer <= 0.0f )
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
    m_retargetTimer(0.0f),
    m_life(SPAMINFECTION_LIFE),
    m_parentId(-1)
{
    m_retargetTimer = syncfrand(1.0f);

    m_type = EffectSpamInfection;
}


bool SpamInfection::SearchForEntities()
{
    m_targetId = g_app->m_location->m_entityGrid->GetBestEnemy( m_pos.x, m_pos.z,
                                                                SPAMINFECTION_MINSEARCHRANGE,
                                                                SPAMINFECTION_MAXSEARCHRANGE, 1 );

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
        float distance = ( building->m_pos - m_pos ).Mag();
        if( distance > 400.0f )
        {
            m_targetPos = building->m_pos;
            return true;
        }
    }

    m_targetPos = m_pos;
    m_targetPos.x += syncsfrand(200.0f);
    m_targetPos.y += syncsfrand(200.0f);
    m_targetPos.z += syncsfrand(200.0f);

    float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );
    m_targetPos.y = max( m_targetPos.y, landHeight );

    return true;
}


bool SpamInfection::SearchForSpirits()
{
    // We don't always want this thing to convert the spirits.
    // It's better if sometimes the spirits are used in eggs to create more virii
    // as it shows the reproduction system better
    if( syncfrand(2.0f) < 1.0f ) return false;

    int index = -1;
    float nearest = 9999.9f;

    for( int i = 0; i < g_app->m_location->m_spirits.Size(); ++i )
    {
        if( g_app->m_location->m_spirits.ValidIndex(i) )
        {
            Spirit *s = g_app->m_location->m_spirits.GetPointer(i);
            float theDist = ( s->m_pos - m_pos ).Mag();

            if( theDist <= SPAMINFECTION_MAXSEARCHRANGE &&
                theDist >= SPAMINFECTION_MINSEARCHRANGE &&
                theDist < nearest &&
                s->m_state == Spirit::StateFloating &&
                s->m_pos.y > 10.0f )
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
        m_retargetTimer = 1.0f;
        bool targetFound = false;
        if( syncfrand(2.0f) < 1.0f )
        {
            if( !targetFound ) targetFound = SearchForEntities();
            if( !targetFound ) targetFound = SearchForSpirits();
        }
        else
        {
            if( !targetFound ) targetFound = SearchForSpirits();
            if( !targetFound ) targetFound = SearchForEntities();
        }
        if( !targetFound ) targetFound = SearchForRandomPosition();
    }

    AdvanceToTargetPosition();
}


void SpamInfection::AdvanceAttackingEntity()
{
    //
    // Is our target alive and well?

	bool friendlyTarget = g_app->m_location->IsFriend(m_targetId.GetTeamId(),m_id.GetTeamId());
    Entity *target = g_app->m_location->GetEntity( m_targetId );
    if( !target || target->m_dead || friendlyTarget )
    {
        m_state = StateIdle;
        return;
    }

    m_targetPos = target->m_pos;
    bool arrived = AdvanceToTargetPosition();

	//Team *_team = g_app->m_location->m_teams[m_id.GetTeamId()];
	//WorldObject *wobj = GetEntity( _id );
   // Entity *ent = (Entity *) wobj;

    if( arrived )
    {
        //if( m_targetId.GetTeamId() != m_id.GetTeamId() )
		//bool friendly
		if (m_targetId.GetTeamId() != m_id.GetTeamId() && (g_app->m_location->GetEntity(m_targetId)->m_type == Entity::TypeDarwinian) )
        {
            // Green darwinian
            int darwinianResearch = g_app->m_globalWorld->m_research->CurrentLevel( GlobalResearch::TypeDarwinian );
            if( darwinianResearch > 2 && syncfrand(10.0f) < 5.0f )
            {
                g_app->m_location->SpawnEntities( target->m_pos, m_id.GetTeamId(), -1, Entity::TypeDarwinian, 1, target->m_vel, 0.0f );
                g_app->m_location->m_entityGrid->RemoveObject( m_targetId, target->m_pos.x, target->m_pos.z, target->m_radius );
                g_app->m_location->m_teams[m_targetId.GetTeamId()].m_others.MarkNotUsed( m_targetId.GetIndex() );
                delete target;
            }
            else
            {
                target->ChangeHealth( -999 );
            }
        }
		else if( (m_targetId.GetTeamId() != m_id.GetTeamId()))
        {
            // Player
            target->ChangeHealth( -999 );
        }

        int numFlashes = 5 + darwiniaRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0f), frand(15.0f), sfrand(15.0f) );
            float size = i * 30;
            Vector3 pos = m_pos + Vector3(0,50,0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeFire, size );
        }

        m_life += 1.0f;
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


    Spirit *spirit = g_app->m_location->m_spirits.GetPointer(m_spiritId);

    if( spirit->m_state != Spirit::StateFloating )
    {
        m_state = StateIdle;
        return;
    }

    m_targetPos = spirit->m_pos;

    bool arrived = AdvanceToTargetPosition();
	//Team *team = g_app->m_location->m_teams[m_id.GetTeamId];

    if( arrived )
    {
        int entityType = Entity::TypeVirii;
		if( syncfrand(20.0f) < 5.0f ) entityType = Entity::TypeDarwinian;
        else if( syncfrand(20.0f) < 1.0f ) entityType = Entity::TypeSpider;
        g_app->m_location->SpawnEntities( spirit->m_pos, m_id.GetTeamId(), -1, entityType, 1, g_zeroVector, 0.0f, 200.0f );
        g_app->m_location->m_spirits.MarkNotUsed(m_spiritId);

        int numFlashes = 5 + darwiniaRandom() % 5;
        for( int i = 0; i < numFlashes; ++i )
        {
            Vector3 vel( sfrand(15.0f), frand(15.0f), sfrand(15.0f) );
            float size = i * 30;
            Vector3 pos = m_pos + Vector3(0,50,0);
            g_app->m_particleSystem->CreateParticle( m_pos, vel, Particle::TypeFire, size );
        }

        m_life += 1.0f;
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

    Vector3 targetVel = ( m_targetPos - m_pos ).SetLength( 100.0f );
    float factor = SERVER_ADVANCE_PERIOD * 0.5f;
    m_vel = m_vel * (1.0f-factor) + targetVel * factor;
    m_pos += m_vel * SERVER_ADVANCE_PERIOD;

    float distance = ( m_targetPos - m_pos ).Mag();
    return( distance < 20.0f );
}


bool SpamInfection::Advance()
{
    switch( m_state )
    {
        case StateIdle:                     AdvanceIdle();
        case StateAttackingEntity:          AdvanceAttackingEntity();
        case StateAttackingSpirit:          AdvanceAttackingSpirit();
    }

    m_life -= SERVER_ADVANCE_PERIOD;
    if( m_life <= 0.0f ) return true;

    return false;
}


void SpamInfection::Render( float _time )
{
    Vector3 predictedPos = m_pos + m_vel * _time;

    //RenderSphere( predictedPos, 200.0f, RGBAColour(255,0,0,255) );
    //RenderArrow( predictedPos, m_targetPos, 1.0f, RGBAColour(255,255,255,255) );

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
        float size = 0.3f * numRepeats;
        for( int i = 1; i < maxLength; ++i )
        {
            float alpha = 1.0f - i / (float) maxLength;
            alpha *= 0.75f;
			RGBAColour spamColour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;
            glColor4f( spamColour.r, spamColour.g, spamColour.b, alpha );
            Vector3 thisPos = *m_positionHistory.GetPointer(i);
            Vector3 lastPos = *m_positionHistory.GetPointer(i-1);
            Vector3 rightAngle = (thisPos - lastPos) ^ ( camPos - thisPos );
            rightAngle.SetLength( size );
            glBegin( GL_QUADS );
                glTexCoord2f(0.2f, 0.0f);       glVertex3fv( (thisPos - rightAngle).GetData() );
                glTexCoord2f(0.2f, 1.0f);       glVertex3fv( (thisPos + rightAngle).GetData() );
                glTexCoord2f(0.8f, 1.0f);       glVertex3fv( (lastPos + rightAngle).GetData() );
                glTexCoord2f(0.8f, 0.0f);       glVertex3fv( (lastPos - rightAngle).GetData() );
            glEnd();
        }
        if( m_positionHistory.Size() > 0 )
        {
            //glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
			RGBAColour spamColour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;
            glColor4f( spamColour.r, spamColour.g, spamColour.b, 1.0f );
            Vector3 lastPos = *m_positionHistory.GetPointer(0);
            Vector3 thisPos = predictedPos;
            Vector3 rightAngle = (thisPos - lastPos) ^ ( camPos - thisPos );
            rightAngle.SetLength( size );
            glBegin( GL_QUADS );
                glTexCoord2f(0.2f, 0.0f);       glVertex3fv( (thisPos - rightAngle).GetData() );
                glTexCoord2f(0.2f, 1.0f);       glVertex3fv( (thisPos + rightAngle).GetData() );
                glTexCoord2f(0.8f, 1.0f);       glVertex3fv( (lastPos + rightAngle).GetData() );
                glTexCoord2f(0.8f, 0.0f);       glVertex3fv( (lastPos - rightAngle).GetData() );
            glEnd();
        }
    }

    //glDepthMask( true );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_DEPTH_TEST );
    glShadeModel( GL_FLAT );
}


