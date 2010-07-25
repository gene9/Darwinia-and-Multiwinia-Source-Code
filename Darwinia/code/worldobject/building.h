#ifndef _included_building_h
#define _included_building_h

#include "lib/vector3.h"
#include "lib/matrix34.h"
#include "lib/llist.h"

#include "worldobject/entity.h"
#include "worldobject/worldobject.h"

#include "globals.h"

class Shape;
class ShapeFragment;
class ShapeMarker;
class TextReader;
class BuildingPort;
class FileWriter;


class Building: public WorldObject
{
public:
    enum
    {
        TypeInvalid,		// When you add an entry here remember to update building.cpp
        TypeFactory,		// 1
        TypeCave,			// 2
        TypeRadarDish,
        TypeLaserFence,
        TypeControlTower,
        TypeGunTurret,
        TypeBridge,
		TypePowerstation,
        TypeTree,
        TypeWall,
        TypeTrunkPort,
        TypeResearchItem,
        TypeLibrary,
        TypeGenerator,
        TypePylon,
        TypePylonStart,
        TypePylonEnd,
        TypeSolarPanel,
        TypeTrackLink,
        TypeTrackJunction,
        TypeTrackStart,
        TypeTrackEnd,
        TypeRefinery,
        TypeMine,
        TypeYard,
        TypeDisplayScreen,
		TypeUpgradePort,
        TypePrimaryUpgradePort,
        TypeIncubator,
        TypeAntHill,
        TypeSafeArea,
        TypeTriffid,
        TypeSpiritReceiver,
        TypeReceiverLink,
        TypeReceiverSpiritSpawner,
        TypeSpiritProcessor,
        TypeSpawnPoint,
        TypeSpawnPopulationLock,
        TypeSpawnPointMaster,
        TypeSpawnLink,
        TypeAITarget,
        TypeAISpawnPoint,
        TypeBlueprintStore,
        TypeBlueprintConsole,
        TypeBlueprintRelay,
        TypeScriptTrigger,
        TypeSpam,
        TypeGodDish,
        TypeStaticShape,
        TypeFuelGenerator,
        TypeFuelPipe,
        TypeFuelStation,
        TypeEscapeRocket,
        TypeFenceSwitch,
        TypeDynamicHub,
        TypeDynamicNode,
        TypeFeedingTube,
		TypeControlStation,
		NumBuildingTypes
    };

    Vector3     m_front;
    Vector3     m_up;
	float		m_timeOfDeath;
	bool		m_dynamic;								// Only appears on this level, not all levels for this map
	bool		m_isGlobal;
    Vector3     m_centrePos;
    float       m_radius;

	bool		m_destroyed;							// Building has been destroyed using the script command DestroyBuilding, remove it next Advance

    Shape		*m_shape;
    LList       <ShapeMarker *> m_lights;               // Ownership lights
    LList       <BuildingPort *> m_ports;               // Require Darwinians in them to operate

    static Shape        *s_controlPad;
    static ShapeMarker  *s_controlPadStatus;

public:
    Building();

    virtual void Initialise( Building *_template );
    virtual bool Advance();

    virtual void SetShape       ( Shape *_shape );
    void         SetShapeLights ( ShapeFragment *_fragment );   // Recursivly search for lights
    void         SetShapePorts  ( ShapeFragment *_fragment );

    virtual void SetDetail      ( int _detail );

    virtual bool IsInView       ();

    virtual void Render         ( float predictionTime );
    virtual void RenderAlphas   ( float predictionTime );
    virtual void RenderLights   ();
    virtual void RenderPorts    ();
	virtual void RenderHitCheck ();
    virtual void RenderLink     ();                             // ie link to another building

    virtual bool PerformDepthSort( Vector3 &_centrePos );       // Return true if you plan to use transparencies

    virtual void SetTeamId          ( int _teamId );
    virtual void Reprogram          ( float _complete );
    virtual void ReprogramComplete  ();

    virtual void Damage( float _damage );
	virtual void Destroy( float _intensity );

    Vector3 PushFromBuilding( Vector3 const &_pos, float _radius );

    virtual void            EvaluatePorts          ();
    virtual int             GetNumPorts            ();
    virtual int             GetNumPortsOccupied    ();
    virtual WorldObjectId   GetPortOccupant        ( int _portId );
    virtual bool            GetPortPosition        ( int _portId, Vector3 &_pos, Vector3 &_front );

    virtual void            OperatePort            ( int _portId, int _teamId );
    virtual int             GetPortOperatorCount   ( int _portId, int _teamId );

    virtual char            *GetObjectiveCounter   ();

    virtual bool DoesSphereHit          (Vector3 const &_pos, float _radius);
    virtual bool DoesShapeHit           (Shape *_shape, Matrix34 _transform);
    virtual bool DoesRayHit             (Vector3 const &_rayStart, Vector3 const &_rayDir,
                                        float _rayLen=1e10, Vector3 *_pos=NULL, Vector3 *_norm=NULL);        // pos/norm will not always be available

    virtual void ListSoundEvents        ( LList<char *> *_list );

    virtual void Read   ( TextReader *_in, bool _dynamic );     // Use these to read/write additional building-specific
    virtual void Write  ( FileWriter *_out );					// data to the level files

    virtual int  GetBuildingLink();                             // Allows a building to link to another
    virtual void SetBuildingLink( int _buildingId );            // eg control towers

    static char *GetTypeName    ( int _type );
    static int   GetTypeId	    ( char const *_name );
    static Building *CreateBuilding ( int _type );
    static Building *CreateBuilding ( char *_name );

    static char *GetTypeNameTranslated( int _type );
};



class BuildingPort
{
public:
    ShapeMarker     *m_marker;
    WorldObjectId   m_occupant;
    Matrix34        m_mat;
    int             m_counter[NUM_TEAMS];
};

#endif
