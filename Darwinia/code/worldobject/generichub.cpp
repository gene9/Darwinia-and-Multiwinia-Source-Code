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
#include "team.h"

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
// Class DynamicConstruction
// ****************************************************************************

DynamicConstruction::DynamicConstruction()
:   DynamicBase(),
    m_finished(false),
    m_progress(0.0f),
	m_perTick(1.0f),
	m_linkId(-1)
{
    m_type = TypeDynamicConstruction;
	m_minimumPorts = 1;
    //SetShape( g_app->m_resource->GetShape( "solarpanel.shp" ) );
}

void DynamicConstruction::Initialise( Building *_template )
{
    DynamicBase::Initialise( _template );
    DynamicConstruction *nodeCopy = (DynamicConstruction *) _template;
    m_progress = nodeCopy->m_progress;
	m_perTick = nodeCopy->m_perTick;
	m_linkId = nodeCopy->GetBuildingLink();
}

bool DynamicConstruction::Advance()
{

	RecalculateOwnership();

	// This is the only place where a buildings m_construction can be set to true, as it is never saved and defaults to false
	// We unset it later if the building is finished
	if ( !m_finished ) {
		if ( g_app->m_location->m_buildings.ValidIndex(m_linkId) )
		{
			Building *building = g_app->m_location->m_buildings.GetData(m_linkId);
			DarwiniaDebugAssert( building );

			building->m_underConstruction = true;

		}
	}

	if ( !m_finished && m_progress >= 100.0f )
	{
		m_finished = true;
		m_ports.Empty();	// Remove the darwinian pots

		// Turn us on!
        GlobalBuilding *gb = g_app->m_globalWorld->GetBuilding( m_id.GetUniqueId(), g_app->m_locationId );
        if( gb && !gb->m_online )
        {
            gb->m_online = true;
            g_app->m_globalWorld->EvaluateEvents();
        }
		
		if ( m_linkId && g_app->m_location->m_buildings.ValidIndex(m_linkId) )
		{
			Building *building = g_app->m_location->m_buildings.GetData(m_linkId);
			DarwiniaDebugAssert( building );

			if ( g_app->m_location->IsFriend(m_id.GetTeamId(), 2) ) {
				building->m_id.SetTeamId(2);	// A friendly team activated this, so make it a player controlled building
			} else {
				building->m_id.SetTeamId(m_id.GetTeamId());
			}
			building->m_underConstruction = false;
			building->ReprogramComplete();
		}
	}
    return DynamicBase::Advance();
}


void DynamicConstruction::RenderPorts()
{
	// Stolen from ControlStation
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


void DynamicConstruction::Render( float _predictionTime )
{
	/*
    if( g_app->m_editing )
    {
        m_up = g_app->m_location->m_landscape.m_normalMap->GetValue( m_pos.x, m_pos.z );
        Vector3 right( 1, 0, 0 );
        m_front = right ^ m_up;
    }
*/
#ifdef DEBUG_RENDER_ENABLED
	if (g_app->m_editing)
	{
		Vector3 pos(m_pos);
		pos.y += 5.0f;
		RenderArrow(pos, pos + m_front * 20.0f, 4.0f);
	}
#endif
	
	glShadeModel( GL_SMOOTH );
    if( m_shape )
    {
		Matrix34 mat(m_front, m_up, m_pos);
		// If we are done or we have a link, render full
		if ( m_finished || g_app->m_location->m_buildings.ValidIndex(m_linkId) ) {
			m_shape->Render(_predictionTime, mat);
		} else {
			// We have no link and are not finished, so we must be self-assembling, render special instead
			m_shape->RenderSpecial(_predictionTime, mat, m_progress/100.0f);
		}
    }
    else
    {
        RenderSphere( m_pos, 40.0f );
    }

	// If we aren't finished then we are responsible for rendering our linked building
	if ( m_progress < 100.0f && m_linkId && g_app->m_location->m_buildings.ValidIndex(m_linkId) )
	{
		Building *building = g_app->m_location->m_buildings.GetData(m_linkId);
		DarwiniaDebugAssert( building );

		Matrix34 mat2(building->m_front, building->m_up, building->m_pos);
		if ( building->m_shape ) { building->m_shape->RenderSpecial( _predictionTime, mat2, m_progress/100.0f ); }
	}
    //DynamicBase::Render( _predictionTime );
    glShadeModel( GL_FLAT );
}


void DynamicConstruction::RenderAlphas( float _predictionTime )
{
    DynamicBase::RenderAlphas( _predictionTime );
}


void DynamicConstruction::ListSoundEvents( LList<char *> *_list )
{
    DynamicBase::ListSoundEvents( _list );

    _list->PutData( "Operate" );
}

void DynamicConstruction::ReprogramComplete()
{
}

void DynamicConstruction::Read( TextReader *_in, bool _dynamic )
{
    DynamicBase::Read( _in, _dynamic );
    m_progress = atof( _in->GetNextToken() );
	if ( _in->TokenAvailable() ) m_perTick = atof( _in->GetNextToken() );
	if ( _in->TokenAvailable() ) m_linkId = atoi( _in->GetNextToken() );
}

void DynamicConstruction::Write( FileWriter *_out )
{
    DynamicBase::Write( _out );
    _out->printf( "%6.2f", m_progress );
	_out->printf( "%6.2f", m_perTick );
	_out->printf( "%6d", m_linkId );
}

int DynamicConstruction::GetBuildingLink()
{
	return m_linkId;
}

void DynamicConstruction::SetBuildingLink( int _buildingId )
{
	m_linkId = _buildingId;
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