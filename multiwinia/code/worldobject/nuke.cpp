#include "lib/universal_include.h"

#include "lib/resource.h"
#include "lib/debug_utils.h"
#include "lib/debug_render.h"
#include "lib/math_utils.h"
#include "lib/shape.h"
#include "lib/bitmap.h"
#include "lib/filesys/binary_stream_readers.h"
#include "lib/math/random_number.h"
#include "lib/text_renderer.h"
#include "lib/language_table.h"
#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"
#include "unit.h"
#include "gametimer.h"
#include "multiwinia.h"
#include "obstruction_grid.h"

#include "worldobject/nuke.h"

Nuke::Nuke()
:   Entity(),
    m_totalDistance(0.0f),
    m_historyTimer(0.0f),
    m_numDeaths(0),
    m_casualtyMessageTimer(0.0f),
    m_exploded(false),
    m_launched(false),
    m_subTimer(2.5),
    m_armed(false),
    m_launchTimer(-1.0f),
    m_renderMarker(false)
{
    m_type = Entity::TypeNuke;
}

Nuke::~Nuke()
{
    m_history.EmptyAndDelete();
}

void Nuke::Begin()
{
    Entity::Begin();
    m_radius = 5.0f;

    m_subPos = m_pos;
    m_subPos.y = -30.0f;
}

bool Nuke::Advance( Unit *_unit )
{
    if( m_launched )
    {
        if( AdvanceLaunched() ) return true;
    }

    bool subFinished = AdvanceSub();
    if( m_launchTimer >= 0.0f )
    {
        m_launchTimer -= SERVER_ADVANCE_PERIOD;
        if( m_launchTimer <= 0.0f )
        {
            m_launchTimer = -1.0f;
            Launch();
        }
    }
    
    //if( m_launched && 
    //    !m_exploded )
    //{
    //    for( int i = 0; i < 4; ++i )
    //    {
    //        Vector3 vel( m_vel / 10.0f );
    //        vel.x += syncsfrand(10.0f);
    //        vel.y += syncsfrand(10.0f);
    //        vel.z += syncsfrand(10.0f);
    //        float size = 100.0f + syncsfrand(55.0f);
    //        g_app->m_particleSystem->CreateParticle(m_pos - m_vel * SERVER_ADVANCE_PERIOD, vel, Particle::TypeFire, size);
    //    }
    //}

    return subFinished;
}

bool Nuke::AdvanceLaunched()
{
    if( !m_exploded )
    {
        m_historyTimer -= SERVER_ADVANCE_PERIOD;
        if( m_historyTimer <= 0.0f )
        {
            m_history.PutDataAtStart( new Vector3(m_pos.x, m_pos.y, m_pos.z) );
            m_historyTimer = 0.5;
        }
        AdvanceToTarget();
    }
    else
    {
        m_casualtyMessageTimer -= SERVER_ADVANCE_PERIOD;
        if( m_casualtyMessageTimer <= 0.0f ) return true;
    }
    return false;
}

bool Nuke::AdvanceSub()
{
    //if( m_subPos.y < -35.0f ) return false;

    if( !m_exploded && m_subPos.y >= 30.0f && m_launchTimer < 0.0f && !m_launched )
    {
        m_subVel.Zero();
        m_subTimer -= SERVER_ADVANCE_PERIOD;
        if( m_subTimer <= 0.0 )
        {
            double baseTime = g_app->m_location->m_landscape.GetWorldSizeX() / 100.0;
            m_launchTimer = baseTime - (( m_subPos - m_targetPos ).Mag() / 100.0);
            m_launchTimer = max( 0.0, m_launchTimer );
            //return true;
        }
    }
    else
    {
        int direction = 0;
        if( m_subPos.y < 30.0f ) direction = 1;
        if( m_exploded ) direction = -1;
        Vector3 oldPos = m_subPos;
        m_subPos.y += 1.0f * direction;

        m_subVel = (m_subPos - oldPos) / SERVER_ADVANCE_PERIOD;
        if( m_subPos.y <= -35.0f ) return true;
    }
  
    return false;
}

bool Nuke::AdvanceToTarget()
{
    double remainingDistance = (m_targetPos - m_pos).Mag();
    double fractionDistance = 1 - remainingDistance / m_totalDistance;

    //fractionDistance = max(fractionDistance, 0.1 );

    int direction = 1;
    /*if( m_subPos.x < g_app->m_location->m_landscape.GetWorldSizeX() / 2.0f &&
        m_subPos.z < g_app->m_location->m_landscape.GetWorldSizeZ() / 2.0f)
    {
        direction = -1;
    }*/
    Vector3 subPos = m_subPos;
    double heightFactor = ( subPos - m_targetPos ).Mag() / 3.0;
    Vector3 front = (m_targetPos - m_pos).Normalise();  
    double fractionNorth = 5 * m_pos.y / ( heightFactor );
    fractionNorth = max( fractionNorth, 2.0 );
    
    Vector3 d = (m_targetPos - subPos).Normalise();
    subPos.y = 0.0f;
    Vector3 r = (d ^ g_upVector).Normalise();
    front.RotateAround( r * (M_PI / (direction * fractionNorth ) ));
    front.RotateAround( r * fractionDistance * (-direction * (M_PI/fractionNorth)));

   /* if( front.y < 0.0f && fractionDistance < 0.5f )
    {
        // we're pointing down when we shouldnt be, change direction
        front = (m_targetPos - m_pos).Normalise(); 
        front.RotateAround( r * (M_PI / (-1 * fractionNorth)));
        front.RotateAround( r * fractionDistance * ((M_PI/fractionNorth)));

    }
    */

    //front.RotateAroundZ( M_PI / (fractionNorth) );
    //front.RotateAroundZ( fractionDistance * (-1 * M_PI/fractionNorth) );

    m_vel = Vector3(front * (m_stats[StatSpeed]/2 + m_stats[StatSpeed]/2 * fractionDistance * fractionDistance));
       
    m_pos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
    double newDistance = ( m_targetPos - m_pos ).Mag();
    m_front = front;

    if( m_pos.y > 150.0 && !m_armed )
    {
        m_armed = true;
    }

    if( (m_pos - m_targetPos).Mag() < 5.0 * g_gameTimer.GetGameSpeed() )
    {
        // explode!
        Vector3 pos = m_pos;
        pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( m_targetPos.x, m_targetPos.z );

		if( g_app->m_multiwinia->m_coopMode && m_id.GetTeamId() == g_app->m_globalWorld->m_myTeamId )
        {
            // Wrong Game Achievement check
            for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
            {
                if( g_app->m_location->m_buildings.ValidIndex(i) )
                {
                    Building *b = g_app->m_location->m_buildings[i];
                    if( b->m_type == Building::TypeSpawnPoint )
                    {
                        if( ( m_targetPos - b->m_pos ).Mag() < 120.0f )
                        {
                            if( g_app->m_location->IsFriend( m_id.GetTeamId(), b->m_id.GetTeamId() ) )
                            {
                                g_app->GiveAchievement( App::DarwiniaAchievementWrongGame );
                                g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_wronggame") );
                                break;
                            }
                        }
                    }
                }
            }
        }

        m_numDeaths = g_app->m_location->Bang( pos, 50.0f, 100.0f );

        g_app->m_soundSystem->TriggerEntityEvent( this, "Explode" );
        m_exploded = true;
        m_casualtyMessageTimer = 5.0f;

        g_app->m_location->m_teams[m_id.GetTeamId()]->m_teamKills += m_numDeaths;
        return true;
    }
    
    return false;  
}
void Nuke::Launch()
{
    m_pos.y = m_subPos.y;
    //m_targetPos.y -= 20.0f;
    m_totalDistance = (m_targetPos - m_pos).Mag();

    m_launched = true;
}

void Nuke::Render( double _predictionTime )
{
    if( m_renderMarker )
    {
        RenderGroundMarker();
    }
    RenderSub( _predictionTime );
    if( m_exploded )
    {
        //RenderDeaths();
    }
    else
    {
        if( !m_launched ) return;

        Vector3 front = m_front;
        front.RotateAroundY( M_PI / 2.0f );
        Vector3 predictedPos = m_pos + m_vel * _predictionTime;;
        Vector3 entityUp = m_front;
        Vector3 entityRight (front ^ entityUp);

        Vector3 lengthVector = m_vel;
        lengthVector.SetLength( 10.0f );
        Vector3 fromPos = predictedPos;
        Vector3 toPos = predictedPos - lengthVector;

        Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
        Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
        float   camDistSqd      = camToMidPoint.MagSquared();
        Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();

        entityRight = rightAngle;

        float size = 10.0f;  
        size *= (1.0f + 0.03f * (( m_id.GetIndex() * m_id.GetUniqueId() ) % 10));
        entityRight *= size;
        entityUp *= size * 2.0f;

        glDepthMask     ( false );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glEnable        ( GL_TEXTURE_2D );
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "sprites/nuke.bmp" ) );

        RGBAColour colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
        if( m_id.GetTeamId() == g_app->m_location->GetMonsterTeamId() )
        {
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            colour.Set( 255, 255, 255, 0 );
        }
        glColor4ubv(colour.GetData());

        glBegin(GL_QUADS);
            glTexCoord2i(0, 1);     glVertex3dv( (predictedPos - entityRight + entityUp).GetData() );
            glTexCoord2i(1, 1);     glVertex3dv( (predictedPos + entityRight + entityUp).GetData() );
            glTexCoord2i(1, 0);     glVertex3dv( (predictedPos + entityRight).GetData() );
            glTexCoord2i(0, 0);     glVertex3dv( (predictedPos - entityRight).GetData() );
        glEnd();

        RenderHistory( _predictionTime );

        glShadeModel    ( GL_FLAT );
        glDisable       ( GL_TEXTURE_2D );
        glDepthMask     ( true );
    }
}

void Nuke::RenderHistory( double _predictionTime )
{
    if( m_history.Size() > 0 && m_id.GetTeamId() != 255 )
    {
        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );

        Vector3 predictedPos = m_pos + m_vel * SERVER_ADVANCE_PERIOD;
        Vector3 lastPos = predictedPos;

        RGBAColour colour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
        if( m_id.GetTeamId() == g_app->m_location->GetMonsterTeamId() )
        {
            glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
            colour.Set( 255, 255, 255, 0 );
        }

        for( int i = 0; i < m_history.Size(); ++i )
        {
            Vector3 historyPos, thisPos;
			thisPos = historyPos = *m_history[i];

            Vector3 diff = thisPos - lastPos;
            lastPos += diff * 0.1f;
            if( m_id.GetTeamId() != g_app->m_location->GetMonsterTeamId() )
            {
                colour.a = 255 - 255 * (float) i / (float) m_history.Size();
            }

            glColor4ubv( colour.GetData() );

            Vector3 lengthVector = (thisPos - lastPos).Normalise();
	        lengthVector.SetLength((thisPos - lastPos).Mag());
            Vector3 fromPos = lastPos;
            Vector3 toPos = thisPos;//lastPos - lengthVector;

            Vector3 midPoint        = fromPos + (toPos - fromPos)/2.0f;
            Vector3 camToMidPoint   = g_app->m_camera->GetPos() - midPoint;
            float   camDistSqd      = camToMidPoint.MagSquared();
            Vector3 rightAngle      = (camToMidPoint ^ ( midPoint - toPos )).Normalise();
            
            //rightAngle *= 0.8f;
            rightAngle.SetLength(5.0f);

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
                glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     

                glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
                glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     

                glTexCoord2i(0,0);      glVertex3dv( (fromPos - rightAngle).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (fromPos + rightAngle).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (toPos + rightAngle).GetData() );                
                glTexCoord2i(1,0);      glVertex3dv( (toPos - rightAngle).GetData() );                     
            glEnd();

            lastPos = historyPos;
        }
    }
}

void Nuke::RenderDeaths()
{
    float fontSize = 15.0f;

    float timeOnScreen = 5.0f - m_casualtyMessageTimer;

    char msg[256];
    sprintf( msg, "Darwinia Hit, %d Dead", m_numDeaths );
    float fractionShown = timeOnScreen/1.5f;
    if( fractionShown < 1.0f ) msg[ int(strlen(msg) * fractionShown) ] = '\x0';

    float alpha = 1.0f;
    if( timeOnScreen > 4.0f ) alpha = 5.0f - timeOnScreen;
    alpha = max(alpha, 0.0f);
    alpha *= 255;

    Vector3 pos = m_pos;
    pos.y += 200.0f;

    g_gameFont.SetRenderOutline(true);
    glColor4f(0.75f, 0.75f, 0.75f,0.0f);
    g_gameFont.DrawText3DCentre( pos, fontSize, msg );

    g_gameFont.SetRenderOutline(false);
    glColor4f(1.0f,1.0f,1.0f,1.0f);
    g_gameFont.DrawText3DCentre( pos, fontSize, msg );
}


void Nuke::RenderSub( double _predictionTime )
{
    if( m_subPos.y <= -35.0f ) return;

    glEnable        ( GL_TEXTURE_2D );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
 	glEnable		( GL_BLEND );
    glDepthMask     ( false );    
    glDisable       ( GL_CULL_FACE );

    float size = 100.0f;
    Vector3 pos = m_subPos + m_subVel * _predictionTime;      

    Vector3 up = g_app->m_camera->GetUp();
    Vector3 right = g_app->m_camera->GetRight();

    char filename[256];
    char shadow[256];

    sprintf( filename, "sprites/sub.bmp" );
    sprintf(shadow, "sprites/subshadow.bmp");

    float alpha = max(m_subPos.y,0.0) / 30.0f;

    glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
    glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(shadow));
    glColor4f   ( 1.0f, 1.0f, 1.0f, 0.0f );
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
    glEnd();

    RGBAColour col = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
    col.a = alpha * 255;
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ) );
    glColor4ubv( col.GetData() );  
    glBegin( GL_QUADS );
        glTexCoord2i( 0, 0 );       glVertex3dv( (pos - right * size - up * size).GetData() );
        glTexCoord2i( 0, 1 );       glVertex3dv( (pos - right * size + up * size).GetData() );
        glTexCoord2i( 1, 1 );       glVertex3dv( (pos + right * size + up * size).GetData() );
        glTexCoord2i( 1, 0 );       glVertex3dv( (pos + right * size - up * size).GetData() );    
    glEnd();
    
    glEnable        ( GL_CULL_FACE );
	glDisable       ( GL_BLEND );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}

void Nuke::RenderGroundMarker()
{
    glEnable        (GL_TEXTURE_2D);
	glEnable        (GL_BLEND);
    glDisable       (GL_CULL_FACE );
    glDepthMask     (false);
    glDisable       (GL_DEPTH_TEST);

    Vector3 pos = m_targetPos;
    pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );

    float scale = 40.0f + 25.0f * sin(GetHighResTime());
    /*float camDist = ( g_app->m_camera->GetPos() - pos ).Mag();
    scale *= sqrtf(camDist) / 40.0f;*/

    Vector3 front(1,0,0);
    Vector3 up = g_app->m_location->m_landscape.m_normalMap->GetValue( pos.x, pos.z );
    Vector3 rightAngle = (front ^ up).Normalise();

    pos -= 0.5f * rightAngle * scale;
    pos += 0.5f * front * scale;
    pos += up * 3.0f;

    RGBAColour colour(200,20,20,255);

    char filename[256];
    char shadow[256];

    sprintf( filename, "icons/nukesymbol.bmp" );
    sprintf(shadow, "shadow_%s", filename);

    if( !g_app->m_resource->DoesTextureExist( shadow ) )
    {
        BinaryReader *binReader = g_app->m_resource->GetBinaryReader( filename );
        BitmapRGBA bmp( binReader, "bmp" );
	    SAFE_DELETE(binReader);

        bmp.ApplyBlurFilter( 10.0f );
	    g_app->m_resource->AddBitmap(shadow, bmp);
    }

    glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture(shadow));
    glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR);
    glColor4f       (1.0f, 1.0f, 1.0f, 0.0f);

    glBegin( GL_QUADS );
        glTexCoord2f(0,1);      glVertex3dv( (pos).GetData() );
        glTexCoord2f(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
        glTexCoord2f(1,0);      glVertex3dv( (pos - front * scale + rightAngle * scale).GetData() );
        glTexCoord2f(0,0);      glVertex3dv( (pos - front * scale).GetData() );
    glEnd();

    glColor4ubv     (colour.GetData());
	glBindTexture   (GL_TEXTURE_2D, g_app->m_resource->GetTexture( filename ));
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE);							

    glBegin( GL_QUADS );
        glTexCoord2i(0,1);      glVertex3dv( (pos).GetData() );
        glTexCoord2i(1,1);      glVertex3dv( (pos + rightAngle * scale).GetData() );
        glTexCoord2i(1,0);      glVertex3dv( (pos - front * scale + rightAngle * scale).GetData() );
        glTexCoord2i(0,0);      glVertex3dv( (pos - front * scale).GetData() );
    glEnd();

    glDepthMask     (true);
    glEnable        (GL_DEPTH_TEST );    
    glDisable       (GL_BLEND);
	glBlendFunc     (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);							
	glDisable       (GL_TEXTURE_2D);
    glEnable        (GL_CULL_FACE);
}


void Nuke::ListSoundEvents( LList<char *> *_list )
{
    Entity::ListSoundEvents( _list );

    _list->PutData( "Explode" );
}
