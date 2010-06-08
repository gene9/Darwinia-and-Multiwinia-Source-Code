#include "lib/universal_include.h"
#include "lib/filesys/text_file_writer.h"
#include "lib/filesys/text_stream_readers.h"
#include "lib/debug_render.h"

#include "worldobject/restrictionzone.h"

#include "app.h"
#include "location.h"
#include "location_editor.h"

// ===============================================================
// The Restriction Zone doesn't do anything itself, although it
// prevents crates from dropping inside their area
// This lets you prevent crates from falling on a players starting
// island, for example
// Additional functionality can be added later (restriction of
// powerups running, for example)
// Gary
// ================================================================

LList<int> RestrictionZone::s_restrictionZones;

RestrictionZone::RestrictionZone()
:   Building(),
    m_size(300.0f),
    m_blockPowerups(false)
{
    m_type = Building::TypeRestrictionZone;
}

RestrictionZone::~RestrictionZone()
{
    int index = s_restrictionZones.FindData( m_id.GetUniqueId() );
    if( index != -1 )
    {
        s_restrictionZones.RemoveData( index );
    }
}

void RestrictionZone::Initialise( Building *_template )
{
    Building::Initialise( _template );
    m_size = ((RestrictionZone *)_template)->m_size;
    m_blockPowerups = ((RestrictionZone *)_template)->m_blockPowerups;

    s_restrictionZones.PutData( m_id.GetUniqueId() );
}

bool RestrictionZone::Advance()
{
    return false;
}

void RestrictionZone::RenderAlphas( double _predictionTime )
{
#ifdef LOCATION_EDITOR
    if( g_app->m_editing )
    {
        Building::RenderAlphas( _predictionTime );
        RenderSphere( m_pos, 30.0 );
        if( g_app->m_editing &&
            g_app->m_locationEditor->m_mode == LocationEditor::ModeBuilding &&
            g_app->m_locationEditor->m_selectionId == m_id.GetUniqueId() )
        {
            RenderSphere( m_pos, m_size );
        }
    }

    //RenderSphere( m_pos, m_size );
#endif
}

void RestrictionZone::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );
    m_size = iv_atof( _in->GetNextToken() );
    if( _in->TokenAvailable() ) m_blockPowerups = atoi( _in->GetNextToken() );
}

void RestrictionZone::Write( TextWriter *_out )
{
    Building::Write( _out );
    _out->printf( "%-2.2f", m_size );
    _out->printf( "%5d", m_blockPowerups );
}

bool RestrictionZone::IsRestricted(Vector3 _pos, bool _powerups)
{
    Vector3 pos = _pos;
    for( int i = 0; i < s_restrictionZones.Size(); ++i )
    {
        RestrictionZone *rz = (RestrictionZone *)g_app->m_location->GetBuilding( s_restrictionZones[i] );
        if( rz && rz->m_type == Building::TypeRestrictionZone )
        {
            pos.y = rz->m_pos.y;
            if( (rz->m_pos - pos).Mag() < rz->m_size )
            {
                if( !_powerups || rz->m_blockPowerups == 1 )
                {
                    return true;
                }
            }
        }
    }

    return false;
}

bool RestrictionZone::DoesSphereHit(Vector3 const &_pos, double _radius)
{
    return false;
}


bool RestrictionZone::DoesShapeHit(Shape *_shape, Matrix34 _transform)
{
    return false;
}


bool RestrictionZone::DoesRayHit(Vector3 const &_rayStart, Vector3 const &_rayDir, 
                                double _rayLen, Vector3 *_pos, Vector3 *_norm)
{
    if( g_app->m_editing )
    {
        return Building::DoesRayHit( _rayStart, _rayDir, _rayLen, _pos, _norm );
    }
    else
    {
        return false;
    }
}