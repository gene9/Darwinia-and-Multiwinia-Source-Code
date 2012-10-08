#include "lib/universal_include.h"

#include "lib/file_writer.h"
#include "lib/resource.h"
#include "lib/text_stream_readers.h"
#include "lib/shape.h"
#include "lib/debug_render.h"
#include "lib/language_table.h"

#include "worldobject/pixelmine.h"

#include "app.h"
#include "location.h"
#include "camera.h"
#include "sepulveda.h"


PixelMine::PixelMine()
:   Building(),
	m_nextSpawn(0.0f),
	m_spawnDelay(MINE_SPAWN_INTERVAL)
{
    m_type = TypePixelMine;
	SetShape( g_app->m_resource->GetShape( "arch.shp" ) );
    m_door = m_shape->m_rootFragment->LookupMarker( "MarkerMineEntrance" );

    DarwiniaDebugAssert( m_door );

}
void PixelMine::Initialise( Building *_template )
{
    Building::Initialise( _template );

    m_nextSpawn = ((PixelMine *) _template)->m_nextSpawn;
    m_spawnDelay = ((PixelMine *) _template)->m_spawnDelay;

}

void PixelMine::GetExitPoint( Vector3 &_pos, Vector3 &_front )
{
    Matrix34 mat( m_front, g_upVector, m_pos );
    Matrix34 dock = m_door->GetWorldMatrix( mat );
    _pos = dock.pos;
    _pos = PushFromBuilding( _pos, 5.0f );
    _front = dock.f;
}

bool PixelMine::Advance()
{
	if ( m_nextSpawn <= 0 )
	{
		// Count nearby polygons

		int numNearby = 0;

		for ( int i = 0; i < g_app->m_location->m_polygons.Size(); i++ )
		{
			if ( g_app->m_location->m_polygons.ValidIndex(i) )
			{
				LoosePolygon *polygon = g_app->m_location->m_polygons.GetPointer(i);
				DarwiniaDebugAssert(polygon);

				Vector3 dist = m_pos - polygon->m_pos;
				if ( dist.Mag() <= MINE_POLYGON_SEARCH_RADIUS ) { numNearby += 1; }
			}
		}
		if ( numNearby >= MINE_MAX_NEARBY_POLYGONS ) { m_nextSpawn = m_spawnDelay; return Building::Advance(); }

		// Spawn a new one
		Matrix34 mat( m_front, g_upVector, m_pos );
		Matrix34 door = m_door->GetWorldMatrix(mat);

		int teamId = 255;
		while ( teamId == 255 )
		{
			teamId = (int) floor(frand(NUM_TEAMS));
			if ( teamId < 0 || teamId >= NUM_TEAMS ) { teamId = 255; }
		}
        Vector3 pos = door.pos;
        float radius = syncfrand(10.0f);
        float theta = syncfrand(M_PI * 2);
        pos.x += radius * sinf(theta);
        pos.z += radius * cosf(theta);

        Vector3 vel = ( pos - m_pos );
        vel.SetLength( syncfrand(5.0f) );

		g_app->m_location->SpawnPolygon( pos, vel, teamId );

		m_nextSpawn = m_spawnDelay;
	}
	else { m_nextSpawn -= SERVER_ADVANCE_PERIOD; }

    return Building::Advance();
}

void PixelMine::Read( TextReader *_in, bool _dynamic )
{
    Building::Read( _in, _dynamic );

    if( _in->TokenAvailable() )
    {
        m_spawnDelay = atof( _in->GetNextToken() );
    }
}


void PixelMine::Write( FileWriter *_out )
{
    Building::Write( _out );

    _out->printf( "%6.2f", m_spawnDelay );

}
