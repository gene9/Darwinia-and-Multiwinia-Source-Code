#include "lib/universal_include.h"
#include "lib/hi_res_time.h"
#include "lib/debug_render.h"
#include "lib/debug_utils.h"
#include "lib/vector2.h"

#include "app.h"
#include "location.h"
#include "obstruction_grid.h"

#include "worldobject/laserfence.h"



ObstructionGrid::ObstructionGrid( double _cellSizeX, double _cellSizeZ )
{
    double sizeX = g_app->m_location->m_landscape.GetWorldSizeX();
    double sizeZ = g_app->m_location->m_landscape.GetWorldSizeZ();

    ObstructionGridCell outSideValue;
    m_cells.Initialise( sizeX, sizeZ, 0.0, 0.0, _cellSizeX, _cellSizeZ, outSideValue);

    CalculateAll();
}

void ObstructionGrid::AddBuildingToCell( int _cellX, int _cellY, int _buildingId )
{
    ObstructionGridCell *cell = m_cells.GetPointer( _cellX, _cellY );
    if( cell )
    {
        AddBuildingToCell( cell, _buildingId );
    }
}

void ObstructionGrid::AddBuildingToCell(ObstructionGridCell *_cell, int _buildingId)
{
    bool found = false;
    Building *building = g_app->m_location->GetBuilding( _buildingId );
    if( building )
    {
	    for( int j = 0; j < _cell->m_buildings.Size(); ++j )
	    {
		    if( _cell->m_buildings[j] == building->m_id.GetUniqueId() )
		    {
			    found = true;
		    }
	    }

	    if( !found )
	    {
		    _cell->m_buildings.PutData( _buildingId );
	    }
    }
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
                double cellCentreX = (double) x * m_cells.m_cellSizeX + m_cells.m_cellSizeX/2.0;
                double cellCentreZ = (double) z * m_cells.m_cellSizeY + m_cells.m_cellSizeY/2.0;                
                double cellRadius = m_cells.m_cellSizeX*0.5;

                Vector3 cellPos( cellCentreX, 0.0, cellCentreZ );
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
				//if( fence->IsEnabled() )
				{
					Building *link = g_app->m_location->GetBuilding( building->GetBuildingLink() );

					Vector3 direction = ( link->m_pos - building->m_pos );

					int numCells = ( link->m_pos - building->m_pos ).Mag() + 1;

                    int oldCellX = -1;
                    int oldCellY = -1;

					for( int i = 0; i < numCells; ++i )
					{
						Vector3 pos = building->m_pos + (direction / numCells) * i;
						int cellX = m_cells.GetMapIndexX( pos.x );
						int cellY = m_cells.GetMapIndexY( pos.z );

						ObstructionGridCell *cell = m_cells.GetPointer( cellX, cellY );
                        AddBuildingToCell( cell, _buildingId );

                        if( cellX + 1 != oldCellX && 
                            cellX - 1 != oldCellX && 
                            cellX     != oldCellX &&
                            cellY + 1 != oldCellY &&
                            cellY - 1 != oldCellY &&
                            cellY     != oldCellY )
                        {
                            // due to distance calculations, we have skipped diagonally over a cell
                            // this could mean we are just skimming the corner of that cell, or
                            // we've some how skipped over a large chunk of the corner and landed on the edge of 
                            // the adjacent cells
                            // this can cause gaps in the grid which let darwinians through the laser fences, 
                            // so we have to fill all the surrounding cells now to ensure this gap is filled

                            AddBuildingToCell( cellX + 1, cellY, _buildingId );
                            AddBuildingToCell( cellX - 1, cellY, _buildingId );
                            AddBuildingToCell( cellX, cellY + 1, _buildingId );
                            AddBuildingToCell( cellX, cellY - 1, _buildingId );

                            /*AddBuildingToCell( oldCellX + 1, oldCellY, _buildingId );
                            AddBuildingToCell( oldCellX - 1, oldCellY, _buildingId );
                            AddBuildingToCell( oldCellX, oldCellY + 1, _buildingId );
                            AddBuildingToCell( oldCellX, oldCellY - 1, _buildingId );*/
                        }

                        int oldCellX = cellX;
                        int oldCellY = cellY;
					}
				}
			}
		}
    }
}


void ObstructionGrid::CalculateAll()
{
    double startTime = GetHighResTime();
    
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

    double totalTime = GetHighResTime() - startTime;
    AppDebugOut( "ObstructionGrid took %dms to generate\n", int(totalTime * 1000) );
}


LList<int> *ObstructionGrid::GetBuildings( double _locationX, double _locationZ )
{
    return (LList<int> *) &m_cells.GetValueNearest( _locationX, _locationZ ).m_buildings;
}


void ObstructionGrid::Render()
{
    glLineWidth (2.0);
    glDisable   ( GL_CULL_FACE );
    glEnable    ( GL_BLEND );
    glDepthMask ( false );

    double height = 150.0;

	glBegin( GL_QUADS );

	for( int x = 0; x < m_cells.GetNumColumns(); ++x )
    {
        for( int z = 0; z < m_cells.GetNumRows(); ++z )
        {
            double worldX = (double) x * m_cells.m_cellSizeX;
            double worldZ = (double) z * m_cells.m_cellSizeY;
            double w = m_cells.m_cellSizeX;
            double h = m_cells.m_cellSizeY;

            int numBuildings = GetBuildings( worldX, worldZ )->Size();
            glColor4f( 1.0, 1.0, 1.0, numBuildings/3.0 );

            glVertex3f( worldX, height, worldZ );
            glVertex3f( worldX + w, height, worldZ );
            glVertex3f( worldX + w, height, worldZ + h );
            glVertex3f( worldX, height, worldZ + h );
        }
    }

	glEnd();

    glDepthMask ( true );
    glDisable   ( GL_BLEND );
    glEnable    ( GL_CULL_FACE );
}

