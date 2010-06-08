#ifndef ENTITY_GRID_H
#define ENTITY_GRID_H

#include "globals.h"


class EntityGridCell;
class WorldObjectId;


// ****************************************************************************
//  Class EntityGrid
// ****************************************************************************

class EntityGrid
{
private:
    std::vector<WorldObjectId> m_neighbours;
	EntityGridCell      *m_cells[NUM_TEAMS];
	
    int                 m_numCellsX;
	int                 m_numCellsZ;
    double               m_cellSizeX;
    double               m_cellSizeZ;
    double               m_cellSizeXRecip; // 1/m_cellSizeX
    double               m_cellSizeZRecip;

	EntityGridCell      *GetCell(double _worldX, double _worldZ, int _team);
	EntityGridCell      *GetCell(int _indexX, int _indexZ, int _team);

    void                EnsureMaxNeighbours( std::vector<WorldObjectId> &_neighbours, int _maxNeighbours );
    
public:
	EntityGrid(double _cellSizeX, double _cellSizeZ);
	~EntityGrid();

	int GetGridIndexX   (double _worldX);
    int GetGridIndexZ   (double _worldZ);

	int GetNumCellsX	();
	int GetNumCellsZ	();

    void AddObject      (WorldObjectId _objectID, double _worldX, double _worldZ, double _radius );
    void RemoveObject   (WorldObjectId _objectID, double _worldX, double _worldZ, double _radius );
    void UpdateObject   (WorldObjectId _objectId, double _oldWorldX, double _oldWorldZ,
                                             double _newWorldX, double _newWorldZ,
                                             double _radius );

    void GetNeighbours          (std::vector<WorldObjectId> &_neighbours, double _worldX, double _worldZ, double _range, 
								 int *_numFound, bool _includeTeam[NUM_TEAMS] );

    void GetNeighbours          (std::vector<WorldObjectId> &_neighbours, double _worldX, double _worldZ, double _range, 
                                 int *_numFound);

    void GetEnemies             (std::vector<WorldObjectId> &_neighbours, double _worldX, double _worldZ, double _range,
                                 int *_numFound, unsigned char _myTeam);

	WorldObjectId GetBestEnemy  (double _worldX, double _worldZ, 
								 double _minRange, double _maxRange, unsigned char _myTeam);

    void GetFriends             (std::vector<WorldObjectId> &_neighbours, double _worldX, double _worldZ, double _range,
                                 int *_numFound, unsigned char _myTeam);

    int GetNumNeighbours        (double _worldX, double _worldZ, double _range, bool _includeTeam[NUM_TEAMS] );    // fast
    int GetNumEnemies           (double _worldX, double _worldZ, double _range, unsigned char _myTeam );           // fast
    int GetNumFriends           (double _worldX, double _worldZ, double _range, unsigned char _myTeam );           // fast
    
    bool AreNeighboursPresent   (double _worldX, double _worldZ, double _range);                                   // very fast
    bool AreNeighboursPresent   (double _worldX, double _worldZ, double _range, bool _includeTeam[NUM_TEAMS] );    // very fast
    bool AreEnemiesPresent      (double _worldX, double _worldZ, double _range, unsigned char _myTeam );           // very fast
    bool AreFriendsPresent      (double _worldX, double _worldZ, double _range, unsigned char _myTeam );           // very fast

    void Render ();
};


#endif
