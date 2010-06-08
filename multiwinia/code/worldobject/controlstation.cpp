#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/matrix34.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/filesys/text_stream_readers.h"
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
    m_pulseBombId(-1)
{
	m_type = Building::TypeControlStation;
    SetShape( g_app->m_resource->GetShape( "controlstation.shp" ) );	    
}

void ControlStation::Initialise(Building *_template)
{
	Building::Initialise( _template );

	m_controlBuildingId = ((ControlStation *)_template)->m_controlBuildingId;
}


void ControlStation::LinkToPulseBomb( int buildingId )
{
    if( m_pulseBombId == -1 )
    {
        m_pulseBombId = buildingId;
        SetShape( g_app->m_resource->GetShape( "pulsebombcontrolstation.shp" ) );
        
        m_lights.Empty();
        m_ports.Empty();
        Building::Initialise( this );
    }
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
    if( m_pulseBombId != -1 ) num = 1;
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

    if( winningTeam == -1 )
    {
        /*Building *b = g_app->m_location->GetBuilding( m_controlBuildingId );
	    if( b )
        {
		    b->SetTeamId(255);
        }*/
		SetTeamId(255);
    }
    else
    {
        Building *b = g_app->m_location->GetBuilding( m_controlBuildingId );
	    if( b )
        {
		    b->SetTeamId(winningTeam);
        }
		SetTeamId(winningTeam);
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
            RGBAColour teamColour = g_app->m_location->m_teams[occupantId.GetTeamId()]->m_colour;
            glColor4ubv( teamColour.GetData() );
        }

        glTexCoord2i( 0, 0 );           glVertex3dv( (statusPos - camR - camU).GetData() );
        glTexCoord2i( 1, 0 );           glVertex3dv( (statusPos + camR - camU).GetData() );
        glTexCoord2i( 1, 1 );           glVertex3dv( (statusPos + camR + camU).GetData() );
        glTexCoord2i( 0, 1 );           glVertex3dv( (statusPos - camR + camU).GetData() );
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
    if( m_pulseBombId != -1 ) return false;

    _centrePos = m_centrePos;
    return true;    
}


void ControlStation::RenderAlphas( double _predictionTime )
{    
    Building::RenderAlphas( _predictionTime );

    Building *target = g_app->m_location->GetBuilding( m_pulseBombId );
    
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

        RGBAColour teamColour = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
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
                glVertex3dv( (thisFromPos + rightAngle * size).GetData() );

                glColor4ub( teamColour.r, teamColour.g, teamColour.b, 0 );
                glVertex3dv( (thisToPos + rightAngle * size * 1.5f).GetData() );

                rightAngle.RotateAround( dirNormalised * 2.0f * M_PI * 0.1f );
            }
            glEnd();
        }

        glShadeModel( GL_FLAT );
    }
}



void ControlStation::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    char *word = _in->GetNextToken();  
    m_controlBuildingId = atoi(word);    
}

void ControlStation::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6d", m_controlBuildingId);
}

bool ControlStation::IsInView()
{
    if( m_pulseBombId != -1 ) return true;
    
    return Building::IsInView();
}