
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

public:
    ObstructionGrid( float _cellSizeX, float _cellSizeZ );

    void CalculateAll();

    LList<int> *GetBuildings( float _locationX, float _locationZ );

    void Render();
};



#endif
