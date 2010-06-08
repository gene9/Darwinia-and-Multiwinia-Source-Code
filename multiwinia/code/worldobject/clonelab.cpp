#include "lib/universal_include.h"

#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/debug_render.h"

#include "app.h"
#include "globals.h"
#include "location.h"
#include "particle_system.h"
#include "camera.h"
#include "team.h"
#include "global_world.h"

#include "worldobject/incubator.h"
#include "worldobject/clonelab.h"
#include "worldobject/ai.h"



CloneLab::CloneLab()
:   Incubator(),
    m_startingSpirits(-1),
    m_aiTarget(NULL)
{
    m_type = Building::TypeCloneLab;

    SetShape( g_app->m_resource->GetShape( "clonelab.shp" ) );
}

CloneLab::~CloneLab()
{
    if( m_aiTarget )
    {
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.EmptyAndDelete();
        m_aiTarget = NULL;
    }
}

void CloneLab::Initialise( Building *_template )
{
    Incubator::Initialise( _template );
}

void CloneLab::InitialiseAITarget()
{
    m_aiTarget = (AITarget *)Building::CreateBuilding( Building::TypeAITarget );

    Matrix34 mat( m_front, g_upVector, m_pos );

    AITarget targetTemplate;
    targetTemplate.m_pos       = m_exit->GetWorldMatrix(mat).pos;
    targetTemplate.m_pos.y     = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

    m_aiTarget->Initialise( (Building *)&targetTemplate );
    m_aiTarget->m_id.SetUnitId( UNIT_BUILDINGS );
    m_aiTarget->m_id.SetUniqueId( g_app->m_globalWorld->GenerateBuildingId() );   
    //m_aiTarget->m_priorityModifier = 0.1;

    g_app->m_location->m_buildings.PutData( m_aiTarget );

    //g_app->m_location->RecalculateAITargets();
}


bool CloneLab::Advance()
{
    if( !m_aiTarget &&
        NumSpiritsInside() > 0 )
    {
        InitialiseAITarget();
    }

    if( m_startingSpirits == -1 )
    {
        m_startingSpirits = NumSpiritsInside();
    }

    RecalculateOwnership();

    if( NumSpiritsInside() == 0 &&
        m_aiTarget )
    {
        // no more spirits left - this building is now worthless so remove it from ai grid
        m_aiTarget->m_destroyed = true;
        m_aiTarget->m_neighbours.EmptyAndDelete();
        m_aiTarget = NULL;
    }

    if( m_id.GetTeamId() != 255 )
    {
        return Incubator::Advance();
    }
    else
    {
        for( int i = 0; i < m_spirits.Size(); ++i )
        {
            if( m_spirits.ValidIndex(i) )
            {
                Spirit *spirit = m_spirits.GetPointer(i);
                spirit->Advance();
            }
        }
        return Building::Advance();
    }
}

void CloneLab::Read( TextReader *_in, bool _dynamic )
{
    Incubator::Read( _in, _dynamic );
}


void CloneLab::Write( TextWriter *_out )
{   
    Incubator::Write( _out );
}

void CloneLab::RecalculateOwnership()
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
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        if( teamCount[i] > 2 && 
            winningTeam == -1 )
        {
            winningTeam = i;
        }
        else if( winningTeam != -1 && 
                 teamCount[i] > 2 && 
                 teamCount[i] > teamCount[winningTeam] )
        {
            winningTeam = i;
        }
    }

    if( winningTeam == -1 )
    {
        SetTeamId(255);
    }
    else
    {
        SetTeamId(winningTeam);
    }
}

void CloneLab::RenderPorts()
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
