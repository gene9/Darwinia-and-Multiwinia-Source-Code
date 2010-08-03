#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"
#include "lib/text_renderer.h"
#include "lib/math_utils.h"
#include "lib/3d_sprite.h"
#include "lib/preferences.h"

#include "worldobject/researchcrate.h"

#include "explosion.h"
#include "main.h"
#include "app.h"
#include "global_world.h"
#include "camera.h"
#include "renderer.h"
#include "globals.h"
#include "location.h"
#include "taskmanager.h"
#include "taskmanager_interface.h"
#include "team.h"

#include "sound/soundsystem.h"


ResearchCrate::ResearchCrate()
:   Building(),
    m_researchType(-1),
    m_inLibrary(false),
    m_reprogrammed(100.0f),
    m_end1(NULL),
    m_end2(NULL),
    m_level(1),
	m_flashTimer(0.0f)
{
    m_type = TypeResearchCrate;
    m_researchType = GlobalResearch::TypeEngineer;

    SetShape( g_app->m_resource->GetShape( "researchcrate.shp" ) );

    m_front.RotateAroundY( frand(2.0f * M_PI) );

    m_end1 = m_shape->m_rootFragment->LookupMarker( "MarkerGrab1" );
    m_end2 = m_shape->m_rootFragment->LookupMarker( "MarkerGrab2" );
}


void ResearchCrate::Initialise ( Building *_template )
{
    Building::Initialise( _template );

    m_researchType = ((ResearchCrate *) _template)->m_researchType;
    m_level = ((ResearchCrate *) _template)->m_level;
}

void ResearchCrate::RenderPorts()
{    
    glDisable       ( GL_CULL_FACE );
    glEnable        ( GL_TEXTURE_2D );
    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );
    glDepthMask     ( false );
    glEnable        ( GL_BLEND );
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
    glBegin         ( GL_QUADS );

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        Vector3 portPos;
        Vector3 portFront;
        GetPortPosition( i, portPos, portFront );

        Vector3 portUp = g_upVector;
        Matrix34 mat( portFront, portUp, portPos );
         
        //
        // Render the status light

        double size = 6.0;
        Vector3 camR = g_app->m_camera->GetRight() * size;
        Vector3 camU = g_app->m_camera->GetUp() * size;

        Vector3 statusPos = s_controlPadStatus->GetWorldMatrix( mat ).pos;
        statusPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(statusPos.x, statusPos.z);
        statusPos.y += 5.0;
        
        WorldObjectId occupantId = GetPortOccupant(i);
        if( !occupantId.IsValid() ) 
        {
            glColor4ub( 150, 150, 150, 255 );
        }
        else
        {
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()].m_colour;
            glColor4ubv( teamColour.GetData() );
        }

        glTexCoord2i( 0, 0 );           glVertex3fv( (statusPos - camR - camU).GetData() );
        glTexCoord2i( 1, 0 );           glVertex3fv( (statusPos + camR - camU).GetData() );
        glTexCoord2i( 1, 1 );           glVertex3fv( (statusPos + camR + camU).GetData() );
        glTexCoord2i( 0, 1 );           glVertex3fv( (statusPos - camR + camU).GetData() );
    }

    glEnd           ();
    glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glDisable       ( GL_BLEND );
    glDepthMask     ( true );
    glDisable       ( GL_TEXTURE_2D );
    glEnable        ( GL_CULL_FACE );

}

void ResearchCrate::SetDetail( int _detail )
{
    m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
    m_pos.y += 20.0f;

    Matrix34 mat( m_front, m_up, m_pos );
    m_centrePos = m_shape->CalculateCentre( mat );
    m_radius = m_shape->CalculateRadius( mat, m_centrePos );
}


bool ResearchCrate::Advance()
{
	Building::Advance();

    if( m_vel.Mag() > 1.0f )
    {
        m_pos += m_vel * SERVER_ADVANCE_PERIOD;
        m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
        m_vel *= ( 1.0f - SERVER_ADVANCE_PERIOD * 0.5f );

        Matrix34 mat( m_front, g_upVector, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
    }
    else
    {
        m_vel.Zero();
    }

	// Do the reprogramming
    int teamCount[NUM_TEAMS];
    memset( teamCount, 0, NUM_TEAMS*sizeof(int));

    for( int i = 0; i < GetNumPorts(); ++i )
    {
        WorldObjectId id = GetPortOccupant(i);
        if( id.IsValid() )
        {
            teamCount[id.GetTeamId()] ++;
        }
    }

    int winningTeam = -1;
    int num = 2;

	for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > num && 
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 && 
                 teamCount[i] > num && 
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 ) {
		SetTeamId(255);
		m_reprogrammed = 100.0f;
	} else if ( g_app->m_location->IsFriend(winningTeam,m_id.GetTeamId()) ) {
		Reprogram( m_id.GetTeamId() );
	} else {
		m_reprogrammed = 100.0f; // We changed hands
		SetTeamId(winningTeam);
    }

	if ( m_id.GetTeamId() < 0 || m_id.GetTeamId() >= NUM_TEAMS ) { return false; }

    if( m_reprogrammed <= 0.0f )
    {
		ReprogramComplete();

        Matrix34 mat( m_front, m_up, m_pos );
        g_explosionManager.AddExplosion( m_shape, mat, 1.0f );


		for ( int i = 0; i < NUM_TEAMS; i++ )
		{
			if ( g_app->m_location->IsFriend(m_id.GetTeamId(), i) )
			{
				int existingLevel = g_app->m_globalWorld->m_research->CurrentLevel( i, m_researchType );

				g_app->m_globalWorld->m_research->AddResearch( i, m_researchType );
				g_app->m_globalWorld->m_research->m_researchLevel[i][ m_researchType ] = m_level;

				g_app->m_soundSystem->TriggerBuildingEvent( this, "AquireResearch" );
			}
		}
		int teamId = m_id.GetTeamId();
		if ( g_app->m_location->IsFriend(teamId,2) ) { teamId = 2; }
		int existingLevel = g_app->m_globalWorld->m_research->CurrentLevel( teamId, m_researchType );
		if( existingLevel == 0 )
		{
			g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearch, m_researchType, 4.0f, teamId );
		}
		else
		{
			g_app->m_taskManagerInterface->SetCurrentMessage( TaskManagerInterface::MessageResearchUpgrade, m_researchType, 4.0f, teamId );
		}
        return true;
    }

    if( m_researchType > -1 &&
        g_app->m_globalWorld->m_research->HasResearch(m_id.GetTeamId(), m_researchType ) &&
        g_app->m_globalWorld->m_research->CurrentLevel(m_id.GetTeamId(),  m_researchType ) >= m_level )
    {
        return true;
    }

    return false;
}


bool ResearchCrate::NeedsReprogram()
{
    return( m_reprogrammed > 0.0f );
}


bool ResearchCrate::Reprogram( char _teamId )
{
	// If someone else takes over reprogramming, reset the progress
	//if ( _teamId != m_id.GetTeamId() )
	if ( !g_app->m_location->IsFriend(_teamId, m_id.GetTeamId()) )
	{
		m_id.SetTeamId(_teamId);
		m_reprogrammed = 100.0f;
	}
    m_reprogrammed -= SERVER_ADVANCE_PERIOD * 3.0f;
    return( m_reprogrammed <= 0.0f );
}


void ResearchCrate::GetEndPositions( Vector3 &_end1, Vector3 &_end2 )
{
    Matrix34 mat( m_front, m_up, m_pos );

    _end1 = m_end1->GetWorldMatrix(mat).pos;
    _end2 = m_end2->GetWorldMatrix(mat).pos;
}


void ResearchCrate::Render( float _predictionTime )
{
    if( m_reprogrammed <= 0.0f ) return;

    Vector3 rotateAround = g_upVector;
    rotateAround.RotateAroundX( g_gameTime * 1.0f );
    rotateAround.RotateAroundZ( g_gameTime * 0.7f );
    rotateAround.Normalise();

    m_front.RotateAround( rotateAround * g_advanceTime );
    m_up.RotateAround( rotateAround * g_advanceTime );

    Vector3 predictedPos = m_pos + m_vel * _predictionTime;
	Matrix34 mat(m_front, m_up, predictedPos);

	m_shape->Render(0.0f, mat);

    if( g_app->m_editing && m_researchType != -1 )
    {
        g_gameFont.DrawText3DCentre( predictedPos + Vector3(0,25,0), 5, GlobalResearch::GetTypeName( m_researchType ) );
        g_gameFont.DrawText3DCentre( predictedPos + Vector3(0,20,0), 5, "%2.2f", m_reprogrammed );
    }
}


void ResearchCrate::RenderAlphas( float _predictionTime )
{
	if ( g_app->m_editing )
	{
        Matrix34 mat( m_front, m_up, m_pos );
        m_centrePos = m_shape->CalculateCentre( mat );
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );
	}

	m_flashTimer -= SERVER_ADVANCE_PERIOD;

    Building::RenderAlphas( _predictionTime );

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

    for( int i = 0; i < maxBlobs; ++i )
    {
        Vector3 pos = m_centrePos;
        pos.x += sinf(timeIndex+i) * i * 0.3f;
        pos.y += cosf(timeIndex+i) * sinf(i*10) * 5;
        pos.z += cosf(timeIndex+i) * i * 0.3f;

        float size = 5.0f + sinf(timeIndex+i*10) * 7.0f;
        size = max( size, 2.0f );

        //glColor4f( 0.6f, 0.2f, 0.1f, alpha);
        glColor4f( 0.1f, 0.2f, 0.8f, alpha);
		if ( m_id.GetTeamId() != 255 )
		{
			if ( m_flashTimer <= 1.0f )
			{
				if ( m_flashTimer <= 0.0f ) { m_flashTimer = m_reprogrammed / 10.0; }
				glColor3ub( g_app->m_location->m_teams[m_id.GetTeamId()].m_colour.r, g_app->m_location->m_teams[m_id.GetTeamId()].m_colour.g, g_app->m_location->m_teams[m_id.GetTeamId()].m_colour.b);

			}
		}
        glBegin( GL_QUADS );
            glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
            glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
        glEnd();
    }


    //
    // Starbursts

    alpha = 1.0f - m_reprogrammed / 100.0f;
    alpha *= 0.3f;

    glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/starburst.bmp" ) );

    if( alpha > 0.0f )
    {
        int numStars = 10;
        if( buildingDetail == 2 ) numStars = 5;
        if( buildingDetail == 3 ) numStars = 2;

        for( int i = 0; i < numStars; ++i )
        {
            Vector3 pos = m_centrePos;
            pos.x += sinf(timeIndex+i) * i * 0.3f;
            pos.y += (cosf(timeIndex+i) * cosf(i*10) * 2);
            pos.z += cosf(timeIndex+i) * i * 0.3f;

            float size = i * 10 * alpha;
            if( i > numStars - 2 ) size = i * 20 * alpha;

            //glColor4f( 1.0f, 0.4f, 0.2f, alpha );
            glColor4f( 0.1f, 0.2f, 0.8f, alpha);

            glBegin( GL_QUADS );
                glTexCoord2i(0,0);      glVertex3fv( (pos - camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,0);      glVertex3fv( (pos + camRight * size + camUp * size).GetData() );
                glTexCoord2i(1,1);      glVertex3fv( (pos + camRight * size - camUp * size).GetData() );
                glTexCoord2i(0,1);      glVertex3fv( (pos - camRight * size - camUp * size).GetData() );
            glEnd();
        }
    }


    //
    // Draw control line to heaven

    alpha = 1.0f - m_reprogrammed / 100.0f;
    alpha *= ( 0.5f + fabs(cosf(g_gameTime)) * 0.5f );

    if( alpha > 0.0f )
    {
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
        glDisable( GL_CULL_FACE );
        glShadeModel( GL_SMOOTH );

        float w = 10.0f * alpha;

        glBegin( GL_QUADS );
            glColor4f( 0.1f, 0.2f, 0.8f, alpha);
            glTexCoord2i(0,0);      glVertex3fv( (m_pos + Vector3(0,-50,0) - camRight * w).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (m_pos + Vector3(0,-50,0) + camRight * w).GetData() );

            glColor4f( 0.1f, 0.2f, 0.8f, 0.0f );
            glTexCoord2i(1,1);      glVertex3fv( (m_pos + Vector3(0,1000,0) + camRight * w).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (m_pos + Vector3(0,1000,0) - camRight * w).GetData() );
        glEnd();

        w *= 0.3f;

        glBegin( GL_QUADS );
            glColor4f( 0.1f, 0.2f, 0.8f, alpha);
            glTexCoord2i(0,0);      glVertex3fv( (m_pos + Vector3(0,-50,0) - camRight * w).GetData() );
            glTexCoord2i(0,1);      glVertex3fv( (m_pos + Vector3(0,-50,0) + camRight * w).GetData() );

            glColor4f( 0.1f, 0.2f, 0.8f, 0.0f );
            glTexCoord2i(1,1);      glVertex3fv( (m_pos + Vector3(0,1000,0) + camRight * w).GetData() );
            glTexCoord2i(1,0);      glVertex3fv( (m_pos + Vector3(0,1000,0) - camRight * w).GetData() );
        glEnd();
    }


    glShadeModel    ( GL_FLAT );
    glDisable       ( GL_TEXTURE_2D );
    glDepthMask     ( true );

}


bool ResearchCrate::RenderPixelEffect(float _predictionTime)
{
//	Matrix34 mat(m_front, m_up, m_pos);
//	m_shape->Render(0.0f, mat);
//	g_app->m_renderer->MarkUsedCells(m_shape, mat);
    return false;
}


void ResearchCrate::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_researchType = GlobalResearch::GetType( _in->GetNextToken() );

    if( _in->TokenAvailable() )
    {
        m_level = atoi( _in->GetNextToken() );
    }
}


void ResearchCrate::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%s ", GlobalResearch::GetTypeName( m_researchType) );
    _out->printf( "%6d", m_level );
}


void ResearchCrate::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "AquireResearch" );
}

/*
bool ResearchCrate::DoesSphereHit(Vector3 const &_pos, float _radius)
{
    return false;
}


bool ResearchCrate::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool ResearchCrate::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir,
                              float _rayLen, Vector3 *_pos, Vector3 *norm )
{
    return RaySphereIntersection(_rayStart, _rayDir, m_pos, m_radius, _rayLen);
}

*/

bool ResearchCrate::IsInView()
{
    if( Building::IsInView() ) return true;

    if( g_app->m_camera->PosInViewFrustum( m_pos+Vector3(0,g_app->m_camera->GetPos().y,0) ))
    {
        return true;
    }

    return false;
}

