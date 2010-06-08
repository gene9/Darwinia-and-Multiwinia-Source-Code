
#ifndef _included_obstructiongrid_h
#define _included_obstructiongrid_h

#include "lib/2d_array.h"
#include "lib/2d_surface_map.h"


class ObstructionGridCell
{
public:                
    LList<int> m_buildings;
};



class ObstructionGrid
{
protected:
    SurfaceMap2D <ObstructionGridCell> m_cells;
    
    void CalculateBuildingArea( int _buildingId );              // This cannot be called once on its own
                                                                // It must be called as part of a complete recalc

    void AddBuildingToCell( ObstructionGridCell *_cell, int _buildingId );
    void AddBuildingToCell( int _cellX, int _cellY, int _buildingId );

public:
    ObstructionGrid( double _cellSizeX, double _cellSizeZ );

    void CalculateAll();

    LList<int> *GetBuildings( double _locationX, double _locationZ );

    void Render();
};



#endif
