#include "lib/universal_include.h"
#include "lib/math/random_number.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"
#include "lib/math_utils.h"
#include "lib/preferences.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"

#include "worldobject/pulsebomb.h"
#include "worldobject/controlstation.h"

#include "app.h"
#include "camera.h"
#include "explosion.h"
#include "level_file.h"
#include "location.h"
#include "main.h"
#include "multiwinia.h"
#include "particle_system.h"
#include "renderer.h"
#include "team.h"

int PulseBomb::s_pulseBombs[NUM_TEAMS];
double PulseBomb::s_defaultTime = 0.0;

PulseBomb::PulseBomb()
:   Building(),
    m_startTime(0.0),
    m_active(true),
    m_scale(0.75),
    m_detonateTime(0.0),
    m_oneMinuteWarning(false)
{
    m_type = TypePulseBomb;
    m_shape =  g_app->m_resource->GetShapeCopy( "pulsebomb.shp", false );
    m_shape->ScaleRadius( m_scale );
}

PulseBomb::~PulseBomb()
{
    if( m_shape )
    {
        delete m_shape;
        m_shape = NULL;
    }
}

void PulseBomb::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_startTime = ((PulseBomb *)_template)->m_startTime;
    s_pulseBombs[m_id.GetTeamId()] = m_id.GetUniqueId();
    for( int i = 0; i < ((PulseBomb *)_template)->m_links.Size(); ++i )
    {
        SetBuildingLink( ((PulseBomb *)_template)->m_links[i] );
    }

    if( g_app->m_multiwinia->m_pulseTimer > 0.0 )
    {
        m_startTime = g_app->m_multiwinia->m_pulseTimer;
    }

    if( s_defaultTime == 0.0 ) s_defaultTime = m_startTime;

    g_app->m_multiwinia->m_timeRemaining = (int) m_startTime;
}

void PulseBomb::SetDetail( int _detail )
{
    //Building::SetDetail( _detail );

    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    if( m_shape )
    {
        Matrix34 mat( m_front, m_up, m_pos );
        mat.u *= m_scale;
        mat.r *= m_scale;
        mat.f *= m_scale;

        m_centrePos = m_shape->CalculateCentre( mat );        
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );
    }
}

bool PulseBomb::Advance()
{
    if( m_destroyed ) return false;

    for( int i = 0; i < m_links.Size(); ++i )
    {
        Building *b = g_app->m_location->GetBuilding( m_links[i] );
        if( b && b->m_type == Building::TypeControlStation )
        {
            ((ControlStation *)b)->LinkToPulseBomb(m_id.GetUniqueId());
        }
    }

    bool allStationsCaptured = true;
    for( int i = 0; i < m_links.Size(); ++i )
    {
        Building *b = g_app->m_location->GetBuilding( m_links[i] );
        if( b && b->m_type == Building::TypeControlStation )
        {
            if( g_app->m_location->IsFriend(b->m_id.GetTeamId(), m_id.GetTeamId() ) ||
                b->m_id.GetTeamId() == 255 )
            {
                allStationsCaptured = false;
                break;
            }
        }
    }

    if( allStationsCaptured )
    {
        Destroy();
        g_app->m_multiwinia->RecordDefenseTime( m_startTime - g_app->m_multiwinia->GetPreciseTimeRemaining(), m_id.GetTeamId() );
        return false;
    }

    if( g_app->m_multiwinia->m_timeRemaining <= 60 && !m_oneMinuteWarning )
    {
        m_oneMinuteWarning = true;
        g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_oneminutewarning") );
    }



    if( g_app->m_multiwinia->m_timeRemaining <= 0 && m_detonateTime == 0.0 )
    {
        g_app->m_multiwinia->RecordDefenseTime( m_startTime, m_id.GetTeamId() );
        m_detonateTime = GetNetworkTime();
        // fully counted down, explode and wipe out the target team
        PulseWave *pulse = new PulseWave();
        pulse->m_radius = 200.0;
        pulse->m_pos = m_pos;        
        int index = g_app->m_location->m_effects.PutData( pulse );
        pulse->m_id.Set( m_id.GetTeamId(), UNIT_EFFECTS, index, -1 );
        pulse->m_id.GenerateUniqueId();

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *b = g_app->m_location->m_buildings[i];
                if( b->m_type == Building::TypeTrunkPort &&
                    b->m_id.GetTeamId() != m_id.GetTeamId() )
                {
                    b->m_id.SetTeamId(255);
                }
            }
        }
    }

    return false;
}

void PulseBomb::Render( double _predictionTime )
{
    if( m_destroyed ) return;
#ifdef DEBUG_RENDER_ENABLED
	if (g_app->m_editing)
	{
		Vector3 pos(m_pos);
		pos.y += 5.0;
		RenderArrow(pos, pos + m_front * 20.0, 4.0);
	}
#endif

    Vector3 predictedPos = m_pos;
	Matrix34 mat(m_front, g_upVector, predictedPos);
    mat.f *= m_scale;
    mat.u *= m_scale;
    mat.r *= m_scale;

	m_shape->Render(_predictionTime, mat);
}

void PulseBomb::RenderAlphas( double _predictionTime )
{
    if( m_destroyed ) return;

    Building::RenderAlphas( _predictionTime );

    if( g_app->m_editing )
    {
        Vector3 pos = m_pos;
        pos.y += 50.0;
        for( int i = 0; i < m_links.Size(); ++i )
        {
            Building *b = g_app->m_location->GetBuilding( m_links[i] );
            if( b )
            {
                RenderArrow( pos, b->m_pos, 5.0 );
            }
        }
    }

    if( m_detonateTime > 0.0f ) return;

    Vector3 thisPos = m_pos;
    thisPos.y += m_radius * 1.3;
    thisPos.y += 200.0 * ( 1.0 - ( g_app->m_multiwinia->GetPreciseTimeRemaining() / m_startTime ));

    Vector3 camUp = g_app->m_camera->GetUp();
    Vector3 camRight = g_app->m_camera->GetRight();
        
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/glow.bmp" ) );
    
    double timeIndex = g_gameTime + m_id.GetUniqueId() * 10.0;

    int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
    int maxBlobs = 20;
    if( buildingDetail == 2 ) maxBlobs = 10;
    if( buildingDetail == 3 ) maxBlobs = 2;

    double alpha = 1.0;

    for( int i = 0; i < maxBlobs; ++i )
    {        
        Vector3 pos = thisPos;
        pos += Vector3(0,25,0);
        pos.x += iv_sin(timeIndex+i) * i * 0.7;
        pos.y += iv_cos(timeIndex+i) * iv_sin(i*10) * 12;
        pos.z += iv_cos(timeIndex+i) * i * 0.7;

        double size = 2.0;
        double factor = 200.0;
        if( g_app->m_multiwinia->GetPreciseTimeRemaining() >= 0.0 ) size += iv_sin(timeIndex+i*10) * factor * ( 1.0 - ( g_app->m_multiwinia->GetPreciseTimeRemaining() / m_startTime ) );
        size = max( size, 2.0 );
        
        if( m_id.GetTeamId() != 255 )
        {            
            
            Team *team = g_app->m_location->m_teams[ m_id.GetTeamId() ];
            RGBAColour colour = team->m_colour;
            colour.a *= alpha;
            glColor4ubv( colour.GetData() );

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3dv( (pos - camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,0);      glVertex3dv( (pos + camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,1);      glVertex3dv( (pos + camRight * size - camUp * size).GetData() );
                glTexCoord2i(0,1);      glVertex3dv( (pos - camRight * size - camUp * size).GetData() );
            glEnd();
        }
    }
           
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

    RenderCountdown( _predictionTime );
}

void PulseBomb::RenderCountdown( double _predictionTime )
{
    if( g_app->m_editing ) return;

    if( g_app->m_multiwinia->m_timeRemaining <= 10 && !m_destroyed )
    {
        double screenSize = 60.0;
        Vector3 screenFront = m_front;
        //screenFront.RotateAroundY( 0.33 * M_PI );
        Vector3 screenRight = screenFront ^ g_upVector;
        Vector3 screenPos = m_pos + Vector3(0,50,0);
        screenPos -= screenRight * screenSize * 0.5;
        screenPos += screenFront * 60;
        Vector3 screenUp = g_upVector;

        //
        // Render lines for over effect

        glBindTexture( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/interface_grey.bmp" ) );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
        glEnable( GL_TEXTURE_2D );
        glEnable( GL_BLEND );
        glDepthMask( false );
        glShadeModel( GL_SMOOTH );
        glDisable( GL_CULL_FACE );

        double texX = 0.0;
        double texW = 3.0;
        double texY = g_gameTime*0.01;
        double texH = 0.3;

        glColor4f( 0.0, 0.0, 0.0, 0.5 );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        
        glBegin( GL_QUADS );
            glVertex3dv( screenPos.GetData() );
            glVertex3dv( (screenPos + screenRight * screenSize).GetData() );
            glVertex3dv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );
            glVertex3dv( (screenPos + screenUp * screenSize).GetData() );
        glEnd();

        glColor4ubv( g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour.GetData() );

        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        
        for( int i = 0; i < 2; ++i )
        {
            glBegin( GL_QUADS );
                glTexCoord2f(texX,texY);            
                glVertex3dv( screenPos.GetData() );

                glTexCoord2f(texX+texW,texY);       
                glVertex3dv( (screenPos + screenRight * screenSize).GetData() );
               
                glTexCoord2f(texX+texW,texY+texH);  
                glVertex3dv( (screenPos + screenRight * screenSize + screenUp * screenSize).GetData() );

                glTexCoord2f(texX,texY+texH);       
                glVertex3dv( (screenPos + screenUp * screenSize).GetData() );
            glEnd();

            texY *= 1.5;
            texH = 0.1;
        }


        glDepthMask( false );
        
        //
        // Render countdown

        glColor4f( 1.0, 1.0, 1.0, 1.0 );
        Vector3 textPos = screenPos 
                            + screenRight * screenSize * 0.5
                            + screenUp * screenSize * 0.5;

        if( g_app->m_multiwinia->m_timeRemaining > 0.0 )
        {
            g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "%d", g_app->m_multiwinia->m_timeRemaining );
        }
        else
        {
            if( fmod( g_gameTime, 2 ) < 1 )
            {
                g_gameFont.DrawText3D( textPos, screenFront, g_upVector, 50, "0" );
            }
        }
    
    
        glDepthMask     ( true );
        glEnable        ( GL_DEPTH_TEST );
        glDisable       ( GL_TEXTURE_2D );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );        
        glShadeModel    ( GL_FLAT );
    }
}

void PulseBomb::Destroy()
{
    ExplodeShape( 1.0 );
    m_destroyed = true;
    int defenderTeamId = -1;
    for( int i = 0; i < g_app->m_location->m_levelFile->m_numPlayers; ++i )
    {
        if( s_pulseBombs[i] != -1 ) defenderTeamId = i;
    }

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) ) 
        {
            Building *b = g_app->m_location->m_buildings[i];
            if( b->m_type == Building::TypeTrunkPort &&
                b->m_id.GetTeamId() != defenderTeamId )
            {
                b->m_id.SetTeamId( 255 );
            }
        }
    }

    g_app->m_location->Bang( m_pos, 75.0, 5.0);
    g_app->m_location->SetCurrentMessage( LANGUAGEPHRASE("dialog_pulsebombdestroyed") );
}


void PulseBomb::ExplodeShape( double _fraction )
{
    Matrix34 transform( m_front, m_up, m_pos );
    g_explosionManager.AddExplosion( m_shape, transform, _fraction );
}

void PulseBomb::Read(TextReader *_in, bool _dynamic)
{
    Building::Read( _in, _dynamic );
    m_startTime = iv_atof( _in->GetNextToken() );
    while( _in->TokenAvailable() )
    {
        SetBuildingLink( atoi(_in->GetNextToken()) );
    }
}

void PulseBomb::Write(TextWriter *_out)
{
    Building::Write( _out );
    _out->printf( "%-6.2f  ", m_startTime );
    for( int i = 0; i < m_links.Size(); ++i )
    {
        _out->printf( "%-4d", m_links[i] );
    }
}


bool PulseBomb::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                          double _rayLen, Vector3 *_pos, Vector3 *norm )
{
	if (m_shape)
	{
		RayPackage ray(_rayStart, _rayDir, _rayLen);
		Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
		return m_shape->RayHit(&ray, transform, true);
	}
	else
	{
		return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
	}	
}

bool PulseBomb::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    if(m_shape)
    {
        SpherePackage sphere(_pos, _radius);
        Matrix34 transform(m_front, m_up, m_pos);
        transform.u *= m_scale;
        transform.r *= m_scale;
        transform.f *= m_scale;
        return m_shape->SphereHit(&sphere, transform);
    }
    else
    {
        double distance = (_pos - m_pos).Mag();
        return( distance <= _radius + m_radius );
    }
}

bool PulseBomb::DoesShapeHit(Shape *_shape, Matrix34 _theTransform)
{
    if( m_shape )
    {
        Matrix34 ourTransform(m_front, m_up, m_pos);
        ourTransform.u *= m_scale;
        ourTransform.r *= m_scale;
        ourTransform.f *= m_scale;

        //return m_shape->ShapeHit( _shape, _theTransform, ourTransform, true );
        return _shape->ShapeHit( m_shape, ourTransform, _theTransform, true );
    }
    else
    {
        SpherePackage package( m_pos, m_radius );
        return _shape->SphereHit( &package, _theTransform, true );
    }
}

void PulseBomb::SetBuildingLink(int _buildingId)
{
    bool linkFound = false;
    for( int i = 0; i < m_links.Size(); ++i )
    {
        if( m_links[i] == _buildingId ) 
        {
            linkFound = true;
            break;
        }
    }

    if( !linkFound ) 
    {
        m_links.PutData( _buildingId );
    }
}