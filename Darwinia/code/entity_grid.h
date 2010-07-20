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
    WorldObjectId       *m_neighbours;
    int                 m_maxNeighbours;

	EntityGridCell      *m_cells[NUM_TEAMS];

    int                 m_numCellsX;
	int                 m_numCellsZ;
    float               m_cellSizeX;
    float               m_cellSizeZ;
    float               m_cellSizeXRecip; // 1/m_cellSizeX
    float               m_cellSizeZRecip;

	EntityGridCell      *GetCell(float _worldX, float _worldZ, int _team);
	EntityGridCell      *GetCell(int _indexX, int _indexZ, int _team);

    void                EnsureMaxNeighbours( int _maxNeighbours );

public:
	EntityGrid(float _cellSizeX, float _cellSizeZ);
	~EntityGrid();

	int GetGridIndexX   (float _worldX);
    int GetGridIndexZ   (float _worldZ);

    void AddObject      (WorldObjectId _objectID, float _worldX, float _worldZ, float _radius );
    void RemoveObject   (WorldObjectId _objectID, float _worldX, float _worldZ, float _radius );
    void UpdateObject   (WorldObjectId _objectId, float _oldWorldX, float _oldWorldZ,
                                             float _newWorldX, float _newWorldZ,
                                             float _radius );

    WorldObjectId *GetNeighbours(float _worldX, float _worldZ, float _range,
								 int *_numFound, bool _includeTeam[NUM_TEAMS] );

    WorldObjectId *GetNeighbours(float _worldX, float _worldZ, float _range,
                                 int *_numFound);

    WorldObjectId *GetEnemies   (float _worldX, float _worldZ, float _range,
                                 int *_numFound, unsigned char _myTeam);

	WorldObjectId GetBestEnemy  (float _worldX, float _worldZ,
								 float _minRange, float _maxRange, unsigned char _myTeam);

    WorldObjectId *GetFriends   (float _worldX, float _worldZ, float _range,
                                 int *_numFound, unsigned char _myTeam);

    int GetNumNeighbours        (float _worldX, float _worldZ, float _range, bool _includeTeam[NUM_TEAMS] );    // fast
    int GetNumEnemies           (float _worldX, float _worldZ, float _range, unsigned char _myTeam );           // fast
    int GetNumFriends           (float _worldX, float _worldZ, float _range, unsigned char _myTeam );           // fast

    bool AreNeighboursPresent   (float _worldX, float _worldZ, float _range, bool _includeTeam[NUM_TEAMS] );    // very fast
    bool AreEnemiesPresent      (float _worldX, float _worldZ, float _range, unsigned char _myTeam );           // very fast
    bool AreFriendsPresent      (float _worldX, float _worldZ, float _range, unsigned char _myTeam );           // very fast

    void Render ();
};


#endif
