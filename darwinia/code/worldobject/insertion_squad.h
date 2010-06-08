#ifndef INCLUDED_INSERTION_SQUAD
#define INCLUDED_INSERTION_SQUAD


#include "unit.h"


#define GAP_BETWEEN_MEN	10.0f


//*****************************************************************************
// Class HistoricWayPoint
// 
// Stores positions where the user clicked.
//*****************************************************************************

class HistoricWayPoint
{
public:
	Vector3				m_pos;
	unsigned int		m_id;
	static unsigned int	s_lastId;

	HistoricWayPoint(Vector3 const &_pos)
	:	m_pos(_pos)
	{
		s_lastId++;
		m_id = s_lastId;
	}
};


//*****************************************************************************
// Class InsertionSquad
//*****************************************************************************

class InsertionSquad: public Unit
{
protected:
    LList <HistoricWayPoint *>	m_positionHistory;          	// A list of all the places the user has clicked. Most recent first
   
public:
    int     m_weaponType;                                       // Indexes into GlobalResearch
    int     m_controllerId;                                     // Task ID of controller if this squad is running one
    int     m_teleportId;                                       // Id of teleport build we wish to enter, or -1

public:
    InsertionSquad				    (int teamId, int _unitId, int numEntities, Vector3 const &_pos);
	~InsertionSquad				    ();

	virtual void    SetWayPoint	    (Vector3 const &_pos);
	Vector3         GetTargetPos    (float _distFromPointMan);
	Entity          *GetPointMan	();
    void            SetWeaponType   ( int _weaponType );                    // Indexes into GlobalResearch
    void            Attack          ( Vector3 pos, bool withGrenade );

    void            DirectControl   ( TeamControls const& _teamControls ); // used when the squad is being directly controlled by the player using a control pad
};


//*****************************************************************************
// Class Squadie
//*****************************************************************************

class Squadie: public Entity
{    
public:
    bool        m_justFired;
    float       m_secondaryTimer;    

protected:
    ShapeMarker *m_laser;
    ShapeMarker *m_brass;
    ShapeMarker *m_eye1;
    ShapeMarker *m_eye2;

protected:    
    WorldObjectId   m_enemyId;
    float           m_retargetTimer;
    void            RunAI();               // Call this if the player isnt' controlling us
    
public:
    Squadie();        
            
    void Begin                  ();
    bool Advance                ( Unit *_unit );    
    void ChangeHealth           ( int _amount );
    void Attack                 ( Vector3 const &_pos );

    void Render                 ( float _predictionTime );
    bool RenderPixelEffect      ( float _predictionTime );

    bool HasSecondaryWeapon     ();
    void FireSecondaryWeapon    ( Vector3 const &_pos );

    void ListSoundEvents	    ( LList<char *> *_list );

	Vector3 GetCameraFocusPoint	();

	Vector3 GetSecondaryWeaponTarget();
};



#endif
