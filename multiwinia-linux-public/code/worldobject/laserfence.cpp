#include "lib/universal_include.h"

#include <math.h>

#include "lib/math_utils.h"
#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/matrix33.h"
#include "lib/profiler.h"
#include "lib/shape.h"
#include "lib/resource.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/ogl_extensions.h"
#include "lib/debug_render.h"
#include "lib/preferences.h"
#include "lib/math/random_number.h"

#include "main.h"
#include "app.h"
#include "location.h"
#include "camera.h"
#include "renderer.h"
#include "level_file.h"
#include "obstruction_grid.h"
#include "team.h"
#include "particle_system.h"
#include "multiwinia.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "worldobject/building.h"
#include "worldobject/laserfence.h"

LaserFence::LaserFence()
:   Building(),
    m_nextLaserFenceId(-1),
    m_status(0.0),
    m_mode(ModeDisabled),
    m_scale(1.0),
    m_marker1(NULL),
    m_marker2(NULL),
    m_sparkTimer(0.0),
    m_radiusSet(false),
	m_nextToggled(false),
    m_solarMode(false),
    m_surgeReceived(0.0),
	m_solarLinked(false)
{
    m_type = Building::TypeLaserFence;
    
	SetShape( g_app->m_resource->GetShape("laserfence.shp") );

    const char marker1Name[] = "MarkerFence01";
    m_marker1 = m_shape->m_rootFragment->LookupMarker( marker1Name );
    AppReleaseAssert( m_marker1, "LaserFence: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", marker1Name, m_shape->m_name );

    const char marker2Name[] = "MarkerFence02";
    m_marker2 = m_shape->m_rootFragment->LookupMarker( marker2Name );
    AppReleaseAssert( m_marker2, "LaserFence: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", marker2Name, m_shape->m_name );
}


void LaserFence::Initialise( Building *_template )
{
    Building::Initialise( _template );
	AppDebugAssert(_template->m_type == Building::TypeLaserFence);

    m_nextLaserFenceId = ((LaserFence *) _template)->m_nextLaserFenceId;
    m_scale            = ((LaserFence *) _template)->m_scale;
    m_mode             = ((LaserFence *) _template)->m_mode;

    if( m_mode == ModeEnabled ||
        m_mode == ModeAlwaysOn ) m_status = 1.0;

    m_sparkTimer = syncfrand(10.0);
}

void LaserFence::SetDetail( int _detail )
{
    m_radiusSet = false;
    Building::SetDetail( _detail );
}

void LaserFence::Spark()
{
    Vector3 sparkPos = m_pos;
    sparkPos.y += frand( m_scale*50.0 );

    LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( m_nextLaserFenceId );

    int numSparks = 5.0 + frand(5.0);
    for( int i = 0; i < numSparks; ++i )
    {
        Vector3 particleVel;
        if( nextFence ) particleVel = ( m_pos - nextFence->m_pos ) ^ g_upVector;
        else            particleVel = Vector3( sfrand(10.0), sfrand(5.0), sfrand(10.0) );
        
        particleVel.SetLength( 40.0+frand(20.0) );
        particleVel += Vector3( frand() * 20.0, sfrand() * 20.0, sfrand() * 20.0 );
        double size = 25.0 + frand(25.0);
        g_app->m_particleSystem->CreateParticle( sparkPos, particleVel, Particle::TypeSpark, size );        
    }

    g_app->m_soundSystem->TriggerBuildingEvent( this, "Spark" );
}


bool LaserFence::Advance ()
{   
    if( !m_radiusSet )
    {
        Building *building = g_app->m_location->GetBuilding( m_nextLaserFenceId );
        if( building )
        {
            m_centrePos = ( building->m_pos + m_pos ) / 2.0;
            m_radius = ( building->m_pos - m_pos ).Mag() / 2.0 + m_radius;
        }
        m_radiusSet = true;
    }

    if( m_mode != ModeDisabled )
    {
        m_sparkTimer -= SERVER_ADVANCE_PERIOD;
        if( m_sparkTimer < 0.0 )
        {
            m_sparkTimer = 8.0 + syncfrand(4.0);
            Spark();           
        }
    }

    if( m_solarMode )
    {
        if( GetNetworkTime() - m_surgeReceived > 5.0 )
        {
            if( IsEnabled() || m_mode == ModeNeverOn )
            {
                Disable();
            }
        }
        else if( !IsEnabled() )
        {
            Enable();
        }
    }

    switch( m_mode )
    {
        case ModeEnabling:
            m_status += LASERFENCE_RAISESPEED * SERVER_ADVANCE_PERIOD;
            if( m_status >= 0.5 && m_nextLaserFenceId != -1 && !m_nextToggled )
            {
                LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( m_nextLaserFenceId );
                if( nextFence ) 
				{
					nextFence->Enable();
					m_nextToggled = true;
				}
            }
            if( m_status >= 1.0 )
            {
                m_status = 1.0;
                m_mode = ModeEnabled;
                if( m_nextLaserFenceId == -1)
                {
                    //g_app->m_location->m_obstructionGrid->CalculateAll();
                }
            }
            break;

        case ModeDisabling:
            if( m_status <= 0.5 && m_nextLaserFenceId != -1 && !m_nextToggled)
            {
                LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( m_nextLaserFenceId );
                if( nextFence ) 
				{
					nextFence->Disable();
					m_nextToggled = true;
				}
            }
            m_status -= LASERFENCE_RAISESPEED * SERVER_ADVANCE_PERIOD;
            if( m_status <= 0.0 )
            {
                m_status = 0.0;
                m_mode = ModeDisabled;
                if( m_nextLaserFenceId == -1 && m_mode != ModeAlwaysOn )
                {
                    //g_app->m_location->m_obstructionGrid->CalculateAll();
                }
            }
            break;

        case ModeEnabled:
        case ModeAlwaysOn:
        {
            if( m_status < 1.0 )
            {
                m_status = 1.0;
                if( m_nextLaserFenceId == -1)
                {
                    //g_app->m_location->m_obstructionGrid->CalculateAll();
                }
            }
            break;
        }
        case ModeDisabled:
            m_status = 0.0;
            break;
    }

    return Building::Advance();
}


void LaserFence::Render ( double predictionTime )
{
	Matrix34 mat(m_front, g_upVector, m_pos);
    mat.f *= m_scale;
    mat.u *= m_scale;
    mat.r *= m_scale;

    glEnable( GL_NORMALIZE );
	m_shape->Render(predictionTime, mat);
    glDisable( GL_NORMALIZE );
}


double LaserFence::GetFenceFullHeight()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;
    Vector3 marker1 = m_marker1->GetWorldMatrix( mat ).pos;
    Vector3 marker2 = m_marker2->GetWorldMatrix( mat ).pos;

    return ( marker2.y - marker1.y );
}


bool LaserFence::IsInView()
{
    if( g_app->m_multiwinia->GameInGracePeriod() ) return true;

    return Building::IsInView();

    // No need to do the usual midpoint / radius calculation here
    // as it has already been done in LaserFence::Advance.
    // Our midpoint and radius values are frigged in order to 
    // correctly fill the obstruction grid.
}


bool LaserFence::PerformDepthSort( Vector3 &_centrePos )
{
    if( m_mode == ModeDisabled ) return false;


    if( m_nextLaserFenceId != -1 )
    {
        _centrePos = m_centrePos;
        return true;
    }
    
    
    return false;;
}

void LaserFence::RenderAlphas( double predictionTime )
{
    Building::RenderAlphas( predictionTime );
    
    if( m_mode != ModeDisabled &&
		m_mode != ModeNeverOn )
    {
        //
        // Draw the laser fence connecting to the next laser fence
    
        if( m_nextLaserFenceId != -1 )
        {
            Building *nextFence = g_app->m_location->GetBuilding( m_nextLaserFenceId );                       
            if( !nextFence || nextFence->m_type != Building::TypeLaserFence )
            {
                m_nextLaserFenceId = -1;
                return;
            }
            LaserFence *nextLaserFence = (LaserFence *) nextFence;

            int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );

            glDisable           ( GL_CULL_FACE );
            glEnable            ( GL_BLEND );
            glBlendFunc         ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );        
            glDepthMask         ( false );

            unsigned char alpha = 150;
            if( m_id.GetTeamId() == 255 )
            {
                glColor4ub( 100, 100, 100, alpha );
            }
            else if( m_solarMode || m_solarLinked )
            {
                RGBAColour teamColour = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
                teamColour.r *= 0.9f;
                teamColour.g *= 0.9f;
                teamColour.b *= 0.9f;
                RGBAColour baseColour( 20, 20, 20, alpha );
                float timeFactor = fabs(sinf( GetHighResTime() * 4 ));
                //timeFactor = max( timeFactor, 0.0f );

                RGBAColour actualColour = (baseColour * (1.0f - timeFactor)) + (teamColour * timeFactor);
                glColor4ub( actualColour.r, actualColour.g, actualColour.b, alpha );

            }
            else
            {
                const RGBAColour &colour = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
                glColor4ub( colour.r, colour.g, colour.b, alpha );
            }

            double ourFenceMaxHeight = GetFenceFullHeight();
            double theirFenceMaxHeight = nextLaserFence->GetFenceFullHeight();
            double predictedStatus = m_status;
            if( m_mode == ModeDisabling ) predictedStatus -= LASERFENCE_RAISESPEED * predictionTime;
            if( m_mode == ModeEnabling ) predictedStatus += LASERFENCE_RAISESPEED * predictionTime;
            if( g_app->m_editing ) predictedStatus = 1.0;
            
            double ourFenceHeight = ourFenceMaxHeight * predictedStatus;
            double theirFenceHeight = theirFenceMaxHeight * predictedStatus;
            double distance = ( m_pos - nextFence->m_pos ).Mag();
            double dx = distance / (ourFenceMaxHeight*2.0);
            double dz = ourFenceHeight / ourFenceMaxHeight;
            double timeOff = g_gameTime / 15.0;
            float texH = 1.0f;
            float texY = 0.0f;

            if( m_solarMode || m_solarLinked )
            {                
                texY += timeOff * 10;
                texH *= 7;             
                timeOff = 0;
            }

			Vector3 
				bottomLeft( m_pos - Vector3(0,ourFenceHeight/3,0) ),
				topLeft( m_pos + Vector3(0,ourFenceHeight,0) ),
				topRight( nextFence->m_pos + Vector3(0,theirFenceHeight,0) ),
				bottomRight( nextFence->m_pos - Vector3(0,theirFenceHeight/3,0) );

			int laserFenceTextureId = g_app->m_resource->GetTexture( "textures/laserfence.bmp" );
			int laserFence2TextureId = g_app->m_resource->GetTexture( "textures/laserfence2.bmp" );

            if( m_solarMode || m_solarLinked )
            {
                laserFence2TextureId = laserFenceTextureId;
            }

            if( buildingDetail < 3 )
            {

                gglActiveTextureARB  (GL_TEXTURE0_ARB);
                glBindTexture       (GL_TEXTURE_2D, laserFenceTextureId );
	            glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	            glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_REPLACE);        
                glEnable            (GL_TEXTURE_2D);

                gglActiveTextureARB  (GL_TEXTURE1_ARB);
                glBindTexture       (GL_TEXTURE_2D, laserFence2TextureId );
	            glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	            glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
                glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
                glTexEnvf           (GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE);
                glEnable            (GL_TEXTURE_2D);
        
                glBegin(GL_QUADS);
                    gglMultiTexCoord2fARB ( GL_TEXTURE0_ARB, timeOff, 0.0 );          
                    gglMultiTexCoord2fARB ( GL_TEXTURE1_ARB, 0, texY );          
                    glVertex3dv ( bottomLeft.GetData() );
           
                    gglMultiTexCoord2fARB ( GL_TEXTURE0_ARB, timeOff, dz );
                    gglMultiTexCoord2fARB ( GL_TEXTURE1_ARB, 0, texY+texH );          
                    glVertex3dv ( topLeft.GetData() );
            
                    gglMultiTexCoord2fARB ( GL_TEXTURE0_ARB, timeOff + dx, dz );
                    gglMultiTexCoord2fARB ( GL_TEXTURE1_ARB, 1, texY+texH );          
                    glVertex3dv ( topRight.GetData() );
            
                    gglMultiTexCoord2fARB ( GL_TEXTURE0_ARB, timeOff + dx, 0.0 );
                    gglMultiTexCoord2fARB ( GL_TEXTURE1_ARB, 1, texY );          
                    glVertex3dv ( bottomRight.GetData() );
                glEnd();

                gglActiveTextureARB  (GL_TEXTURE1_ARB);
                glDisable           (GL_TEXTURE_2D);
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
            
                gglActiveTextureARB  (GL_TEXTURE0_ARB);
                glDisable           (GL_TEXTURE_2D);
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
                glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
                glTexEnvf           (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

            }
            
            //
            // Blend another poly over the top for burn effect

            glBindTexture       (GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laserfence2.bmp" ) );
	        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	        glTexParameteri	    (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
            glBlendFunc         (GL_SRC_ALPHA, GL_ONE);
            glEnable            (GL_TEXTURE_2D);

            glBegin(GL_QUADS);
                glTexCoord2f( 0, texY );          
				glVertex3dv ( bottomLeft.GetData() );
            
                glTexCoord2f( 0, texY+texH );          
                glVertex3dv ( topLeft.GetData() );
            
                glTexCoord2f( 1, texY+texH );          
                glVertex3dv ( topRight.GetData() );
            
                glTexCoord2f( 1, texY );          
                glVertex3dv ( bottomRight.GetData() );
            glEnd();

            glDisable           (GL_TEXTURE_2D);
        

            //
            // Gimme a line across the top
            glLineWidth ( 2.0 );
            if( m_solarMode || m_solarLinked )
            {
                glLineWidth( 0.5f );
            }

            glEnable    ( GL_LINE_SMOOTH );
       
            glBegin( GL_LINES );
                glVertex3dv( topLeft.GetData() );
                glVertex3dv( topRight.GetData() );
            glEnd();

            glDepthMask ( true );
            glDisable   ( GL_BLEND );
            glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glEnable    ( GL_CULL_FACE );
            glDisable   ( GL_LINE_SMOOTH );
        }
    }
}


void LaserFence::RenderLights()
{
}


void LaserFence::Enable()
{
    if( m_mode != ModeEnabled &&
		m_mode != ModeNeverOn &&
        m_mode != ModeAlwaysOn )
    {
        m_mode = ModeEnabling;
    }

	if( m_mode == ModeNeverOn )
	{
		LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( m_nextLaserFenceId );
        if( nextFence ) nextFence->Enable();
	}

	m_nextToggled = false;
}


void LaserFence::Disable ()
{
    if( m_mode != ModeDisabled &&
		m_mode != ModeNeverOn &&
        m_mode != ModeAlwaysOn)
    {
        m_mode = ModeDisabling;
    }

	if( m_mode == ModeNeverOn )
	{
		LaserFence *nextFence = (LaserFence *) g_app->m_location->GetBuilding( m_nextLaserFenceId );
        if( nextFence ) nextFence->Disable();
	}

	m_nextToggled = false;
}


void LaserFence::Toggle ()
{
    switch( m_mode )
    {
        case ModeDisabled:
        case ModeDisabling:
            Enable();
            break;

        case ModeEnabled:
        case ModeEnabling:
            Disable();
            break;
    }

	m_nextToggled = false;
}


bool LaserFence::IsEnabled()
{
	return (m_mode == ModeEnabled) || (m_mode == ModeEnabling) || ( m_mode == ModeAlwaysOn );
}


void LaserFence::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    char *word = _in->GetNextToken();  
    m_nextLaserFenceId = atoi(word);    

    if( _in->TokenAvailable() ) m_scale = iv_atof(_in->GetNextToken());
    if( _in->TokenAvailable() ) m_mode = iv_atof(_in->GetNextToken());
}


void LaserFence::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_nextLaserFenceId);
    _out->printf( "%-6.2f", m_scale);
    _out->printf( "%d", m_mode );
}


int LaserFence::GetBuildingLink()
{
    return m_nextLaserFenceId;
}


void LaserFence::SetBuildingLink( int _buildingId )
{
    m_nextLaserFenceId = _buildingId;
}


void LaserFence::Electrocute( Vector3 const &_pos )
{
    g_app->m_soundSystem->TriggerBuildingEvent( this, "Electrocute" );
}


bool LaserFence::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    if( m_mode == ModeDisabled || g_app->m_editing )
    {
        SpherePackage sphere(_pos, _radius);
        Matrix34 transform(m_front, m_up, m_pos);
        transform.f *= m_scale;
        transform.u *= m_scale;
        transform.r *= m_scale;
        return m_shape->SphereHit(&sphere, transform);
    }

    //
    // Fake this for now using 2 ray hits
    // One running across the sphere in the x direction, the other in the z direction

    Vector3 ray1Start = _pos - Vector3(_radius,0,0);
    Vector3 ray1Dir(1,0,0);
    double ray1Len = _radius * 2;
    if( DoesRayHit( ray1Start, ray1Dir, ray1Len ) ) 
		return true;

    Vector3 ray2Start = _pos - Vector3(0,0,_radius);
    Vector3 ray2Dir(0,0,1);
    double ray2Len = _radius * 2;
    if( DoesRayHit( ray2Start, ray2Dir, ray2Len ) ) 
		return true;

    //
    // Fence is ok
    // Test against the shape
    
    SpherePackage sphere(_pos, _radius);
    Matrix34 transform(m_front, m_up, m_pos);
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
    return m_shape->SphereHit(&sphere, transform);

}


bool LaserFence::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                            double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( m_mode == ModeDisabled ||  g_app->m_editing )
    {
		RayPackage ray(_rayStart, _rayDir, _rayLen);
		Matrix34 transform(m_front, m_up, m_pos);
        transform.f *= m_scale;
        transform.u *= m_scale;
        transform.r *= m_scale;
		return m_shape->RayHit(&ray, transform, true);
    }
    
    if( m_nextLaserFenceId != -1 && m_status > 0.0 )
    {
        Building *nextFence = g_app->m_location->GetBuilding( m_nextLaserFenceId );
        double maxHeight = GetFenceFullHeight();
        double fenceHeight = maxHeight * m_status;
        Vector3 pos1 = m_pos - Vector3(0,fenceHeight/3,0);
        Vector3 pos2 = m_pos + Vector3(0,fenceHeight,0);
        Vector3 pos3 = nextFence->m_pos - Vector3(0,fenceHeight/3,0);
        Vector3 pos4 = nextFence->m_pos + Vector3(0,fenceHeight,0);
           
        bool hitTri1 = false;
        bool hitTri2 = false;

        hitTri1 = RayTriIntersection( _rayStart, _rayDir, pos1, pos2, pos4, _rayLen, _pos );
        if( !hitTri1 ) hitTri2 = RayTriIntersection( _rayStart, _rayDir, pos4, pos3, pos1, _rayLen, _pos );

        if( hitTri1 || hitTri2 )
        {
            if( _norm )
            {
                Vector3 v1 = pos1 - pos2;
                Vector3 v2 = pos1 - pos3;
                *_norm = ( v2 ^ v1 ).Normalise();
            }
            return true;
        }  
    }
    
    //return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );

	RayPackage ray(_rayStart, _rayDir, _rayLen);
	Matrix34 transform(m_front, m_up, m_pos);
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
	return m_shape->RayHit(&ray, transform, true);

}


bool LaserFence::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return DoesSphereHit( _transform.pos, _shape->m_rootFragment->m_radius );
}


void LaserFence::ListSoundEvents(LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "Spark" );
    _list->PutData( "Electrocute" );
}

Vector3 LaserFence::GetTopPosition()
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    mat.u *= m_scale;
    mat.r *= m_scale;
    mat.f *= m_scale;
    return m_marker2->GetWorldMatrix( mat ).pos;
}

void LaserFence::ProvidePower()
{
    m_solarMode = true;
    m_surgeReceived = GetNetworkTime();
	if( !m_solarLinked )
	{
		SetSolarLinked();
	}
}

void LaserFence::SetSolarLinked()
{
	if( m_mode != LaserFence::ModeAlwaysOn )
	{
		m_solarLinked = true;
		if( GetBuildingLink() != -1 )
		{
			LaserFence *fence = (LaserFence *)g_app->m_location->GetBuilding( GetBuildingLink() );
			if( fence && !fence->m_solarLinked )
			{
				fence->SetSolarLinked();
				
			}
		}
	}
}
