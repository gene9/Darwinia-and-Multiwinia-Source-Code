#include "lib/universal_include.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/resource.h"
#include "lib/math/random_number.h"

#include "worldobject/chess.h"
#include "worldobject/carryablebuilding.h"

#include "app.h"
#include "location.h"
#include "team.h"
#include "global_world.h"
#include "multiwinia.h"
#include "explosion.h"
#include "gametimer.h"



ChessBase::ChessBase()
:   Building(),
    m_width(100.0),
    m_depth(30.0)    
{
    m_sparePoints = g_app->m_multiwinia->GetGameOption("TotalSize");
    m_timer = 1.0;
    
    m_type = TypeChessBase;
}


void ChessBase::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_width = ((ChessBase *)_template)->m_width;
    m_depth = ((ChessBase *)_template)->m_depth;
}


bool ChessBase::Advance()
{
    m_timer -= SERVER_ADVANCE_PERIOD;
    if( m_timer <= 0.0 )
    {
        m_timer = 1.0;

        //
        // If we have spare points, spawn some new pieces until they are used up

        if( m_sparePoints > 0.3 )
        {
            double scaleVariance = g_app->m_multiwinia->GetGameOption("SizeVariance");
            scaleVariance /= 100.0;
            double thisScale = 1.0 + syncsfrand(scaleVariance);
            if( thisScale > m_sparePoints ) thisScale = m_sparePoints;
            if( thisScale < 0.1 ) thisScale = 0.1;
            m_sparePoints -= thisScale;

            SpawnPiece( thisScale );
        }


        //
        // Are there any enemy chess pieces in the zone?
        
        Vector3 right = m_front ^ m_up;

        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                ChessPiece *chessPiece = (ChessPiece *) g_app->m_location->m_buildings[i];
                if( chessPiece->m_type == TypeChessPiece &&
                    !g_app->m_location->IsFriend( chessPiece->m_id.GetTeamId(), m_id.GetTeamId() ) )
                {
                    Vector3 chessPiecePos = chessPiece->m_pos - chessPiece->m_front * 35 * chessPiece->m_scale;
                    bool intersection = SphereTriangleIntersection( chessPiecePos, 35*chessPiece->m_scale,
                                                                    m_pos - right * m_width * 0.5 - m_front * m_depth * 0.5,
                                                                    m_pos - right * m_width * 0.5 + m_front * m_depth * 0.5,
                                                                    m_pos + right * m_width * 0.5 + m_front * m_depth * 0.5 );
                    if( !intersection )
                    {
                         intersection = SphereTriangleIntersection( chessPiecePos, 35*chessPiece->m_scale,
                                                                    m_pos + right * m_width * 0.5 + m_front * m_depth * 0.5,
                                                                    m_pos + right * m_width * 0.5 - m_front * m_depth * 0.5,
                                                                    m_pos - right * m_width * 0.5 - m_front * m_depth * 0.5 );
                    }

                    if( intersection )
                    {
                        chessPiece->HandleSuccess();
                    }
                }
            }
        }
    }

    return Building::Advance();
}


void ChessBase::SpawnPiece( double _scale )
{
    Vector3 right = m_front ^ m_up;
    Vector3 spawnPos = m_pos;
    spawnPos += right * m_width * syncsfrand(1);
    spawnPos += m_front * m_depth * syncsfrand(1);

    double pieceSize = 50;

    //
    // Push from other chess pieces
    
    while( true )
    {
        bool hitSomething = false;
        for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
        {
            if( g_app->m_location->m_buildings.ValidIndex(i) )
            {
                CarryableBuilding *carryable = (CarryableBuilding *) g_app->m_location->m_buildings[i];
                if( carryable->m_type == TypeChessPiece )
                {
                    double distance = ( carryable->m_pos - spawnPos ).Mag();
                    if( distance < pieceSize * _scale + pieceSize * carryable->m_scale )
                    {
                        spawnPos += ( spawnPos - carryable->m_pos ).SetLength( 10 );
                        hitSomething = true;
                    }
                }
            }
        }
        if( !hitSomething ) break;
    }

    ChessPiece carryTemplate;    
    carryTemplate.m_pos = spawnPos;
    carryTemplate.m_front = m_front;

    ChessPiece *building = (ChessPiece *) Building::CreateBuilding( Building::TypeChessPiece );
    g_app->m_location->m_buildings.PutData( building );
    building->m_scale = _scale;
    building->m_chessBaseId = m_id.GetUniqueId();
    building->Initialise((Building *)&carryTemplate);
    int id = g_app->m_globalWorld->GenerateBuildingId();
    building->m_id.SetTeamId( m_id.GetTeamId() );
    building->m_id.SetUnitId( UNIT_BUILDINGS );
    building->m_id.SetUniqueId( id ); 
}


void ChessBase::Render( double _precictionTime )
{
    Building::Render( _precictionTime );
    
    Vector3 right = m_front ^ m_up;

    Vector3 pos1 = m_pos - right * m_width * 0.5 - m_front * m_depth * 0.5;
    Vector3 pos2 = m_pos - right * m_width * 0.5 + m_front * m_depth * 0.5;
    Vector3 pos3 = m_pos + right * m_width * 0.5 + m_front * m_depth * 0.5;
    Vector3 pos4 = m_pos + right * m_width * 0.5 - m_front * m_depth * 0.5;

    pos1.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos1.x, pos1.z) + 5.0;
    pos2.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos2.x, pos2.z) + 5.0;
    pos3.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos3.x, pos3.z) + 5.0;
    pos4.y = g_app->m_location->m_landscape.m_heightMap->GetValue(pos4.x, pos4.z) + 5.0;
    
    RGBAColour col( 128, 128, 128, 255 );
    if( m_id.GetTeamId() != 255 )
    {
        col = g_app->m_location->m_teams[m_id.GetTeamId()]->m_colour;
    }
    glColor4ubv( col.GetData() );

    glDisable( GL_LIGHTING );
    glBegin( GL_LINE_LOOP );
        glVertex3dv( pos1.GetData() );
        glVertex3dv( pos2.GetData() );
        glVertex3dv( pos3.GetData() );
        glVertex3dv( pos4.GetData() );
    glEnd();
    glEnable( GL_LIGHTING );
}


void ChessBase::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    m_width = atoi( _in->GetNextToken() );
    m_depth = atoi( _in->GetNextToken() );
}


void ChessBase::Write( TextWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%2.2 %2.2", m_width, m_depth );
}


// ============================================================================

ChessPiece::ChessPiece()
:   CarryableBuilding(),
    m_damage(0.0)
{
    m_type = TypeChessPiece;
    SetShape( g_app->m_resource->GetShape("chesspiece.shp") );
}


void ChessPiece::Initialise( Building *_template )
{
    CarryableBuilding::Initialise( _template );

    ExplodeShape( 0.5 );

    m_waypoint = m_pos;
}


bool ChessPiece::Advance()
{
    if( m_scale < 0.1 ) return true;

    return CarryableBuilding::Advance();
}


void ChessPiece::ExplodeShape( double _fraction )
{
    Matrix34 transform( m_front, m_up, m_pos );
    transform.f *= m_scale;
    transform.u *= m_scale;
    transform.r *= m_scale;
    
    g_explosionManager.AddExplosion( m_shape, transform, _fraction );
}


void ChessPiece::SetWaypoint( Vector3 const &_waypoint )
{
    Vector3 travelDir = ( _waypoint - m_pos ).Normalise();
    Vector3 right = m_front ^ m_up;
    if( (travelDir ^ right).y < 0.0 )
    {
        m_waypoint = _waypoint;
    }
    else
    {
        m_waypoint = m_pos;
    }
}


void ChessPiece::HandleCollision( double _force )
{
    m_damage += _force * SERVER_ADVANCE_PERIOD;
    
    if( m_damage >= 2.0 )
    {
        m_scale -= 0.05;
        m_damage = 0.0;

        ExplodeShape( 0.3 );
    }
}


void ChessPiece::HandleSuccess()
{
    //
    // Spawn a replacement piece

    ChessBase *base = (ChessBase *) g_app->m_location->GetBuilding( m_chessBaseId );
    if( base && base->m_type == TypeChessBase )
    {
        base->SpawnPiece( m_scale );
    }


    //
    // Score points

    g_app->m_multiwinia->AwardPoints( m_id.GetTeamId(), int( 10 * m_scale ) );
    
    
    //
    // Ensure we die

    ExplodeShape( 0.5 );

    m_scale = 0.01;
}
