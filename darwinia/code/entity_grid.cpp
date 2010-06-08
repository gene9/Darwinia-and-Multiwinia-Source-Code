#include "lib/universal_include.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/text_renderer.h"
#include "lib/hi_res_time.h"

#include "worldobject/entity.h"

#include "app.h"
#include "entity_grid.h"
#include "location.h"
#include "team.h"


#define END_OF_LIST -100000

//#define DEBUG_ENTITY_GRID

#ifdef DEBUG_ENTITY_GRID
struct EntityGridError
{
    Vector3 m_pos;
    WorldObjectId m_id;
    int m_errorCode;
};
static LList<EntityGridError *> s_entityGridErrors;
#endif


// ****************************************************************************
//  Class EntityGridCell
// ****************************************************************************

#define INITIAL_OBJECT_ID_ARRAY_SIZE	0

unsigned int g_counter = 0;

class EntityGridCell
{
public:
	int m_numSlotsFree;
	int m_firstFree;

    void OutputContents();

public:
	WorldObjectId *m_objectIds;
    int *m_usageLists;
    int m_arraySize;

	EntityGridCell();
	~EntityGridCell();
	void AddObjectId(WorldObjectId _objectId);
	bool RemoveObjectId(WorldObjectId _objectId);
};


// *** Constructor
EntityGridCell::EntityGridCell()
:	m_arraySize(INITIAL_OBJECT_ID_ARRAY_SIZE),
	m_numSlotsFree(INITIAL_OBJECT_ID_ARRAY_SIZE),
	m_firstFree(END_OF_LIST)
{
	m_objectIds = NULL;
    m_usageLists = NULL;
}


// *** Destructor
EntityGridCell::~EntityGridCell()
{
	delete [] m_objectIds;
    delete [] m_usageLists;
}


// *** OutputContents
void EntityGridCell::OutputContents()
{
    char buffer[256];
    sprintf(buffer, "Grid Cell: ");
    int i;
    for (i = 0; i < m_arraySize; i++)
    {
        char buf2[128];
        sprintf(buf2, "%d ", m_usageLists[i]);
        strcat(buffer, buf2);
    }
    strcat(buffer, "\n");
    DebugOut(buffer);
}


// *** AddObjectId
void EntityGridCell::AddObjectId(WorldObjectId _objectId)
{
//    OutputContents();

	if (m_firstFree == END_OF_LIST)
	{
        DarwiniaDebugAssert( m_numSlotsFree == 0 );

		int newArraySize = m_arraySize * 2;
		if (m_arraySize == 0)
		{
			newArraySize = 2;
			m_numSlotsFree = 2;
		}
		else
		{
			m_numSlotsFree = m_arraySize;
		}
		WorldObjectId *newObjectIds = new WorldObjectId[newArraySize];
        int *newUsageLists = new int[newArraySize];

		// Copy data from old array into first half of new array
		for (int i = 0; i < m_arraySize; ++i)
		{
			newObjectIds[i] = m_objectIds[i];
            newUsageLists[i] = m_usageLists[i];
		}

		// Fill the second half of the new usage array with free list entries
		for (int i = m_arraySize; i < newArraySize - 1; ++i)
		{
			newUsageLists[i] = i + 1;
		}
        newUsageLists[newArraySize - 1] = END_OF_LIST;

		// Throw away old arrays and update everything to reference the new one
		delete [] m_objectIds;
        delete [] m_usageLists;
		m_firstFree = m_arraySize;
		m_objectIds = newObjectIds;
        m_usageLists = newUsageLists;
		m_arraySize = newArraySize;
	}

	int target = m_firstFree;
	
    DarwiniaDebugAssert( target >= 0 );
    DarwiniaDebugAssert( target < m_arraySize);

    m_firstFree = m_usageLists[target];

    m_objectIds[target] = _objectId;
    m_usageLists[target] = END_OF_LIST;
	m_numSlotsFree--;
}


// *** RemoveObject
bool EntityGridCell::RemoveObjectId(WorldObjectId _objectId)
{
	// Search for the slot containing the specified ID
    bool found = false;

	for (int i = 0; i < m_arraySize; i++)
	{
        if (m_objectIds[i] == _objectId)
		{
			m_objectIds[i].SetInvalid();
            m_usageLists[i] = m_firstFree;
			m_firstFree = i;
			m_numSlotsFree++;
            found = true;
		}
	}

    return found;
}


// ****************************************************************************
//  Class EntityGrid
// ****************************************************************************


void LogEntityGridError( WorldObjectId _id, Vector3 const &_pos, int _error )
{
#ifdef DEBUG_ENTITY_GRID
    for( int i = 0; i < s_entityGridErrors.Size(); ++i )
    {
        EntityGridError *theError = s_entityGridErrors[i];
        if( theError->m_id == _id ) 
        {
            // The error already exists
            return;
        }
    }
    EntityGridError *theError = new EntityGridError();
    theError->m_id = _id;
    theError->m_pos = _pos;
    theError->m_errorCode = _error;
    s_entityGridErrors.PutData( theError );
#else
    DarwiniaDebugAssert( false );            
#endif
}


// *** Constructor
EntityGrid::EntityGrid(float _cellSizeX, float _cellSizeZ)
:   m_cellSizeX(_cellSizeX),
    m_cellSizeZ(_cellSizeZ),
    m_neighbours(NULL),
    m_maxNeighbours(0)
{
    m_cellSizeXRecip = 1.0f / _cellSizeX;
    m_cellSizeZRecip = 1.0f / _cellSizeZ;

    m_numCellsX = int(g_app->m_location->m_landscape.GetWorldSizeX() / _cellSizeX) + 1;
    m_numCellsZ = int(g_app->m_location->m_landscape.GetWorldSizeZ() / _cellSizeZ) + 1;

//	SetNewReportingThreshold(13);
    EnsureMaxNeighbours( 100 );

	for( int t = 0; t < NUM_TEAMS; ++t )
    {
        m_cells[t] = new EntityGridCell[m_numCellsX * m_numCellsZ];
    }
    
//	SetNewReportingThreshold(-1);
}


void EntityGrid::EnsureMaxNeighbours( int _maxNeighbours )
{
    if( _maxNeighbours > m_maxNeighbours )
    {
        float startTime = GetHighResTime();

        int newMaxNeighbours = m_maxNeighbours + 100;
        WorldObjectId *newNeighbours = new WorldObjectId[ newMaxNeighbours ];
        
        if( m_neighbours )
        {
            memcpy( newNeighbours, m_neighbours, m_maxNeighbours * sizeof(WorldObjectId) );
            delete m_neighbours;
        }

        m_neighbours = newNeighbours;
        m_maxNeighbours = newMaxNeighbours;

        float time = GetHighResTime() - startTime;
        DebugOut( "EntityGrid max neighbours set to %d (time taken %2.2fms)\n", m_maxNeighbours, time*1000.0f );
    }
}


// *** Destructor
EntityGrid::~EntityGrid()
{
    for( int t = 0; t < NUM_TEAMS; ++t )
    {
	    delete [] m_cells[t];
    }
}


// *** GetGridIndexX
int EntityGrid::GetGridIndexX(float _worldX)
{
    return (int) (_worldX * m_cellSizeXRecip);
}


// *** GetGridIndexZ
int EntityGrid::GetGridIndexZ(float _worldZ)
{
    return (int) (_worldZ * m_cellSizeZRecip);
}


// *** GetCell
EntityGridCell *EntityGrid::GetCell(float _worldX, float _worldZ, int _team)
{
    int indexX = GetGridIndexX(_worldX);
    int indexZ = GetGridIndexZ(_worldZ);
    return GetCell(indexX, indexZ, _team);
}


// *** GetCell
EntityGridCell *EntityGrid::GetCell(int _indexX, int _indexZ, int _team)
{
    DarwiniaDebugAssert(_indexX >= 0 && _indexX < m_numCellsX);
    DarwiniaDebugAssert(_indexZ >= 0 && _indexZ < m_numCellsZ);

    return &m_cells[_team][_indexZ * m_numCellsX + _indexX];
}


// *** AddObject
// WorldX and WorldY MUST BE VALID
// ObjectID MUST NOT already exist within the grid cell
void EntityGrid::AddObject (WorldObjectId _objectID, float _worldX, float _worldZ, float _radius )
{    
    int teamId = _objectID.GetTeamId();
    if( teamId == 255 ) return;

    if( _radius == 0.0f )
    {
        EntityGridCell *gridCell = GetCell(_worldX, _worldZ, teamId );
        gridCell->AddObjectId( _objectID );
    }
    else
    {
        int leftMostCell    = GetGridIndexX(_worldX - _radius/2);
        int rightMostCell   = GetGridIndexX(_worldX + _radius/2);
        int upMostCell      = GetGridIndexZ(_worldZ - _radius/2);
        int downMostCell    = GetGridIndexZ(_worldZ + _radius/2);

        leftMostCell    = max(0, leftMostCell);
        rightMostCell   = min(m_numCellsX - 1, rightMostCell);
        upMostCell      = max(0, upMostCell);
        downMostCell    = min(m_numCellsZ - 1, downMostCell);
        
        for( int x = leftMostCell; x <= rightMostCell; ++x )
        {
            for( int z = upMostCell; z <= downMostCell; ++z )
            {
                EntityGridCell *ogc = GetCell(x, z, teamId);
                ogc->AddObjectId( _objectID );
            }
        }
    }
}


// *** RemoveObject
// WorldX and WorldY MUST BE VALID
// ObjectID MUST already exist within the grid cell
void EntityGrid::RemoveObject (WorldObjectId _objectID, float _worldX, float _worldZ, float _radius )
{
    int teamId = _objectID.GetTeamId();
    if( teamId == 255 ) return;

    if( _radius == 0.0f )
    {
        EntityGridCell *gridCell = GetCell(_worldX, _worldZ, teamId);
        bool success = gridCell->RemoveObjectId( _objectID );
        if( !success ) LogEntityGridError( _objectID, Vector3(_worldX, 0.0f, _worldZ), 2 );
    }
    else
    {
        int leftMostCell    = GetGridIndexX(_worldX - _radius/2);
        int rightMostCell   = GetGridIndexX(_worldX + _radius/2);
        int upMostCell      = GetGridIndexZ(_worldZ - _radius/2);
        int downMostCell    = GetGridIndexZ(_worldZ + _radius/2);

        leftMostCell    = max(0, leftMostCell);
        rightMostCell   = min(m_numCellsX - 1, rightMostCell);
        upMostCell      = max(0, upMostCell);
        downMostCell    = min(m_numCellsZ - 1, downMostCell);
        
        for( int x = leftMostCell; x <= rightMostCell; ++x )
        {
            for( int z = upMostCell; z <= downMostCell; ++z )
            {
                EntityGridCell *ogc = GetCell(x, z, teamId);
                bool success = ogc->RemoveObjectId( _objectID );
                if( !success ) LogEntityGridError( _objectID, Vector3(x*m_cellSizeX, 0.0f, z*m_cellSizeZ), 2 );
            }
        }
    }
}


// *** UpdateObject
void EntityGrid::UpdateObject (WorldObjectId _objectId, float _oldWorldX, float _oldWorldZ,
                                                   float _newWorldX, float _newWorldZ,
                                                   float _radius )
{
    int oldIndexX = GetGridIndexX(_oldWorldX);
    int oldIndexZ = GetGridIndexZ(_oldWorldZ);

    int newIndexX = GetGridIndexX(_newWorldX);
    int newIndexZ = GetGridIndexZ(_newWorldZ);

    if ( oldIndexX != newIndexX || 
         oldIndexZ != newIndexZ ||
         _radius > 0.0f )
    {
        RemoveObject( _objectId, _oldWorldX, _oldWorldZ, _radius);
        AddObject(_objectId, _newWorldX, _newWorldZ, _radius);
    }
}


// *** GetEnemies
WorldObjectId *EntityGrid::GetEnemies(float _worldX, float _worldZ, float _range,
                                      int *_numFound, unsigned char _myTeam)
{
    bool include[NUM_TEAMS];

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        include[i] = !g_app->m_location->IsFriend(i, _myTeam);
    }

    return GetNeighbours( _worldX, _worldZ, _range, _numFound, include );
}


// *** GetBestEnemy
// Returns the nearest enemy with between the _minRange and _maxRange.
// Returns an "invalid" WorldObjectId if no enemy is within that range.
WorldObjectId EntityGrid::GetBestEnemy(float _worldX, float _worldZ, 
								   float _minRange, float _maxRange, unsigned char _myTeam)
{
    int numFound;
    WorldObjectId *ids = GetEnemies( _worldX, _worldZ, _maxRange, &numFound, _myTeam );

    WorldObjectId targetId;
    float bestDistanceSqrd = FLT_MAX;
    float minRangeSqrd = _minRange * _minRange;

    for( int i = 0; i < numFound; ++i )
    {
        WorldObjectId id = ids[i];
        Entity *entity = g_app->m_location->GetEntity( id );
        float deltaX = entity->m_pos.x - _worldX;
        float deltaZ = entity->m_pos.z - _worldZ;
		float distanceSqrd = deltaX * deltaX + deltaZ * deltaZ;
        if( distanceSqrd < bestDistanceSqrd &&
            distanceSqrd >= minRangeSqrd &&
            !entity->m_dead )
        {
            bestDistanceSqrd = distanceSqrd;
            targetId = id;
        }
    }

	return targetId;
}


// *** GetFriends
WorldObjectId *EntityGrid::GetFriends(float _worldX, float _worldZ, float _range,
                                      int *_numFound, unsigned char _myTeam)
{
    bool include[NUM_TEAMS];

    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        include[i] = g_app->m_location->IsFriend(i, _myTeam);
    }

    return GetNeighbours( _worldX, _worldZ, _range, _numFound, include );
}


// *** GetNeighbours
WorldObjectId *EntityGrid::GetNeighbours(float _worldX, float _worldZ, float _range, 
										 int *_numNeighbours)
{
    bool include[NUM_TEAMS];
    memset( include, true, NUM_TEAMS * sizeof(bool) );

    return GetNeighbours( _worldX, _worldZ, _range, _numNeighbours, include );
}


// *** GetNeighbours
WorldObjectId *EntityGrid::GetNeighbours(float _worldX, float _worldZ, float _range, 
            							 int *_numFound, bool _includeTeam[NUM_TEAMS] )
{
    int numFoundSoFar = 0;
    
    // Find out which cells to look it
    int leftMostCell = GetGridIndexX(_worldX - _range);
    int rightMostCell = GetGridIndexX(_worldX + _range);
    int upMostCell = GetGridIndexZ(_worldZ - _range);
    int downMostCell = GetGridIndexZ(_worldZ + _range);

    leftMostCell = max(0, leftMostCell);
    rightMostCell = min(m_numCellsX - 1, rightMostCell);
    upMostCell = max(0, upMostCell);
    downMostCell = min(m_numCellsZ - 1, downMostCell);

    float rangeSqrd = _range * _range;

    // For each cell
    for (int x = leftMostCell; x <= rightMostCell; x++)
    {
        for (int z = upMostCell; z <= downMostCell; z++)
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( _includeTeam[t] )
                {
                    EntityGridCell *ogc = GetCell(x, z, t);
            
                    for (int i = 0; i < ogc->m_arraySize; i++)
                    {
                        WorldObjectId const &objId = ogc->m_objectIds[i];

                        if (!objId.IsValid()) continue;

                        // Is this ID already added?  
                        // (some entities occupy more than one entity grid square)
                        bool added = false;
                        for( int j = 0; j < numFoundSoFar; ++j )
                        {
                            if( m_neighbours[j] == objId )
                            {
                                added = true;
                                break;
                            }
                        }
                        if( added ) continue;

                        Entity *obj = g_app->m_location->GetEntity( objId );
                        if( !obj ) 
                        {
                            LogEntityGridError( objId, Vector3(x*m_cellSizeX, 0.0f, z*m_cellSizeZ), 1 );                
                            continue;
                        }

                        float deltaX = obj->m_pos.x - _worldX;
                        float deltaZ = obj->m_pos.z - _worldZ;
                        float distSqrd = deltaX * deltaX + deltaZ * deltaZ;

                        if (distSqrd < rangeSqrd)
                        {
                            //DarwiniaReleaseAssert(numFoundSoFar < MAX_NUM_NEIGHBOURS, "Too many entities in one area" );
                            EnsureMaxNeighbours( numFoundSoFar+1 );
                            m_neighbours[numFoundSoFar] = objId;
                            numFoundSoFar++;
                        }
                    }
                }
            }
        }
    }

    *_numFound = numFoundSoFar;
    return m_neighbours;
}


int EntityGrid::GetNumNeighbours(float _worldX, float _worldZ, float _range, bool _includeTeam[NUM_TEAMS] )
{
    // Find out which cells to look it
    int leftMostCell = GetGridIndexX(_worldX - _range);
    int rightMostCell = GetGridIndexX(_worldX + _range);
    int upMostCell = GetGridIndexZ(_worldZ - _range);
    int downMostCell = GetGridIndexZ(_worldZ + _range);

    leftMostCell = max(0, leftMostCell);
    rightMostCell = min(m_numCellsX - 1, rightMostCell);
    upMostCell = max(0, upMostCell);
    downMostCell = min(m_numCellsZ - 1, downMostCell);

    float rangeSqrd = _range * _range;

    int numFoundSoFar = 0;

    // For each cell
    for (int x = leftMostCell; x <= rightMostCell; x++)
    {
        for (int z = upMostCell; z <= downMostCell; z++)
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( _includeTeam[t] )
                {
                    EntityGridCell *ogc = GetCell(x, z, t);
            
                    for (int i = 0; i < ogc->m_arraySize; i++)
                    {
                        WorldObjectId const &objId = ogc->m_objectIds[i];
                
                        if (!objId.IsValid()) continue;

                        // Is this ID already added?  
                        // (some entities occupy more than one entity grid square)
                        bool added = false;
                        for( int j = 0; j < numFoundSoFar; ++j )
                        {
                            if( m_neighbours[j] == objId )
                            {
                                added = true;
                                break;
                            }
                        }
                        if( added ) continue;

                        EnsureMaxNeighbours( numFoundSoFar+1 );
                        m_neighbours[numFoundSoFar] = objId;
                        numFoundSoFar++;
                    }
                }
            }
        }
    }

    return numFoundSoFar;
}


int EntityGrid::GetNumFriends(float _worldX, float _worldZ, float _range, unsigned char _myTeam )
{
    bool includeTeam[NUM_TEAMS];
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        includeTeam[i] = g_app->m_location->IsFriend( i, _myTeam );        
    }

    return GetNumNeighbours( _worldX, _worldZ, _range, includeTeam );
}


int EntityGrid::GetNumEnemies(float _worldX, float _worldZ, float _range, unsigned char _myTeam )
{
    bool includeTeam[NUM_TEAMS];
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        includeTeam[i] = !g_app->m_location->IsFriend( i, _myTeam );        
    }

    return GetNumNeighbours( _worldX, _worldZ, _range, includeTeam );
}


bool EntityGrid::AreNeighboursPresent(float _worldX, float _worldZ, float _range, bool _includeTeam[NUM_TEAMS] )
{
    // Find out which cells to look it
    int leftMostCell = GetGridIndexX(_worldX - _range);
    int rightMostCell = GetGridIndexX(_worldX + _range);
    int upMostCell = GetGridIndexZ(_worldZ - _range);
    int downMostCell = GetGridIndexZ(_worldZ + _range);

    leftMostCell = max(0, leftMostCell);
    rightMostCell = min(m_numCellsX - 1, rightMostCell);
    upMostCell = max(0, upMostCell);
    downMostCell = min(m_numCellsZ - 1, downMostCell);

    float rangeSqrd = _range * _range;

    int numFoundSoFar = 0;

    // For each cell
    for (int x = leftMostCell; x <= rightMostCell; x++)
    {
        for (int z = upMostCell; z <= downMostCell; z++)
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                if( _includeTeam[t] )
                {
                    EntityGridCell *ogc = GetCell(x, z, t);
                    if( ogc->m_arraySize > 0 ) return true;
                }
            }
        }
    }

    return false;
}


bool EntityGrid::AreEnemiesPresent(float _worldX, float _worldZ, float _range, unsigned char _myTeam )
{
    bool includeTeam[NUM_TEAMS];
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        includeTeam[i] = !g_app->m_location->IsFriend( i, _myTeam );        
    }

    return AreNeighboursPresent( _worldX, _worldZ, _range, includeTeam );
}


bool EntityGrid::AreFriendsPresent(float _worldX, float _worldZ, float _range, unsigned char _myTeam )
{
    bool includeTeam[NUM_TEAMS];
    for( int i = 0; i < NUM_TEAMS; ++i )
    {
        includeTeam[i] = g_app->m_location->IsFriend( i, _myTeam );        
    }

    return AreNeighboursPresent( _worldX, _worldZ, _range, includeTeam );
}



#if 1
void EntityGrid::Render ()
{

    int x, z;

    float cellSizeX = g_app->m_location->m_landscape.GetWorldSizeX() / (float) m_numCellsX;
    float cellSizeZ = g_app->m_location->m_landscape.GetWorldSizeZ() / (float) m_numCellsZ;
 
    glDisable( GL_CULL_FACE );
    glEnable( GL_BLEND );

    for ( x = 0; x < m_numCellsX; ++x ) 
    {
        float worldX = ((float) x / (float) m_numCellsX) * g_app->m_location->m_landscape.GetWorldSizeX();
        for ( z = 0; z < m_numCellsZ; ++z )
        {
            for( int t = 0; t < NUM_TEAMS; ++t )
            {
                EntityGridCell *gridCell = GetCell( x, z, t );
                int numEntities = gridCell->m_arraySize - gridCell->m_numSlotsFree;

                if (numEntities > 0)
                {
                     float worldZ = ((float) z / (float) m_numCellsZ) * g_app->m_location->m_landscape.GetWorldSizeZ();
                     float worldY = g_app->m_location->m_landscape.m_heightMap->GetValue( worldX, worldZ ) + 10.0f;
                     worldY = 100.0f + t * 30.0f;
                
                     float alpha = 128;
                     RGBAColour col = g_app->m_location->m_teams[t].m_colour;
                     glColor4ub(col.r, col.g, col.b, alpha);

                     glBegin(GL_QUADS);
                        glVertex3f( worldX, worldY, worldZ );
                        glVertex3f( worldX + cellSizeX, worldY, worldZ );
                        glVertex3f( worldX + cellSizeX, worldY, worldZ + cellSizeZ );
                        glVertex3f( worldX, worldY, worldZ + cellSizeZ );
                     glEnd();

                     g_editorFont.DrawText3DCentre( Vector3(worldX,worldY,worldZ), 5.0f, "%d", numEntities );
                }
            }
        }
    }

    glDisable( GL_BLEND );
    glEnable( GL_CULL_FACE );

#ifdef DEBUG_ENTITY_GRID
    if( s_entityGridErrors.Size() > 0 )
    {
        for( int i = 0; i < s_entityGridErrors.Size(); ++i )
        {
            EntityGridError *theError = s_entityGridErrors[i];
            Vector3 pos = theError->m_pos;
            glColor4f( 1.0f, 0.0f, 0.0f, 1.0f );
            glBegin( GL_LINES );
                glVertex3fv( (pos - Vector3(0,500,0)).GetData() );
                glVertex3fv( (pos + Vector3(0,500,0)).GetData() );
            glEnd();

            int index = theError->m_id.GetIndex();
            int uniqueIndex = theError->m_id.GetUniqueId();
            int errorCode = theError->m_errorCode;
            float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue( pos.x, pos.z );
            Vector3 thisPos = pos;
            thisPos.y = landHeight;
            thisPos.y += uniqueIndex;
            g_editorFont.DrawText3D( thisPos, 5, "id %d, uniqueID %d, errCode %d", index, uniqueIndex, errorCode );
        }

        g_editorFont.BeginText2D();
        g_editorFont.DrawText2D( 10, 100, 20, "Entity Grid Errors : %d", s_entityGridErrors.Size() );
        g_editorFont.EndText2D();
    }

    static float lastCheck = 0;
    if( GetHighResTime() > lastCheck + 1.0f )
    {
        for( int x = 0; x < m_numCellsX; ++x )
        {
            for( int z = 0; z < m_numCellsZ; ++z )
            {
                EntityGridCell *ogc = GetCell(x, z);
            
                for (int i = 0; i < ogc->m_arraySize; i++)
                {
                    WorldObjectId const &objId = ogc->m_objectIds[i];				    
                    if (!objId.IsValid()) continue;

                    Entity *obj = g_app->m_location->GetEntity( objId );
                    if( !obj ) 
                    {
                        LogEntityGridError( objId, Vector3(x*m_cellSizeX, 0.0f, z*m_cellSizeZ), 1 );                                        
                    }
                }
            }
        }

        lastCheck = GetHighResTime();
    }
#endif

}
#endif
