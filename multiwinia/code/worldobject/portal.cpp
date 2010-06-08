#include "lib/universal_include.h"

#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"
#include "lib/resource.h"
#include "lib/text_renderer.h"

#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"

#include "app.h"
#include "camera.h"
#include "entity_grid.h"
#include "explosion.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "multiwinia.h"
#include "particle_system.h"
#include "renderer.h"
#include "resource.h"
#include "team.h"
#include "landscape.h"
#include "unit.h"

#include "worldobject/darwinian.h"
#include "worldobject/portal.h"
#include "worldobject/shaman.h"
#include "worldobject/tentacle.h"
#include "worldobject/triffid.h"

Portal::Portal()
:   Building(),
    m_portalRadius(0.0),
    //m_portalHeightModifier(0.0),
    m_finalSummonState(SummonStatePortalExpand),
    m_explosionTimer(0.0)
{
    m_type = Building::TypePortal;
    //SetShape( g_app->m_resource->GetShape("portal.shp") );

//    memset( m_vunerable, false, sizeof(bool) * NUM_TEAMS );
 //   memset( m_summonType, Entity::TypeInvalid, sizeof(int) * NUM_TEAMS );
  //  memset( m_souls, 0, sizeof(int) * NUM_TEAMS );
}

bool Portal::Advance()
{
    if( m_finalSummonState > SummonStateInactive )
    {
        AdvanceFinalSummon();
    }

    bool dead = Building::Advance();
    if( dead )
    {
        g_app->m_location->AddBurnPatch( m_pos, m_portalRadius * 2.0 );
    }

    return dead;
}

void Portal::AdvanceFinalSummon()
{
    switch( m_finalSummonState )
    {
        case SummonStateLoweringPortal: AdvanceStateLowering();         break;
        case SummonStatePortalExpand:   AdvanceStateExpanding();        break;
        case SummonStateThrash:         AdvanceStateThrashing();        break;
        case SummonStateExplode:        AdvanceStateExploding();        break;
        case SummonStateCoolDown:       AdvanceStateCooldown();         break;
    }

    if( m_finalSummonState > SummonStateLoweringPortal )
    {
        Vector3 centre = GetPortalCenter();
        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, centre.x, centre.z, m_portalRadius, &numFound );
        for( int i = 0; i < numFound; ++i )
        {
            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
            if( d )
            {
                d->SetFire();
            }
        }
    }
}

void Portal::AdvanceStateLowering()
{
/*    m_portalHeightModifier += 1.0;
    if( m_portalHeightModifier >= 50.0 )
    {
        m_finalSummonState++;
    }*/
}

void Portal::AdvanceStateExpanding()
{
    m_portalRadius += 1.0;
    if( m_portalRadius >= 100.0 )
    {
        m_finalSummonState++;
        m_explosionTimer = 45.0;

        double degreesPerTentacle = (2 * M_PI) / NUM_TENTACLES;
        for( int i = 0; i < NUM_TENTACLES; ++i )
        {
	        double degrees = -(M_PI/2.0) + ( i * degreesPerTentacle );

            Vector3 pos = GetPortalCenter();
	        pos.x += iv_cos(degrees) * (m_portalRadius - 10.0);
	        pos.z += iv_sin(degrees) * (m_portalRadius - 10.0);

            int num = 12;

            int unitId;
            int teamId = g_app->m_location->GetMonsterTeamId();
            Unit *unit = g_app->m_location->m_teams[teamId]->NewUnit( Entity::TypeTentacle, num, &unitId, pos );
            g_app->m_location->SpawnEntities( pos, teamId, unitId, Entity::TypeTentacle, num, g_zeroVector, 10.0);
            for( int j = 0; j < unit->m_entities.Size(); ++j )
            {
                Tentacle *t = (Tentacle *)unit->m_entities[j];
                if( t && t->m_type == Entity::TypeTentacle )
                {
                    t->SetPortalId( m_id.GetUniqueId() );

                    Matrix34 mat( t->m_front, g_upVector, t->m_pos );
                    g_explosionManager.AddExplosion( t->m_shape, mat, 3.0 );
                }
            }
        }
    }
}

void Portal::AdvanceStateThrashing()
{
    m_explosionTimer -= SERVER_ADVANCE_PERIOD;
    if( m_explosionTimer <= 0.0 )
    {
        m_finalSummonState++;
        m_explosionTimer = 10.0;
    }
}

void Portal::AdvanceStateExploding()
{
    m_explosionTimer -= SERVER_ADVANCE_PERIOD;
    if( m_explosionTimer <= 0.0 )
    {
        g_app->m_location->Bang( GetPortalCenter(), 200.0, 255.0, m_id.GetTeamId() );
        m_finalSummonState++;
        m_explosionTimer = 1.0;
    }
}

void Portal::AdvanceStateCooldown()
{
    m_explosionTimer -= SERVER_ADVANCE_PERIOD;
    if( m_explosionTimer <= 0.0 )
    {
        m_destroyed = true;
    }
}

Vector3 Portal::GetPortalCenter()
{
    if( m_portalCentre != g_zeroVector ) return m_portalCentre;

    Vector3 portalPos = m_pos;
    //portalPos.y += 50 - m_portalHeightModifier;
    return portalPos;
}

void Portal::RenderAlphas(double _predictionTime)
{
    Building::RenderAlphas( _predictionTime );
    if( g_app->m_editing )
    {
        RenderSphere( m_pos, m_radius, RGBAColour(255,255,255,255) );
    }

    if( m_finalSummonState == SummonStateExplode )
    {
        RenderBeam();
        RenderExplosionBuildup( _predictionTime );
    }
    else if( m_finalSummonState == SummonStateCoolDown )
    {
        glColor4f(1.0,1.0,1.0,1.0);
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

        g_app->m_camera->CreateCameraShake( 1.0 );
    }


//    RenderLinks( _predictionTime );

    Vector3 portalPos = GetPortalCenter();

    Vector3 up = g_upVector;
    Vector3 right = up ^ m_front;

    double size = 5.0;
    double circumference = (size + m_portalRadius ) * 2 * M_PI;
    double increment = circumference / (size );
    double degreesPerStep = (2 * M_PI ) / increment;
    int steps = increment + 1;

    for( int s = 0; s < steps; ++s )
    {
        if( rand() % 20 == 0 )
        {
            Vector3 centrePos = portalPos;
            double d = degreesPerStep * s + g_gameTime;
            centrePos += iv_sin(d) * right * m_portalRadius;
            centrePos += iv_cos(d) * m_front * m_portalRadius;
            centrePos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( centrePos.x, centrePos.z );

            double size = frand(150.0) + 150.0;
            int type = Particle::TypePortalFire;
            Vector3 vel = (centrePos - m_pos).SetLength(15.0);
            if( rand() % 4 == 0 )
            {
                type = Particle::TypeMissileTrail;
                vel.y += 10.0;
            }
            g_app->m_particleSystem->CreateParticle( centrePos, vel, type, size );
        }
    }

    for( int i = 0; i < 15; ++i )
    {
        Vector3 centrePos = portalPos;
        double d = frand(g_gameTime);
        centrePos += iv_sin(d) * right * frand(m_portalRadius);
        centrePos += iv_cos(d) * m_front * frand(m_portalRadius);
        centrePos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( centrePos.x, centrePos.z );

        double size = frand(50.0) + 150.0;
        int type = Particle::TypeHotFeet;
        Vector3 vel = (centrePos - m_pos).SetLength(5.0);
        if( rand() % 4 == 0 )
        {
            type = Particle::TypeMissileTrail;
        }
        g_app->m_particleSystem->CreateParticle( centrePos, vel, type, size );
    }
}

void Portal::RenderExplosionBuildup(double _predictionTime)
{
    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/cloudyglow.bmp" ) );
    glDisable       ( GL_DEPTH_TEST );

    double timeIndex = g_gameTime * 2;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 50;
    if( buildingDetail == 2 ) maxBlobs = 25;
    if( buildingDetail == 3 ) maxBlobs = 10;
    

    //
    // Calculate alpha value

    double alpha = (10.0 - m_explosionTimer) * 0.1;
    alpha = min( alpha, 1.0 );
    
    //
    // Central glow effect

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = GetPortalCenter();

        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_abs(iv_cos(timeIndex+i) * iv_cos(i*20) * 64);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = 20.0 * iv_sin(timeIndex+i*13);
        size *= ( (10.0 - m_explosionTimer) / 10.0 );
        size = max( size, 5.0 );
        
        glColor4f( 0.1, 0.2, 0.6, alpha);        
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    
    //
    // Central starbursts

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    int numStars = 10;
    if( buildingDetail == 2 ) numStars = 5;
    if( buildingDetail == 3 ) numStars = 2;
    
    for( int i = 0; i < numStars; ++i )
    {
        Vector3 pos = GetPortalCenter();
        pos.y += 50.0;

        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_abs(iv_cos(timeIndex+i) * iv_cos(i*20) * 64);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = i * 30.0;
        size *= ( (10.0 - m_explosionTimer) / 10.0 );
        
        glColor4f( 0.2, 0.4, 1.0, alpha );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }

    glEnable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
}

void Portal::RenderBeam()
{
    
    if( m_finalSummonState == SummonStateThrash ||
        m_finalSummonState == SummonStateExplode )
    {
        Vector3 from = m_pos;
        from.y = g_app->m_location->m_landscape.m_heightMap->GetValue( from.x, from.z );
        from.y -= 20.0;

        Vector3 to = m_pos;
        to.y += 100.0;
        RenderBeam( from, to );
    } 
}

void Portal::RenderBeam( Vector3 _from, Vector3 _to )
{
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glDisable       ( GL_DEPTH_TEST );
    glDepthMask     ( false );

    /*double nearPlaneStart = g_app->m_renderer->GetNearPlane();
    g_app->m_camera->SetupProjectionMatrix(nearPlaneStart * 1.5,
						 			       g_app->m_renderer->GetFarPlane());*/

    Vector3 ourPos = _from;
    Vector3 theirPos = _to;

    Vector3 camToTheirPos = g_app->m_camera->GetPos() - theirPos;
    Vector3 lineTheirPos = camToTheirPos ^ ( ourPos - theirPos );
    lineTheirPos.SetLength( 10.0 );

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/beam.bmp" ) );
        
    for( int i = 0; i < 10; ++i )
    {
        Vector3 pos = theirPos;
        pos.x += iv_sin( g_gameTime + i ) * 10;
        pos.z += iv_cos( g_gameTime + i ) * 10;


        glColor4f( 0.1, 0.2, 0.6, 0.65 );                              

        glBegin( GL_QUADS );
            glTexCoord2i(1,0);      glVertex3dv( (ourPos - lineTheirPos).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (ourPos + lineTheirPos).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos + lineTheirPos).GetData() );
            glTexCoord2i(0,0);      glVertex3dv( (pos - lineTheirPos).GetData() );
        glEnd();
    }

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    glColor4f( 0.1, 0.2, 0.8, 0.05);        

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
    double timeIndex = g_gameTime * 2;

    for( int i = 0; i < 3; ++i )
    {
        Vector3 pos = (theirPos + ourPos) / 2.0;
        pos.x += iv_sin(timeIndex+i) * i * 1.7;
        pos.y += iv_cos(timeIndex+i);
        pos.z += iv_cos(timeIndex+i) * i * 1.7;

        double size = (theirPos - ourPos).Mag();
        size = max( size, 5.0 );

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camUp * size - camRight * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camUp * size - camRight * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camUp * size + camRight * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camUp * size + camRight * size).GetData() );
        glEnd();        
    }

    /*g_app->m_camera->SetupProjectionMatrix(nearPlaneStart,
							 		       g_app->m_renderer->GetFarPlane());*/

    glDepthMask     ( true );
    glEnable        ( GL_DEPTH_TEST );
    glDisable       ( GL_TEXTURE_2D );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );  
}

void Portal::SpecialSummon( int _teamId )
{
    m_finalSummonState = SummonStatePortalExpand;
    m_portalRadius = 0.0;

    Team *team = g_app->m_location->m_teams[m_id.GetTeamId()];
    for( int i = 0; i < team->m_specials.Size(); ++i )
    {
        Shaman *e = (Shaman *)g_app->m_location->GetEntitySafe( *team->m_specials.GetPointer(i), Entity::TypeShaman );
        if( e )
        {
            m_portalCentre = e->m_pos;
//            m_targetShaman = e->m_id;
            e->Paralyze();
            break;
        }
    }

    // cancel all other final-summons in progress at this portal
    /*for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( i == _teamId ) continue;
        if( i == g_app->m_location->GetMonsterTeamId() ) continue;
        if( g_app->m_location->m_teams[i]->m_teamType == TeamTypeUnused ) continue;

        ReleaseSpirits(i);
        m_summonType[i] = 0;
        m_spirits[i].Empty();
        m_souls[i] = 0;
    }*/
}

bool Portal::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool Portal::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

bool Portal::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *norm )
{
    return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}