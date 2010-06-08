#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/vector2.h"

#include "app.h"
#include "location.h"
#include "obstruction_grid.h"

#include "worldobject/laserfence.h"



ObstructionGrid::ObstructionGrid( float _cellSizeX, float _cellSizeZ )
{
    float sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    float sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

    ObstructionGridCell outSideValue;
    m_cells.Initialise( sizeX, sizeZ, 0.0f, 0.0f, _cellSizeX, _cellSizeZ, outSideValue);

    CalculateAll();
}


void ObstructionGrid::CalculateBuildingArea( int _buildingId )
{    
    Building *building = g_app->m_location->GetBuilding( _buildingId );
    if( building )
    {
        int buildingCellX = m_cells.GetMapIndexX( building->m_centrePos.x );
        int buildingCellZ = m_cells.GetMapIndexY( building->m_centrePos.z );
        int range = ceil( building->m_radius / m_cells.m_cellSizeX );

        int minX = max( buildingCellX - range, 0 );
        int minZ = max( buildingCellZ - range, 0 );
        int maxX = min( buildingCellX + range, (int) m_cells.GetNumRows() );
        int maxZ = min( buildingCellZ + range, (int) m_cells.GetNumColumns() );

        for( int x = minX; x <= maxX; ++x )
        {
            for( int z = minZ; z <= maxZ; ++z )
            {
                ObstructionGridCell *cell = m_cells.GetPointer(x,z);
                float cellCentreX = (float) x * m_cells.m_cellSizeX + m_cells.m_cellSizeX/2.0f;
                float cellCentreZ = (float) z * m_cells.m_cellSizeY + m_cells.m_cellSizeY/2.0f;                
                float cellRadius = m_cells.m_cellSizeX*0.5f;

                Vector3 cellPos( cellCentreX, 0.0f, cellCentreZ );
                cellPos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(cellPos.x, cellPos.z);

                if( building->DoesSphereHit( cellPos, cellRadius ) )
                {
                    cell->m_buildings.PutData( _buildingId );    
                }
            }
        }

		if( building->m_type == Building::TypeLaserFence )
		{
			if( building->GetBuildingLink() != -1 )
			{
				LaserFence *fence = (LaserFence *)building;
				if( fence->IsEnabled() )
				{
					Building *link = g_app->m_location->GetBuilding( building->GetBuildingLink() );

					Vector3 direction = ( link->m_pos - building->m_pos );

					int numCells = ( link->m_pos - building->m_pos ).Mag() + 1;

					for( int i = 0; i < numCells; ++i )
					{
						Vector3 pos = building->m_pos + (direction / numCells) * i;
						int cellX = m_cells.GetMapIndexX( pos.x );
						int cellY = m_cells.GetMapIndexY( pos.z );

						ObstructionGridCell *cell = m_cells.GetPointer( cellX, cellY );

						bool found = false;
						for( int j = 0; j < cell->m_buildings.Size(); ++j )
						{
							if( cell->m_buildings[j] == building->m_id.GetUniqueId() )
							{
								found = true;
							}
						}

						if( !found )
						{
							cell->m_buildings.PutData( _buildingId );
						}
					}
				}
			}
		}
    }
}


void ObstructionGrid::CalculateAll()
{
    float startTime = GetHighResTime();
    
    //
    // Clear the obstruction grid

    for( int x = 0; x < m_cells.GetNumColumns(); ++x )
    {
        for( int z = 0; z < m_cells.GetNumRows(); ++z )
        {
            ObstructionGridCell *cell = m_cells.GetPointer(x,z);
            cell->m_buildings.Empty();
        }
    }
    
    //
    // Add each building to the grid

    for( int i = 0; i < g_app->m_location->m_buildings.Size(); ++i )
    {
        if( g_app->m_location->m_buildings.ValidIndex(i) )
        {
            Building *building = g_app->m_location->m_buildings[i];
            CalculateBuildingArea( building->m_id.GetUniqueId() );
        }
    }

    float totalTime = GetHighResTime() - startTime;
    DebugOut( "ObstructionGrid took %dms to generate\n", int(totalTime * 1000) );
}


LList<int> *ObstructionGrid::GetBuildings( float _locationX, float _locationZ )
{
    return (LList<int> *) &m_cells.GetValueNearest( _locationX, _locationZ ).m_buildings;
}


void ObstructionGrid::Render()
{
    glLineWidth (2.0f);
    glDisable   ( GL_CULL_FACE );
    glEnable    ( GL_BLEND );
    glDepthMask ( false );

    float height = 150.0f;

    for( int x = 0; x < m_cells.GetNumColumns(); ++x )
    {
        for( int z = 0; z < m_cells.GetNumRows(); ++z )
        {
            float worldX = (float) x * m_cells.m_cellSizeX;
            float worldZ = (float) z * m_cells.m_cellSizeY;
            float w = m_cells.m_cellSizeX;
            float h = m_cells.m_cellSizeY;

            int numBuildings = GetBuildings( worldX, worldZ )->Size();
            glColor4f( 1.0f, 1.0f, 1.0f, numBuildings/3.0f );

            glBegin( GL_QUADS );
                glVertex3f( worldX, height, worldZ );
                glVertex3f( worldX + w, height, worldZ );
                glVertex3f( worldX + w, height, worldZ + h );
                glVertex3f( worldX, height, worldZ + h );
            glEnd();
        }
    }

    glDepthMask ( true );
    glDisable   ( GL_BLEND );
    glEnable    ( GL_CULL_FACE );
}

