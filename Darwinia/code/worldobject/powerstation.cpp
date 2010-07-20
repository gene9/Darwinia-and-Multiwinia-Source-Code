#include "lib/universal_include.h"

#include <math.h>

#include "lib/debug_utils.h"
#include "lib/file_writer.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/shape.h"
#include "lib/text_stream_readers.h"

#include "app.h"
#include "location.h"

#include "worldobject/building.h"
#include "worldobject/laserfence.h"
#include "worldobject/powerstation.h"


// ****************************************************************************
// Class Powerstation
// ****************************************************************************

// *** Constructor
Powerstation::Powerstation()
:   Building(),
    m_linkedBuildingId(-1)
{
    m_type = Building::TypePowerstation;
	SetShape( g_app->m_resource->GetShape("powerstation.shp") );
}


// *** Initialise
void Powerstation::Initialise( Building *_template )
{
    Building::Initialise( _template );
	DarwiniaDebugAssert(_template->m_type == Building::TypePowerstation);
    m_linkedBuildingId = ((Powerstation *) _template)->m_linkedBuildingId;
}


// *** Advance
bool Powerstation::Advance()
{
	Building *b = g_app->m_location->GetBuilding(m_linkedBuildingId);
	if (b->m_type == Building::TypeLaserFence)
    {
        LaserFence *fence = (LaserFence *) b;
        if( !fence->IsEnabled() && GetNumPorts() == GetNumPortsOccupied() )
        {
            fence->Enable();
        }

        if( fence->IsEnabled() && GetNumPortsOccupied() <= GetNumPorts() * 3.0f/4.0f )
        {
            fence->Disable();
        }
    }

    return Building::Advance();
}


// *** Render
void Powerstation::Render( float predictionTime )
{
	Building::Render(predictionTime);
}


// *** GetBuildingLink
int Powerstation::GetBuildingLink()
{
    return m_linkedBuildingId;
}


// *** SetBuildingLink
void Powerstation::SetBuildingLink( int _buildingId )
{
    m_linkedBuildingId = _buildingId;
}


// *** Read
void Powerstation::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    char *word = _in->GetNextToken();
    m_linkedBuildingId = atoi(word);
}


// *** Write
void Powerstation::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%-8d", m_linkedBuildingId);
}
