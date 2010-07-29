#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/matrix34.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"
#include "lib/math_utils.h"
#include "lib/hi_res_time.h"

#include "network/clienttoserver.h"

#include "sound/soundsystem.h"

#include "app.h"
#include "camera.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#ifdef USE_SEPULVEDA_HELP_TUTORIAL
    #include "sepulveda.h"
#endif
#include "main.h"
#include "particle_system.h"
#include "global_world.h"

#include "worldobject/controlstation.h"
#include "worldobject/trunkport.h"


ControlStation::ControlStation()
:	Building(),
	m_controlBuildingId(-1),
	m_scored(false)
{
	m_type = Building::TypeControlStation;
    SetShape( g_app->m_resource->GetShape( "controlstation.shp" ) );	    
    m_lightPos = m_shape->m_rootFragment->LookupMarker("MarkerLightPos");
}

void ControlStation::Initialise(Building *_template)
{
	Building::Initialise( _template );

	m_controlBuildingId = ((ControlStation *)_template)->m_controlBuildingId;
}

bool ControlStation::Advance()
{
	RecalculateOwnership();
	return Building::Advance();

}

void ControlStation::RecalculateOwnership()
{
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
	
	Building *targetBuilding = g_app->m_location->GetBuilding( m_controlBuildingId );
    if( winningTeam == -1 )
    {
        /*Building *b = g_app->m_location->GetBuilding( m_controlBuildingId );
	    if( b )
        {
		    b->SetTeamId(255);
        }*/
		SetTeamId(255);
    }
    else if ( winningTeam != m_id.GetTeamId() )
    {
        Building *b = g_app->m_location->GetBuilding( m_controlBuildingId );
	    if( b )
        {
		    b->SetTeamId(winningTeam);
        }
		SetTeamId(winningTeam);
		if ( g_app->m_location->IsFriend(winningTeam,2) )
		{
			if ( !m_scored ) { g_app->m_globalWorld->m_research->GiveResearchPoints( GLOBALRESEARCH_POINTS_CONTROLTOWER ); m_scored = true; }
			ReprogramComplete();
			if ( targetBuilding ) { targetBuilding->ReprogramComplete(); }
			g_app->SaveProfile( true, true );
		}
    }
}

void ControlStation::SetBuildingLink(int _buildingId)
{
	m_controlBuildingId = _buildingId;
}

int ControlStation::GetBuildingLink()
{
	return m_controlBuildingId;
}

void ControlStation::RenderPorts()
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


bool ControlStation::PerformDepthSort( Vector3 &_centrePos )
{
    _centrePos = m_centrePos;
    return true;    
}


void ControlStation::RenderAlphas( double _predictionTime )
{    
    Building::RenderAlphas( _predictionTime );

    //
    // Control lines will be bright when we are near a control tower
    // And dim when we are not
    // Recalculate our distance to the nearest control tower once per second

    static int s_lastRecalculation = 0.0f;
    static float s_distanceScale = 0.0f;
    static float s_desiredDistanceScale = 0.0f;

    if( (int) GetHighResTime() > s_lastRecalculation )
    {
        s_lastRecalculation = (int) GetHighResTime();

        float nearest = 99999.9f;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && (building->m_type == TypeControlTower || building->m_type == TypeControlStation) )
                {
                    float camDist = (building->m_pos - g_app->m_camera->GetPos()).Mag();
                    if( camDist < nearest ) nearest = camDist;
                }
            }
        }

        if   ( nearest < 200.0f )      s_desiredDistanceScale = 1.0f;
        else                           s_desiredDistanceScale = 0.1f;
    }

    if( s_desiredDistanceScale > s_distanceScale )
    {
        s_distanceScale = ( s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.1f ) +
                          ( s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.1f) );
    }
    else
    {
        s_distanceScale = ( s_desiredDistanceScale * SERVER_ADVANCE_PERIOD * 0.03f ) +
                          ( s_distanceScale * (1.0f - SERVER_ADVANCE_PERIOD * 0.03f) );
    }
	
	//
    // Pre-compute some positions and shit

	Matrix34 rootMat(m_front, g_upVector, m_pos);
    Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
	Vector3 lightPos = worldMat.pos;

    Vector3 camR = g_app->m_camera->GetRight();
    Vector3 camU = g_app->m_camera->GetUp();

    RGBAColour colour;
    if( m_id.GetTeamId() == 255 )
	{
		colour.Set( 128, 128, 128, 255 );
	}
	else
	{
		colour = g_app->m_location->m_teams[ m_id.GetTeamId() ].m_colour;
	}

	Building *target = g_app->m_location->GetBuilding( m_controlBuildingId );
    
/*
if( target &&
        m_id.GetTeamId() >= 0 && 
        m_id.GetTeamId() < NUM_TEAMS &&
        !target->m_destroyed )
    {       
        ShapeMarker *marker = m_shape->m_rootFragment->LookupMarker( "MarkerDishDirection" );
        if( !marker ) return;

        Matrix34 mat( m_front, m_up, m_pos );
        Vector3 fromPos = marker->GetWorldMatrix(mat).pos;

        Vector3 targetPos = target->m_centrePos + g_upVector * 100;

        RGBAColour teamColour = g_app->m_location->m_teams[m_id.GetTeamId()].m_colour;
        glColor4ubv( teamColour.GetData() );

        Vector3 dirNormalised = (targetPos - fromPos).Normalise();

        targetPos = fromPos + ( targetPos - fromPos ) * 0.75f;

        Vector3 rightAngle = ( targetPos - fromPos ) ^ g_upVector;
        rightAngle.SetLength( 15 );

        glEnable( GL_BLEND );
        glBlendFunc( GL_SRC_ALPHA, GL_ONE );
        glShadeModel( GL_SMOOTH );
        glDisable( GL_CULL_FACE );
        glEnable( GL_DEPTH_TEST );
        glDepthMask( false );

        int numSteps = ( targetPos - fromPos ).Mag() / 15.0f;

        for( int j = 0; j < numSteps; ++j )
        {
            float timeOffset = fmodf( GetHighResTime() * 3, 1.0f ) * 1/(float)numSteps;
            Vector3 thisFromPos = fromPos + ( targetPos - fromPos ) * (timeOffset + j/(float)numSteps);
            Vector3 thisToPos = fromPos + ( targetPos - fromPos ) * (timeOffset + (j+1)/(float)numSteps);
            
            float alpha = 255 - 255 * j/(float)numSteps;
            float size = 1.0f;  // + j/(float)numSteps;

            glBegin( GL_QUAD_STRIP );
            for( int i = 0; i <= 10; ++i )
            {
                glColor4ub( teamColour.r, teamColour.g, teamColour.b, alpha );
                glVertex3fv( (thisFromPos + rightAngle * size).GetData() );

                glColor4ub( teamColour.r, teamColour.g, teamColour.b, 0 );
                glVertex3fv( (thisToPos + rightAngle * size * 1.5f).GetData() );

                rightAngle.RotateAround( dirNormalised * 2.0f * M_PI * 0.1f );
            }
            glEnd();
        }

        glShadeModel( GL_FLAT );
    }
*/
    //
    // Draw control line to heaven

    if( !g_app->m_editing )
    {
        Vector3 controlUp( 0, 50.0f + (m_id.GetUniqueId() % 50), 0 );

        glDisable       ( GL_CULL_FACE );
        glEnable        ( GL_BLEND );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE );
        glShadeModel    ( GL_SMOOTH );
        glDepthMask     ( false );

        glEnable        ( GL_TEXTURE_2D );
        glBindTexture   ( GL_TEXTURE_2D, g_app->m_resource->GetTexture( "textures/laser.bmp" ) );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	    glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );

        float w = (lightPos  - g_app->m_camera->GetPos()).Mag() * 0.002f;
        w = max( 0.5f, w );

        for( int i = 0; i < 10; ++i )
        {
            Vector3 thisUp1 = Vector3(0,-20,0) + controlUp * (float) i;
            Vector3 thisUp2 = Vector3(0,-20,0) + controlUp * (float) (i+1);

            int alpha = 255 - 255 * (float) i / 10.0f;
            int alpha2 = 255 - 255 * (float) (i+1) / 10.0f;

            alpha *= fabs( sinf( g_gameTime * 2 + (float) i / 5.0f ) );
            alpha2 *= fabs( sinf( g_gameTime * 2 + (float) (i+1) / 5.0f ) );

            alpha *= s_distanceScale;
            alpha2 *= s_distanceScale;

            float y = (float) i / 10.0f;
            float h = 1.0f / 10.0f;

            glBegin( GL_QUADS );
                glColor4ub( colour.r, colour.g, colour.b, alpha );

                glTexCoord2f(y,0);      glVertex3fv( (lightPos - camR * w + thisUp1).GetData() );
                glTexCoord2f(y,1);      glVertex3fv( (lightPos + camR * w + thisUp1).GetData() );

                glColor4ub( colour.r, colour.g, colour.b, alpha2 );

                glTexCoord2f(y+h,1);    glVertex3fv( (lightPos + camR * w + thisUp2).GetData() );
                glTexCoord2f(y+h,0);    glVertex3fv( (lightPos - camR * w + thisUp2).GetData() );
            glEnd();
        }

        glDisable       ( GL_TEXTURE_2D );

        glDepthMask     ( true );
        glShadeModel    ( GL_FLAT );
        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );
        glEnable        ( GL_CULL_FACE );
    }

    //
    // Draw our signal flash

    int lastSeqId = g_app->m_clientToServer->m_lastValidSequenceIdFromServer;

    if( (m_id.GetTeamId() != 255 && (lastSeqId % 10)/2 == m_id.GetTeamId() ) ||
        g_app->m_editing )
    {

	    Matrix34 rootMat(m_front, g_upVector, m_pos);
        Matrix34 worldMat = m_lightPos->GetWorldMatrix(rootMat);
	    Vector3 lightPos = worldMat.pos;

        float signalSize = 5.0f;

        glColor4ub( colour.r, colour.g, colour.b, 255 );

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
            float size = signalSize * (float) i / 10.0f;
            glBegin( GL_QUADS );
                glTexCoord2f    ( 0.0f, 0.0f );             glVertex3fv     ( (lightPos - camR * size - camU * size).GetData() );
                glTexCoord2f    ( 1.0f, 0.0f );             glVertex3fv     ( (lightPos + camR * size - camU * size).GetData() );
                glTexCoord2f    ( 1.0f, 1.0f );             glVertex3fv     ( (lightPos + camR * size + camU * size).GetData() );
                glTexCoord2f    ( 0.0f, 1.0f );             glVertex3fv     ( (lightPos - camR * size + camU * size).GetData() );
            glEnd();
        }

        glBlendFunc     ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
        glDisable       ( GL_BLEND );

        glDepthMask     ( true );
        glEnable        ( GL_CULL_FACE );
        glDisable       ( GL_TEXTURE_2D );
    }
}



void ControlStation::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    char *word = _in->GetNextToken();  
    m_controlBuildingId = atoi(word);
	if ( _in->TokenAvailable() ) {
		if ( atoi(_in->GetNextToken()) ) { m_scored = true; }
	}
}

void ControlStation::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d", m_controlBuildingId);
	if ( m_scored ) {
		_out->printf( "%6d", 1);
	} else {
		_out->printf( "%6d", 0);
	}
}

bool ControlStation::IsInView()
{
    return Building::IsInView();
}