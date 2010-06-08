#ifndef _INCLUDED_TEAM_H
#define _INCLUDED_TEAM_H

#include "lib/fast_darray.h"
#include "lib/slice_darray.h"
#include "lib/rgb_colour.h"

#include "worldobject/worldobject.h"
#include "worldobject/entity.h"

class Unit;
class InsertionSquad;


// ****************************************************************************
//  Class Team
// ****************************************************************************

class Team
{
public:
    enum
    {
        TeamTypeUnused = -1,
        TeamTypeLocalPlayer,
        TeamTypeRemotePlayer,
        TeamTypeCPU
    };

    int                         m_teamId;
    int                         m_teamType;

    FastDArray  <Unit *>        m_units;
    SliceDArray <Entity *>      m_others;
    LList       <WorldObjectId> m_specials;             // Officers and tanks for quick lookup

    RGBAColour               m_colour;

    int             m_currentUnitId;                    //
    int             m_currentEntityId;                  // Do not set these directly
    int             m_currentBuildingId;                // They are updated by the network
                                                        //
    Vector3         m_currentMousePos;                  //

public:
    Team();

    void Initialise     (int _teamId);                  // Call when this team enters the game
    void SetTeamType    (int _teamType);

    void SelectUnit     (int _unitId, int _entityId, int _buildingId );

    void RegisterSpecial    ( WorldObjectId _id );
    void UnRegisterSpecial  ( WorldObjectId _id );

	Entity *RayHitEntity(Vector3 const &_rayStart, Vector3 const &_rayEnd);
    Unit   *GetMyUnit   ();
    Entity *GetMyEntity ();
    Unit   *NewUnit     (int _troopType, int _numEntities, int *_unitId, Vector3 const &_pos);
    Entity *NewEntity   (int _troopType, int _unitId, int *_index);

    int  NumEntities    (int _troopType);               // Counts the total number

    void Advance        (int _slice);

    void Render             ();
    void RenderVirii        (float _predictionTime);
    void RenderDarwinians   (float _predictionTime);
    void RenderOthers       (float _predictionTime);
};


// ****************************************************************************
//  Class TeamControls
//
//   capture all the control information necessary to send
//   over the network for "remote" control of units
// ****************************************************************************

class TeamControls {
public:
	TeamControls();

	unsigned short GetFlags() const;
	void SetFlags( unsigned short _flags );
	void ClearFlags();
	void Advance();
	void Clear();

public:

	Vector3			m_mousePos;

	// Be sure to update GetFlags, SetFlags, ZeroFlags if you change these flags
	// Also, NetworkUpdate::GetByteStream and NetworkUpdate::ReadByteStream
	// if you use more than 8 bits

	unsigned int	m_unitMove : 1;
	unsigned int	m_primaryFireTarget : 1;
	unsigned int	m_secondaryFireTarget : 1;
	unsigned int	m_primaryFireDirected : 1;
	unsigned int	m_secondaryFireDirected : 1;
	unsigned int	m_cameraEntityTracking : 1;
	unsigned int	m_directUnitMove : 1;
	unsigned int	m_unitSecondaryMode : 1;
	unsigned int	m_endSetTarget : 1;

	int				m_directUnitMoveDx;
	int				m_directUnitMoveDy;
    int             m_directUnitFireDx;
    int             m_directUnitFireDy;
};

#endif
