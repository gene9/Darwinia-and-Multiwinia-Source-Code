#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/hi_res_time.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_renderer.h"
#include "lib/text_stream_readers.h"
#include "lib/debug_render.h"

#include "worldobject/generichub.h"
#include "worldobject/darwinian.h"
#include "worldobject/controltower.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "global_world.h"
#include "particle_system.h"
#include "main.h"
#include "entity_grid.h"
#include "user_input.h"

#include "sound/soundsystem.h"


// ****************************************************************************
// Class DynamicBase
// ****************************************************************************

DynamicBase::DynamicBase()
:   Building(),
    m_buildingLink(-1)
{
    strcpy( m_shapeName, "none" );
}

void DynamicBase::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_buildingLink = ((DynamicBase *) _template)->m_buildingLink;
    SetShapeName( ((DynamicBase *) _template)->m_shapeName );
}

void DynamicBase::Render( float _predictionTime )
{
    if( m_shape )
    {
        Building::Render( _predictionTime );
    }
    else
    {
        RenderSphere( m_pos, 40.0f );
    }
	/*Matrix34 mat(m_front, m_up, m_pos);
	m_shape->Render(_predictionTime, mat);*/
}

bool DynamicBase::Advance()
{
    return Building::Advance();
}

void DynamicBase::ListSoundEvents( LList<char *> *_list )
{
    Building::ListSoundEvents( _list );

    _list->PutData( "TriggerSurge" );
}


void DynamicBase::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_buildingLink = atoi( _in->GetNextToken() );
    SetShapeName( _in->GetNextToken() );
}

void DynamicBase::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_buildingLink );
    _out->printf( "%s  ", m_shapeName );
}

int DynamicBase::GetBuildingLink()
{
    return m_buildingLink;
}

void DynamicBase::SetBuildingLink( int _buildingId )
{
    m_buildingLink = _buildingId;
    //m_powerLink = _buildingId;
}

void DynamicBase::SetShapeName( char *_shapeName )
{
    strcpy( m_shapeName, _shapeName );

    if( strcmp( m_shapeName, "none" ) != 0 )
    {
        SetShape( g_app->m_resource->GetShape( m_shapeName ) );

        Matrix34 mat( m_front, m_up, m_pos );

        m_centrePos = m_shape->CalculateCentre( mat );        
        m_radius = m_shape->CalculateRadius( mat, m_centrePos );
    }
}


// ****************************************************************************
// Class DynamicHub
// ****************************************************************************

DynamicHub::DynamicHub()
:   DynamicBase(),
    m_enabled(false),
    m_activeLinks(0),
    m_numLinks(-1),
    m_reprogrammed(false),
    m_currentScore(0),
    m_requiredScore(0),
    m_minActiveLinks(0)
{
    m_type = TypeDynamicHub;
    //SetShape( g_app->m_resource->GetShape( "generator.shp" ) );
}

void DynamicHub::Initialise( Building *_template )
{
    DynamicBase::Initialise( _template );
    DynamicHub *hubCopy = (DynamicHub *) _template;
    m_requiredScore = hubCopy->m_requiredScore;
    m_currentScore = hubCopy->m_currentScore;
    m_minActiveLinks = hubCopy->m_minActiveLinks;
}

void DynamicHub::ReprogramComplete()
{
    m_reprogrammed = true;
    g_app->m_soundSystem->TriggerBuildingEvent( this, "Enable" );
}


void DynamicHub::ListSoundEvents( LList<char *> *_list )
{
    DynamicBase::ListSoundEvents( _list );

    _list->PutData( "Enable" );
}


bool DynamicHub::Advance()
{
    if( !m_reprogrammed ||
        m_numLinks == -1)
    {
        m_numLinks = 0;
        //
        // Check to see if our control tower has been captured.
        // This can happen if a user captures the control tower, exits the level and saves,
        // then returns to the level.  The tower is captured and cannot be changed, but
        // the m_enabled state of this building has been lost.
        
        bool towerFound = false;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                Building *building = g_app->m_location->m_buildings[i];
                if( building && building->m_type == TypeControlTower )
                {
                    ControlTower *tower = (ControlTower *) building;
                    if( tower->GetBuildingLink() == m_id.GetUniqueId() )
                    {
                        towerFound = true;
                        if( tower->m_id.GetTeamId() == m_id.GetTeamId() )
                        {                        
                            m_reprogrammed = true;
                            break;
                        }
                    }
                }

                if( building && building->m_type == TypeDynamicNode )
                {
                    DynamicNode *node = (DynamicNode *)building;
                    if( node->GetBuildingLink() == m_id.GetUniqueId() )
                    {
                        m_numLinks++;
                    }
                }
            }
        }
        if( !towerFound )
        {
            m_reprogrammed = true;
        }
    }

    if( m_reprogrammed )
    {
        if( m_requiredScore > 0 &&
           m_currentScore >= m_requiredScore &&
           m_activeLinks >= m_minActiveLinks )
        {
            m_enabled = true;
        }

        if( m_currentScore >= m_requiredScore )
        {
            if( m_enabled )
            {
                GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
                if( gb && !gb->m_online )
                {
                    gb->m_online = true;
                    g_app->m_globalWorld->EvaluateEvents();
                } 
            }
            else if( !g_app->m_location->MissionComplete() )
            {
                GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
                if( gb && gb->m_online )
                {
                    gb->m_online = false;
                    g_app->m_globalWorld->EvaluateEvents();
                }
            }
        }
    }


    return DynamicBase::Advance();
}


void DynamicHub::Render( float _predictionTime )
{
    DynamicBase::Render( _predictionTime );
}

void DynamicHub::ActivateLink()
{
    m_activeLinks++;
    if( m_activeLinks >= m_numLinks )
    {
        m_enabled = true;
    }
}

void DynamicHub::DeactivateLink()
{
    m_activeLinks--;
    if( m_activeLinks < m_numLinks )
    {
        m_enabled = false;
    }
}

bool DynamicHub::ChangeScore( int _points )
{
    if( m_reprogrammed && 
        m_currentScore < m_requiredScore )
    {
        m_currentScore += _points;
        m_currentScore = min( m_currentScore, m_requiredScore );
        return true;
    }
    return false;
}

void DynamicHub::Read( TextReader *_in, bool _dynamic )
{
    DynamicBase::Read( _in, _dynamic );
    m_requiredScore = atoi( _in->GetNextToken() );
    m_currentScore = atoi( _in->GetNextToken() );
    m_minActiveLinks = atoi( _in->GetNextToken() );
}

void DynamicHub::Write( FileWriter *_out )
{
    DynamicBase::Write( _out );
    _out->printf( "%-8d", m_requiredScore );
    _out->printf( "%-8d", m_currentScore );
    _out->printf( "%-8d", m_minActiveLinks );
}

char *DynamicHub::GetObjectiveCounter()
{
    static char result[256];

    if( m_requiredScore > 0 )
    {
        float current = m_currentScore;
        float required = m_requiredScore;
        float percentComplete = current / required * 100.0f;
        sprintf( result, "percent complete: %.0f", percentComplete );
    }
    else
    {
        sprintf( result, "" );
    }
    return result;
}

int DynamicHub::PointsPerHub()
{
    return m_requiredScore / m_minActiveLinks;
}

// ****************************************************************************
// Class DynamicNode
// ****************************************************************************

DynamicNode::DynamicNode()
:   DynamicBase(),
    m_operating(false),
    m_scoreValue(0),
    m_scoreTimer(1.0f),
    m_scoreSupplied(0)
{
    m_type = TypeDynamicNode;
    //SetShape( g_app->m_resource->GetShape( "solarpanel.shp" ) );
}

void DynamicNode::Initialise( Building *_template )
{
    DynamicBase::Initialise( _template );
    DynamicNode *nodeCopy = (DynamicNode *) _template;
    m_scoreValue = nodeCopy->m_scoreValue;
    m_scoreSupplied = nodeCopy->m_scoreSupplied;
}

bool DynamicNode::Advance()
{
    bool friendly = true;
    for( int i = 0; i < GetNumPorts(); ++i )
    {
        if( m_ports[i]->m_occupant.GetTeamId() == 1 )
        {
            friendly = false;
            break;
        }
    }

    if( !m_operating )
    {
        if( GetNumPorts() > 0 )
        {
            if( GetNumPortsOccupied() == GetNumPorts() )
            {
                if( friendly )
                {
                    DynamicHub *hub = (DynamicHub *)g_app->m_location->GetBuilding( m_buildingLink );
                    if( hub && hub->m_type == Building::TypeDynamicHub )
                    {
                        m_operating = true;
                        hub->ActivateLink();
                    }
                }
            }
        }
    }
    else
    {     
        if( GetNumPortsOccupied() < GetNumPorts() )
        {
            DynamicHub *hub = (DynamicHub *)g_app->m_location->GetBuilding( m_buildingLink );
            if( hub && hub->m_type == Building::TypeDynamicHub )
            {
                m_operating = false;
                hub->DeactivateLink();
            }
        }
        else
        {
            if( m_scoreValue > 0 )
            {
                m_scoreTimer -= SERVER_ADVANCE_PERIOD;
                if( m_scoreTimer <= 0.0f )
                {
                    m_scoreTimer = 1.0f;
                    DynamicHub *hub = (DynamicHub *)g_app->m_location->GetBuilding( m_buildingLink );
                    if( hub && hub->m_type == Building::TypeDynamicHub )
                    {
                        int scoreMod = 1;
                        if( !friendly ) scoreMod = -1;

                        if( hub->PointsPerHub() > m_scoreSupplied )
                        {
                            if( hub->ChangeScore( m_scoreValue * scoreMod ) )
                            {
                                m_scoreSupplied += m_scoreValue * scoreMod;
                            }
                        }
                    }
                }
            }
        }
    }
    return DynamicBase::Advance();
}


void DynamicNode::RenderPorts()
{
    Building::RenderPorts();
}


void DynamicNode::Render( float _predictionTime )
{
    if( g_app->m_editing )
    {
        m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        Vector3 right( 1, 0, 0 );
        m_front = right ^ m_up;
    }

    glShadeModel( GL_SMOOTH );
    DynamicBase::Render( _predictionTime );
    glShadeModel( GL_FLAT );
}


void DynamicNode::RenderAlphas( float _predictionTime )
{
    DynamicBase::RenderAlphas( _predictionTime );
}


void DynamicNode::ListSoundEvents( LList<char *> *_list )
{
    DynamicBase::ListSoundEvents( _list );

    _list->PutData( "Operate" );
}

void DynamicNode::ReprogramComplete()
{
    if( GetNumPorts() == 0 )
    {
        DynamicHub *hub = (DynamicHub *)g_app->m_location->GetBuilding( m_buildingLink );
        if( hub && hub->m_type == Building::TypeDynamicHub )
        {
            m_operating = true;
            hub->ActivateLink();
        }
    }
}

void DynamicNode::Read( TextReader *_in, bool _dynamic )
{
    DynamicBase::Read( _in, _dynamic );
    m_scoreValue = atoi( _in->GetNextToken() );
    m_scoreSupplied = atoi( _in->GetNextToken() );
}

void DynamicNode::Write( FileWriter *_out )
{
    DynamicBase::Write( _out );
    _out->printf( "%-8d", m_scoreValue );
    _out->printf( "%-8d", m_scoreSupplied );
}
