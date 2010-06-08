#include "lib/universal_include.h"

#include "lib/debug_utils.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/math_utils.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/math/random_number.h"

#include "app.h"
#include "entity_grid.h"
#include "location.h"
#include "renderer.h"
#include "team.h"
#include "main.h"

#include "worldobject/bridge.h"


Bridge::Bridge()
:   Teleport(),
    m_nextBridgeId(-1),
    m_status(0.0),
    m_signal(NULL),
    m_beingOperated(false)
{
    m_type = Building::TypeBridge;
    m_sendPeriod = BRIDGE_TRANSPORTPERIOD;

    m_shapes[0] = g_app->m_resource->GetShape( "bridgeend.shp" );
    m_shapes[1] = g_app->m_resource->GetShape( "bridgetower.shp" );

    AppDebugAssert( m_shapes[0] );
    AppDebugAssert( m_shapes[1] );
    
    SetBridgeType( BridgeTypeTower );
}

void Bridge::Initialise( Building *_template )
{
    Teleport::Initialise( _template );

    m_nextBridgeId = ((Bridge *)_template)->m_nextBridgeId;    
    m_status = ((Bridge *)_template)->m_status;
    SetBridgeType( ((Bridge *)_template)->m_bridgeType );
}

void Bridge::SetBridgeType ( int _type )
{
    m_bridgeType = _type;
    SetShape( m_shapes[m_bridgeType] );

    const char signalName[] = "MarkerSignal";
    m_signal = m_shape->m_rootFragment->LookupMarker( signalName );
    AppReleaseAssert( m_signal, "Bridge: Can't get Marker(%s) from shape(%s), probably a corrupted file\n", signalName, m_shape->m_name );
}


void Bridge::Render( double predictionTime )
{
    //
    // Render our shape

    Matrix34 mat(m_front, g_upVector, m_pos);
	m_shape->Render(predictionTime, mat);
}


void Bridge::RenderAlphas ( double predictionTime )
{
    //
    // Render our signal

    if( m_nextBridgeId != -1 )
    {
        Building *building = g_app->m_location->GetBuilding( m_nextBridgeId );
        if( building && building->m_type == Building::TypeBridge )
        {
            Bridge *bridge = (Bridge *) building;
            if( m_status > 0.0 && bridge->m_status > 0.0 )
            {
                Vector3 ourPos = GetStartPoint();
                Vector3 theirPos = bridge->GetStartPoint();                
            
		        if( m_id.GetTeamId() == 255 )
		        {
			        glColor4f( 0.5, 0.5, 0.5, m_status / 100.0 );
		        }
		        else
		        {
                    RGBAColour col = g_app->m_location->m_teams[ m_id.GetTeamId() ]->m_colour;
			        glColor4ub( col.r, col.g, col.b, (unsigned char)(255.0 * m_status / 100.0) );
		        }

                glLineWidth ( 3.0 );
                glEnable    ( GL_BLEND );
                glEnable    ( GL_LINE_SMOOTH );
                glBegin( GL_LINES );
                    glVertex3dv( ourPos.GetData() );
                    glVertex3dv( theirPos.GetData() );
                glEnd();
                glDisable   ( GL_LINE_SMOOTH );
                glDisable   ( GL_BLEND );
            }
        }
    }


    Teleport::RenderAlphas(predictionTime);
}

bool Bridge::Advance()
{
    if( m_status < 0.0 ) return true;

    if( m_beingOperated )
    {
        m_status = 100.0;
    }
    else
    {
        m_status = 0.0;
    }
    
    return Teleport::Advance();
}

bool Bridge::GetAvailablePosition( Vector3 &_pos, Vector3 &_front )
{           
    Matrix34 ourMat(m_front, g_upVector, m_pos);
    Matrix34 ourEngineer = m_signal->GetWorldMatrix(ourMat);             
    _pos = ourEngineer.pos;
    _front = ourEngineer.f;

    return( !m_beingOperated );
}


void Bridge::BeginOperation()
{
    m_beingOperated = true;
}


void Bridge::EndOperation()
{
    m_beingOperated = false;
}


int Bridge::GetBuildingLink()
{
    return m_nextBridgeId;
}

void Bridge::SetBuildingLink( int _buildingId )
{
    m_nextBridgeId = _buildingId;
}

void Bridge::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    
    char *word = _in->GetNextToken();  
    m_nextBridgeId = atoi(word);    

    word = _in->GetNextToken();
    m_bridgeType = atoi(word);
    SetBridgeType( m_bridgeType );
}

void Bridge::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_nextBridgeId);    
    _out->printf( "%-8d", m_bridgeType);
}

bool Bridge::ReadyToSend ()
{
    Building *nextBridge = g_app->m_location->GetBuilding( m_nextBridgeId );
    return( m_status > 0.0 &&
            nextBridge && 
            m_bridgeType == Bridge::BridgeTypeEnd &&
            nextBridge->m_type == Building::TypeBridge &&             
            Teleport::ReadyToSend() );
}

Vector3 Bridge::GetStartPoint()
{
    Matrix34 ourMat(m_front, g_upVector, m_pos);
    Matrix34 ourSignal = m_signal->GetWorldMatrix(ourMat);             
    return ourSignal.pos;
}

Vector3 Bridge::GetEndPoint()
{
    Bridge *nextBridge = (Bridge *) this;
    while( nextBridge->m_nextBridgeId != -1 )
    {
        nextBridge = (Bridge *) g_app->m_location->GetBuilding( nextBridge->m_nextBridgeId );
    }  

    if( !nextBridge ) return g_zeroVector;
    
    Vector3 theirPos = nextBridge->GetStartPoint();

    return theirPos;
}

bool Bridge::GetExit( Vector3 &_pos, Vector3 &_front )
{
    Bridge *nextBridge = (Bridge *) this;
    while( nextBridge->m_nextBridgeId != -1 )
    {
        nextBridge = (Bridge *) g_app->m_location->GetBuilding( nextBridge->m_nextBridgeId );
    }  

    if( !nextBridge ) return false;

    Matrix34 theirMat(nextBridge->m_front, g_upVector, nextBridge->m_pos);
    Matrix34 theirEntrance = nextBridge->m_entrance->GetWorldMatrix(theirMat);

    _pos = theirEntrance.pos;
    _front = theirEntrance.f;

    return true;
}

bool Bridge::UpdateEntityInTransit( Entity *_entity )
{
    Building *building = g_app->m_location->GetBuilding( m_nextBridgeId );    
    Bridge *nextBridge = (Bridge *) building;

    WorldObjectId id( _entity->m_id );
    
    if( m_status > 0.0 &&
        nextBridge && 
        nextBridge->m_type == Building::TypeBridge &&
        nextBridge->m_status > 0.0 )
    {
        Matrix34 theirMat(nextBridge->m_front, g_upVector, nextBridge->m_pos);
        Matrix34 theirSignal = nextBridge->m_signal->GetWorldMatrix(theirMat);
        Vector3 offset = (theirSignal.pos - _entity->m_pos).Normalise();
        double dist = ( _entity->m_pos - theirSignal.pos ).Mag();
        bool arrived = false;

        _entity->m_vel = offset * BRIDGE_TRANSPORTSPEED;
        if( _entity->m_vel.Mag() * SERVER_ADVANCE_PERIOD > dist )
        {
            _entity->m_vel = ( _entity->m_pos - theirSignal.pos ) / SERVER_ADVANCE_PERIOD;
            arrived = true;
        }

        _entity->m_pos += _entity->m_vel * SERVER_ADVANCE_PERIOD;
        _entity->m_onGround = false;
        _entity->m_enabled = false;

        
        if( arrived )
        {
            // We are there
            if( nextBridge->m_bridgeType == Bridge::BridgeTypeEnd )
            {
                Vector3 exitPos, exitFront;
                nextBridge->GetExit( exitPos, exitFront );
                _entity->m_pos = exitPos;
                _entity->m_front = exitFront;
                _entity->m_enabled = true;
                _entity->m_onGround = true;
                _entity->m_vel.Zero();

                g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );
                return true;                
            }
            else if( nextBridge->m_bridgeType == Bridge::BridgeTypeTower )
            {
                nextBridge->EnterTeleport( id, true );
                return true;
            }
        }
                
        return false;
    }
    else
    {
        // Shit - we lost the carrier signal, so we die
        _entity->ChangeHealth( -500 );
        _entity->m_enabled = true;        
        _entity->m_vel = Vector3(SFRAND(10.0), FRAND(10.0), SFRAND(10.0) );

        g_app->m_location->m_entityGrid->AddObject( id, _entity->m_pos.x, _entity->m_pos.z, _entity->m_radius );
        return true;
    }
}
