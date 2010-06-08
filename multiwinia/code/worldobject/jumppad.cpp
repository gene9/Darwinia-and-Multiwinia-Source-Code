#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/debug_render.h"

#include "lib/math/random_number.h"

#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"

#include "app.h"
#include "globals.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "main.h"
#include "entity_grid.h"
#include "location_editor.h"

#include "sound/soundsystem.h"

#include "worldobject/jumppad.h"
#include "worldobject/darwinian.h"

#define JUMPPAD_RADIUS 40.0


JumpPad::JumpPad()
:   Building(),
    m_force(0.0),
    m_angle(0.0),
    m_launchTimer(0.0)
{
    m_type = Building::TypeJumpPad;
}

void JumpPad::Initialise( Building *_template )
{
    Building::Initialise( _template );
    JumpPad *pad = (JumpPad *)_template;
    m_force = pad->m_force;
    m_angle = pad->m_angle;
}

bool JumpPad::Advance()
{   
    m_launchTimer -= SERVER_ADVANCE_PERIOD;
    if( m_launchTimer <= 0.0 )
    {
        m_launchTimer = 0.3;

        Vector3 targetDirection = m_front;
        Vector3 rotation = m_front ^ g_upVector;
        targetDirection.RotateAround( rotation * m_angle );
        targetDirection.SetLength( m_force );

        int numFound;
        g_app->m_location->m_entityGrid->GetNeighbours( s_neighbours, m_pos.x, m_pos.z, JUMPPAD_RADIUS, &numFound );

        for( int i = 0; i < numFound; ++i )
        {
            Darwinian *d = (Darwinian *)g_app->m_location->GetEntitySafe( s_neighbours[i], Entity::TypeDarwinian );
            if( d && d->m_onGround && d->m_state != Darwinian::StateInsideArmour )
            {
                d->m_vel = targetDirection;
                d->m_onGround = false;
                d->m_state = Darwinian::StateIdle;
                d->m_wayPoint = g_zeroVector;
                d->m_ordersSet = false;
            }
        }
    }
    return false;
}

void JumpPad::Render( double _predictionTime )
{
}

void JumpPad::RenderAlphas( double _predictionTime )
{   
    float c = 255.0f * (0.3f + abs(sinf(g_gameTime) * 0.7f));
    RGBAColour col(c, c, c, 1.0f);
    if( g_app->m_editing )
    {
#ifdef LOCATION_EDITOR
        Building::RenderAlphas( _predictionTime );
        RenderSphere( m_pos, 30.0 );
        if( g_app->m_editing &&
            g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            RenderSphere( m_pos, JUMPPAD_RADIUS );
        }

        RenderArrow( m_pos, m_pos + m_front * 50.0, 5.0 );

        if( g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            Vector3 targetDirection = m_front;
            Vector3 rotation = m_front ^ g_upVector;
            targetDirection.RotateAround( rotation * m_angle );
            targetDirection.SetLength( m_force );

            Darwinian darwinian;
            darwinian.m_pos = m_pos;
            darwinian.m_vel = targetDirection;
            darwinian.m_onGround = false;
        
            glColor4f( 1.0, 1.0, 1.0, 1.0 );
            glLineWidth( 2.0 );
            glBegin( GL_LINES );
            while( true )
            {
                glVertex3dv( darwinian.m_pos.GetData() );
                darwinian.AdvanceInAir(NULL);
                glVertex3dv( darwinian.m_pos.GetData() );
            
                if( darwinian.m_onGround ) break;
            }
            glEnd();
        }
#endif
    }
    else
    {
       /* float angle = g_gameTime;//
        int steps = 30;
        double size = 60.0;
        double degreesPerStep = (2 * M_PI) / steps;
        double area = JUMPPAD_RADIUS;

        for( int i = 0; i < steps; ++i )
        {
            Vector3 pos = m_pos;
            pos.x += iv_sin( i * degreesPerStep + angle ) * area;
            pos.z += iv_cos( i * degreesPerStep + angle ) * area;
            pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z ) + 5.0;
            g_app->m_particleSystem->CreateParticle( pos, g_zeroVector, Particle::TypeSelectionMarker, size, col );
        }*/
    }

    /*Vector3 rightAngle = (m_front ^ g_upVector).Normalise();
    
	glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE );
    glDepthMask     (false);
    
    double scale = JUMPPAD_RADIUS;

    Vector3 pos = m_pos;
    pos -= rightAngle * scale / 2.0;
    pos += m_front * scale / 2.0;

    glColor4f( 1.0, 1.0, 1.0, 1.0 );
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture("icons/arrow.bmp"));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);		

    Vector3 p1, p2, p3, p4;
    p1 = pos;
    p2 = pos + rightAngle * scale;
    p3 = pos - m_front * scale + rightAngle * scale;
    p4 = pos - m_front * scale;

    p1.y = g_app->m_location->m_landscape.m_heightMap->GetValue( p1.x, p1.z ) + 10.0;
    p2.y = g_app->m_location->m_landscape.m_heightMap->GetValue( p2.x, p2.z ) + 10.0;
    p3.y = g_app->m_location->m_landscape.m_heightMap->GetValue( p3.x, p3.z ) + 10.0;
    p4.y = g_app->m_location->m_landscape.m_heightMap->GetValue( p4.x, p4.z ) + 10.0;

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( (p1).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (p2).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (p3).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (p4).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );    
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);*/

    glEnable    ( GL_BLEND );
//    glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
    glDisable   ( GL_DEPTH_TEST );

    //double stepSize = g_app->m_camera->GetPos().y / 10.0;
    double stepSize = 5.0;

    Vector3 from = m_pos - m_front * JUMPPAD_RADIUS;
    Vector3 to = m_pos + m_front * JUMPPAD_RADIUS;
    
    Vector3 rightAngle = ( from - to ) ^ g_upVector;
    rightAngle.y = 0;
    rightAngle.SetLength(20.0);

    //return;

    double distance = ( from - to ).Mag();
    int numSteps = 2;//distance / stepSize;

    double timeNow = GetHighResTime() * 5.0;

    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );    
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "icons/thickarrow.bmp" ) );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glShadeModel    ( GL_SMOOTH );
    glBegin         ( GL_QUAD_STRIP );

    double halfNumSteps = numSteps/2.0;

    for( int i = 0; i <= numSteps; ++i )
    {
        double alpha = i/halfNumSteps;
        if( i >= halfNumSteps ) alpha = 1.0 - (i-halfNumSteps) / halfNumSteps;
        //alpha *= 0.5;
        alpha *= 255;
        
        //alpha = 0.5;
        //if( alpha < 0 ) alpha = 0;
        //if( alpha > 1 ) alpha = 1;

        col.a = alpha;
        glColor4ubv( col.GetData() );

        Vector3 thisPos = from + ( to - from ) * ( i / (double)numSteps);
        
        Vector3 right = thisPos + rightAngle;
        Vector3 left = thisPos - rightAngle;

        right.y = g_app->m_location->m_landscape.m_heightMap->GetValue( right.x, right.z );
        left.y = g_app->m_location->m_landscape.m_heightMap->GetValue( left.x, left.z );

        glTexCoord2f(0,i-timeNow/4);      glVertex3dv( (left).GetData() );
        glTexCoord2f(1,i-timeNow/4);      glVertex3dv( (right).GetData() );
    }

    glEnd();
    glShadeModel    ( GL_FLAT );
    glDisable       ( GL_TEXTURE_2D );

    glLineWidth( 1.0 );
    glDisable( GL_POLYGON_OFFSET_FILL );
    glEnable( GL_DEPTH_TEST );
}

void JumpPad::Write( TextWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-2.2f ", m_force );
    _out->printf( "%-2.2f ", m_angle );
}

void JumpPad::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_force = iv_atof( _in->GetNextToken() );
    m_angle = iv_atof( _in->GetNextToken() );
}


bool JumpPad::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool JumpPad::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}

bool JumpPad::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *norm )
{
    return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}