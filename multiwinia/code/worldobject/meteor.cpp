#include "lib/universal_include.h"

#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/shape.h"
#include "lib/math/random_number.h"
#include "lib/text_renderer.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "main.h"

#include "worldobject/meteor.h"

static int s_meteorCount = 0;

Meteor::Meteor()
:   Entity(),
    m_life(5.0f),
    m_timeSlowTriggered(false)
{
    m_type = Entity::TypeMeteor;

    m_shape = g_app->m_resource->GetShape( "meteor.shp" );

    ++s_meteorCount;
}

Meteor::~Meteor()
{
    --s_meteorCount;
}

void Meteor::Begin()
{
    Entity::Begin();
    m_radius = 20.0f;
}

bool Meteor::Advance( Unit *_unit )
{
    if( m_dead )
    {
        return true;
        m_life -= SERVER_ADVANCE_PERIOD;
    }
    else 
    {
        for( int i = 0; i < 5; ++i )
        {
            Vector3 pos = m_pos;
            pos.x += sfrand( m_radius );
            pos.y += sfrand( m_radius );
            pos.z += sfrand( m_radius );

            Vector3 vel( m_vel / 10.0f );
            vel.x += sfrand(10.0f);
            vel.y += sfrand(10.0f);
            vel.z += sfrand(10.0f);
            float size = 300.0f + syncsfrand(60.0f);
            g_app->m_particleSystem->CreateParticle(pos, vel, Particle::TypeFire, size);
        }

        if( !m_timeSlowTriggered )
        {
            double distToTarget = ( m_pos - m_targetPos ).Mag();
            if( distToTarget < 200 )
            {                
                g_gameTimer.RequestSpeedChange( 0.4, 0.1, 4.0 );                
                m_timeSlowTriggered = true;
            }
        }

        if( AdvanceToTarget())
        {
            g_app->m_soundSystem->TriggerEntityEvent( this, "Land" );
            g_app->m_location->Bang( m_pos, 50.0, 100.0, m_id.GetTeamId() );
            m_dead = true;
            m_vel.Zero();
            return true;
        }
    }
    return (m_life <= 0.0f);
}

bool Meteor::AdvanceToTarget()
{
    Vector3 oldPos = m_pos;
    Vector3 targetDir = (m_targetPos - m_pos).Normalise();

    double speed = EntityBlueprint::GetStat( Entity::TypeMeteor, StatSpeed );

    Vector3 newPos = m_pos + targetDir * speed;
    Vector3 targetVel = (newPos - m_pos).SetLength( speed );

    Vector3 moved = newPos - oldPos;
    if( moved.Mag() > speed * SERVER_ADVANCE_PERIOD ) moved.SetLength( speed * SERVER_ADVANCE_PERIOD );
    newPos = m_pos + moved;

    m_pos = newPos;
    m_vel = (m_pos - oldPos) / SERVER_ADVANCE_PERIOD;
    
    return ( ( m_pos - m_targetPos ).Mag() < 10.0f ||
              m_pos.y < g_app->m_location->m_landscape.m_heightMap->GetValue( m_pos.x, m_pos.z ) );
}

void Meteor::Render( double _predictionTime )
{
    Vector3 predictedPos = m_pos + m_vel * _predictionTime;

    if( !m_dead )
    {
        //RenderSphere( predictedPos, m_radius );

        //
        // Render our shape

        Vector3 front = m_front;
        Vector3 up = g_upVector;
        Vector3 right = front ^ up;

        front.RotateAround( right * g_gameTimer.GetAccurateTime() * 5.0f );
        up.RotateAround( right * g_gameTimer.GetAccurateTime() * 5.0f );

        Matrix34 mat( front, up, predictedPos );

        g_app->m_renderer->SetObjectLighting();
        glDisable       (GL_TEXTURE_2D);
        glShadeModel    (GL_SMOOTH);
        glEnable        (GL_NORMALIZE);

        m_shape->Render( _predictionTime, mat );

        glDisable       (GL_NORMALIZE);
        glShadeModel    (GL_FLAT);
        glEnable        (GL_TEXTURE_2D);
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        g_app->m_renderer->UnsetObjectLighting();

        g_app->m_renderer->MarkUsedCells(m_shape, mat);
    }

    glEnable        ( GL_TEXTURE_2D );   
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );    
    glDisable       ( GL_CULL_FACE );
    glDepthMask     ( false );

    glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDisable( GL_DEPTH_TEST );

    float alpha = 0.35f;
    if( m_dead )
    {
        alpha *= ( m_life / 5.0f );
    }
    
    glColor4f( 0.8f, 0.2f, 0.1f, alpha);        

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
    double timeIndex = g_gameTime * 2;

    for( int i = 0; i < 10; ++i )
    {
        Vector3 pos = m_pos + m_vel * _predictionTime;
        pos.x += sinf(timeIndex+i) * i * 1.7f;
        pos.y += cosf(timeIndex+i);
        pos.z += cosf(timeIndex+i) * i * 1.7f;

        float size = m_radius * 4.0f;
        size = max( size, 5.0f );
        if( m_dead )
        {
            size *= 2.0f;
        }

        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3dv( (pos - camUp * size - camRight * size).GetData() );
            glTexCoord2i(1,0);      glVertex3dv( (pos + camUp * size - camRight * size).GetData() );
            glTexCoord2i(1,1);      glVertex3dv( (pos + camUp * size + camRight * size).GetData() );
            glTexCoord2i(0,1);      glVertex3dv( (pos - camUp * size + camRight * size).GetData() );
        glEnd();        
    }

    glEnable( GL_DEPTH_TEST );
    glDepthMask     ( true );
    glEnable        ( GL_CULL_FACE );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );    
    glDisable       ( GL_TEXTURE_2D );
    glDisable       ( GL_BLEND );
}


void Meteor::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Land" );
}


